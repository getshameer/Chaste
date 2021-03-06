/*

Copyright (c) 2005-2014, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef TESTNODEBASEDCELLPOPULATIONPARALLELMETHODS_HPP_
#define TESTNODEBASEDCELLPOPULATIONPARALLELMETHODS_HPP_

#include <cxxtest/TestSuite.h>
#include "AbstractCellBasedTestSuite.hpp"

#include "NodeBasedCellPopulation.hpp"
#include "NodesOnlyMesh.hpp"
#include "CellsGenerator.hpp"
#include "FixedDurationGenerationBasedCellCycleModel.hpp"
#include "TrianglesMeshReader.hpp"
#include "BetaCateninOneHitCellMutationState.hpp"
#include "WildTypeCellMutationState.hpp"
#include "TransitCellProliferativeType.hpp"
#include "DifferentiatedCellProliferativeType.hpp"
#include "CellLabel.hpp"
#include "CellPropertyRegistry.hpp"
#include "SmartPointers.hpp"
#include "FileComparison.hpp"

#include "PetscSetupAndFinalize.hpp"

class TestNodeBasedCellPopulationParallelMethods : public AbstractCellBasedTestSuite
{
private:

    NodesOnlyMesh<3>* mpNodesOnlyMesh;

    NodeBasedCellPopulation<3>* mpNodeBasedCellPopulation;

    void setUp()
    {
        AbstractCellBasedTestSuite::setUp();

        std::vector<Node<3>* > nodes;
        for (unsigned i=0; i<PetscTools::GetNumProcs(); i++)
        {
            nodes.push_back(new Node<3>(0, false, 0.0, 0.0, 0.5+(double)i));
        }

        mpNodesOnlyMesh = new NodesOnlyMesh<3>;
        mpNodesOnlyMesh->ConstructNodesWithoutMesh(nodes, 1.0);

        std::vector<CellPtr> cells;
        CellsGenerator<FixedDurationGenerationBasedCellCycleModel, 3> cells_generator;
        cells_generator.GenerateBasic(cells, mpNodesOnlyMesh->GetNumNodes());

        mpNodeBasedCellPopulation = new NodeBasedCellPopulation<3>(*mpNodesOnlyMesh, cells);

        for (unsigned i=0; i<nodes.size(); i++)
        {
            delete nodes[i];
        }
    }

    void tearDown()
    {
        delete mpNodeBasedCellPopulation;
        delete mpNodesOnlyMesh;

        AbstractCellBasedTestSuite::tearDown();
    }
public:

    void TestGetCellAndNodePair() throw (Exception)
    {
        unsigned node_index = mpNodesOnlyMesh->GetNodeIteratorBegin()->GetIndex();

        std::pair<CellPtr, Node<3>* > pair = mpNodeBasedCellPopulation->GetCellNodePair(node_index);

        TS_ASSERT_EQUALS(pair.second->GetIndex(), node_index);

        CellPtr p_returned_cell = pair.first;
        TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->GetLocationIndexUsingCell(p_returned_cell), node_index);
    }

    void TestAddNodeAndCellsToSend() throw (Exception)
    {
        unsigned index_of_node_to_send = mpNodesOnlyMesh->GetNodeIteratorBegin()->GetIndex();
        mpNodeBasedCellPopulation->AddNodeAndCellToSendRight(index_of_node_to_send);
        mpNodeBasedCellPopulation->AddNodeAndCellToSendLeft(index_of_node_to_send);

        TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->mCellsToSendRight.size(), 1u);
        TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->mCellsToSendLeft.size(), 1u);

        unsigned node_right_index = (*mpNodeBasedCellPopulation->mCellsToSendRight.begin()).second->GetIndex();
        TS_ASSERT_EQUALS(node_right_index, index_of_node_to_send);

        unsigned node_left_index = (*mpNodeBasedCellPopulation->mCellsToSendLeft.begin()).second->GetIndex();
        TS_ASSERT_EQUALS(node_left_index, index_of_node_to_send);
    }

    void TestSendAndRecieveCells() throw (Exception)
    {
        unsigned index_of_node_to_send = mpNodesOnlyMesh->GetNodeIteratorBegin()->GetIndex();;
        mpNodeBasedCellPopulation->AddNodeAndCellToSendRight(index_of_node_to_send);
        mpNodeBasedCellPopulation->AddNodeAndCellToSendLeft(index_of_node_to_send);

        TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->mCellCommunicationTag, 123u);

        TS_ASSERT(!(mpNodeBasedCellPopulation->mpCellsRecvRight));
        TS_ASSERT(!(mpNodeBasedCellPopulation->mpCellsRecvLeft));

#if BOOST_VERSION < 103700
        TS_ASSERT_THROWS_THIS(mpNodeBasedCellPopulation->SendCellsToNeighbourProcesses(),
                              "Parallel cell-based Chaste requires Boost >= 1.37");
#else
        mpNodeBasedCellPopulation->SendCellsToNeighbourProcesses();

        if (!PetscTools::AmTopMost())
        {
            TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->mpCellsRecvRight->size(), 1u);

            unsigned index = (*mpNodeBasedCellPopulation->mpCellsRecvRight->begin()).second->GetIndex();
            TS_ASSERT_EQUALS(index, PetscTools::GetMyRank() + 1);
        }
        if (!PetscTools::AmMaster())
        {
            TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->mpCellsRecvLeft->size(), 1u);

            unsigned index = (*mpNodeBasedCellPopulation->mpCellsRecvLeft->begin()).second->GetIndex();
            TS_ASSERT_EQUALS(index, PetscTools::GetMyRank() - 1);
        }
#endif
    }

    void TestSendAndRecieveCellsNonBlocking() throw (Exception)
    {
        unsigned index_of_node_to_send = mpNodesOnlyMesh->GetNodeIteratorBegin()->GetIndex();;
        mpNodeBasedCellPopulation->AddNodeAndCellToSendRight(index_of_node_to_send);
        mpNodeBasedCellPopulation->AddNodeAndCellToSendLeft(index_of_node_to_send);

        TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->mCellCommunicationTag, 123u);

        TS_ASSERT(!(mpNodeBasedCellPopulation->mpCellsRecvRight));
        TS_ASSERT(!(mpNodeBasedCellPopulation->mpCellsRecvLeft));

#if BOOST_VERSION < 103700
        TS_ASSERT_THROWS_THIS(mpNodeBasedCellPopulation->SendCellsToNeighbourProcesses(),
                              "Parallel cell-based Chaste requires Boost >= 1.37");
#else
        mpNodeBasedCellPopulation->NonBlockingSendCellsToNeighbourProcesses();

        mpNodeBasedCellPopulation->GetReceivedCells();

        if (!PetscTools::AmTopMost())
        {
            TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->mpCellsRecvRight->size(), 1u);

            unsigned index = (*mpNodeBasedCellPopulation->mpCellsRecvRight->begin()).second->GetIndex();
            TS_ASSERT_EQUALS(index, PetscTools::GetMyRank() + 1);
        }
        if (!PetscTools::AmMaster())
        {
            TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->mpCellsRecvLeft->size(), 1u);

            unsigned index = (*mpNodeBasedCellPopulation->mpCellsRecvLeft->begin()).second->GetIndex();
            TS_ASSERT_EQUALS(index, PetscTools::GetMyRank() - 1);
        }
#endif
    }

    void TestUpdateCellProcessLocation() throw (Exception)
    {
        if (PetscTools::GetNumProcs() > 1)
        {
            if (PetscTools::AmMaster())
            {
                // Move node to the next location.
                c_vector<double, 3> new_location = zero_vector<double>(3);
                new_location[2] = 1.6;
                ChastePoint<3> point(new_location);
                mpNodesOnlyMesh->GetNode(0)->SetPoint(point);
            }
            if (PetscTools::GetMyRank() == 1)
            {
                // Move node to the next location.
                c_vector<double, 3> new_location = zero_vector<double>(3);
                new_location[2] = 0.5;
                ChastePoint<3> point(new_location);
                mpNodesOnlyMesh->GetNode(1)->SetPoint(point);
            }
#if BOOST_VERSION < 103700
        TS_ASSERT_THROWS_THIS(mpNodeBasedCellPopulation->SendCellsToNeighbourProcesses(),
                              "Parallel cell-based Chaste requires Boost >= 1.37");
#else
            mpNodeBasedCellPopulation->UpdateCellProcessLocation();

            if (PetscTools::AmMaster())
            {
                TS_ASSERT_EQUALS(mpNodesOnlyMesh->GetNumNodes(), 1u);
                TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->GetNumRealCells(), 1u);
            }
            if (PetscTools::GetMyRank() == 1)
            {
                TS_ASSERT_EQUALS(mpNodesOnlyMesh->GetNumNodes(), 1u);
                TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->GetNumRealCells(), 1u);

                AbstractMesh<3,3>::NodeIterator node_iter = mpNodesOnlyMesh->GetNodeIteratorBegin();
                TS_ASSERT_DELTA(node_iter->rGetLocation()[2], 1.6, 1e-4);
            }
#endif
        }
    }

    void TestRefreshHaloCells() throw (Exception)
    {
#if BOOST_VERSION < 103700
        TS_ASSERT_THROWS_THIS(mpNodeBasedCellPopulation->SendCellsToNeighbourProcesses(),
                              "Parallel cell-based Chaste requires Boost >= 1.37");
#else
        // Set up the halo boxes and nodes.
        mpNodeBasedCellPopulation->Update();

        // Send and receive halo nodes.
        mpNodeBasedCellPopulation->RefreshHaloCells();

        mpNodeBasedCellPopulation->AddReceivedHaloCells();

        if (!PetscTools::AmMaster() && !PetscTools::AmTopMost())
        {
           TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->mHaloCells.size(), 2u);
           TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->mHaloCellLocationMap[mpNodeBasedCellPopulation->mHaloCells[0]], PetscTools::GetMyRank() - 1);
           TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->mHaloCellLocationMap[mpNodeBasedCellPopulation->mHaloCells[1]], PetscTools::GetMyRank() + 1);
        }
        else if (!PetscTools::AmMaster() || !PetscTools::AmTopMost())
        {
           TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->mHaloCells.size(), 1u);
        }
#endif
    }

    void TestUpdateWithLoadBalanceDoesntThrow() throw (Exception)
    {
#if BOOST_VERSION < 103700
        TS_ASSERT_THROWS_THIS(mpNodeBasedCellPopulation->SendCellsToNeighbourProcesses(),
                              "Parallel cell-based Chaste requires Boost >= 1.37");
#else
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(10.0, 1);

        mpNodeBasedCellPopulation->SetLoadBalanceMesh(true);

        mpNodeBasedCellPopulation->SetLoadBalanceFrequency(50);

        TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->mLoadBalanceFrequency, 50u);

        TS_ASSERT_THROWS_NOTHING(mpNodeBasedCellPopulation->Update());
#endif
    }

    void TestGetCellUsingLocationIndexWithHaloCell() throw (Exception)
    {
        boost::shared_ptr<Node<3> > p_node(new Node<3>(10, false, 0.0, 0.0, 0.0));

        // Create a cell.
        MAKE_PTR(WildTypeCellMutationState, p_state);
        MAKE_PTR(TransitCellProliferativeType, p_type);
        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();

        CellPtr p_cell(new Cell(p_state, p_model));

        mpNodeBasedCellPopulation->AddHaloCell(p_cell, p_node);

        TS_ASSERT_THROWS_NOTHING(mpNodeBasedCellPopulation->GetCellUsingLocationIndex(10));
        TS_ASSERT_EQUALS(mpNodeBasedCellPopulation->GetCellUsingLocationIndex(10), p_cell);
    }
};

#endif /*TESTNODEBASEDCELLPOPULATIONPARALLELMETHODS_HPP_*/
