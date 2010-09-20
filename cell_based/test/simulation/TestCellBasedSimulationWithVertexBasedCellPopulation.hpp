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
#ifndef TESTCELLBASEDSIMULATIONWITHVERTEXBASEDCELLPOPULATION_HPP_
#define TESTCELLBASEDSIMULATIONWITHVERTEXBASEDCELLPOPULATION_HPP_

#include <cxxtest/TestSuite.h>

// Must be included before other cell_based headers
#include "CellBasedSimulationArchiver.hpp"

#include "CellBasedSimulation.hpp"
#include "FixedDurationGenerationBasedCellCycleModel.hpp"
#include "StochasticDurationGenerationBasedCellCycleModel.hpp"
#include "VertexBasedCellPopulation.hpp"
#include "NagaiHondaForce.hpp"
#include "WelikyOsterForce.hpp"
#include "AbstractCellKiller.hpp"
#include "TargetedCellKiller.hpp"
#include "AbstractCellBasedTestSuite.hpp"
#include "HoneycombMutableVertexMeshGenerator.hpp"
#include "VertexMeshWriter.hpp"
#include "WildTypeCellMutationState.hpp"
#include "CellLabel.hpp"
#include "Warnings.hpp"
#include "LogFile.hpp"


class TestCellBasedSimulationWithVertexBasedCellPopulation : public AbstractCellBasedTestSuite
{
private:

    double mLastStartTime;
    void setUp()
    {
        mLastStartTime = std::clock();
        AbstractCellBasedTestSuite::setUp();
    }
    void tearDown()
    {
        double time = std::clock();
        double elapsed_time = (time - mLastStartTime)/(CLOCKS_PER_SEC);
        std::cout << "Elapsed time: " << elapsed_time << std::endl;
        AbstractCellBasedTestSuite::tearDown();
    }

public:

    void TestSingleCellRelaxation() throw (Exception)
    {
        // Construct a 2D vertex mesh consisting of a single element
        std::vector<Node<2>*> nodes;
        unsigned num_nodes = 20;
        for (unsigned i=0; i<num_nodes; i++)
        {
            double theta = M_PI+2.0*M_PI*(double)(i)/(double)(num_nodes);
            nodes.push_back(new Node<2>(i, true, cos(theta), sin(theta)));
        }

        std::vector<VertexElement<2,2>*> elements;
        elements.push_back(new VertexElement<2,2>(0, nodes));

        double cell_swap_threshold = 0.01;
        double edge_division_threshold = 2.0;
        MutableVertexMesh<2,2> mesh(nodes, elements, cell_swap_threshold, edge_division_threshold);

        // Set up cells, one for each VertexElement. Give each cell
        // a birth time of 0
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        for (unsigned elem_index=0; elem_index<mesh.GetNumElements(); elem_index++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(DIFFERENTIATED);

            CellPtr p_cell(new Cell(p_state, p_model));
            double birth_time = -1.0;
            p_cell->SetBirthTime(birth_time);
            cells.push_back(p_cell);
        }

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(mesh, cells);

        // Create a force system
        NagaiHondaForce<2> force;
        std::vector<AbstractForce<2>* > force_collection;
        force_collection.push_back(&force);

        // Set up cell-based simulation
        CellBasedSimulation<2> simulator(cell_population, force_collection);
        simulator.SetOutputDirectory("TestSingleCellRelaxation");
        simulator.SetEndTime(1.0);

        // Run simulation
        simulator.Solve();

        // Test relaxes to circle (can be more stringent with more nodes and more time)
        TS_ASSERT_DELTA(cell_population.rGetMesh().GetVolumeOfElement(0), 1.0, 0.05);
        TS_ASSERT_DELTA(cell_population.rGetMesh().GetSurfaceAreaOfElement(0), 3.5449077, 0.1);

        // Test Warnings
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNumWarnings(), 1u);
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNextWarningMessage(),"Vertices are moving more than half the CellRearrangementThreshold this could cause elements to become inverted the motion has been restricted: - To avoid these warnings use a smaller timestep");
        Warnings::QuietDestroy();
    }

    void TestSingleCellRelaxationWelikyOster() throw (Exception)
    {
        // Construct a 2D vertex mesh consisting of a single element
        std::vector<Node<2>*> nodes;
        unsigned num_nodes = 20;
        for (unsigned i=0; i<num_nodes; i++)
        {
            double theta = M_PI+2.0*M_PI*(double)(i)/(double)(num_nodes);
            nodes.push_back(new Node<2>(i, true, cos(theta), sin(theta)));
        }

        std::vector<VertexElement<2,2>*> elements;
        elements.push_back(new VertexElement<2,2>(0, nodes));

        double cell_swap_threshold = 0.01;
        double edge_division_threshold = 2.0;
        MutableVertexMesh<2,2> mesh(nodes, elements, cell_swap_threshold, edge_division_threshold);

        // Set up cells, one for each VertexElement. Give each cell
        // a birth time of 0
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        for (unsigned elem_index=0; elem_index<mesh.GetNumElements(); elem_index++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(DIFFERENTIATED);

            CellPtr p_cell(new Cell(p_state, p_model));
            p_cell->SetBirthTime(-1.0);
            cells.push_back(p_cell);
        }

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(mesh, cells);

        // Create a force system
        WelikyOsterForce<2> force;
        std::vector<AbstractForce<2>* > force_collection;
        force_collection.push_back(&force);

        // Set up cell-based simulation
        CellBasedSimulation<2> simulator(cell_population, force_collection);
        simulator.SetOutputDirectory("TestSingleCellRelaxationWelikyOster");
        simulator.SetEndTime(1.0);

        // Run simulation
        simulator.Solve();

        // Test relaxes to circle (can be more stringent with more nodes and more time)
        TS_ASSERT_DELTA(cell_population.rGetMesh().GetVolumeOfElement(0), 1.0, 0.1);
        TS_ASSERT_DELTA(cell_population.rGetMesh().GetSurfaceAreaOfElement(0), 3.5449077, 0.1);

        //Test Warnings
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNumWarnings(), 0u);
        Warnings::QuietDestroy();
    }

    void TestSingleCellDividing() throw (Exception)
    {
        // Create a simple 2D MutableVertexMesh with only one cell
        HoneycombMutableVertexMeshGenerator generator(1, 1);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

        // Set up cell.
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);

        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
        p_model->SetCellProliferativeType(TRANSIT);

        CellPtr p_cell(new Cell(p_state, p_model));
        double birth_time = -20.0; // divides straight away
        p_cell->SetBirthTime(birth_time);
        cells.push_back(p_cell);

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);


        // Create a force system
        NagaiHondaForce<2> force;
        std::vector<AbstractForce<2>* > force_collection;
        force_collection.push_back(&force);

        // Set up cell-based simulation
        CellBasedSimulation<2> simulator(cell_population, force_collection);
        simulator.SetOutputDirectory("TestSingleCellDividing");
        simulator.SetEndTime(1.0);

        // Run simulation
        simulator.Solve();

        // Check that cell divided successfully
        unsigned new_num_nodes = simulator.rGetCellPopulation().GetNumNodes();
        unsigned new_num_elements = (static_cast<VertexBasedCellPopulation<2>*>(&(simulator.rGetCellPopulation())))->GetNumElements();
        unsigned new_num_cells = simulator.rGetCellPopulation().GetNumRealCells();

        TS_ASSERT_EQUALS(new_num_nodes, 8u);
        TS_ASSERT_EQUALS(new_num_elements, 2u);
        TS_ASSERT_EQUALS(new_num_cells, 2u);

        // Test Warnings
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNumWarnings(), 1u);
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNextWarningMessage(),"Vertices are moving more than half the CellRearrangementThreshold this could cause elements to become inverted the motion has been restricted: - To avoid these warnings use a smaller timestep");
        Warnings::QuietDestroy();
    }

    void TestVertexMonolayerWithCellBirth() throw (Exception)
    {
        // Create a simple 2D MutableVertexMesh
        HoneycombMutableVertexMeshGenerator generator(5, 5);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

        // Set up cells, one for each VertexElement. Give each cell
        // a birth time of -elem_index, so its age is elem_index
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        for (unsigned elem_index=0; elem_index<p_mesh->GetNumElements(); elem_index++)
        {
            CellProliferativeType cell_type = DIFFERENTIATED;
            double birth_time = 0.0 - elem_index;

            // Cell should divide at time t=0.05
            if (elem_index==12) //(elem_index==6 ||elem_index==7 || elem_index==8 || elem_index==11 ||elem_index==12 || elem_index==13 ||elem_index==16 ||elem_index==17 || elem_index==18)
            {
                cell_type = STEM;
                birth_time = -23.95;
            }

            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(cell_type);

            CellPtr p_cell(new Cell(p_state, p_model));
            p_cell->SetBirthTime(birth_time);
            cells.push_back(p_cell);
        }

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);

        unsigned old_num_nodes = cell_population.GetNumNodes();
        unsigned old_num_elements = cell_population.GetNumElements();
        unsigned old_num_cells = cell_population.GetNumRealCells();

        // Create a force system
        NagaiHondaForce<2> force;
        std::vector<AbstractForce<2>* > force_collection;
        force_collection.push_back(&force);

        // Set up cell-based simulation
        CellBasedSimulation<2> simulator(cell_population, force_collection);
        simulator.SetOutputDirectory("TestVertexMonolayerWithCellBirth");
        simulator.SetSamplingTimestepMultiple(50);
        simulator.SetEndTime(0.1);

        // Run simulation
        simulator.Solve();

        // Check that cell 12 divided successfully
        unsigned new_num_nodes = simulator.rGetCellPopulation().GetNumNodes();
        unsigned new_num_elements = (static_cast<VertexBasedCellPopulation<2>*>(&(simulator.rGetCellPopulation())))->GetNumElements();
        unsigned new_num_cells = simulator.rGetCellPopulation().GetNumRealCells();

        TS_ASSERT_EQUALS(new_num_nodes, old_num_nodes+2); // as division of element is longer than threshold so is divided
        TS_ASSERT_EQUALS(new_num_elements, old_num_elements+1);
        TS_ASSERT_EQUALS(new_num_cells, old_num_cells+1);

        // Test Warnings
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNumWarnings(), 1u);
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNextWarningMessage(),"Vertices are moving more than half the CellRearrangementThreshold this could cause elements to become inverted the motion has been restricted: - To avoid these warnings use a smaller timestep");
        Warnings::QuietDestroy();
    }

    // This test uses a larger timestep to run faster.
    void TestVertexMonolayerWithVoid() throw (Exception)
    {

        // Create a simple 2D MutableVertexMesh
        HoneycombMutableVertexMeshGenerator generator(3, 3);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

        // create a horseshoe shaped mesh
        p_mesh->DeleteElementPriorToReMesh(0);
        p_mesh->DeleteElementPriorToReMesh(1);
        p_mesh->DeleteElementPriorToReMesh(4);
        p_mesh->DeleteElementPriorToReMesh(6);

        p_mesh->ReMesh();

        // Set up cells, one for each VertexElement. Give each cell
        // a birth time of -elem_index-19, so its age is elem_index+19, so the first cell divides straight away.
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        for (unsigned elem_index=0; elem_index<p_mesh->GetNumElements(); elem_index++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(STEM);

            CellPtr p_cell(new Cell(p_state, p_model));
            double birth_time;
            birth_time = -(double)elem_index -19.0;
            p_cell->SetBirthTime(birth_time);
            cells.push_back(p_cell);
        }

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);

        // Create a force system
        NagaiHondaForce<2> force;
        std::vector<AbstractForce<2>* > force_collection;
        force_collection.push_back(&force);

        // Set up cell-based simulation
        CellBasedSimulation<2> simulator(cell_population, force_collection);
        simulator.SetOutputDirectory("TestVertexMonolayerWithVoid");
        simulator.SetEndTime(20.0);

        ////////////////////////////////////////////
        /// Strange setup to speed up simulation ///
        ////////////////////////////////////////////
        p_mesh->SetCellRearrangementThreshold(0.05);
        simulator.SetDt(0.1);
        ////////////////////////////////////////////

        // Run simulation
        simulator.Solve();

        // Check that void has been removed and vertices are in the correct position
        TS_ASSERT_EQUALS(simulator.rGetCellPopulation().GetNumNodes(), 40u);
        TS_ASSERT_EQUALS((static_cast<VertexBasedCellPopulation<2>*>(&(simulator.rGetCellPopulation())))->GetNumElements(),15u);
        TS_ASSERT_EQUALS(simulator.rGetCellPopulation().GetNumRealCells(), 15u);

        // Test Warnings
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNumWarnings(), 1u);
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNextWarningMessage(),"Vertices are moving more than half the CellRearrangementThreshold this could cause elements to become inverted the motion has been restricted: - To avoid these warnings use a smaller timestep");
        Warnings::QuietDestroy();
    }

    void TestVertexMonolayerWithCellDeath() throw (Exception)
    {
        /*
         * We don't want apoptosing cells to be labelled as dead after a certain time in
         * vertex simulations, so set the apoptosis time to something large.
         */

        // Create a simple 2D MutableVertexMesh
        HoneycombMutableVertexMeshGenerator generator(4,4);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();
        p_mesh->SetCellRearrangementThreshold(0.1);
        p_mesh->SetT2Threshold(1.0); // so T2Swaps once it becomes a triangle

        // Set up cells, one for each VertexElement. Give each cell
        // a birth time of -elem_index, so its age is elem_index
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        for (unsigned elem_index=0; elem_index<p_mesh->GetNumElements(); elem_index++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(DIFFERENTIATED);

            CellPtr p_cell(new Cell(p_state, p_model));
            double birth_time = 0.0 - elem_index;
            p_cell->SetBirthTime(birth_time);

            if (elem_index==6)
            {
                p_cell->StartApoptosis(false);
            }

            if (elem_index==14)
			{
				p_cell->StartApoptosis(true);
			}

            cells.push_back(p_cell);
        }

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);

        unsigned old_num_nodes = cell_population.GetNumNodes();
        unsigned old_num_elements = cell_population.GetNumElements();
        unsigned old_num_cells = cell_population.GetNumRealCells();

        // Create a force system
        NagaiHondaForce<2> force;
        std::vector<AbstractForce<2>* > force_collection;
        force_collection.push_back(&force);

        // Set up cell-based simulation
        CellBasedSimulation<2> simulator(cell_population, force_collection);
        simulator.SetOutputDirectory("TestVertexMonolayerWithCellDeath");
        simulator.SetEndTime(0.5);

        // Create a cell killer and pass in to simulation (note we must account for element index changes following each kill)
        TargetedCellKiller<2> cell0_killer(&cell_population, 0);    // element on the SW corner
        TargetedCellKiller<2> cell2_killer(&cell_population, 2);    // element on the S boundary
        TargetedCellKiller<2> cell9_killer(&cell_population, 9);  // element on the interior

        simulator.AddCellKiller(&cell0_killer);
        simulator.AddCellKiller(&cell2_killer);
        simulator.AddCellKiller(&cell9_killer);

        // Run simulation
        simulator.Solve();

        // Check that cells 6 and 14 have now been removed.
        unsigned new_num_nodes = simulator.rGetCellPopulation().GetNumNodes();
        unsigned new_num_elements = (static_cast<VertexBasedCellPopulation<2>*>(&(simulator.rGetCellPopulation())))->GetNumElements();
        unsigned new_num_cells = simulator.rGetCellPopulation().GetNumRealCells();

        TS_ASSERT_EQUALS(new_num_nodes, old_num_nodes-7);	// Due to the cells on the boundary that get killed and the apoptotic cell that does a T2 swap
        TS_ASSERT_EQUALS(new_num_elements, old_num_elements-5);
        TS_ASSERT_EQUALS(new_num_cells, old_num_cells-5);
        TS_ASSERT_EQUALS(new_num_cells, new_num_elements);

        // Test Warnings
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNumWarnings(), 2u);
		TS_ASSERT_EQUALS(Warnings::Instance()->GetNextWarningMessage(),"Vertices are moving more than half the CellRearrangementThreshold this could cause elements to become inverted the motion has been restricted: - To avoid these warnings use a smaller timestep");
		TS_ASSERT_EQUALS(Warnings::Instance()->GetNextWarningMessage(),"Cell removed due to T2Swap this is not counted in the dead cells counter");
		Warnings::QuietDestroy();
    }

    /*
     * This test visualizing cells of 2 mutation types, wildtype and labelled type.
	 * It asserts that neighboring cells have the correct adhesion parameter for difference
	 * pairs of nodes.
	 */
	void TestVertexMonolayerWithTwoMutationTypes() throw (Exception)
	{
		// Create a simple 2D MutableVertexMesh with only four cells
		HoneycombMutableVertexMeshGenerator generator(2, 2);
		MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

		// Set up cell.
		std::vector<CellPtr> cells;
		boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
		boost::shared_ptr<AbstractCellProperty> p_label(new CellLabel);

		for (unsigned elem_index=0; elem_index<p_mesh->GetNumElements(); elem_index++)
		{
			FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
			p_model->SetCellProliferativeType(TRANSIT);

            CellPtr p_cell(new Cell(p_state, p_model));
            double birth_time = -2.0;
            p_cell->SetBirthTime(birth_time);

			if (elem_index == 0 || elem_index == 2)
			{
				p_cell->AddCellProperty(p_label);
			}
            cells.push_back(p_cell);
		}

		// Create cell population
		VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);
        cell_population.SetOutputCellMutationStates(true);

		// Create a force system
		NagaiHondaForce<2> force;
		std::vector<AbstractForce<2>* > force_collection;
		force_collection.push_back(&force);

		// Set up cell-based simulation
		CellBasedSimulation<2> simulator(cell_population, force_collection);
		simulator.SetOutputDirectory("TestVertexMonolayerWithTwoMutationStates");
		simulator.SetEndTime(1.0);

		// Run simulation
		simulator.Solve();

		///\todo test against a saved simulation or something similar, i.e check the positions of some vertices.
		TS_ASSERT_EQUALS(p_mesh->GetNode(13)->IsBoundaryNode(), true);
		TS_ASSERT_EQUALS(p_mesh->GetNumElements(),4u);
		TS_ASSERT_EQUALS(p_state->GetColour(), 0u);
		TS_ASSERT_EQUALS(cells[0]->HasCellProperty<CellLabel>(), true);

		// Test Warnings
		TS_ASSERT_EQUALS(Warnings::Instance()->GetNumWarnings(), 1u);
		TS_ASSERT_EQUALS(Warnings::Instance()->GetNextWarningMessage(),"Vertices are moving more than half the CellRearrangementThreshold this could cause elements to become inverted the motion has been restricted: - To avoid these warnings use a smaller timestep");
		Warnings::QuietDestroy();
	}


    void TestSingleCellRelaxationAndApoptosis() throw (Exception)
    {
        // Construct a 2D vertex mesh consisting of a single element
        std::vector<Node<2>*> nodes;
        unsigned num_nodes = 20;
        for (unsigned i=0; i<num_nodes; i++)
        {
            double theta = M_PI+2.0*M_PI*(double)(i)/(double)(num_nodes);
            nodes.push_back(new Node<2>(i, true, cos(theta), sin(theta)));
        }

        std::vector<VertexElement<2,2>*> elements;
        elements.push_back(new VertexElement<2,2>(0, nodes));

        MutableVertexMesh<2,2> mesh(nodes, elements);
        mesh.SetCellRearrangementThreshold(0.1);

        // Set up cells, one for each VertexElement
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        for (unsigned elem_index=0; elem_index<mesh.GetNumElements(); elem_index++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(DIFFERENTIATED);

            CellPtr p_cell(new Cell(p_state, p_model));
            double birth_time = -1.0;
            p_cell->SetBirthTime(birth_time);
            cells.push_back(p_cell);
        }

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(mesh, cells);

        // Create a force system
        NagaiHondaForce<2> force;
        std::vector<AbstractForce<2>* > force_collection;
        force_collection.push_back(&force);

        // Set up cell-based simulation
        CellBasedSimulation<2> simulator(cell_population, force_collection);
        simulator.SetOutputDirectory("TestVertexSingleCellApoptosis");
        simulator.SetEndTime(2.0);

        // Run simulation
        simulator.Solve();

        // Test relaxes to circle (can be more stringent with more nodes and more time)
        TS_ASSERT_DELTA(cell_population.rGetMesh().GetVolumeOfElement(0), 1.0, 1e-1);
        TS_ASSERT_DELTA(cell_population.rGetMesh().GetSurfaceAreaOfElement(0), 3.5449077, 1e-1);

        CellPtr p_cell = simulator.rGetCellPopulation().GetCellUsingLocationIndex(0);
        p_cell->StartApoptosis(false);

        simulator.SetEndTime(2.25); // any longer and cell target area is zero but element can't be removed as its the only one.

        // Run simulation
        simulator.Solve();

        TS_ASSERT_DELTA(cell_population.rGetMesh().GetVolumeOfElement(0), 0.5098, 1e-4);
        TS_ASSERT_DELTA(cell_population.rGetMesh().GetSurfaceAreaOfElement(0), 2.5417, 1e-3);

        // Test Warnings
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNumWarnings(), 1u);
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNextWarningMessage(),"Vertices are moving more than half the CellRearrangementThreshold this could cause elements to become inverted the motion has been restricted: - To avoid these warnings use a smaller timestep");
        Warnings::QuietDestroy();
    }


    /*
     * This test is to stress test the vertex simulations by creating a massive monolayer,
     * it would also be useful for benchmarking.
     *
     * \todo In order to work the mesh archiving must support boundary nodes (see #1076)
     */
    void noTestVertexStressTest() throw (Exception)
    {
        double start_time=0.0;
        double end_time=100.0;
        std::string output_directory = "StressTestVertex";

        // Create a simple 2D MutableVertexMesh
        HoneycombMutableVertexMeshGenerator generator(3, 3);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

        // Set up cells, one for each VertexElement. Give each cell
        // a birth time of -elem_index, so its age is elem_index
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        for (unsigned elem_index=0; elem_index<p_mesh->GetNumElements(); elem_index++)
        {
            StochasticDurationGenerationBasedCellCycleModel* p_model = new StochasticDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(STEM);

            CellPtr p_cell(new Cell(p_state, p_model));
            double birth_time = -(double)elem_index;
            p_cell->SetBirthTime(birth_time);
            cells.push_back(p_cell);
        }

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);

        // Create a force system
        NagaiHondaForce<2> force;
        std::vector<AbstractForce<2>* > force_collection;
        force_collection.push_back(&force);

        // Set up cell-based simulation
        CellBasedSimulation<2> simulator(cell_population, force_collection);

        simulator.SetOutputDirectory(output_directory);
        simulator.SetSamplingTimestepMultiple(50);

        simulator.SetEndTime(end_time);

        // Run simulation
        simulator.Solve();

        // Archive now and then reload

        // Save simulation in steady state
        CellBasedSimulationArchiver<2, CellBasedSimulation<2> >::Save(&simulator);

        // Now save and reload to find where it breaks!
        for (unsigned i=0; i<40; i++)
        {
            start_time = end_time;
            end_time = end_time + 10.0;

            CellBasedSimulation<2>* p_simulator = CellBasedSimulationArchiver<2, CellBasedSimulation<2> >::Load(output_directory,start_time);
            p_simulator->SetDt(0.002);
            p_simulator->SetSamplingTimestepMultiple(50);
            p_simulator->SetEndTime(end_time);
            p_simulator->Solve();

            CellBasedSimulationArchiver<2, CellBasedSimulation<2> >::Save(p_simulator);
            delete p_simulator;
        }
        Warnings::QuietDestroy();
    }


    // Test archiving of a CellBasedSimulation that uses a VertexBasedCellPopulation.
    void TestArchiving() throw (Exception)
    {
        // Set end time
        double end_time = 0.1;

        // Create a simple 2D MutableVertexMesh
        HoneycombMutableVertexMeshGenerator generator(6, 6);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

        // Set up cells, one for each VertexElement. Give each cell
        // a birth time of -elem_index, so its age is elem_index
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        for (unsigned elem_index=0; elem_index<p_mesh->GetNumElements(); elem_index++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(DIFFERENTIATED);

            CellPtr p_cell(new Cell(p_state, p_model));
            double birth_time = 0.0 - elem_index;
            p_cell->SetBirthTime(birth_time);
            cells.push_back(p_cell);
        }

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);

        // Create a force system
        NagaiHondaForce<2> force;
        std::vector<AbstractForce<2>* > force_collection;
        force_collection.push_back(&force);

        // Set up cell-based simulation
        CellBasedSimulation<2> simulator(cell_population, force_collection);
        simulator.SetOutputDirectory("TestCellBasedSimulationWithVertexBasedCellPopulationSaveAndLoad");
        simulator.SetEndTime(end_time);

        TS_ASSERT_DELTA(simulator.GetDt(), 0.002, 1e-12);

        // Run and save simulation
        TS_ASSERT_THROWS_NOTHING(simulator.Solve());

        CellBasedSimulationArchiver<2, CellBasedSimulation<2> >::Save(&simulator);

        TS_ASSERT_EQUALS(simulator.rGetCellPopulation().GetNumRealCells(), 36u);
        TS_ASSERT_EQUALS(simulator.rGetCellPopulation().GetNumNodes(), 96u);
        TS_ASSERT_EQUALS((static_cast<VertexBasedCellPopulation<2>*>(&(simulator.rGetCellPopulation())))->GetNumElements(), 36u);

        TS_ASSERT_DELTA(SimulationTime::Instance()->GetTime(), 0.1, 1e-9);
        CellPtr p_cell = simulator.rGetCellPopulation().GetCellUsingLocationIndex(23);
        TS_ASSERT_DELTA(p_cell->GetAge(), 23.1, 1e-4);

        SimulationTime::Destroy();
        SimulationTime::Instance()->SetStartTime(0.0);

        // Load simulation
        CellBasedSimulation<2>* p_simulator
            = CellBasedSimulationArchiver<2, CellBasedSimulation<2> >::Load("TestCellBasedSimulationWithVertexBasedCellPopulationSaveAndLoad", end_time);

        p_simulator->SetEndTime(0.2);

        TS_ASSERT_EQUALS(p_simulator->rGetCellPopulation().GetNumRealCells(), 36u);
        TS_ASSERT_EQUALS(p_simulator->rGetCellPopulation().GetNumNodes(), 96u);
        TS_ASSERT_EQUALS((static_cast<VertexBasedCellPopulation<2>*>(&(p_simulator->rGetCellPopulation())))->GetNumElements(), 36u);

        TS_ASSERT_DELTA(SimulationTime::Instance()->GetTime(), 0.1, 1e-9);
        CellPtr p_cell2 = p_simulator->rGetCellPopulation().GetCellUsingLocationIndex(23);
        TS_ASSERT_DELTA(p_cell2->GetAge(), 23.1, 1e-4);

        // Run simulation
        TS_ASSERT_THROWS_NOTHING(p_simulator->Solve());

        // Tidy up
        delete p_simulator;

        // Test Warnings
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNumWarnings(), 1u);
        TS_ASSERT_EQUALS(Warnings::Instance()->GetNextWarningMessage(),"Vertices are moving more than half the CellRearrangementThreshold this could cause elements to become inverted the motion has been restricted: - To avoid these warnings use a smaller timestep");
        Warnings::QuietDestroy();

    }
};

#endif /*TESTCELLBASEDSIMULATIONWITHVERTEXBASEDCELLPOPULATION_HPP_*/