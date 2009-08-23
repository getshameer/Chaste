/*

Copyright (C) University of Oxford, 2005-2009

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
#include "TysonNovakCellCycleModel.hpp"



#ifdef CHASTE_CVODE
CvodeAdaptor TysonNovakCellCycleModel::msSolver;
#else
BackwardEulerIvpOdeSolver TysonNovakCellCycleModel::msSolver(6);
#endif //CHASTE_CVODE


TysonNovakCellCycleModel::TysonNovakCellCycleModel()
{
    mpOdeSystem = new TysonNovak2001OdeSystem;
    mpOdeSystem->SetStateVariables(mpOdeSystem->GetInitialConditions());
#ifdef CHASTE_CVODE
    msSolver.CheckForStoppingEvents();
    msSolver.SetMaxSteps(10000);
    //msSolver.SetTolerances(1e-6, 1e-8);
#endif //CHASTE_CVODE
}

TysonNovakCellCycleModel::TysonNovakCellCycleModel(const TysonNovakCellCycleModel& rOtherModel)
    : AbstractOdeBasedCellCycleModel(rOtherModel)
{
    if (rOtherModel.mpOdeSystem != NULL)
    {
        mpOdeSystem = new TysonNovak2001OdeSystem(*static_cast<TysonNovak2001OdeSystem*>(rOtherModel.mpOdeSystem));
    }
}


void TysonNovakCellCycleModel::ResetForDivision()
{
    AbstractOdeBasedCellCycleModel::ResetForDivision();

    assert(mpOdeSystem!=NULL);

    /**
     * This model needs the protein concentrations and phase resetting to G0/G1.
     *
     * In theory, the solution to the Tyson-Novak equations should exhibit stable
     * oscillations, and we only need to halve the mass of the cell each period.
     *
     * However, the backward Euler solver used to solve the equations
     * currently returns a solution that diverges after long times, so
     * we must reset the initial conditions each period.
     *
     * When running with CVODE however we can use the halving the mass of the cell method.
     */
#ifdef CHASTE_CVODE
    mpOdeSystem->rGetStateVariables()[5] = 0.5*mpOdeSystem->rGetStateVariables()[5];
#else
    mpOdeSystem->SetStateVariables(mpOdeSystem->GetInitialConditions());
#endif //CHASTE_CVODE
}


void TysonNovakCellCycleModel::InitialiseDaughterCell()
{
    if (mpCell->GetCellType() == STEM)
    {
        mpCell->SetCellType(TRANSIT);
    }
}

AbstractCellCycleModel* TysonNovakCellCycleModel::CreateCellCycleModel()
{
    return new TysonNovakCellCycleModel(*this);
}


bool TysonNovakCellCycleModel::SolveOdeToTime(double currentTime)
{
    double dt = 0.1/60.0;

    msSolver.SolveAndUpdateStateVariable(mpOdeSystem,mLastTime,currentTime,dt);

    return msSolver.StoppingEventOccurred();
}


double TysonNovakCellCycleModel::GetOdeStopTime()
{
    assert(msSolver.StoppingEventOccurred());
    return msSolver.GetStoppingTime();
}


double TysonNovakCellCycleModel::GetSDuration()
{
    /**
     * Tyson & Novak pretends it is running ODEs in just G1,
     * but they really represent the whole cell cycle, so
     * we set the other phases to zero.
     */
    return 0.0;
}


double TysonNovakCellCycleModel::GetG2Duration()
{
    /**
     * Tyson & Novak pretends it is running ODEs in just G1,
     * but they really represent the whole cell cycle so
     * we set the other phases to zero.
     */
    return 0.0;
}


double TysonNovakCellCycleModel::GetMDuration()
{
    /**
     * Tyson & Novak pretends it is running ODEs in just G1,
     * but they really represent the whole cell cycle so
     * we set the other phases to zero.
     */
    return 0.0;
}
