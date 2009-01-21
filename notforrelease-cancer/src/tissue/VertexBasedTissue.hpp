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
#ifndef VERTEXBASEDTISSUE_HPP_
#define VERTEXBASEDTISSUE_HPP_

#include "AbstractTissue.hpp"
#include "VertexMesh.hpp"
#include "MeshArchiveInfo.hpp"

#include <climits> // work around boost bug

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>


/**
 * A facade class encapsulating a vertex-based 'tissue'.
 *
 * Contains a group of cells and maintains the associations
 * between TissueCells and elements in the VertexMesh.
 *
 */
template<unsigned DIM>
class VertexBasedTissue : public AbstractTissue<DIM>
{
private:

    /** Vertex-based mesh associated with the tissue. */
    VertexMesh<DIM, DIM>& mrMesh;

    /**
     * Whether to delete the mesh when we are destroyed.
     * Needed if this tissue has been de-serialized.
     */
    bool mDeleteMesh;

    /** Results file for elements. */
    out_stream mpElementFile;

    friend class boost::serialization::access;
    /**
     * Serialize the facade.
     *
     * Note that serialization of the mesh and cells is handled by load/save_construct_data.
     *
     * Note also that member data related to writers is not saved - output must
     * be set up again by the caller after a restart.
     */
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
        archive & boost::serialization::base_object<AbstractTissue<DIM> >(*this);
    }

public:

    /**
     * Create a new tissue facade from a mesh and collection of cells.
     *
     * There must be precisely one TissueCell for each VertexElement in
     * the mesh.
     *
     * @param rMesh reference to a VertexMesh
     * @param cells reference to a vector of TissueCells
     * @param deleteMesh set to true if you want the tissue to free the mesh memory on destruction
     * @param validate whether to validate the tissue when it is created (defaults to true)
     */
    VertexBasedTissue(VertexMesh<DIM, DIM>& rMesh,
                      const std::vector<TissueCell>& rCells,
                      bool deleteMesh=false,
                      bool validate=true,
                      const std::vector<unsigned> locationIndices=std::vector<unsigned>());

    /**
     * Constructor for use by the de-serializer.
     *
     * @param rMesh a vertex mesh.
     */
    VertexBasedTissue(VertexMesh<DIM, DIM>& rMesh);

    /**
     * Destructor, which frees any memory allocated by the constructor.
     */
    virtual ~VertexBasedTissue();

    /**
     * Overridden GetDampingConstant() method.
     *
     * @param nodeIndex the global index of this node
     * @return the damping constant for the given nod.
     */
    double GetDampingConstant(unsigned nodeIndex);

    /**
     * @return reference to  mrMesh.
     */
    VertexMesh<DIM, DIM>& rGetMesh();

    /**
     * @return const reference to mrMesh (used in archiving).
     */
    const VertexMesh<DIM, DIM>& rGetMesh() const;

    /**
     * Get a particular VertexElement.
     *
     * @param elementIndex the global index of the VertexElement
     *
     * @return a pointer to the VertexElement.
     */
    VertexElement<DIM, DIM>* GetElement(unsigned elementIndex);

    /**
     * @return the number of VertexElements in the tissue.
     */
    unsigned GetNumElements();

    /**
     * Overridden GetNumNodes() method.
     *
     * @return the number of nodes in the tissue.
     */
    unsigned GetNumNodes();

    /**
     * Overridden GetNode() method.
     *
     * @param index global index of the specified node
     *
     * @return a pointer to the node.
     */
    Node<DIM>* GetNode(unsigned index);

    /**
     * Overridden AddNode() method.
     *
     * Add a new node to the tissue.
     *
     * @param pNewNode pointer to the new node
     * @return global index of new node in tissue
     */
    unsigned AddNode(Node<DIM> *pNewNode);

    /**
     * Overridden UpdateNodeLocations() method.
     * 
     * @param rNodeForces a vector containing the force on each node in the tissue
     * @param dt the time step
     */
    void UpdateNodeLocations(const std::vector< c_vector<double, DIM> >& rNodeForces, double dt);

    /**
     * Overridden SetNode() method.
     *
     * Move the node with a given index to a new point in space.
     *
     * @param index the index of the node to be moved
     * @param rNewLocation the new target location of the node
     */
    void SetNode(unsigned index, ChastePoint<DIM>& rNewLocation);

    /**
     * Get a pointer to the element corresponding to a given TissueCell.
     */
    VertexElement<DIM, DIM>* GetElementCorrespondingToCell(TissueCell* pCell);

    /**
     * Overridden AddCell() method.
     *
     * Add a new cell to the tissue.
     *
     * @param rNewCell  the cell to add
     * @param newLocation  the position in space at which to put it
     * @param pParentCell pointer to a parent cell (if required)
     * @returns address of cell as it appears in the cell list (internal of this method uses a copy constructor along the way)
     */
    TissueCell* AddCell(TissueCell& rNewCell, c_vector<double,DIM> newLocation, TissueCell* pParentCell=NULL);

    /**
     * Remove all cells labelled as dead.
     *
     * Note that after calling this method the tissue will be in an inconsistent state until
     * the equivalent of a 'remesh' is performed! So don't try iterating over cells or anything
     * like that.
     *
     * @return number of cells removed
     */
    unsigned RemoveDeadCells();

    /**
     * Overridden IsCellAssociatedWithADeletedNode() method.
     *
     * @param rCell the cell
     * @return whether a given cell is associated with a deleted node.
     */
    bool IsCellAssociatedWithADeletedNode(TissueCell& rCell);

    /**
     * Remove the VertexElements which have been marked as deleted, perform
     * any cell rearrangements if required, and update the correspondence
     * with TissueCells.
     */
    void Update();

    /**
     * Check the consistency of internal data structures.
     * Each VertexElement must have a TissueCell associated with it.
     */
    virtual void Validate();

    /**
     * Overridden WriteMeshToFile() method. For use by
     * the TissueSimulationArchiver.
     *
     * @param rArchiveDirectory directory in which archive is stored
     * @param rMeshFileName base name for mesh files
     */
    void WriteMeshToFile(const std::string &rArchiveDirectory, const std::string &rMeshFileName);

    /**
     * Overridden CreateOutputFiles() method.
     *
     * @param rDirectory  pathname of the output directory, relative to where Chaste output is stored
     * @param rCleanOutputDirectory  whether to delete the contents of the output directory prior to output file creation
     * @param outputCellMutationStates  whether to create a cell mutation state results file
     * @param outputCellTypes  whether to create a cell type results file
     * @param outputCellVariables  whether to create a cell-cycle variable results file
     * @param outputCellCyclePhases  whether to create a cell-cycle phase results file
     * @param outputCellAncestors  whether to create a cell ancestor results file
     */
    void CreateOutputFiles(const std::string &rDirectory,
                           bool rCleanOutputDirectory,
                           bool outputCellMutationStates,
                           bool outputCellTypes,
                           bool outputCellVariables,
                           bool outputCellCyclePhases,
                           bool outputCellAncestors);
    /**
     * Overridden CloseOutputFiles() method.
     *
     * @param outputCellMutationStates  whether a cell mutation state results file is open
     * @param outputCellTypes  whether a cell type results file is open
     * @param outputCellVariables  whether a cell-cycle variable results file is open
     * @param outputCellCyclePhases  whether a cell-cycle phase results file is open
     * @param outputCellAncestors  whether a cell ancestor results file is open
     */
    void CloseOutputFiles(bool outputCellMutationStates,
                          bool outputCellTypes,
                          bool outputCellVariables,
                          bool outputCellCyclePhases,
                          bool outputCellAncestors);

    /**
     * Overridden WriteResultsToFiles() method.
     *
     * @param outputCellMutationStates  whether to output cell mutation state results
     * @param outputCellTypes  whether to output cell type results
     * @param outputCellVariables  whether to output cell-cycle variable results
     * @param outputCellCyclePhases  whether to output cell-cycle phase results
     * @param outputCellAncestors  whether to output cell ancestor results
     */
    void WriteResultsToFiles(bool outputCellMutationStates,
                             bool outputCellTypes,
                             bool outputCellVariables,
                             bool outputCellCyclePhases,
                             bool outputCellAncestors);

};

#include "TemplatedExport.hpp"
EXPORT_TEMPLATE_CLASS_SAME_DIMS(VertexBasedTissue)

namespace boost
{
namespace serialization
{
/**
 * Serialize information required to construct a Tissue facade.
 */
template<class Archive, unsigned DIM>
inline void save_construct_data(
    Archive & ar, const VertexBasedTissue<DIM> * t, const BOOST_PFTO unsigned int file_version)
{
    // Save data required to construct instance
    const VertexMesh<DIM,DIM>* p_mesh = &(t->rGetMesh());
    ar & p_mesh;
}

/**
 * De-serialize constructor parameters and initialise Tissue.
 * Loads the mesh from separate files.
 */
template<class Archive, unsigned DIM>
inline void load_construct_data(
    Archive & ar, VertexBasedTissue<DIM> * t, const unsigned int file_version)
{
    // Retrieve data from archive required to construct new instance
    assert(MeshArchiveInfo::meshPathname.length() > 0);
    VertexMesh<DIM,DIM>* p_mesh;
    ar >> p_mesh;

    // Re-initialise the mesh
    p_mesh->Clear();
    VertexMeshReader2d mesh_reader(MeshArchiveInfo::meshPathname);
    p_mesh->ConstructFromMeshReader(mesh_reader);

    // Remesh
    NodeMap map(p_mesh->GetNumElements());
    p_mesh->ReMesh(map);

    // Invoke inplace constructor to initialise instance
    ::new(t)VertexBasedTissue<DIM>(*p_mesh);
}
}
} // namespace ...

#endif /*VERTEXBASEDTISSUE_HPP_*/

