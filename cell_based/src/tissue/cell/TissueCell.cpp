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

#include "TissueCell.hpp"
#include "ApoptoticCellMutationState.hpp"

unsigned TissueCell::mMaxCellId = 0;

struct null_deleter
{
    void operator()(void const *) const
    {
    }
};

TissueCell::TissueCell(boost::shared_ptr<AbstractCellMutationState> pMutationState,
                       AbstractCellCycleModel* pCellCycleModel,
                       bool archiving)
    : mCanDivide(false),
      mpMutationState(pMutationState),
      mpCellCycleModel(pCellCycleModel),
      mAncestor(UNSIGNED_UNSET), // Has to be set by a SetAncestor() call (usually from Tissue)
      mDeathTime(DBL_MAX), // This has to be initialised for archiving,
      mStartOfApoptosisTime(DBL_MAX),
      mUndergoingApoptosis(false),
      mIsDead(false),
      mIsLogged(false)
{
    if (SimulationTime::Instance()->IsStartTimeSetUp()==false)
    {
        EXCEPTION("TissueCell is setting up a cell cycle model but SimulationTime has not been set up");
    }

    if (pCellCycleModel == NULL)
    {
        EXCEPTION("Cell cycle model is null");
    }

    mpCellCycleModel->SetCell(TissueCellPtr(this, null_deleter()));

    // Set Cell identifier & mutation state count
    mCellId = ++ mMaxCellId -1;
    if (!archiving)
    {
        mpMutationState->IncrementCellCount();
    }
}

TissueCell::~TissueCell()
{
    mpMutationState->DecrementCellCount();
    delete mpCellCycleModel;
}


void TissueCell::SetCellCycleModel(AbstractCellCycleModel* pCellCycleModel)
{
    if (mpCellCycleModel != pCellCycleModel)
    {
        delete mpCellCycleModel;
    }
    mpCellCycleModel = pCellCycleModel;
    mpCellCycleModel->SetCell(shared_from_this());
}


AbstractCellCycleModel* TissueCell::GetCellCycleModel() const
{
    return mpCellCycleModel;
}


void TissueCell::InitialiseCellCycleModel()
{
    mpCellCycleModel->Initialise();
}

double TissueCell::GetAge() const
{
    return mpCellCycleModel->GetAge();
}


double TissueCell::GetBirthTime() const
{
    return mpCellCycleModel->GetBirthTime();
}


void TissueCell::SetBirthTime(double birthTime)
{
    mpCellCycleModel->SetBirthTime(birthTime);
}


void TissueCell::SetMutationState(boost::shared_ptr<AbstractCellMutationState> pMutationState)
{
    mpMutationState->DecrementCellCount();
    mpMutationState = pMutationState;
    mpMutationState->IncrementCellCount();
}


boost::shared_ptr<AbstractCellMutationState> TissueCell::GetMutationState() const
{
    return mpMutationState;
}


void TissueCell::SetLogged()
{
    mIsLogged = true;
}


bool TissueCell::IsLogged()
{
    return mIsLogged;
}


void TissueCell::StartApoptosis(bool setDeathTime)
{
    assert(!IsDead());

    if (mUndergoingApoptosis)
    {
        EXCEPTION("StartApoptosis() called when already undergoing apoptosis");
    }
    mUndergoingApoptosis = true;
    mStartOfApoptosisTime = SimulationTime::Instance()->GetTime();
    if (setDeathTime)
    {
        mDeathTime = mStartOfApoptosisTime + TissueConfig::Instance()->GetApoptosisTime();
    }
    else
    {
        mDeathTime = DBL_MAX;
    }

    ///\todo Fix this usage of cell mutation state (see #1145, #1267 and #1285)
    boost::shared_ptr<AbstractCellMutationState> p_apoptotic_state(CellMutationStateRegistry::Instance()->Get<ApoptoticCellMutationState>());
    SetMutationState(p_apoptotic_state);
}


bool TissueCell::HasApoptosisBegun() const
{
    return mUndergoingApoptosis;
}

double TissueCell::GetStartOfApoptosisTime() const
{
    return mStartOfApoptosisTime;
}

double TissueCell::GetTimeUntilDeath() const
{
    if (!mUndergoingApoptosis || mDeathTime==DBL_MAX)
    {
        EXCEPTION("Shouldn't be checking time until apoptosis as it isn't set");
    }

    return mDeathTime - SimulationTime::Instance()->GetTime();
}


bool TissueCell::IsDead()
{
    if (mUndergoingApoptosis)
    {
        if (SimulationTime::Instance()->GetTime() >= mDeathTime)
        {
            this->Kill();
        }
    }
    return mIsDead;
}


void TissueCell::Kill()
{
    mIsDead = true;
}

void TissueCell::SetAncestor(unsigned ancestorIndex)
{
    mAncestor = ancestorIndex;
}

unsigned TissueCell::GetAncestor() const
{
    return mAncestor;
}

unsigned TissueCell::GetCellId() const
{
    return mCellId;
}

void TissueCell::ResetMaxCellId()
{
    mMaxCellId = 0;
}

bool TissueCell::ReadyToDivide()
{
    assert(!IsDead());
    if (mUndergoingApoptosis || mpMutationState->IsType<ApoptoticCellMutationState>())
    {
        return false;
    }

    mCanDivide = mpCellCycleModel->ReadyToDivide();

    return mCanDivide;
}


TissueCellPtr TissueCell::Divide()
{
    // Check we're allowed to divide
    assert(!IsDead());
    assert(mCanDivide);
    mCanDivide = false;

    // Reset properties of parent cell
    mpCellCycleModel->ResetForDivision();

    // Create daughter cell
    TissueCellPtr p_new_cell(new TissueCell(mpMutationState, mpCellCycleModel->CreateCellCycleModel()));

    // Initialise properties of daughter cell
    p_new_cell->GetCellCycleModel()->InitialiseDaughterCell();
    p_new_cell->SetAncestor(GetAncestor());

    return p_new_cell;
}
