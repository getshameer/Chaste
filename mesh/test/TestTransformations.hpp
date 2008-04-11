/*
Copyright (C) Oxford University 2008

This file is part of CHASTE.

CHASTE is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

CHASTE is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with CHASTE.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TESTTRANSFORMATIONS_HPP_
#define TESTTRANSFORMATIONS_HPP_

#include <cxxtest/TestSuite.h>
#include "ConformingTetrahedralMesh.hpp"
#include "TrianglesMeshReader.hpp"
#include "TrianglesMeshWriter.hpp"
#include "PetscSetupAndFinalize.hpp"

#include <cmath>

class TestTransformations : public CxxTest::TestSuite
{
public:


    void TestRefreshMeshByScaling(void)
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        TS_ASSERT_DELTA(mesh.CalculateMeshVolume(),1.0,1e-6);
        TS_ASSERT_DELTA(mesh.CalculateMeshSurface(),6.0,1e-6);
        
        //Change coordinates
        
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            Node<3> *p_node=mesh.GetNode(i);
            ChastePoint<3> point=p_node->GetPoint();
            point.SetCoordinate(0,point[0]*2.0);
            point.SetCoordinate(1,point[1]*2.0);
            point.SetCoordinate(2,point[2]*2.0);
            p_node->SetPoint(point);
        }
        
        mesh.RefreshMesh();
        
        TS_ASSERT_DELTA(mesh.CalculateMeshVolume(),8.0,1e-6);
        TS_ASSERT_DELTA(mesh.CalculateMeshSurface(),24.0,1e-6);
        
    }
    
    
    void TestTranslation3DWithUblas(void)
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        double volume = mesh.CalculateMeshVolume();
        double surface_area = mesh.CalculateMeshSurface();
        Node<3> *p_node1 = mesh.GetNode(36);
        ChastePoint<3> point1=p_node1->GetPoint();
        Node<3> *p_node2 = mesh.GetNode(23);
        ChastePoint<3> point2=p_node2->GetPoint();
        
        c_vector<double, 3> old_location1 = point1.rGetLocation();
        c_vector<double, 3> old_location2 = point2.rGetLocation();
        //Set Translation Vector
        c_vector<double, 3> trans_vec;
        trans_vec(0)=2.0;
        trans_vec(1)=2.0;
        trans_vec(2)=2.0;
        
        //Translate
        mesh.Translate(trans_vec);
        c_vector<double, 3> new_location1 = point1.rGetLocation();
        c_vector<double, 3> new_location2 = point2.rGetLocation();
        
        // Check Volume and Surface Area are invariant
        TS_ASSERT_DELTA(mesh.CalculateMeshVolume(),volume,1e-6);
        TS_ASSERT_DELTA(mesh.CalculateMeshSurface(),surface_area,1e-6);
        
        // Spot check a couple of nodes
        TS_ASSERT_DELTA(inner_prod(new_location1-old_location1, trans_vec), 0, 1e-6);
        TS_ASSERT_DELTA(inner_prod(new_location2-old_location2, trans_vec), 0, 1e-6);
    }
    
    
    
    void TestTranslation2DWithUblas(void)
    {
        TrianglesMeshReader<2,2> mesh_reader("mesh/test/data/2D_0_to_1mm_200_elements");
        ConformingTetrahedralMesh<2,2> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        double volume = mesh.CalculateMeshVolume();
        double surface_area = mesh.CalculateMeshSurface();
        Node<2> *p_node1 = mesh.GetNode(36);
        ChastePoint<2> point1=p_node1->GetPoint();
        Node<2> *p_node2 = mesh.GetNode(23);
        ChastePoint<2> point2=p_node2->GetPoint();
        
        c_vector<double, 2> old_location1 = point1.rGetLocation();
        c_vector<double, 2> old_location2 = point2.rGetLocation();
        //Set Translation Vector
        c_vector<double, 2> trans_vec;
        trans_vec(0)=2.0;
        trans_vec(1)=2.0;
        
        //Translate
        mesh.Translate(trans_vec);
        c_vector<double, 2> new_location1 = point1.rGetLocation();
        c_vector<double, 2> new_location2 = point2.rGetLocation();
        
        // Check Volume and Surface Area are invariant
        TS_ASSERT_DELTA(mesh.CalculateMeshVolume(),volume,1e-6);
        TS_ASSERT_DELTA(mesh.CalculateMeshSurface(),surface_area,1e-6);
        
        // Spot check a couple of nodes
        TS_ASSERT_DELTA(inner_prod(new_location1-old_location1, trans_vec), 0, 1e-6);
        TS_ASSERT_DELTA(inner_prod(new_location2-old_location2, trans_vec), 0, 1e-6);
    }
    
    
    void TestTranslationMethod( void ) throw (Exception)
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        // Pick a random node and store spatial position
        Node<3> *node = mesh.GetNode(10);
        ChastePoint<3> original_coordinate = node->GetPoint();
        
        double mesh_volume = mesh.CalculateMeshVolume();
        
        const double x_movement = 1.0, y_movement = 2.5, z_movement = -3.75;
        mesh.Translate(x_movement,y_movement,z_movement);
        
        ChastePoint<3>  new_coordinate = node->GetPoint() ;
        double new_mesh_volume = mesh.CalculateMeshVolume();
        
        TS_ASSERT_DELTA(mesh_volume,new_mesh_volume,1e-6);
        TS_ASSERT_DELTA(original_coordinate[0], new_coordinate[0]-x_movement, 1e-6);
        TS_ASSERT_DELTA(original_coordinate[1], new_coordinate[1]-y_movement, 1e-6);
        TS_ASSERT_DELTA(original_coordinate[2], new_coordinate[2]-z_movement, 1e-6);
    }
    
    void Test3DMeshTranslationWithUblasMethod()
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        
        ConformingTetrahedralMesh<3,3> mesh;
        
        mesh.ConstructFromMeshReader(mesh_reader);
        // Translations -- Add a constant vector to each node
        
        c_vector<double,3>  displacement ;
        displacement(0) = 1 ;
        displacement(1) = 1 ;
        displacement(2) = 1 ;
        
        // Translate the mesh along the vector displacement
        mesh.Translate(displacement);
        
        ConformingTetrahedralMesh<3,3> original_mesh;
        
        original_mesh.ConstructFromMeshReader(mesh_reader);
        
        for (unsigned i=0; i<mesh.GetNumNodes();i++)
        {
        
            // Find new coordinates of the translated node
            Node<3> * node = mesh.GetNode(i) ;
            ChastePoint<3>  new_coordinate = node->GetPoint() ;
            
            // Get original node
            Node<3> * original_node = original_mesh.GetNode(i) ;
            ChastePoint<3>  original_coordinate = original_node->GetPoint() ;
            
            // Run a test to make sure the node has gone to the correct place
            
            TS_ASSERT_DELTA(original_coordinate[0]+ displacement[0], new_coordinate[0] , 1e-5);
            TS_ASSERT_DELTA(original_coordinate[1]+ displacement[1], new_coordinate[1] , 1e-5);
            TS_ASSERT_DELTA(original_coordinate[2]+ displacement[2], new_coordinate[2] , 1e-5);
        }
        // Check volume conservation
        double mesh_volume = mesh.CalculateMeshVolume();
        double original_mesh_volume = original_mesh.CalculateMeshVolume();
        
        TS_ASSERT_DELTA(mesh_volume, original_mesh_volume, 1e-5);
        
    }
    
    void TestXaxisRotation3DWithHomogeneousUblas(void)
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        TS_ASSERT_DELTA(mesh.CalculateMeshVolume(),1.0,1e-6);
        TS_ASSERT_DELTA(mesh.CalculateMeshSurface(),6.0,1e-6);
        
        //Change coordinates
        
        c_matrix<double, 4, 4> x_rotation_matrix = identity_matrix<double>(4);
        
        double theta = M_PI/2;
        
        x_rotation_matrix(1,1) = cos(theta);
        x_rotation_matrix(1,2) = sin(theta);
        x_rotation_matrix(2,1) = -sin(theta);
        x_rotation_matrix(2,2) = cos(theta);
        
        
        
        
        ChastePoint<3> corner_before=mesh.GetNode(6)->GetPoint();
        TS_ASSERT_EQUALS(corner_before[0], 1.0);
        TS_ASSERT_EQUALS(corner_before[1], 1.0);
        TS_ASSERT_EQUALS(corner_before[2], 1.0);
        
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            Node<3> *p_node=mesh.GetNode(i);
            ChastePoint<3> point=p_node->GetPoint();
            
            c_vector<double, 4> point_location;
            
            point_location[0] = point[0];
            point_location[1] = point[1];
            point_location[2] = point[2];
            point_location[3] = 1.0;
            
            c_vector<double, 4> new_point_location = prod(x_rotation_matrix, point_location);
            
            TS_ASSERT_EQUALS(new_point_location[3], 1.0);
            
            point.SetCoordinate(0,new_point_location[0]);
            point.SetCoordinate(1,new_point_location[1]);
            point.SetCoordinate(2,new_point_location[2]);
            p_node->SetPoint(point);
            
            
        }
        ChastePoint<3> corner_after=mesh.GetNode(6)->GetPoint();
        TS_ASSERT_EQUALS(corner_after[0], 1.0);
        TS_ASSERT_EQUALS(corner_after[1], 1.0);
        TS_ASSERT_DELTA(corner_after[2], -1.0, 1e-7);
        
        mesh.RefreshMesh();
        
        TS_ASSERT_DELTA(mesh.CalculateMeshVolume(),1.0,1e-6);
        TS_ASSERT_DELTA(mesh.CalculateMeshSurface(),6.0,1e-6);
        
        
    }
    
    void TestXaxisRotation3DWithMethod(void)
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        ChastePoint<3> corner_before=mesh.GetNode(6)->GetPoint();
        TS_ASSERT_EQUALS(corner_before[0], 1.0);
        TS_ASSERT_EQUALS(corner_before[1], 1.0);
        TS_ASSERT_EQUALS(corner_before[2], 1.0);
        
        double mesh_volume = mesh.CalculateMeshVolume();
        
        mesh.RotateX(M_PI/2.0);
        
        double new_mesh_volume = mesh.CalculateMeshVolume();
        TS_ASSERT_DELTA(mesh_volume,new_mesh_volume,1e-6);
        
        ChastePoint<3> corner_after=mesh.GetNode(6)->GetPoint();
        TS_ASSERT_DELTA(corner_after[0],  1.0, 1e-7);
        TS_ASSERT_DELTA(corner_after[1],  1.0, 1e-7);
        TS_ASSERT_DELTA(corner_after[2], -1.0, 1e-7);
    }
    void TestYaxisRotation3DWithMethod(void)
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        double mesh_volume = mesh.CalculateMeshVolume();
        
        mesh.RotateY(M_PI/2.0);
        
        double new_mesh_volume = mesh.CalculateMeshVolume();
        TS_ASSERT_DELTA(mesh_volume,new_mesh_volume,1e-6);
        
        ChastePoint<3> corner_after=mesh.GetNode(6)->GetPoint();
        TS_ASSERT_DELTA(corner_after[0],  -1.0, 1e-7);
        TS_ASSERT_DELTA(corner_after[1],  1.0, 1e-7);
        TS_ASSERT_DELTA(corner_after[2],  1.0, 1e-7);
    }
    void TestZaxisRotation3DWithMethod(void)
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        double mesh_volume = mesh.CalculateMeshVolume();
        
        mesh.RotateZ(M_PI/2.0);
        
        double new_mesh_volume = mesh.CalculateMeshVolume();
        TS_ASSERT_DELTA(mesh_volume,new_mesh_volume,1e-6);
        
        ChastePoint<3> corner_after=mesh.GetNode(6)->GetPoint();
        TS_ASSERT_DELTA(corner_after[0],  1.0, 1e-7);
        TS_ASSERT_DELTA(corner_after[1], -1.0, 1e-7);
        TS_ASSERT_DELTA(corner_after[2],  1.0, 1e-7);
    }
    
    void TestGeneralConvolution3DWithHomogeneousUblas(void)
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        TS_ASSERT_DELTA(mesh.CalculateMeshVolume(),1.0,1e-6);
        TS_ASSERT_DELTA(mesh.CalculateMeshSurface(),6.0,1e-6);
        
        //Change coordinates
        
        c_matrix<double, 4, 4> x_rotation_matrix = identity_matrix<double>(4);
        c_matrix<double, 4, 4> y_rotation_matrix = identity_matrix<double>(4);
        c_matrix<double, 4, 4> z_rotation_matrix = identity_matrix<double>(4);
        c_matrix<double, 4, 4> translation_matrix = identity_matrix<double>(4);
        
        double theta = 0.7;
        double phi = 0.3;
        double psi = 1.4;
        
        x_rotation_matrix(1,1) = cos(theta);
        x_rotation_matrix(1,2) = sin(theta);
        x_rotation_matrix(2,1) = -sin(theta);
        x_rotation_matrix(2,2) = cos(theta);
        
        y_rotation_matrix(0,0) = cos(phi);
        y_rotation_matrix(0,2) = -sin(phi);
        y_rotation_matrix(2,0) = sin(phi);
        y_rotation_matrix(2,2) = cos(phi);
        
        z_rotation_matrix(0,0) = cos(psi);
        z_rotation_matrix(0,1) = sin(psi);
        z_rotation_matrix(1,0) = -sin(psi);
        z_rotation_matrix(1,1) = cos(psi);
        
        translation_matrix(0,3) = 2.3;
        translation_matrix(1,3) = 3.1;
        translation_matrix(2,3) = 1.7;
        
        /*
        Note: because we are using column-major vectors this tranformation:
        RotX(theta) . RotY(phi) . RotZ(psi) . Trans(...)
        is actually being applied right-to-left
        See test below.
        */
        c_matrix<double, 4, 4> transformation_matrix = prod (x_rotation_matrix, y_rotation_matrix);
        transformation_matrix = prod (transformation_matrix, z_rotation_matrix);
        transformation_matrix = prod (transformation_matrix, translation_matrix);
        
        
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            Node<3> *p_node=mesh.GetNode(i);
            ChastePoint<3> point=p_node->GetPoint();
            
            c_vector<double, 4> point_location;
            
            point_location[0] = point[0];
            point_location[1] = point[1];
            point_location[2] = point[2];
            point_location[3] = 1.0;
            
            c_vector<double, 4> new_point_location = prod(transformation_matrix, point_location);
            
            TS_ASSERT_EQUALS(new_point_location[3], 1.0);
            
            point.SetCoordinate(0,new_point_location[0]);
            point.SetCoordinate(1,new_point_location[1]);
            point.SetCoordinate(2,new_point_location[2]);
            p_node->SetPoint(point);
            
            
        }
        mesh.RefreshMesh();
        
        TS_ASSERT_DELTA(mesh.CalculateMeshVolume(),1.0,1e-6);
        TS_ASSERT_DELTA(mesh.CalculateMeshSurface(),6.0,1e-6);
        
        ChastePoint<3> corner_after=mesh.GetNode(6)->GetPoint();
        TS_ASSERT_DELTA(corner_after[0],  3.59782,  5e-5);
        TS_ASSERT_DELTA(corner_after[1],  0.583418, 5e-5);
        TS_ASSERT_DELTA(corner_after[2],  4.65889,  5e-5);
        
        //Write to file
        TrianglesMeshWriter<3,3> mesh_writer("","TransformedMesh");
        mesh_writer.WriteFilesUsingMesh(mesh);
        
        /*
        * Now try 
        tetview /tmp/chaste/testoutput/TransformedMesh
        */
        
    }
    void TestGeneralConvolution3DWithMethod(void)
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        double mesh_volume = mesh.CalculateMeshVolume();
        
        mesh.Translate(2.3, 3.1, 1.7);
        mesh.RotateZ(1.4);
        mesh.RotateY(0.3);
        mesh.RotateX(0.7);
        
        double new_mesh_volume = mesh.CalculateMeshVolume();
        TS_ASSERT_DELTA(mesh_volume,new_mesh_volume,1e-6);
        
        ChastePoint<3> corner_after=mesh.GetNode(6)->GetPoint();
        TS_ASSERT_DELTA(corner_after[0],  3.59782,  5e-5);
        TS_ASSERT_DELTA(corner_after[1],  0.583418, 5e-5);
        TS_ASSERT_DELTA(corner_after[2],  4.65889,  5e-5);
    }
    
    
    void Test3DAngleAxisRotation()
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        
        ConformingTetrahedralMesh<3,3> mesh;
        
        mesh.ConstructFromMeshReader(mesh_reader);
        
        c_vector<double,3>  axis ;
        axis(0) = 1 ;
        axis(1) = 0 ;
        axis(2) = 0 ;
        
        double angle = M_PI;
        
        mesh.Rotate(axis, angle);
        
        ConformingTetrahedralMesh<3,3> original_mesh;
        
        original_mesh.ConstructFromMeshReader(mesh_reader);
        
        for (unsigned i=0; i<mesh.GetNumNodes();i++)
        {
        
            Node<3> * node = mesh.GetNode(i) ;
            ChastePoint<3>  new_coordinate = node->GetPoint() ;
            
            // Get original node
            Node<3> * original_node = original_mesh.GetNode(i) ;
            ChastePoint<3>  original_coordinate = original_node->GetPoint() ;
            
            // Run a test to make sure the node has gone to the correct place
            
            TS_ASSERT_DELTA(original_coordinate[0], new_coordinate[0] , 1e-5);
            TS_ASSERT_DELTA(original_coordinate[1], -new_coordinate[1] , 1e-5);
            TS_ASSERT_DELTA(original_coordinate[2], -new_coordinate[2] , 1e-5);
        }
        // Check volume conservation
        double mesh_volume = mesh.CalculateMeshVolume();
        double original_mesh_volume = original_mesh.CalculateMeshVolume();
        
        TS_ASSERT_DELTA(mesh_volume, original_mesh_volume, 1e-5);
        
    }
    
    void Test2DMeshRotation()
    {
        TrianglesMeshReader<2,2> mesh_reader("mesh/test/data/2D_0_to_1mm_200_elements");
        
        ConformingTetrahedralMesh<2,2> mesh;
        
        mesh.ConstructFromMeshReader(mesh_reader);
        
        double angle = M_PI;
        
        mesh.Rotate(angle);
        
        ConformingTetrahedralMesh<2,2> original_mesh;
        
        original_mesh.ConstructFromMeshReader(mesh_reader);
        
        for (unsigned i=0; i<mesh.GetNumNodes();i++)
        {
        
            // Find new coordinates of the translated node
            Node<2> * node = mesh.GetNode(i) ;
            ChastePoint<2>  new_coordinate = node->GetPoint() ;
            
            // Get original node
            Node<2> * original_node = original_mesh.GetNode(i) ;
            ChastePoint<2>  original_coordinate = original_node->GetPoint() ;
            
            // Run a test to make sure the node has gone to the correct place
            
            TS_ASSERT_DELTA(original_coordinate[0], -new_coordinate[0] , 1e-5);
            TS_ASSERT_DELTA(original_coordinate[1], -new_coordinate[1] , 1e-5);
        }
        // Check volume conservation
        double mesh_volume = mesh.CalculateMeshVolume();
        double original_mesh_volume = original_mesh.CalculateMeshVolume();
        
        TS_ASSERT_DELTA(mesh_volume, original_mesh_volume, 1e-5);
        
    }
    void TestScalingWithMethod(void)
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        double mesh_volume = mesh.CalculateMeshVolume();
        
        mesh.Scale(1.0);
        TS_ASSERT_DELTA(mesh_volume,mesh.CalculateMeshVolume(),1e-6);
        
        mesh.Scale(2.0, 3.0, 4.0);
        TS_ASSERT_DELTA(24.0*mesh_volume,mesh.CalculateMeshVolume(),1e-6);
        
        ChastePoint<3> corner_after=mesh.GetNode(6)->GetPoint();
        TS_ASSERT_DELTA(corner_after[0],  2.0, 1e-7);
        TS_ASSERT_DELTA(corner_after[1],  3.0, 1e-7);
        TS_ASSERT_DELTA(corner_after[2],  4.0, 1e-7);
        
        
        mesh.Scale(.5,1.0/3.0,.25);
        
        TS_ASSERT_DELTA(mesh_volume,mesh.CalculateMeshVolume(),1e-6);
        
        corner_after=mesh.GetNode(6)->GetPoint();
        TS_ASSERT_DELTA(corner_after[0],  1.0, 1e-7);
        TS_ASSERT_DELTA(corner_after[1],  1.0, 1e-7);
        TS_ASSERT_DELTA(corner_after[2],  1.0, 1e-7);
        
    }
};

#endif /*TESTTRANSFORMATIONS_HPP_*/
