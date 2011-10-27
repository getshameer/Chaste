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


#ifndef _TESTVTKMESHWRITER_HPP_
#define _TESTVTKMESHWRITER_HPP_

#include <cxxtest/TestSuite.h>
#include "TrianglesMeshReader.hpp"
#include "TrianglesMeshWriter.hpp"
#include "OutputFileHandler.hpp"
#include "TetrahedralMesh.hpp"
#include "VtkMeshWriter.hpp"
#include "PetscSetupAndFinalize.hpp"
#include "DistributedTetrahedralMesh.hpp"
#include <iostream>

#ifdef CHASTE_VTK
#define _BACKWARD_BACKWARD_WARNING_H 1 //Cut out the strstream deprecated warning for now (gcc4.3)
#include <vtkVersion.h>
#endif

class TestVtkMeshWriter : public CxxTest::TestSuite
{

public:

    void TestBasicVtkMeshWriter() throw(Exception)
    {
#ifdef CHASTE_VTK
// Requires  "sudo aptitude install libvtk5-dev" or similar
        TrianglesMeshReader<3,3> reader("mesh/test/data/cube_2mm_12_elements");
        TetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(reader);

        VtkMeshWriter<3,3> writer("TestVtkMeshWriter", "cube_2mm_12_elements");

        TS_ASSERT_THROWS_NOTHING(writer.WriteFilesUsingMesh(mesh));

        //1.6K uncompressed, 1.3K compressed
        std::string results_dir = OutputFileHandler::GetChasteTestOutputDirectory() + "TestVtkMeshWriter/";

        {
            //Check that the reader can see it
            VtkMeshReader<3,3> vtk_reader(results_dir+"cube_2mm_12_elements.vtu");
            TS_ASSERT_EQUALS(vtk_reader.GetNumNodes(), mesh.GetNumNodes());
            TS_ASSERT_EQUALS(vtk_reader.GetNumElements(), mesh.GetNumElements());
            TS_ASSERT_EQUALS(vtk_reader.GetNumFaces(), mesh.GetNumBoundaryElements());

            // Check we have the right number of nodes & elements when we re-construct it
            TetrahedralMesh<3,3> vtk_mesh;
            vtk_mesh.ConstructFromMeshReader(vtk_reader);
            TS_ASSERT_EQUALS(mesh.GetNumNodes(), vtk_mesh.GetNumNodes());
            TS_ASSERT_EQUALS(mesh.GetNumElements(), vtk_mesh.GetNumElements());
            TS_ASSERT_EQUALS(mesh.GetNumBoundaryElements(), vtk_mesh.GetNumBoundaryElements());
        }
#endif //CHASTE_VTK
    }

    void TestSequentialMeshCannotWriteParallelFiles() throw(Exception)
    {
#ifdef CHASTE_VTK
// Requires  "sudo aptitude install libvtk5-dev" or similar
        TrianglesMeshReader<3,3> reader("mesh/test/data/cube_2mm_12_elements");
        TetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(reader);

        VtkMeshWriter<3,3> writer("TestVtkMeshWriter", "cube_2mm_12_elements");

        TS_ASSERT_THROWS_THIS( writer.SetParallelFiles(mesh),
                               "Cannot write parallel files using a sequential mesh");
#endif //CHASTE_VTK
    }

    void TestParallelVtkMeshWriter() throw(Exception)
    {
#ifdef CHASTE_VTK
// Requires  "sudo aptitude install libvtk5-dev" or similar
        TrianglesMeshReader<3,3> reader("mesh/test/data/cube_2mm_12_elements");
        DistributedTetrahedralMesh<3,3> mesh(DistributedTetrahedralMeshPartitionType::DUMB);
        mesh.ConstructFromMeshReader(reader);

        VtkMeshWriter<3,3> writer("TestVtkMeshWriter", "cube_2mm_12_elements_with_data");

        writer.SetParallelFiles(mesh);

        // Add distance from origin into the node "point" data
        std::vector<double> distance;
        for (DistributedTetrahedralMesh<3,3>::NodeIterator node_iter = mesh.GetNodeIteratorBegin();
               node_iter != mesh.GetNodeIteratorEnd();
               ++node_iter)
        {
            distance.push_back(norm_2(node_iter->rGetLocation()));
        }
        writer.AddPointData("Distance from origin", distance);

        // Add location (vector) to "point" data
        std::vector< c_vector<double, 3> > location;
        for (DistributedTetrahedralMesh<3,3>::NodeIterator node_iter = mesh.GetNodeIteratorBegin();
               node_iter != mesh.GetNodeIteratorEnd();
               ++node_iter)
        {
            location.push_back(node_iter->rGetLocation());
        }
        writer.AddPointData("Location", location);

        // Add element quality into the element "cell" data
        std::vector<double> quality;
        for (DistributedTetrahedralMesh<3,3>::ElementIterator ele_iter = mesh.GetElementIteratorBegin();
               ele_iter != mesh.GetElementIteratorEnd();
               ++ele_iter)
        {
            quality.push_back(ele_iter->CalculateQuality());
        }
        writer.AddCellData("Quality", quality);

        // Add fibre type to "cell" data
        std::vector< c_vector<double, 3> > centroid;
        for (DistributedTetrahedralMesh<3,3>::ElementIterator ele_iter = mesh.GetElementIteratorBegin();
               ele_iter != mesh.GetElementIteratorEnd();
               ++ele_iter)
        {
            centroid.push_back(ele_iter->CalculateCentroid());
        }
        writer.AddCellData("Centroid", centroid);

        writer.WriteFilesUsingMesh(mesh);

        PetscTools::Barrier("Wait for files to be written");

        std::stringstream filepath;
        filepath << OutputFileHandler::GetChasteTestOutputDirectory() << "TestVtkMeshWriter/cube_2mm_12_elements_with_data";
        // Add suffix to VTK vtu file
        if (PetscTools::IsSequential())
        {
            filepath <<  ".vtu";
        }
        else
        {
            /*
             * Check that the pvtu file exists. Note that checking its content is hard
             * because the number of processes (.vtu file references) will vary.
             */
            FileFinder vtk_file(filepath.str() + ".pvtu", RelativeTo::Absolute);
            TS_ASSERT(vtk_file.Exists());
            // Add suffix to VTK vtu file
            filepath << "_" << PetscTools::GetMyRank() << ".vtu";
        }
        {
            // Check that the reader can see it
            VtkMeshReader<3,3> vtk_reader(filepath.str());
            TS_ASSERT_EQUALS(vtk_reader.GetNumNodes(), mesh.GetNumLocalNodes() + mesh.GetNumHaloNodes());
            TS_ASSERT_EQUALS(vtk_reader.GetNumElements(), mesh.GetNumLocalElements());

            // Check that it has the correct data
            std::vector<double> distance_read;
            vtk_reader.GetPointData("Distance from origin", distance_read);
            TS_ASSERT_EQUALS(distance.size(), mesh.GetNumLocalNodes() );
            TS_ASSERT_EQUALS(distance_read.size(), mesh.GetNumLocalNodes()  + mesh.GetNumHaloNodes());

            for (unsigned i=0; i<distance.size(); i++)
            {
                TS_ASSERT_EQUALS(distance[i], distance_read[i]);
            }

            std::vector<c_vector<double,3> > location_read;
            vtk_reader.GetPointData("Location", location_read);
            TS_ASSERT_EQUALS(location.size(), mesh.GetNumLocalNodes() );
            TS_ASSERT_EQUALS(location_read.size(), mesh.GetNumLocalNodes()  + mesh.GetNumHaloNodes());
            for (unsigned i=0; i<location.size(); i++)
            {
                for (unsigned j=0; j<3; j++)
                {
                    TS_ASSERT_EQUALS(location[i][j], location_read[i][j]);
                }
            }

            std::vector<double> quality_read;
            vtk_reader.GetCellData("Quality", quality_read);
            TS_ASSERT_EQUALS(quality.size(), mesh.GetNumLocalElements() );
            TS_ASSERT_EQUALS(quality_read.size(), mesh.GetNumLocalElements());
            for (unsigned i=0; i<quality_read.size(); i++)
            {
                TS_ASSERT_EQUALS(quality[i], quality_read[i]);
            }

            std::vector<c_vector<double,3> > centroid_read;
            vtk_reader.GetCellData("Centroid", centroid_read);
            TS_ASSERT_EQUALS(centroid.size(), mesh.GetNumLocalElements() );
            TS_ASSERT_EQUALS(centroid_read.size(), mesh.GetNumLocalElements());
            for (unsigned i=0; i<centroid_read.size(); i++)
            {
                for (unsigned j=0; j<3; j++)
                {
                    TS_ASSERT_EQUALS(centroid[i][j], centroid_read[i][j]);
                }
            }
        }
#endif //CHASTE_VTK
    }

    void TestParallelVtkMeshWriter2d() throw(Exception)
    {
#ifdef CHASTE_VTK
// Requires  "sudo aptitude install libvtk5-dev" or similar
        TrianglesMeshReader<2,2> reader("mesh/test/data/2D_0_to_1mm_200_elements");
        DistributedTetrahedralMesh<2,2> mesh;
        mesh.ConstructFromMeshReader(reader);

        VtkMeshWriter<2,2> writer("TestVtkMeshWriter", "2D_0_to_1mm_200_elements_parallel_data", false);
        writer.SetParallelFiles(mesh);
        // Add distance from origin into the node "point" data
        std::vector<double> rank;
        // Real rank for the owned nodes
        for (unsigned i=0; i<mesh.GetNumLocalNodes(); i++)
        {
            rank.push_back(PetscTools::GetMyRank());
        }
        writer.AddPointData("Process rank", rank);

        writer.WriteFilesUsingMesh(mesh);

        PetscTools::Barrier("Wait for files to be written");

        std::stringstream filepath;
        filepath << OutputFileHandler::GetChasteTestOutputDirectory() << "TestVtkMeshWriter/2D_0_to_1mm_200_elements_parallel_data";
        // Add suffix to VTK vtu file.
        if (PetscTools::IsSequential())
        {
            filepath <<  ".vtu";
        }
        else
        {
            /*
             * Check that the pvtu file exists. Note that checking its content is hard
             * because the number of processes (.vtu file references) will vary.
             */
            FileFinder vtk_file(filepath.str() + ".pvtu", RelativeTo::Absolute);
            TS_ASSERT(vtk_file.Exists());
            // Add suffix to VTK vtu file
            filepath << "_" << PetscTools::GetMyRank() << ".vtu";
        }

        {
            // Check that the reader can see it
            VtkMeshReader<2,2> vtk_reader(filepath.str());
            TS_ASSERT_EQUALS(vtk_reader.GetNumNodes(), mesh.GetNumLocalNodes() + mesh.GetNumHaloNodes());
            TS_ASSERT_EQUALS(vtk_reader.GetNumElements(), mesh.GetNumLocalElements());

            // Check that it has the correct data
            std::vector<double> rank_read;
            vtk_reader.GetPointData("Process rank", rank_read);
            TS_ASSERT_EQUALS(rank.size(), mesh.GetNumLocalNodes() );
            TS_ASSERT_EQUALS(rank_read.size(), mesh.GetNumLocalNodes()  + mesh.GetNumHaloNodes());
            for (unsigned i=0; i<rank.size(); i++)
            {
                TS_ASSERT_EQUALS(rank[i], rank_read[i]);
                TS_ASSERT_EQUALS(rank_read[i], PetscTools::GetMyRank());
            }
        }
#endif //CHASTE_VTK
    }

    void TestVtkMeshWriter2D() throw(Exception)
    {
#ifdef CHASTE_VTK
// Requires  "sudo aptitude install libvtk5-dev" or similar
        TrianglesMeshReader<2,2> reader("mesh/test/data/2D_0_to_1mm_200_elements");
        TetrahedralMesh<2,2> mesh;
        mesh.ConstructFromMeshReader(reader);

        VtkMeshWriter<2,2> writer("TestVtkMeshWriter", "2D_0_to_1mm_200_elements", false);

        // Add distance from origin into the node "point" data
        std::vector<double> distance;
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            distance.push_back(norm_2(mesh.GetNode(i)->rGetLocation()));
        }
        writer.AddPointData("Distance from origin", distance);

        // Add fibre type to "point" data
        std::vector< c_vector<double, 2> > location;
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            location.push_back(mesh.GetNode(i)->rGetLocation());
        }
        writer.AddPointData("Location", location);

        // Add element quality into the element "cell" data
        std::vector<double> quality;
        for (unsigned i=0; i<mesh.GetNumElements(); i++)
        {
            quality.push_back(mesh.GetElement(i)->CalculateQuality());
        }
        writer.AddCellData("Quality", quality);

        // Add fibre type to "cell" data
        std::vector< c_vector<double, 2> > centroid;
        for (unsigned i=0; i<mesh.GetNumElements(); i++)
        {
            centroid.push_back(mesh.GetElement(i)->CalculateCentroid());
        }
        writer.AddCellData("Centroid", centroid);


        writer.WriteFilesUsingMesh(mesh);
        //13K uncompressed, 3.7K compressed

        {
            // Check that the reader can see it
            VtkMeshReader<2,2> vtk_reader(OutputFileHandler::GetChasteTestOutputDirectory() + "TestVtkMeshWriter/2D_0_to_1mm_200_elements.vtu");
            TS_ASSERT_EQUALS(vtk_reader.GetNumNodes(), mesh.GetNumNodes());
            TS_ASSERT_EQUALS(vtk_reader.GetNumElements(), mesh.GetNumElements());

            // Check that it has the correct data
            std::vector<double> distance_read;
            vtk_reader.GetPointData("Distance from origin", distance_read);
            for (unsigned i=0; i<distance_read.size(); i++)
            {
                TS_ASSERT_EQUALS(distance[i], distance_read[i]);
            }
            std::vector<c_vector<double,2> > location_read;
            vtk_reader.GetPointData("Location", location_read);
            for (unsigned i=0; i<location_read.size(); i++)
            {
                for (unsigned j=0; j<2; j++)
                {
                    TS_ASSERT_EQUALS(location[i][j], location_read[i][j]);
                }
            }
            std::vector<double> quality_read;
            vtk_reader.GetCellData("Quality", quality_read);
            for (unsigned i=0; i<quality_read.size(); i++)
            {
                TS_ASSERT_EQUALS(quality[i], quality_read[i]);
            }
            std::vector<c_vector<double,2> > centroid_read;
            vtk_reader.GetCellData("Centroid", centroid_read);
            for (unsigned i=0; i<centroid_read.size(); i++)
            {
                for (unsigned j=0; j<2; j++)
                {
                    TS_ASSERT_EQUALS(centroid[i][j], centroid_read[i][j]);
                }
            }
        }
#endif //CHASTE_VTK
    }

    void TestVtkMeshWriterWithData() throw(Exception)
    {
#ifdef CHASTE_VTK
// Requires  "sudo aptitude install libvtk5-dev" or similar
        TrianglesMeshReader<3,3> reader("heart/test/data/HeartDecimation_173nodes");
        TetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(reader);

        VtkMeshWriter<3,3> writer("TestVtkMeshWriter", "heart_decimation", false);

        // Add element quality into the element "cell" data
        std::vector<double> quality;
        for (unsigned i=0; i<mesh.GetNumElements(); i++)
        {
            quality.push_back(mesh.GetElement(i)->CalculateQuality());
        }
        writer.AddCellData("Quality", quality);

        // Add fibre type to "cell" data
        std::vector< c_vector<double, 3> > centroid;
        for (unsigned i=0; i<mesh.GetNumElements(); i++)
        {
            centroid.push_back(mesh.GetElement(i)->CalculateCentroid());
        }
        writer.AddCellData("Centroid", centroid);

        // Add distance from origin into the node "point" data
        std::vector<double> distance;
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            distance.push_back(norm_2(mesh.GetNode(i)->rGetLocation()));
        }
        writer.AddPointData("Distance from origin", distance);

        // Add fibre type to "point" data
        std::vector< c_vector<double, 3> > location;
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            location.push_back(mesh.GetNode(i)->rGetLocation());
        }
        writer.AddPointData("Location", location);

        writer.WriteFilesUsingMesh(mesh);
        //32K uncompressed, 19K compressed


        {
            // Check that the reader can see it
            VtkMeshReader<3,3> vtk_reader(OutputFileHandler::GetChasteTestOutputDirectory() + "TestVtkMeshWriter/heart_decimation.vtu");
            TS_ASSERT_EQUALS(vtk_reader.GetNumNodes(), mesh.GetNumNodes());
            TS_ASSERT_EQUALS(vtk_reader.GetNumElements(), mesh.GetNumElements());

            // Check that it has the correct data
            std::vector<double> distance_read;
            vtk_reader.GetPointData("Distance from origin", distance_read);
            for (unsigned i=0; i<distance_read.size(); i++)
            {
                TS_ASSERT_EQUALS(distance[i], distance_read[i]);
            }
            std::vector<double> quality_read;
            vtk_reader.GetCellData("Quality", quality_read);
            for (unsigned i=0; i<quality_read.size(); i++)
            {
                TS_ASSERT_EQUALS(quality[i], quality_read[i]);
            }

            std::vector<c_vector<double,3> > centroid_read;
            vtk_reader.GetCellData("Centroid", centroid_read);
            TS_ASSERT_EQUALS(centroid_read.size(),centroid.size());
            ///\todo #1731 - need to read the tensors too.
        }
#endif //CHASTE_VTK
    }
};

#endif //_TESTVTKMESHWRITER_HPP_