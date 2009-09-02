/*

Copyright (C) University of Oxford, 2005-2009

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Chaste is free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Chaste is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details. The offer of Chaste under the terms of the
License is subject to the License being interpreted in accordance with
English Law and subject to any action against the University of Oxford
being under the jurisdiction of the English Courts.

You should have received a copy of the GNU Lesser General Public License
along with Chaste. If not, see <http://www.gnu.org/licenses/>.

*/


#ifndef ABSTRACTCARDIACPROBLEM_HPP_
#define ABSTRACTCARDIACPROBLEM_HPP_

#include <climits> // Work around a boost bug - see #1024.
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include "ClassIsAbstract.hpp"

#include <string>
#include <vector>

#include "AbstractCardiacCellFactory.hpp"
#include "AbstractCardiacPde.hpp"
#include "AbstractDynamicAssemblerMixin.hpp"
#include "AbstractTetrahedralMesh.hpp"

#include "BoundaryConditionsContainer.hpp"
#include "Hdf5DataReader.hpp"
#include "Hdf5DataWriter.hpp"

/**
 * Base class for cardiac problems; contains code generic to both mono- and bidomain.
 *
 * See tutorials for usage.
 */
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
class AbstractCardiacProblem
{
friend class TestBidomainWithBathAssembler;

private:
    /** Needed for serialization. */
    friend class boost::serialization::access;
    
    /**
     * Save the member variables.
     *
     * @param archive
     * @param version
     */
    template<class Archive>
    void save(Archive & archive, const unsigned int version) const
    {
        archive & mMeshFilename;
        archive & mpMesh;
        //archive & mAllocatedMemoryForMesh; // Will always be true after a load
        archive & mNodesPerProcessorFilename;
        archive & mUseMatrixBasedRhsAssembly;
        archive & mWriteInfo;
        archive & mPrintOutput;
        archive & mCallChaste2Meshalyzer;
        archive & mNodesToOutput;
        //archive & mVoltageColumnId; // Created by InitialiseWriter, called from Solve
        //archive & mExtraVariablesId; // Created by InitialiseWriter, called from Solve
        //archive & mTimeColumnId; // Created by InitialiseWriter, called from Solve
        //archive & mNodeColumnId; // Created by InitialiseWriter, called from Solve
        //archive & mpWriter; // Created by InitialiseWriter, called from Solve
        archive & mpCardiacPde;
        //archive & mpAssembler; // Only exists during calls to the Solve method
        bool has_solution = (mSolution != NULL);
        archive & has_solution;
        if (has_solution)
        {
            // TODO: Save using PetscTools:DumpPetscObject
//            std::string filename = ArchiveLocationInfo::GetArchiveDirectory() + "AbstractCardiacProblem_mSolution.vec";
//            PetscTools::DumpPetscObject(mSolution, filename);
        }
        archive & mCurrentTime;
        archive & mArchiveKSP;
        
        // Save boundary conditions
        SaveBoundaryConditions(archive, mpMesh, mpBoundaryConditionsContainer);
        SaveBoundaryConditions(archive, mpMesh, mpDefaultBoundaryConditionsContainer);
    }
    
    /**
     * Load the member variables.
     * 
     * @param archive
     * @param version
     */
    template<class Archive>
    void load(Archive & archive, const unsigned int version)
    {
        archive & mMeshFilename;
        archive & mpMesh;
        //archive & mAllocatedMemoryForMesh; // Will always be true after a load
        archive & mNodesPerProcessorFilename;
        archive & mUseMatrixBasedRhsAssembly;
        archive & mWriteInfo;
        archive & mPrintOutput;
        archive & mCallChaste2Meshalyzer;
        archive & mNodesToOutput;
        // TODO: think about the writer
        archive & mpCardiacPde;
        //archive & mpAssembler; // Only exists during calls to the Solve method
        bool has_solution;
        archive & has_solution;
        if (has_solution)
        {
            // TODO: Load using PetscTools::ReadPetscObject
//            std::string filename = ArchiveLocationInfo::GetArchiveDirectory() + "AbstractCardiacProblem_mSolution.vec";
//            PetscTools::ReadPetscObject(mSolution, filename);
        }
        archive & mCurrentTime;
        archive & mArchiveKSP;
        
        // Load boundary conditions
        LoadBoundaryConditions(archive, mpMesh, mpBoundaryConditionsContainer);
        LoadBoundaryConditions(archive, mpMesh, mpDefaultBoundaryConditionsContainer);
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    
    template<class Archive>
    void SaveBoundaryConditions(Archive & archive,
                                AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh,
                                BoundaryConditionsContainer<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM>* pBcc) const
    {
        bool have_object = (pBcc != NULL);
        archive & have_object;
        if (have_object)
        {
            pBcc->SaveToArchive(archive);
        }
    }
    
    template<class Archive>
    void LoadBoundaryConditions(Archive & archive,
                                AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh,
                                BoundaryConditionsContainer<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM>* pBcc)
    {
        bool have_object;
        archive & have_object;
        if (have_object)
        {
            /// \todo memory leak
            pBcc = new BoundaryConditionsContainer<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM>;
            pBcc->LoadFromArchive(archive, pMesh);
        }
    }

protected:
    /** Meshes can be read from file or instantiated and passed directly to this
     *  class, this is for the former */
    std::string mMeshFilename;

    /** If this is set, the nodes for each processor are read */
    std::string mNodesPerProcessorFilename;

    /**
     *  Whether to use matrix-based assembly of the RHS vector (much more efficient).
     *  True by default
     */
    bool mUseMatrixBasedRhsAssembly;
    /** Whether this problem class has created the mesh itself, as opposed to being given it */
    bool mAllocatedMemoryForMesh;
    /** Whether to print some statistics (max/min voltage) to screen during the simulation */
    bool mWriteInfo;
    /** Whether to write any output at all */
    bool mPrintOutput;
    /** Whether to convert the output from HDF5 to meshalyzer readable format */
    bool mCallChaste2Meshalyzer;

    /** If only outputing voltage for selected nodes, which nodes to output at */
    std::vector<unsigned> mNodesToOutput;

    /** Used by the writer */
    unsigned mVoltageColumnId;
    /** List of extra variables to be written to HDF5 file */
    std::vector<unsigned> mExtraVariablesId;
    /** Used by the writer */
    unsigned mTimeColumnId;
    /** Used by the writer */
    unsigned mNodeColumnId;

    /** The monodomain or bidomain pde */
    AbstractCardiacPde<ELEMENT_DIM,SPACE_DIM>* mpCardiacPde;

    /** Boundary conditions container used in the simulation */
    BoundaryConditionsContainer<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM>* mpBoundaryConditionsContainer;
    /** It is convenient to also have a separate variable for default (zero-Neumann) boundary conditions */
    BoundaryConditionsContainer<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM>* mpDefaultBoundaryConditionsContainer;
    /** The PDE solver */
    AbstractDynamicAssemblerMixin<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM>* mpAssembler;
    /** The cell factory creates the cells for each node */
    AbstractCardiacCellFactory<ELEMENT_DIM,SPACE_DIM>* mpCellFactory;
    /** The mesh. Can either by passed in, or the mesh filename can be set */
    AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* mpMesh;

    /** The current solution vector, of the form [V_0 .. V_N ] for monodomain and
     *  [V_0 phi_0 .. V_N phi_N] for bidomain */
    Vec mSolution;
    
    /**
     * The current simulation time.
     * 
     * This is used to be able to restart simulations at a point other than time zero,
     * either because of repeated calls to Solve (with increased simulation duration)
     * or because of restarting from a checkpoint.
     */
    double mCurrentTime;
    
    /** Tells the destructor to archive the linear system */
    bool mArchiveKSP;

    /**
     * Subclasses must override this method to create a PDE object of the appropriate type.
     *
     * This class will take responsibility for freeing the object when it is finished with.
     */
    virtual AbstractCardiacPde<ELEMENT_DIM,SPACE_DIM>* CreateCardiacPde() =0;

    /**
     * Subclasses must override this method to create a suitable assembler object.
     *
     * This class will take responsibility for freeing the object when it is finished with.
     */
    virtual AbstractDynamicAssemblerMixin<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM>* CreateAssembler() =0;

public:
    /**
     * The object to use to write results to disk.
     * 
     * This (and things in MonodomainProblem) being public are hacks for
     * CardiacElectroMechanicsProblem to work.
     * 
     * \todo CardiacElectroMechanicsProblem should be a friend, but not sure
     * how to get friends to work when both friends are templated and abstract.
     */
    Hdf5DataWriter* mpWriter;

public:
    /**
     * Constructor
     * @param pCellFactory User defined cell factory which shows how the pde should
     * create cells.
     */
    AbstractCardiacProblem(AbstractCardiacCellFactory<ELEMENT_DIM,SPACE_DIM>* pCellFactory);
    
    /**
     * Constructor used by archiving.
     */
    AbstractCardiacProblem();
    
    /**
     *  Destructor
     */
    virtual ~AbstractCardiacProblem();

    /**
     * Initialise the system, once parameters have been set up.
     * 
     * Must be called before first calling Solve().  If loading from a checkpoint,
     * do NOT call this method, as it can also be used to reset the problem to
     * perform another simulation from time 0.
     */
    void Initialise();

    /**
     *  Set a file from which the nodes for each processor are read
     * 
     * @param rFilename
     */
    void SetNodesPerProcessorFilename(const std::string& rFilename);

    /**
     *  Set the boundary conditions container.
     *  @param pbcc is a pointer to a boundary conditions container
     */
    void SetBoundaryConditionsContainer(BoundaryConditionsContainer<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM>* pbcc);

    /**
     *  Performs a series of checks before solving.
     *  It checks whether the cardiac pde has been defined,
     *  whether the simulation time is greater than zero and
     *  whether the output directory is specified (or the output is set not to be produced).
     *  It throws exceptions if any of the above checks fails.
     */
    virtual void PreSolveChecks();

    /**
     * Perhaps this should be a method of AbstractCardiacPde??
     * This is virtual so BidomainProblem can overwrite V to zero for bath nodes, if
     * there are any.
     */
    virtual Vec CreateInitialCondition();

    /**
     *  Set whether to call the Chaste2Meshalyzer script.
     *  This script gets everything ready to visualize the results with meshalyser
     *  and is useful in testing. By default the script is called.
     *  In performance testing for example it desirable to disable the script.
     * 
     * @param call whether to call the script
     */
    void ConvertOutputToMeshalyzerFormat(bool call = true);

    /**
     * This only needs to be called if a mesh filename has not been set.
     * 
     * @param pMesh  the mesh object to use
     */
    void SetMesh(AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh);

    /**
     *  Set whether the simulation will generate results files.
     * 
     * @param rPrintOutput
     */
    void PrintOutput(bool rPrintOutput);

    /**
     *  Set whether extra info will be written to stdout during computation.
     * 
     * @param writeInfo
     */
    void SetWriteInfo(bool writeInfo = true);

    /**
     *  Get the final solution vector. This vector is distributed over all processes.
     *
     *  In case of Bidomain, this is of length 2*numNodes, and of the form
     *  (V_1, phi_1, V_2, phi_2, ......, V_N, phi_N).
     *  where V_j is the voltage at node j and phi_j is the
     *  extracellular potential at node j.
     *
     *  Use with caution since we don't want to alter the state of the PETSc vector.
     */
    Vec GetSolution();
    
    /**
     * Get the solution vector, wrapped in a DistributedVector.
     * 
     * See also GetSolution.
     */
    DistributedVector GetSolutionDistributedVector();

    /**
     * @return the mesh used
     */
    AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM> & rGetMesh();

    /**
     * @return the cardiac PDE used
     */
    AbstractCardiacPde<ELEMENT_DIM,SPACE_DIM>* GetPde();

    /**
     *  First performs some checks by calling  the PreSolveChecks method.
     *  It creates an assembler to which it passes the boundary conditions specified by the user
     *  (otherwise it passes the defauls bcc).
     *   It then calls the Solve method in the assembler class.
     *   It also handles the output, if necessary.
     */
    void Solve();

    /**
     * Closes the files where the solution is stored and,
     * if specified so (as it is by default), converts the output to Meshalyzer format
     * by calling the WriteFilesUsingMesh method in the MeshalyzerWriter class.
     */
    void CloseFilesAndPostProcess();

    /**
     * Write informative details about the progress of the simulation to standard output.
     * 
     * Implemented only in subclasses.
     * 
     * @param time  the current time
     */
    virtual void WriteInfo(double time)=0;

    /**
     * Define what variables are written to the primary results file.
     */
    virtual void DefineWriterColumns();
    
    /**
     * Define the user specified variables to be written to the primary results file
     */
    void DefineExtraVariablesWriterColumns();

    /**
     * Write one timestep of output data to the primary results file.
     * 
     * @param time  the current time
     * @param voltageVec  the solution vector to write
     */
    virtual void WriteOneStep(double time, Vec voltageVec) = 0;
    
    /**
     * Write one timestep of output data for the extra variables to the primary results file.
     */
    void WriteExtraVariablesOneStep();

    /**
     * It creates and initialises the hdf writer from the Hdf5DataWriter class.
     * It passes the output directory and file name to it.
     * It is called by Solve(), if the output needs to be generated.
     */
    void InitialiseWriter();

    /**
     * Specifies which nodes in the mesh to output.
     * 
     * @param rNodesToOutput is a reference to a vector with the indexes of the nodes
     * where the output is desired.
     * If empty, the output will be for all the nodes in the mesh.
     */
    void SetOutputNodes(std::vector<unsigned> & rNodesToOutput);

    /**
     * Create and return a data reader configured to read the results we've been outputting.
     */
    Hdf5DataReader GetDataReader();

    /**
     *  Whether to use matrix-based RHS assembly or not.
     * 
     * @param usematrix
     */
    void UseMatrixBasedRhsAssembly(bool usematrix=true);

    /**
     *  Called at end of each time step in the main time-loop in
     *  Solve(). Empty implementation but can be overloaded by child
     *  classes.
     * 
     * @param time  the current time
     */
    virtual void OnEndOfTimestep(double time)
    {}
    
    /**
     *  Tells the problem class to archive the linear system after every call to Solve()
     * 
     * @param  archive set true to archive the LinearSystem object at the end
     */
    void SetArchiveLinearSystemObject(bool archive=true);  

};

TEMPLATED_CLASS_IS_ABSTRACT_3_UNSIGNED(AbstractCardiacProblem);

#endif /*ABSTRACTCARDIACPROBLEM_HPP_*/
