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

#include "CryptSimulation2d.hpp"
#include "WntConcentration.hpp"
#include "VanLeeuwen2009WntSwatCellCycleModelHypothesisOne.hpp"
#include "VanLeeuwen2009WntSwatCellCycleModelHypothesisTwo.hpp"

CryptSimulation2d::CryptSimulation2d(AbstractCellPopulation<2>& rCellPopulation,
                                     bool deleteCellPopulationAndForceCollection,
                                     bool initialiseCells)
    : CellBasedSimulation<2>(rCellPopulation,
                             deleteCellPopulationAndForceCollection,
                             initialiseCells),
      mUseJiggledBottomCells(false),
      mWriteBetaCatenin(false)
{
    mpStaticCastCellPopulation = static_cast<MeshBasedCellPopulationWithGhostNodes<2>*>(&mrCellPopulation);

    /*
     * To check if beta-catenin results will be written to file, we test if the first
     * cell has a cell cycle model that is a subclass of AbstractVanLeeuwen2009WntSwatCellCycleModel.
     * In doing so, we assume that all cells in the simulation have the same cell cycle
     * model.
     */
    if (dynamic_cast<AbstractVanLeeuwen2009WntSwatCellCycleModel*>(mrCellPopulation.Begin()->GetCellCycleModel()))
    {
        mWriteBetaCatenin = true;
    }
}

c_vector<double, 2> CryptSimulation2d::CalculateCellDivisionVector(CellPtr pParentCell)
{
    // Location of parent and daughter cells
    c_vector<double, 2> parent_coords = mrCellPopulation.GetLocationOfCellCentre(pParentCell);
    c_vector<double, 2> daughter_coords;

    // Get separation parameter
    double separation = mpStaticCastCellPopulation->GetMeinekeDivisionSeparation();

    // Make a random direction vector of the required length
    c_vector<double, 2> random_vector;

    /*
     * Pick a random direction and move the parent cell backwards by 0.5*separation
     * in that direction and return the position of the daughter cell 0.5*separation
     * forwards in that direction.
     */

    double random_angle = RandomNumberGenerator::Instance()->ranf();
    random_angle *= 2.0*M_PI;

    random_vector(0) = 0.5*separation*cos(random_angle);
    random_vector(1) = 0.5*separation*sin(random_angle);

    c_vector<double, 2> proposed_new_parent_coords = parent_coords - random_vector;
    c_vector<double, 2> proposed_new_daughter_coords = parent_coords + random_vector;

    if ((proposed_new_parent_coords(1) >= 0.0) && (proposed_new_daughter_coords(1) >= 0.0))
    {
        // We are not too close to the bottom of the cell population, so move parent
        parent_coords = proposed_new_parent_coords;
        daughter_coords = proposed_new_daughter_coords;
    }
    else
    {
        proposed_new_daughter_coords = parent_coords + 2.0*random_vector;
        while (proposed_new_daughter_coords(1) < 0.0)
        {
            random_angle = RandomNumberGenerator::Instance()->ranf();
            random_angle *= 2.0*M_PI;

            random_vector(0) = separation*cos(random_angle);
            random_vector(1) = separation*sin(random_angle);
            proposed_new_daughter_coords = parent_coords + random_vector;
        }
        daughter_coords = proposed_new_daughter_coords;
    }

    assert(daughter_coords(1) >= 0.0); // to make sure dividing cells stay in the cell population
    assert(parent_coords(1) >= 0.0);   // to make sure dividing cells stay in the cell population

    // Set the parent to use this location
    ChastePoint<2> parent_coords_point(parent_coords);

    unsigned node_index = mrCellPopulation.GetLocationIndexUsingCell(pParentCell);
    mrCellPopulation.SetNode(node_index, parent_coords_point);

    return daughter_coords;
}

void CryptSimulation2d::WriteVisualizerSetupFile()
{
    *mpVizSetupFile << "MeshWidth\t" << mrCellPopulation.GetWidth(0) << "\n";
}

void CryptSimulation2d::SetupWriteBetaCatenin()
{
    OutputFileHandler output_file_handler(this->mSimulationOutputDirectory + "/", false);
    mVizBetaCateninResultsFile = output_file_handler.OpenOutputFile("results.vizbetacatenin");
    *mpVizSetupFile << "BetaCatenin\n";
}

void CryptSimulation2d::WriteBetaCatenin(double time)
{
    *mVizBetaCateninResultsFile <<  time << "\t";

    unsigned global_index;
    double x;
    double y;
    double b_cat_membrane;
    double b_cat_cytoplasm;
    double b_cat_nuclear;

    for (AbstractCellPopulation<2>::Iterator cell_iter = mrCellPopulation.Begin();
         cell_iter != mrCellPopulation.End();
         ++cell_iter)
    {
        global_index = mrCellPopulation.GetLocationIndexUsingCell(*cell_iter);
        x = mrCellPopulation.GetLocationOfCellCentre(*cell_iter)[0];
        y = mrCellPopulation.GetLocationOfCellCentre(*cell_iter)[1];

        // We should only be calling this code block if mWriteBetaCatenin has been set to true in the constructor
        assert(mWriteBetaCatenin);

        AbstractVanLeeuwen2009WntSwatCellCycleModel* p_model = dynamic_cast<AbstractVanLeeuwen2009WntSwatCellCycleModel*>(cell_iter->GetCellCycleModel());
        b_cat_membrane = p_model->GetMembraneBoundBetaCateninLevel();
        b_cat_cytoplasm = p_model->GetCytoplasmicBetaCateninLevel();
        b_cat_nuclear = p_model->GetNuclearBetaCateninLevel();

        *mVizBetaCateninResultsFile << global_index << " " << x << " " << y << " " << b_cat_membrane << " " << b_cat_cytoplasm << " " << b_cat_nuclear << " ";
    }

    *mVizBetaCateninResultsFile << "\n";
}

void CryptSimulation2d::SetupSolve()
{
    /*
     * If there are any cells in the simulation, and mWriteBetaCatenin has been set
     * to true in the constructor, then set up the beta-catenin results file and
     * write the initial conditions to file.
     */
    bool any_cells_present = (mrCellPopulation.Begin() != mrCellPopulation.End());
    if (any_cells_present && mWriteBetaCatenin)
    {
        SetupWriteBetaCatenin();
        double current_time = SimulationTime::Instance()->GetTime();
        WriteBetaCatenin(current_time);
    }
}

void CryptSimulation2d::PostSolve()
{
    SimulationTime* p_time = SimulationTime::Instance();

    if ((p_time->GetTimeStepsElapsed()+1)%mSamplingTimestepMultiple==0)
    {
        /*
         * If there are any cells in the simulation, and mWriteBetaCatenin has been set
         * to true in the constructor, then set up the beta-catenin results file and
         * write the initial conditions to file.
         */
        bool any_cells_present = (mrCellPopulation.Begin() != mrCellPopulation.End());
        if (any_cells_present && mWriteBetaCatenin)
        {
            double time_next_step = p_time->GetTime() + p_time->GetTimeStep();
            WriteBetaCatenin(time_next_step);
        }
    }
}

void CryptSimulation2d::AfterSolve()
{
    /*
     * If there are any cells in the simulation, and mWriteBetaCatenin has been set
     * to true in the constructor, then close the beta-catenin results file.
     */
    bool any_cells_present = (mrCellPopulation.Begin() != mrCellPopulation.End());
    if (any_cells_present && mWriteBetaCatenin)
    {
        mVizBetaCateninResultsFile->close();
    }
}

void CryptSimulation2d::UseJiggledBottomCells()
{
    mUseJiggledBottomCells = true;
}

void CryptSimulation2d::ApplyCellPopulationBoundaryConditions(const std::vector< c_vector<double, 2> >& rOldLocations)
{
    bool is_wnt_included = WntConcentration<2>::Instance()->IsWntSetUp();
    if (!is_wnt_included)
    {
        WntConcentration<2>::Destroy();
    }

    // Iterate over all nodes associated with real cells to update their positions
    // according to any cell population boundary conditions
    for (AbstractCellPopulation<2>::Iterator cell_iter = mrCellPopulation.Begin();
         cell_iter != mrCellPopulation.End();
         ++cell_iter)
    {
        // Get index of node associated with cell
        unsigned node_index = mrCellPopulation.GetLocationIndexUsingCell(*cell_iter);

        // Get pointer to this node
        Node<2>* p_node = mrCellPopulation.GetNode(node_index);

        if (!is_wnt_included)
        {
            /**
             * If WntConcentration is not set up then stem cells must be pinned,
             * so we reset the location of each stem cell.
             */
            if (cell_iter->GetCellCycleModel()->GetCellProliferativeType()==STEM)
            {
                // Get old node location
                c_vector<double, 2> old_node_location = rOldLocations[node_index];

                // Return node to old location
                p_node->rGetModifiableLocation()[0] = old_node_location[0];
                p_node->rGetModifiableLocation()[1] = old_node_location[1];
            }
        }

        // Any cell that has moved below the bottom of the crypt must be moved back up
        if (p_node->rGetLocation()[1] < 0.0)
        {
            p_node->rGetModifiableLocation()[1] = 0.0;
            if (mUseJiggledBottomCells)
            {
               /*
                * Here we give the cell a push upwards so that it doesn't
                * get stuck on the bottom of the crypt (as per #422).
                *
                * Note that all stem cells may get moved to the same height, so
                * we use a random perturbation to help ensure we are not simply
                * faced with the same problem at a different height!
                */
                p_node->rGetModifiableLocation()[1] = 0.05*mpRandomGenerator->ranf();
            }
        }
        assert(p_node->rGetLocation()[1] >= 0.0);
    }
}

void CryptSimulation2d::SetBottomCellAncestors()
{
    unsigned index = 0;
    for (AbstractCellPopulation<2>::Iterator cell_iter = mrCellPopulation.Begin();
         cell_iter != mrCellPopulation.End();
         ++cell_iter)
    {
        if (mrCellPopulation.GetLocationOfCellCentre(*cell_iter)[1] < 0.5)
        {
            cell_iter->SetAncestor(index++);
        }
    }
}

void CryptSimulation2d::OutputSimulationParameters(out_stream& rParamsFile)
{
    *rParamsFile << "\t\t<CryptCircumference>"<< mrCellPopulation.GetWidth(0) << "</CryptCircumference>\n";
	*rParamsFile << "\t\t<UseJiggledBottomCells>"<< mUseJiggledBottomCells << "</UseJiggledBottomCells>\n";

	// Call method on direct parent class
	CellBasedSimulation<2>::OutputSimulationParameters(rParamsFile);
}

// Serialization for Boost >= 1.36
#include "SerializationExportWrapperForCpp.hpp"
CHASTE_CLASS_EXPORT(CryptSimulation2d)