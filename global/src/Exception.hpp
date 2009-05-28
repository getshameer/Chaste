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


#ifndef _EXCEPTION_HPP_
#define _EXCEPTION_HPP_

#include <ostream>
#include <string>
#include <sstream>

#include <cfloat>
#include <climits> //For UINT_MAX etc., necessary in gcc-4.3
#include <cstdlib> //For system() etc., necessary in gcc-4.3
const unsigned UNSIGNED_UNSET=UINT_MAX;
const int INT_UNSET=INT_MAX;
const double DOUBLE_UNSET=DBL_MAX;
#include "Debug.hpp"
/**
 * Exception class.
 * All exceptions thrown by this code are currently instances of this class.
 *
 * \todo Might we want this class to inherit from STL exceptions?
 */
class Exception
{
private:
    std::string mMessage; /**< Exception message */

public:
    /**
     * Construct an exception with a message string.
     *
     * @param message  the message
     * @param filename  which source file threw the exception
     * @param rLineNumber  which line number of the source file threw the exception
     */
    Exception(std::string message, std::string filename, const unsigned rLineNumber);

    /** Get the message associated with the exception
     *
     * @return The message set when the exception was thrown.
     **/
    std::string GetMessage() const;
};

#define EXCEPTION(message) throw Exception(message, __FILE__, __LINE__)

#define NEVER_REACHED EXCEPTION("Should have been impossible to reach this line of code")

// This is to cope with NDEBUG causing variables to not be used, since they are only
// used in assert()s
#ifdef NDEBUG
#define UNUSED_OPT(var) var=var
#else
#define UNUSED_OPT(var)
#endif

// This macro is handy for calling functions like system which return non-zero on error
#define EXPECT0(cmd, arg) { \
    std::string _arg = (arg); \
    int ret = cmd(_arg.c_str()); \
    if (ret != 0) { \
        EXCEPTION("Failed to execute command: " #cmd "(" + _arg + ")"); \
    } }
// Or if you don't care about errors for some reason...
#define IGNORE_RET(cmd, arg) { \
    std::string _arg = (arg); \
    int ret = cmd(_arg.c_str()); \
    ret = ret; \
    }

#endif // _EXCEPTION_HPP_
