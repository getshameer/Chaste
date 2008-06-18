/*

Copyright (C) University of Oxford, 2008

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

#ifndef TESTMONODOMAINFASTSLOWPROBLEM_HPP_
#define TESTMONODOMAINFASTSLOWPROBLEM_HPP_

#include "UblasCustomFunctions.hpp"
#include "PetscSetupAndFinalize.hpp"
#include "SimpleStimulus.hpp"
#include "FastSlowLuoRudyIModel1991.hpp"
#include "LuoRudyIModel1991OdeSystem.hpp"
#include "MonodomainFastSlowProblem.hpp"



// simple cell factory that creates fast-slow cells.
class MyCellFactory : public AbstractCardiacCellFactory<2>
{
private:
    SimpleStimulus* mpStimulus;
    bool mFastSlow;

public:
    MyCellFactory(bool fastSlow) : AbstractCardiacCellFactory<2>(0.0025)
    {
        mpStimulus = new SimpleStimulus(-10000*500.0, 0.5);
        mFastSlow = fastSlow;
    }

    AbstractCardiacCell* CreateCardiacCellForNode(unsigned nodeIndex)
    {
        double x = mpMesh->GetNode(nodeIndex)->rGetLocation()[0];

        // stimulus is zero unless x=0
        AbstractStimulusFunction* p_stim;
        if(fabs(x)<1e-6)
        {
            p_stim = mpStimulus;
        }
        else
        {
            p_stim = mpZeroStimulus;
        }

        if(mFastSlow)
        {
            // fast-slow cells
            return new FastSlowLuoRudyIModel1991(mpSolver, mTimeStep, p_stim); // state unset at the moment
        }
        else
        {
            // normal cells
            return new LuoRudyIModel1991OdeSystem(mpSolver, mTimeStep, p_stim);
        }
    }

    ~MyCellFactory(void)
    {
        delete mpStimulus;
    }
};


class TestMonodomainFastSlowProblem : public CxxTest::TestSuite
{
public:
    // run the fast slow problem and compare solution with a normal problem
    //
    // todo: fix test below. also check with different meshes and endtimes and
    // timesteps. With num_coarse_elem_each_dir = 2, num_fine_elem_each_dir=20
    // a gating variable exception occured..........
    void TestMonodomainFastSlowProblemAgainstNormal() throw (Exception)
    {
        EXIT_IF_PARALLEL;

        double simulation_time = 10; //ms

        double width = 2; //cm
        double heigth = 2; //cm
        double coarse_ds = 0.1; //cm
        double fine_ds = 0.01;
        
        unsigned num_coarse_elem_each_dir = width/coarse_ds;
        unsigned num_fine_elem_each_dir = width/fine_ds;

        //////////////////////////////////////////////////
        // solve a mixed mesh, fast/slow problem
        //////////////////////////////////////////////////
        MixedTetrahedralMesh<2,2> mixed_mesh;
        mixed_mesh.ConstructRectangularMeshes(width, heigth, num_coarse_elem_each_dir, num_fine_elem_each_dir);
        ReplicatableVector voltage_fast_slow;
        {

    
            MyCellFactory cell_factory(true);
    
            MonodomainFastSlowProblem<2> monodomain_fast_slow_prob( &cell_factory, mixed_mesh, 1.0 );
    
            monodomain_fast_slow_prob.SetEndTime(simulation_time);   // ms
            monodomain_fast_slow_prob.SetOutputDirectory("MonodomainFastSlow");
            monodomain_fast_slow_prob.SetOutputFilenamePrefix("res");
    
            monodomain_fast_slow_prob.Initialise();
            monodomain_fast_slow_prob.Solve();
    
            EventHandler::Headings();
            EventHandler::Report();
    
            voltage_fast_slow.ReplicatePetscVector( monodomain_fast_slow_prob.GetVoltage() );
            TS_ASSERT_EQUALS(voltage_fast_slow.size(), mixed_mesh.GetFineMesh()->GetNumNodes() );

            EventHandler::Reset();
        }

        //////////////////////////////////////////////////
        // solve using normal monodomain problem
        //////////////////////////////////////////////////
        ReplicatableVector voltage_normal;
        {
            MyCellFactory cell_factory_normal(false);
    
            MonodomainProblem<2> monodomain_prob( &cell_factory_normal);
            monodomain_prob.SetMesh(mixed_mesh.GetFineMesh());
    
            monodomain_prob.SetEndTime(simulation_time);   // ms
            monodomain_prob.SetOutputDirectory("MonodomainNormalToCompareWithFastSlow");
            monodomain_prob.SetOutputFilenamePrefix("res");
    
            monodomain_prob.Initialise();
            monodomain_prob.Solve();
            EventHandler::Headings();
            EventHandler::Report();
            EventHandler::Reset();
            voltage_normal.ReplicatePetscVector( monodomain_prob.GetVoltage() );
        }

        //////////////////////////////////////////////////
        // compare 
        //////////////////////////////////////////////////
        TS_ASSERT_EQUALS(voltage_fast_slow.size(), voltage_normal.size() );

        bool some_voltage_greater_than_zero = false;
        for(unsigned i=0; i<voltage_fast_slow.size(); i++)
        {
            TS_ASSERT_DELTA(voltage_fast_slow[i], voltage_normal[i], 1.0);
            if(voltage_fast_slow[i] > 0.0)
            {
                some_voltage_greater_than_zero = true;
            }
        }
        TS_ASSERT(some_voltage_greater_than_zero);

        EventHandler::Enable();
    }

};

#endif /*TESTMONODOMAINFASTSLOWPROBLEM_HPP_*/
