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
#ifndef TESTFORCES_HPP_
#define TESTFORCES_HPP_

#include <cxxtest/TestSuite.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "CryptCellsGenerator.hpp"
#include "FixedDurationGenerationBasedCellCycleModel.hpp"
#include "VanLeeuwen2009WntSwatCellCycleModelHypothesisOne.hpp"
#include "VanLeeuwen2009WntSwatCellCycleModelHypothesisTwo.hpp"
#include "MeshBasedCellPopulationWithGhostNodes.hpp"
#include "HoneycombMeshGenerator.hpp"
#include "LinearSpringWithVariableSpringConstantsForce.hpp"
#include "WntConcentration.hpp"
#include "AbstractCellBasedTestSuite.hpp"
#include "ApcOneHitCellMutationState.hpp"
#include "ApcTwoHitCellMutationState.hpp"
#include "BetaCateninOneHitCellMutationState.hpp"
#include "WildTypeCellMutationState.hpp"
#include "CellLabel.hpp"

class TestForces : public AbstractCellBasedTestSuite
{
public:

    void TestGeneralisedLinearSpringForceMethods() throw (Exception)
    {
        unsigned cells_across = 7;
        unsigned cells_up = 5;
        unsigned thickness_of_ghost_layer = 3;

        SimulationTime::Instance()->SetEndTimeAndNumberOfTimeSteps(1.0,1);

        HoneycombMeshGenerator generator(cells_across, cells_up, thickness_of_ghost_layer, false);
        MutableMesh<2,2>* p_mesh = generator.GetMesh();
        std::vector<unsigned> location_indices = generator.GetCellLocationIndices();

        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        boost::shared_ptr<AbstractCellMutationState> p_apc2(new ApcTwoHitCellMutationState);
        for (unsigned i=0; i<location_indices.size(); i++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(STEM);

            if (i==60)
            {
                CellPtr p_cell(new Cell(p_apc2, p_model));
                p_cell->SetBirthTime(-10);
                cells.push_back(p_cell);
            }
            else
            {
                CellPtr p_cell(new Cell(p_state, p_model));
                p_cell->SetBirthTime(-10);
                cells.push_back(p_cell);
            }
        }

        MeshBasedCellPopulationWithGhostNodes<2> cell_population(*p_mesh, cells, location_indices);
        GeneralisedLinearSpringForce<2> linear_force;

        // Test set/get method
        TS_ASSERT_DELTA(linear_force.GetMeinekeDivisionRestingSpringLength(), 0.5, 1e-6);
        TS_ASSERT_DELTA(linear_force.GetMeinekeSpringStiffness(), 15.0, 1e-6);

        linear_force.SetMeinekeDivisionRestingSpringLength(0.8);
        linear_force.SetMeinekeSpringStiffness(20.0);

        TS_ASSERT_DELTA(linear_force.GetMeinekeDivisionRestingSpringLength(), 0.8, 1e-6);
        TS_ASSERT_DELTA(linear_force.GetMeinekeSpringStiffness(), 20.0, 1e-6);

        linear_force.SetMeinekeDivisionRestingSpringLength(0.5);
        linear_force.SetMeinekeSpringStiffness(15.0);

        /*
         ************************************************************************
         ************************************************************************
         *  Test node force calculation
         ************************************************************************
         ************************************************************************
         */

        // Initialise a vector of node forces
        std::vector<c_vector<double, 2> > node_forces;
        node_forces.reserve(cell_population.GetNumNodes());

        for (unsigned i=0; i<cell_population.GetNumNodes(); i++)
        {
             node_forces.push_back(zero_vector<double>(2));
        }

        linear_force.AddForceContribution(node_forces, cell_population);

        // Test forces on non-ghost nodes
        for (AbstractCellPopulation<2>::Iterator cell_iter = cell_population.Begin();
             cell_iter != cell_population.End();
             ++cell_iter)
        {
            unsigned node_index = cell_population.GetLocationIndexUsingCell(*cell_iter);

            TS_ASSERT_DELTA(node_forces[node_index][0], 0.0, 1e-4);
            TS_ASSERT_DELTA(node_forces[node_index][1], 0.0, 1e-4);
        }

        // Move a node along the x-axis and calculate the force exerted on a neighbour
        c_vector<double,2> old_point = p_mesh->GetNode(59)->rGetLocation();
        ChastePoint<2> new_point;
        new_point.rGetLocation()[0] = old_point[0]+0.5;
        new_point.rGetLocation()[1] = old_point[1];

        p_mesh->SetNode(59, new_point, false);

        // Initialise a vector of new node forces
        std::vector<c_vector<double, 2> > new_node_forces;
        new_node_forces.reserve(cell_population.GetNumNodes());

        for (unsigned i=0; i<cell_population.GetNumNodes(); i++)
        {
             new_node_forces.push_back(zero_vector<double>(2));
        }
        linear_force.AddForceContribution(new_node_forces, cell_population);

        TS_ASSERT_DELTA(new_node_forces[60][0], 0.5*linear_force.GetMeinekeSpringStiffness(), 1e-4);
        TS_ASSERT_DELTA(new_node_forces[60][1], 0.0, 1e-4);

        TS_ASSERT_DELTA(new_node_forces[59][0], (-3+4.0/sqrt(7))*linear_force.GetMeinekeSpringStiffness(), 1e-4);
        TS_ASSERT_DELTA(new_node_forces[59][1], 0.0, 1e-4);

        TS_ASSERT_DELTA(new_node_forces[58][0], 0.5*linear_force.GetMeinekeSpringStiffness(), 1e-4);
        TS_ASSERT_DELTA(new_node_forces[58][1], 0.0, 1e-4);

        /*
         ************************************************************************
         ************************************************************************
         *  Test spring force calculation
         ************************************************************************
         ************************************************************************
         */

        c_vector<double,2> force_on_spring; // between nodes 59 and 60

        // Find one of the elements that nodes 59 and 60 live on
        ChastePoint<2> new_point2;
        new_point2.rGetLocation()[0] = new_point[0] + 0.01;
        new_point2.rGetLocation()[1] = new_point[1] + 0.01;

        unsigned elem_index = p_mesh->GetContainingElementIndex(new_point2, false);
        Element<2,2>* p_element = p_mesh->GetElement(elem_index);

        force_on_spring = linear_force.CalculateForceBetweenNodes(p_element->GetNodeGlobalIndex(1),
                                                                  p_element->GetNodeGlobalIndex(0),
                                                                  cell_population);

        TS_ASSERT_DELTA(force_on_spring[0], 0.5*linear_force.GetMeinekeSpringStiffness(), 1e-4);
        TS_ASSERT_DELTA(force_on_spring[1], 0.0, 1e-4);

        /*
         ******************************************
         ******************************************
         *  Test force with cutoff point
         ******************************************
         ******************************************
         */
        double dist = norm_2( p_mesh->GetVectorFromAtoB(p_element->GetNode(0)->rGetLocation(),
                              p_element->GetNode(1)->rGetLocation()) );

        linear_force.UseCutoffPoint(dist-0.1);

        // Coverage
        TS_ASSERT_DELTA(linear_force.GetCutoffPoint(), dist-0.1, 1e-4);

        force_on_spring = linear_force.CalculateForceBetweenNodes(p_element->GetNodeGlobalIndex(1),
                                                                  p_element->GetNodeGlobalIndex(0),
                                                                  cell_population);
        TS_ASSERT_DELTA(force_on_spring[0], 0.0, 1e-4);
        TS_ASSERT_DELTA(force_on_spring[1], 0.0, 1e-4);
    }


    void TestGeneralisedLinearSpringForceWithEdgeLengthBasedSpring() throw (Exception)
    {
        // Set up the simulation time
        SimulationTime::Instance()->SetEndTimeAndNumberOfTimeSteps(1.0, 1);

        unsigned cells_across = 6;
        unsigned cells_up = 12;
        double crypt_width = 5.0;
        unsigned thickness_of_ghost_layer = 3;

        HoneycombMeshGenerator generator(cells_across, cells_up,thickness_of_ghost_layer, false, crypt_width/cells_across);
        MutableMesh<2,2>* p_mesh = generator.GetMesh();
        std::vector<unsigned> location_indices = generator.GetCellLocationIndices();

        // Set up cells
        std::vector<CellPtr> cells;
        CryptCellsGenerator<FixedDurationGenerationBasedCellCycleModel> cells_generator;
        cells_generator.Generate(cells, p_mesh, location_indices, true); // true = mature cells

        MeshBasedCellPopulationWithGhostNodes<2> cell_population(*p_mesh, cells, location_indices);
        LinearSpringWithVariableSpringConstantsForce<2> linear_force;

        // Check that the force between nodes is correctly calculated when the 'spring constant' is constant
        linear_force.SetEdgeBasedSpringConstant(false);

        for (MeshBasedCellPopulation<2>::SpringIterator spring_iterator = cell_population.SpringsBegin();
            spring_iterator != cell_population.SpringsEnd();
            ++spring_iterator)
        {
            unsigned nodeA_global_index = spring_iterator.GetNodeA()->GetIndex();
            unsigned nodeB_global_index = spring_iterator.GetNodeB()->GetIndex();
            c_vector<double, 2> force = linear_force.CalculateForceBetweenNodes(nodeA_global_index,
                                                                                nodeB_global_index,
                                                                                cell_population);
            TS_ASSERT_DELTA(force[0]*force[0] + force[1]*force[1], 6.25, 1e-3);
        }

        // Check that the force between nodes is correctly calculated when the 'spring constant'
        // is proportional to the length of the edge between adjacent cells
        linear_force.SetEdgeBasedSpringConstant(true);
        cell_population.CreateVoronoiTessellation();  // normally done in a simulation loop

        for (MeshBasedCellPopulation<2>::SpringIterator spring_iterator = cell_population.SpringsBegin();
             spring_iterator != cell_population.SpringsEnd();
             ++spring_iterator)
        {
            unsigned nodeA_global_index = spring_iterator.GetNodeA()->GetIndex();
            unsigned nodeB_global_index = spring_iterator.GetNodeB()->GetIndex();
            c_vector<double, 2> force = linear_force.CalculateForceBetweenNodes(nodeA_global_index,
                                                                                nodeB_global_index,
                                                                                cell_population);

            TS_ASSERT_DELTA(force[0]*force[0] + force[1]*force[1], 4.34027778, 1e-3);
        }

        // Choose two interior neighbour nodes
        c_vector<double, 2> force = linear_force.CalculateForceBetweenNodes(41, 42, cell_population);
        TS_ASSERT_DELTA(force[0]*force[0] + force[1]*force[1], 4.34027778, 1e-3);

        // Now move node 42 a bit and check that the force calculation changes correctly
        c_vector<double,2> shift;
        shift[0] = 0.1;
        shift[1] = 0.0;
        ChastePoint<2> new_point(p_mesh->GetNode(42u)->rGetLocation() + shift);
        p_mesh->SetNode(21, new_point, false);

        // Check that the new force between nodes is correctly calculated
        cell_population.CreateVoronoiTessellation();
        c_vector<double, 2> new_force = linear_force.CalculateForceBetweenNodes(41, 42, cell_population);

        // Force calculation: shift is along x-axis so we should have
        // new_edge_length = (5/6 + shift[0])*tan(0.5*arctan(5*sqrt(3)/(5 + 12*shift[0]))),
        // force^2 = mu^2 * (new_edge_length*sqrt(3))^2 * (1 - 5/6 - shift[0])^2
        TS_ASSERT_DELTA(new_force[0]*new_force[0] + new_force[1]*new_force[1], 4.34024, 1e-3);
    }

    // Test on a periodic mesh
    void TestGeneralisedLinearSpringForceWithEdgeBasedSpringsOnPeriodicMesh() throw (Exception)
    {
        // Set up the simulation time
        SimulationTime::Instance()->SetEndTimeAndNumberOfTimeSteps(1.0,1);

        unsigned cells_across = 6;
        unsigned cells_up = 12;
        double crypt_width = 5.0;
        unsigned thickness_of_ghost_layer = 3;

        HoneycombMeshGenerator generator(cells_across, cells_up,thickness_of_ghost_layer, true, crypt_width/cells_across);
        Cylindrical2dMesh* p_mesh = generator.GetCylindricalMesh();
        std::vector<unsigned> location_indices = generator.GetCellLocationIndices();

        // Set up cells
        std::vector<CellPtr> cells;
        CryptCellsGenerator<FixedDurationGenerationBasedCellCycleModel> cells_generator;
        cells_generator.Generate(cells, p_mesh, location_indices, true);// true = mature cells

        MeshBasedCellPopulationWithGhostNodes<2> cell_population(*p_mesh, cells, location_indices);
        LinearSpringWithVariableSpringConstantsForce<2> linear_force;

        // Check that the force between nodes is correctly calculated when the spring constant is constant (!)
        linear_force.SetEdgeBasedSpringConstant(false);

        for (MeshBasedCellPopulation<2>::SpringIterator spring_iterator = cell_population.SpringsBegin();
             spring_iterator != cell_population.SpringsEnd();
             ++spring_iterator)
        {
            unsigned nodeA_global_index = spring_iterator.GetNodeA()->GetIndex();
            unsigned nodeB_global_index = spring_iterator.GetNodeB()->GetIndex();
            c_vector<double, 2> force = linear_force.CalculateForceBetweenNodes(nodeA_global_index,
                                                                                nodeB_global_index,
                                                                                cell_population);

            TS_ASSERT_DELTA(force[0]*force[0] + force[1]*force[1], 6.25, 1e-3);
        }

        // Check that the force between nodes is correctly calculated when the spring constant
        // is proportional to the length of the edge between adjacenet cells
        linear_force.SetEdgeBasedSpringConstant(true);
        cell_population.CreateVoronoiTessellation();

        for (MeshBasedCellPopulation<2>::SpringIterator spring_iterator = cell_population.SpringsBegin();
             spring_iterator != cell_population.SpringsEnd();
             ++spring_iterator)
        {
            unsigned nodeA_global_index = spring_iterator.GetNodeA()->GetIndex();
            unsigned nodeB_global_index = spring_iterator.GetNodeB()->GetIndex();
            c_vector<double, 2> force = linear_force.CalculateForceBetweenNodes(nodeA_global_index,
                                                                                nodeB_global_index,
                                                                                cell_population);
            TS_ASSERT_DELTA(force[0]*force[0] + force[1]*force[1], 4.34027778, 1e-3);
        }
    }

    void TestGeneralisedLinearSpringForceWithSpringConstantsForMutantCells()
    {
        // Create a small cell population
        std::vector<Node<2>*> nodes;
        nodes.push_back(new Node<2>(0, false, 0, 0));
        nodes.push_back(new Node<2>(1, false, 0, 2));
        nodes.push_back(new Node<2>(2, false, 2, 2));
        nodes.push_back(new Node<2>(3, false, 2, 0));

        MutableMesh<2,2> mesh(nodes);

        std::vector<CellPtr> cells;
        CellsGenerator<FixedDurationGenerationBasedCellCycleModel, 2> cells_generator;
        cells_generator.GenerateBasic(cells, mesh.GetNumNodes());

        MeshBasedCellPopulation<2> cell_population(mesh, cells);

        LinearSpringWithVariableSpringConstantsForce<2> linear_force;

        // Set cells' mutation states
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        boost::shared_ptr<AbstractCellMutationState> p_apc2(new ApcTwoHitCellMutationState);
        boost::shared_ptr<AbstractCellMutationState> p_bcat1(new BetaCateninOneHitCellMutationState);
        boost::shared_ptr<AbstractCellProperty> p_label(new CellLabel);

        cell_population.GetCellUsingLocationIndex(0)->SetMutationState(p_state);
        cell_population.GetCellUsingLocationIndex(1)->AddCellProperty(p_label);
        cell_population.GetCellUsingLocationIndex(2)->SetMutationState(p_apc2);
        cell_population.GetCellUsingLocationIndex(3)->SetMutationState(p_bcat1);

        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(0, 1, cell_population)), 15.0, 1e-10);
        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(1, 2, cell_population)), 15.0, 1e-10);
        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(2, 3, cell_population)), 15.0, 1e-10);
        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(3, 0, cell_population)), 15.0, 1e-10);

        linear_force.SetMutantSprings(true);

        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(0, 1, cell_population)), 15.0, 1e-10);
        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(1, 2, cell_population)), 22.5, 1e-10);
        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(2, 3, cell_population)), 30.0, 1e-10);
        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(3, 0, cell_population)), 22.5, 1e-10);

        linear_force.SetMutantSprings(true, 4.0, 3.0);

        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(0, 1, cell_population)), 15.0, 1e-10);
        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(1, 2, cell_population)), 45.0, 1e-10);
        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(2, 3, cell_population)), 60.0, 1e-10);
        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(3, 0, cell_population)), 45.0, 1e-10);
    }

    void TestGeneralisedLinearSpringForceWithSpringConstantsForIngeBCatCells()
    {
        SimulationTime::Instance()->SetEndTimeAndNumberOfTimeSteps(1.0,1);

        HoneycombMeshGenerator generator(6, 12, 0, true, 1.1);
        Cylindrical2dMesh* p_mesh = generator.GetCylindricalMesh();
        std::vector<unsigned> location_indices = generator.GetCellLocationIndices();

        // Set up cells
        std::vector<CellPtr> cells;
        CryptCellsGenerator<VanLeeuwen2009WntSwatCellCycleModelHypothesisTwo> cells_generator;
        cells_generator.Generate(cells, p_mesh, location_indices, false);

        MeshBasedCellPopulationWithGhostNodes<2> crypt(*p_mesh, cells, location_indices);

        WntConcentration<2>::Instance()->SetType(LINEAR);
        WntConcentration<2>::Instance()->SetCellPopulation(crypt);

        // As there is no cell-based simulation, we must explicitly initialise the cells
        crypt.InitialiseCells();

        LinearSpringWithVariableSpringConstantsForce<2> linear_force;

        TS_ASSERT_DELTA(norm_2(linear_force.CalculateForceBetweenNodes(20, 21, crypt)), 1.50, 1e-10);

        linear_force.SetBetaCateninSprings(true);
        crypt.CreateVoronoiTessellation();  // this method is normally called in a simulation loop

        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(20, 21, crypt)), 1.5*8.59312/18.14, 1e-5);

        linear_force.SetBetaCatSpringScaler(20/6.0);
        TS_ASSERT_DELTA( norm_2(linear_force.CalculateForceBetweenNodes(20, 21, crypt)), 1.5*8.59312/20.0, 1e-5);

        // Tidy up
        WntConcentration<2>::Destroy();
    }

    void TestGeneralisedLinearSpringForceWithSpringConstantsForApoptoticCells()
    {
        // Set up stretched cell population
        HoneycombMeshGenerator generator(4, 4, 0, false, 2.0);
        MutableMesh<2,2>* p_mesh = generator.GetMesh();
        std::vector<unsigned> location_indices = generator.GetCellLocationIndices();

        std::vector<CellPtr> cells;
        CellsGenerator<FixedDurationGenerationBasedCellCycleModel,2> cells_generator;
        cells_generator.GenerateGivenLocationIndices(cells, location_indices);

        MeshBasedCellPopulationWithGhostNodes<2> stretched_cell_population(*p_mesh, cells, location_indices);

        // As there is no cell-based simulation we must explicitly initialise the cells
        stretched_cell_population.InitialiseCells();

        // Set one of the non-boundary cells to be necrotic
        boost::shared_ptr<AbstractCellProperty> p_apoptotic_state(new ApoptoticCellProperty);
        stretched_cell_population.GetCellUsingLocationIndex(6)->AddCellProperty(p_apoptotic_state);

        LinearSpringWithVariableSpringConstantsForce<2> linear_force;
        linear_force.SetApoptoticSprings(true);

        TS_ASSERT_EQUALS(stretched_cell_population.GetCellUsingLocationIndex(6)->HasCellProperty<ApoptoticCellProperty>(), true);
        TS_ASSERT_DELTA(norm_2(linear_force.CalculateForceBetweenNodes(6, 10, stretched_cell_population)), 3.3333, 1e-4);

        // Set a neighbouring cell to be necrotic
        stretched_cell_population.GetCellUsingLocationIndex(10)->AddCellProperty(p_apoptotic_state);

        TS_ASSERT_EQUALS(stretched_cell_population.GetCellUsingLocationIndex(10)->HasCellProperty<ApoptoticCellProperty>(), true);
        TS_ASSERT_DELTA(norm_2(linear_force.CalculateForceBetweenNodes(6, 10, stretched_cell_population)), 1.8750, 1e-4);

        // Now do similar tests for a squashed cell population
        HoneycombMeshGenerator generator2(4, 4, 0, false, 0.5);
        MutableMesh<2,2>* p_mesh2 = generator2.GetMesh();
        std::vector<unsigned> location_indices2 = generator2.GetCellLocationIndices();

        std::vector<CellPtr> cells2;
        CellsGenerator<FixedDurationGenerationBasedCellCycleModel,2> cells_generator2;
        cells_generator2.GenerateGivenLocationIndices(cells2, location_indices2);

        MeshBasedCellPopulationWithGhostNodes<2> squashed_cell_population(*p_mesh2, cells2, location_indices2);
        squashed_cell_population.InitialiseCells();

        squashed_cell_population.GetCellUsingLocationIndex(6)->AddCellProperty(p_apoptotic_state);

        LinearSpringWithVariableSpringConstantsForce<2> linear_force2;

        // Test set/get methods
        TS_ASSERT_DELTA(linear_force2.GetApoptoticSpringTensionStiffness(), 3.75, 1e-6);
        TS_ASSERT_DELTA(linear_force2.GetApoptoticSpringCompressionStiffness(), 11.25, 1e-6);

        linear_force2.SetApoptoticSprings(true);

        TS_ASSERT_DELTA(norm_2(linear_force2.CalculateForceBetweenNodes(6, 10, squashed_cell_population)), 4.0909, 1e-4);

        squashed_cell_population.GetCellUsingLocationIndex(10)->AddCellProperty(p_apoptotic_state);

        TS_ASSERT_DELTA( norm_2(linear_force2.CalculateForceBetweenNodes(6, 10, squashed_cell_population)), 2.8125, 1e-4);
    }

    void TestGeneralisedLinearSpringForceCalculationIn1d() throw (Exception)
    {
        // Create a 1D mesh with nodes equally spaced a unit distance apart
        MutableMesh<1,1> mesh;
        mesh.ConstructLinearMesh(5);

        // Set up cells
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        for (unsigned node_index=0; node_index<mesh.GetNumNodes(); node_index++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(DIFFERENTIATED);

            CellPtr p_cell(new Cell(p_state, p_model));

            double birth_time = 0.0 - node_index;
            p_cell->SetBirthTime(birth_time);

            cells.push_back(p_cell);
        }

        // Create cell population
        std::vector<CellPtr> cells_copy(cells);
        MeshBasedCellPopulation<1> cell_population(mesh, cells);

        // Create force law object
        GeneralisedLinearSpringForce<1> linear_force;

        // Initialise a vector of node forces
        std::vector<c_vector<double, 1> > node_forces;
        node_forces.reserve(cell_population.GetNumNodes());

        for (unsigned i=0; i<cell_population.GetNumNodes(); i++)
        {
            node_forces.push_back(zero_vector<double>(1));
        }

        // Compute forces on nodes
        linear_force.AddForceContribution(node_forces, cell_population);

        // Test that all springs are in equilibrium
        for (unsigned node_index=0; node_index<cell_population.GetNumNodes(); node_index++)
        {
            TS_ASSERT_DELTA(node_forces[node_index](0), 0.0, 1e-6);
        }

        // Scale entire mesh and check that forces are correctly calculated
        double scale_factor = 1.5;
        for (unsigned node_index=0; node_index<mesh.GetNumNodes(); node_index++)
        {
            c_vector<double,1> old_point = mesh.GetNode(node_index)->rGetLocation();
            ChastePoint<1> new_point;
            new_point.rGetLocation()[0] = scale_factor*old_point[0];
            mesh.SetNode(node_index, new_point, false);
        }

        // Recalculate node forces (we can re-use node_forces
        // as previously each node had zero net force on it)
        linear_force.AddForceContribution(node_forces, cell_population);

        for (unsigned node_index=0; node_index<cell_population.GetNumNodes(); node_index++)
        {
            if (node_index == 0)
            {
                // The first node only experiences a force from its neighbour to the right
                TS_ASSERT_DELTA(node_forces[node_index](0), linear_force.GetMeinekeSpringStiffness()*(scale_factor-1), 1e-6);
            }
            else if (node_index == cell_population.GetNumNodes()-1)
            {
                // The last node only experiences a force from its neighbour to the left
                TS_ASSERT_DELTA(node_forces[node_index](0), -linear_force.GetMeinekeSpringStiffness()*(scale_factor-1), 1e-6);
            }
            else
            {
                // The net force on each interior node should still be zero
                TS_ASSERT_DELTA(node_forces[node_index](0), 0.0, 1e-6);
            }
        }

        // Create another cell population and force law
        MutableMesh<1,1> mesh2;
        mesh2.ConstructLinearMesh(5);

        MeshBasedCellPopulation<1> cell_population2(mesh2, cells_copy);
        GeneralisedLinearSpringForce<1> linear_force2;

        // Move one node and check that forces are correctly calculated
        ChastePoint<1> shifted_point;
        shifted_point.rGetLocation()[0] = 2.5;
        mesh2.SetNode(2, shifted_point);

        c_vector<double,1> force_between_1_and_2 = linear_force2.CalculateForceBetweenNodes(1, 2, cell_population2);
        TS_ASSERT_DELTA(force_between_1_and_2[0], linear_force.GetMeinekeSpringStiffness()*0.5, 1e-6);

        c_vector<double,1> force_between_2_and_3 = linear_force2.CalculateForceBetweenNodes(2, 3, cell_population2);
        TS_ASSERT_DELTA(force_between_2_and_3[0], -linear_force.GetMeinekeSpringStiffness()*0.5, 1e-6);

        // Initialise a vector of node forces
        std::vector<c_vector<double,1> > node_forces2;
        node_forces2.reserve(cell_population2.GetNumNodes());

        for (unsigned i=0; i<cell_population2.GetNumNodes(); i++)
        {
             node_forces2.push_back(zero_vector<double>(1));
        }

        linear_force2.AddForceContribution(node_forces2, cell_population2);

        TS_ASSERT_DELTA(node_forces2[2](0), -linear_force.GetMeinekeSpringStiffness(), 1e-6);
    }

    void TestGeneralisedLinearSpringForceCalculationIn3d() throw (Exception)
    {
        SimulationTime::Instance()->SetEndTimeAndNumberOfTimeSteps(1.0,1);

        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/3D_Single_tetrahedron_element");
        MutableMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);

        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);

        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
	        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
	        p_model->SetCellProliferativeType(STEM);

	        CellPtr p_cell(new Cell(p_state, p_model));
            p_cell->SetBirthTime(-50.0);
            cells.push_back(p_cell);
        }

        std::vector<CellPtr> cells_copy(cells);
        MeshBasedCellPopulation<3> cell_population(mesh, cells);
        GeneralisedLinearSpringForce<3> linear_force;

        // Test forces on springs
        unsigned nodeA = 0, nodeB = 1;
        Element<3,3>* p_element = mesh.GetElement(0);
        c_vector<double, 3> force = linear_force.CalculateForceBetweenNodes(p_element->GetNodeGlobalIndex(nodeA),
                                                                            p_element->GetNodeGlobalIndex(nodeB),
                                                                            cell_population);
        for (unsigned i=0; i<3; i++)
        {
            TS_ASSERT_DELTA(force[i], 0.0, 1e-6);
        }

        // Initialise a vector of node forces
        std::vector<c_vector<double, 3> > node_forces;
        node_forces.reserve(cell_population.GetNumNodes());

        for (unsigned i=0; i<cell_population.GetNumNodes(); i++)
        {
             node_forces.push_back(zero_vector<double>(3));
        }

        linear_force.AddForceContribution(node_forces, cell_population);

        for (unsigned j=0; j<4; j++)
        {
            for (unsigned k=0; k<3; k++)
            {
                TS_ASSERT_DELTA(node_forces[j](k), 0.0, 1e-6);
            }
        }

        // Scale entire mesh and check that forces are correctly calculated
        double scale_factor = 1.5;

        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            c_vector<double,3> old_point = mesh.GetNode(i)->rGetLocation();
            ChastePoint<3> new_point;
            new_point.rGetLocation()[0] = scale_factor*old_point[0];
            new_point.rGetLocation()[1] = scale_factor*old_point[1];
            new_point.rGetLocation()[2] = scale_factor*old_point[2];
            mesh.SetNode(i, new_point, false);
        }

        // Recalculate node forces (we can just re-use node_forces,
        // as previously each node had zero net force on it)
        linear_force.AddForceContribution(node_forces, cell_population);

        for (unsigned j=0; j<4; j++)
        {
            for (unsigned k=0; k<3; k++)
            {
                TS_ASSERT_DELTA(fabs(node_forces[j](k)), linear_force.GetMeinekeSpringStiffness()*(scale_factor-1)*sqrt(2),1e-6);
            }
        }

        // Move one node and check that forces are correctly calculated
        MutableMesh<3,3> mesh2;
        mesh2.ConstructFromMeshReader(mesh_reader);

        MeshBasedCellPopulation<3> cell_population2(mesh2, cells_copy);
        GeneralisedLinearSpringForce<3> linear_force2;

        c_vector<double,3> old_point = mesh2.GetNode(0)->rGetLocation();
        ChastePoint<3> new_point;
        new_point.rGetLocation()[0] = 0.0;
        new_point.rGetLocation()[1] = 0.0;
        new_point.rGetLocation()[2] = 0.0;
        mesh2.SetNode(0, new_point, false);

        unsigned nodeA2 = 0, nodeB2 = 1;
        Element<3,3>* p_element2 = mesh2.GetElement(0);
        c_vector<double,3> force2 = linear_force2.CalculateForceBetweenNodes(p_element2->GetNodeGlobalIndex(nodeA2),
                                                                             p_element2->GetNodeGlobalIndex(nodeB2),
                                                                             cell_population2);

        for (unsigned i=0; i<3; i++)
        {
            TS_ASSERT_DELTA(fabs(force2[i]),linear_force.GetMeinekeSpringStiffness()*(1 - sqrt(3)/(2*sqrt(2)))/sqrt(3.0),1e-6);
        }

        // Initialise a vector of node forces
        std::vector<c_vector<double,3> > node_forces2;
        node_forces2.reserve(cell_population.GetNumNodes());

        for (unsigned i=0; i<cell_population.GetNumNodes(); i++)
        {
             node_forces2.push_back(zero_vector<double>(3));
        }

        linear_force2.AddForceContribution(node_forces2, cell_population2);

        for (unsigned i=0; i<3; i++)
        {
            TS_ASSERT_DELTA(node_forces2[0](i),linear_force.GetMeinekeSpringStiffness()*(1 - sqrt(3)/(2*sqrt(2)))/sqrt(3.0),1e-6);
        }
    }

    void TestForceOutputParameters()
    {
		std::string output_directory = "TestForcesOutputParameters";
		OutputFileHandler output_file_handler(output_directory, false);

		// Test with LinearSpringWithVariableSpringConstantsForce<2> variable_force;
    	LinearSpringWithVariableSpringConstantsForce<2> variable_force;
		TS_ASSERT_EQUALS(variable_force.GetIdentifier(), "LinearSpringWithVariableSpringConstantsForce-2");

		out_stream variable_force_parameter_file = output_file_handler.OpenOutputFile("variable_results.parameters");
		variable_force.OutputForceParameters(variable_force_parameter_file);
		variable_force_parameter_file->close();

		std::string variable_force_results_dir = output_file_handler.GetOutputDirectoryFullPath();
		TS_ASSERT_EQUALS(system(("diff " + variable_force_results_dir + "variable_results.parameters     	cell_based/test/data/TestForces/variable_results.parameters").c_str()), 0);

		// Test with GeneralisedLinearSpringForce
		GeneralisedLinearSpringForce<2> linear_force;

		TS_ASSERT_EQUALS(linear_force.GetIdentifier(), "GeneralisedLinearSpringForce-2");

		out_stream linear_force_parameter_file = output_file_handler.OpenOutputFile("linear_results.parameters");
		linear_force.OutputForceParameters(linear_force_parameter_file);
		linear_force_parameter_file->close();

		std::string linear_force_results_dir = output_file_handler.GetOutputDirectoryFullPath();
		TS_ASSERT_EQUALS(system(("diff " + linear_force_results_dir + "linear_results.parameters			cell_based/test/data/TestForces/linear_results.parameters").c_str()), 0);
    }


    void TestGeneralisedLinearSpringForceArchiving() throw (Exception)
    {
        OutputFileHandler handler("archive", false);    // don't erase contents of folder
        std::string archive_filename = handler.GetOutputDirectoryFullPath() + "meineke_spring_system.arch";

        {
            TrianglesMeshReader<2,2> mesh_reader("mesh/test/data/square_2_elements");

            MutableMesh<2,2> mesh;
            mesh.ConstructFromMeshReader(mesh_reader);

            SimulationTime::Instance()->SetEndTimeAndNumberOfTimeSteps(1.0,1);

            std::vector<CellPtr> cells;
            boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);

            for (unsigned i=0; i<mesh.GetNumNodes(); i++)
            {
		        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
		        p_model->SetCellProliferativeType(STEM);

		        CellPtr p_cell(new Cell(p_state, p_model));
                p_cell->SetBirthTime(-50.0);
                cells.push_back(p_cell);
            }

            MeshBasedCellPopulation<2> cell_population(mesh, cells);
            LinearSpringWithVariableSpringConstantsForce<2> linear_force;

            std::ofstream ofs(archive_filename.c_str());
            boost::archive::text_oarchive output_arch(ofs);

            // Serialize via pointer
            LinearSpringWithVariableSpringConstantsForce<2>* const p_linear_force = &linear_force;

            p_linear_force->UseCutoffPoint(1.1);
            p_linear_force->SetEdgeBasedSpringConstant(true);
            p_linear_force->SetMutantSprings(true, 0.2, 0.3);
            p_linear_force->SetBetaCateninSprings(true);
            p_linear_force->SetApoptoticSprings(true);
            p_linear_force->SetBetaCatSpringScaler(20/6.0);
            p_linear_force->SetApoptoticSpringTensionStiffness(4.0);
            p_linear_force->SetApoptoticSpringCompressionStiffness(5.0);

            output_arch << p_linear_force;
        }

        {
            ArchiveLocationInfo::SetMeshPathname("mesh/test/data", "square_2_elements");

            // Create an input archive
            std::ifstream ifs(archive_filename.c_str(), std::ios::binary);
            boost::archive::text_iarchive input_arch(ifs);

            LinearSpringWithVariableSpringConstantsForce<2>* p_linear_force;

            // Restore from the archive
            input_arch >> p_linear_force;

            // Test the member data
            TS_ASSERT_EQUALS(p_linear_force->mUseCutoffPoint, true);
            TS_ASSERT_EQUALS(CellBasedConfig::Instance()->GetMechanicsCutOffLength(), 1.1);
            TS_ASSERT_EQUALS(p_linear_force->mUseEdgeBasedSpringConstant, true);
            TS_ASSERT_EQUALS(p_linear_force->mUseEdgeBasedSpringConstant, true);
            TS_ASSERT_EQUALS(p_linear_force->mUseMutantSprings, true);
            TS_ASSERT_EQUALS(p_linear_force->mUseBCatSprings, true);
            TS_ASSERT_EQUALS(p_linear_force->mUseApoptoticSprings, true);
            TS_ASSERT_DELTA(p_linear_force->mMutantMutantMultiplier, 0.2, 1e-12);
            TS_ASSERT_DELTA(p_linear_force->mNormalMutantMultiplier, 0.3, 1e-12);
            TS_ASSERT_DELTA(p_linear_force->mBetaCatSpringScaler, 20/6.0, 1e-12);
            TS_ASSERT_DELTA(p_linear_force->mApoptoticSpringTensionStiffness, 4.0, 1e-12);
            TS_ASSERT_DELTA(p_linear_force->mApoptoticSpringCompressionStiffness, 5.0, 1e-12);

            delete p_linear_force;
        }
    }
};

#endif /*TESTFORCES_HPP_*/

