#ifndef TESTFINITEELASTICITYASSEMBLERLONG_HPP_
#define TESTFINITEELASTICITYASSEMBLERLONG_HPP_

#include <cxxtest/TestSuite.h>
#include "FiniteElasticityAssembler.cpp"
#include "TriangulationVertexIterator.hpp"
#include "DofVertexIterator.hpp"
#include "MooneyRivlinMaterialLaw.hpp"
#include "PolynomialMaterialLaw3d.hpp"
#include "ExponentialMaterialLaw.hpp"
#include "FiniteElasticityTools.hpp"


// longer tests, including 3d tests, for nightly runs
class TestFiniteElasticityAssemblerLong : public CxxTest::TestSuite
{
public :

    // Run same simulation on two meshes (one more refined than the other)
    // and test they agree on shared gridpoints
    void Test2dProblemOnSquareForConvergence() throw(Exception)
    {
        ////////////////////////////////////////////////
        // run 1: on a mesh which is refined 3 times..
        ////////////////////////////////////////////////
        
        // note small value of body force and mooney-rivlin const (as if rescaled)
        // - speeds up GMRES        
        Vector<double> body_force(2);
        body_force(0) = 0.06;
        MooneyRivlinMaterialLaw<2> mooney_rivlin_law(0.02);
        
        Triangulation<2> mesh;
        GridGenerator::hyper_cube(mesh, 0.0, 1.0);
        mesh.refine_global(3);
        FiniteElasticityTools<2>::SetFixedBoundary(mesh, 0);
        
        
        FiniteElasticityAssembler<2> finite_elasticity(&mesh,
                                                       &mooney_rivlin_law,
                                                       body_force,
                                                       1.0,
                                                       "finite_elas/simple2d");
                                                       
                                                       
        finite_elasticity.Solve();
        
        // get deformed position
        std::vector<Vector<double> >& deformed_position
            = finite_elasticity.rGetDeformedPosition();
        
        //////////////////////////////////////////////////////////////
        // run 2: same problem, on a mesh which is refined 4 times..
        //////////////////////////////////////////////////////////////
        Triangulation<2> mesh_refined;
        GridGenerator::hyper_cube(mesh_refined, 0.0, 1.0);
        mesh_refined.refine_global(4);
        FiniteElasticityTools<2>::SetFixedBoundary(mesh_refined, 0);
        
        FiniteElasticityAssembler<2> finite_elasticity_ref(&mesh_refined,
                                                           &mooney_rivlin_law,
                                                           body_force,
                                                           1.0,
                                                           "finite_elas/simple2d");
        finite_elasticity_ref.Solve();
        
        // get deformed position
        std::vector<Vector<double> >& deformed_position_ref
            = finite_elasticity_ref.rGetDeformedPosition();
        
        
        //////////////////////////////////////////////////////////////
        // compare the solution with that on the previous mesh
        //////////////////////////////////////////////////////////////
        for (unsigned i=0; i<deformed_position[0].size(); i++)
        {
            TS_ASSERT_DELTA(deformed_position[0](i), deformed_position_ref[0](i), 1e-2);
            TS_ASSERT_DELTA(deformed_position[1](i), deformed_position_ref[1](i), 1e-2);
        }
        
        // check nothing has changed
        TS_ASSERT_DELTA( deformed_position[0](6), 1.2158, 1e-3);
        TS_ASSERT_DELTA( deformed_position[1](6), 0.5,    1e-3);
    }
    
    
    
    void Test3dProblemOnCube() throw(Exception)
    {
        Vector<double> body_force(3);
        body_force(1) = 0.01;
        
        MooneyRivlinMaterialLaw<3> mooney_rivlin_law(0.02,0.02);
        
        Triangulation<3> mesh;
        GridGenerator::hyper_cube(mesh, 0.0, 0.1);
        mesh.refine_global(2);
        FiniteElasticityTools<3>::SetFixedBoundary(mesh,0);
        
        FiniteElasticityAssembler<3> finite_elasticity(&mesh,
                                                       &mooney_rivlin_law,
                                                       body_force,
                                                       1.0,
                                                       "finite_elas/simple3d");
        finite_elasticity.Solve();
        
        // get undeformed position
        std::vector<Vector<double> >& undeformed_position
            = finite_elasticity.rGetUndeformedPosition();
        
        // get deformed position
        std::vector<Vector<double> >& deformed_position
            = finite_elasticity.rGetDeformedPosition();
        
        TS_ASSERT_EQUALS(deformed_position.size(),3u);
        
        for (unsigned vertex_index=0; vertex_index<deformed_position[0].size(); vertex_index++)
        {
            // todo: TEST THESE!!
            double X = undeformed_position[0](vertex_index);
            double Y = undeformed_position[1](vertex_index);
            double Z = undeformed_position[2](vertex_index);
            double x = deformed_position[0](vertex_index);
            double y = deformed_position[1](vertex_index);
            double z = deformed_position[2](vertex_index);
            std::cout << vertex_index << " " << X << " " << Y << " " << Z
                                      << " " << x << " " << y << " " << z << "\n";
        }
        
    }
    
    
    
    void Test3dProblemOnCubeFixedDisplacement() throw(Exception)
    {
        Vector<double> body_force(3); // zero vector
        body_force(2)=0.05;
        
        MooneyRivlinMaterialLaw<3> mooney_rivlin_law(0.02,0.02);
        
        Triangulation<3> mesh;
        GridGenerator::hyper_cube(mesh, 0.0, 1.0);
        mesh.refine_global(1);
        
        Triangulation<3>::cell_iterator element_iter = mesh.begin_active();
        while (element_iter!=mesh.end())
        {
            for (unsigned face_index=0; face_index<GeometryInfo<3>::faces_per_cell; face_index++)
            {
                if (element_iter->face(face_index)->at_boundary())
                {
                    double z = element_iter->face(face_index)->center()(2);
                    
                    if (fabs(z)<1e-4)
                    {
                        // z=0, label as fixed boundary
                        element_iter->face(face_index)->set_boundary_indicator(FIXED_BOUNDARY);
                    }
                    else if (fabs(z-1)<1e-4)
                    {
                        // z=1, label as dirichlet boundary
                        element_iter->face(face_index)->set_boundary_indicator(DIRICHLET_BOUNDARY);
                    }
                    else
                    {
                        // z!=0 or 1, label as neumann boundary
                        element_iter->face(face_index)->set_boundary_indicator(NEUMANN_BOUNDARY);
                    }
                }
            }
            element_iter++;
        }
        
        FiniteElasticityAssembler<3> finite_elasticity(&mesh,
                                                       &mooney_rivlin_law,
                                                       body_force,
                                                       1.0,
                                                       "finite_elas/fixeddisp3d");
                                                       
        DoFHandler<3>& dof_handler = finite_elasticity.rGetDofHandler();
        
        
        std::map<unsigned,double> boundary_values;
        
        std::vector<bool> component_mask(3+1); // dim+1
        component_mask[0] = true;
        component_mask[1] = true;
        component_mask[2] = true;
        component_mask[3] = false;
        
        VectorTools::interpolate_boundary_values(dof_handler,
                                                 FIXED_BOUNDARY,
                                                 ZeroFunction<3>(3+1),  // note the "+1" here! - number of components
                                                 boundary_values,
                                                 component_mask);
                                                 
        assert(!boundary_values.empty());
        
        
        VectorTools::interpolate_boundary_values(dof_handler,
                                                 DIRICHLET_BOUNDARY,
                                                 ComponentSelectFunction<3>(2,-0.1,4),
                                                 boundary_values,
                                                 component_mask);
                                                 
        finite_elasticity.SetBoundaryValues(boundary_values);
        

        finite_elasticity.Solve();
        
        // get deformed position
        std::vector<Vector<double> >& deformed_position
            = finite_elasticity.rGetDeformedPosition();

        // UPDATE THE NODE POSITIONS:
        // GetVertex returns a reference to a Point<DIM>, so this changes the mesh
        // directly. Do this so the new volume can be computed
        TriangulationVertexIterator<3> vertex_iter(&mesh);
        while (!vertex_iter.ReachedEnd())
        {
            unsigned vertex_index = vertex_iter.GetVertexGlobalIndex();
            vertex_iter.GetVertex()[0] = deformed_position[0](vertex_index);
            vertex_iter.GetVertex()[1] = deformed_position[1](vertex_index) ;
            vertex_iter.GetVertex()[2] = deformed_position[2](vertex_index);
            
            vertex_iter.Next();
        }
        
        // compute the deformed volume
        // NOTE: this aren't very accurate volumes, since we have lost the
        // positions of the extra nodes (those used with quadratic basis functions)
        // and the measure() function below must use linear interpolation. Hence
        // the high tolerances
        double deformed_volume = 0.0;
        element_iter = mesh.begin_active();
        while (element_iter!=mesh.end())
        {
            double element_volume = element_iter->measure();
            TS_ASSERT_DELTA(element_volume, 1.0/mesh.n_active_cells(), 1e-2);
            
            deformed_volume += element_volume;
            element_iter++;
        }
        
        TS_ASSERT_DELTA(deformed_volume, 1.0, 0.05);
    }
    
    
// fails - something odd happens with this choice of params - needs fixing..
    void xTestOnHeterogeneousProblem()
    {
        Triangulation<2> mesh;
        GridGenerator::hyper_cube(mesh, 0.0, 1.0);
        mesh.refine_global(3);
        FiniteElasticityTools<2>::SetFixedBoundary(mesh, 0);
                
        Triangulation<2>::cell_iterator element_iter = mesh.begin_active();
        while (element_iter!=mesh.end())
        {
            if (element_iter->center()[0] < 0.5)
            {
                element_iter->set_material_id(5);
            }
            else
            {
                element_iter->set_material_id(6);
            }
            element_iter++;
        }
        
        std::vector<unsigned> material_ids;
        material_ids.push_back(5);
        material_ids.push_back(6);
                
        Vector<double> body_force(2);
        body_force(1) = 0.03;
        
        MooneyRivlinMaterialLaw<2> mooney_rivlin_law_stiff(0.1);
        MooneyRivlinMaterialLaw<2> mooney_rivlin_law_weak(0.02);
        
        std::vector<AbstractIncompressibleMaterialLaw<2>*> material_laws;
        material_laws.push_back(&mooney_rivlin_law_stiff);
        material_laws.push_back(&mooney_rivlin_law_weak);
                
        FiniteElasticityAssembler<2> finite_elasticity(&mesh,
                                                       &mooney_rivlin_law_weak,
                                                       body_force,
                                                       1.0,
                                                       "finite_elas/heterogeneous2d");
                                                                                                              
        finite_elasticity.SetMaterialLawsForHeterogeneousProblem(material_laws, material_ids);                
        finite_elasticity.Solve();
        
        // get undeformed position
        std::vector<Vector<double> >& undeformed_position
            = finite_elasticity.rGetUndeformedPosition();
        
        // get deformed position
        std::vector<Vector<double> >& deformed_position
            = finite_elasticity.rGetDeformedPosition();
        
        for (unsigned vertex_index=0; vertex_index<deformed_position[0].size(); vertex_index++)
        {
            // todo: TEST THESE!!
            double X = undeformed_position[0](vertex_index);
            double Y = undeformed_position[1](vertex_index);
            double x = deformed_position[0](vertex_index);
            double y = deformed_position[1](vertex_index);
            
            std::cout << vertex_index << " " << X << " " << Y
                                      << " " << x << " " << y << "\n";
        }
    }
};
#endif /*TESTFINITEELASTICITYASSEMBLERLONG_HPP_*/
