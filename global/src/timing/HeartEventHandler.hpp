/*

Copyright (C) University of Oxford, 2005-2011

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

#ifndef HEARTEVENTHANDLER_HPP_
#define HEARTEVENTHANDLER_HPP_

#include "GenericEventHandler.hpp"

/**
 * An event handler class with event types suitable for cardiac electrophysiological
 * simulations.
 *
 * It also contains events suitable to most generic PDE solvers too.
 */
class HeartEventHandler : public GenericEventHandler<16, HeartEventHandler>
{
public:

    /** Character array holding heart event names. There are eleven heart events. */
    static const char* EventName[16];

    /** Definition of heart event types. */
    typedef enum
    {
        READ_MESH=0,
        INITIALISE,
        ASSEMBLE_SYSTEM,
        SOLVE_ODES,
        COMMUNICATION,
        ASSEMBLE_RHS,
        NEUMANN_BCS,
        DIRICHLET_BCS,
        SOLVE_LINEAR_SYSTEM,
        WRITE_OUTPUT,
        DATA_CONVERSION,
        POST_PROC,
        USER1,
        USER2,
        USER3,
        EVERYTHING
    } EventType;
};

#endif /*HEARTEVENTHANDLER_HPP_*/