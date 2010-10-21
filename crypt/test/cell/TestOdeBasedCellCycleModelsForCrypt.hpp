/*

Copyright (C) University of Oxford, 2005-2010

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
#ifndef TESTODEBASEDCELLCYCLEMODELSFORCRYPT_HPP_
#define TESTODEBASEDCELLCYCLEMODELSFORCRYPT_HPP_

#include <cxxtest/TestSuite.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <fstream>
#include <boost/shared_ptr.hpp>

#include "Alarcon2004OxygenBasedCellCycleModel.hpp"
#include "VanLeeuwen2009WntSwatCellCycleModelHypothesisOne.hpp"
#include "VanLeeuwen2009WntSwatCellCycleModelHypothesisTwo.hpp"
#include "WntCellCycleModel.hpp"
#include "StochasticWntCellCycleModel.hpp"

#include "AbstractCellMutationState.hpp"
#include "WildTypeCellMutationState.hpp"
#include "ApcOneHitCellMutationState.hpp"
#include "ApcTwoHitCellMutationState.hpp"
#include "BetaCateninOneHitCellMutationState.hpp"

#include "CellwiseData.hpp"
#include "OutputFileHandler.hpp"
#include "CheckReadyToDivideAndPhaseIsUpdated.hpp"
#include "AbstractCellBasedTestSuite.hpp"

/**
 * This class contains tests for methods on classes
 * inheriting from AbstractOdeBasedCellCycleModel.
 */
class TestOdeBasedCellCycleModelsForCrypt : public AbstractCellBasedTestSuite
{
public:

    /**
     * In this test we use a WntCellCycleModel and begin with a steady-state
     * Wnt concentration of 1.0. Under such circumstances, the cell cycle model
     * would normally go into S phase at time t=5.971. Instead, we reduce the
     * Wnt concentration linearly to zero over the time interval 1<t<2, and the
     * cell doesn't divide.
     */
    void TestWntCellCycleModelForVaryingWntStimulus() throw(Exception)
    {
        // Set up Wnt concentration
        double wnt_level = 1.0;
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_level);

        // Create cell cycle model and associated cell
        WntCellCycleModel* p_cell_model = new WntCellCycleModel();
        p_cell_model->SetDimension(2);
        p_cell_model->SetCellProliferativeType(STEM);

        // Set up simulation time
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        double end_time = 10.0 + p_cell_model->GetMDuration(); // hours
		unsigned num_timesteps = 1000*(unsigned)end_time;
		p_simulation_time->SetEndTimeAndNumberOfTimeSteps(end_time, num_timesteps); // 15.971 hours to go into S phase

        TS_ASSERT_EQUALS(p_cell_model->CanCellTerminallyDifferentiate(), false);

        boost::shared_ptr<AbstractCellMutationState> p_healthy_state(new WildTypeCellMutationState);
        CellPtr p_stem_cell(new Cell(p_healthy_state, p_cell_model));
        p_stem_cell->InitialiseCellCycleModel();

        /*
         * When using a WntCellCycleModel, there is no such thing as a 'stem' cell. The cell
         * proliferative type is updated to transit or differentiated, depending on the Wnt
         * concentration, when InitialiseCellCycleModel() is called.
         */
        TS_ASSERT_EQUALS(p_stem_cell->GetCellCycleModel()->GetCellProliferativeType(), TRANSIT);

        /*
         * For coverage, we create another cell cycle model that is identical except that we
         * manually pass in an ODE solver. In this case, our ODE solver (RungeKutta4IvpOdeSolver)
         * is the same type as the solver used by the cell cycle model if no solver is provided
         * (unless CVODE is used), so our results should be identical.
         */
#ifdef CHASTE_CVODE
        boost::shared_ptr<CellCycleModelOdeSolver<WntCellCycleModel, CvodeAdaptor> >
            p_solver(CellCycleModelOdeSolver<WntCellCycleModel, CvodeAdaptor>::Instance());
        p_solver->Initialise();
        p_solver->CheckForStoppingEvents();
        p_solver->SetMaxSteps(10000);
#else
        boost::shared_ptr<CellCycleModelOdeSolver<WntCellCycleModel, RungeKutta4IvpOdeSolver> >
            p_solver(CellCycleModelOdeSolver<WntCellCycleModel, RungeKutta4IvpOdeSolver>::Instance());
        p_solver->Initialise();
#endif //CHASTE_CVODE

        WntCellCycleModel* p_other_cell_model = new WntCellCycleModel(p_solver);
        p_other_cell_model->SetDimension(2);
        p_other_cell_model->SetCellProliferativeType(STEM);

        CellPtr p_other_stem_cell(new Cell(p_healthy_state, p_other_cell_model));
        p_other_stem_cell->InitialiseCellCycleModel();

        // Progress both cells through the cell cycle
        for (unsigned i=0; i<num_timesteps; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            double time = p_simulation_time->GetTime();

            bool result = p_cell_model->ReadyToDivide();
            bool other_result = p_other_cell_model->ReadyToDivide();

            // The Wnt concentration reduces from 1 to 0 over the interval 1<t<2 (at beginning of G1 phase)
            if (time <= 2.0)
            {
                wnt_level = 2.0 - time;
            }
            else
            {
                wnt_level = 0.0;
            }
            WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_level);

            // Test that the cell cycle model does not stop for division
            TS_ASSERT_EQUALS(result, false);
            TS_ASSERT_EQUALS(other_result, false);
        }

        // Test ODE solution
        double tol = 1e-5;

        std::vector<double> test_results = p_cell_model->GetProteinConcentrations();
        std::vector<double> other_test_results = p_other_cell_model->GetProteinConcentrations();
#ifdef CHASTE_CVODE
        TS_ASSERT_DELTA(test_results[0], 0.7329922345, tol);
#else
        TS_ASSERT_DELTA(test_results[0], 7.330036281693106e-01, tol);
#endif //CHASTE_CVODE
        TS_ASSERT_DELTA(test_results[1], 1.715690244022676e-01, tol);
        TS_ASSERT_DELTA(test_results[2], 6.127460817296076e-02, tol);
        TS_ASSERT_DELTA(test_results[3], 1.549402358669023e-07, tol);
        TS_ASSERT_DELTA(test_results[4], 4.579067802591843e-08, tol);
        TS_ASSERT_DELTA(test_results[5], 9.999999999999998e-01, tol);
        TS_ASSERT_DELTA(test_results[6], 0.5*7.415537855270896e-03, tol);
        TS_ASSERT_DELTA(test_results[7], 0.5*7.415537855270896e-03, tol);
        TS_ASSERT_DELTA(test_results[8], 0.0, tol);

#ifdef CHASTE_CVODE
        TS_ASSERT_DELTA(other_test_results[0], 0.7329922345, tol);
#else
        TS_ASSERT_DELTA(other_test_results[0], 7.330036281693106e-01, tol);
#endif //CHASTE_CVODE
        TS_ASSERT_DELTA(other_test_results[1], 1.715690244022676e-01, tol);
        TS_ASSERT_DELTA(other_test_results[2], 6.127460817296076e-02, tol);
        TS_ASSERT_DELTA(other_test_results[3], 1.549402358669023e-07, tol);
        TS_ASSERT_DELTA(other_test_results[4], 4.579067802591843e-08, tol);
        TS_ASSERT_DELTA(other_test_results[5], 9.999999999999998e-01, tol);
        TS_ASSERT_DELTA(other_test_results[6], 0.5*7.415537855270896e-03, tol);
        TS_ASSERT_DELTA(other_test_results[7], 0.5*7.415537855270896e-03, tol);
        TS_ASSERT_DELTA(other_test_results[8], 0.0, tol);

        // Test that, since the cell now experiences a low Wnt concentration,
        // it has indeed changed cell type to differentiated
        TS_ASSERT_EQUALS(p_stem_cell->GetCellCycleModel()->GetCellProliferativeType(), DIFFERENTIATED);

        TS_ASSERT_EQUALS(p_other_stem_cell->GetCellCycleModel()->GetCellProliferativeType(), DIFFERENTIATED);

        double diff = 1.0;
        test_results[6] = test_results[6] + diff;

        p_cell_model->SetProteinConcentrationsForTestsOnly(1.0, test_results);

        test_results = p_cell_model->GetProteinConcentrations();

        TS_ASSERT_DELTA(test_results[6], diff + 0.5*7.415537855270896e-03, 1e-5);
        TS_ASSERT_DELTA(test_results[5], 0.9999999999999998, 1e-5);

        // Tidy up
        WntConcentration<2>::Destroy();
    }

    void TestVanLeeuwen2009WntSwatCellCycleModelHypothesisOne() throw(Exception)
    {
        // Set up simulation time
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        double end_time = 30; // hours
        unsigned num_timesteps = 100*(unsigned)end_time;
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(end_time, num_timesteps); // 15.971 hours to go into S phase

        // Set up Wnt concentration
        double wnt_level = 1.0;
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_level);

        // Create cell cycle model and associated cell
        VanLeeuwen2009WntSwatCellCycleModelHypothesisOne* p_cell_model = new VanLeeuwen2009WntSwatCellCycleModelHypothesisOne();
        p_cell_model->SetDimension(2);
        p_cell_model->SetCellProliferativeType(STEM);

        // Test that member variables are set correctly
        boost::shared_ptr<AbstractCellMutationState> p_healthy_state(new WildTypeCellMutationState);
        CellPtr p_stem_cell(new Cell(p_healthy_state, p_cell_model));

        p_stem_cell->InitialiseCellCycleModel();

        // When using a WntCellCycleModel, there is no such thing as
        // a 'stem cell'. Cell type is changed to transit or
        // differentiated, depending on the Wnt concentration, when
        // InitialiseCellCycleModel() is called.
        TS_ASSERT_EQUALS(p_stem_cell->GetCellCycleModel()->GetCellProliferativeType(), TRANSIT);

        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(1.0);

        double tol = 1e-4;
#ifdef CHASTE_CVODE
        const double expected_g1_duration = 6.18252;
        tol = 1e-5;
#else
        const double expected_g1_duration = 6.1959;
#endif //CHASTE_CVODE

        // Progress through the cell cycle under a constant Wnt concentration
        for (unsigned i=0; i<21*num_timesteps/30.0; i++)
        {
            p_simulation_time->IncrementTimeOneStep();

            // Call ReadyToDivide on the cell, then test the results
            // of calling ReadyToDivide on the model and test (in
            // CheckReadyToDivideAndPhaseIsUpdated).
            p_stem_cell->ReadyToDivide();
            CheckReadyToDivideAndPhaseIsUpdated(p_cell_model, expected_g1_duration);
        }

        // Test that the cell cycle model stopped for division correctly
        TS_ASSERT_DELTA(SimulationTime::Instance()->GetTime(), 21.0, 1e-4);
        TS_ASSERT_EQUALS(p_cell_model->ReadyToDivide(), true);

        std::vector<double> test_results = p_cell_model->GetProteinConcentrations();

        // Test ODE solution (the correct values were found to an accuracy
        // of around 1e-6 using the Matlab solver ode15s)
        TS_ASSERT_DELTA(test_results[0] , 2.93699961539512e-01 , 2*10*tol);
        TS_ASSERT_DELTA(test_results[1] , 1.000000000000000, 2*100*tol);
        TS_ASSERT_DELTA(test_results[2] , 2.40050625298734 , 2*10*tol);
        TS_ASSERT_DELTA(test_results[3] , 1.39281551739568 , 2*10*tol);
        TS_ASSERT_DELTA(test_results[4] , 1.35836451056026e-01 , 2*10*tol);
        TS_ASSERT_DELTA(test_results[5] , 1.428571428571429e-01, tol);
        TS_ASSERT_DELTA(test_results[6] , 2.857142857142857e-02, tol);
        TS_ASSERT_DELTA(test_results[7] , 2.120643654085205e-01, tol);
        TS_ASSERT_DELTA(test_results[8] , 1.439678172957377e+01, 10*tol);
        TS_ASSERT_DELTA(test_results[9] ,                     0, tol);
        TS_ASSERT_DELTA(test_results[10],                     0, tol);
        TS_ASSERT_DELTA(test_results[11],                     0, tol);
        TS_ASSERT_DELTA(test_results[12], 1.000000000000002e+01, tol);
        TS_ASSERT_DELTA(test_results[13], 1.028341552112414e+02, 100*tol);
        TS_ASSERT_DELTA(test_results[14],                     0, tol);
        TS_ASSERT_DELTA(test_results[15], 2.499999999999999e+01, tol);
        TS_ASSERT_DELTA(test_results[16], 1.439678172957377e+01, 10*tol);
        TS_ASSERT_DELTA(test_results[17],                     0, tol);
        TS_ASSERT_DELTA(test_results[18],                     0, tol);
        TS_ASSERT_DELTA(test_results[19],                     0, tol);
        TS_ASSERT_DELTA(test_results[20], 2.235636835087684, tol);
        TS_ASSERT_DELTA(test_results[21], 1.000000000000000, tol);

        // The cell cycle model acts as if it was divided at time = 16.1877. This
        // is fine as the cell cycle model dictates the division time, not when
        // the cell is actually divided.
        CellPtr p_daughter_cell = p_stem_cell->Divide();
        AbstractCellCycleModel* p_cell_model2 = p_daughter_cell->GetCellCycleModel();

        TS_ASSERT_EQUALS(p_cell_model->GetCurrentCellCyclePhase(), M_PHASE);
        TS_ASSERT_EQUALS(p_cell_model2->GetCurrentCellCyclePhase(), M_PHASE);

        TS_ASSERT_EQUALS(p_cell_model->ReadyToDivide(), false);
        TS_ASSERT_EQUALS(p_cell_model2->ReadyToDivide(), false);

        // Check that the first five protein levels have been reset and the rest are the same as before.
        tol = 1e-4;
        test_results = p_cell_model->GetProteinConcentrations();
        TS_ASSERT_DELTA(test_results[0] , 2.631865125420296e-01, 10*tol);
        TS_ASSERT_DELTA(test_results[1] , 2.678271949808561e-01, 10*tol);
        TS_ASSERT_DELTA(test_results[2] , 2.389956081120099, 10*tol);
        TS_ASSERT_DELTA(test_results[3] , 1.390258620103223, 10*tol);
        TS_ASSERT_DELTA(test_results[4] , 1.218603203963113e-01, 10*tol);
        TS_ASSERT_DELTA(test_results[5] , 1.428571428571429e-01, tol);
        TS_ASSERT_DELTA(test_results[6] , 2.857142857142857e-02, tol);
        TS_ASSERT_DELTA(test_results[7] , 2.120643654085205e-01, tol);
        TS_ASSERT_DELTA(test_results[8] , 1.439678172957377e+01, 10*tol);
        TS_ASSERT_DELTA(test_results[9] ,                     0, tol);
        TS_ASSERT_DELTA(test_results[10],                     0, tol);
        TS_ASSERT_DELTA(test_results[11],                     0, tol);
        TS_ASSERT_DELTA(test_results[12], 1.000000000000002e+01, tol);
        TS_ASSERT_DELTA(test_results[13], 1.028341552112414e+02, 100*tol);
        TS_ASSERT_DELTA(test_results[14],                     0, tol);
        TS_ASSERT_DELTA(test_results[15], 2.499999999999999e+01, tol);
        TS_ASSERT_DELTA(test_results[16], 1.439678172957377e+01, 10*tol);
        TS_ASSERT_DELTA(test_results[17],                     0, tol);
        TS_ASSERT_DELTA(test_results[18],                     0, tol);
        TS_ASSERT_DELTA(test_results[19],                     0, tol);
        TS_ASSERT_DELTA(test_results[20], 2.235636835087684, tol);
        TS_ASSERT_DELTA(test_results[21], 1.000000000000000, tol);

        // Now progress through the cell cycle under a decreasing Wnt concentration
        for (unsigned i=0; i<9*num_timesteps/30.0; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            double time = p_simulation_time->GetTime();
            bool result = p_cell_model->ReadyToDivide();
            bool result2 = p_cell_model2->ReadyToDivide();

            if (time <= 22.0)
            {
                wnt_level = 22.0 - time;
            }
            else
            {
                wnt_level = 0.0;
            }
            WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_level);

            TS_ASSERT_EQUALS(result, false);
            TS_ASSERT_EQUALS(result2, false);
        }
        TS_ASSERT_DELTA(SimulationTime::Instance()->GetTime(), 30.0, 1e-4);

        test_results = p_cell_model->GetProteinConcentrations();

        // Test ODE solution (the tolerances for some values are deliberately
        // loose, so that we don't need different answers for CVODE and RK4)
        TS_ASSERT_DELTA(test_results[0] , 0.3648, 2e-3);
        TS_ASSERT_DELTA(test_results[1] , 1.000, 1e-2);
        TS_ASSERT_DELTA(test_results[2] , 1.4955, 1e-2);
        TS_ASSERT_DELTA(test_results[3] , 0.8125, 2e-2);
        TS_ASSERT_DELTA(test_results[4] , 0.0996, 1e-2);
        TS_ASSERT_DELTA(test_results[5] , 0.6666, 1e-4);
        TS_ASSERT_DELTA(test_results[6] , 0.0666, 1e-4);
        TS_ASSERT_DELTA(test_results[7] , 0.7311, 2e-3);
        TS_ASSERT_DELTA(test_results[8] , 6.0299, 4e-2);
        TS_ASSERT_DELTA(test_results[9] , 0, 1e-4);
        TS_ASSERT_DELTA(test_results[10], 0, 1e-4);
        TS_ASSERT_DELTA(test_results[11], 0, 1e-4);
        TS_ASSERT_DELTA(test_results[12], 17.5407, 1e-2);
        TS_ASSERT_DELTA(test_results[13], 75.5926, .5);
        TS_ASSERT_DELTA(test_results[14], 0, 1e-4);
        TS_ASSERT_DELTA(test_results[15], 29.5666, 1e-2);
        TS_ASSERT_DELTA(test_results[16], 7.1333, 1e-1);
        TS_ASSERT_DELTA(test_results[17], 0, 1e-4);
        TS_ASSERT_DELTA(test_results[18], 0, 1e-4);
        TS_ASSERT_DELTA(test_results[19], 0, 1e-4);
        TS_ASSERT_DELTA(test_results[20], 1.5991, 2e-2);
        TS_ASSERT_DELTA(test_results[21], 0.0000, 1e-4);

        // Test that, since the cell now experiences a low Wnt concentration,
        // it has indeed changed cell type to differentiated
        TS_ASSERT_EQUALS(p_stem_cell->GetCellCycleModel()->GetCellProliferativeType(), DIFFERENTIATED);

        // Test beta catenin levels

        // membrane_beta_cat = Ca + Ma
        double membrane_beta_cat = test_results[13]+test_results[14];

        // cytoplasmic_beta_cat = Cu + Co + Cc + Mo + Mc
        double cytoplasm_beta_cat = test_results[7] + test_results[8]
                          + test_results[9] + test_results[10]+test_results[11];

        // nuclear_beta_cat = Cot + Cct + Mot + Mct
        double nuclear_beta_cat = test_results[16] + test_results[17]
                                    + test_results[18] + test_results[19];

        TS_ASSERT_DELTA(p_cell_model->GetMembraneBoundBetaCateninLevel(), membrane_beta_cat, 1e-4);
        TS_ASSERT_DELTA(p_cell_model->GetCytoplasmicBetaCateninLevel(), cytoplasm_beta_cat, 1e-4);
        TS_ASSERT_DELTA(p_cell_model->GetNuclearBetaCateninLevel(), nuclear_beta_cat, 1e-4);

        // Coverage of 1D

        WntConcentration<1>::Instance()->SetConstantWntValueForTesting(wnt_level);
        VanLeeuwen2009WntSwatCellCycleModelHypothesisOne* p_cell_model_1d = new VanLeeuwen2009WntSwatCellCycleModelHypothesisOne();
        p_cell_model_1d->SetDimension(1);
        p_cell_model_1d->SetCellProliferativeType(STEM);

        TS_ASSERT_EQUALS(p_cell_model_1d->GetDimension(), 1u);

        CellPtr p_stem_cell_1d(new Cell(p_healthy_state, p_cell_model_1d));
        p_stem_cell_1d->InitialiseCellCycleModel();

        SimulationTime::Destroy();
        SimulationTime::Instance()->SetStartTime(0.0);
        SimulationTime::Instance()->SetEndTimeAndNumberOfTimeSteps(end_time, num_timesteps);
        SimulationTime::Instance()->IncrementTimeOneStep();
        TS_ASSERT_EQUALS(p_cell_model_1d->ReadyToDivide(), false);

        // Coverage of 3D

        WntConcentration<3>::Instance()->SetConstantWntValueForTesting(wnt_level);
        VanLeeuwen2009WntSwatCellCycleModelHypothesisOne* p_cell_model_3d = new VanLeeuwen2009WntSwatCellCycleModelHypothesisOne();
        p_cell_model_3d->SetDimension(3);
        p_cell_model_3d->SetCellProliferativeType(STEM);

        TS_ASSERT_EQUALS(p_cell_model_3d->GetDimension(), 3u);

        CellPtr p_stem_cell_3d(new Cell(p_healthy_state, p_cell_model_3d));
        p_stem_cell_3d->InitialiseCellCycleModel();

        SimulationTime::Destroy();
        SimulationTime::Instance()->SetStartTime(0.0);
        SimulationTime::Instance()->SetEndTimeAndNumberOfTimeSteps(end_time, num_timesteps);
        SimulationTime::Instance()->IncrementTimeOneStep();
        TS_ASSERT_EQUALS(p_cell_model_3d->ReadyToDivide(), false);

        // Tidy up
        WntConcentration<1>::Destroy();
        WntConcentration<2>::Destroy();
        WntConcentration<3>::Destroy();
    }

    void TestVanLeeuwen2009WntSwatCellCycleModelHypothesisTwo() throw(Exception)
    {
        // Set up simulation time
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        double end_time = 30; // hours
        unsigned num_timesteps = 100*(unsigned)end_time;
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(end_time, num_timesteps); // 15.971 hours to go into S phase

        // Set up Wnt concentration
        double wnt_level = 1.0;
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_level);

        // Create cell cycle model and associated cell
        VanLeeuwen2009WntSwatCellCycleModelHypothesisTwo* p_cell_model = new VanLeeuwen2009WntSwatCellCycleModelHypothesisTwo();
        p_cell_model->SetDimension(2);
        p_cell_model->SetCellProliferativeType(STEM);

        boost::shared_ptr<AbstractCellMutationState> p_healthy_state(new WildTypeCellMutationState);
        CellPtr p_stem_cell(new Cell(p_healthy_state, p_cell_model));

        p_stem_cell->InitialiseCellCycleModel();

        /*
         * In this cell cycle model, there is no such thing as a 'stem cell'. Instead, the
         * cell proliferative type is changed to transit or differentiated, depending on the
         * Wnt concentration, when InitialiseCellCycleModel() is called.
         */
        TS_ASSERT_EQUALS(p_stem_cell->GetCellCycleModel()->GetCellProliferativeType(), TRANSIT);

        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(1.0);

        // These numbers (below) have been lifted from the above test for hypothesis one -
        // and so are probably not correct for hypothesis two if run for long enough

#ifdef CHASTE_CVODE
        const double expected_g1_duration = 6.18252;
#else
        const double expected_g1_duration = 6.1959;
#endif //CHASTE_CVODE

        // Progress through the cell cycle under a constant Wnt concentration
        for (unsigned i=0; i<num_timesteps; i++)
        {
            p_simulation_time->IncrementTimeOneStep();

            // Call ReadyToDivide on the cell, then test the results
            // of calling ReadyToDivide on the model and test (in
            // CheckReadyToDivideAndPhaseIsUpdated).
            p_stem_cell->ReadyToDivide();
            CheckReadyToDivideAndPhaseIsUpdated(p_cell_model, expected_g1_duration);
        }

        // The cell cycle model acts as if it was divided at time = 16.1877. This
        // is fine as the cell cycle model dictates the division time, not when
        // the cell is actually divided.
        CellPtr p_daughter_cell = p_stem_cell->Divide();
        AbstractCellCycleModel* p_cell_model2 = p_daughter_cell->GetCellCycleModel();

        TS_ASSERT_EQUALS(p_cell_model->GetCurrentCellCyclePhase(), M_PHASE);
        TS_ASSERT_EQUALS(p_cell_model2->GetCurrentCellCyclePhase(), M_PHASE);

        TS_ASSERT_EQUALS(p_cell_model->ReadyToDivide(), false);
        TS_ASSERT_EQUALS(p_cell_model2->ReadyToDivide(), false);

        // Tidy up
        WntConcentration<1>::Destroy();
        WntConcentration<2>::Destroy();
        WntConcentration<3>::Destroy();
    }

    void TestWntCellCycleModelForAPCSingleHit() throw(Exception)
    {
        // Set up simulation time
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        unsigned num_timesteps = 500;
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(40, num_timesteps); // 15.971 hours to go into S phase

        // Set up Wnt concentration
        double wnt_level = 1.0;
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_level);

        // Create cell cycle model and associated cell
        WntCellCycleModel* p_cell_model = new WntCellCycleModel();
        p_cell_model->SetDimension(2);
        p_cell_model->SetCellProliferativeType(STEM);

        boost::shared_ptr<AbstractCellMutationState> p_healthy_state(new WildTypeCellMutationState);

        CellPtr p_stem_cell(new Cell(p_healthy_state, p_cell_model));
        p_stem_cell->InitialiseCellCycleModel();

        double SG2M_duration = p_cell_model->GetSG2MDuration();
        TS_ASSERT_THROWS_NOTHING(WntCellCycleModel cell_model_3());

        // Create another cell cycle model and associated cell
        WntCellCycleModel* p_cell_model_1 = new WntCellCycleModel();
        p_cell_model_1->SetDimension(2);
        p_cell_model_1->SetCellProliferativeType(STEM);

        boost::shared_ptr<AbstractCellMutationState> p_mutation(new ApcOneHitCellMutationState);

        CellPtr p_stem_cell_1(new Cell(p_mutation, p_cell_model_1));
        p_stem_cell_1->InitialiseCellCycleModel();

        // Run the Wnt model for a full constant Wnt stimulus for 20 hours.
        // Model should enter S phase at 4.804 hrs and then finish dividing
        // 10 hours later at 14.804 hours.
#ifdef CHASTE_CVODE
        double expected_g1_duration = 4.79772;
#else
        double expected_g1_duration = 4.8084;
#endif //CHASTE_CVODE
        for (unsigned i=0; i<num_timesteps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            CheckReadyToDivideAndPhaseIsUpdated(p_cell_model_1, expected_g1_duration);
        }

        p_cell_model_1->ResetForDivision();
#ifdef CHASTE_CVODE
        expected_g1_duration = 4.80678;
#else
        expected_g1_duration = 4.8084;
#endif //CHASTE_CVODE

        TS_ASSERT_DELTA(SG2M_duration, 10.0, 1e-5);
        for (unsigned i=0; i<num_timesteps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            CheckReadyToDivideAndPhaseIsUpdated(p_cell_model_1, expected_g1_duration);
        }

        // Tidy up
        WntConcentration<2>::Destroy();
    }

    void TestWntCellCycleModelForBetaCatSingleHit() throw(Exception)
    {
        // Set up simulation time
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        unsigned num_timesteps = 500;
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(40, num_timesteps); // 15.971 hours to go into S phase

        // Set up Wnt concentration
        double wnt_level = 0.0;
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_level);

        // Create cell cycle model and associated cell
        WntCellCycleModel* p_cell_model = new WntCellCycleModel();
        p_cell_model->SetDimension(2);
        p_cell_model->SetCellProliferativeType(STEM);

        boost::shared_ptr<AbstractCellMutationState> p_mutation(new BetaCateninOneHitCellMutationState);

        CellPtr p_stem_cell(new Cell(p_mutation, p_cell_model));
        p_stem_cell->InitialiseCellCycleModel();

        TS_ASSERT_THROWS_NOTHING(WntCellCycleModel cell_model_3());

        // Create another cell cycle model and associated cell
        WntCellCycleModel* p_cell_model_1 = new WntCellCycleModel();
        p_cell_model_1->SetDimension(2);
        p_cell_model_1->SetCellProliferativeType(STEM);
        CellPtr p_stem_cell_1(new Cell(p_mutation, p_cell_model_1));
        p_stem_cell_1->InitialiseCellCycleModel();

        // Run the Wnt model for a full constant Wnt stimulus for 20 hours.
        // Model should enter S phase at 7.82 hrs and then finish dividing
        // 10 hours later at 17.82 hours.
#ifdef CHASTE_CVODE
        double expected_g1_duration = 7.81718;
#else
        double expected_g1_duration = 7.8342;
#endif //CHASTE_CVODE

        for (unsigned i=0; i<num_timesteps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            CheckReadyToDivideAndPhaseIsUpdated(p_cell_model_1, expected_g1_duration);
        }

        p_cell_model_1->ResetForDivision();
#ifdef CHASTE_CVODE
        expected_g1_duration = 7.81873;
#else
        expected_g1_duration = 7.8342;
#endif //CHASTE_CVODE

        // Test progress through the cell cycle
        for (unsigned i=0; i<num_timesteps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            CheckReadyToDivideAndPhaseIsUpdated(p_cell_model_1, expected_g1_duration);
        }

        // Tidy up
        WntConcentration<2>::Destroy();
    }

    void TestWntCellCycleModelForAPCDoubleHit() throw(Exception)
    {
        // Set up simulation time
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        unsigned num_timesteps = 500;
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(40, num_timesteps); // 15.971 hours to go into S phase

        // Set up Wnt concentration
        double wnt_level = 0.738; // the Wnt concentrationshouldn't matter for a cell with APC double hit
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_level);

        // Create cell cycle model and associated cell
        WntCellCycleModel* p_cell_model_1 = new WntCellCycleModel();
        p_cell_model_1->SetDimension(2);
        p_cell_model_1->SetCellProliferativeType(STEM);

        boost::shared_ptr<AbstractCellMutationState> p_mutation(new ApcTwoHitCellMutationState);

        CellPtr p_stem_cell_1(new Cell(p_mutation, p_cell_model_1));
        p_stem_cell_1->InitialiseCellCycleModel();

        // Create another cell cycle model and associated cell
        WntCellCycleModel* p_cell_model_2 = new WntCellCycleModel();
        p_cell_model_2->SetDimension(2);
        p_cell_model_2->SetCellProliferativeType(STEM);

        CellPtr p_stem_cell_2(new Cell(p_mutation, p_cell_model_2));
        p_stem_cell_2->InitialiseCellCycleModel();

        // Run the Wnt model for a full constant Wnt stimulus for 20 hours.
        // Model should enter S phase at 3.943 hrs and then finish dividing
        // 10 hours later at 13.9435 hours.
#ifdef CHASTE_CVODE
        double expected_g1_duration = 3.93959;
#else
        double expected_g1_duration = 3.9455;
#endif //CHASTE_CVODE

        // Test progress through the cell cycle
        for (unsigned i=0; i<num_timesteps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            CheckReadyToDivideAndPhaseIsUpdated(p_cell_model_2, expected_g1_duration);
        }

        p_cell_model_2->ResetForDivision();

#ifdef CHASTE_CVODE
        expected_g1_duration = 3.94529;
#else
        expected_g1_duration = 3.9455;
#endif //CHASTE_CVODE

        // Test progress through the cell cycle
        for (unsigned i=0; i<num_timesteps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            CheckReadyToDivideAndPhaseIsUpdated(p_cell_model_2, expected_g1_duration);
        }

        // Tidy up
        WntConcentration<2>::Destroy();
    }

    void TestWntCellCycleModelForConstantWntStimulusHealthyCell() throw(Exception)
    {
        // Set up simulation time
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        unsigned num_timesteps = 500;
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(40, num_timesteps);// 15.971 hours to go into S phase

        // Set up Wnt concentration
        double wnt_level = 1.0;
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_level);

        // Create cell cycle model and associated cell
        WntCellCycleModel* p_cell_model_1 = new WntCellCycleModel();
        p_cell_model_1->SetDimension(2);
        p_cell_model_1->SetCellProliferativeType(STEM);

        boost::shared_ptr<AbstractCellMutationState> p_healthy_state(new WildTypeCellMutationState);

        CellPtr p_stem_cell_1(new Cell(p_healthy_state, p_cell_model_1));
        p_stem_cell_1->InitialiseCellCycleModel();

        // Create another cell cycle model and associated cell
        WntCellCycleModel* p_cell_model_2 = new WntCellCycleModel();
        p_cell_model_2->SetDimension(2);
        p_cell_model_2->SetCellProliferativeType(STEM);

        CellPtr p_stem_cell_2(new Cell(p_healthy_state, p_cell_model_2));
        p_stem_cell_2->InitialiseCellCycleModel();

        // Run the Wnt model for a full constant Wnt stimulus for 20 hours.
        // Model should enter S phase at 5.971 hrs and then finish dividing
        // 10 hours later at 15.971 hours.
#ifdef CHASTE_CVODE
        double expected_g1_duration = 5.96441;
#else
        double expected_g1_duration = 5.9782;
#endif //CHASTE_CVODE

        // Test progress through the cell cycle
        for (unsigned i=0; i<num_timesteps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            CheckReadyToDivideAndPhaseIsUpdated(p_cell_model_2, expected_g1_duration);
        }

        p_cell_model_2->ResetForDivision();
#ifdef CHASTE_CVODE
        expected_g1_duration = 5.96016;
#else
        expected_g1_duration = 5.9782;
#endif //CHASTE_CVODE

        // Test progress through the cell cycle
        for (unsigned i=0; i<num_timesteps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            CheckReadyToDivideAndPhaseIsUpdated(p_cell_model_2, expected_g1_duration);
        }

        // Tidy up
        WntConcentration<2>::Destroy();
    }

    void TestStochasticWntCellCycleModel() throw (Exception)
    {
        // Set up simulation time
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        unsigned num_timesteps = 100;
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(20, num_timesteps);// 15.971 hours to go into S phase

        // Set up Wnt concentration
        double wnt_level = 1.0;
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_level);

        // Create cell cycle model and associated cell
        StochasticWntCellCycleModel* p_cell_model = new StochasticWntCellCycleModel();
        p_cell_model->SetDimension(2);
        p_cell_model->SetCellProliferativeType(STEM);

        boost::shared_ptr<AbstractCellMutationState> p_healthy_state(new WildTypeCellMutationState);

        CellPtr p_stem_cell(new Cell(p_healthy_state, p_cell_model));
        p_stem_cell->InitialiseCellCycleModel();

        // A WntCellCycleModel does this:
        // Run the Wnt model for a full constant Wnt stimulus for 20 hours.
        // Model should enter S phase at 5.971 hrs and then finish dividing
        // 10 hours later at 15.971 hours.
        //
        // A StochasticWntCellCycleModel does this:
        // divides at the same time with a random normal distribution
        // for the SG2M time (default 10) in this case 9.0676

        // Test progress through the cell cycle
        for (unsigned i=0; i<num_timesteps; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            double time = p_simulation_time->GetTime();
            bool result = p_cell_model->ReadyToDivide();

            if (time < 5.971 + 9.0676)
            {
                TS_ASSERT_EQUALS(result, false);
            }
            else
            {
                TS_ASSERT_EQUALS(result, true);
            }
        }

        StochasticWntCellCycleModel* p_cell_model2 = new StochasticWntCellCycleModel();
        p_cell_model2->SetDimension(2);
        p_cell_model2->SetCellProliferativeType(STEM);

        // Coverage
        p_cell_model2->SetMinimumGapDuration(1e20);

        CellPtr p_cell2(new Cell(p_healthy_state, p_cell_model2));
        p_cell2->InitialiseCellCycleModel();

        TS_ASSERT_DELTA(p_cell_model2->GetG2Duration(), 1e20, 1e-4);

        // Tidy up
        WntConcentration<2>::Destroy();
    }

    void TestArchiveWntCellCycleModel()
    {
        // Set up
        OutputFileHandler handler("archive", false);
        std::string archive_filename = handler.GetOutputDirectoryFullPath() + "wnt_cell_cycle.arch";
        WntConcentration<3>::Instance()->SetConstantWntValueForTesting(1.0);

        {
            // Set up simulation time
            SimulationTime* p_simulation_time = SimulationTime::Instance();
            p_simulation_time->SetEndTimeAndNumberOfTimeSteps(16, 2);

            // Create cell cycle model and associated cell
            WntCellCycleModel* p_cell_model = new WntCellCycleModel();
            p_cell_model->SetDimension(3);
            p_cell_model->SetCellProliferativeType(STEM);

            boost::shared_ptr<AbstractCellMutationState> p_healthy_state(new WildTypeCellMutationState);

            CellPtr p_stem_cell(new Cell(p_healthy_state, p_cell_model));
            p_stem_cell->InitialiseCellCycleModel();

            p_simulation_time->IncrementTimeOneStep();
            TS_ASSERT_EQUALS(p_stem_cell->GetCellCycleModel()->ReadyToDivide(), false);
            p_simulation_time->IncrementTimeOneStep();
            TS_ASSERT_EQUALS(p_stem_cell->GetCellCycleModel()->ReadyToDivide(), true);

            // Should be in G2 after a couple of timesteps
            TS_ASSERT_EQUALS(p_cell_model->GetCurrentCellCyclePhase(), G_TWO_PHASE);

            p_stem_cell->GetCellCycleModel()->SetBirthTime(-1.0);

            // Create an output archive
            std::ofstream ofs(archive_filename.c_str());
            boost::archive::text_oarchive output_arch(ofs);

            // Archive cell
            CellPtr const p_const_cell = p_stem_cell;
            output_arch << p_const_cell;

            SimulationTime::Destroy();
        }

        {
            // Set up simulation time
            SimulationTime* p_simulation_time = SimulationTime::Instance();
            p_simulation_time->SetStartTime(0.0);
            p_simulation_time->SetEndTimeAndNumberOfTimeSteps(1.0, 1);

            CellPtr p_cell;

            // Create an input archive
            std::ifstream ifs(archive_filename.c_str(), std::ios::binary);
            boost::archive::text_iarchive input_arch(ifs);

            // Restore from the archive
            input_arch >> p_cell;

            // Test archiving
            AbstractCellCycleModel* p_cell_model = p_cell->GetCellCycleModel();
            TS_ASSERT_EQUALS(p_cell, p_cell_model->GetCell());

            TS_ASSERT_EQUALS(p_cell_model->ReadyToDivide(), true);
            TS_ASSERT_DELTA(p_cell_model->GetBirthTime(), -1.0, 1e-12);
            TS_ASSERT_DELTA(p_cell_model->GetAge(), 17.0, 1e-12);
            TS_ASSERT_DELTA(p_cell_model->GetSG2MDuration(), 10.0, 1e-12);
            TS_ASSERT_EQUALS(p_cell_model->GetCurrentCellCyclePhase(), G_TWO_PHASE);
            TS_ASSERT_EQUALS((static_cast<WntCellCycleModel*>(p_cell_model))->GetDimension(), 3u);
        }

        // Tidy up
        WntConcentration<2>::Destroy();
        WntConcentration<3>::Destroy();
    }

    void TestArchiveVanLeeuwen2009WntSwatCellCycleModelHypothesisOne()
    {
        // Set up
        OutputFileHandler handler("archive", false);
        std::string archive_filename = handler.GetOutputDirectoryFullPath() + "wnt_swat_one.arch";
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(1.0);

        {
            // Set up simulation time
            SimulationTime* p_simulation_time = SimulationTime::Instance();
            p_simulation_time->SetEndTimeAndNumberOfTimeSteps(17, 2);

            // Create cell cycle model and associated cell
            AbstractVanLeeuwen2009WntSwatCellCycleModel* p_cell_model = new VanLeeuwen2009WntSwatCellCycleModelHypothesisOne();
            p_cell_model->SetDimension(2);
            p_cell_model->SetCellProliferativeType(STEM);

            boost::shared_ptr<AbstractCellMutationState> p_healthy_state(new WildTypeCellMutationState);

            CellPtr p_stem_cell(new Cell(p_healthy_state, p_cell_model));
            p_stem_cell->InitialiseCellCycleModel();

            p_simulation_time->IncrementTimeOneStep();
            TS_ASSERT_EQUALS(p_stem_cell->GetCellCycleModel()->ReadyToDivide(), false);
            p_simulation_time->IncrementTimeOneStep();
            TS_ASSERT_EQUALS(p_stem_cell->GetCellCycleModel()->ReadyToDivide(), true);

            p_stem_cell->GetCellCycleModel()->SetBirthTime(-1.0);

            // Create an output archive
            std::ofstream ofs(archive_filename.c_str());
            boost::archive::text_oarchive output_arch(ofs);

            // Archive cell
            CellPtr const p_const_cell = p_stem_cell;
            output_arch << p_const_cell;

            SimulationTime::Destroy();
        }

        {
            // Set up simulation time
            SimulationTime* p_simulation_time = SimulationTime::Instance();
            p_simulation_time->SetStartTime(0.0);
            p_simulation_time->SetEndTimeAndNumberOfTimeSteps(1.0, 1);

            CellPtr p_cell;

            // Create an input archive
            std::ifstream ifs(archive_filename.c_str(), std::ios::binary);
            boost::archive::text_iarchive input_arch(ifs);

            // Restore from the archive
            input_arch >> p_cell;

            // Test archiving
            AbstractCellCycleModel* p_cell_model = p_cell->GetCellCycleModel();
            TS_ASSERT_EQUALS(p_cell, p_cell_model->GetCell());

            TS_ASSERT_EQUALS(p_cell_model->ReadyToDivide(), true);
            TS_ASSERT_DELTA(p_cell_model->GetBirthTime(), -1.0, 1e-12);
            TS_ASSERT_DELTA(p_cell_model->GetAge(), 18.0, 1e-12);
            TS_ASSERT_DELTA(p_cell_model->GetSG2MDuration(), 10.0, 1e-12);
            TS_ASSERT_EQUALS(p_cell_model->GetCurrentCellCyclePhase(), G_TWO_PHASE);
        }

        // Tidy up
        WntConcentration<2>::Destroy();
    }

    void TestArchiveVanLeeuwen2009WntSwatCellCycleModelHypothesisTwo()
    {
        // Set up
        OutputFileHandler handler("archive", false);
        std::string archive_filename = handler.GetOutputDirectoryFullPath() + "wnt_swat_two.arch";
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(1.0);

        {
            // Set up simulation time
            SimulationTime* p_simulation_time = SimulationTime::Instance();
            p_simulation_time->SetEndTimeAndNumberOfTimeSteps(17, 2);

            // Create cell cycle model and associated cell
            AbstractVanLeeuwen2009WntSwatCellCycleModel* p_cell_model = new VanLeeuwen2009WntSwatCellCycleModelHypothesisTwo();
            p_cell_model->SetDimension(2);
            p_cell_model->SetCellProliferativeType(STEM);

            boost::shared_ptr<AbstractCellMutationState> p_healthy_state(new WildTypeCellMutationState);

            CellPtr p_stem_cell(new Cell(p_healthy_state, p_cell_model));
            p_stem_cell->InitialiseCellCycleModel();

            p_simulation_time->IncrementTimeOneStep();
            TS_ASSERT_EQUALS(p_stem_cell->GetCellCycleModel()->ReadyToDivide(), false);
            p_simulation_time->IncrementTimeOneStep();
            TS_ASSERT_EQUALS(p_stem_cell->GetCellCycleModel()->ReadyToDivide(), true);

            p_stem_cell->GetCellCycleModel()->SetBirthTime(-1.0);

            // Create an output archive
            std::ofstream ofs(archive_filename.c_str());
            boost::archive::text_oarchive output_arch(ofs);

            // Archive cell
            CellPtr const p_const_cell = p_stem_cell;
            output_arch << p_const_cell;

            SimulationTime::Destroy();
        }

        {
            // Set up simulation time
            SimulationTime* p_simulation_time = SimulationTime::Instance();
            p_simulation_time->SetStartTime(0.0);
            p_simulation_time->SetEndTimeAndNumberOfTimeSteps(1.0, 1);

            CellPtr p_cell;

            // Create an input archive
            std::ifstream ifs(archive_filename.c_str(), std::ios::binary);
            boost::archive::text_iarchive input_arch(ifs);

            // Restore from the archive
            input_arch >> p_cell;

            // Test archiving
            AbstractCellCycleModel* p_cell_model = p_cell->GetCellCycleModel();
            TS_ASSERT_EQUALS(p_cell, p_cell_model->GetCell());

            TS_ASSERT_EQUALS(p_cell_model->ReadyToDivide(), true);
            TS_ASSERT_DELTA(p_cell_model->GetBirthTime(), -1.0, 1e-12);
            TS_ASSERT_DELTA(p_cell_model->GetAge(), 18.0, 1e-12);
            TS_ASSERT_DELTA(p_cell_model->GetSG2MDuration(), 10.0, 1e-12);
            TS_ASSERT_EQUALS(p_cell_model->GetCurrentCellCyclePhase(), G_TWO_PHASE);
        }

        // Tidy up
        WntConcentration<2>::Destroy();
    }

    void TestArchiveStochasticWntCellCycleModels()
    {
        // In this case the first cycle time will be 5.971+9.0676 = 15.0386
        // note that the S-G2-M time is assigned when the cell finishes G1
        // (i.e. at time 5.971 here so the model has to be archived BEFORE that.

        // Set up
        OutputFileHandler handler("archive", false);
        std::string archive_filename = handler.GetOutputDirectoryFullPath() + "stochastic_wnt_cell_cycle.arch";
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(1.0);

        {
            // In this test the RandomNumberGenerator in existence

            // Set up simulation time
            SimulationTime* p_simulation_time = SimulationTime::Instance();
            p_simulation_time->SetEndTimeAndNumberOfTimeSteps(16.0, 1000);

            // Create cell cycle model and associated cell
            StochasticWntCellCycleModel* p_stoc_model = new StochasticWntCellCycleModel();
            p_stoc_model->SetDimension(2);
            p_stoc_model->SetCellProliferativeType(STEM);

            boost::shared_ptr<AbstractCellMutationState> p_healthy_state(new WildTypeCellMutationState);

            CellPtr p_stoc_cell(new Cell(p_healthy_state, p_stoc_model));
            p_stoc_cell->InitialiseCellCycleModel();

            // Create another cell cycle model and associated cell
            WntCellCycleModel* p_wnt_model = new WntCellCycleModel();
            p_wnt_model->SetDimension(2);
            p_wnt_model->SetCellProliferativeType(STEM);

            CellPtr p_wnt_cell(new Cell(p_healthy_state, p_wnt_model));
            p_wnt_cell->InitialiseCellCycleModel();

            p_simulation_time->IncrementTimeOneStep(); // 5.5

            while (p_simulation_time->GetTime() < 4.0)
            {
                p_simulation_time->IncrementTimeOneStep();
            }

            TS_ASSERT_EQUALS(p_stoc_cell->GetCellCycleModel()->ReadyToDivide(), false);
            TS_ASSERT_EQUALS(p_wnt_cell->GetCellCycleModel()->ReadyToDivide(), false);
            TS_ASSERT_EQUALS(p_stoc_cell->GetCellCycleModel()->GetCurrentCellCyclePhase(), G_ONE_PHASE);

            // When the above tests are included here they pass, so we
            // also put them after the load to see if they still pass.

            // Create an output archive
            std::ofstream ofs(archive_filename.c_str());
            boost::archive::text_oarchive output_arch(ofs);

            // Archive cells
            CellPtr const p_const_stoc_cell = p_stoc_cell;
            CellPtr const p_const_wnt_cell = p_wnt_cell;
            output_arch << p_const_stoc_cell;
            output_arch << p_const_wnt_cell;

            SimulationTime::Destroy();
        }

        {
            // Set up simulation time
            SimulationTime* p_simulation_time = SimulationTime::Instance();
            p_simulation_time->SetStartTime(0.0);
            p_simulation_time->SetEndTimeAndNumberOfTimeSteps(16.0, 2);

            CellPtr p_stoc_cell;
            CellPtr p_wnt_cell;

            std::vector<double> cell_cycle_influence1;
            cell_cycle_influence1.push_back(1.0);

            // Create an input archive
            std::ifstream ifs(archive_filename.c_str(), std::ios::binary);
            boost::archive::text_iarchive input_arch(ifs);

            // Restore from the archive
            input_arch >> p_stoc_cell;
            input_arch >> p_wnt_cell;

            // Test archiving
            TS_ASSERT_EQUALS(p_stoc_cell->GetCellCycleModel()->GetCurrentCellCyclePhase(), G_ONE_PHASE);

            // Check - stochastic should divide at 15.03
            // Wnt should divide at 15.971
            while (p_simulation_time->GetTime() < 15.0)
            {
                p_simulation_time->IncrementTimeOneStep();
            }

            TS_ASSERT_EQUALS(p_stoc_cell->GetCellCycleModel()->ReadyToDivide(), false);
            TS_ASSERT_EQUALS(p_wnt_cell->GetCellCycleModel()->ReadyToDivide(), false);

            while (p_simulation_time->GetTime() < 15.5)
            {
                p_simulation_time->IncrementTimeOneStep();
            }

            TS_ASSERT_EQUALS(p_stoc_cell->GetCellCycleModel()->ReadyToDivide(), true); // only for stochastic
            TS_ASSERT_EQUALS(p_wnt_cell->GetCellCycleModel()->ReadyToDivide(), false);

            while (p_simulation_time->GetTime() < 16.0)
            {
                p_simulation_time->IncrementTimeOneStep();
            }

            TS_ASSERT_EQUALS(p_stoc_cell->GetCellCycleModel()->ReadyToDivide(), true);
            TS_ASSERT_EQUALS(p_wnt_cell->GetCellCycleModel()->ReadyToDivide(), true);

            TS_ASSERT_DELTA(p_stoc_cell->GetCellCycleModel()->GetBirthTime(), 0.0, 1e-12);
            TS_ASSERT_DELTA(p_stoc_cell->GetCellCycleModel()->GetAge(), 16.0, 1e-12);
            TS_ASSERT_DELTA(p_stoc_cell->GetCellCycleModel()->GetSG2MDuration(), 10.0, 1e-12);
        }

        // Tidy up
        WntConcentration<2>::Destroy();
    }
};

#endif /*TESTODEBASEDCELLCYCLEMODELSFORCRYPT_HPP_*/