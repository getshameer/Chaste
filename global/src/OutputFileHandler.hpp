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

#ifndef OUTPUTFILEHANDLER_HPP_
#define OUTPUTFILEHANDLER_HPP_

#include <string>
#include <fstream>
#include <sstream>
#include <memory>


/** Type of our output streams; a managed pointer to an std::ofstream */
typedef std::auto_ptr<std::ofstream> out_stream;

/**
 * This file abstracts stuff that needs to be done when creating output files for tests.
 * It defines some helpful functions, so that things that are otherwise repeated in lots
 * of places are just done here.
 */
class OutputFileHandler
{
private:
    std::string mDirectory; ///< The directory to store output files in (always ends in "/")
    
    /**
     * Makes a little file called ".chaste" that signifies that OutputFileHandler created this
     * directory. We can't wipe a directory unless this is present.
     * @param  directory
     */
    static void CreateChasteSignatureOnCreatedDirectory(std::string directory);
    
public:
    /**
     * Create an OutputFileHandler that will create output files in the given directory.
     * The directory name should be relative to the place where Chaste test output is
     * stored.  If the user needs to know where this is, use the GetChasteTestOutputDirectory
     * method.
     *
     * Will check that the directory exists and create it if needed.
     *
     * @param rDirectory  the directory to put output files in.
     * @param cleanOutputDirectory  whether to remove any existing files in the output directory
     */
    OutputFileHandler(const std::string& rDirectory,
                      bool cleanOutputDirectory = true);

    /**
     *  Static method for getting the test output directory (the directory where
     *  chaste stores test out, which is the environment variable CHASTE_TESTOUTPUT
     *
     *  Static so an output file handler does not have to be created if the test output
     *  directory is wanted for, say, reading a file
     */
    static std::string GetChasteTestOutputDirectory();

    /**
     * Check that the desired output directory exists and is writable by us.
     * Create it if needed.
     * Return the full pathname of the output directory.
     *
     * The environment variable CHASTE_TEST_OUTPUT will be examined.  If it is set
     * and non-empty it is taken to be a directory where test output should be stored.
     * Otherwise the current directory is used.
     *
     * @param rDirectory  pathname of the output directory, relative to where Chaste
     *         output will be stored (user shouldn't care about this).
     * @return full pathname to the output directory
     */
    std::string GetOutputDirectoryFullPath(const std::string& rDirectory);
    
    /**
     * Return the full pathname to the directory this object will create files
     * in.
     */
    std::string GetOutputDirectoryFullPath();
    
    /**
     * Helper method to set up ArchiveLocationInfo.
     */
    void SetArchiveDirectory();

    /**
     * Open an output file in our directory, and check it was opened successfully.
     * Throws an Exception if not.
     *
     * @param rFileName  the name of the file to open, relative to the output directory.
     * @param mode  optionally, flags to use when opening the file (defaults are as for
     *         std::ofstream).
     * @return  a managed pointer to the opened file stream.
     */
    out_stream OpenOutputFile(const std::string& rFileName,
                              std::ios_base::openmode mode=std::ios::out | std::ios::trunc);


    /**
     * This just calls the other OpenOutputFile after concatenating the first three arguments
     * together to make the full filename. For example OpenOutputFile("results_", 3, ".dat")
     * creates results_3.dat. See documentation for
     * OpenOutputFile(std::string, std::ios_base::openmode).
     *
     * @param rFileName  the root name of the file to open
     * @param number  the number to append to the root name of the file
     * @param rFileFormat  the file format
     * @param mode  optionally, flags to use when opening the file (defaults are as for
     *         std::ofstream).
     */
    out_stream OpenOutputFile(const std::string& rFileName,
                              unsigned number,
                              const std::string& rFileFormat,
                              std::ios_base::openmode mode=std::ios::out | std::ios::trunc);
    
};

#endif /*OUTPUTFILEHANDLER_HPP_*/
