#ifndef _TESTMONODOMAINSLOW_HPP_
#define _TESTMONODOMAINSLOW_HPP_

#include <cxxtest/TestSuite.h>
#include "petscvec.h"
#include <vector>
#include <iostream>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>

#include "SimpleLinearSolver.hpp"
#include "ConformingTetrahedralMesh.cpp"
#include "Node.hpp"
#include "Element.hpp"
#include "BoundaryConditionsContainer.hpp"
#include "SimpleDg0ParabolicAssembler.hpp"  
#include "MonodomainDg0Assembler.hpp"
#include "TrianglesMeshReader.hpp"
#include "ColumnDataWriter.hpp"
#include "ColumnDataReader.hpp"
#include "PropagationPropertiesCalculator.hpp"

#include "MonodomainPde.hpp"
#include "MockEulerIvpOdeSolver.hpp"
#include "FischerPde.hpp"

#include "PetscSetupAndFinalize.hpp"
#include "MonodomainProblem.hpp"
#include "AbstractLinearParabolicPde.hpp"
#include "AbstractMonodomainProblemStimulus.hpp"

class PointStimulus1D: public AbstractMonodomainProblemStimulus<1>
{
public:
    virtual void Apply(MonodomainPde<1> *pPde)
    {
        static InitialStimulus stimulus(-600.0, 0.5);
        pPde->SetStimulusFunctionAtNode(0, &stimulus);
    }
};

class TestMonodomainDg0Assembler : public CxxTest::TestSuite 
{   
private:
    /**
     * Refactor code to set up a PETSc vector holding the initial condition.
     */
    Vec CreateInitialConditionVec(int size)
    {
        Vec initial_condition;
        VecCreate(PETSC_COMM_WORLD, &initial_condition);
        VecSetSizes(initial_condition, PETSC_DECIDE, size);
        VecSetFromOptions(initial_condition);
        return initial_condition;
    }
    
public:
    void TestMonodomainDg01D()
    {
        PointStimulus1D point_stimulus_1D;
        MonodomainProblem<1> monodomainProblem("mesh/test/data/1D_0_to_1_100_elements",
                                               30, 
                                               "testoutput/MonoDg01d",
                                               "NewMonodomainLR91_1d",
                                               &point_stimulus_1D);

        monodomainProblem.Solve();
        
        double* currentVoltageArray;
    
        // test whether voltages and gating variables are in correct ranges

        int ierr = VecGetArray(monodomainProblem.mCurrentVoltage, &currentVoltageArray); 
        
        for(int global_index=monodomainProblem.mLo; global_index<monodomainProblem.mHi; global_index++)
        {
            // assuming LR model has Ena = 54.4 and Ek = -77 and given magnitude of initial stim = -80
            double Ena   =  54.4;
            double Ek    = -77.0;
            double Istim = -80.0;
            
            TS_ASSERT_LESS_THAN_EQUALS(   currentVoltageArray[global_index-monodomainProblem.mLo] , Ena +  30);
            TS_ASSERT_LESS_THAN_EQUALS(  -currentVoltageArray[global_index-monodomainProblem.mLo] + (Ek-30), 0);
                
            std::vector<double> odeVars = monodomainProblem.mMonodomainPde->GetOdeVarsAtNode(global_index);           
            for(int j=0; j<8; j++)
            {
                // if not voltage or calcium ion conc, test whether between 0 and 1 
                if((j!=4) && (j!=3))
                {
                    TS_ASSERT_LESS_THAN_EQUALS(  odeVars[j], 1.0);        
                    TS_ASSERT_LESS_THAN_EQUALS( -odeVars[j], 0.0);        
                }   
            }
               
            if (global_index==25)
            {
                TS_ASSERT_DELTA(currentVoltageArray[global_index-monodomainProblem.mLo], 8.3749, 0.001);
            }
            if (global_index==30)
            {
                TS_ASSERT_DELTA(currentVoltageArray[global_index-monodomainProblem.mLo], 8.0799, 0.001);
            }
            if (global_index==40)
            {
                TS_ASSERT_DELTA(currentVoltageArray[global_index-monodomainProblem.mLo], 7.4640, 0.001);
            }
            if (global_index==50)
            {
                TS_ASSERT_DELTA(currentVoltageArray[global_index-monodomainProblem.mLo], 6.8283, 0.001);
            }
            if (global_index==51)
            {
                TS_ASSERT_DELTA(currentVoltageArray[global_index-monodomainProblem.mLo], 6.7649, 0.001);
            }
            if (global_index==75)
            {
                TS_ASSERT_DELTA(currentVoltageArray[global_index-monodomainProblem.mLo], 5.4807, 0.001);
            }
            
        }
        VecRestoreArray(monodomainProblem.mCurrentVoltage, &currentVoltageArray);      
        VecAssemblyBegin(monodomainProblem.mCurrentVoltage);
        VecAssemblyEnd(monodomainProblem.mCurrentVoltage);
        VecDestroy(monodomainProblem.mCurrentVoltage);
        
        int num_procs;
        MPI_Comm_size(PETSC_COMM_WORLD, &num_procs);

        if (num_procs == 1)
        {
            // Calculate the conduction velocity
            ColumnDataReader simulation_data("testoutput/MonoDg01d",
                                             "NewMonodomainLR91_1d");
            PropagationPropertiesCalculator ppc(&simulation_data);
            double velocity;
            
            // Check action potential propagated to node 95
            TS_ASSERT_THROWS_NOTHING(velocity=ppc.CalculateConductionVelocity(5,95,0.9));
            
            // The value should be approximately 50cm/sec
            // i.e. 0.05 cm/msec (which is the units of the simulation)
            TS_ASSERT_DELTA(velocity, 0.05, 0.005);
        }
    }
    
};
#endif //_TESTMONODOMAINSLOW_HPP_
