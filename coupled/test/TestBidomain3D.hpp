#ifndef TESTBIDOMAIN3D_HPP_
#define TESTBIDOMAIN3D_HPP_


// Element.hpp includes the Boost ublas objects - these need to
// be included early...  We think.  We're not that sure.
#include "Element.hpp"
#include <cxxtest/TestSuite.h>
#include <petscvec.h>
#include <vector>
//#include <iostream>
#include "PetscSetupAndFinalize.hpp"
#include "BidomainProblem.hpp"
#include "MonodomainProblem.hpp"
#include "AbstractCardiacCellFactory.hpp"
#include "LuoRudyIModel1991OdeSystem.hpp"


class BidomainFaceStimulusCellFactory : public AbstractCardiacCellFactory<3>
{
private:
    InitialStimulus *mpStimulus;
public:
    BidomainFaceStimulusCellFactory() : AbstractCardiacCellFactory<3>(0.01)
    {
        mpStimulus = new InitialStimulus(-600.0, 0.5);
    }
    
    AbstractCardiacCell* CreateCardiacCellForNode(int node)
    {
        if (mpMesh->GetNodeAt(node)->GetPoint()[0] == 0.0)
        {
            //std::cout << node+1 << "\n";
            return new LuoRudyIModel1991OdeSystem(mpSolver, mTimeStep, mpStimulus, mpZeroStimulus);
        }
        else
        {
            return new LuoRudyIModel1991OdeSystem(mpSolver, mTimeStep, mpZeroStimulus, mpZeroStimulus);
        }
    }
        
    ~BidomainFaceStimulusCellFactory(void)
    {
        delete mpStimulus;
    }
};

class TestBidomain3D :  public CxxTest::TestSuite 
{
public:
    void noTestBidomain3d()
    {
        BidomainFaceStimulusCellFactory bidomain_cell_factory;
        
        BidomainProblem<3> bidomain_problem( &bidomain_cell_factory );

        bidomain_problem.SetMeshFilename("mesh/test/data/3D_0_to_1mm_6000_elements");
        bidomain_problem.SetEndTime(1);   // 1 ms
        bidomain_problem.SetOutputDirectory("testoutput/Bidomain3d");
        bidomain_problem.SetOutputFilenamePrefix("bidomain3d");

        bidomain_problem.Initialise();

        bidomain_problem.Solve();
    }
    
    void TestCompareBidomainProblemWithMonodomain3D()
    {
        ///////////////////////////////////////////////////////////////////
        // monodomain
        ///////////////////////////////////////////////////////////////////
        BidomainFaceStimulusCellFactory cell_factory;
        MonodomainProblem<3> monodomain_problem( &cell_factory );
        
        monodomain_problem.SetMeshFilename("mesh/test/data/3D_0_to_1mm_6000_elements");
        monodomain_problem.SetEndTime(1);   // 1 ms
        monodomain_problem.SetOutputDirectory("testoutput/Monodomain3d");
        monodomain_problem.SetOutputFilenamePrefix("monodomain3d");
        
        monodomain_problem.Initialise();

        // set the intra conductivity
        c_matrix<double,3,3> sigma_i = 0.0005*identity_matrix<double>(3);
        monodomain_problem.GetMonodomainPde()->SetIntracellularConductivityTensor(sigma_i);
        
        // now solve       
        monodomain_problem.Solve();


        ///////////////////////////////////////////////////////////////////
        // bidomain
        ///////////////////////////////////////////////////////////////////
        BidomainProblem<3> bidomain_problem( &cell_factory );

        bidomain_problem.SetMeshFilename("mesh/test/data/3D_0_to_1mm_6000_elements");
        bidomain_problem.SetEndTime(1);   // 1 ms
        bidomain_problem.SetOutputDirectory("testoutput/Bidomain3d");
        bidomain_problem.SetOutputFilenamePrefix("bidomain3d");

        bidomain_problem.Initialise();
        
        // set the intra conductivity to be the same as monodomain
        // and the extra conductivity to be very large in comparison
        c_matrix<double,3,3> sigma_e = 1*identity_matrix<double>(3);
        bidomain_problem.GetBidomainPde()->SetIntracellularConductivityTensor(sigma_i);
        bidomain_problem.GetBidomainPde()->SetExtracellularConductivityTensor(sigma_e);
        
        // now solve
        bidomain_problem.Solve();
        
                
        ///////////////////////////////////////////////////////////////////
        // compare
        ///////////////////////////////////////////////////////////////////
        double* p_mono_voltage_array;
        double* p_bi_voltage_array;
        int bi_lo, bi_hi, mono_lo, mono_hi;

        bidomain_problem.GetVoltageArray(&p_bi_voltage_array, bi_lo, bi_hi); 
        monodomain_problem.GetVoltageArray(&p_mono_voltage_array, mono_lo, mono_hi); 
        
        for (int global_index=mono_lo; global_index<mono_hi; global_index++)
        {
            int local_index = global_index - mono_lo;
            
            double monodomain_voltage      =   p_mono_voltage_array[local_index];
            double   bidomain_voltage      =   p_bi_voltage_array  [2*local_index];
            double extracellular_potential =   p_bi_voltage_array  [2*local_index+1];
            // std::cout << p_mono_voltage_array[local_index] << " " << p_bi_voltage_array[2*local_index] << "\n";

            // the mono and bidomains should agree closely 
            TS_ASSERT_DELTA(monodomain_voltage, bidomain_voltage, 0.1);
            
            // the extracellular potential should be uniform 
            TS_ASSERT_DELTA(extracellular_potential, 0, 0.05);
        } 
    }
    
};


#endif /*TESTBIDOMAIN3D_HPP_*/
