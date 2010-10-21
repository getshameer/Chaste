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
#ifndef CYLINDRICAL2DMESH_HPP_
#define CYLINDRICAL2DMESH_HPP_

#include "ChasteSerialization.hpp"
#include <boost/serialization/base_object.hpp>

#include <cmath>
#include <map>

#include "MutableMesh.hpp"
#include "TrianglesMeshWriter.hpp"

/**
 * A subclass of MutableMesh<2,2> for a rectangular mesh with
 * periodic left and right boundaries, representing a cylindrical geometry.
 *
 * The class works by overriding calls such as ReMesh() and
 * GetVectorFromAtoB() so that simulation classes can treat this
 * class in exactly the same way as a MutableMesh<2,2>.
 */
class Cylindrical2dMesh : public MutableMesh<2,2>
{
    friend class TestCylindrical2dMesh;
private:

    /** The circumference of the cylinder. */
    double mWidth;

    /** The top of the cylinder (y coordinate). */
    double mTop;

    /** The bottom of the cylinder (y coordinate). */
    double mBottom;

    /** The left nodes which have been mirrored during the remesh. */
    std::vector<unsigned> mLeftOriginals;

    /** The image nodes corresponding to these left nodes (on right of mesh). */
    std::vector<unsigned> mLeftImages;

    /** A map from image node index (on right of mesh) to original node index (on left of mesh). */
    std::map<unsigned, unsigned> mImageToLeftOriginalNodeMap;

    /** The right nodes which have been mirrored during the remesh. */
    std::vector<unsigned> mRightOriginals;

    /** The image nodes corresponding to these right nodes (on left of mesh). */
    std::vector<unsigned> mRightImages;

    /** A map from image node index (on left of mesh) to original node index (on right of mesh). */
    std::map<unsigned, unsigned> mImageToRightOriginalNodeMap;

    /** The indices of elements which straddle the left periodic boundary. */
    std::set<unsigned> mLeftPeriodicBoundaryElementIndices;

    /** The indices of elements which straddle the right periodic boundary. */
    std::set<unsigned> mRightPeriodicBoundaryElementIndices;

    /** The indices of nodes on the top boundary. */
    std::vector<unsigned > mTopHaloNodes;

    /** The indices of nodes on the bottom boundary. */
    std::vector<unsigned > mBottomHaloNodes;

    /**
     * Calls AbstractMesh<2,2>::CalculateBoundingBox() to calculate mTop and mBottom
     * for the cylindrical mesh.
     *
     * This method should only ever be called by the public ReMesh() method.
     */
    void UpdateTopAndBottom();

    /**
     * This method creates a compressed row of nodes, just above and below the main mesh
     * at a constant height (a 'halo'). These will mesh to a known configuration
     * (each one on the boundary), this avoids boundary elements of lengths over
     * half the mesh width, which prevent the process of cylindrical meshing.
     *
     * The nodes which are created are later removed by DeleteHaloNodes().
     */
    void CreateHaloNodes();

    /**
     * Creates a set of mirrored nodes for a cylindrical re-mesh. Updates
     * mRightImages and mLeftImages. All mesh points should be 0 < x < mWidth.
     *
     * This method should only ever be called by the public ReMesh() method.
     */
    void CreateMirrorNodes();

    /**
     *
     * After any corrections have been made to the boundary elements (see UseTheseElementsToDecideMeshing())
     * this method deletes the mirror image nodes, elements and boundary elements created
     * for a cylindrical remesh by cycling through the elements and changing
     * elements with partly real and partly imaginary elements to be real with
     * periodic real nodes instead of mirror image nodes. We end up with very
     * strangely shaped elements which cross the whole mesh but specify the correct
     * connections between nodes.
     *
     * This method should only ever be called by the public ReMesh() method.
     */
    void ReconstructCylindricalMesh();

    /**
     * This method should only ever be called by the public ReMesh method.
     *
     * This method removes the nodes which were added by CreateHaloNodes()
     * before the remeshing algorithm was called.
     */
    void DeleteHaloNodes();

    /**
     * This method should only ever be called by the public ReMesh() method.
     *
     * Uses mLeftPeriodicBoundaryElementIndices and mRightPeriodicBoundaryElementIndices
     * and compares the nodes in each to ensure that both boundaries have been meshed
     * identically. If they have not it calls UseTheseElementsToDecideMeshing() to
     * sort out the troublesome elements which have been meshed differently on each
     * side and uses the meshing of the elements on the right hand boundary to decide
     * on how to mesh the left hand side.
     */
    void CorrectNonPeriodicMesh();

    /**
     * This method should only ever be called by the public ReMesh method.
     *
     * The elements which straddle the periodic boundaries need to be
     * identified in order to compare the list on the right with
     * the list on the left and reconstruct a cylindrical mesh.
     *
     * Empties and repopulates the member variables
     * mLeftPeriodicBoundaryElementIndices and mRightPeriodicBoundaryElementIndices
     */
    void GenerateVectorsOfElementsStraddlingPeriodicBoundaries();

    /**
     * This method should only ever be called by the public ReMesh() method.
     *
     * @param nodeIndex  The index of an original/mirrored node
     * @return the index of the corresponding mirror image of that node 
     *         (can be either an original or mirror node)
     */
    unsigned GetCorrespondingNodeIndex(unsigned nodeIndex);

    /**
     * This method takes in two elements which are not meshed in the same way
     * on the opposite boundary. It deletes the corresponding two elements
     * (connecting the same four nodes) and makes two new elements which are
     * connected in the same way. We should then be able to reconstruct the
     * cylindrical mesh properly.
     *
     * @param rMainSideElements two elements (usually in a square) which have
     *                          been meshed differently on the opposite boundary
     */
    void UseTheseElementsToDecideMeshing(std::set<unsigned>& rMainSideElements);

    /** Needed for serialization. */
    friend class boost::serialization::access;
    /**
     * Archives the member variables of the Cylindrical2dMesh class which
     * have to be preserved during the lifetime of the mesh.
     *
     * The remaining member variables are re-initialised before being used
     * by each ReMesh() call so they do not need to be archived.
     *
     * @param archive the archive
     * @param version the current version of this class the current version 
     *                of this class
     */
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
        archive & boost::serialization::base_object<MutableMesh<2,2> >(*this);
        archive & mWidth;
        archive & mTop;
        archive & mBottom;
    }

public:

    /**
     * Constructor.
     *
     * @param width the width of the mesh (circumference)
     */
    Cylindrical2dMesh(double width);

    /**
     * A constructor which reads in a width and collection of nodes, then
     * calls a ReMesh() command to create the elements of the mesh.
     *
     * @param width the periodic length scale
     * @param nodes a collection of nodes to construct the mesh with
     */
    Cylindrical2dMesh(double width, std::vector<Node<2>*> nodes);

    /**
     * Destructor.
     */
    ~Cylindrical2dMesh();

    /**
     * Overridden ReMesh() method.
     * 
     * Conduct a cylindrical remesh by calling CreateMirrorNodes() to create
     * mirror image nodes, then calling ReMesh() on the parent class, then
     * mapping the new node indices and calling ReconstructCylindricalMesh()
     * to remove surplus nodes, leaving a fully periodic mesh.
     *
     * @param rMap a reference to a nodemap which should be created with the required number of nodes.
     */
    void ReMesh(NodeMap& rMap);

    /**
     * Overridden GetVectorFromAtoB() method.
     * 
     * Evaluates the (surface) distance between two points in a 2D cylindrical
     * geometry.
     *
     * @param rLocation1 the x and y co-ordinates of point 1
     * @param rLocation2 the x and y co-ordinates of point 2
     * @return the vector from location1 to location2
     */
    c_vector<double, 2> GetVectorFromAtoB(const c_vector<double, 2>& rLocation1, const c_vector<double, 2>& rLocation2);

    /**
     * Overridden SetNode() method.
     *
     * If the location should be set outside a cylindrical boundary, it is moved
     * back onto the cylinder.
     * 
     * @param index is the index of the node to be moved
     * @param point is the new target location of the node
     * @param concreteMove is set to false if we want to skip the signed area tests
     */
    void SetNode(unsigned index, ChastePoint<2> point, bool concreteMove);

    /**
     * Overridden GetWidth() method.
     * 
     * Calculate the 'width' of any dimension of the mesh, taking periodicity
     * into account.
     *
     * @param rDimension a dimension (0 or 1)
     * @return The maximum distance between any nodes in this dimension.
     */
    double GetWidth(const unsigned& rDimension) const;

    /**
     * Overridden AddNode() method.
     *
     * @param pNewNode the node to be added to the mesh
     * @return the global index of the new node
     */
    unsigned AddNode(Node<2>* pNewNode);
};

namespace boost
{
namespace serialization
{
/**
 * Serialize information required to construct a Cylindrical2dMesh.
 */
template<class Archive>
inline void save_construct_data(
    Archive & ar, const Cylindrical2dMesh * t, const BOOST_PFTO unsigned int file_version)
{
    // Save data required to construct instance
    const double width = t->GetWidth(0);
    ar & width;
}

/**
 * De-serialize constructor parameters and initialise a Cylindrical2dMesh.
 */
template<class Archive>
inline void load_construct_data(
    Archive & ar, Cylindrical2dMesh * t, const unsigned int file_version)
{
    // Retrieve data from archive required to construct new instance
    double width;
    ar & width;

    // Invoke inplace constructor to initialise instance
    ::new(t)Cylindrical2dMesh(width);
}
}
} // namespace ...

#include "SerializationExportWrapper.hpp"
CHASTE_CLASS_EXPORT(Cylindrical2dMesh)

#endif /*CYLINDRICAL2DMESH_HPP_*/