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

#ifndef _TETRAHEDRALMESH_HPP_
#define _TETRAHEDRALMESH_HPP_

#include "ChasteSerialization.hpp"
#include <boost/serialization/base_object.hpp>

#include "UblasVectorInclude.hpp"
#include "UblasMatrixInclude.hpp"

#include <vector>
#include <string>
#include <set>

#include "AbstractTetrahedralMesh.hpp"
#include "AbstractMeshReader.hpp"
#include "ChastePoint.hpp"


class triangulateio; /**< Forward declaration for triangle helper methods (used in MutableMesh QuadraticMesh)*/

//////////////////////////////////////////////////////////////////////////
//   DECLARATION
//////////////////////////////////////////////////////////////////////////

/**
 * A concrete tetrahedral mesh class.
 */
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
class TetrahedralMesh : public AbstractTetrahedralMesh< ELEMENT_DIM, SPACE_DIM>
{
    friend class TestTetrahedralMesh; // to give access to private methods (not variables)
    friend class TestCryptSimulation2d; // to give access to private methods (not variables)
private:
    /** Needed for serialization.*/
    friend class boost::serialization::access;
    /**
     * Serialize the mesh.
     *
     * @param archive the archive
     * @param version the current version of this class
     */
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
       archive & boost::serialization::base_object<AbstractTetrahedralMesh<ELEMENT_DIM, SPACE_DIM> >(*this);
    }

protected:

    /**
     * Overridden solve node mapping method.
     *
     * @param index the global index of the node
     */
    unsigned SolveNodeMapping(unsigned index) const;

    /**
     * Overridden solve element mapping method.
     *
     * @param index the global index of the element
     */
    unsigned SolveElementMapping(unsigned index) const;

    /**
     * Overridden solve boundary element mapping method.
     *
     * @param index the global index of the boundary element
     */
    unsigned SolveBoundaryElementMapping(unsigned index) const;

    /** Vector storing the weighted direction for each element in the mesh. */
    std::vector< c_vector<double, SPACE_DIM> > mElementWeightedDirections;

    /** Vector storing the Jacobian matrix for each element in the mesh. */
    std::vector< c_matrix<double, SPACE_DIM, ELEMENT_DIM> > mElementJacobians;

    /** Vector storing the inverse Jacobian matrix for each element in the mesh. */
    std::vector< c_matrix<double, ELEMENT_DIM, SPACE_DIM> > mElementInverseJacobians;

    /** Vector storing the Jacobian determinant for each element in the mesh. */
    std::vector<double> mElementJacobianDeterminants;

    /** Vector storing the weighted direction for each boundary element in the mesh. */
    std::vector< c_vector<double, SPACE_DIM> > mBoundaryElementWeightedDirections;

    /** Vector storing the determinant of the Jacobian matrix for each boundary element in the mesh. */
    std::vector<double> mBoundaryElementJacobianDeterminants;

    /**
     * Export the mesh (currently only the nodes) to an external mesher
     * This is determined at compile time when the MESHER_IO template is
     * instantiated to either
     *   - triangulateio (for triangle remesher in 2D)
     *   - tetgenio (for tetgen remesher in 3D)
     * Since conditional compilation is not allowed, care must be taken to only use
     * common data members in this method
     * @param map is a NodeMap which associates the indices of nodes in the old mesh
     * with indices of nodes in the new mesh.  This should be created with the correct size (NumAllNodes)
     * @param mesherInput is a triangulateio or tetgenio class (decided at compile time)
     *  Note that only nodes are exported and thus any late mesh is based on the convex hull
     * @param elementList is a pointer to either mesherInput.trianglelist or mesherInput.tetrahedronlist (used when we are not remeshing, but converting the form of an existing mesh)
     * Note that this should have been re-malloced and mesherInput.numberoftriangles or mesherInput.numberoftetrahedra should be set prior to the call when elementList is non-NULL
     *
     */
    template <class MESHER_IO>
    void ExportToMesher(NodeMap& map, MESHER_IO& mesherInput, int *elementList=NULL);

    /**
     * Import the mesh from an external mesher
     * This is determined at compile time when the MESHER_IO template is
     * instantiated to either
     *   - triangulateio (for triangle remesher in 2D)
     *   - tetgenio (for tetgen remesher in 3D)
     * Since conditional compilation is not allowed, care must be taken to only use
     * common data members in this method (or add arguments \todo #1545 ...)
     * @param mesherOutput is a triangulateio or tetgenio class (decided at compile time)
     * @param numberOfElements is a copy of either mesherOutput.numberoftriangles or mesherOutput.numberoftetrahedra
     * @param elementList is a pointer to either mesherOutput.trianglelist or mesherOutput.tetrahedronlist
     * @param numberOfFaces is a copy of either mesherOutput.edges or mesherOutput.numberoftrifaces
     * @param faceList is a pointer to either mesherOutput.edgelist or mesherOutput.trifacelist
     * @param edgeMarkerList is a pointer to either mesherOutput.edgemarkerlist or NULL
     *
     */
    template <class MESHER_IO>
    void ImportFromMesher(MESHER_IO& mesherOutput, unsigned numberOfElements, int *elementList, unsigned numberOfFaces, int *faceList, int *edgeMarkerList);


    /**
     * Convenience method to tidy up a triangleio data structure before use
     * @param mesherIo is a triangulateio class
     */
    void InitialiseTriangulateIo(triangulateio& mesherIo);

    /**
     * Convenience method to tidy up a triangleio data structure after use
     * @param mesherIo is a triangulateio class
     */
    void FreeTriangulateIo(triangulateio& mesherIo);

public:

    /**
     * Constructor.
     */
    TetrahedralMesh();

    /**
     * Construct the mesh using a MeshReader.
     *
     * @param rMeshReader the mesh reader
     */
    void ConstructFromMeshReader(AbstractMeshReader<ELEMENT_DIM,SPACE_DIM>& rMeshReader);

    /**
     * Read in the number of nodes per processor from file.
     *
     * @param rNodesPerProcessorFile the name of the file
     */
    void ReadNodesPerProcessorFile(const std::string& rNodesPerProcessorFile);

    /**
     * Check whether mesh is conforming
     * Conforming (defn.): the intersection of two elements should be
     * either the empty set, a vertex, an edge or a face.
     *
     * It may be possible to construct non-conforming meshes which contain
     * internal faces owned by only one element: two coplanar triangular faces
     * of two elements form a square, but the same square on the adjacent pair of
     * elements is formed by splitting the diagonal the other way.
     *
     * @return false if there are any orphaned internal faces
     */
    bool CheckIsConforming();

    /**
     * Return the volume of the mesh, calculated by adding the determinant of each element
     * and dividing by n!, where n is the element dimension.
     */
    double GetVolume();

    /**
     * Return the surface area of the mesh.
     */
    double GetSurfaceArea();

    /**
     * Overridden RefreshMesh method. This method calls RefreshJacobianCachedData.
     */
    void RefreshMesh();

    /**
     * Permute the nodes randomly so that they appear in a different order in mNodes
     * (and their mIndex's are altered accordingly).
     */
    void PermuteNodes();

    /**
     * Permute the nodes so that they appear in a different order in mNodes
     * (and their mIndex's are altered accordingly).
     * @param perm is a vector containing the new indices
     */
    void PermuteNodes(const std::vector<unsigned>& perm);

    /**
     * Return the element index for the first element that contains a test point
     *
     * @param rTestPoint reference to the point
     * @param strict  Should the element returned contain the point in the interior and
     *      not on an edge/face/vertex (default = not strict)
     * @param testElements  a set of guesses for the element (a set of element indices), to be checked
     *      first for potential efficiency improvements. (default = empty set)
     * @param onlyTryWithTestElements Do not continue with other elements after trying the with testElements
     *      (for cases where you know the testPoint must be in the set of test elements or maybe outside
     *      the mesh).
     */
    unsigned GetContainingElementIndex(const ChastePoint<SPACE_DIM>& rTestPoint,
                                       bool strict=false,
                                       std::set<unsigned> testElements=std::set<unsigned>(),
                                       bool onlyTryWithTestElements = false);


    /**
     * Return the element index for the first element that contains a test point. Like GetContainingElementIndex
     * but uses the user given element (M say) as the first element checked, and then checks M+1,M+2,..,Ne,0,1..
     *
     * @param rTestPoint reference to the point
     * @param startingElementGuess Which element to try first.
     * @param strict  Should the element returned contain the point in the interior and
     *      not on an edge/face/vertex (default = not strict)
     */
    unsigned GetContainingElementIndexWithInitialGuess(const ChastePoint<SPACE_DIM>& rTestPoint, unsigned startingElementGuess, bool strict=false);

    /**
     * Return the element index for an element is closest to the testPoint.
     *
     * "Closest" means that the minimum interpolation weights for the testPoint are
     * maximised for this element.
     *
     * @param rTestPoint reference to the point
     */
    unsigned GetNearestElementIndex(const ChastePoint<SPACE_DIM>& rTestPoint);

    /** As with GetNearestElementIndex() except only searches in the given set of elements.
     *  @param rTestPoint reference to the point
     *  @param testElements a set of elements (element indices) to look in
     */
    unsigned GetNearestElementIndexFromTestElements(const ChastePoint<SPACE_DIM>& rTestPoint,
                                                    std::set<unsigned> testElements);


    /**
     * Return all element indices for elements that are known to contain a test point.
     *
     * @param rTestPoint reference to the point
     */
    std::vector<unsigned> GetContainingElementIndices(const ChastePoint<SPACE_DIM>& rTestPoint);

    /**
     * Clear all the data in the mesh.
     */
    virtual void Clear();

    /**
     * Return the set of nodes which are on the boundary of the flagged region(s).
     */
    std::set<unsigned> CalculateBoundaryOfFlaggedRegion();

    /**
     * Calcuate the angle between the node at indexB and the x axis about
     * the node at indexA. The angle returned is in the range (-pi,pi].
     *
     * @param indexA a node index
     * @param indexB a node index
     */
    double GetAngleBetweenNodes(unsigned indexA, unsigned indexB);

    /**
     * Unflag all elements in the mesh.
     */
    void UnflagAllElements();

    /**
     * Flag all elements not containing ANY of the given nodes
     *
     * @param rNodes  set of nodes to check for
     */
    void FlagElementsNotContainingNodes(const std::set<unsigned> rNodes);

    /** Update mElementJacobians, mElementWeightedDirections and mBoundaryElementWeightedDirections. */
    virtual void RefreshJacobianCachedData();

    /**
     * Get the Jacobian matrix and its determinant for a given element.
     *
     * @param elementIndex index of the element in the mesh
     * @param rJacobian the Jacobian matrix
     * @param rJacobianDeterminant the determinant of the Jacobian matrix
     */
    virtual void GetJacobianForElement(unsigned elementIndex, c_matrix<double, SPACE_DIM, SPACE_DIM>& rJacobian, double& rJacobianDeterminant) const;

    /**
     * Get the Jacobian matrix, its inverse and its determinant for a given element.
     *
     * @param elementIndex index of the element in the mesh
     * @param rJacobian the Jacobian matrix
     * @param rJacobianDeterminant the determinant of the Jacobian matrix
     * @param rInverseJacobian the inverse Jacobian matrix
     */
    virtual void GetInverseJacobianForElement(unsigned elementIndex, c_matrix<double, SPACE_DIM, ELEMENT_DIM>& rJacobian, double& rJacobianDeterminant, c_matrix<double, ELEMENT_DIM, SPACE_DIM>& rInverseJacobian) const;

    /**
     * Get the weighted direction and the determinant of the Jacobian for a given element.
     *
     * @param elementIndex index of the element in the mesh
     * @param rWeightedDirection the weighted direction
     * @param rJacobianDeterminant the determinant of the Jacobian matrix
     */
    virtual void GetWeightedDirectionForElement(unsigned elementIndex, c_vector<double, SPACE_DIM>& rWeightedDirection, double& rJacobianDeterminant) const;

    /**
     * Get the weighted direction and the determinant of the Jacobian for a given boundary element.
     *
     * @param elementIndex index of the element in the mesh
     * @param rWeightedDirection the weighted direction
     * @param rJacobianDeterminant the determinant of the Jacobian matrix
     */
    virtual void GetWeightedDirectionForBoundaryElement(unsigned elementIndex, c_vector<double, SPACE_DIM>& rWeightedDirection, double& rJacobianDeterminant) const;

    /**
     * Iterator over edges in the mesh.
     *
     * This class takes care of the logic to make sure that you consider each edge exactly once.
     *
     */
    class EdgeIterator
    {
    public:
        /**
         * Get a pointer to the node in the mesh at end A of the spring.
         */
        Node<SPACE_DIM>* GetNodeA();
        /**
         * Get a pointer to the node in the mesh at end B of the spring.
         */
        Node<SPACE_DIM>* GetNodeB();

        /**
         * Comparison not-equal-to.
         *
         * @param rOther edge iterator with which comparison is made
         */
        bool operator!=(const TetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::EdgeIterator& rOther);

        /**
         * Prefix increment operator.
         */
        EdgeIterator& operator++();

        /**
         * Constructor for a new edge iterator.
         *
         * @param rMesh  The mesh
         * @param elemIndex  An element index
         */
        EdgeIterator(TetrahedralMesh& rMesh, unsigned elemIndex);




    private:
        /** Keep track of what edges have been visited
         *  Each edge is stored as a pair of ordered indices
         */
        std::set< std::pair<unsigned, unsigned> > mEdgesVisited;

        TetrahedralMesh& mrMesh;   /**< The mesh. */

        unsigned mElemIndex;       /**< Element index. */
        unsigned mNodeALocalIndex; /**< Index of one node on the edge. */
        unsigned mNodeBLocalIndex; /**< Index of the other node on the edge. */

    };

    /**
     * @return iterator pointing to the first edge (i.e. connection between 2 nodes) of the mesh
     */
    EdgeIterator EdgesBegin();

    /**
     * @return iterator pointing to one past the last edge (i.e. connection between 2 nodes)
     * of the mesh
     */
    EdgeIterator EdgesEnd();
};

#include "SerializationExportWrapper.hpp"
EXPORT_TEMPLATE_CLASS_ALL_DIMS(TetrahedralMesh)

#endif //_TETRAHEDRALMESH_HPP_
