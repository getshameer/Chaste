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
#ifndef STOCHASTICCELLCYCLEMODELCELLSGENERATOR_HPP_
#define STOCHASTICCELLCYCLEMODELCELLSGENERATOR_HPP_

#include "AbstractCellsGenerator.hpp"
#include "StochasticCellCycleModel.hpp"

/**
 * A helper class for generating a vector of cells for a given mesh
 */
template<unsigned DIM>
class StochasticCellCycleModelCellsGenerator : public AbstractCellsGenerator<DIM>
{
public :

    AbstractCellCycleModel* CreateCellCycleModel();
    
    double GetTypicalTransitCellCycleTime();
    
    double GetTypicalStemCellCycleTime();
    
    void GenerateForCrypt(std::vector<TissueCell>& rCells,
                             TetrahedralMesh<2,2>& rMesh,
                             bool randomBirthTimes,
                             double y0 = 0.3,
                             double y1 = 2.0,
                             double y2 = 3.0,
                             double y3 = 4.0,
                             bool initialiseCells = false);

};

template<unsigned DIM>
AbstractCellCycleModel* StochasticCellCycleModelCellsGenerator<DIM>::CreateCellCycleModel()
{
    return new StochasticCellCycleModel();
}

template<unsigned DIM>
double StochasticCellCycleModelCellsGenerator<DIM>::GetTypicalTransitCellCycleTime()
{    
    return CancerParameters::Instance()->GetTransitCellG1Duration()
            + CancerParameters::Instance()->GetSG2MDuration();    
}

template<unsigned DIM>
double StochasticCellCycleModelCellsGenerator<DIM>::GetTypicalStemCellCycleTime()
{
    return CancerParameters::Instance()->GetStemCellG1Duration()
            + CancerParameters::Instance()->GetSG2MDuration();
}


template<unsigned DIM>
void StochasticCellCycleModelCellsGenerator<DIM>::GenerateForCrypt(std::vector<TissueCell>& rCells,
                                 TetrahedralMesh<2,2>& rMesh,
                                 bool randomBirthTimes,
                                 double y0,
                                 double y1,
                                 double y2,
                                 double y3,
                                 bool initialiseCells)
{
    assert(DIM==2);
    RandomNumberGenerator *p_random_num_gen = RandomNumberGenerator::Instance();
    unsigned num_cells = rMesh.GetNumNodes();

    AbstractCellCycleModel* p_cell_cycle_model = NULL;
    double typical_transit_cycle_time;
    double typical_stem_cycle_time;

    rCells.clear();
    rCells.reserve(num_cells);

    for (unsigned i=0; i<num_cells; i++)
    {
        CellType cell_type;
        unsigned generation;

        double y = rMesh.GetNode(i)->GetPoint().rGetLocation()[1];

        p_cell_cycle_model = CreateCellCycleModel();
        typical_transit_cycle_time = this->GetTypicalTransitCellCycleTime();
        typical_stem_cycle_time = GetTypicalStemCellCycleTime();
        
        double birth_time = 0.0;

        if (y <= y0)
        {
            cell_type = STEM;
            generation = 0;
            if (randomBirthTimes)
            {
                birth_time = -p_random_num_gen->ranf()*typical_stem_cycle_time; // hours
            }
        }
        else if (y < y1)
        {
            cell_type = TRANSIT;
            generation = 1;
            if (randomBirthTimes)
            {
                birth_time = -p_random_num_gen->ranf()*typical_transit_cycle_time; // hours
            }
        }
        else if (y < y2)
        {
            cell_type = TRANSIT;
            generation = 2;
            if (randomBirthTimes)
            {
                birth_time = -p_random_num_gen->ranf()*typical_transit_cycle_time; // hours
            }
        }
        else if (y < y3)
        {
            cell_type = TRANSIT;
            generation = 3;
            if (randomBirthTimes)
            {
                birth_time = -p_random_num_gen->ranf()*typical_transit_cycle_time; // hours
            }
        }
        else
        {
            if (randomBirthTimes)
            {
                birth_time = -p_random_num_gen->ranf()*typical_transit_cycle_time; // hours
            }
            cell_type = DIFFERENTIATED;
            generation = 4;
        }

        p_cell_cycle_model->SetGeneration(generation);
        TissueCell cell(cell_type, HEALTHY, p_cell_cycle_model);
        if (initialiseCells)
        {
            cell.InitialiseCellCycleModel();
        }

        cell.SetNodeIndex(i);
        cell.SetBirthTime(birth_time);
        rCells.push_back(cell);
    }
}

#endif /*STOCHASTICCELLCYCLEMODELCELLSGENERATOR_HPP_*/
