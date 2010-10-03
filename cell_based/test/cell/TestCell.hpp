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
#ifndef TESTCELL_HPP_
#define TESTCELL_HPP_

#include <cxxtest/TestSuite.h>

#include <fstream>
#include <iostream>

#include "Cell.hpp"
#include "AbstractCellBasedTestSuite.hpp"
#include "WildTypeCellMutationState.hpp"
#include "FixedDurationGenerationBasedCellCycleModel.hpp"
#include "CellPropertyRegistry.hpp"
#include "CellLabel.hpp"
#include "ApcTwoHitCellMutationState.hpp"
#include "ApcOneHitCellMutationState.hpp"
#include "StochasticWntCellCycleModel.hpp"
#include "StochasticDurationGenerationBasedCellCycleModel.hpp"
#include "TysonNovakCellCycleModel.hpp"


class TestCell: public AbstractCellBasedTestSuite
{
public:

    void TestCellConstructor() throw(Exception)
    {
    	// Set up SimulationTime
    	SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(200, 20);

        // Create a cell mutation state
        boost::shared_ptr<AbstractCellProperty> p_state(new WildTypeCellMutationState);

        // Create a cell cycle model
        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
        p_model->SetCellProliferativeType(TRANSIT);

        // Create a cell
        CellPtr p_cell(new Cell(p_state, p_model));
        p_cell->SetBirthTime(-0.5);

        // Test members of cell directly
        TS_ASSERT_EQUALS(p_cell->GetCellId(), 0u);
        TS_ASSERT_DELTA(p_cell->GetBirthTime(), -0.5, 1e-6);

        // Test members of cell through cell cycle model
        TS_ASSERT_EQUALS(p_cell->GetCellCycleModel()->GetCell()->GetCellId(), 0u);
        TS_ASSERT_DELTA(p_cell->GetCellCycleModel()->GetCell()->GetBirthTime(), -0.5, 1e-6);
    }

    void TestWithCellPropertyCollection() throw(Exception)
    {
    	// Set up SimulationTime
    	SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(24, 3);

        // Create a cell mutation state
        boost::shared_ptr<AbstractCellProperty> p_wild_type(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        // Create a cell cycle model
        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
        p_model->SetCellProliferativeType(STEM);

        boost::shared_ptr<AbstractCellProperty> p_label(new CellLabel);
        boost::shared_ptr<AbstractCellProperty> p_label_2(new CellLabel);
        boost::shared_ptr<AbstractCellProperty> p_apc2_mutation(new ApcTwoHitCellMutationState);

        // Create a cell property collection (for the time being, populate with mutation states)
        CellPropertyCollection collection;
        collection.AddProperty(p_label);

        // Create a cell
        CellPtr p_cell(new Cell(p_wild_type, p_model, false, collection));
        p_cell->SetBirthTime(-1.0);
        p_cell->InitialiseCellCycleModel();

        TS_ASSERT_EQUALS(static_cast<FixedDurationGenerationBasedCellCycleModel*>(p_cell->GetCellCycleModel())->GetGeneration(), 0u);

        // Test cell property collection
        TS_ASSERT_EQUALS(p_cell->rGetCellPropertyCollection().GetSize(), 2u);
        TS_ASSERT_EQUALS(p_cell->rGetCellPropertyCollection().HasProperty(p_wild_type), true);
        TS_ASSERT_EQUALS(p_cell->rGetCellPropertyCollection().HasProperty(p_label), true);
        TS_ASSERT_EQUALS(p_cell->rGetCellPropertyCollection().HasProperty(p_apc2_mutation), false);
        TS_ASSERT_EQUALS(p_cell->HasCellProperty<WildTypeCellMutationState>(), true);
        TS_ASSERT_EQUALS(p_cell->HasCellProperty<CellLabel>(), true);
        TS_ASSERT_EQUALS(p_cell->HasCellProperty<ApcTwoHitCellMutationState>(), false);
        TS_ASSERT_EQUALS(p_cell->rGetCellPropertyCollection().HasPropertyType<AbstractCellProperty>(), true);
        TS_ASSERT_EQUALS(p_cell->rGetCellPropertyCollection().HasPropertyType<AbstractCellMutationState>(), true);

        // Test creating another cell with the same cell property collection
        TS_ASSERT_EQUALS(p_wild_type->GetCellCount(), 1u);

        FixedDurationGenerationBasedCellCycleModel* p_model2 = new FixedDurationGenerationBasedCellCycleModel();
        p_model2->SetCellProliferativeType(STEM);
        CellPtr p_cell2(new Cell(p_wild_type, p_model2, false, collection));
        p_cell2->SetBirthTime(-1.0);
        p_cell2->InitialiseCellCycleModel();

        TS_ASSERT_EQUALS(p_wild_type->GetCellCount(), 2u);

        // Test adding a cell property
        boost::shared_ptr<AbstractCellProperty> p_apoptotic(CellPropertyRegistry::Instance()->Get<ApoptoticCellProperty>());

        TS_ASSERT_EQUALS(p_cell->rGetCellPropertyCollection().HasProperty(p_apoptotic), false);
        TS_ASSERT_EQUALS(p_apoptotic->GetCellCount(), 0u);

        p_cell->AddCellProperty(p_apoptotic);

        TS_ASSERT_EQUALS(p_cell->rGetCellPropertyCollection().HasProperty(p_apoptotic), true);
        TS_ASSERT_EQUALS(p_apoptotic->GetCellCount(), 1u);

        CellPropertyCollection apoptotic_collection = p_cell->rGetCellPropertyCollection().GetProperties<ApoptoticCellProperty>();
        TS_ASSERT_EQUALS(apoptotic_collection.GetProperty()->GetCellCount(), 1u);

        // Test removing a cell property
        p_cell->RemoveCellProperty<ApoptoticCellProperty>();

        TS_ASSERT_EQUALS(p_cell->rGetCellPropertyCollection().HasProperty(p_apoptotic), false);
        TS_ASSERT_EQUALS(p_apoptotic->GetCellCount(), 0u);

        // Now age cell
        p_simulation_time->IncrementTimeOneStep(); // t=8
        TS_ASSERT_EQUALS(p_cell->ReadyToDivide(), false);

        p_simulation_time->IncrementTimeOneStep(); // t=16
        TS_ASSERT_EQUALS(p_cell->ReadyToDivide(), false);

        p_simulation_time->IncrementTimeOneStep(); // t=24
        TS_ASSERT_EQUALS(p_cell->ReadyToDivide(), true);

        // Divide cell
        CellPtr p_daughter_cell = p_cell->Divide();

        TS_ASSERT_EQUALS(p_cell->ReadyToDivide(), false);
        TS_ASSERT_EQUALS(static_cast<FixedDurationGenerationBasedCellCycleModel*>(p_daughter_cell->GetCellCycleModel())->GetGeneration(), 1u);
        TS_ASSERT_EQUALS(p_daughter_cell->GetCellCycleModel()->GetCellProliferativeType(), TRANSIT);
        TS_ASSERT_DELTA(p_daughter_cell->GetAge(), 0.0, 1e-9);

        // Test cell property collection has been inherited correctly during division
        TS_ASSERT_EQUALS(p_daughter_cell->rGetCellPropertyCollection().GetSize(), 2u);

        TS_ASSERT_EQUALS(p_daughter_cell->rGetCellPropertyCollection().HasProperty(p_wild_type), true);
        TS_ASSERT_EQUALS(p_daughter_cell->rGetCellPropertyCollection().HasProperty(p_apc2_mutation), false);
        TS_ASSERT_EQUALS(p_daughter_cell->HasCellProperty<WildTypeCellMutationState>(), true);
        TS_ASSERT_EQUALS(p_daughter_cell->HasCellProperty<ApcOneHitCellMutationState>(), false);
        TS_ASSERT_EQUALS(p_daughter_cell->HasCellProperty<ApcTwoHitCellMutationState>(), false);
        TS_ASSERT_EQUALS(p_daughter_cell->rGetCellPropertyCollection().HasPropertyType<AbstractCellProperty>(), true);
        TS_ASSERT_EQUALS(p_daughter_cell->rGetCellPropertyCollection().HasPropertyType<AbstractCellMutationState>(), true);

        TS_ASSERT(&(p_daughter_cell->rGetCellPropertyCollection()) != &(p_cell->rGetCellPropertyCollection()));


        TS_ASSERT_EQUALS(p_cell->HasCellProperty<WildTypeCellMutationState>(), true);
        TS_ASSERT_EQUALS(p_daughter_cell->HasCellProperty<WildTypeCellMutationState>(), true);
        unsigned num_wild_type = CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>()->GetCellCount();
        TS_ASSERT_EQUALS(num_wild_type, 3u);

        /// \todo #1285 Check that the registry gets updated...
        p_daughter_cell->Kill();
        //num_wild_type = CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>()->GetCellCount();
        //TS_ASSERT_EQUALS(num_wild_type, 2u);

        p_cell->Kill();
        //num_wild_type = CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>()->GetCellCount();
        //TS_ASSERT_EQUALS(num_wild_type, 1u);

        p_cell2->Kill();
        //num_wild_type = CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>()->GetCellCount();
        //TS_ASSERT_EQUALS(num_wild_type, 0u);

    }

    void TestCellsAgeingCorrectly() throw(Exception)
    {
        // These lines are added to cover the exception case that a cell is
        // created without simulation time being set up...
        FixedDurationGenerationBasedCellCycleModel fixed_model;
        SimulationTime::Destroy();

        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());
        boost::shared_ptr<AbstractCellProperty> p_label(CellPropertyRegistry::Instance()->Get<CellLabel>());

        TS_ASSERT_THROWS_THIS(CellPtr p_bad_cell(new Cell(p_healthy_state, &fixed_model)),
                              "Cell is setting up a cell cycle model but SimulationTime has not been set up");

        // Cell wasn't created - count should be zero
        TS_ASSERT_EQUALS(p_healthy_state->GetCellCount(), 0u);

        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetStartTime(0.0);
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(2.0, 4);

        TS_ASSERT_THROWS_THIS(CellPtr p_stem_cell(new Cell(p_healthy_state, NULL)),
                              "Cell cycle model is null");

        // Cell wasn't created - count should be zero
        TS_ASSERT_EQUALS(p_healthy_state->GetCellCount(), 0u);

        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
        p_model->SetCellProliferativeType(STEM);

        TS_ASSERT_THROWS_THIS(CellPtr p_another_bad_cell(new Cell(p_label, &fixed_model)),
                              "Attempting to create cell with a cell mutation state is not a subtype of AbstractCellMutationState");

        CellPtr p_stem_cell(new Cell(p_healthy_state, p_model));
        p_stem_cell->InitialiseCellCycleModel();

        TS_ASSERT_THROWS_THIS(p_stem_cell->SetMutationState(p_label),
                              "Attempting to give cell a cell mutation state is not a subtype of AbstractCellMutationState");

        // Cell was created - count should be one
        TS_ASSERT_EQUALS(p_healthy_state->GetCellCount(), 1u);

        p_simulation_time->IncrementTimeOneStep();

        TS_ASSERT_EQUALS(p_stem_cell->GetAge(), 0.5);

        p_simulation_time->IncrementTimeOneStep();
        p_stem_cell->SetBirthTime(p_simulation_time->GetTime());
        p_simulation_time->IncrementTimeOneStep();
        p_simulation_time->IncrementTimeOneStep();
        TS_ASSERT_EQUALS(p_stem_cell->GetAge(), 1.0);

        TS_ASSERT_EQUALS(p_stem_cell->IsDead(), false);
        p_stem_cell->Kill();
        TS_ASSERT_EQUALS(p_stem_cell->IsDead(), true);

        // Coverage of operator equals.
        FixedDurationGenerationBasedCellCycleModel* p_model2 = new FixedDurationGenerationBasedCellCycleModel();
        p_model2->SetCellProliferativeType(STEM);
        CellPtr p_live_cell(new Cell(p_healthy_state, p_model2));

        TS_ASSERT_EQUALS(p_live_cell->IsDead(), false);
        p_live_cell = p_stem_cell;
        TS_ASSERT_EQUALS(p_live_cell->IsDead(), true);
    }

    void TestCellDivision()
    {
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(54.0, 9);

        // We are going to start at t=0 and jump up in steps of 6.0
        p_simulation_time->IncrementTimeOneStep(); //t=6

        // Cover bad cell cycle model

        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        TS_ASSERT_THROWS_THIS(CellPtr p_bad_cell2(new Cell(p_healthy_state, NULL)), "Cell cycle model is null");

        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
        p_model->SetCellProliferativeType(STEM);
        CellPtr p_stem_cell(new Cell(p_healthy_state, p_model));
        p_stem_cell->InitialiseCellCycleModel();

        // This test needs particular cell cycle times
        TS_ASSERT_DELTA(p_model->GetStemCellG1Duration(), 14.0, 1e-12);
        TS_ASSERT_DELTA(p_model->GetTransitCellG1Duration(), 2.0, 1e-12);
        TS_ASSERT_DELTA(p_model->GetSG2MDuration(), 10.0, 1e-12);

        // Test coverage of operator=
        FixedDurationGenerationBasedCellCycleModel* p_model2 = new FixedDurationGenerationBasedCellCycleModel();
        p_model2->SetCellProliferativeType(TRANSIT);
        CellPtr p_other_cell(new Cell(p_healthy_state, p_model2));
        p_other_cell->InitialiseCellCycleModel();

        // This test needs particular cell cycle times
        TS_ASSERT_DELTA(p_model2->GetStemCellG1Duration(), 14.0, 1e-12);
        TS_ASSERT_DELTA(p_model2->GetTransitCellG1Duration(), 2.0, 1e-12);
        TS_ASSERT_DELTA(p_model2->GetSG2MDuration(), 10.0, 1e-12);

        TS_ASSERT_EQUALS(p_other_cell->GetCellCycleModel()->GetCellProliferativeType(), TRANSIT);
        p_other_cell = p_stem_cell;
        TS_ASSERT_EQUALS(p_other_cell->GetCellCycleModel()->GetCellProliferativeType(), STEM);

        // Back to the test
        p_simulation_time->IncrementTimeOneStep(); //t=12
        p_simulation_time->IncrementTimeOneStep(); //t=18
        p_simulation_time->IncrementTimeOneStep(); //t=24

        TS_ASSERT_EQUALS(p_stem_cell->ReadyToDivide(), false);

        p_simulation_time->IncrementTimeOneStep(); //t=30

        TS_ASSERT_EQUALS(p_stem_cell->ReadyToDivide(), true);

        // Create transit progeny of stem
        CellPtr p_daughter_cell = p_stem_cell->Divide();

        TS_ASSERT_EQUALS(p_stem_cell->ReadyToDivide(), false);
        TS_ASSERT_EQUALS(static_cast<FixedDurationGenerationBasedCellCycleModel*>(p_daughter_cell->GetCellCycleModel())->GetGeneration(), 1u);
        TS_ASSERT_EQUALS(p_daughter_cell->GetCellCycleModel()->GetCellProliferativeType(), TRANSIT);
        TS_ASSERT_DELTA(p_daughter_cell->GetAge(), 0, 1e-9);

        p_simulation_time->IncrementTimeOneStep(); //t=36

        TS_ASSERT_EQUALS(p_daughter_cell->ReadyToDivide(), false);

        p_simulation_time->IncrementTimeOneStep(); //t=42

        TS_ASSERT_EQUALS(p_daughter_cell->ReadyToDivide(), true);

        // Create transit progeny of transit
        CellPtr p_grandaughter_cell = p_daughter_cell->Divide();

        p_simulation_time->IncrementTimeOneStep(); //t=48

        TS_ASSERT_EQUALS(p_stem_cell->ReadyToDivide(), false);
        TS_ASSERT_EQUALS(p_grandaughter_cell->ReadyToDivide(), false);
        TS_ASSERT_EQUALS(p_daughter_cell->ReadyToDivide(), false);

        // Stem cell ready to divide again
        p_simulation_time->IncrementTimeOneStep(); //t=54
        TS_ASSERT_EQUALS(p_stem_cell->ReadyToDivide(), true);

        // Both grandaughter and daughter cells should be ready to divide
        TS_ASSERT_EQUALS(p_grandaughter_cell->ReadyToDivide(), true);
        TS_ASSERT_EQUALS(p_daughter_cell->ReadyToDivide(), true);
    }

    /*
     * ReadyToDivide() now calls UpdateCellProliferativeType() where appropriate.
     * (at the moment in Wnt-dependent cells).
     */
    void TestUpdateCellProliferativeTypes() throw (Exception)
    {
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(200, 20);
        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
        p_model->SetCellProliferativeType(STEM);
        CellPtr p_stem_cell(new Cell(p_healthy_state, p_model));
        p_stem_cell->InitialiseCellCycleModel();
        p_stem_cell->ReadyToDivide();

        TS_ASSERT_EQUALS(p_stem_cell->GetCellCycleModel()->GetCellProliferativeType(),STEM);

        p_stem_cell->GetCellCycleModel()->SetCellProliferativeType(TRANSIT);

        p_stem_cell->ReadyToDivide();

        TS_ASSERT_EQUALS(p_stem_cell->GetCellCycleModel()->GetCellProliferativeType(),TRANSIT);

        // Test a Wnt dependent cell
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(0.0);

        WntCellCycleModel* p_cell_cycle_model1 = new WntCellCycleModel();
        p_cell_cycle_model1->SetDimension(2);
        p_cell_cycle_model1->SetCellProliferativeType(TRANSIT);
        CellPtr p_wnt_cell(new Cell(p_healthy_state, p_cell_cycle_model1));

        TS_ASSERT_EQUALS(p_wnt_cell->GetCellCycleModel()->GetCellProliferativeType(),TRANSIT);

        p_wnt_cell->InitialiseCellCycleModel();

        TS_ASSERT_EQUALS(p_wnt_cell->GetCellCycleModel()->GetCellProliferativeType(),DIFFERENTIATED);

        p_wnt_cell->ReadyToDivide();

        TS_ASSERT_EQUALS(p_wnt_cell->GetCellCycleModel()->GetCellProliferativeType(),DIFFERENTIATED);

        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(1.0);

        // Go forward through time
        for (unsigned i=0; i<20; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
        }

        p_wnt_cell->ReadyToDivide();

        TS_ASSERT_EQUALS(p_wnt_cell->GetCellCycleModel()->GetCellProliferativeType(),TRANSIT);

        // Tidy up
        WntConcentration<2>::Destroy();
    }

    void Test0DBucket()
    {
        double end_time = 61.0;
        int time_steps = 61;

        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(end_time, time_steps);
        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
        p_model->SetCellProliferativeType(STEM);
        CellPtr p_stem_cell(new Cell(p_healthy_state, p_model));
        p_stem_cell->InitialiseCellCycleModel();

        std::vector<CellPtr> cells;
        std::vector<CellPtr> newly_born;
        std::vector<unsigned> stem_cells(time_steps);
        std::vector<unsigned> transit_cells(time_steps);
        std::vector<unsigned> differentiated_cells(time_steps);
        std::vector<double> times(time_steps);

        cells.push_back(p_stem_cell);
        std::vector<CellPtr>::iterator cell_iterator;

        unsigned i=0;
        while (p_simulation_time->GetTime()< end_time)
        {
            // Produce the offspring of the cells
            p_simulation_time->IncrementTimeOneStep();
            cell_iterator = cells.begin();
            while (cell_iterator < cells.end())
            {
                if ((*cell_iterator)->ReadyToDivide())
                {
                    newly_born.push_back((*cell_iterator)->Divide());
                }
                ++cell_iterator;
            }

            // Copy offspring in newly_born vector to cells vector
            cell_iterator = newly_born.begin();
            while (cell_iterator < newly_born.end())
            {
                cells.push_back(*cell_iterator);
                ++cell_iterator;
            }
            newly_born.clear();

            // Update cell counts
            cell_iterator = cells.begin();
            while (cell_iterator < cells.end())
            {
                switch ((*cell_iterator)->GetCellCycleModel()->GetCellProliferativeType())
                {
                    case STEM:
                        stem_cells[i]++;
                        break;
                    case TRANSIT:
                        transit_cells[i]++;
                        break;
                    default:
                        differentiated_cells[i]++;
                        break;
                }

                ++cell_iterator;
            }
            times[i] = p_simulation_time->GetTime();
            i++;
        }
        TS_ASSERT_EQUALS(stem_cells[59], 1u);
        TS_ASSERT_EQUALS(transit_cells[59], 2u);
        TS_ASSERT_EQUALS(differentiated_cells[59], 8u);
    }

    void TestWithFixedDurationGenerationBasedCellCycleModel() throw(Exception)
    {
        // Simulation time is 6000 because we want to test that differentiated cells never divide.

        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(6000.0, 1000);

        p_simulation_time->IncrementTimeOneStep();
        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        //  Creating different types of cells with different cell cycle models at SimulationTime = 6 hours
        FixedDurationGenerationBasedCellCycleModel* p_stem_model = new FixedDurationGenerationBasedCellCycleModel();
        p_stem_model->SetCellProliferativeType(STEM);
        CellPtr p_stem_cell(new Cell(p_healthy_state, p_stem_model));
        p_stem_cell->InitialiseCellCycleModel();

        // This test needs particular cell cycle times could also test for other cell cycle models
        TS_ASSERT_DELTA(p_stem_model->GetStemCellG1Duration(), 14.0, 1e-12);
        TS_ASSERT_DELTA(p_stem_model->GetTransitCellG1Duration(), 2.0, 1e-12);
        TS_ASSERT_DELTA(p_stem_model->GetSG2MDuration(), 10.0, 1e-12);

        StochasticDurationGenerationBasedCellCycleModel* p_stoch_model = new StochasticDurationGenerationBasedCellCycleModel();
        p_stoch_model->SetCellProliferativeType(STEM);
        CellPtr p_stochastic_stem_cell(new Cell(p_healthy_state, p_stoch_model));
        p_stochastic_stem_cell->InitialiseCellCycleModel();

        FixedDurationGenerationBasedCellCycleModel* p_diff_model = new FixedDurationGenerationBasedCellCycleModel();
        p_diff_model->SetCellProliferativeType(DIFFERENTIATED);
        p_diff_model->SetGeneration(6);
        CellPtr p_differentiated_cell(new Cell(p_healthy_state, p_diff_model));
        p_differentiated_cell->InitialiseCellCycleModel();

        StochasticDurationGenerationBasedCellCycleModel* p_stoch_diff_model = new StochasticDurationGenerationBasedCellCycleModel();
        p_stoch_diff_model->SetCellProliferativeType(DIFFERENTIATED);
        p_stoch_diff_model->SetGeneration(6);
        CellPtr p_stochastic_differentiated_cell(new Cell(p_healthy_state, p_stoch_diff_model));
        p_stochastic_differentiated_cell->InitialiseCellCycleModel();

        FixedDurationGenerationBasedCellCycleModel* p_transit_model = new FixedDurationGenerationBasedCellCycleModel();
        p_transit_model->SetCellProliferativeType(TRANSIT);
        p_transit_model->SetGeneration(2);
        CellPtr p_transit_cell(new Cell(p_healthy_state, p_transit_model));
        p_transit_cell->InitialiseCellCycleModel();

        // SimulationTime = 6 hours
        p_simulation_time->IncrementTimeOneStep();
        p_simulation_time->IncrementTimeOneStep();

        // SimulationTime = 18 hours
        TS_ASSERT_EQUALS(p_stochastic_stem_cell->ReadyToDivide(), false);
        TS_ASSERT_EQUALS(p_transit_cell->ReadyToDivide(), true);

        p_simulation_time->IncrementTimeOneStep();

        // SimulationTime = 24 hours
        TS_ASSERT_EQUALS(p_stem_cell->ReadyToDivide(), false);
        TS_ASSERT_EQUALS(p_stochastic_stem_cell->ReadyToDivide(), true);

        p_simulation_time->IncrementTimeOneStep();

        // SimulationTime = 30 hours
        TS_ASSERT_EQUALS(p_stem_cell->ReadyToDivide(), true);
        TS_ASSERT_EQUALS(p_stochastic_stem_cell->ReadyToDivide(), true);

        CellPtr p_daughter_cell1 = p_stem_cell->Divide();
        TS_ASSERT(typeid(p_daughter_cell1->GetCellCycleModel()) == typeid(p_stem_cell->GetCellCycleModel()));

        // Go to large time to ensure that differentiated cells can not divide
        for (unsigned i=0; i<990; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
        }
        TS_ASSERT_EQUALS(p_differentiated_cell->ReadyToDivide(), false);
        TS_ASSERT_EQUALS(p_stochastic_differentiated_cell->ReadyToDivide(), false);
    }

    void TestStochasticCycleModel() throw(Exception)
    {
        // Go up in steps of 0.01 to test stochasticity in cell cycle models
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(54.0, 5400);

        for (unsigned i=0; i<600; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
        }

        // Now at t=6.00
        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        FixedDurationGenerationBasedCellCycleModel* p_transit_model = new FixedDurationGenerationBasedCellCycleModel();
        p_transit_model->SetCellProliferativeType(TRANSIT);
        p_transit_model->SetGeneration(2);
        CellPtr p_transit_cell(new Cell(p_healthy_state, p_transit_model));
        p_transit_cell->InitialiseCellCycleModel();

        // This test needs particular cell cycle times
        TS_ASSERT_DELTA(p_transit_model->GetStemCellG1Duration(), 14.0, 1e-12);
        TS_ASSERT_DELTA(p_transit_model->GetTransitCellG1Duration(), 2.0, 1e-12);
        TS_ASSERT_DELTA(p_transit_model->GetSG2MDuration(), 10.0, 1e-12);

        for (unsigned i=0; i<1199; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
        }

        // Now at t = 17.99, cell is 11.99 old
        TS_ASSERT_EQUALS(p_transit_cell->ReadyToDivide(), false);

        StochasticDurationGenerationBasedCellCycleModel* p_cell_cycle_model = new StochasticDurationGenerationBasedCellCycleModel;
        p_cell_cycle_model->SetCellProliferativeType(TRANSIT);

        // This now resets the age of the cell to 0.0 so more time added in underneath
        p_transit_cell->SetCellCycleModel(p_cell_cycle_model);
        p_transit_cell->InitialiseCellCycleModel();

        TS_ASSERT_EQUALS(p_transit_cell->GetCellCycleModel(), p_cell_cycle_model);
        for (unsigned i=0; i<1399; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
        }
        TS_ASSERT_EQUALS(p_transit_cell->ReadyToDivide(), true);

        // Ensure transit cell divides
        while (!p_transit_cell->ReadyToDivide())
        {
            p_simulation_time->IncrementTimeOneStep();
        }
        CellPtr p_daughter_cell2 = p_transit_cell->Divide();
        TS_ASSERT(typeid(p_daughter_cell2->GetCellCycleModel()) == typeid(p_transit_cell->GetCellCycleModel()));
    }

    void Test0DBucketStochastic()
    {
        const double end_time = 70.0;
        const unsigned number_of_simulations = 1000;

        std::vector<CellPtr> cells;
        std::vector<CellPtr> newly_born;

        std::vector<unsigned> stem_cells(number_of_simulations);
        std::vector<unsigned> transit_cells(number_of_simulations);
        std::vector<unsigned> differentiated_cells(number_of_simulations);
        double stem_cell_mean = 0.0;
        double transit_cell_mean = 0.0;
        double differentiated_cell_mean = 0.0;

        for (unsigned simulation_number=0; simulation_number<number_of_simulations; simulation_number++)
        {
            SimulationTime::Destroy();
            SimulationTime* p_simulation_time = SimulationTime::Instance();
            p_simulation_time->SetStartTime(0.0);
            p_simulation_time->SetEndTimeAndNumberOfTimeSteps(70.0, 70);
            boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

            StochasticDurationGenerationBasedCellCycleModel* p_model = new StochasticDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(STEM);
            CellPtr p_stem_cell(new Cell(p_healthy_state, p_model));
            p_stem_cell->InitialiseCellCycleModel();
            cells.push_back(p_stem_cell);

            // Produce the offspring of the cells
            std::vector<CellPtr>::iterator cell_iterator = cells.begin();

            while (p_simulation_time->GetTime()< end_time)
            {
                p_simulation_time->IncrementTimeOneStep();
                cell_iterator = cells.begin();
                while (cell_iterator < cells.end())
                {
                    if ((*cell_iterator)->ReadyToDivide())
                    {
                        newly_born.push_back((*cell_iterator)->Divide());
                    }
                    ++cell_iterator;
                }

                // Copy offspring in newly_born vector to cells vector
                cell_iterator = newly_born.begin();
                while (cell_iterator < newly_born.end())
                {
                    cells.push_back(*cell_iterator);
                    ++cell_iterator;
                }

                newly_born.clear();
            }
            cell_iterator = cells.begin();
            while (cell_iterator < cells.end())
            {
                switch ((*cell_iterator)->GetCellCycleModel()->GetCellProliferativeType())
                {
                    case STEM:
                        stem_cells[simulation_number]++;
                        stem_cell_mean++;
                        break;
                    case TRANSIT:
                        transit_cells[simulation_number]++;
                        transit_cell_mean++;
                        break;
                    default:
                        differentiated_cells[simulation_number]++;
                        differentiated_cell_mean++;
                        break;
                }

                ++cell_iterator;
            }
            cells.clear();
        }
        stem_cell_mean=stem_cell_mean/(double) number_of_simulations;
        transit_cell_mean=transit_cell_mean/(double) number_of_simulations;
        differentiated_cell_mean=differentiated_cell_mean/(double) number_of_simulations;

        TS_ASSERT_DELTA(stem_cell_mean, 1.0, 1e-12);
        TS_ASSERT_DELTA(transit_cell_mean, 6.84, 1.0);
        TS_ASSERT_DELTA(differentiated_cell_mean, 16.0, 1.0);
    }

    void TestInitialise0DBucket()
    {
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(60.0, 60);

        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        FixedDurationGenerationBasedCellCycleModel* p_stem_model = new FixedDurationGenerationBasedCellCycleModel();
        p_stem_model->SetCellProliferativeType(STEM);
        CellPtr p_stem_cell(new Cell(p_healthy_state, p_stem_model));
        p_stem_cell->InitialiseCellCycleModel();
        cells.push_back(p_stem_cell);

        FixedDurationGenerationBasedCellCycleModel* p_transit_model = new FixedDurationGenerationBasedCellCycleModel();
        p_transit_model->SetCellProliferativeType(TRANSIT);
        p_transit_model->SetGeneration(1);
        CellPtr p_transit_cell_1(new Cell(p_healthy_state, p_transit_model));
        p_transit_cell_1->InitialiseCellCycleModel();
        cells.push_back(p_transit_cell_1);

        FixedDurationGenerationBasedCellCycleModel* p_transit_model2 = new FixedDurationGenerationBasedCellCycleModel();
        p_transit_model2->SetCellProliferativeType(TRANSIT);
        p_transit_model2->SetGeneration(2);
        CellPtr p_transit_cell_2(new Cell(p_healthy_state, p_transit_model2));
        p_transit_cell_2->InitialiseCellCycleModel();
        cells.push_back(p_transit_cell_2);

        FixedDurationGenerationBasedCellCycleModel* p_transit_model3 = new FixedDurationGenerationBasedCellCycleModel();
        p_transit_model3->SetCellProliferativeType(TRANSIT);
        p_transit_model3->SetGeneration(3);
        CellPtr p_transit_cell_3(new Cell(p_healthy_state, p_transit_model3));
        p_transit_cell_3->InitialiseCellCycleModel();
        cells.push_back(p_transit_cell_3);

        FixedDurationGenerationBasedCellCycleModel* p_diff_model = new FixedDurationGenerationBasedCellCycleModel();
        p_diff_model->SetCellProliferativeType(DIFFERENTIATED);
        p_diff_model->SetGeneration(4);
        CellPtr p_differentiated_cell(new Cell(p_healthy_state, p_diff_model));
        p_differentiated_cell->InitialiseCellCycleModel();
        cells.push_back(p_differentiated_cell);

        std::vector<CellPtr> newly_born;
        std::vector<unsigned> stem_cells(p_simulation_time->GetTotalNumberOfTimeSteps());
        std::vector<unsigned> transit_cells(p_simulation_time->GetTotalNumberOfTimeSteps());
        std::vector<unsigned> differentiated_cells(p_simulation_time->GetTotalNumberOfTimeSteps());
        std::vector<double> times(p_simulation_time->GetTotalNumberOfTimeSteps());

        std::vector<CellPtr>::iterator cell_iterator;

        unsigned i = 0;
        while (!p_simulation_time->IsFinished())
        {
            p_simulation_time->IncrementTimeOneStep();

            // Produce the offspring of the cells
            cell_iterator = cells.begin();
            while (cell_iterator < cells.end())
            {
                TS_ASSERT_EQUALS((*cell_iterator)->GetMutationState()->IsType<WildTypeCellMutationState>(), true);
                if ((*cell_iterator)->ReadyToDivide())
                {
                    newly_born.push_back((*cell_iterator)->Divide());
                }
                ++cell_iterator;
            }

            // Copy offspring in newly_born vector to cells vector
            cell_iterator = newly_born.begin();
            while (cell_iterator < newly_born.end())
            {
                cells.push_back(*cell_iterator);
                ++cell_iterator;
            }
            newly_born.clear();

            // Count number of cells of each type
            cell_iterator = cells.begin();
            stem_cells[i] = 0;
            transit_cells[i] = 0;
            differentiated_cells[i] = 0;
            while (cell_iterator < cells.end())
            {
                switch ((*cell_iterator)->GetCellCycleModel()->GetCellProliferativeType())
                {
                    case STEM:
                        stem_cells[i]++;
                        break;
                    case TRANSIT:
                        transit_cells[i]++;
                        break;
                    default:
                        differentiated_cells[i]++;
                        break;
                }
                ++cell_iterator;
            }

            times[i] = p_simulation_time->GetTime();
            i++;
        }

        TS_ASSERT_EQUALS(stem_cells[59], 1u);
        TS_ASSERT_EQUALS(transit_cells[59], 2u);
        TS_ASSERT_EQUALS(differentiated_cells[59], 23u);
    }

    /*
     * We are checking that the CellPtrs work with the Wnt cell cycle models here
     * That division of wnt cells and stuff works OK.
     *
     * It checks that the cell division thing works nicely too.
     */
    void TestWithWntCellCycleModel() throw(Exception)
    {
        SimulationTime* p_simulation_time = SimulationTime::Instance();

        unsigned num_steps = 100;
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(50.0, num_steps+1);

        double wnt_stimulus = 1.0;
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_stimulus);
        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        WntCellCycleModel* p_cell_cycle_model1 = new WntCellCycleModel();
        p_cell_cycle_model1->SetDimension(2);
        p_cell_cycle_model1->SetCellProliferativeType(TRANSIT);
        CellPtr p_wnt_cell(new Cell(p_healthy_state, p_cell_cycle_model1));
        p_wnt_cell->InitialiseCellCycleModel();

        double SG2MDuration = p_cell_cycle_model1->GetSG2MDuration();

#ifdef CHASTE_CVODE
        const double expected_g1_duration = 5.96441;
#else
        const double expected_g1_duration = 5.971;
#endif //CHASTE_CVODE

        for (unsigned i=0; i<num_steps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            double time = p_simulation_time->GetTime();

            if (time >= expected_g1_duration+SG2MDuration)
            {
                TS_ASSERT_EQUALS(p_wnt_cell->ReadyToDivide(), true);
            }
            else
            {
                TS_ASSERT_EQUALS(p_wnt_cell->ReadyToDivide(), false);
            }
        }

        p_simulation_time->IncrementTimeOneStep();
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_stimulus);

        TS_ASSERT_EQUALS(p_wnt_cell->ReadyToDivide(), true);

        CellPtr p_wnt_cell2 = p_wnt_cell->Divide();

        double time_of_birth = p_wnt_cell->GetBirthTime();
        double time_of_birth2 = p_wnt_cell2->GetBirthTime();

        TS_ASSERT_DELTA(time_of_birth, time_of_birth2, 1e-9);

        for (unsigned i=0; i<num_steps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            double time = p_simulation_time->GetTime();

            bool result1 = p_wnt_cell->ReadyToDivide();
            bool result2 = p_wnt_cell2->ReadyToDivide();

            if (time >= expected_g1_duration + SG2MDuration + time_of_birth)
            {
                TS_ASSERT_EQUALS(result1, true);
                TS_ASSERT_EQUALS(result2, true);
            }
            else
            {
                TS_ASSERT_EQUALS(result1, false);
                TS_ASSERT_EQUALS(result2, false);
            }
        }

        // Tidy up
        WntConcentration<2>::Destroy();
    }

    /*
     * We are checking that the CellPtrs work with the StochasticWnt cell cycle models here
     * That division of wnt cells and stuff works OK.
     *
     * It checks that the cell division thing works nicely too.
     */
    void TestWithStochasticWntCellCycleModel() throw(Exception)
    {
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        unsigned num_steps = 100;
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(50.0, num_steps+1);

        double wnt_stimulus = 1.0;
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_stimulus);

        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        StochasticWntCellCycleModel* p_cell_model = new StochasticWntCellCycleModel();
        p_cell_model->SetDimension(2);
        p_cell_model->SetCellProliferativeType(TRANSIT);
        CellPtr p_wnt_cell(new Cell(p_healthy_state, p_cell_model));
        p_wnt_cell->InitialiseCellCycleModel();

        // These are the first three normal random with mean 10, s.d. 1 and this seed (0)
		double SG2MDuration1 = p_cell_model->GetSDuration() + 3.16084 + p_cell_model->GetMDuration();
		double SG2MDuration2 = p_cell_model->GetSDuration() + 5.0468  + p_cell_model->GetMDuration();
		double SG2MDuration3 = p_cell_model->GetSDuration() + 3.34408 + p_cell_model->GetMDuration();
		double g1_duration = 5.971;

        for (unsigned i=0; i<num_steps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            double time = p_simulation_time->GetTime();

            if (time >= g1_duration+SG2MDuration1)
            {
                TS_ASSERT_EQUALS(p_wnt_cell->ReadyToDivide(), true);
            }
            else
            {
                TS_ASSERT_EQUALS(p_wnt_cell->ReadyToDivide(), false);
            }
        }

        p_simulation_time->IncrementTimeOneStep();
        TS_ASSERT_EQUALS(p_wnt_cell->ReadyToDivide(), true);

        CellPtr p_wnt_cell2 = p_wnt_cell->Divide();

        double time_of_birth = p_wnt_cell->GetBirthTime();
        double time_of_birth2 = p_wnt_cell2->GetBirthTime();

        TS_ASSERT_DELTA(time_of_birth, time_of_birth2, 1e-9);

        for (unsigned i=0; i<num_steps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            double time = p_simulation_time->GetTime();

            bool parent_ready = p_wnt_cell->ReadyToDivide();
            bool daughter_ready = p_wnt_cell2->ReadyToDivide();

            if (time >= g1_duration+SG2MDuration2+time_of_birth)
            {
                TS_ASSERT_EQUALS(parent_ready, true);
            }
            else
            {
                TS_ASSERT_EQUALS(parent_ready, false);
            }
            if (time >= g1_duration+SG2MDuration3+time_of_birth2)
            {
                TS_ASSERT_EQUALS(daughter_ready, true);
            }
            else
            {
                TS_ASSERT_EQUALS(daughter_ready, false);
            }
        }

        // Tidy up
        WntConcentration<2>::Destroy();
    }

    /*
     * We are checking that the CellPtrs work with the T&N cell cycle models here
     * That division of wnt cells and stuff works OK.
     *
     * It checks that the cell division thing works nicely too.
     */
    void TestWithTysonNovakCellCycleModel() throw(Exception)
    {
        double standard_tyson_duration = 1.242;

        SimulationTime* p_simulation_time = SimulationTime::Instance();
        unsigned num_steps = 100;
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(200.0/60.0, num_steps+1);

        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        TysonNovakCellCycleModel* p_cell_model = new TysonNovakCellCycleModel();
        p_cell_model->SetCellProliferativeType(TRANSIT);
        CellPtr p_tn_cell(new Cell(p_healthy_state, p_cell_model));
        p_tn_cell->InitialiseCellCycleModel();

        for (unsigned i=0; i<num_steps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            double time = p_simulation_time->GetTime();
            if (time>standard_tyson_duration)
            {
                TS_ASSERT_EQUALS(p_tn_cell->ReadyToDivide(), true);
            }
            else
            {
                TS_ASSERT_EQUALS(p_tn_cell->ReadyToDivide(), false);
            }
        }

        p_simulation_time->IncrementTimeOneStep();
        TS_ASSERT_EQUALS(p_tn_cell->ReadyToDivide(), true);

        CellPtr p_tn_cell2 = p_tn_cell->Divide();

        double time_of_birth = p_tn_cell->GetBirthTime();
        double time_of_birth2 = p_tn_cell2->GetBirthTime();

        TS_ASSERT_DELTA(time_of_birth, time_of_birth2, 1e-9);

        for (unsigned i=0; i<num_steps/2; i++)
        {
            p_simulation_time->IncrementTimeOneStep();
            double time = p_simulation_time->GetTime();
            bool result1 = p_tn_cell->ReadyToDivide();
            bool result2 = p_tn_cell2->ReadyToDivide();

            if (time >= standard_tyson_duration + time_of_birth)
            {
                TS_ASSERT_EQUALS(result1, true);
                TS_ASSERT_EQUALS(result2, true);
            }
            else
            {
                TS_ASSERT_EQUALS(result1, false);
                TS_ASSERT_EQUALS(result2, false);
            }
        }
    }

    void TestTysonNovakSteadyState()
    {
        // Keep dividing until we reach steady-state
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        unsigned num_steps=100000;
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(20000.0/60.0, num_steps+1);

        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        TysonNovakCellCycleModel* p_cell_model = new TysonNovakCellCycleModel();
        p_cell_model->SetCellProliferativeType(TRANSIT);
        CellPtr p_tn_cell(new Cell(p_healthy_state, p_cell_model));
        p_tn_cell->InitialiseCellCycleModel();

        unsigned num_divisions = 0;

        while (!p_simulation_time->IsFinished())
        {
            while (!p_simulation_time->IsFinished() && !p_tn_cell->ReadyToDivide())
            {
                p_simulation_time->IncrementTimeOneStep();
            }
            if (p_tn_cell->ReadyToDivide())
            {
                CellPtr p_tn_cell2 = p_tn_cell->Divide();
                ++num_divisions;
            }
        }
        TS_ASSERT_EQUALS(num_divisions, 268u);
    }

    void TestApoptosisAndDeath()
    {
        // We are going to start at t=0 and jump up in steps of 0.2
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(0.6, 3);

        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        FixedDurationGenerationBasedCellCycleModel* p_cell_model = new FixedDurationGenerationBasedCellCycleModel();
        p_cell_model->SetCellProliferativeType(TRANSIT);

        CellPtr p_cell(new Cell(p_healthy_state, p_cell_model));
        p_cell->InitialiseCellCycleModel();

        TS_ASSERT_EQUALS(p_cell->HasApoptosisBegun(), false);
        TS_ASSERT_EQUALS(p_cell->IsDead(), false);
        TS_ASSERT_THROWS_THIS(p_cell->GetTimeUntilDeath(),"Shouldn\'t be checking time until apoptosis as it isn\'t set");
        TS_ASSERT_DELTA(p_cell->GetApoptosisTime(), 0.25, 1e-6);

        p_simulation_time->IncrementTimeOneStep(); // t=0.2

        p_cell->StartApoptosis();
        TS_ASSERT_THROWS_THIS(p_cell->StartApoptosis(),"StartApoptosis() called when already undergoing apoptosis");

        TS_ASSERT_EQUALS(p_cell->HasApoptosisBegun(), true);
        TS_ASSERT_EQUALS(p_cell->IsDead(), false);
        TS_ASSERT_DELTA(p_cell->GetTimeUntilDeath(),0.25,1e-12);

        // Check that we can copy a cell that has started apoptosis
        CellPtr p_cell2 = p_cell;

        p_simulation_time->IncrementTimeOneStep(); // t=0.4
        TS_ASSERT_EQUALS(p_cell->HasApoptosisBegun(), true);
        TS_ASSERT_EQUALS(p_cell->IsDead(), false);
        TS_ASSERT_DELTA(p_cell->GetTimeUntilDeath(),0.05,1e-12);

        TS_ASSERT_EQUALS(p_cell2->HasApoptosisBegun(), true);
        TS_ASSERT_EQUALS(p_cell2->IsDead(), false);
        TS_ASSERT_DELTA(p_cell2->GetTimeUntilDeath(),0.05,1e-12);

        p_simulation_time->IncrementTimeOneStep(); // t=0.6
        TS_ASSERT_EQUALS(p_cell->HasApoptosisBegun(), true);
        TS_ASSERT_EQUALS(p_cell->IsDead(), true);
    }

    void TestCantDivideIfUndergoingApoptosis()
    {
        // We are going to start at t=0 and jump up to t=25
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(25, 1);

        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        FixedDurationGenerationBasedCellCycleModel* p_cell_model = new FixedDurationGenerationBasedCellCycleModel();
        p_cell_model->SetCellProliferativeType(TRANSIT);
        CellPtr p_cell(new Cell(p_healthy_state, p_cell_model));
        p_cell->InitialiseCellCycleModel();
        p_simulation_time->IncrementTimeOneStep(); // t=25

        TS_ASSERT_EQUALS(p_cell->ReadyToDivide(), true);
        p_cell->StartApoptosis();
        TS_ASSERT_EQUALS(p_cell->ReadyToDivide(), false);
    }

    void Test0DBucketWithDeath()
    {
        double end_time = 92.0;
        unsigned num_time_steps = 92;

        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(end_time, num_time_steps);

        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        FixedDurationGenerationBasedCellCycleModel* p_cell_model = new FixedDurationGenerationBasedCellCycleModel();
        p_cell_model->SetCellProliferativeType(STEM);
        CellPtr p_stem_cell(new Cell(p_healthy_state, p_cell_model));
        p_stem_cell->InitialiseCellCycleModel();

        std::vector<CellPtr> cells;
        std::vector<CellPtr> newly_born;
        std::vector<unsigned> stem_cells(num_time_steps);
        std::vector<unsigned> transit_cells(num_time_steps);
        std::vector<unsigned> differentiated_cells(num_time_steps);
        std::vector<unsigned> dead_cells(num_time_steps);
        std::vector<double> times(num_time_steps);

        cells.push_back(p_stem_cell);
        std::vector<CellPtr>::iterator cell_iterator;

        unsigned i = 0;
        while (p_simulation_time->GetTime() < end_time)
        {
            // Produce the offspring of the cells

            p_simulation_time->IncrementTimeOneStep();
            cell_iterator = cells.begin();
            while (cell_iterator < cells.end())
            {
                if (!(*cell_iterator)->IsDead())
                {
                    if ((*cell_iterator)->ReadyToDivide())
                    {
                        newly_born.push_back((*cell_iterator)->Divide());
                    }

                    if (((*cell_iterator)->GetAge() > 30))
                    {
                        (*cell_iterator)->StartApoptosis();
                    }
                }
                ++cell_iterator;
            }

            // Copy offspring in newly_born vector to cells vector
            cell_iterator = newly_born.begin();
            while (cell_iterator < newly_born.end())
            {
                cells.push_back(*cell_iterator);
                ++cell_iterator;
            }
            newly_born.clear();

            // Update cell counts
            cell_iterator = cells.begin();
            while (cell_iterator < cells.end())
            {
                if (!(*cell_iterator)->IsDead())
                {
                    switch ((*cell_iterator)->GetCellCycleModel()->GetCellProliferativeType())
                    {
                        case STEM:
                            stem_cells[i]++;
                            break;
                        case TRANSIT:
                            transit_cells[i]++;
                            break;
                        default:
                            differentiated_cells[i]++;
                            break;
                    }
                }
                else
                {
                    dead_cells[i]++;
                }

                ++cell_iterator;
            }
            times[i] = p_simulation_time->GetTime();
            i++;
        }

        TS_ASSERT_EQUALS(stem_cells[num_time_steps-1], 1u);
        TS_ASSERT_EQUALS(transit_cells[num_time_steps-1], 2u);
        TS_ASSERT_EQUALS(differentiated_cells[num_time_steps-1], 8u);
        TS_ASSERT_EQUALS(dead_cells[num_time_steps-1], 8u);
    }

    /*
     * We are checking that the CellPtrs work with the Wnt
     * cell cycle models here. This just tests the set-up and checks that
     * the functions can all be called (not what they return).
     *
     * For more in depth tests see TestNightlyCellPtr.hpp
     * (these test that the cell cycle times are correct for the
     * various mutant cells)
     */
    void TestWntMutantVariantsAndLabelling() throw(Exception)
    {
        SimulationTime* p_simulation_time = SimulationTime::Instance();

        unsigned num_steps = 10;
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(1.0, num_steps+1);

        double wnt_stimulus = 1.0;
        WntConcentration<2>::Instance()->SetConstantWntValueForTesting(wnt_stimulus);

        boost::shared_ptr<AbstractCellProperty> p_wt_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());
        boost::shared_ptr<AbstractCellProperty> p_apc_one_hit_state(CellPropertyRegistry::Instance()->Get<ApcOneHitCellMutationState>());
        boost::shared_ptr<AbstractCellProperty> p_apc_two_hit_state(CellPropertyRegistry::Instance()->Get<ApcTwoHitCellMutationState>());
        boost::shared_ptr<AbstractCellProperty> p_bcat_one_hit_state(CellPropertyRegistry::Instance()->Get<BetaCateninOneHitCellMutationState>());
        boost::shared_ptr<AbstractCellProperty> p_label(CellPropertyRegistry::Instance()->Get<CellLabel>());

        WntCellCycleModel* p_cell_cycle_model1 = new WntCellCycleModel();
        p_cell_cycle_model1->SetDimension(2);
        p_cell_cycle_model1->SetCellProliferativeType(TRANSIT);
        CellPtr p_wnt_cell(new Cell(p_apc_one_hit_state, p_cell_cycle_model1));
        p_wnt_cell->InitialiseCellCycleModel();

        WntCellCycleModel* p_cell_cycle_model2 = new WntCellCycleModel();
        p_cell_cycle_model2->SetDimension(2);
        p_cell_cycle_model2->SetCellProliferativeType(TRANSIT);
        CellPtr p_wnt_cell2(new Cell(p_bcat_one_hit_state, p_cell_cycle_model2));
        p_wnt_cell2->InitialiseCellCycleModel();

        WntCellCycleModel* p_cell_cycle_model3 = new WntCellCycleModel();
        p_cell_cycle_model3->SetDimension(2);
        p_cell_cycle_model3->SetCellProliferativeType(TRANSIT);
        CellPtr p_wnt_cell3(new Cell(p_apc_two_hit_state, p_cell_cycle_model3));
        p_wnt_cell3->InitialiseCellCycleModel();

        WntCellCycleModel* p_cell_cycle_model4 = new WntCellCycleModel();
        p_cell_cycle_model4->SetDimension(2);
        p_cell_cycle_model4->SetCellProliferativeType(TRANSIT);
        CellPropertyCollection collection;
        collection.AddProperty(p_label);
        CellPtr p_wnt_cell4(new Cell(p_wt_state, p_cell_cycle_model4, false, collection));
        p_wnt_cell4->InitialiseCellCycleModel();

        TS_ASSERT_EQUALS(p_wnt_cell->ReadyToDivide(), false);
        TS_ASSERT_EQUALS(p_wnt_cell2->ReadyToDivide(), false);
        TS_ASSERT_EQUALS(p_wnt_cell3->ReadyToDivide(), false);
        TS_ASSERT_EQUALS(p_wnt_cell4->ReadyToDivide(), false);

        //Tidy up
        WntConcentration<2>::Destroy();
    }

    void TestIsLogged()
    {
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(25, 1);

        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        FixedDurationGenerationBasedCellCycleModel* p_cell_model = new FixedDurationGenerationBasedCellCycleModel();
        p_cell_model->SetCellProliferativeType(STEM);
        CellPtr p_cell(new Cell(p_healthy_state, p_cell_model));
        p_cell->InitialiseCellCycleModel();

        TS_ASSERT_EQUALS(p_cell->IsLogged(), false);

        p_cell->SetLogged();

        TS_ASSERT_EQUALS(p_cell->IsLogged(), true);

        CellPtr p_copied_cell = p_cell;

        TS_ASSERT_EQUALS(p_copied_cell->IsLogged(), true);

        p_simulation_time->IncrementTimeOneStep();

        TS_ASSERT_EQUALS(p_cell->ReadyToDivide(), true);

        CellPtr p_daughter_cell = p_cell->Divide();

        TS_ASSERT_EQUALS(p_cell->IsLogged(), true);
        TS_ASSERT_EQUALS(p_daughter_cell->IsLogged(), false);
    }

    void TestAncestors() throw (Exception)
    {
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(25, 2);

        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        FixedDurationGenerationBasedCellCycleModel* p_cell_model = new FixedDurationGenerationBasedCellCycleModel();
        p_cell_model->SetCellProliferativeType(STEM);
        CellPtr p_cell(new Cell(p_healthy_state, p_cell_model));
        p_cell->InitialiseCellCycleModel();
        p_cell->SetAncestor(2u);

        p_simulation_time->IncrementTimeOneStep();
        p_simulation_time->IncrementTimeOneStep();

        TS_ASSERT_EQUALS(p_cell->ReadyToDivide(), true);

        CellPtr p_cell2 = p_cell->Divide();

        TS_ASSERT_EQUALS(p_cell->GetAncestor(), 2u);
        TS_ASSERT_EQUALS(p_cell2->GetAncestor(), 2u);
    }

    void TestCellId() throw (Exception)
    {
        // Resetting the Maximum cell Id to zero (to account for previous tests)
        Cell::ResetMaxCellId();

        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(25, 2);

        boost::shared_ptr<AbstractCellProperty> p_healthy_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());

        FixedDurationGenerationBasedCellCycleModel* p_cell_model = new FixedDurationGenerationBasedCellCycleModel();
        p_cell_model->SetCellProliferativeType(STEM);
        CellPtr p_cell(new Cell(p_healthy_state, p_cell_model));
        p_cell->InitialiseCellCycleModel();

        p_simulation_time->IncrementTimeOneStep();
        p_simulation_time->IncrementTimeOneStep();

        TS_ASSERT_EQUALS(p_cell->ReadyToDivide(), true);

        CellPtr p_cell2 = p_cell->Divide();

        TS_ASSERT_EQUALS(p_cell->GetCellId(), 0u);
        TS_ASSERT_EQUALS(p_cell2->GetCellId(), 1u);
    }
};

#endif /*TESTCELL_HPP_*/
