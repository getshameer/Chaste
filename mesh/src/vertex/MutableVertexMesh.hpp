/*

Copyright (c) 2005-2014, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#ifndef MUTABLEVERTEXMESH_HPP_
#define MUTABLEVERTEXMESH_HPP_

// Forward declaration prevents circular include chain
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
class VertexMeshWriter;

#include <iostream>
#include <map>
#include <algorithm>

#include "ChasteSerialization.hpp"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>

#include "VertexMesh.hpp"

/**
 * A mutable vertex-based mesh class, which inherits from VertexMesh and allows for local
 * remeshing. This is implemented through simple operations including node merging, neighbour
 * exchange ("T1 swap"), node/edge merging in the case of intersections ("T3 swap") and
 * removal of small triangular elements ("T2 swap").
 *
 * MutableVertexMesh is used as a member of the VertexBasedCellPopulation class to represent
 * the junctional network of cells that forms the basis of simulations of off-lattice
 * vertex-based models.
 */
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
class MutableVertexMesh : public VertexMesh<ELEMENT_DIM, SPACE_DIM>
{
    friend class TestMutableVertexMesh;
    friend class TestMutableVertexMeshReMesh;

protected:

    /** The minimum distance apart that two nodes in the mesh can be without causing element rearrangement. */
    double mCellRearrangementThreshold;

    /**
     * The ratio between the minimum distance apart that two nodes in the mesh can be without causing element
     * rearrangement and their separation after remeshing.
     */
    double mCellRearrangementRatio;

    /** The area threshold at which T2 swaps occur in an apoptotic, triangular cell/element. */
    double mT2Threshold;

    /** Whether to check for edges intersections (true) or not (false). */
    bool mCheckForInternalIntersections;

    /** Indices of nodes that have been deleted. These indices can be reused when adding new elements/nodes. */
    std::vector<unsigned> mDeletedNodeIndices;

    /** Indices of elements that have been deleted. These indices can be reused when adding new elements. */
    std::vector<unsigned> mDeletedElementIndices;

    /**
     * Locations of T1 swaps (the mid point of the moving nodes), stored so they can be accessed and output by the cell population.
     * The locations are stored until they are cleared by ClearLocationsOfT1Swaps().
     */
    std::vector< c_vector<double, SPACE_DIM> > mLocationsOfT1Swaps;

    /**
     * Locations of T2 swaps (the centre of the removed triangle), stored so they can be accessed and output by the cell population.
     * The locations are stored until they are cleared by ClearLocationsOfT2Swaps().
     */
    std::vector< c_vector<double, SPACE_DIM> > mLocationsOfT2Swaps;

    /**
     * Locations of T3 swaps (the location of the intersection with the edge), stored so they can be accessed and output by the cell population.
     * The locations are stored until they are cleared by ClearLocationsOfT3Swaps().
     */
    std::vector< c_vector<double, SPACE_DIM> > mLocationsOfT3Swaps;

    /**
     * Divide an element along the axis passing through two of its nodes.
     *
     * \todo This method currently assumes SPACE_DIM = 2 (see #866)
     *
     * @param pElement the element to divide
     * @param nodeAIndex the local index of one node within this element
     * @param nodeBIndex the local index of another node within this element
     * @param placeOriginalElementBelow whether to place the original element below (in the y direction) the new element (defaults to false)
     *
     * @return the index of the new element
     */
    unsigned DivideElement(VertexElement<ELEMENT_DIM,SPACE_DIM>* pElement,
                           unsigned nodeAIndex,
                           unsigned nodeBIndex,
                           bool placeOriginalElementBelow=false);

    /**
     * Helper method for ReMesh().
     *
     * Check if any neighbouring nodes in an element are closer than the mCellRearrangementThreshold
     * and are not contained in any triangular elements. If any such pair of nodes are found, then
     * call IdentifySwapType(), which in turn implements the appropriate local remeshing operation
     * (a T1, T2 or T3 swap or node merge).
     *
     * @param rElementMap a VertexElementMap which associates the indices of VertexElements in the old mesh
     *                   with indices of VertexElements in the new mesh.  This should be created
     *                   with the correct size, GetNumElements()
     *
     * @return whether we need to check for, and implement, any further local remeshing operations
     */
    bool CheckForT1Swaps(VertexElementMap& rElementMap);

    /**
     * Helper method for ReMesh().
     *
     * Check if any elements have become intersected and correct this by implementing the appropriate
     * local remeshing operation (a T3 swap or node merge).
     *
     * @return whether to recheck the mesh again
     */
    bool CheckForIntersections();

    /**
     * Helper method for ReMesh(), called by CheckForT1Swaps().
     *
     * Identify the type of local remeshing operation required (T1 swap, T3 swap or node merge) when
     * neighbouring nodes in an element have been found to be closer than the mCellRearrangementThreshold
     * and are not contained in any triangular elements.
     *
     * @param pNodeA one of the nodes to perform the swap with
     * @param pNodeB the other node to perform the swap
     * @param rElementMap a VertexElementMap which associates the indices of VertexElements in the old mesh
     *                   with indices of VertexElements in the new mesh.  This should be created
     *                   with the correct size, GetNumElements()
     */
    void IdentifySwapType(Node<SPACE_DIM>* pNodeA, Node<SPACE_DIM>* pNodeB, VertexElementMap& rElementMap);

    /**
     * Helper method for ReMesh(), called by IdentifySwapType().
     *
     * Merge two given nodes in the mesh and update node/element ownership, by replacing
     * the node contained in the least number of elements with the other node.
     *
     * @param pNodeA one of the nodes to perform the merge with
     * @param pNodeB the other node to perform the merge with
     */
    void PerformNodeMerge(Node<SPACE_DIM>* pNodeA, Node<SPACE_DIM>* pNodeB);

    /**
     * Helper method for ReMesh(), called by IdentifySwapType().
     *
     * Perform a T1 swap on two given nodes contained in a given set of elements.
     * This involves replacing the two nodes with two new nodes placed on either
     * side of the previous shared edge, such that the edge formed by the two new nodes
     * is the perpendicular bisector of the previous shared edge, and 'just larger' (by a
     * factor mCellRearrangementRatio) than mThresholdDistance.
     *
     * @param pNodeA one of the nodes to perform the swap with
     * @param pNodeB the other node to perform the swap
     * @param rElementsContainingNodes set of common elements
     */
    void PerformT1Swap(Node<SPACE_DIM>* pNodeA, Node<SPACE_DIM>* pNodeB, std::set<unsigned>& rElementsContainingNodes);

    /**
     * Helper method for ReMesh(), called by CheckForIntersections().
     *
     * Perform an element swap to resolve the situation where a given node
     * has been found to overlap a given element not containing it.
     *
     * @param pNode pointer to the node
     * @param elementIndex global index of the element in the mesh
     */
    void PerformIntersectionSwap(Node<SPACE_DIM>* pNode, unsigned elementIndex);

    /**
     * Helper method for ReMesh(), called by CheckForT2Swaps().
     *
     * Perform a T2 swap on a given triangular element whose area is smaller than mT2Threshold
     * by replacing it with a new node located at the centroid of the former element and updating
     * node/element ownership.
     *
     * @param rElement the element to remove
     */
    void PerformT2Swap(VertexElement<ELEMENT_DIM,SPACE_DIM>& rElement);

    /**
     * Helper method for ReMesh(), called by CheckForIntersections().
     *
     * Perform a T3 swap on a given node that has been found to overlap a given element
     * by moving the node back onto the edge of that element, associating it with the
     * element and adding new nodes to maintain three elements per node.
     *
     * @param pNode pointer to the node
     * @param elementIndex global index of the element in the mesh
     */
    void PerformT3Swap(Node<SPACE_DIM>* pNode, unsigned elementIndex);

    /**
     * Helper method for ReMesh(), called by IdentifySwapType().
     *
     * Remove a triangular void bounded by three given nodes, in which one of the edges is
     * less than mCellRearrangementThreshold, through calls to PerformNodeMerge().
     *
     * @param pNodeA one of the nodes on the short edge
     * @param pNodeB the other node on the short edge
     * @param pNodeC the other node in the triangular void
     */
    void PerformVoidRemoval(Node<SPACE_DIM>* pNodeA, Node<SPACE_DIM>* pNodeB, Node<SPACE_DIM>* pNodeC);

    /** Needed for serialization. */
    friend class boost::serialization::access;

    /**
     * Serialize the mesh.
     *
     * Note that if you are calling this method (from subclasses) you should archive your
     * member variables FIRST. So that this method can call a ReMesh
     * (to convert from TrianglesMeshReader input format into your native format).
     *
     * @param archive the archive
     * @param version the current version of this class
     */
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
        // NOTE - Subclasses must archive their member variables BEFORE calling this method.
        archive & mCellRearrangementThreshold;
        archive & mCellRearrangementRatio;
        archive & mT2Threshold;
        archive & mCheckForInternalIntersections;
        archive & mDeletedNodeIndices;
        archive & mDeletedElementIndices;
        ///todo: maybe we should archive the mLocationsOfT1Swaps and mDeletedNodeIndices etc. as well?

        archive & boost::serialization::base_object<VertexMesh<ELEMENT_DIM, SPACE_DIM> >(*this);
    }

public:

    /**
     * Default constructor.
     *
     * @param nodes vector of pointers to nodes
     * @param vertexElements vector of pointers to VertexElements
     * @param cellRearrangementThreshold the minimum threshold distance for element rearrangement (defaults to 0.01)
     * @param t2Threshold the maximum threshold distance for Type 2 swaps (defaults to 0.001)
     * @param cellRearrangementRatio ratio between the minimum threshold distance for element
     *                                rearrangement node separation after remeshing (defaults to 1.5)
     */
    MutableVertexMesh(std::vector<Node<SPACE_DIM>*> nodes,
                      std::vector<VertexElement<ELEMENT_DIM, SPACE_DIM>*> vertexElements,
                      double cellRearrangementThreshold=0.01,
                      double t2Threshold=0.001,
                      double cellRearrangementRatio=1.5);

    /**
     * Default constructor for use by serializer.
     */
    MutableVertexMesh();

    /**
     * Destructor.
     */
    virtual ~MutableVertexMesh();

    /**
     * Set method for mCellRearrangementThreshold.
     *
     * @param cellRearrangementThreshold
     */
    void SetCellRearrangementThreshold(double cellRearrangementThreshold);

    /**
     * Set method for mT2Threshold.
     *
     * @param t2Threshold
     */
    void SetT2Threshold(double t2Threshold);

    /**
     * Set method for mCellRearrangementRatio.
     *
     * @param cellRearrangementRatio
     */
    void SetCellRearrangementRatio(double cellRearrangementRatio);

    /**
     * Move the node with a particular index to a new point in space.
     *
     * @param nodeIndex the index of the node to be moved
     * @param point the new target location of the node
     */
    virtual void SetNode(unsigned nodeIndex, ChastePoint<SPACE_DIM> point);

    /**
     * Set method for mCheckForInternalIntersections.
     *
     * @param checkForInternalIntersections
     */
    void SetCheckForInternalIntersections(bool checkForInternalIntersections);

    /**
     * @return mCellRearrangementThreshold
     */
    double GetCellRearrangementThreshold() const;

    /**
     * @return mT2Threshold
     */
    double GetT2Threshold() const;

    /**
     * @return mCellRearrangementRatio
     */
    double GetCellRearrangementRatio() const;

    /**
     * @return the number of Nodes in the mesh.
     */
    unsigned GetNumNodes() const;

    /**
     * @return the number of VertexElements in the mesh.
     */
    unsigned GetNumElements() const;

    /**
     * @return mCheckForInternalIntersections, either to check for edges intersections or not.
     */
    bool GetCheckForInternalIntersections() const;

    /**
     * @return the locations of the T1 swaps
     */
    std::vector< c_vector<double, SPACE_DIM> > GetLocationsOfT1Swaps();

    /**
     * @return the locations of the T2 swaps
     */
    std::vector< c_vector<double, SPACE_DIM> > GetLocationsOfT2Swaps();

    /**
     * @return the locations of the T3 swaps
     */
    std::vector< c_vector<double, SPACE_DIM> > GetLocationsOfT3Swaps();

    /**
     * Helper method to clear the stored T1 swaps
     */
    void ClearLocationsOfT1Swaps();

    /**
     * Helper method to clear the stored T2 swaps
     */
    void ClearLocationsOfT2Swaps();

    /**
     * Helper method to clear the stored T3 swaps
     */
    void ClearLocationsOfT3Swaps();

    /**
     * Add a node to the mesh.
     *
     * Note: After calling this one or more times, you must then call ReMesh.
     *
     * @param pNewNode pointer to the new node
     * @return the global index of the new node in the mesh.
     */
    unsigned AddNode(Node<SPACE_DIM>* pNewNode);

    /**
     * Mark an element as deleted. Note that it DOES NOT deal with the associated
     * nodes and therefore should only be called immediately prior to a ReMesh()
     * being called.
     *
     * @param index  the global index of a specified vertex element
     */
    void DeleteElementPriorToReMesh(unsigned index);

    /**
     * Mark a given node as deleted. Note that this method DOES NOT deal with the
     * associated elements and therefore should only be called immediately prior
     * to a ReMesh() being called.
     *
     * @param index The index of the node to delete
     */
    void DeleteNodePriorToReMesh(unsigned index);

    /**
     * Divide an element along its short axis.
     *
     * @param pElement the element to divide
     * @param placeOriginalElementBelow whether to place the original element below (in the y direction) the new element (defaults to false)
     *
     * @return the index of the new element
     */
    unsigned DivideElementAlongShortAxis(VertexElement<ELEMENT_DIM,SPACE_DIM>* pElement,
                                         bool placeOriginalElementBelow=false);

    /**
     * Divide an element along a specified axis.
     *
     * If the new nodes (intersections of axis with element) are within
     * mCellRearrangementThreshold of existing nodes then they are
     * moved 2*mCellRearrangementThreshold away.
     *
     * @param pElement the element to divide
     * @param axisOfDivision axis to divide the element by
     * @param placeOriginalElementBelow whether to place the original element below (in the y direction) the new element (defaults to false)
     *
     * @return the index of the new element
     */
    unsigned DivideElementAlongGivenAxis(VertexElement<ELEMENT_DIM,SPACE_DIM>* pElement,
                                         c_vector<double, SPACE_DIM> axisOfDivision,
                                         bool placeOriginalElementBelow=false);

    /**
     * Add an element to the mesh.
     *
     * @param pNewElement the new element
     *
     * @return the index of the new element in the mesh
     */
    unsigned AddElement(VertexElement<ELEMENT_DIM, SPACE_DIM>* pNewElement);

    /**
     * Helper method for ReMesh().
     *
     * Check for any triangular element whose area is smaller than mT2Threshold
     * and call PerformT2Swap() on any such element.
     *
     * @param rElementMap a VertexElementMap which associates the indices of VertexElements in the old mesh
     *                   with indices of VertexElements in the new mesh.  This should be created
     *                   with the correct size, GetNumElements()
     *
     * @return whether we need to check for, and implement, any further local remeshing operations
     */
    bool CheckForT2Swaps(VertexElementMap& rElementMap);

    /**
     * Delete mNodes and mElements.
     */
    void Clear();

    /**
     * Add a node on the edge between two nodes.
     *
     * @param pNodeA a pointer to one node
     * @param pNodeB a pointer to the other nodes
     */
    void DivideEdge(Node<SPACE_DIM>* pNodeA, Node<SPACE_DIM>* pNodeB);

    /**
     * Helper method for ReMesh(). Removes the deleted nodes and elements from the mesh and updates the
     * rElementMap accordingly.
     *
     * @param rElementMap a VertexElementMap which associates the indices of VertexElements in the old mesh
     *                   with indices of VertexElements in the new mesh.  This should be created
     *                   with the correct size, GetNumElements()
     */
    void RemoveDeletedNodesAndElements(VertexElementMap& rElementMap);

    /**
     * Helper method for ReMesh(). Removes the deleted nodes from the mesh and relabels the node indices.
     */
    void RemoveDeletedNodes();

    /**
     * Update the state of the mesh by implementing any local remeshing operations (node merging,
     * or T1, T2 or T3 swaps) that are required, and store any changes in element indices using
     * the given VertexElementMap.
     *
     * This method calls several other methods, in particular CheckForT2Swaps(), CheckForT1Swaps()
     * and CheckForIntersections().
     *
     * @param rElementMap a VertexElementMap which associates the indices of VertexElements in the old mesh
     *                   with indices of VertexElements in the new mesh.  This should be created
     *                   with the correct size, GetNumElements()
     */
    void ReMesh(VertexElementMap& rElementMap);

    /**
     * Alternative version of ReMesh which takes no parameters and does not require a VertexElementMap.
     * Note: inherited classes should overload ReMesh(VertexElementMap&).
     *
     * \todo This method seems to be redundant; remove it? (#2401)
     */
    void ReMesh();
};

#include "SerializationExportWrapper.hpp"
EXPORT_TEMPLATE_CLASS_ALL_DIMS(MutableVertexMesh)

#endif /*MUTABLEVERTEXMESH_HPP_*/
