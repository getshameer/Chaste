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


#ifndef _ABSTRACTMESHREADER_HPP_
#define _ABSTRACTMESHREADER_HPP_

#include <vector>
#include <string>

/**
 * Helper structure that stores the nodes and any attribute value
 * associated with an Element.
 */
struct ElementData
{
    std::vector<unsigned> NodeIndices; /**< Vector of Node indices owned by the element. */
    unsigned AttributeValue; /**< Attribute value associated with the element. */
    unsigned ContainingElement; /**< Only applies to boundary elements: which element contains this boundary element. Only set if reader called with correct params */
};

/**
 * An abstract mesh reader class. Reads output generated by a mesh generator
 * and converts it to a standard format for use in constructing a finite
 * element mesh structure.
 *
 * A derived class TrianglesMeshReader exists for reading meshes generated
 * by Triangles (in 2-d) and TetGen (in 3-d).
 *
 * A derived class MemfemMeshReader reads 3D data from the Tulane University code
 *
 * A derived class FemlabMeshReader reads 2D data from Femlab or Matlab PDEToolbox
 */
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
class AbstractMeshReader
{

public:

    virtual ~AbstractMeshReader()
    {}

    /** Returns the number of elements in the mesh */
    virtual unsigned GetNumElements() const =0;

    /** Returns the number of nodes in the mesh */
    virtual unsigned GetNumNodes() const =0;

    /** Returns the number of faces in the mesh (also has synonym GetNumEdges()) */
    virtual unsigned GetNumFaces() const =0;
    
    /** Returns the number of cable elements in the mesh */
    virtual unsigned GetNumCableElements() const;

    /** Returns the number of element attributes in the mesh */
    virtual unsigned GetNumElementAttributes() const;

    /** Returns the number of face attributes in the mesh */
    virtual unsigned GetNumFaceAttributes() const;

    /** Returns the number of cable element attributes in the mesh */
    virtual unsigned GetNumCableElementAttributes() const;

    /**
     * @returns the vector of node attributes
     * Returns an empty vector here. Over-ride in child classes if needed.
     * Ideally, this method would be in AbstractCachedMeshReader (where it would return the cached attribuites)
     * but TrianglesMeshReader (the class this method was created for)
     * does not inherit from AbstractCachedMeshReader, so it needs to be here.
     */
    virtual std::vector<double> GetNodeAttributes();

    /** Returns the number of edges in the mesh (synonym of GetNumFaces()) */
    unsigned GetNumEdges() const;

    /** Returns a vector of the coordinates of each node in turn */
    virtual std::vector<double> GetNextNode()=0;

    /** Resets pointers to beginning*/
    virtual void Reset()=0;

    /** Returns a vector of the node indices of each element (and any attribute infomation, if there is any) in turn */
    virtual ElementData GetNextElementData()=0;

    /** Returns a vector of the node indices of each face (and any attribute/containment infomation, if there is any) in turn */
    virtual ElementData GetNextFaceData()=0;

    /** Returns a vector of the node indices of each cable element (and any attribute infomation, if there is any) in turn */
    virtual ElementData GetNextCableElementData();

    /** Returns a vector of the node indices of each edge (and any attribute/containment infomation, if there is any) in turn (synonym of GetNextFaceData()) */
    ElementData GetNextEdgeData();


    /**
     *  Normally throws an exception.  Only implemented for tetrahedral mesh reader of binary files.
     *
     * @param index  The global node index
     * @return a vector of the coordinates of the node
     */
     virtual std::vector<double> GetNode(unsigned index);

    /**
     *  Normally throws an exception.  Only implemented for tetrahedral mesh reader of binary files.
     *
     * @param index  The global element index
     * @return a vector of the node indices of the element (and any attribute infomation, if there is any)
     */
    virtual ElementData GetElementData(unsigned index);

    /**
     *  Normally throws an exception.  Only implemented for tetrahedral mesh reader of binary files.
     *
     * @param index  The global face index
     * @return a vector of the node indices of the face (and any attribute/containment infomation, if there is any)
     */
    virtual ElementData GetFaceData(unsigned index);

    /**
     *  Synonym of GetFaceData(index)
     *
     * @param index  The global edge index
     * @return a vector of the node indices of the edge (and any attribute/containment infomation, if there is any)
     */
    ElementData GetEdgeData(unsigned index);

    /**
     *  Normally throws an exception.  When implemented by derived classes, returns a list of the elements
     *  that contain the node (only available for binary files).
     *
     * @param index  The global node index
     * @return a vector of the node indices of the face (and any attribute/containment infomation, if there is any)
     */
    virtual std::vector<unsigned> GetContainingElementIndices(unsigned index);

    /**
     * Get the base name (less any extension) for mesh files.  Only implemented for some mesh types.
     */
    virtual std::string GetMeshFileBaseName();

    /**
     * Returns true if reading binary files, false if reading ascii files.
     *
     * Note, this will always return false unless over-ridden by a derived class that is able to support binary file
     * formats.
     */
    virtual bool IsFileFormatBinary();

    /**
     * Returns true if there is a node connectivity list (NCL) file available.
     *
     * Note, this will always return false unless over-ridden by a derived class that is able to support NCL files.
     *
     * @return whether there is a node connectivity list (NCL) file available
     */
    virtual bool HasNclFile();

};

#endif //_ABSTRACTMESHREADER_HPP_
