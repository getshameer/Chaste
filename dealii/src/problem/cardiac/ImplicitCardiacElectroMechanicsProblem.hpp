#ifndef IMPLICITCARDIACELECTROMECHANICSPROBLEM_HPP_
#define IMPLICITCARDIACELECTROMECHANICSPROBLEM_HPP_

#include "MonodomainProblem.hpp"
#include "ImplicitOneDimCardiacMechanicsAssembler.hpp"
#include "EulerIvpOdeSolver.hpp"
#include "FiniteElasticityTools.hpp"
#include "NhsCellularMechanicsOdeSystem.hpp"


#include "CardiacElectroMechanicsProblem.hpp" // for the ElementsAndWeights class


// todos: - see CardiacElectroMechanicsProblem
 


template<unsigned DIM>
class ImplicitCardiacElectroMechanicsProblem
{
private: 
    /*< The cardiac problem class */
    MonodomainProblem<DIM>* mpMonodomainProblem;
    
    /*< The mechanics assembler */
    ImplicitOneDimCardiacMechanicsAssembler* mpCardiacMechAssembler;

    double mEndTime;
    double mTimeStep;    
    
    /*< A chaste mesh for the electrics */
    ConformingTetrahedralMesh<DIM,DIM>* mpElectricsMesh;
    /*<  A dealii mesh for the mechanics */
    Triangulation<DIM>*                 mpMechanicsMesh;

    /** 
     *  The (electrics-mesh) element numbers and saying which element each 
     *  (mechanics-mesh) gauss point is in, and the weight of that gauss point 
     *  for that particular element is.
     */
    std::vector<ElementAndWeights<DIM> > mElementAndWeightsForQuadPoints;

    std::string mOutputDirectory;
    bool mWriteOutput;
    /*< when to write output */    
    const static int WRITE_EVERY_NTH_TIME = 1; 
        
public:
    /** 
     *  Constructor
     *  @param pCellFactory cell factory for creating cells (see Monodomain tests)
     *  @endTime end time of the simulation. Start time is assumed to be 0.0
     *  @timeStep time step for the electrics (and currently the mechanics too)
     *  @outputDirectory. Output directory. Omit if no output is required.
     * 
     *  The meshes are currently hardcoded in here.
     */
    ImplicitCardiacElectroMechanicsProblem(AbstractCardiacCellFactory<DIM>* pCellFactory,
                                           double endTime,
                                           double timeStep,
                                           std::string outputDirectory = "")
    {
        assert(pCellFactory != NULL);
        mpMonodomainProblem = new MonodomainProblem<DIM>(pCellFactory);
        
        assert(endTime > 0);
        mEndTime = endTime;
        mTimeStep = timeStep;
        
        // check whether output is required
        mWriteOutput = (outputDirectory!="");
        mOutputDirectory = outputDirectory;
                
        // create electrics mesh
        mpElectricsMesh = new ConformingTetrahedralMesh<DIM,DIM>();
        unsigned num_elem = 128;
        mpElectricsMesh->ConstructLinearMesh(num_elem);
        mpElectricsMesh->Scale(1.0/num_elem);
        
        assert(DIM==1);
        mpMonodomainProblem->SetMesh(mpElectricsMesh);
        mpMonodomainProblem->Initialise();
        
        // create mechanics mesh
        mpMechanicsMesh = new Triangulation<DIM>();
        GridGenerator::hyper_cube(*mpMechanicsMesh, 0.0, 1);
        mpMechanicsMesh->refine_global(7);
        
        assert(mpMechanicsMesh->n_vertices()==mpElectricsMesh->GetNumNodes());

        mpCardiacMechAssembler = new ImplicitOneDimCardiacMechanicsAssembler(mpMechanicsMesh);
        
        // find the element nums and weights for each gauss point in the mechanics mesh
        mElementAndWeightsForQuadPoints.resize(mpCardiacMechAssembler->GetTotalNumQuadPoints());
        std::vector<std::vector<double> > quad_point_posns
           = FiniteElasticityTools<DIM>::GetQuadPointPositions(*mpMechanicsMesh, mpCardiacMechAssembler->GetNumQuadPointsInEachDimension());
        
        for(unsigned i=0; i<quad_point_posns.size(); i++)
        {
            assert(DIM==1);
            ChastePoint<DIM> point(quad_point_posns[i][0]);
            
            unsigned elem_index = mpElectricsMesh->GetContainingElementIndex(point);
            c_vector<double,DIM+1> weight = mpElectricsMesh->GetElement(elem_index)->CalculateInterpolationWeights(point);
            
            mElementAndWeightsForQuadPoints[i].ElementNum = elem_index;
            mElementAndWeightsForQuadPoints[i].Weights = weight;
        }
    }
    

    /** 
     *  Solve the electromechanincs problem
     */    
    void Solve()
    {
        // get an electrics assembler from the problem. Note that we don't call
        // Solve() on the CardiacProblem class, we do the looping here.
        AbstractDynamicAssemblerMixin<DIM,DIM,1>* mpElectricsAssembler 
           = mpMonodomainProblem->CreateAssembler();

        // set up initial voltage etc
        Vec voltage;        
        Vec initial_voltage = mpMonodomainProblem->CreateInitialCondition();

        // create stores of lambda, lambda_dot and old lambda
        unsigned num_quad_points = mpCardiacMechAssembler->GetTotalNumQuadPoints();
        
        std::vector<double> calcium_conc(num_quad_points);

        unsigned mech_writer_counter = 0;

        if(mWriteOutput)
        {
            OutputFileHandler output_file_handler(mOutputDirectory, true);
            out_stream p_file = output_file_handler.OpenOutputFile("results_", mech_writer_counter, ".dat");
            std::vector<Vector<double> >& deformed_position = mpCardiacMechAssembler->rGetDeformedPosition();
            for(unsigned i=0; i<deformed_position[0].size(); i++)
            {
                assert(DIM==1);
                (*p_file) << deformed_position[0](i) << "\n";
            }
        }


        unsigned counter = 0;

        TimeStepper stepper(0.0, mEndTime, mTimeStep);
        while ( !stepper.IsTimeAtEnd() )
        {
            std::cout << "**Time = " << stepper.GetTime() << "\n" << std::flush;
            
            // solve the electrics
            mpElectricsAssembler->SetTimes(stepper.GetTime(), stepper.GetNextTime(), mTimeStep);
            mpElectricsAssembler->SetInitialCondition( initial_voltage );
            voltage = mpElectricsAssembler->Solve();

            VecDestroy(initial_voltage);
            initial_voltage = voltage;
         
            // compute Ca_I at each quad point (by interpolation, using the info on which
            // electrics element the quad point is in, and set on the nhs systems
            for(unsigned i=0; i<mElementAndWeightsForQuadPoints.size(); i++)
            {
                double interpolated_Ca_I = 0;

                Element<DIM,DIM>& element = *(mpElectricsMesh->GetElement(mElementAndWeightsForQuadPoints[i].ElementNum));
                for(unsigned node_index = 0; node_index<element.GetNumNodes(); node_index++)
                {
                    unsigned global_node_index = element.GetNodeGlobalIndex(node_index);
                    double Ca_I_at_node = mpMonodomainProblem->GetPde()->GetCardiacCell(global_node_index)->GetIntracellularCalciumConcentration();
                    interpolated_Ca_I += Ca_I_at_node*mElementAndWeightsForQuadPoints[i].Weights(node_index);
                }

                calcium_conc[i] = interpolated_Ca_I;
            }
            mpCardiacMechAssembler->SetIntracellularCalciumConcentration(calcium_conc);
            
            // NOTE: HERE WE SHOULD REALLY CHECK WHETHER THE CELL MODELS HAVE Ca_Trop
            // AND UPDATE FROM NHS TO CELL_MODEL, BUT NOT SURE HOW TO DO THIS..
            

            // solve the mechanics
            mpCardiacMechAssembler->Solve(stepper.GetTime(), stepper.GetNextTime(), stepper.GetNextTime()-stepper.GetTime());

            if(mWriteOutput && (counter++)%WRITE_EVERY_NTH_TIME==0)
            {            
                OutputFileHandler output_file_handler(mOutputDirectory, false);
                out_stream p_file = output_file_handler.OpenOutputFile("results_", mech_writer_counter, ".dat");
                std::vector<Vector<double> >& deformed_position = mpCardiacMechAssembler->rGetDeformedPosition();
                for(unsigned i=0; i<deformed_position[0].size(); i++)
                {
                    assert(DIM==1);
                    (*p_file) << deformed_position[0](i) << "\n";
                }
                mech_writer_counter++;
            }                        
            // update the current time
            stepper.AdvanceOneTimeStep();
        }
        
        delete mpElectricsAssembler;
    }
};

#endif /*IMPLICITCARDIACELECTROMECHANICSPROBLEM_HPP_*/
