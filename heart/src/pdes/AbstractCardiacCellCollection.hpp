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
#ifndef ABSTRACTCARDIACCELLCOLLECTION_HPP_
#define ABSTRACTCARDIACCELLCOLLECTION_HPP_

#include <set>
#include <vector>
#include "ChasteSerialization.hpp"
#include "ClassIsAbstract.hpp"
#include <boost/serialization/base_object.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/serialization/version.hpp>

#include "AbstractCardiacCell.hpp"
#include "AbstractCardiacCellFactory.hpp"
#include "AbstractConductivityTensors.hpp"

#include "ReplicatableVector.hpp"
#include "HeartConfig.hpp"
#include "ArchiveLocationInfo.hpp"
#include "AbstractDynamicallyLoadableEntity.hpp"
#include "DynamicModelLoaderRegistry.hpp"

//// OLD NOTE: read this if AbstractPde is brought back
// IMPORTANT NOTE: the inheritance of AbstractPde has to be 'virtual'
// ie "class AbstractCardiacCellCollection : public virtual AbstractPde"
// because AbstractPde will be the top class in a 'dreaded diamond':
//      A
//     / \     A = AbstractPde, B = AbstractCardiac, C = AbstractLinearParabolic (etc)
//    B   C    D = MonodomainCellCollection
//     \ /
//      D
//
// B and C must use virtual inheritence of A in order for D to only contain 1 instance
// of the member variables in A

/**
 * Class containing common functionality to monodomain and bidomain PDEs.
 *
 * Contains the cardiac cells (ODE systems for each node of the mesh),
 * conductivity tensors (dependent on fibre directions).
 *
 * Also contains knowledge of parallelisation in the form of the
 * distributed vector factory.
 */
template <unsigned ELEMENT_DIM, unsigned SPACE_DIM = ELEMENT_DIM>
class AbstractCardiacCellCollection : boost::noncopyable
{
private:

    /** Needed for serialization. */
    friend class boost::serialization::access;
    /**
     * Archive the member variables.
     *
     * @param archive
     * @param version
     */
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
        // archive & mpMesh; Archived in save/load_constructs at the bottom of mono/bidomainPde.hpp
        // archive & mpIntracellularConductivityTensors; Loaded from HeartConfig every time constructor is called
        // archive & mCellsDistributed; Archived in save/load_constructs at the bottom of mono/bidomainPde.hpp
        // archive & mIionicCacheReplicated; // will be regenerated
        // archive & mIntracellularStimulusCacheReplicated; // will be regenerated
        // archive & mStride; // archiving constructor sets this.
        archive & mDoCacheReplication;


        // we no longer have a bool mDoOneCacheReplication, but to maintain backwards compatibility
        // we archive something if version==0
        if(version==0)
        {
            bool do_one_cache_replication = true;
            archive & do_one_cache_replication;
        }

        (*ProcessSpecificArchive<Archive>::Get()) & mpDistributedVectorFactory;

        // Paranoia: check we agree with the mesh on who owns what
        assert(mpDistributedVectorFactory->GetLow()==mpMesh->GetDistributedVectorFactory()->GetLow());
        assert(mpDistributedVectorFactory->GetLocalOwnership()==mpMesh->GetDistributedVectorFactory()->GetLocalOwnership());
        // archive & mMeshUnarchived; Not archived since set to true when archiving constructor is called.
    }

    /**
     * Convenience method for intracellular conductivity tensor creation
     */
    void CreateIntracellularConductivityTensor();

protected:

    /** It's handy to keep a pointer to the mesh object*/
    AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* mpMesh;

    /** Intracellular conductivity tensors.  Not archived, since it's loaded from the HeartConfig singleton. */
    AbstractConductivityTensors<SPACE_DIM> *mpIntracellularConductivityTensors;

    /** The vector of cells. Distributed. */
    std::vector< AbstractCardiacCell* > mCellsDistributed;

    /**
     *  Cache containing all the ionic currents for each node,
     *  replicated over all processes.
     */
    ReplicatableVector mIionicCacheReplicated;

    /**
     *  Cache containing all the stimulus currents for each node,
     *  replicated over all processes.
     */
    ReplicatableVector mIntracellularStimulusCacheReplicated;

    /**
     *  Constant set to 1 in monodomain and 2 in bidomain. Used when accessing
     *  the voltage components in the solution vector (because the solution vector
     *  is of the form (V_1, phi_1, V_2, phi_2, ......, V_N, phi_N), where V_j is
     *  the voltage at node j and phi_j is the extracellular potential at node j.
     */
    const unsigned mStride;

    /** Local pointer to the HeartConfig singleton instance, for convenience. */
    HeartConfig* mpConfig;

    /**
     * Whether we need to replicate the caches.
     *
     * When doing matrix-based RHS assembly, we only actually need information from
     * cells/nodes local to the processor, so replicating the caches is an
     * unnecessary communication overhead.
     *
     * Defaults to true.
     */
    bool mDoCacheReplication;

    /**
     * Local pointer to the distributed vector factory associated with the mesh object used.
     *
     * Used to retrieve node ownership range when needed.
     *
     * NB: This is set from mpMesh->GetDistributedVectorFactory() and thus always equal to
     * that.  We never assume ownership of the object.
     */
    DistributedVectorFactory* mpDistributedVectorFactory;

    /**
     * Whether the mesh was unarchived or got from elsewhere.
     */
    bool mMeshUnarchived;

    /**
     * Helper method for destructor.
     * @param deleteFakeCells  whether to delete fake bath cells
     */
    void DeleteCells(bool deleteFakeCells);

public:
    /**
     * This constructor is called from the Initialise() method of the CardiacProblem class.
     * It creates all the cell objects, and sets up the conductivities.
     *
     * @param pCellFactory  factory to use to create cells.
     * @param stride  determines how to access \f$V_m\f$ in the solution vector (1 for monodomain, 2 for bidomain).
     */
    AbstractCardiacCellCollection(AbstractCardiacCellFactory<ELEMENT_DIM,SPACE_DIM>* pCellFactory,
                       const unsigned stride=1);

    /**
     * This constructor is called by the archiver
     *
     * @param rCellsDistributed  pointers to the cardiac cells.
     * @param pMesh  a pointer to the AbstractTetrahedral mesh.
     * @param stride  determines how to access \f$V_m\f$ in the solution vector (1 for monodomain, 2 for bidomain).
     */
    AbstractCardiacCellCollection(std::vector<AbstractCardiacCell*>& rCellsDistributed,
                       AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh,
                       const unsigned stride);

    /** Virtual destructor */
    virtual ~AbstractCardiacCellCollection();

    /**
     * Add more cells to this cardiac PDE.
     *
     * This method is used by the checkpoint migration code to load a simulation checkpointed in parallel onto
     * a single process.  It adds the cells previously contained on one of the non-master processes to this
     * process' collection.
     *
     * @param rOtherCells  the cells to add.  This vector will have the same length as our collection, but
     *   contain non-NULL pointers in (some of) the places we have NULLs.
     */
    void MergeCells(const std::vector<AbstractCardiacCell*>& rOtherCells);

    /**
     * Set whether or not to replicate the caches across all processors.
     *
     * See also mDoCacheReplication.
     * @param doCacheReplication - true if the cache needs to be replicated
     */
    void SetCacheReplication(bool doCacheReplication);

    /**
     * Get whether or not to replicate the caches across all processors.
     *
     * @return mDoCacheReplication - true if the cache needs to be replicated
     */
    bool GetDoCacheReplication();

    /** Get the intracellular conductivity tensor for the given element
     * @param elementIndex  index of the element of interest
     */
    const c_matrix<double, SPACE_DIM, SPACE_DIM>& rGetIntracellularConductivityTensor(unsigned elementIndex);

    /**
     * Get a pointer to a cell, indexed by the global node index.
     *
     * \note Should only called by the process owning the cell -
     * triggers an assertion otherwise.
     *
     * @param globalIndex  global node index for which to retrieve a cell
     */
    AbstractCardiacCell* GetCardiacCell( unsigned globalIndex );

    /**
     * Integrate the cell ODEs and update ionic current etc for each of the
     * cells, between the two times provided.
     *
     * \note This used to be called PrepareForAssembleSystem, but
     * that method is now a virtual method in the assemblers not the
     * pdes.
     *
     * @param existingSolution  the current voltage solution vector
     * @param time  the current simulation time
     * @param nextTime  when to simulate the cells until
     */
    virtual void SolveCellSystems(Vec existingSolution, double time, double nextTime);

    /** Get the entire ionic current cache */
    ReplicatableVector& rGetIionicCacheReplicated();

    /** Get the entire stimulus current cache */
    ReplicatableVector& rGetIntracellularStimulusCacheReplicated();


    /**
     * Update the Iionic and intracellular stimulus caches.
     *
     * @param globalIndex  global index of the entry to update
     * @param localIndex  local index of the entry to update
     * @param nextTime  the next PDE time point, at which to evaluate the stimulus current
     */
    void UpdateCaches(unsigned globalIndex, unsigned localIndex, double nextTime);

    /**
     *  Replicate the Iionic and intracellular stimulus caches.
     */
    void ReplicateCaches();

    /**
     *  Returns a reference to the vector of distributed cells. Needed for archiving.
     */
    const std::vector<AbstractCardiacCell*>& rGetCellsDistributed() const;

    /**
     *  Returns a pointer to the mesh object
     *
     *  @return pointer to mesh object
     */
    const AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pGetMesh() const;

    /**
     * Save our collection of cells to an archive.
     *
     * Writes:
     *  -# #mpDistributedVectorFactory
     *  -# number of cells on this process
     *  -# each cell pointer in turn
     *
     * @param archive  the master archive; cells will actually be written to the process-specific archive.
     * @param version
     */
    template<class Archive>
    void SaveCardiacCells(Archive & archive, const unsigned int version) const
    {
        Archive& r_archive = *ProcessSpecificArchive<Archive>::Get();
        const std::vector<AbstractCardiacCell*> & r_cells_distributed = rGetCellsDistributed();
        r_archive & mpDistributedVectorFactory; // Needed when loading
        const unsigned num_cells = r_cells_distributed.size();
        r_archive & num_cells;
        for (unsigned i=0; i<num_cells; i++)
        {
            AbstractDynamicallyLoadableEntity* p_entity = dynamic_cast<AbstractDynamicallyLoadableEntity*>(r_cells_distributed[i]);
            bool is_dynamic = (p_entity != NULL);
            r_archive & is_dynamic;
            if (is_dynamic)
            {
#ifdef CHASTE_CAN_CHECKPOINT_DLLS
                r_archive & p_entity->GetLoader()->GetLoadableModulePath();
#else
                // We should have thrown an exception before this point
                NEVER_REACHED;
#endif // CHASTE_CAN_CHECKPOINT_DLLS
            }
            r_archive & r_cells_distributed[i];
        }
    }

    /**
     * Load our collection of cells from an archive.
     *
     * Handles the checkpoint migration case, deleting loaded cells immediately if they are
     * not local to this process.
     *
     * @param archive  the process-specific archive to load from
     * @param version  archive version
     * @param rCells  vector to fill in with pointers to local cells
     * @param pMesh  the mesh, so we can get at the node permutation, if any
     */
    template<class Archive>
    static void LoadCardiacCells(Archive & archive, const unsigned int version,
                                 std::vector<AbstractCardiacCell*>& rCells,
                                 AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh)
    {
        DistributedVectorFactory* p_factory;
        archive & p_factory;
        unsigned num_cells;
        archive & num_cells;
        rCells.resize(p_factory->GetLocalOwnership());
#ifndef NDEBUG
        // Paranoia
        for (unsigned i=0; i<rCells.size(); i++)
        {
            assert(rCells[i] == NULL);
        }
#endif

        // We don't store a cell index in the archive, so need to work out what global
        // index this collection of cells starts up.  If we're migrating (so have an
        // original factory) we use the original low index; otherwise we use the current
        // low index.
        unsigned index_low = p_factory->GetOriginalFactory() ? p_factory->GetOriginalFactory()->GetLow() : p_factory->GetLow();

        // Track fake bath cells to make sure we only delete non-local ones
        std::set<FakeBathCell*> fake_bath_cells_non_local, fake_bath_cells_local;

        for (unsigned local_index=0; local_index<num_cells; local_index++)
        {
            // If we're permuting, figure out where this cell goes
            unsigned original_global_index = index_low + local_index;
            const std::vector<unsigned>& r_permutation = pMesh->rGetNodePermutation();
            unsigned new_global_index;
            if (r_permutation.empty())
            {
                new_global_index = original_global_index;
            }
            else
            {
                ///\todo #1199 test this
//                new_global_index = r_permutation[original_global_index];
                NEVER_REACHED;
            }
            unsigned new_local_index = new_global_index - p_factory->GetLow();
            bool local = p_factory->IsGlobalIndexLocal(new_global_index);

            bool is_dynamic;
            archive & is_dynamic;
            if (is_dynamic)
            {
#ifdef CHASTE_CAN_CHECKPOINT_DLLS
                // Ensure the shared object file for this cell model is loaded.
                // We need to do this here, rather than in the class' serialization code,
                // because that code won't be available until this is done...
                std::string shared_object_path;
                archive & shared_object_path;
                DynamicModelLoaderRegistry::Instance()->GetLoader(shared_object_path);
#else
                // Since checkpoints with dynamically loadable cells can only be
                // created on Boost>=1.37, trying to load such a checkpoint on an
                // earlier Boost would give an error when first opening the archive.
                NEVER_REACHED;
#endif // CHASTE_CAN_CHECKPOINT_DLLS
            }
            AbstractCardiacCell* p_cell;
            archive & p_cell;
            // Check if it's a fake cell
            FakeBathCell* p_fake = dynamic_cast<FakeBathCell*>(p_cell);
            if (local)
            {
                rCells[new_local_index] = p_cell; // Add to local cells collection
                if (p_fake)
                {
                    fake_bath_cells_local.insert(p_fake);
                }
            }
            else
            {
                if (p_fake)
                {
                    fake_bath_cells_non_local.insert(p_fake);
                }
                else
                {
                    // Non-local real cell, so free the memory.
                    delete p_cell;
                }
            }
        }

        // Delete any unused fake cells
        if (!fake_bath_cells_non_local.empty())
        {
            for (std::set<FakeBathCell*>::iterator it = fake_bath_cells_non_local.begin();
                 it != fake_bath_cells_non_local.end();
                 ++it)
            {
                if (fake_bath_cells_local.find(*it) == fake_bath_cells_local.end())
                {
                    delete (*it);
                }
            }
        }
    }
};

TEMPLATED_CLASS_IS_ABSTRACT_2_UNSIGNED(AbstractCardiacCellCollection)

namespace boost { 
namespace serialization {
/**
 * Specify a version number for archive backwards compatibility.
 * 
 * This is how to do BOOST_CLASS_VERSION(AbstractCardiacCellCollection, 1)
 * with a templated class.
 */
template <unsigned ELEMENT_DIM, unsigned SPACE_DIM>
struct version<AbstractCardiacCellCollection<ELEMENT_DIM, SPACE_DIM> >
{
    /** Version number */
    BOOST_STATIC_CONSTANT(unsigned, value = 1);
};
} // namespace serialization
} // namespace boost

#endif /*ABSTRACTCARDIACCELLCOLLECTION_HPP_*/
