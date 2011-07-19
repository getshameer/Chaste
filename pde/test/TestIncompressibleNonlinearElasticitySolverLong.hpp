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

#ifndef TESTINCOMPRESSIBLENONLINEARELASTICITYSOLVERLONG_HPP_
#define TESTINCOMPRESSIBLENONLINEARELASTICITYSOLVERLONG_HPP_

#include <cxxtest/TestSuite.h>
#include "UblasCustomFunctions.hpp"
#include "IncompressibleNonlinearElasticitySolver.hpp"
#include "PetscSetupAndFinalize.hpp"
#include "ExponentialMaterialLaw.hpp"
#include "MooneyRivlinMaterialLaw.hpp"

/*
 * Defines the body force and surface traction for a 3d problem
 * on a cube with a Neo-Hookean law with a known solution. See
 * documentation for the test class below and references therein.
 */
class ThreeDimensionalModelProblem
{
public:

    static double a;
    static double b;
    static double c1;

    static c_vector<double,3> GetBodyForce(c_vector<double,3>& X, double t)
    {
        assert(X(0)>=0 && X(0)<=1 && X(1)>=0 && X(1)<=1 && X(2)>=0 && X(2)<=1);

        double lam1 = 1+a*X(0);
        double lam2 = 1+b*X(1);
        double invlam1 = 1.0/lam1;
        double invlam2 = 1.0/lam2;

        c_vector<double,3> body_force;
        body_force(0) = a;
        body_force(1) = b;
        body_force(2) = 2*X(2)*invlam1*invlam2*( a*a*invlam1*invlam1 + b*b*invlam2*invlam2 );

        return -2*c1*body_force;
    }

    static c_vector<double,3> GetTraction(c_vector<double,3>& X, double t)
    {
        c_vector<double,3> traction = zero_vector<double>(3);

        double lam1 = 1+a*X(0);
        double lam2 = 1+b*X(1);

        double invlam1 = 1.0/lam1;
        double invlam2 = 1.0/lam2;

        double Z = X(2);

        if ( fabs(X(0)-1)<1e-6 )
        {
            traction(0) =  lam1 - invlam1;
            traction(1) =  0.0;
            traction(2) =  -a*Z*invlam1*invlam1*invlam2;
        }
        else if ( fabs(X(1)-0)<1e-6 )
        {
            traction(0) =  0.0;
            traction(1) =  0.0;
            traction(2) =  b*Z*invlam1;
        }
        else if ( fabs(X(1)-1)<1e-6 )
        {
            traction(0) =  0.0;
            traction(1) =  lam2 - invlam2;
            traction(2) =  -b*Z*invlam2*invlam2*invlam1;
        }
        else if ( fabs(X(2)-0)<1e-6 )
        {
            traction(0) =  0.0;
            traction(1) =  0.0;
            traction(2) =  lam1*lam2 - invlam1*invlam2;
        }
        else if ( fabs(X(2)-1)<1e-6 )
        {
            traction(0) =  -a*invlam1*invlam1;
            traction(1) =  -b*invlam2*invlam2;
            traction(2) =  invlam1*invlam2 - lam1*lam2;
        }
        else
        {
            NEVER_REACHED;
        }
        return 2*c1*traction;
    }
};

double ThreeDimensionalModelProblem::a = 0.1;
double ThreeDimensionalModelProblem::b = 0.1;
double ThreeDimensionalModelProblem::c1 = 0.1;


class TestIncompressibleNonlinearElasticitySolverLong : public CxxTest::TestSuite
{
public:
    /**
     * Solve 3D nonlinear elasticity problem with an exact solution.
     *
     * For full details see Pathmanathan, Gavaghan, Whiteley "A comparison of numerical
     * methods used in finite element modelling of soft tissue deformation", J. Strain
     * Analysis, to appear.
     *
     * We solve a 3d problem on a cube with a Neo-Hookean material, assuming the solution
     * will be
     *   x = X+aX^2/2
     *   y = Y+bY^2/2
     *   z = Z/(1+aX)(1+bY)
     * with p=2c (c the material parameter),
     * which, note, has been constructed to be an incompressible. We assume displacement
     * boundary conditions on X=0 and traction boundary conditions on the remaining 5 surfaces.
     * It is then possible to determine the body force and surface tractions required for
     * this deformation, and they are defined in the above class.
     */
    void TestSolve3d() throw(Exception)
    {
        unsigned num_elem_each_dir = 5;
        QuadraticMesh<3> mesh(1.0/num_elem_each_dir, 1.0, 1.0, 1.0);

        // Neo-Hookean material law
        MooneyRivlinMaterialLaw<3> law(ThreeDimensionalModelProblem::c1, 0.0);

        // Define displacement boundary conditions
        std::vector<unsigned> fixed_nodes;
        std::vector<c_vector<double,3> > locations;
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            double X = mesh.GetNode(i)->rGetLocation()[0];
            double Y = mesh.GetNode(i)->rGetLocation()[1];
            double Z = mesh.GetNode(i)->rGetLocation()[2];

            // if X=0
            if ( fabs(X)<1e-6)
            {
                fixed_nodes.push_back(i);
                c_vector<double,3> new_position;
                new_position(0) = 0.0;
                new_position(1) = Y + Y*Y*ThreeDimensionalModelProblem::b/2.0;
                new_position(2) = Z/((1+X*ThreeDimensionalModelProblem::a)*(1+Y*ThreeDimensionalModelProblem::b));
                locations.push_back(new_position);
            }
        }
        assert(fixed_nodes.size()==(2*num_elem_each_dir+1)*(2*num_elem_each_dir+1));

        // Define traction boundary conditions on all boundary elems that are not on X=0
        std::vector<BoundaryElement<2,3>*> boundary_elems;
        for (TetrahedralMesh<3,3>::BoundaryElementIterator iter
              = mesh.GetBoundaryElementIteratorBegin();
            iter != mesh.GetBoundaryElementIteratorEnd();
            ++iter)
        {
            if (fabs((*iter)->CalculateCentroid()[0])>1e-6)
            {
                BoundaryElement<2,3>* p_element = *iter;
                boundary_elems.push_back(p_element);
            }
        }
        assert(boundary_elems.size()==10*num_elem_each_dir*num_elem_each_dir);

        SolidMechanicsProblemDefinition<3> problem_defn(mesh);
        problem_defn.SetFixedNodes(fixed_nodes, locations);
        problem_defn.SetBodyForce(ThreeDimensionalModelProblem::GetBodyForce);
        problem_defn.SetTractionBoundaryConditions(boundary_elems, ThreeDimensionalModelProblem::GetTraction);


        IncompressibleNonlinearElasticitySolver<3> solver(mesh,
                                                          problem_defn,
                                                          &law,
                                                          "nonlin_elas_3d");

        solver.Solve();

        // Compare
        std::vector<c_vector<double,3> >& r_solution = solver.rGetDeformedPosition();
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            double X = mesh.GetNode(i)->rGetLocation()[0];
            double Y = mesh.GetNode(i)->rGetLocation()[1];
            double Z = mesh.GetNode(i)->rGetLocation()[2];

            double exact_x = X + X*X*ThreeDimensionalModelProblem::a/2.0;
            double exact_y = Y + Y*Y*ThreeDimensionalModelProblem::b/2.0;
            double exact_z = Z/((1+X*ThreeDimensionalModelProblem::a)*(1+Y*ThreeDimensionalModelProblem::b));

            TS_ASSERT_DELTA(r_solution[i](0), exact_x, 1e-2);
            TS_ASSERT_DELTA(r_solution[i](1), exact_y, 1e-2);
            TS_ASSERT_DELTA(r_solution[i](2), exact_z, 1e-2);
        }

        for (unsigned i=0; i<mesh.GetNumVertices(); i++)
        {
            TS_ASSERT_DELTA( solver.rGetPressures()[i]/(2*ThreeDimensionalModelProblem::c1), 1.0, 2e-1);
        }
    }


    /**
     *  Solve a problem in which the traction boundary condition is a normal pressure
     *  applied to a surface of the deformed body.
     *
     *  The initial body is the unit cube. The deformation is given by x=Rx', where
     *  R is a rotation matrix and x'=[X/(lambda^2)  lambda*Y lambda*Z], ie simple stretching
     *  followed by a rotation.
     *
     *  Ignoring R for the time being, the deformation gradient would be
     *  F=diag(1/lambda^2, lambda, lambda).
     *  Assuming a Neo-Hookean material law, the first Piola-Kirchoff tensor is
     *
     *   S = 2c F^T - p F^{-1}
     *     = diag ( 2c lam^{-2} - p lam^2,  2c lam - p lam^{-1},  2c lam - p lam^{-1}
     *
     *  We choose the internal pressure (p) to be 2clam^2, so that
     *
     *  S = diag( 2c (lam^{-2} - lam^4), 0, 0)
     *
     *  To obtain this deformation as the solution, we can specify fixed location boundary conditions
     *  on X=0 and specify a traction of t=[2c(lam^{-2} - lam^4) 0 0] on the X=1 surface.
     *
     *  Now, including the rotation matrix, we can specify appropriate fixed location boundary conditions
     *  on the X=0 surface, and specify that a normal pressure should act on the *deformed* X=1 surface,
     *  with value P = 2c(lam^{-2} - lam^4)/(lambda^2) [divided by lambda^2 as the deformed surface
     *  has a smaller area than the undeformed surface.
     *
     */
    void TestWithPressureOnDeformedSurfaceBoundaryCondition3d() throw (Exception)
    {
        double lambda = 0.85;

        unsigned num_elem_each_dir = 5;
        QuadraticMesh<3> mesh(1.0/num_elem_each_dir, 1.0, 1.0, 1.0);

        // Neo-Hookean material law
        double c1 = 1.0;
        MooneyRivlinMaterialLaw<3> law(c1, 0.0);

        // anything will do here
        c_matrix<double,3,3> rotation_matrix = identity_matrix<double>(3);
        rotation_matrix(0,0)=1.0/sqrt(2);
        rotation_matrix(0,1)=-1.0/sqrt(2);
        rotation_matrix(1,0)=1.0/sqrt(2);
        rotation_matrix(1,1)=1.0/sqrt(2);


        // Define displacement boundary conditions
        std::vector<unsigned> fixed_nodes;
        std::vector<c_vector<double,3> > locations;

        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            double X = mesh.GetNode(i)->rGetLocation()[0];
            double Y = mesh.GetNode(i)->rGetLocation()[1];
            double Z = mesh.GetNode(i)->rGetLocation()[2];

            // if X=0
            if ( fabs(X)<1e-6)
            {
                fixed_nodes.push_back(i);
                c_vector<double,3> new_position_before_rotation;
                new_position_before_rotation(0) = 0.0;
                new_position_before_rotation(1) = lambda*Y;
                new_position_before_rotation(2) = lambda*Z;
                locations.push_back(prod(rotation_matrix, new_position_before_rotation));
            }
        }
        assert(fixed_nodes.size()==(2*num_elem_each_dir+1)*(2*num_elem_each_dir+1));

        // Define pressure boundary conditions X=1 surface
        std::vector<BoundaryElement<2,3>*> boundary_elems;

//        std::vector<c_vector<double,3> > tractions;
//        c_vector<double,3> traction = zero_vector<double>(3);
//        traction(0)=2*c1*(pow(lambda,-2)-pow(lambda,4));

        double pressure_bc = 2*c1*((pow(lambda,-2)-pow(lambda,4)))/(lambda*lambda);

        for (TetrahedralMesh<3,3>::BoundaryElementIterator iter
              = mesh.GetBoundaryElementIteratorBegin();
            iter != mesh.GetBoundaryElementIteratorEnd();
            ++iter)
        {
            if (fabs((*iter)->CalculateCentroid()[0]-1)<1e-6)
            {
                BoundaryElement<2,3>* p_element = *iter;
                boundary_elems.push_back(p_element);
//                tractions.push_back(traction);
            }
        }
        assert(boundary_elems.size()==2*num_elem_each_dir*num_elem_each_dir);

        SolidMechanicsProblemDefinition<3> problem_defn(mesh);
        problem_defn.SetFixedNodes(fixed_nodes, locations);
        problem_defn.SetApplyNormalPressureOnDeformedSurface(boundary_elems, pressure_bc);
//        problem_defn.SetTractionBoundaryConditions(boundary_elems, tractions);


        IncompressibleNonlinearElasticitySolver<3> solver(mesh,
                                                          problem_defn,
                                                          &law,
                                                          "nonlin_elas_3d_pressure_on_deformed");

        solver.Solve();

        // Compare
        std::vector<c_vector<double,3> >& r_solution = solver.rGetDeformedPosition();
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            double X = mesh.GetNode(i)->rGetLocation()[0];
            double Y = mesh.GetNode(i)->rGetLocation()[1];
            double Z = mesh.GetNode(i)->rGetLocation()[2];

            c_vector<double,3> new_position_before_rotation;
            new_position_before_rotation(0) = X/(lambda*lambda);
            new_position_before_rotation(1) = lambda*Y;
            new_position_before_rotation(2) = lambda*Z;

            c_vector<double,3> new_position = prod(rotation_matrix, new_position_before_rotation);

            TS_ASSERT_DELTA(r_solution[i](0), new_position(0), 1e-2);
            TS_ASSERT_DELTA(r_solution[i](1), new_position(1), 1e-2);
            TS_ASSERT_DELTA(r_solution[i](2), new_position(2), 1e-2);
        }

        for (unsigned i=0; i<mesh.GetNumVertices(); i++)
        {
            TS_ASSERT_DELTA( solver.rGetPressures()[i], 2*c1*lambda*lambda, 0.05);
        }
    }
};


#endif /*TESTINCOMPRESSIBLENONLINEARELASTICITYSOLVERLONG_HPP_*/