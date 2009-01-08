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
#ifndef HONEYCOMBMESHGENERATOR_HPP_
#define HONEYCOMBMESHGENERATOR_HPP_

#include <cmath>
#include <vector>

#include "Cylindrical2dMesh.hpp"
#include "TrianglesMeshReader.hpp"
#include "OutputFileHandler.hpp"
#include "CancerParameters.hpp"

#include "PetscTools.hpp"


/**
 *  Generator of honeycomb meshes, used as starting points for many simualtions.
 *
 *  This class takes in options such as width, height, number of ghost nodes
 *  and generates a honeycomb mesh (with equal distance between nodes), and ghost
 *  node information when requested.
 *
 *  NOTE: the user should delete the mesh after use to manage memory.
 */
class HoneycombMeshGenerator
{
private:
    /** A pointer to the mesh this class creates */
    MutableMesh<2,2>* mpMesh;
    /** The indices of the nodes in this mesh which are 'ghost nodes'  */
    std::set<unsigned> mGhostNodeIndices;
    /** The mesh is generated by writing out a series of nodes and reading them in from this file*/
    std::string mMeshFilename;
    /** The (x) width of the crypt to be constructed*/
    double mCryptWidth;
    /** The (y) depth of the crypt to be constructed*/
    double mCryptDepth;
    /** The y coordinate of the bottom row of cells (ghosts if requested) */
    double mBottom;
    /** The y coordinate of the top row of cells (ghosts if requested) */
    double mTop;
    /** The number of columns of cells to put across the x coordinate of the mesh */
    unsigned mNumCellWidth;
    /** The number of rows of cells to put up the y coordinate of the mesh */
    unsigned mNumCellLength;
    /** Whether we are creating a cylindrical mesh or not */
    bool mCylindrical;

    /**
     * Periodic honeycomb mesh maker
     * @param width  The periodic length scale to base this mesh around
     * @param ghosts  The number of rows of ghost nodes to add on at the top and bottom
     */
    void Make2dPeriodicCryptMesh(double width, unsigned ghosts);

public:

    /**
     * Crypt Periodic Honeycomb Mesh Generator
     *
     * Overwritten constructor for a mesh so mesh can be compressed by changing crypt width
     *
     * @param numNodesAlongWidth  The number of stem cells you want,
     * @param numNodesAlongLength  The number of cells you want along crypt axis,
     * @param ghosts  The thickness of ghost nodes to put around the edge (defaults to 3),
     * @param cylindrical  Whether the mesh should be cylindrically periodic (defaults to true),
     * @param scaleFactor  The scale factor for the width (circumference) of the cells.
     *
     * Note: this class creates a cancer params instance and sets the crypt width and length
     * accordingly in the parameters class to be used elsewhere.
     */
    HoneycombMeshGenerator(unsigned numNodesAlongWidth, unsigned numNodesAlongLength, unsigned ghosts=3, bool cylindrical=true, double scaleFactor=1.0);

    /**
     * Destructor - deletes the mesh object and pointer
     */
    ~HoneycombMeshGenerator();

    /**
     * @return a honeycomb mesh based on a 2D plane.
     */
    MutableMesh<2,2>* GetMesh();

    /**
     * @return a honeycomb mesh with cylindrical boundaries.
     */
    Cylindrical2dMesh* GetCylindricalMesh();

    /**
     * Returns the indices of the nodes in the mesh which are ghost nodes. This
     * information needs to be passed in when constructing a Tissue class.
     * @return indices of nodes
     */
    std::set<unsigned> GetGhostNodeIndices();

    /**
     * @return  a honeycomb mesh constructed to be roughly circular.
     */
    MutableMesh<2,2>* GetCircularMesh(double radius);

};
#endif /*HONEYCOMBMESHGENERATOR_HPP_*/
