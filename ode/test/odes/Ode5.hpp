/*

Copyright (C) University of Oxford, 2005-2012

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


#ifndef ODE5_HPP_
#define ODE5_HPP_

#include "AbstractOdeSystem.hpp"
#include "OdeSystemInformation.hpp"

/**
 * dy/dt = 100 y (1-y), y(0) = 0.2.
 */
class Ode5 : public AbstractOdeSystem
{
public:
    Ode5() : AbstractOdeSystem(1)  // 1 here is the number of unknowns
    {
        mpSystemInfo = OdeSystemInformation<Ode5>::Instance();
    }

    void EvaluateYDerivatives(double time, const std::vector<double>& rY, std::vector<double>& rDY)
    {
        double alpha = 100;
        rDY[0] = alpha*rY[0]*(1-rY[0]);
    }
};

template<>
void OdeSystemInformation<Ode5>::Initialise()
{
    this->mVariableNames.push_back("Variable_1");
    this->mVariableUnits.push_back("dimensionless");
    this->mInitialConditions.push_back(0.2);

    this->mInitialised = true;
}

#endif /*ODE5_HPP_*/
