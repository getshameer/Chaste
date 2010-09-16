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

#include "AbstractCellProperty.hpp"
#include "Exception.hpp"

#include <typeinfo>

#include <boost/serialization/extended_type_info.hpp>
#include <boost/serialization/extended_type_info_typeid.hpp>
#include <boost/serialization/extended_type_info_no_rtti.hpp>
#include <boost/serialization/type_info_implementation.hpp>

AbstractCellProperty::AbstractCellProperty()
    : mCellCount(0)
{
}

AbstractCellProperty::~AbstractCellProperty()
{
}

bool AbstractCellProperty::IsSame(const AbstractCellProperty* pOther) const
{
    const std::type_info& r_our_info = typeid(*this);
    const std::type_info& r_their_info = typeid(*pOther);
    return r_our_info == r_their_info;
}

bool AbstractCellProperty::IsSame(boost::shared_ptr<const AbstractCellProperty> pOther) const
{
    return IsSame(pOther.get());
}

void AbstractCellProperty::IncrementCellCount()
{
    mCellCount++;
}

void AbstractCellProperty::DecrementCellCount()
{
    if (mCellCount == 0)
    {
        EXCEPTION("Cannot decrement cell count: no cells have this cell property");
    }
    mCellCount--;
}

unsigned AbstractCellProperty::GetCellCount() const
{
    return mCellCount;
}

std::string AbstractCellProperty::GetIdentifier() const
{
#if BOOST_VERSION >= 103700
    return boost::serialization::type_info_implementation<AbstractCellProperty>::type::get_const_instance().get_derived_extended_type_info(*this)->get_key();
#else
    return boost::serialization::type_info_implementation<AbstractCellProperty>::type::get_derived_extended_type_info(*this)->get_key();
#endif
}