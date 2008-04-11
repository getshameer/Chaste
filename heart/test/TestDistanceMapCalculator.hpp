#ifndef TESTDISTANCEMAPCALCULATOR_
#define TESTDISTANCEMAPCALCULATOR_

#include "TrianglesMeshReader.hpp"
#include "ConformingTetrahedralMesh.hpp"
#include "DistanceMapCalculator.hpp"

class TestDistanceMapCalculator : public CxxTest::TestSuite
{
public:
    void TestDistancesToCorner()
    {
        TrianglesMeshReader<3,3> mesh_reader("heart/test/data/cube_21_nodes_side/Cube21"); // 5x5x5mm cube (internode distance = 0.25mm)
        
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        TS_ASSERT_EQUALS(mesh.GetNumNodes(), 9261u); // 21x21x21 nodes
        TS_ASSERT_EQUALS(mesh.GetNumElements(), 48000u);
        TS_ASSERT_EQUALS(mesh.GetNumBoundaryElements(), 4800u);
        
        DistanceMapCalculator<3> distance_calculator(&mesh);
        
        std::vector<unsigned> map_origin;
        map_origin.push_back(0u);
        
        std::vector<double> distances(mesh.GetNumNodes());       
        distance_calculator.ComputeDistanceMap(map_origin, distances);
        
        c_vector<double, 3> origin_node = mesh.GetNode(0)->rGetLocation();
        
        for (unsigned index=0; index<distances.size(); index++)
        {
            c_vector<double, 3> node = mesh.GetNode(index)->rGetLocation();
            
            double dist = sqrt(  (origin_node(0)-node(0))*(origin_node(0)-node(0)) 
                               + (origin_node(1)-node(1))*(origin_node(1)-node(1))
                               + (origin_node(2)-node(2))*(origin_node(2)-node(2))
                              );

            TS_ASSERT_DELTA(distances[index], dist, 1e-11);
        }        
    }
    
    void TestDistancesToFace()
    {
        TrianglesMeshReader<3,3> mesh_reader("heart/test/data/cube_21_nodes_side/Cube21"); // 5x5x5mm cube (internode distance = 0.25mm)
        
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        TS_ASSERT_EQUALS(mesh.GetNumNodes(), 9261u); // 21x21x21 nodes
        TS_ASSERT_EQUALS(mesh.GetNumElements(), 48000u);
        TS_ASSERT_EQUALS(mesh.GetNumBoundaryElements(), 4800u);
        
        DistanceMapCalculator<3> distance_calculator(&mesh);
        
        std::vector<unsigned> map_origin;
        for (unsigned index=0; index<mesh.GetNumNodes(); index++)
        {
            if (mesh.GetNode(index)->rGetLocation()[0] + 0.25 < 1e-6)
            {
                map_origin.push_back(index);
            }
        }
        
        assert(map_origin.size() == 21*21);       
        
        std::vector<double> distances(mesh.GetNumNodes());        
        distance_calculator.ComputeDistanceMap(map_origin, distances);
        
        for (unsigned index=0; index<distances.size(); index++)
        {
            c_vector<double, 3> node = mesh.GetNode(index)->rGetLocation();                        
            TS_ASSERT_DELTA(distances[index], node[0]+0.25,1e-11);
        }            
    }
};

#endif /*TESTDISTANCEMAPCALCULATOR_*/
