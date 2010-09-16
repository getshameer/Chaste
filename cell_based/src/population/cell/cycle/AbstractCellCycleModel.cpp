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
#include "AbstractCellCycleModel.hpp"

AbstractCellCycleModel::AbstractCellCycleModel()
    : mBirthTime(SimulationTime::Instance()->GetTime()),
      mCurrentCellCyclePhase(M_PHASE),
      mG1Duration(DOUBLE_UNSET),
      mReadyToDivide(false),
      mDimension(0),
      mMinimumGapDuration(0.01) // an educated guess
{
}

AbstractCellCycleModel::~AbstractCellCycleModel()
{
}

void AbstractCellCycleModel::Initialise()
{
}

void AbstractCellCycleModel::InitialiseDaughterCell()
{
}

void AbstractCellCycleModel::SetCell(CellPtr pCell)
{
    mpCell = pCell;
}

CellPtr AbstractCellCycleModel::GetCell()
{
    assert(mpCell != NULL);
    return mpCell;
}

void AbstractCellCycleModel::SetBirthTime(double birthTime)
{
    mBirthTime = birthTime;
}

double AbstractCellCycleModel::GetBirthTime() const
{
    return mBirthTime;
}

double AbstractCellCycleModel::GetAge()
{
    return SimulationTime::Instance()->GetTime() - mBirthTime;
}

CellCyclePhase AbstractCellCycleModel::GetCurrentCellCyclePhase()
{
    return mCurrentCellCyclePhase;
}

void AbstractCellCycleModel::ResetForDivision()
{
    assert(mReadyToDivide);
    mCurrentCellCyclePhase = M_PHASE;
    mReadyToDivide = false;
}

double AbstractCellCycleModel::GetSDuration()
{
    return CellBasedConfig::Instance()->GetSDuration();
}

double AbstractCellCycleModel::GetG1Duration()
{
    return mG1Duration;
}

double AbstractCellCycleModel::GetG2Duration()
{
    return CellBasedConfig::Instance()->GetG2Duration();
}

double AbstractCellCycleModel::GetMDuration()
{
    return CellBasedConfig::Instance()->GetMDuration();
}

bool AbstractCellCycleModel::ReadyToDivide()
{
    assert(mpCell != NULL);

    if (!mReadyToDivide)
    {
        UpdateCellCyclePhase();
        if ( (mCurrentCellCyclePhase != G_ZERO_PHASE) &&
             (GetAge() >= GetMDuration() + GetG1Duration() + GetSDuration() + GetG2Duration()) )
        {
            mReadyToDivide = true;
        }
    }
    return mReadyToDivide;
}

void AbstractCellCycleModel::SetDimension(unsigned dimension)
{
    if (dimension != 1 && dimension !=2 && dimension != 3)
    {
        EXCEPTION("Dimension must be 1, 2 or 3");
    }
    mDimension = dimension;
}

unsigned AbstractCellCycleModel::GetDimension()
{
    return mDimension;
}

double AbstractCellCycleModel::GetAverageTransitCellCycleTime()
{
    return CellBasedConfig::Instance()->GetTransitCellG1Duration()
            + CellBasedConfig::Instance()->GetSG2MDuration();
}

double AbstractCellCycleModel::GetAverageStemCellCycleTime()
{
    return CellBasedConfig::Instance()->GetStemCellG1Duration()
            + CellBasedConfig::Instance()->GetSG2MDuration();
}

bool AbstractCellCycleModel::CanCellTerminallyDifferentiate()
{
    return true;
}

void AbstractCellCycleModel::SetCellProliferativeType(CellProliferativeType cellType)
{
    mCellProliferativeType = cellType;
}


CellProliferativeType AbstractCellCycleModel::GetCellProliferativeType() const
{
    return mCellProliferativeType;
}

void AbstractCellCycleModel::SetMinimumGapDuration(double minimumGapDuration)
{
    assert(minimumGapDuration > 0.0);
    mMinimumGapDuration = minimumGapDuration;
}

double AbstractCellCycleModel::GetMinimumGapDuration()
{
    return mMinimumGapDuration;
}