/*
Copyright (C) Oxford University 2008

This file is part of CHASTE.

CHASTE is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

CHASTE is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with CHASTE.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _TESTMONODOMAINCONDUCTIONVELOCITY_HPP_
#define _TESTMONODOMAINCONDUCTIONVELOCITY_HPP_


#include <cxxtest/TestSuite.h>
#include <vector>

#include "ConformingTetrahedralMesh.hpp"
#include "PropagationPropertiesCalculator.hpp"
#include "Hdf5DataReader.hpp"
#include "PetscSetupAndFinalize.hpp"
#include "MonodomainProblem.hpp"
#include "CheckMonoLr91Vars.hpp"
#include "PlaneStimulusCellFactory.hpp"


class TestMonodomainConductionVelocity : public CxxTest::TestSuite
{
public:
    // Solve on a 1D string of cells, 1cm long with a space step of 0.1mm.
    void TestMonodomainDg01DWith100elements()
    {
        PlaneStimulusCellFactory<1> cell_factory;
        MonodomainProblem<1> monodomain_problem(&cell_factory);
        
        monodomain_problem.SetMeshFilename("mesh/test/data/1D_0_to_1_100_elements");
        monodomain_problem.SetEndTime(30);   // 30 ms
        monodomain_problem.SetOutputDirectory("MonoConductionVel");
        monodomain_problem.SetOutputFilenamePrefix("MonodomainLR91_1d");
        
        monodomain_problem.SetIntracellularConductivities(Create_c_vector(0.0005));
        
        monodomain_problem.Initialise();
        
        monodomain_problem.GetMonodomainPde()->SetSurfaceAreaToVolumeRatio(1.0);
        monodomain_problem.GetMonodomainPde()->SetCapacitance(1.0);
        
        monodomain_problem.Solve();
        
        // test whether voltages and gating variables are in correct ranges
        CheckMonoLr91Vars<1>(monodomain_problem);
        
        // Calculate the conduction velocity
        Hdf5DataReader simulation_data("MonoConductionVel",
                                       "MonodomainLR91_1d");
        PropagationPropertiesCalculator ppc(&simulation_data);
        double velocity=0.0;
        
        // Check action potential propagated to node 95
        TS_ASSERT_THROWS_NOTHING(velocity=ppc.CalculateConductionVelocity(5,95,0.9));
        
        // The value should be approximately 50cm/sec
        // i.e. 0.05 cm/msec (which is the units of the simulation)
        TS_ASSERT_DELTA(velocity, 0.05, 0.003);
    }
    
    // Solve on a 1D string of cells, 1cm long with a space step of 0.5mm.
    //
    // Note that this space step ought to be too big!
    void TestMonodomainDg01DWith20elements()
    {
//#ifdef NDEBUG
//        TS_ASSERT(true);
//#else
        //Note that this test *used to* FAIL under any NDEBUG build.
        /*
         * This is because it is testing exceptions which are tripped by gating variables going 
         * out of range in the Luo-Rudy cell model.  These variable ranges are not tested in
         * production builds.  They are guarded with  "#ifndef NDEBUG"
         */ 
        
        PlaneStimulusCellFactory<1> cell_factory;
        MonodomainProblem<1> monodomain_problem(&cell_factory);
        
        monodomain_problem.SetMeshFilename("mesh/test/data/1D_0_to_1_20_elements");
        monodomain_problem.SetEndTime(1);   // 1 ms
        monodomain_problem.SetOutputDirectory("MonoConductionVel");
        monodomain_problem.SetOutputFilenamePrefix("MonodomainLR91_1d");
        
        monodomain_problem.SetIntracellularConductivities(Create_c_vector(0.0005));
        
        monodomain_problem.Initialise();
        
        monodomain_problem.GetMonodomainPde()->SetSurfaceAreaToVolumeRatio(1.0);
        monodomain_problem.GetMonodomainPde()->SetCapacitance(1.0);

        // the mesh is too coarse, and this simulation will result in cell gating
        // variables going out of range. An exception should be thrown in the
        // EvaluateYDerivatives() method of the cell model

        TS_ASSERT_THROWS_ANYTHING(monodomain_problem.Solve());
//#endif
    }

};
#endif //_TESTMONODOMAINCONDUCTIONVELOCITY_HPP_
