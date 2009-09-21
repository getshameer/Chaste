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

#ifndef TESTPARALLELTETRAHEDRALMESH_HPP_
#define TESTPARALLELTETRAHEDRALMESH_HPP_

#include <cxxtest/TestSuite.h>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "UblasCustomFunctions.hpp"
#include "PetscSetupAndFinalize.hpp"
#include "ParallelTetrahedralMesh.hpp"
#include "TetrahedralMesh.hpp"
#include "TrianglesMeshReader.hpp"
#include "TrianglesMeshWriter.hpp"
#include "PetscTools.hpp"

class TestParallelTetrahedralMesh : public CxxTest::TestSuite
{

public:

    void TestConstructFromMeshReader1D()
    {
        TrianglesMeshReader<1,1> mesh_reader("mesh/test/data/1D_0_to_1_10_elements_with_attributes");

        ParallelTetrahedralMesh<1,1> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);

        // Check we have the right number of nodes & elements
        TS_ASSERT_EQUALS(mesh.GetNumNodes(), 11u);
        TS_ASSERT_EQUALS(mesh.GetNumElements(), 10u);

        TS_ASSERT_EQUALS(mesh_reader.GetNumElementAttributes(), 1u);
        for (unsigned i=0; i<mesh.GetNumElements(); i++)
        {
            try
            {
                unsigned region = mesh.GetElement(i)->GetRegion();
                TS_ASSERT_EQUALS(region, i%5+1);
            }
            catch(Exception& e)
            {
                // I don't own this element do I?
            }
        }
    }

    void TestConstructFromMeshReader2DWithoutReordering()
    {
        /*
         * In this test we don't use reordering since we want to check that a TetrahedralMesh and
         * a ParallelTetrahedralMesh create the same geometry from the same file.
         */
        TrianglesMeshReader<2,2> mesh_reader("mesh/test/data/disk_984_elements");

        ParallelTetrahedralMesh<2,2> mesh(ParallelTetrahedralMesh<2,2>::DUMB); // No reordering
        mesh.ConstructFromMeshReader(mesh_reader);

        // Check we have the right number of nodes & elements
        TS_ASSERT_EQUALS(mesh.GetNumNodes(), 543u);
        TS_ASSERT_EQUALS(mesh.GetDistributedVectorFactory()->GetProblemSize(), 543u);
        TS_ASSERT_EQUALS(mesh.GetNumElements(), 984u);
        TS_ASSERT_EQUALS(mesh.GetNumBoundaryElements(), 100u);

        // For coverage purposes
        mesh.SetElementOwnerships(0,1); // see comment in ParallelTetrahedralMesh

        for (AbstractTetrahedralMesh<2,2>::ElementIterator iter = mesh.GetElementIteratorBegin();
             iter != mesh.GetElementIteratorEnd();
             ++iter)
        {
            TS_ASSERT(iter->GetOwnership());
        }

        // Check the inverse Jacobian
        c_matrix<double, 2, 2> jacobian;
        double jacobian_determinant;
        c_matrix<double, 2, 2> inverse_jacobian;

        c_matrix<double, 2, 2> element_jacobian;
        double element_jacobian_determinant;
        c_matrix<double, 2, 2> element_inverse_jacobian;

        try
        {
            mesh.GetInverseJacobianForElement(0, jacobian, jacobian_determinant, inverse_jacobian);
            mesh.GetElement(0)->CalculateInverseJacobian(element_jacobian, element_jacobian_determinant, element_inverse_jacobian);

            TS_ASSERT_EQUALS(element_jacobian_determinant, jacobian_determinant);

            for (unsigned row=0; row<2; row++)
            {
                for (unsigned col=0; col<2; col++)
                {
                    TS_ASSERT_EQUALS(element_inverse_jacobian(row,col), inverse_jacobian(row,col));
                }
            }
        }
        catch(Exception& e)
        {
            // I don't own this element do I?
        }

        c_vector<double, 2> direction;
        c_vector<double, 2> element_direction;

        try
        {
            mesh.GetWeightedDirectionForBoundaryElement(0, direction, jacobian_determinant);
            mesh.GetBoundaryElement(0)->CalculateWeightedDirection(element_direction, element_jacobian_determinant);

            TS_ASSERT_EQUALS(element_jacobian_determinant, jacobian_determinant);

            for (unsigned row=0; row<2; row++)
            {
                TS_ASSERT_EQUALS(element_direction(row), direction(row));
            }
        }
        catch(Exception& e)
        {
            // I don't own this boundary element do I?
        }

        mesh_reader.Reset();
        TetrahedralMesh<2,2> seq_mesh;
        seq_mesh.ConstructFromMeshReader(mesh_reader);

        for (AbstractTetrahedralMesh<2,2>::ElementIterator iter = mesh.GetElementIteratorBegin();
             iter != mesh.GetElementIteratorEnd();
             ++iter)
        {
            unsigned element_index = iter->GetIndex();

            Element<2,2>* p_sequ_element = seq_mesh.GetElement(element_index);
            TS_ASSERT_EQUALS(element_index, p_sequ_element->GetIndex());

            for (unsigned node_local_index=0; node_local_index < iter->GetNumNodes(); node_local_index++)
            {
                TS_ASSERT_EQUALS(iter->GetNodeGlobalIndex(node_local_index),
                                 p_sequ_element->GetNodeGlobalIndex(node_local_index));

                TS_ASSERT_EQUALS(iter->GetNode(node_local_index)->GetPoint()[0],
                                 p_sequ_element->GetNode(node_local_index)->GetPoint()[0]);
            }
        }

        for (ParallelTetrahedralMesh<2,2>::BoundaryElementIterator it=mesh.GetBoundaryElementIteratorBegin();
             it!=mesh.GetBoundaryElementIteratorEnd();
             ++it)
        {
            BoundaryElement<1,2>* p_para_boundary_element = *it;
            unsigned boundary_element_index = p_para_boundary_element->GetIndex();

            BoundaryElement<1,2>* p_sequ_boundary_element = seq_mesh.GetBoundaryElement(boundary_element_index);
            TS_ASSERT_EQUALS(boundary_element_index, p_sequ_boundary_element->GetIndex());

            for (unsigned node_local_index=0; node_local_index < p_para_boundary_element->GetNumNodes(); node_local_index++)
            {
                TS_ASSERT_EQUALS(p_para_boundary_element->GetNodeGlobalIndex(node_local_index),
                                 p_sequ_boundary_element->GetNodeGlobalIndex(node_local_index));

                TS_ASSERT_EQUALS(p_para_boundary_element->GetNode(node_local_index)->GetPoint()[0],
                                 p_sequ_boundary_element->GetNode(node_local_index)->GetPoint()[0]);
            }
        }
    }

    void TestConstructFromMeshReader3D()
    {
        /*
         * In this test we let METIS reorder the ParallelTetrahedralMesh. We want to check that although
         * the indices of the nodes have changed, the location of the nodes is consistent with a
         * TetrahedralMesh representation of the same mesh.
         */
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ParallelTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);

        // Check we have the right number of nodes & elements
        TS_ASSERT_EQUALS(mesh.GetNumNodes(), 51u);
        TS_ASSERT_EQUALS(mesh.GetNumElements(), 136u);
        TS_ASSERT_EQUALS(mesh.GetNumBoundaryElements(), 96u);

        mesh_reader.Reset();
        TetrahedralMesh<3,3> seq_mesh;
        seq_mesh.ConstructFromMeshReader(mesh_reader);

        for (AbstractTetrahedralMesh<3,3>::ElementIterator iter = mesh.GetElementIteratorBegin();
             iter != mesh.GetElementIteratorEnd();
             ++iter)
        {
            unsigned element_index = iter->GetIndex();

            Element<3,3>* p_sequ_element = seq_mesh.GetElement(element_index);

            // The elements have the same index and the nodes are located in the same position.
            TS_ASSERT_EQUALS(element_index, p_sequ_element->GetIndex());
            for (unsigned node_local_index=0; node_local_index < iter->GetNumNodes(); node_local_index++)
            {
                for (unsigned dim=0; dim<3; dim++)
                {
                    TS_ASSERT_EQUALS(iter->GetNode(node_local_index)->GetPoint()[dim],
                                     p_sequ_element->GetNode(node_local_index)->GetPoint()[dim]);
                }
            }
        }

        for (ParallelTetrahedralMesh<3,3>::BoundaryElementIterator it=mesh.GetBoundaryElementIteratorBegin();
             it!=mesh.GetBoundaryElementIteratorEnd();
             ++it)
        {
            BoundaryElement<2,3>* p_para_boundary_element = *it;
            unsigned boundary_element_index = p_para_boundary_element->GetIndex();

            BoundaryElement<2,3>* p_sequ_boundary_element = seq_mesh.GetBoundaryElement(boundary_element_index);

            // The boundary elements have the same index and the nodes are located in the same position.
            TS_ASSERT_EQUALS(boundary_element_index, p_sequ_boundary_element->GetIndex());
            for (unsigned node_local_index=0; node_local_index < p_para_boundary_element->GetNumNodes(); node_local_index++)
            {
                for (unsigned dim=0; dim<3; dim++)
                {
                    TS_ASSERT_EQUALS(p_para_boundary_element->GetNode(node_local_index)->GetPoint()[dim],
                                     p_sequ_boundary_element->GetNode(node_local_index)->GetPoint()[dim]);
                }
            }
        }
    }

    void TestEverythingIsAssignedMetisLibrary()
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ParallelTetrahedralMesh<3,3> mesh(ParallelTetrahedralMesh<3,3>::METIS_LIBRARY);
        mesh.ConstructFromMeshReader(mesh_reader);

        TS_ASSERT_EQUALS(mesh.GetNumNodes(), mesh_reader.GetNumNodes());
        TS_ASSERT_EQUALS(mesh.GetNumElements(), mesh_reader.GetNumElements());
        TS_ASSERT_EQUALS(mesh.GetNumBoundaryElements(), mesh_reader.GetNumFaces());

        /*
         * All the nodes have been assigned
         */
        {
            unsigned total_nodes_this_process = 0;
            unsigned num_global_nodes = mesh.GetNumNodes();
            unsigned nodes_owned[num_global_nodes];

            for (AbstractTetrahedralMesh<3,3>::NodeIterator iter = mesh.GetNodeIteratorBegin();
                 iter != mesh.GetNodeIteratorEnd();
                 ++iter)
            {
                unsigned node_index = iter->GetIndex();
                nodes_owned[node_index] = 1;

                total_nodes_this_process++;
            }

            TS_ASSERT_EQUALS(mesh.GetNumLocalNodes(), total_nodes_this_process);

            // Combine all the local maps by adding them up in the master process
            unsigned nodes_reduction[num_global_nodes];
            MPI_Reduce(&nodes_owned, &nodes_reduction, num_global_nodes, MPI_UNSIGNED, MPI_SUM, PetscTools::MASTER_RANK, PETSC_COMM_WORLD);

            // Make sure every node is owned at least by one processor
            if (PetscTools::AmMaster())
            {
                for (unsigned node_id=0; node_id<num_global_nodes; node_id++)
                {
                    TS_ASSERT(nodes_reduction[node_id] > 0);
                }
            }
        }

        /*
         * All elements have been assigned
         */
        {
        	unsigned total_elements_this_process = 0;
            unsigned num_global_elements = mesh.GetNumElements();
            unsigned elements_owned[num_global_elements];

            // Create a local map of the elements this processor owns
            for (unsigned element_id=0; element_id<num_global_elements; element_id++)
            {
                try
                {
                    unsigned element_index = mesh.GetElement(element_id)->GetIndex();
                    TS_ASSERT_EQUALS(element_id, element_index);

                    elements_owned[element_index] = 1;

                    total_elements_this_process++;
                }
                catch(Exception& e)
                {
                    elements_owned[element_id] = 0;
                }
            }

            TS_ASSERT_EQUALS(mesh.GetNumLocalElements(), total_elements_this_process);

            // Combine all the local maps by adding them up in the master process
            unsigned elements_reduction[num_global_elements];
            MPI_Reduce(&elements_owned, &elements_reduction, num_global_elements, MPI_UNSIGNED, MPI_SUM, PetscTools::MASTER_RANK, PETSC_COMM_WORLD);

            // Make sure every element is owned at least by one processor
            if (PetscTools::AmMaster())
            {
                for (unsigned element_id=0; element_id<num_global_elements; element_id++)
                {
                    TS_ASSERT(elements_reduction[element_id] > 0);
                }
            }
        }

        /*
         * All boundary elements have been assigned
         */
        {
            unsigned num_global_b_elements = mesh.GetNumBoundaryElements();
            unsigned b_elements_owned[num_global_b_elements];

            // Create a local map of the boundary elements this processor owns
            for (unsigned b_element_id=0; b_element_id<num_global_b_elements; b_element_id++)
            {
                try
                {
                    unsigned b_element_index = mesh.GetElement(b_element_id)->GetIndex();
                    TS_ASSERT_EQUALS(b_element_id, b_element_index);

                    b_elements_owned[b_element_index] = 1;
                }
                catch(Exception& e)
                {
                    b_elements_owned[b_element_id] = 0;
                }
            }

            // Combine all the local maps by adding them up in the master process
            unsigned b_elements_reduction[num_global_b_elements];
            MPI_Reduce(&b_elements_owned, &b_elements_reduction, num_global_b_elements, MPI_UNSIGNED, MPI_SUM, PetscTools::MASTER_RANK, PETSC_COMM_WORLD);

            // Make sure every boundary element is owned at least by one processor
            if (PetscTools::AmMaster())
            {
                for (unsigned b_element_id=0; b_element_id<num_global_b_elements; b_element_id++)
                {
                    TS_ASSERT(b_elements_reduction[b_element_id] > 0);
                }
            }
        }
    }

    void TestEverythingIsAssignedMetisBinary()
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ParallelTetrahedralMesh<3,3> mesh(ParallelTetrahedralMesh<3,3>::METIS_BINARY);
        mesh.ConstructFromMeshReader(mesh_reader);

        TS_ASSERT_EQUALS(mesh.GetNumNodes(), mesh_reader.GetNumNodes());
        TS_ASSERT_EQUALS(mesh.GetNumElements(), mesh_reader.GetNumElements());
        TS_ASSERT_EQUALS(mesh.GetNumBoundaryElements(), mesh_reader.GetNumFaces());

        /*
         * All the nodes have been assigned
         */
        {
            unsigned num_global_nodes = mesh.GetNumNodes();
            unsigned nodes_owned[num_global_nodes];

            // Create a local map of the nodes this processor owns
            for (AbstractTetrahedralMesh<3,3>::NodeIterator iter = mesh.GetNodeIteratorBegin();
                 iter != mesh.GetNodeIteratorEnd();
                 ++iter)
            {
                unsigned node_index = (*iter).GetIndex();
                nodes_owned[node_index] = 1;
            }

            // Combine all the local maps by adding them up in the master process
            unsigned nodes_reduction[num_global_nodes];
            MPI_Reduce(&nodes_owned, &nodes_reduction, num_global_nodes, MPI_UNSIGNED, MPI_SUM, PetscTools::MASTER_RANK, PETSC_COMM_WORLD);

            // Make sure every node is owned at least by one processor
            if (PetscTools::AmMaster())
            {
                for (unsigned node_id=0; node_id<num_global_nodes; node_id++)
                {
                    TS_ASSERT(nodes_reduction[node_id] > 0);
                }
            }
        }

        /*
         * All elements have been assigned
         */
        {
            unsigned num_global_elements = mesh.GetNumElements();
            unsigned elements_owned[num_global_elements];

            // Create a local map of the elements this processor owns
            for (unsigned element_id=0; element_id<num_global_elements; element_id++)
            {
                try
                {
                    unsigned element_index = mesh.GetElement(element_id)->GetIndex();
                    TS_ASSERT_EQUALS(element_id, element_index);

                    elements_owned[element_index] = 1;
                }
                catch(Exception& e)
                {
                    elements_owned[element_id] = 0;
                }
            }

            // Combine all the local maps by adding them up in the master process
            unsigned elements_reduction[num_global_elements];
            MPI_Reduce(&elements_owned, &elements_reduction, num_global_elements, MPI_UNSIGNED, MPI_SUM, PetscTools::MASTER_RANK, PETSC_COMM_WORLD);

            // Make sure every element is owned at least by one processor
            if (PetscTools::AmMaster())
            {
                for (unsigned element_id=0; element_id<num_global_elements; element_id++)
                {
                    TS_ASSERT(elements_reduction[element_id] > 0);
                }
            }
        }

        /*
         * All boundary elements have been assigned
         */
        {
            unsigned num_global_b_elements = mesh.GetNumBoundaryElements();
            unsigned b_elements_owned[num_global_b_elements];

            // Create a local map of the boundary elements this processor owns
            for (unsigned b_element_id=0; b_element_id<num_global_b_elements; b_element_id++)
            {
                try
                {
                    unsigned b_element_index = mesh.GetElement(b_element_id)->GetIndex();
                    TS_ASSERT_EQUALS(b_element_id, b_element_index);

                    b_elements_owned[b_element_index] = 1;
                }
                catch(Exception& e)
                {
                    b_elements_owned[b_element_id] = 0;
                }
            }

            // Combine all the local maps by adding them up in the master process
            unsigned b_elements_reduction[num_global_b_elements];
            MPI_Reduce(&b_elements_owned, &b_elements_reduction, num_global_b_elements, MPI_UNSIGNED, MPI_SUM, PetscTools::MASTER_RANK, PETSC_COMM_WORLD);

            // Make sure every boundary element is owned at least by one processor
            if (PetscTools::AmMaster())
            {
                for (unsigned b_element_id=0; b_element_id<num_global_b_elements; b_element_id++)
                {
                    TS_ASSERT(b_elements_reduction[b_element_id] > 0);
                }
            }
        }
    }

    void TestConstruct3DWithRegions() throw (Exception)
    {
        TrianglesMeshReader<3,3> mesh_reader("heart/test/data/box_shaped_heart/box_heart_nonnegative_flags");
        ParallelTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);

        TS_ASSERT_EQUALS(mesh_reader.GetNumElementAttributes(), 1u);

        for (unsigned i=0; i<mesh.GetNumElements(); i++)
        {
            try
            {
                unsigned region = mesh.GetElement(i)->GetRegion();
                TS_ASSERT_EQUALS(region, (i+1)%3+1);
            }
            catch(Exception& e)
            {
                // I don't own this element do I?
            }
        }

        TS_ASSERT_EQUALS(mesh_reader.GetNumFaceAttributes(), 1u);

        for (unsigned i=0; i<mesh.GetNumBoundaryElements(); i++)
        {
            try
            {
                unsigned region = mesh.GetBoundaryElement(i)->GetRegion();
                TS_ASSERT_LESS_THAN(region, 4u);
            }
            catch(Exception& e)
            {
                // I don't own this element do I?
            }
        }
    }

    void TestMetisPartitioning()
    {
        EXIT_IF_SEQUENTIAL;

        {
            TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
            ParallelTetrahedralMesh<3,3> mesh(ParallelTetrahedralMesh<3,3>::METIS_LIBRARY);
            mesh.ConstructFromMeshReader(mesh_reader);

            // Check that each processor owns the number of nodes corresponding to its METIS partition
            unsigned local_nodes = mesh.GetDistributedVectorFactory()->GetLocalOwnership();
            TS_ASSERT_EQUALS(local_nodes, mesh.GetNumLocalNodes());

            typedef ParallelTetrahedralMesh<3,3> MESH_TYPE; // To stop TS_ASSERT mistaking the comma for an argument            
            TS_ASSERT_EQUALS(mesh.GetPartitionType(),MESH_TYPE::METIS_LIBRARY);
        }

        {
            TrianglesMeshReader<2,2> mesh_reader("mesh/test/data/disk_984_elements");
            ParallelTetrahedralMesh<2,2> mesh(ParallelTetrahedralMesh<2,2>::METIS_LIBRARY);
            mesh.ConstructFromMeshReader(mesh_reader);

            // Check that each processor owns the number of nodes corresponding to its METIS partition
            unsigned local_nodes = mesh.GetDistributedVectorFactory()->GetLocalOwnership();
            TS_ASSERT_EQUALS(local_nodes, mesh.GetNumLocalNodes());
        }

        {
            TrianglesMeshReader<2,2> mesh_reader("mesh/test/data/disk_984_elements");
            ParallelTetrahedralMesh<2,2> mesh(ParallelTetrahedralMesh<2,2>::METIS_BINARY);
            mesh.ConstructFromMeshReader(mesh_reader);

            // Check that each processor owns the number of nodes corresponding to its METIS partition
            unsigned local_nodes = mesh.GetDistributedVectorFactory()->GetLocalOwnership();
            TS_ASSERT_EQUALS(local_nodes, mesh.GetNumLocalNodes());
        }
    }

    /*
     * This test creates a ParallelTetrahedralMesh, dumps it to disc (permuted), opens the newly
     * created file, and checks if it is consistent with the object in memory.
     */
    void TestWritingPermutedMesh()
    {
        EXIT_IF_SEQUENTIAL

        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ParallelTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);

        bool trivial_permutation = true;
        for (unsigned node_index=0; node_index<mesh.GetNumNodes(); node_index++)
        {
            assert(node_index < mesh.rGetNodePermutation().size());
            if (mesh.rGetNodePermutation()[node_index] != node_index)
            {
                trivial_permutation = false;
                break;
            }
        }

        TS_ASSERT_EQUALS(trivial_permutation, false);

        mesh_reader.Reset();
        std::string filename = "PermutedMesh";
        TrianglesMeshWriter<3,3> mesh_writer("", filename);
        mesh_writer.WriteFilesUsingMeshReader(mesh_reader, mesh.rGetNodePermutation());

        //Make sure everyone finishes writing
        PetscTools::Barrier();

        std::string output_dir = mesh_writer.GetOutputDirectory();
        TrianglesMeshReader<3,3> permuted_mesh_reader(output_dir+filename);

        /*
         * We are not using NodeIterator because we have to call GetNextNode on the
         * permuted_mesh_reader for each node regardless of whether it is owned by
         * this process. The alternative would be to spool the file to the lowest
         * index owned locally but this would neglect the presence of Halo nodes.
         */
        for (unsigned node_index=0; node_index<mesh.GetNumNodes(); node_index++)
        {
            try
            {
                std::vector<double> file_coordinates = permuted_mesh_reader.GetNextNode();
                ChastePoint<3> mem_coordinates = mesh.GetNode(node_index)->GetPoint();

                for (unsigned coord_index=0; coord_index<file_coordinates.size(); coord_index++)
                {
                    TS_ASSERT_EQUALS(file_coordinates[coord_index], mem_coordinates[coord_index]);
                }
            }
            catch(Exception& e)
            {
            }
        }

        for (unsigned element_index=0; element_index<mesh.GetNumElements(); element_index++)
        {
            try
            {
                ElementData file_nodes = permuted_mesh_reader.GetNextElementData();
                Element<3,3>* p_mem_element = mesh.GetElement(element_index);

                for (unsigned node_index=0; node_index<file_nodes.NodeIndices.size(); node_index++)
                {
                    TS_ASSERT_EQUALS(file_nodes.NodeIndices[node_index], p_mem_element->GetNode(node_index)->GetIndex());
                }
            }
            catch(Exception& e)
            {
            }
        }

        for (unsigned face_index=0; face_index<mesh.GetNumElements(); face_index++)
        {
            try
            {
                ElementData file_nodes = permuted_mesh_reader.GetNextFaceData();
                BoundaryElement<2,3>* p_mem_face = mesh.GetBoundaryElement(face_index);

                for (unsigned node_index=0; node_index<file_nodes.NodeIndices.size(); node_index++)
                {
                    TS_ASSERT_EQUALS(file_nodes.NodeIndices[node_index], p_mem_face->GetNode(node_index)->GetIndex());
                }
            }
            catch(Exception& e)
            {
            }
        }
    }

    void TestArchiving() throw(Exception)
    {
        OutputFileHandler handler("archive",false);
        std::string archive_filename;
        handler.SetArchiveDirectory();
        archive_filename = handler.GetOutputDirectoryFullPath() + "parallel_tetrahedral_mesh.arch";
        ArchiveLocationInfo::SetMeshPathname(handler.GetOutputDirectoryFullPath(), "parallel_tetrahedral_mesh");

        AbstractTetrahedralMesh<2,2>* const p_mesh = new ParallelTetrahedralMesh<2,2>(ParallelTetrahedralMesh<2,2>::DUMB);
        //std::vector<unsigned> halo_node_indices;
        std::vector<Node<2>*> halo_nodes;
        unsigned num_nodes;
        unsigned local_num_nodes;
        unsigned num_elements;
        // archive
        {
            TrianglesMeshReader<2,2> mesh_reader("mesh/test/data/disk_984_elements");

            p_mesh->ConstructFromMeshReader(mesh_reader);
            num_nodes = p_mesh->GetNumNodes();
            local_num_nodes = static_cast<ParallelTetrahedralMesh<2,2>* >(p_mesh)->GetNumLocalNodes();
            num_elements = p_mesh->GetNumElements();
            
            halo_nodes = static_cast<ParallelTetrahedralMesh<2,2>* >(p_mesh)->mHaloNodes;

            std::ofstream ofs(archive_filename.c_str());
            boost::archive::text_oarchive output_arch(ofs);
            output_arch << p_mesh;
        }

        // restore
        {
            // Should archive the most abstract class you can to check boost knows what individual classes are.
            // (but here AbstractMesh doesn't have the methods below).
            AbstractTetrahedralMesh<2,2>* p_mesh_abstract;

            // Create an input archive
            std::ifstream ifs(archive_filename.c_str(), std::ios::binary);
            boost::archive::text_iarchive input_arch(ifs);
            // restore from the archive
            input_arch >> p_mesh_abstract;
            // Check we have the right number of nodes & elements
            ParallelTetrahedralMesh<2,2>* p_mesh2 = static_cast<ParallelTetrahedralMesh<2,2>*>(p_mesh_abstract);
            
            TS_ASSERT_EQUALS(p_mesh2->GetNumNodes(), num_nodes);
            TS_ASSERT_EQUALS(p_mesh2->GetNumLocalNodes(), local_num_nodes);
            TS_ASSERT_EQUALS(p_mesh2->GetNumElements(), num_elements);

            // Check some node co-ordinates

            try
            {
                Node<2>* p_node1 = p_mesh->GetNode(0);
                Node<2>* p_node2 = p_mesh2->GetNode(0);
                TS_ASSERT_DELTA(p_node1->GetPoint()[0], p_node2->GetPoint()[0], 1e-6);
                TS_ASSERT_DELTA(p_node1->GetPoint()[1], p_node2->GetPoint()[1], 1e-6);
            }
            catch(Exception& e)
            {
                TS_ASSERT_DIFFERS((int)e.GetShortMessage().find("does not belong to processor"),-1);
            }

            try
            {
                Node<2>* p_node1 = p_mesh->GetNode(500);
                Node<2>* p_node2 = p_mesh2->GetNode(500);
                TS_ASSERT_DELTA(p_node1->GetPoint()[0], p_node2->GetPoint()[0], 1e-6);
            }
            catch(Exception& e)
            {
                TS_ASSERT_DIFFERS((int)e.GetShortMessage().find("does not belong to processor"),-1);
            }

            // Check first element has the right nodes
            try
            {
                Element<2,2>* p_element = p_mesh->GetElement(0);
                Element<2,2>* p_element2 = p_mesh2->GetElement(0);
                TS_ASSERT_EQUALS(p_element->GetNodeGlobalIndex(0), p_element2->GetNodeGlobalIndex(0));
            }
            catch(Exception& e)
            {
                TS_ASSERT_DIFFERS((int)e.GetShortMessage().find("does not belong to processor"),-1);
            }

            try
            {
                Element<2,2>* p_element = p_mesh->GetElement(500);
                Element<2,2>* p_element2 = p_mesh2->GetElement(500);
                TS_ASSERT_EQUALS(p_element->GetNodeGlobalIndex(0), p_element2->GetNodeGlobalIndex(0));
            }
            catch(Exception& e)
            {
                TS_ASSERT_DIFFERS((int)e.GetShortMessage().find("does not belong to processor"),-1);
            }
            
            // Check the halo nodes are right
            std::vector<Node<2>*> halo_nodes2 = static_cast<ParallelTetrahedralMesh<2,2>* >(p_mesh2)->mHaloNodes;
            TS_ASSERT_EQUALS(halo_nodes2.size(), halo_nodes.size());
            delete p_mesh2;
        }

        // restore from a single processor archive
        {
            std::ifstream ifs("mesh/test/data/parallel_tetrahedral_mesh.arch", std::ios::binary);
            boost::archive::text_iarchive input_arch(ifs);

            AbstractTetrahedralMesh<2,2>* p_mesh3 = NULL;

            if ( PetscTools::IsSequential() )
            {
                TS_ASSERT_THROWS_NOTHING(input_arch >> p_mesh3);
            }
            else
            {

                // Should not read this archive - \todo - broken, see #1089
                TS_ASSERT_THROWS_THIS(input_arch >> p_mesh3, 
                        "This archive was written for a different number of processors");
            }            
            delete p_mesh3;
        }

        delete p_mesh;
    }
    
private:

    template <unsigned DIM>
    void CompareParallelMeshOwnership(ParallelTetrahedralMesh<DIM,DIM> &readMesh, ParallelTetrahedralMesh<DIM,DIM> &constructedMesh)
    {
        TS_ASSERT_EQUALS(constructedMesh.GetNumNodes(), readMesh.GetNumNodes());
        TS_ASSERT_EQUALS(constructedMesh.GetNumLocalNodes(), readMesh.GetNumLocalNodes());
        TS_ASSERT_EQUALS(constructedMesh.GetNumBoundaryNodes(), readMesh.GetNumBoundaryNodes());
        TS_ASSERT_EQUALS(constructedMesh.GetNumBoundaryElements(),  readMesh.GetNumBoundaryElements());
        TS_ASSERT_EQUALS(constructedMesh.GetNumElements(), readMesh.GetNumElements());
        TS_ASSERT_EQUALS(constructedMesh.GetNumLocalElements(), readMesh.GetNumLocalElements());
        
        for (unsigned i=0; i<readMesh.GetNumNodes(); i++)
        {
            try
            {
                unsigned index=constructedMesh.SolveNodeMapping(i);
                //Read mesh didn't throw so owns the node
                TS_ASSERT_THROWS_NOTHING(constructedMesh.GetNode(i));
                TS_ASSERT_EQUALS(index, readMesh.SolveNodeMapping(i));
             }
            catch(Exception& e)
            {
                //Read mesh threw so does not own node
                TS_ASSERT_THROWS_CONTAINS(constructedMesh.GetNode(i), "does not belong to processor");
            }
        }

        for (unsigned i=0; i<readMesh.GetNumElements(); i++)
        {
            try
            {
                unsigned index=constructedMesh.SolveElementMapping(i);
                //Read mesh didn't throw so owns the element
                TS_ASSERT_THROWS_NOTHING(constructedMesh.GetElement(i));
                TS_ASSERT_EQUALS(index, readMesh.SolveElementMapping(i));
             }
            catch(Exception& e)
            {
                //Read mesh threw so does not own element
                TS_ASSERT_THROWS_CONTAINS(constructedMesh.GetElement(i), "does not belong to processor");
            }
        }

        for (unsigned i=0; i<readMesh.GetNumBoundaryElements(); i++)
        {
            try
            {
                
                unsigned index=constructedMesh.SolveBoundaryElementMapping(i);
                //Read mesh didn't throw so owns the element
                TS_ASSERT_THROWS_NOTHING(constructedMesh.GetBoundaryElement(i));
                TS_ASSERT_EQUALS(index, readMesh.SolveBoundaryElementMapping(i));
             }
            catch(Exception& e)
            {
                //Read mesh threw so does not own element
                TS_ASSERT_THROWS_CONTAINS(constructedMesh.GetBoundaryElement(i), "does not belong to processor");
            }
        }
        
    }
public:    
    void TestConstructLinearMesh()
    {
        TrianglesMeshReader<1,1> mesh_reader("mesh/test/data/1D_0_to_1_10_elements_with_attributes");
        ParallelTetrahedralMesh<1,1> read_mesh;
        read_mesh.ConstructFromMeshReader(mesh_reader);
        ParallelTetrahedralMesh<1,1> constructed_mesh;
        constructed_mesh.ConstructLinearMesh(10u);

        CompareParallelMeshOwnership(read_mesh, constructed_mesh);
        
        unsigned owned=constructed_mesh.GetDistributedVectorFactory()->GetLocalOwnership();
        unsigned owned_in_read=read_mesh.GetDistributedVectorFactory()->GetLocalOwnership();
        TS_ASSERT_EQUALS(owned_in_read, owned);
        TS_ASSERT_EQUALS(constructed_mesh.GetNumBoundaryElements(), 2u);
        TS_ASSERT_EQUALS(constructed_mesh.GetNumLocalNodes(), owned);
        //Sequential: Process owns one fewer element than the number of nodes
        //Parallel: End processes own the same as the number of node (since one node is paired with a halo node)
        //Parallel: Middle processes own one more than the number of nodes (since two nodes are paired with a halo nodes)
        unsigned expected_elements=owned+1;
        if (PetscTools::AmMaster())
        {
            expected_elements--;
        }
         if (PetscTools::AmTopMost())
        {
            expected_elements--;
        }
        TS_ASSERT_EQUALS(constructed_mesh.GetNumLocalElements(), expected_elements);

        //Note that boundary nodes are local to the process
        if (PetscTools::IsSequential())
        {
            TS_ASSERT_EQUALS(constructed_mesh.GetNumBoundaryNodes(), 2u);
        }
        else
        {
            TS_ASSERT_LESS_THAN(constructed_mesh.GetNumBoundaryNodes(), 2u);
        }
        
    }
 
 
    void TestConstructLinearMeshVerySmall()
    {
        ParallelTetrahedralMesh<1,1> small_mesh;
        //Coverage hack
        TS_ASSERT_THROWS_THIS(small_mesh.ConstructLinearMesh(1), "There aren't enough nodes to make parallelisation worthwhile");
        if (PetscTools::GetNumProcs() >= 4u)
        {
            TS_ASSERT_THROWS_THIS(small_mesh.ConstructLinearMesh(2), "There aren't enough nodes to make parallelisation worthwhile");
        }
        else
        {
            //Works with up to 3 processes
            small_mesh.ConstructLinearMesh(2);
            //Scale it
            small_mesh.Scale(10.0);
            unsigned owned=small_mesh.GetDistributedVectorFactory()->GetLocalOwnership();
            TS_ASSERT_EQUALS(small_mesh.GetNumNodes(), 3u);
            TS_ASSERT_EQUALS(small_mesh.GetNumLocalNodes(), owned);
            TS_ASSERT_EQUALS(small_mesh.GetNumBoundaryElements(),  2u);
            TS_ASSERT_EQUALS(small_mesh.GetNumElements(), 2u);
            //See logic in earlier test
            unsigned expected_elements=owned+1;
            if (PetscTools::AmMaster())
            {
                expected_elements--;
                //Left processor always owns left node
                TS_ASSERT_EQUALS(small_mesh.GetNode(0),small_mesh.GetAnyNode(0));
                TS_ASSERT_DELTA(small_mesh.GetAnyNode(0)->rGetLocation()[0], 0.0, 1e-5);
                TS_ASSERT_DELTA(small_mesh.GetAnyNode(1)->rGetLocation()[0], 10.0, 1e-5);
            }
            if (PetscTools::AmTopMost())
            {
                expected_elements--;
                if (!PetscTools::IsSequential())
                {
                    TS_ASSERT_THROWS_CONTAINS(small_mesh.GetAnyNode(0), "Requested node/halo");
                    //Right processor has  node 1 as halo
                    TS_ASSERT_THROWS_CONTAINS(small_mesh.GetNode(1), "does not belong to processor");
                    TS_ASSERT_THROWS_NOTHING(small_mesh.GetAnyNode(1));//It's a halo
                    TS_ASSERT_DELTA(small_mesh.GetAnyNode(1)->rGetLocation()[0], 10.0, 1e-5);
                    TS_ASSERT_DELTA(small_mesh.GetAnyNode(2)->rGetLocation()[0], 20.0, 1e-5);
                }
            }
            TS_ASSERT_EQUALS(small_mesh.GetNumLocalElements(), expected_elements);
        }
       
    }
    
    void TestConstructRetangularMesh()
    {
        unsigned width=5;
        unsigned height=4*PetscTools::GetNumProcs()-1; //4*NumProcs layers of nodes (ensure dumb partition works in slices)
        
        TetrahedralMesh<2,2> base_mesh;
        base_mesh.ConstructRectangularMesh(width, height);
        TrianglesMeshWriter<2,2> mesh_writer("", "rectangle");
        mesh_writer.WriteFilesUsingMesh(base_mesh);
        PetscTools::Barrier();
        std::string output_dir = mesh_writer.GetOutputDirectory();
        TrianglesMeshReader<2,2> mesh_reader(output_dir+"rectangle");
        ParallelTetrahedralMesh<2,2> read_mesh(ParallelTetrahedralMesh<2,2>::DUMB);
        read_mesh.ConstructFromMeshReader(mesh_reader);
        
        ParallelTetrahedralMesh<2,2> constructed_mesh;
        //Coverage
        TS_ASSERT_THROWS_THIS(constructed_mesh.ConstructRectangularMesh(width, 1, false), 
                            "There aren't enough nodes to make parallelisation worthwhile");

        //Real mesh construction
        constructed_mesh.ConstructRectangularMesh(width, height, false);
        
        CompareParallelMeshOwnership(read_mesh, constructed_mesh);

        if (PetscTools::AmTopMost())
        {
            //Verify some element indices -- top left diagonal goes NW-SE (normal)
            TS_ASSERT_DELTA(constructed_mesh.GetElement(2*(width)*(height-1))->CalculateCentroid()[0],            2.0/3.0, 1e-5);
            TS_ASSERT_DELTA(constructed_mesh.GetElement(2*(width)*(height-1))->CalculateCentroid()[1], (height-1)+2.0/3.0, 1e-5);
            TS_ASSERT_DELTA(constructed_mesh.GetElement(2*(width)*(height-1)+1)->CalculateCentroid()[0],            1.0/3.0, 1e-5);
            TS_ASSERT_DELTA(constructed_mesh.GetElement(2*(width)*(height-1)+1)->CalculateCentroid()[1], (height-1)+1.0/3.0, 1e-5);
        }
        if (PetscTools::AmMaster())
        {
            //Verify some element indices -- bottom left diagonal goes NW-SE (normal)
            TS_ASSERT_DELTA(constructed_mesh.GetElement(0)->CalculateCentroid()[0], 2.0/3.0, 1e-5);
            TS_ASSERT_DELTA(constructed_mesh.GetElement(0)->CalculateCentroid()[1], 2.0/3.0, 1e-5);
            TS_ASSERT_DELTA(constructed_mesh.GetElement(1)->CalculateCentroid()[0], 1.0/3.0, 1e-5);
            TS_ASSERT_DELTA(constructed_mesh.GetElement(1)->CalculateCentroid()[1], 1.0/3.0, 1e-5);
        }
        
    }
    
    
    void TestConstructRetangularMeshStagger()
    {
        unsigned width=4;
        unsigned height=4*PetscTools::GetNumProcs()-1; //4*NumProcs layers of nodes (ensure dumb partition works in slices)
        
        TetrahedralMesh<2,2> base_mesh;
        base_mesh.ConstructRectangularMesh(width, height, true);
        TrianglesMeshWriter<2,2> mesh_writer("", "rectangle");
        mesh_writer.WriteFilesUsingMesh(base_mesh);
        PetscTools::Barrier();
        std::string output_dir = mesh_writer.GetOutputDirectory();
        TrianglesMeshReader<2,2> mesh_reader(output_dir+"rectangle");
        ParallelTetrahedralMesh<2,2> read_mesh(ParallelTetrahedralMesh<2,2>::DUMB);
        read_mesh.ConstructFromMeshReader(mesh_reader);
        
        ParallelTetrahedralMesh<2,2> constructed_mesh;
        constructed_mesh.ConstructRectangularMesh(width, height, true);
        
        CompareParallelMeshOwnership(read_mesh, constructed_mesh);

        if (PetscTools::AmTopMost())
        {
            //Verify some element indices -- top left diagonal goes NW-SE (normal)
            TS_ASSERT_DELTA(constructed_mesh.GetElement(2*(width)*(height-1))->CalculateCentroid()[0],              2.0/3.0, 1e-5);
            TS_ASSERT_DELTA(constructed_mesh.GetElement(2*(width)*(height-1))->CalculateCentroid()[1],   (height-1)+2.0/3.0, 1e-5);
            TS_ASSERT_DELTA(constructed_mesh.GetElement(2*(width)*(height-1)+1)->CalculateCentroid()[0],            1.0/3.0, 1e-5);
            TS_ASSERT_DELTA(constructed_mesh.GetElement(2*(width)*(height-1)+1)->CalculateCentroid()[1], (height-1)+1.0/3.0, 1e-5);
        }
        if (PetscTools::AmMaster())
        {
            TS_ASSERT_EQUALS(height%2, 1u);//If height is odd the bottom left is not staggered - next one is
            //Verify some element indices -- bottom left diagonal goes SW-NE (stagger)
            TS_ASSERT_DELTA(constructed_mesh.GetElement(2)->CalculateCentroid()[0], 1 + 1.0/3.0, 1e-5);
            TS_ASSERT_DELTA(constructed_mesh.GetElement(2)->CalculateCentroid()[1], 2.0/3.0, 1e-5);
            TS_ASSERT_DELTA(constructed_mesh.GetElement(3)->CalculateCentroid()[0], 1 + 2.0/3.0, 1e-5);
            TS_ASSERT_DELTA(constructed_mesh.GetElement(3)->CalculateCentroid()[1], 1.0/3.0, 1e-5);
        }
    }
    
    void TestConstructCuboidMesh()
    {
        unsigned width=2;
        unsigned height=3;
        unsigned depth=4*PetscTools::GetNumProcs()-1;
        
        TetrahedralMesh<3,3> base_mesh;
        base_mesh.ConstructCuboid(width, height, depth);
        TrianglesMeshWriter<3,3> mesh_writer("", "cuboid");
        mesh_writer.WriteFilesUsingMesh(base_mesh);
        PetscTools::Barrier();
        std::string output_dir = mesh_writer.GetOutputDirectory();
        TrianglesMeshReader<3,3> mesh_reader(output_dir+"cuboid");
        ParallelTetrahedralMesh<3,3> read_mesh(ParallelTetrahedralMesh<3,3>::DUMB);
        read_mesh.ConstructFromMeshReader(mesh_reader);
        
        ParallelTetrahedralMesh<3,3> constructed_mesh;
        //Coverage
        TS_ASSERT_THROWS_THIS(constructed_mesh.ConstructCuboid(width, height, 1), 
                            "There aren't enough nodes to make parallelisation worthwhile");
        constructed_mesh.ConstructCuboid(width, height, depth);
        
        CompareParallelMeshOwnership(read_mesh, constructed_mesh);
    }
};
#endif /*TESTPARALLELTETRAHEDRALMESH_HPP_*/
