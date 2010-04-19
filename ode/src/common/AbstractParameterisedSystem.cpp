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

#include <cassert>

#include "AbstractParameterisedSystem.hpp"

#include "Exception.hpp"

#ifdef CHASTE_CVODE
// CVODE headers
#include <nvector/nvector_serial.h>
#endif

/**
 * Helper function to get a vector component.
 * 
 * This isn't a member so that we can specialise it without having to
 * specialise the whole class.
 * 
 * @param rVec  the vector to access
 * @param index  the index of the component to get
 */
template<typename VECTOR>
inline double GetVectorComponent(const VECTOR& rVec, unsigned index);

/**
 * Helper function to set a vector component.
 * 
 * This isn't a member so that we can specialise it without having to
 * specialise the whole class.
 * 
 * @param rVec  the vector to modify
 * @param index  the index of the component to set
 * @param value  the new value
 */
template<typename VECTOR>
inline void SetVectorComponent(VECTOR& rVec, unsigned index, double value);

/**
 * Helper function to determine a vector's size.
 * 
 * This isn't a member so that we can specialise it without having to
 * specialise the whole class.
 * 
 * @param rVec  the vector
 * @return  its size
 */
template<typename VECTOR>
inline double GetVectorSize(const VECTOR& rVec);

/**
 * Helper function to initialise a vector to be empty/unset.
 * 
 * This isn't a member so that we can specialise it without having to
 * specialise the whole class.
 * 
 * @param rVec  the vector
 */
template<typename VECTOR>
inline void InitialiseEmptyVector(VECTOR& rVec);

//
// Specialisations for std::vector<double>
//

template<>
inline double GetVectorComponent(const std::vector<double>& rVec, unsigned index)
{
    assert(index < rVec.size());
    return rVec[index];
}

template<>
inline void SetVectorComponent(std::vector<double>& rVec, unsigned index, double value)
{
    assert(index < rVec.size());
    rVec[index] = value;
}

template<>
inline double GetVectorSize(const std::vector<double>& rVec)
{
    return rVec.size();
}

template<>
inline void InitialiseEmptyVector(std::vector<double>& rVec)
{
}

//
// Specialisations for N_Vector
//

#ifdef CHASTE_CVODE
template<>
inline double GetVectorComponent(const N_Vector& rVec, unsigned index)
{
    assert(rVec != NULL);
    return NV_Ith_S(rVec, index);
}

template<>
inline void SetVectorComponent(N_Vector& rVec, unsigned index, double value)
{
    assert(rVec != NULL);
    NV_Ith_S(rVec, index) = value;
}

template<>
inline double GetVectorSize(const N_Vector& rVec)
{
    assert(rVec != NULL);
    return NV_LENGTH_S(rVec);
}

template<>
inline void InitialiseEmptyVector(N_Vector& rVec)
{
    rVec = NULL;
}
#endif // CHASTE_CVODE

//
// End of helper functions
//

template<typename VECTOR>
AbstractParameterisedSystem<VECTOR>::AbstractParameterisedSystem(unsigned numberOfStateVariables)
    : mNumberOfStateVariables(numberOfStateVariables)
{
    InitialiseEmptyVector(mParameters);
    InitialiseEmptyVector(mStateVariables);
}

template<typename VECTOR>
boost::shared_ptr<const AbstractOdeSystemInformation> AbstractParameterisedSystem<VECTOR>::GetSystemInformation() const
{
    assert(mpSystemInfo);
    return mpSystemInfo;
}

//
// State variable methods
//

template<typename VECTOR>
unsigned AbstractParameterisedSystem<VECTOR>::GetNumberOfStateVariables() const
{
    return mNumberOfStateVariables;
}

template<typename VECTOR>
double AbstractParameterisedSystem<VECTOR>::GetStateVariable(unsigned index) const
{
    if (index >= mNumberOfStateVariables)
    {
        EXCEPTION("The index passed in must be less than the number of state variables.");
    }
    return GetVectorComponent(mStateVariables, index);
}

template<typename VECTOR>
void AbstractParameterisedSystem<VECTOR>::SetStateVariable(unsigned index, double newValue)
{
    if ( mNumberOfStateVariables <= index )
    {
        EXCEPTION("The index passed in must be less than the number of state variables.");
    }
    SetVectorComponent(mStateVariables, index, newValue);
}

template<typename VECTOR>
const std::vector<std::string>& AbstractParameterisedSystem<VECTOR>::rGetStateVariableNames() const
{
    assert(mpSystemInfo);
    return mpSystemInfo->rGetStateVariableNames();
}

template<typename VECTOR>
const std::vector<std::string>& AbstractParameterisedSystem<VECTOR>::rGetStateVariableUnits() const
{
    assert(mpSystemInfo);
    return mpSystemInfo->rGetStateVariableUnits();
}

template<typename VECTOR>
unsigned AbstractParameterisedSystem<VECTOR>::GetStateVariableIndex(const std::string& rName) const
{
    assert(mpSystemInfo);
    return mpSystemInfo->GetStateVariableIndex(rName);
}

template<typename VECTOR>
std::string AbstractParameterisedSystem<VECTOR>::GetStateVariableUnits(unsigned index) const
{
    assert(mpSystemInfo);
    return mpSystemInfo->GetStateVariableUnits(index);
}

//
// Parameter methods
//

template<typename VECTOR>
unsigned AbstractParameterisedSystem<VECTOR>::GetNumberOfParameters() const
{
    return GetVectorSize(mParameters);
}

template<typename VECTOR>
double AbstractParameterisedSystem<VECTOR>::GetParameter(unsigned index) const
{
    if (index >= GetVectorSize(mParameters))
    {
        EXCEPTION("The index passed in must be less than the number of parameters.");
    }
    return GetVectorComponent(mParameters, index);
}

template<typename VECTOR>
void AbstractParameterisedSystem<VECTOR>::SetParameter(unsigned index, double value)
{
    if (index >= GetVectorSize(mParameters))
    {
        EXCEPTION("The index passed in must be less than the number of parameters.");
    }
    SetVectorComponent(mParameters, index, value);
}

template<typename VECTOR>
const std::vector<std::string>& AbstractParameterisedSystem<VECTOR>::rGetParameterNames() const
{
    assert(mpSystemInfo);
    return mpSystemInfo->rGetParameterNames();
}

template<typename VECTOR>
const std::vector<std::string>& AbstractParameterisedSystem<VECTOR>::rGetParameterUnits() const
{
    assert(mpSystemInfo);
    return mpSystemInfo->rGetParameterUnits();
}

template<typename VECTOR>
unsigned AbstractParameterisedSystem<VECTOR>::GetParameterIndex(const std::string& rName) const
{
    assert(mpSystemInfo);
    return mpSystemInfo->GetParameterIndex(rName);
}

template<typename VECTOR>
std::string AbstractParameterisedSystem<VECTOR>::GetParameterUnits(unsigned index) const
{
    assert(mpSystemInfo);
    return mpSystemInfo->GetParameterUnits(index);
}

//
// "Any variable" methods
//

template<typename VECTOR>
double AbstractParameterisedSystem<VECTOR>::GetAnyVariable(unsigned index) const
{
    if (index < mNumberOfStateVariables)
    {
        return GetVectorComponent(mStateVariables, index);
    }
    else if (index - mNumberOfStateVariables < GetVectorSize(mParameters))
    {
        return GetVectorComponent(mParameters, index - mNumberOfStateVariables);
    }
    else
    {
        EXCEPTION("Invalid index passed to GetAnyVariable.");
    }
}

template<typename VECTOR>
unsigned AbstractParameterisedSystem<VECTOR>::GetAnyVariableIndex(const std::string& rName) const
{
    assert(mpSystemInfo);
    return mpSystemInfo->GetAnyVariableIndex(rName);
}

template<typename VECTOR>
std::string AbstractParameterisedSystem<VECTOR>::GetAnyVariableUnits(unsigned index) const
{
    assert(mpSystemInfo);
    return mpSystemInfo->GetAnyVariableUnits(index);
}


////////////////////////////////////////////////////////////////////////////////////
// Explicit instantiation
////////////////////////////////////////////////////////////////////////////////////

template class AbstractParameterisedSystem<std::vector<double> >;
#ifdef CHASTE_CVODE
template class AbstractParameterisedSystem<N_Vector>;
#endif
