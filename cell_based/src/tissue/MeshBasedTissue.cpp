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
#include "MeshBasedTissue.hpp"
#include "TrianglesMeshWriter.hpp"
#include "CellBasedEventHandler.hpp"

template<unsigned DIM>
MeshBasedTissue<DIM>::MeshBasedTissue(MutableMesh<DIM, DIM>& rMesh,
                                      std::vector<TissueCell>& rCells,
                                      const std::vector<unsigned> locationIndices,
                                      bool deleteMesh,
                                      bool validate)
    : AbstractCellCentreBasedTissue<DIM>(rCells, locationIndices),
      mrMesh(rMesh),
      mpVoronoiTessellation(NULL),
      mDeleteMesh(deleteMesh),
      mUseAreaBasedDampingConstant(false)
{
    // This must always be true
    assert(this->mCells.size() <= mrMesh.GetNumNodes());

    this->mTissueContainsMesh = true;

    if (validate)
    {
        Validate();
    }
}

template<unsigned DIM>
MeshBasedTissue<DIM>::MeshBasedTissue(MutableMesh<DIM, DIM>& rMesh)
             : mrMesh(rMesh)
{
    this->mTissueContainsMesh = true;
    mpVoronoiTessellation = NULL;
    mDeleteMesh = true;
}

template<unsigned DIM>
MeshBasedTissue<DIM>::~MeshBasedTissue()
{
    delete mpVoronoiTessellation;
    if (mDeleteMesh)
    {
        delete &mrMesh;
    }
}

template<unsigned DIM>
bool MeshBasedTissue<DIM>::UseAreaBasedDampingConstant()
{
    return mUseAreaBasedDampingConstant;
}

template<unsigned DIM>
void MeshBasedTissue<DIM>::SetAreaBasedDampingConstant(bool useAreaBasedDampingConstant)
{
    assert(DIM==2);
    mUseAreaBasedDampingConstant = useAreaBasedDampingConstant;
}

template<unsigned DIM>
unsigned MeshBasedTissue<DIM>::AddNode(Node<DIM>* pNewNode)
{
    return mrMesh.AddNode(pNewNode);
}

template<unsigned DIM>
void MeshBasedTissue<DIM>::SetNode(unsigned nodeIndex, ChastePoint<DIM>& rNewLocation)
{
    mrMesh.SetNode(nodeIndex, rNewLocation, false);
}

template<unsigned DIM>
double MeshBasedTissue<DIM>::GetDampingConstant(unsigned nodeIndex)
{
    double damping_multiplier = AbstractCellCentreBasedTissue<DIM>::GetDampingConstant(nodeIndex);

    if (mUseAreaBasedDampingConstant)
    {
        /**
         * We use a linear dependence of the form
         *
         * new_damping_const = old_damping_const * (d0+d1*A)
         *
         * where d0, d1 are parameters, A is the cell's area, and old_damping_const
         * is the damping constant if not using mUseAreaBasedDampingConstant
         */

        assert(DIM==2);

        double rest_length = 1.0;
        double d0 = TissueConfig::Instance()->GetAreaBasedDampingConstantParameter();

        /**
         * Compute the parameter d1 such that d0+A*d1=1, where A is the equilibrium area
         * of a cell (this is equal to sqrt(3)/4, which is a third of the area of a regular
         * hexagon of edge length 1)
         */
        double d1 = 2.0*(1.0 - d0)/(sqrt(3)*rest_length*rest_length);

        double area_cell = GetAreaOfVoronoiElement(nodeIndex);

        /**
         * The cell area should not be too large - the next assertion is to avoid
         * getting an infinite cell area, which may occur if area-based viscosity
         * is chosen in the absence of ghost nodes.
         */
        assert(area_cell < 1000);

        damping_multiplier = d0 + area_cell*d1;
    }

    return damping_multiplier;
}

template<unsigned DIM>
void MeshBasedTissue<DIM>::Validate()
{
    std::vector<bool> validated_node = std::vector<bool>(this->GetNumNodes(), false);

    for (typename AbstractTissue<DIM>::Iterator cell_iter=this->Begin(); cell_iter!=this->End(); ++cell_iter)
    {
        unsigned node_index = GetLocationIndexUsingCell(*cell_iter);
        validated_node[node_index] = true;
    }

    for (unsigned i=0; i<validated_node.size(); i++)
    {
        if (!validated_node[i])
        {
            std::stringstream ss;
            ss << "Node " << i << " does not appear to have a cell associated with it";
            EXCEPTION(ss.str());
        }
    }
}

template<unsigned DIM>
MutableMesh<DIM, DIM>& MeshBasedTissue<DIM>::rGetMesh()
{
    return mrMesh;
}

template<unsigned DIM>
const MutableMesh<DIM, DIM>& MeshBasedTissue<DIM>::rGetMesh() const
{
    return mrMesh;
}

template<unsigned DIM>
unsigned MeshBasedTissue<DIM>::RemoveDeadCells()
{
    unsigned num_removed = 0;
    for (std::list<TissueCell>::iterator it = this->mCells.begin();
         it != this->mCells.end();
         ++it)
    {
        if (it->IsDead())
        {
            // Check if this cell is in a marked spring
            std::vector<const std::set<TissueCell*>*> pairs_to_remove; // Pairs that must be purged
            for (std::set<std::set<TissueCell*> >::iterator it1 = mMarkedSprings.begin();
                 it1 != mMarkedSprings.end();
                 ++it1)
            {
                const std::set<TissueCell*>& r_pair = *it1;
                for (std::set<TissueCell*>::iterator it2 = r_pair.begin();
                     it2 != r_pair.end();
                     ++it2)
                {
                    if (*it2 == &(*it))
                    {
                        // Remember to purge this spring
                        pairs_to_remove.push_back(&r_pair);
                        break;
                    }
                }
            }
            // Purge any marked springs that contained this cell
            for (std::vector<const std::set<TissueCell*>* >::iterator pair_it = pairs_to_remove.begin();
                 pair_it != pairs_to_remove.end();
                 ++pair_it)
            {
                mMarkedSprings.erase(**pair_it);
            }

            // Remove the node from the mesh
            num_removed++;
            mrMesh.DeleteNodePriorToReMesh(this->mCellLocationMap[&(*it)]);

            // Update mappings between cells and location indices
            unsigned location_index_of_removed_node = this->mCellLocationMap[&(*it)];
            this->mCellLocationMap.erase(&(*it));
            this->mLocationCellMap.erase(location_index_of_removed_node);

            // Update vector of cells
            it = this->mCells.erase(it);
            --it;
        }
    }

    return num_removed;
}


template<unsigned DIM>
void MeshBasedTissue<DIM>::Update(bool hasHadBirthsOrDeaths)
{
    NodeMap map(mrMesh.GetNumAllNodes());
    mrMesh.ReMesh(map);

    if (!map.IsIdentityMap())
    {
        UpdateGhostNodesAfterReMesh(map);

        // Update the mappings between cells and location indices
        std::map<TissueCell*, unsigned> old_map = this->mCellLocationMap;

        // Remove any dead pointers from the maps (needed to avoid archiving errors)
        this->mLocationCellMap.clear();
        this->mCellLocationMap.clear();

        for (std::list<TissueCell>::iterator it = this->mCells.begin();
             it != this->mCells.end();
             ++it)
        {
            unsigned old_node_index = old_map[&(*it)];

            // This shouldn't ever happen, as the cell vector only contains living cells
            assert(!map.IsDeleted(old_node_index));

            unsigned new_node_index = map.GetNewIndex(old_node_index);
            this->mLocationCellMap[new_node_index] = &(*it);
            this->mCellLocationMap[&(*it)] = new_node_index;
        }
    }

    // Purge any marked springs that are no longer springs
    std::vector<const std::set<TissueCell*>*> springs_to_remove;
    for (std::set<std::set<TissueCell*> >::iterator spring_it = mMarkedSprings.begin();
         spring_it != mMarkedSprings.end();
         ++spring_it)
    {
        const std::set<TissueCell*>& r_pair = *spring_it;
        assert(r_pair.size() == 2);
        TissueCell* p_cell_1 = *(r_pair.begin());
        TissueCell* p_cell_2 = *(++r_pair.begin());
        Node<DIM>* p_node_1 = this->GetNodeCorrespondingToCell(*p_cell_1);
        Node<DIM>* p_node_2 = this->GetNodeCorrespondingToCell(*p_cell_2);

        bool joined = false;

        // For each element containing node1, if it also contains node2 then the cells are joined
        std::set<unsigned> node2_elements = p_node_2->rGetContainingElementIndices();
        for (typename Node<DIM>::ContainingElementIterator elem_iter = p_node_1->ContainingElementsBegin();
             elem_iter != p_node_1->ContainingElementsEnd();
             ++elem_iter)
        {
            if (node2_elements.find(*elem_iter) != node2_elements.end())
            {
                joined = true;
                break;
            }
        }

        // If no longer joined, remove this spring from the set
        if (!joined)
        {
            springs_to_remove.push_back(&r_pair);
        }
    }

    // Remove any springs necessary
    for (std::vector<const std::set<TissueCell*>* >::iterator spring_it = springs_to_remove.begin();
         spring_it != springs_to_remove.end();
         ++spring_it)
    {
        mMarkedSprings.erase(**spring_it);
    }

    this->Validate();

    // Tessellate if needed
    if (DIM==2 || DIM==3)
    {
        CellBasedEventHandler::BeginEvent(CellBasedEventHandler::TESSELLATION);
        if (mUseAreaBasedDampingConstant || TissueConfig::Instance()->GetOutputVoronoiData() ||
            TissueConfig::Instance()->GetOutputTissueVolumes() || TissueConfig::Instance()->GetOutputCellVolumes() )
        {
            std::vector<unsigned> location_indices;
            for (typename AbstractTissue<DIM>::Iterator cell_iter = this->Begin();
                 cell_iter != this->End();
                 ++cell_iter)
            {
                unsigned node_index = this->mCellLocationMap[&(*cell_iter)];
                location_indices.push_back(node_index);
            }
            CreateVoronoiTessellation(location_indices);
        }
        CellBasedEventHandler::EndEvent(CellBasedEventHandler::TESSELLATION);
    }
}

template<unsigned DIM>
Node<DIM>* MeshBasedTissue<DIM>::GetNode(unsigned index)
{
    return mrMesh.GetNode(index);
}

template<unsigned DIM>
unsigned MeshBasedTissue<DIM>::GetNumNodes()
{
    return mrMesh.GetNumAllNodes();
}

template<unsigned DIM>
void MeshBasedTissue<DIM>::UpdateGhostNodesAfterReMesh(NodeMap& rMap)
{
}

template<unsigned DIM>
TissueCell* MeshBasedTissue<DIM>::AddCell(TissueCell& rNewCell, const c_vector<double,DIM>& rCellDivisionVector, TissueCell* pParentCell)
{
    // Add new cell to tissue
    TissueCell* p_created_cell = AbstractCellCentreBasedTissue<DIM>::AddCell(rNewCell, rCellDivisionVector, pParentCell);

    // Mark spring between parent cell and new cell
    std::set<TissueCell*> cell_pair = CreateCellPair(*pParentCell, *p_created_cell);
    MarkSpring(cell_pair);

    // Return pointer to new cell
    return p_created_cell;
}

//////////////////////////////////////////////////////////////////////////////
//                             Output methods                               //
//////////////////////////////////////////////////////////////////////////////


template<unsigned DIM>
void MeshBasedTissue<DIM>::CreateOutputFiles(const std::string& rDirectory, bool cleanOutputDirectory)
{
    AbstractTissue<DIM>::CreateOutputFiles(rDirectory, cleanOutputDirectory);

    OutputFileHandler output_file_handler(rDirectory, cleanOutputDirectory);
    mpVizElementsFile = output_file_handler.OpenOutputFile("results.vizelements");

    if (TissueConfig::Instance()->GetOutputVoronoiData())
    {
        mpVoronoiFile = output_file_handler.OpenOutputFile("voronoi.dat");
    }
    if (TissueConfig::Instance()->GetOutputTissueVolumes())
    {
        mpTissueVolumesFile = output_file_handler.OpenOutputFile("tissueareas.dat");
    }
    if (TissueConfig::Instance()->GetOutputCellVolumes())
    {
        mpCellVolumesFile = output_file_handler.OpenOutputFile("cellareas.dat");
    }
    mDirPath = rDirectory;
#ifdef CHASTE_VTK
    mpVtkMetaFile = output_file_handler.OpenOutputFile("results.pvd");
    *mpVtkMetaFile << "<?xml version=\"1.0\"?>\n";
    *mpVtkMetaFile << "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\" compressor=\"vtkZLibDataCompressor\">\n";
    *mpVtkMetaFile << "    <Collection>\n";
#endif //CHASTE_VTK
}

template<unsigned DIM>
void MeshBasedTissue<DIM>::CloseOutputFiles()
{
    AbstractTissue<DIM>::CloseOutputFiles();

    mpVizElementsFile->close();

    if (TissueConfig::Instance()->GetOutputVoronoiData())
    {
        mpVoronoiFile->close();
    }
    if (TissueConfig::Instance()->GetOutputTissueVolumes())
    {
        mpTissueVolumesFile->close();
    }
    if (TissueConfig::Instance()->GetOutputCellVolumes())
    {
        mpCellVolumesFile->close();
    }
#ifdef CHASTE_VTK
    *mpVtkMetaFile << "    </Collection>\n";
    *mpVtkMetaFile << "</VTKFile>\n";
    mpVtkMetaFile->close();
#endif //CHASTE_VTK
}

template<unsigned DIM>
void MeshBasedTissue<DIM>::WriteResultsToFiles()
{
    AbstractCellCentreBasedTissue<DIM>::WriteResultsToFiles();

    // Write element data to file

    *mpVizElementsFile << SimulationTime::Instance()->GetTime() << "\t";

    for (typename MutableMesh<DIM,DIM>::ElementIterator elem_iter = mrMesh.GetElementIteratorBegin();
         elem_iter != mrMesh.GetElementIteratorEnd();
         ++elem_iter)
    {
        bool element_contains_dead_cells_or_deleted_nodes = false;

        // Hack that covers the case where the element contains a node that is associated with a cell that has just been killed (#1129)
        for (unsigned i=0; i<DIM+1; i++)
        {
            unsigned node_index = elem_iter->GetNodeGlobalIndex(i);

            if (this->GetNode(node_index)->IsDeleted())
            {
                element_contains_dead_cells_or_deleted_nodes = true;
                break;
            }
            else if (this->mLocationCellMap[node_index])
            {
                if (this->mLocationCellMap[node_index]->IsDead())
                {
                    element_contains_dead_cells_or_deleted_nodes = true;
                    break;
                }
            }
        }
        if (!element_contains_dead_cells_or_deleted_nodes)
        {
            for (unsigned i=0; i<DIM+1; i++)
            {
                *mpVizElementsFile << elem_iter->GetNodeGlobalIndex(i) << " ";
            }
        }
    }
    *mpVizElementsFile << "\n";

    if (mpVoronoiTessellation!=NULL)
    {
        if (TissueConfig::Instance()->GetOutputVoronoiData())
        {
            WriteVoronoiResultsToFile();
        }
        if (TissueConfig::Instance()->GetOutputTissueVolumes())
        {
            WriteTissueVolumeResultsToFile();
        }
        if (TissueConfig::Instance()->GetOutputCellVolumes())
        {
            WriteCellVolumeResultsToFile();
        }
#ifdef CHASTE_VTK
    VertexMeshWriter<DIM, DIM> mesh_writer(mDirPath, "results", false);
    std::stringstream time;
    time << SimulationTime::Instance()->GetTimeStepsElapsed();

    std::vector<double> cell_types;
    std::vector<double> cell_ancestors;
    std::vector<double> cell_mutation_states;
    std::vector<double> cell_ages;
    std::vector<double> cell_cycle_phases;
    std::vector<double> cell_areas;

    // Loop over Voronoi elements
    for (typename VertexMesh<DIM,DIM>::VertexElementIterator elem_iter = mpVoronoiTessellation->GetElementIteratorBegin();
         elem_iter != mpVoronoiTessellation->GetElementIteratorEnd();
         ++elem_iter)
    {
        // Get index of this element in the Voronoi tessellation mesh
        unsigned elem_index = elem_iter->GetIndex();

        // Get the cell corresponding to this element
        TissueCell* p_cell = this->mLocationCellMap[elem_index];

        if (TissueConfig::Instance()->GetOutputCellAncestors())
        {
            double ancestor_index = (p_cell->GetAncestor() == UNSIGNED_UNSET) ? (-1.0) : (double)p_cell->GetAncestor();
            cell_ancestors.push_back(ancestor_index);
        }
        if (TissueConfig::Instance()->GetOutputCellProliferativeTypes())
        {
            double cell_type = p_cell->GetCellProliferativeType();
            cell_types.push_back(cell_type);
        }
        if (TissueConfig::Instance()->GetOutputCellMutationStates())
        {
            double mutation_state = p_cell->GetMutationState()->GetColour();
            cell_mutation_states.push_back(mutation_state);
        }
        if (TissueConfig::Instance()->GetOutputCellAges())
        {
            double age = p_cell->GetAge();
            cell_ages.push_back(age); 
        }
        if (TissueConfig::Instance()->GetOutputCellCyclePhases())
        {
            double cycle_phase = p_cell->GetCellCycleModel()->GetCurrentCellCyclePhase();
            cell_cycle_phases.push_back(cycle_phase);
        }
        if (TissueConfig::Instance()->GetOutputCellVolumes())
        {
            double cell_area;
            if (DIM==2)
            {
                cell_area = GetAreaOfVoronoiElement(elem_index);
            }
            else // DIM==3
            {
                NEVER_REACHED;
                ///\todo #8806
                //cell_area = GetVolumeOfVoronoiElement(elem_index);
            }
            cell_areas.push_back(cell_area);
        }
    }

    if (TissueConfig::Instance()->GetOutputCellProliferativeTypes())
    {
        mesh_writer.AddCellData("Cell types", cell_types);
    }
    if (TissueConfig::Instance()->GetOutputCellAncestors())
    {
        mesh_writer.AddCellData("Ancestors", cell_ancestors);
    }
    if (TissueConfig::Instance()->GetOutputCellMutationStates())
    {
        mesh_writer.AddCellData("Mutation states", cell_mutation_states);
    }
    if (TissueConfig::Instance()->GetOutputCellAges())
    {
        mesh_writer.AddCellData("Ages", cell_ages);
    }
    if (TissueConfig::Instance()->GetOutputCellCyclePhases())
    {
        mesh_writer.AddCellData("Cycle phases", cell_cycle_phases);
    }
    if (TissueConfig::Instance()->GetOutputCellVolumes())
    {
        mesh_writer.AddCellData("Cell areas", cell_areas);
    }

    mesh_writer.WriteVtkUsingMesh(*mpVoronoiTessellation, time.str());
    *mpVtkMetaFile << "        <DataSet timestep=\"";
    *mpVtkMetaFile << SimulationTime::Instance()->GetTimeStepsElapsed();
    *mpVtkMetaFile << "\" group=\"\" part=\"0\" file=\"results_";
    *mpVtkMetaFile << SimulationTime::Instance()->GetTimeStepsElapsed();
    *mpVtkMetaFile << ".vtu\"/>\n";
#endif //CHASTE_VTK
    }
}

template<unsigned DIM>
void MeshBasedTissue<DIM>::WriteVoronoiResultsToFile()
{
    assert(DIM==2 || DIM==3);

    // Write time to file
    *mpVoronoiFile << SimulationTime::Instance()->GetTime() << " ";

    // Loop over elements of the Voronoi mesh
    for (typename VertexMesh<DIM,DIM>::VertexElementIterator elem_iter = mpVoronoiTessellation->GetElementIteratorBegin();
         elem_iter != mpVoronoiTessellation->GetElementIteratorEnd();
         ++elem_iter)
    {
        // Get index of this element in the Voronoi tessellation mesh and write to file
        unsigned elem_index = elem_iter->GetIndex();
        *mpVoronoiFile << elem_index << " ";
 
        // Write node location to file
        c_vector<double, DIM> node_location = this->GetNode(elem_index)->rGetLocation();
        for (unsigned i=0; i<DIM; i++)
        {
            *mpVoronoiFile << node_location[i] << " ";
        }

        // Get cell volume and surface area (in 3D) or area and perimeter (in 2D) and write to file
        if (DIM==2)
        {
            double cell_area = GetAreaOfVoronoiElement(elem_index);
            double cell_perimeter = GetPerimeterOfVoronoiElement(elem_index);
            *mpVoronoiFile << cell_area << " " << cell_perimeter << " ";
        }
        else // DIM==3
        {
            NEVER_REACHED;
            ///\todo #8806
            //double cell_volume = GetVolumeOfVoronoiElement(elem_index);
            //double cell_surface_area = GetSurfaceAreaOfVoronoiElement(elem_index);
            //*mpVoronoiFile << cell_volume << " " << cell_surface_area << " ";
        }
    }
    *mpVoronoiFile << "\n";
}

template<unsigned DIM>
void MeshBasedTissue<DIM>::WriteTissueVolumeResultsToFile()
{
    assert(DIM==2 || DIM==3);

    // Write time to file
    *mpTissueVolumesFile << SimulationTime::Instance()->GetTime() << " ";

    // Don't use the Voronoi tessellation to calculate the total area
    // because it gives huge areas for boundary cells
    double total_area = mrMesh.GetVolume();
    double apoptotic_area = 0.0;

    // Loop over elements of the Voronoi mesh
    for (typename VertexMesh<DIM,DIM>::VertexElementIterator elem_iter = mpVoronoiTessellation->GetElementIteratorBegin();
         elem_iter != mpVoronoiTessellation->GetElementIteratorEnd();
         ++elem_iter)
    {
        // Get index of this element in the Voronoi tessellation mesh and write to file
        unsigned elem_index = elem_iter->GetIndex();

        // Get the cell corresponding to this node
        TissueCell* p_cell =  this->mLocationCellMap[elem_index];

        // Only bother calculating the cell area if it is apoptotic
        if (p_cell->GetCellProliferativeType() == APOPTOTIC)
        {
            if (DIM==2)
            {
                double cell_area = GetAreaOfVoronoiElement(elem_index);
                apoptotic_area += cell_area;
            }
            else // DIM==3
            {
                NEVER_REACHED;
                ///\todo #8806
                //double cell_volume = GetVolumeOfVoronoiElement(elem_index);
                //apoptotic_area += cell_volume;
            }
        }
    }
    *mpTissueVolumesFile << total_area << " " << apoptotic_area << "\n";
}

template<unsigned DIM>
void MeshBasedTissue<DIM>::WriteCellVolumeResultsToFile()
{
    assert(DIM==2 || DIM==3);

     // Write time to file
    *mpCellVolumesFile << SimulationTime::Instance()->GetTime() << " ";
 
    // Loop over elements of the Voronoi mesh
    for (typename VertexMesh<DIM,DIM>::VertexElementIterator elem_iter = mpVoronoiTessellation->GetElementIteratorBegin();
         elem_iter != mpVoronoiTessellation->GetElementIteratorEnd();
         ++elem_iter)
    {
        // Get index of this element in the Voronoi tessellation mesh and write to file
        unsigned elem_index = elem_iter->GetIndex();
        *mpCellVolumesFile << elem_index << " ";

        // Get the cell corresponding to this node
        TissueCell* p_cell =  this->mLocationCellMap[elem_index];

        // Write cell ID to file
        unsigned cell_index = p_cell->GetCellId();
        *mpCellVolumesFile << cell_index << " ";

        // Write node location to file
        c_vector<double, DIM> node_location = this->GetNode(elem_index)->rGetLocation();
        for (unsigned i=0; i<DIM; i++)
        {
            *mpCellVolumesFile << node_location[i] << " ";
        }

        // Write cell volume (in 3D) or area (in 2D) to file
        if (DIM==2)
        {
            double cell_area = GetAreaOfVoronoiElement(elem_index);
            *mpCellVolumesFile << cell_area << " ";
        }
        else // DIM==3
        {
            NEVER_REACHED;
            ///\todo #8806
            //double cell_volume = GetVolumeOfVoronoiElement(elem_index);
            //*mpCellVolumesFile << cell_volume << " ";
        }
    }
    *mpCellVolumesFile << "\n";
}


//////////////////////////////////////////////////////////////////////////////
//                          Spring iterator class                           //
//////////////////////////////////////////////////////////////////////////////

template<unsigned DIM>
Node<DIM>* MeshBasedTissue<DIM>::SpringIterator::GetNodeA()
{
    return mEdgeIter.GetNodeA();
}

template<unsigned DIM>
Node<DIM>* MeshBasedTissue<DIM>::SpringIterator::GetNodeB()
{
    return mEdgeIter.GetNodeB();
}

template<unsigned DIM>
TissueCell& MeshBasedTissue<DIM>::SpringIterator::rGetCellA()
{
    assert((*this) != mrTissue.SpringsEnd());
    return mrTissue.rGetCellUsingLocationIndex(mEdgeIter.GetNodeA()->GetIndex());
}

template<unsigned DIM>
TissueCell& MeshBasedTissue<DIM>::SpringIterator::rGetCellB()
{
    assert((*this) != mrTissue.SpringsEnd());
    return mrTissue.rGetCellUsingLocationIndex(mEdgeIter.GetNodeB()->GetIndex());
}

template<unsigned DIM>
bool MeshBasedTissue<DIM>::SpringIterator::operator!=(const MeshBasedTissue<DIM>::SpringIterator& rOther)
{
    return (mEdgeIter != rOther.mEdgeIter);
}

template<unsigned DIM>
typename MeshBasedTissue<DIM>::SpringIterator& MeshBasedTissue<DIM>::SpringIterator::operator++()
{
    bool edge_is_ghost = false;

    do
    {
        ++mEdgeIter;
        if (*this != mrTissue.SpringsEnd())
        {
            bool a_is_ghost = mrTissue.IsGhostNode(mEdgeIter.GetNodeA()->GetIndex());
            bool b_is_ghost = mrTissue.IsGhostNode(mEdgeIter.GetNodeB()->GetIndex());

            edge_is_ghost = (a_is_ghost || b_is_ghost);
        }
    }
    while (*this!=mrTissue.SpringsEnd() && edge_is_ghost);

    return (*this);
}

template<unsigned DIM>
MeshBasedTissue<DIM>::SpringIterator::SpringIterator(
            MeshBasedTissue<DIM>& rTissue,
            typename MutableMesh<DIM,DIM>::EdgeIterator edgeIter)
    : mrTissue(rTissue),
      mEdgeIter(edgeIter)
{
    if (mEdgeIter!=mrTissue.mrMesh.EdgesEnd())
    {
        bool a_is_ghost = mrTissue.IsGhostNode(mEdgeIter.GetNodeA()->GetIndex());
        bool b_is_ghost = mrTissue.IsGhostNode(mEdgeIter.GetNodeB()->GetIndex());

        if (a_is_ghost || b_is_ghost)
        {
            ++(*this);
        }
    }
}

template<unsigned DIM>
typename MeshBasedTissue<DIM>::SpringIterator MeshBasedTissue<DIM>::SpringsBegin()
{
    return SpringIterator(*this, mrMesh.EdgesBegin());
}

template<unsigned DIM>
typename MeshBasedTissue<DIM>::SpringIterator MeshBasedTissue<DIM>::SpringsEnd()
{
    return SpringIterator(*this, mrMesh.EdgesEnd());
}

template<unsigned DIM>
void MeshBasedTissue<DIM>::CreateVoronoiTessellation(const std::vector<unsigned> locationIndices)
{
    delete mpVoronoiTessellation;
    mpVoronoiTessellation = new VertexMesh<DIM, DIM>(mrMesh, locationIndices);
}

/**
 * The VoronoiTessellation class is only defined in 2D or 3D, hence there
 * are two definitions to this method (one templated and one not).
 */
template<>
void MeshBasedTissue<1>::CreateVoronoiTessellation(const std::vector<unsigned> locationIndices)
{
    // No 1D Voronoi tessellation
    NEVER_REACHED;
}

template<unsigned DIM>
VertexMesh<DIM, DIM>& MeshBasedTissue<DIM>::rGetVoronoiTessellation()
{
    assert(mpVoronoiTessellation!=NULL);
    return *mpVoronoiTessellation;
}

template<unsigned DIM>
double MeshBasedTissue<DIM>::GetAreaOfVoronoiElement(unsigned index)
{
    unsigned element_index = mpVoronoiTessellation->GetVoronoiElementIndexCorrespondingToDelaunayNodeIndex(index);
    double area = mpVoronoiTessellation->GetAreaOfElement(element_index);
    return area;
}

template<unsigned DIM>
double MeshBasedTissue<DIM>::GetPerimeterOfVoronoiElement(unsigned index)
{
    unsigned element_index = mpVoronoiTessellation->GetVoronoiElementIndexCorrespondingToDelaunayNodeIndex(index);
    double perimeter = mpVoronoiTessellation->GetPerimeterOfElement(element_index);
    return perimeter;
}

template<unsigned DIM>
double MeshBasedTissue<DIM>::GetVolumeOfVoronoiElement(unsigned index)
{
    unsigned element_index = mpVoronoiTessellation->GetVoronoiElementIndexCorrespondingToDelaunayNodeIndex(index);
    double volume = mpVoronoiTessellation->GetVolumeOfElement(element_index);
    return volume;
}

template<unsigned DIM>
double MeshBasedTissue<DIM>::GetSurfaceAreaOfVoronoiElement(unsigned index)
{
    unsigned element_index = mpVoronoiTessellation->GetVoronoiElementIndexCorrespondingToDelaunayNodeIndex(index);
    double surface_area = mpVoronoiTessellation->GetSurfaceAreaOfElement(element_index);
    return surface_area;
}

template<unsigned DIM>
double MeshBasedTissue<DIM>::GetVoronoiEdgeLength(unsigned index1, unsigned index2)
{
    unsigned element_index1 = mpVoronoiTessellation->GetVoronoiElementIndexCorrespondingToDelaunayNodeIndex(index1);
    unsigned element_index2 = mpVoronoiTessellation->GetVoronoiElementIndexCorrespondingToDelaunayNodeIndex(index2);
    double edge_length = mpVoronoiTessellation->GetEdgeLength(element_index1, element_index2);
    return edge_length;
}

template<unsigned DIM>
void MeshBasedTissue<DIM>::CheckTissueCellPointers()
{
    bool res = true;
    for (std::list<TissueCell>::iterator it=this->mCells.begin();
         it!=this->mCells.end();
         ++it)
    {
        TissueCell* p_cell = &(*it);
        assert(p_cell);
        AbstractCellCycleModel* p_model = p_cell->GetCellCycleModel();
        assert(p_model);

        // Check cell exists in tissue
        unsigned node_index = this->mCellLocationMap[p_cell];
        std::cout << "Cell at node " << node_index << " addr " << p_cell << std::endl << std::flush;
        TissueCell& r_cell = this->rGetCellUsingLocationIndex(node_index);
#define COVERAGE_IGNORE //Debugging code.  Shouldn't fail under normal conditions
        if (&r_cell != p_cell)
        {
            std::cout << "  Mismatch with tissue" << std::endl << std::flush;
            res = false;
        }

        // Check model links back to cell
        if (p_model->GetCell() != p_cell)
        {
            std::cout << "  Mismatch with cycle model" << std::endl << std::flush;
            res = false;
        }
    }
    assert(res);
#undef COVERAGE_IGNORE

    res = true;
    for (std::set<std::set<TissueCell*> >::iterator it1 = mMarkedSprings.begin();
         it1 != mMarkedSprings.end();
         ++it1)
    {
        const std::set<TissueCell*>& r_pair = *it1;
        assert(r_pair.size() == 2);
        for (std::set<TissueCell*>::iterator it2 = r_pair.begin();
             it2 != r_pair.end();
             ++it2)
        {
            TissueCell* p_cell = *it2;
            assert(p_cell);
            AbstractCellCycleModel* p_model = p_cell->GetCellCycleModel();
            assert(p_model);
            unsigned node_index = this->mCellLocationMap[p_cell];
            std::cout << "Cell at node " << node_index << " addr " << p_cell << std::endl << std::flush;

#define COVERAGE_IGNORE //Debugging code.  Shouldn't fail under normal conditions
            // Check cell is alive
            if (p_cell->IsDead())
            {
                std::cout << "  Cell is dead" << std::endl << std::flush;
                res = false;
            }

            // Check cell exists in tissue
            TissueCell& r_cell = this->rGetCellUsingLocationIndex(node_index);
            if (&r_cell != p_cell)
            {
                std::cout << "  Mismatch with tissue" << std::endl << std::flush;
                res = false;
            }

            // Check model links back to cell
            if (p_model->GetCell() != p_cell)
            {
                std::cout << "  Mismatch with cycle model" << std::endl << std::flush;
                res = false;
            }
        }
#undef COVERAGE_IGNORE
    }
    assert(res);
}

template<unsigned DIM>
std::set<TissueCell*> MeshBasedTissue<DIM>::CreateCellPair(TissueCell& rCell1, TissueCell& rCell2)
{
    std::set<TissueCell *> cell_pair;
    cell_pair.insert(&rCell1);
    cell_pair.insert(&rCell2);
    return cell_pair;
}

template<unsigned DIM>
bool MeshBasedTissue<DIM>::IsMarkedSpring(const std::set<TissueCell*>& rCellPair)
{
    return mMarkedSprings.find(rCellPair) != mMarkedSprings.end();
}

template<unsigned DIM>
void MeshBasedTissue<DIM>::MarkSpring(std::set<TissueCell*>& rCellPair)
{
    mMarkedSprings.insert(rCellPair);
}

template<unsigned DIM>
void MeshBasedTissue<DIM>::UnmarkSpring(std::set<TissueCell*>& rCellPair)
{
    mMarkedSprings.erase(rCellPair);
}


/////////////////////////////////////////////////////////////////////////////
// Explicit instantiation
/////////////////////////////////////////////////////////////////////////////


template class MeshBasedTissue<1>;
template class MeshBasedTissue<2>;
template class MeshBasedTissue<3>;


// Serialization for Boost >= 1.36
#include "SerializationExportWrapperForCpp.hpp"
EXPORT_TEMPLATE_CLASS_SAME_DIMS(MeshBasedTissue)
