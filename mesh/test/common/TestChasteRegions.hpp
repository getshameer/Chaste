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


#ifndef TESTCHASTEREGIONS_HPP_
#define TESTCHASTEREGIONS_HPP_

#include <cxxtest/TestSuite.h>
#include "ChastePoint.hpp"
#include "ChasteCuboid.hpp"
#include "ChasteEllipsoid.hpp"
#include "ChasteNodesList.hpp"
#include "AbstractChasteRegion.hpp"
#include "Node.hpp"

typedef ChasteCuboid<3> POINT_3D;

class TestRegions : public CxxTest::TestSuite
{
public:

    void TestCuboidCreationAndContained() throw(Exception)
    {
        ChastePoint<3> point_a(-3, -3, -3);
        ChastePoint<3> point_b(3, 3, 3);
        ChastePoint<3> point_inside(0, 0, 0);
        ChastePoint<3> point_outside(-4, -4, -4);

        ChasteCuboid<3> cuboid_a_b(point_a, point_b);

        TS_ASSERT_THROWS_THIS(POINT_3D cuboid_b_a(point_b, point_a),"Attempt to create a cuboid with MinCorner greater than MaxCorner in some dimension");

        TS_ASSERT_EQUALS(cuboid_a_b.DoesContain(point_inside), true);
        TS_ASSERT_EQUALS(cuboid_a_b.DoesContain(point_a), true);
        TS_ASSERT_EQUALS(cuboid_a_b.DoesContain(point_b), true);

        TS_ASSERT_EQUALS(cuboid_a_b.DoesContain(point_outside), false);

        // A point that is just outside the cuboid counts as inside to deal with rounding errors
        double just = 3.00000000000000008882; // taken from error in cuboid mesh generation
        ChastePoint<3> just_outside(just, just, just);
        TS_ASSERT_EQUALS(cuboid_a_b.DoesContain(just_outside), true);

        // Lower dimensional cases

        ///\ To do The following test case has been deprecated as of r7769 because the rest of the code assumes that the point are in 3D in a 3D cuboid
//        ChastePoint<2> two_d_point_in(0.0, 0.0);
//        ChastePoint<1> one_d_point_in(0.0);
//
//        ChasteCuboid<3> cuboid_3_2(point_a, point_b);
//        ChasteCuboid<3> cuboid_3_1(point_a, point_b);
//
//        TS_ASSERT(cuboid_3_2.DoesContain(two_d_point_in));
//        TS_ASSERT(cuboid_3_1.DoesContain(one_d_point_in));
//
//        ChastePoint<2> two_d_point_out(-4.0, -4.0);
//        ChastePoint<1> one_d_point_out(-4.0);
//        TS_ASSERT_EQUALS(cuboid_3_2.DoesContain(two_d_point_out), false);
//        TS_ASSERT_EQUALS(cuboid_3_1.DoesContain(one_d_point_out), false);

        ChastePoint<3> upper=cuboid_a_b.rGetUpperCorner();
        c_vector<double, 3> diff_upper = upper.rGetLocation() - point_b.rGetLocation();
        TS_ASSERT_DELTA(norm_2(diff_upper),0.0,1e-10);
        ChastePoint<3> lower=cuboid_a_b.rGetLowerCorner();
        c_vector<double, 3> diff_lower = lower.rGetLocation() - point_a.rGetLocation();
        TS_ASSERT_DELTA(norm_2(diff_lower),0.0,1e-10);
    }

    void TestNodesList() throw(Exception)
    {
        ChastePoint<3> point_a(-3, -3, -3);
        ChastePoint<3> point_b(3, 3, 3);
        ChastePoint<3> point_c(9, 4, 7);

        Node<3> first_node(0u,point_a);
        Node<3> second_node(1u,point_b);
        Node<3> third_node(2u,point_c);

        std::vector<Node<3u>* > array_of_nodes;

        array_of_nodes.push_back(&first_node);
        array_of_nodes.push_back(&second_node);
        array_of_nodes.push_back(&third_node);

        //declare the object
        ChasteNodesList<3> nodes_list(array_of_nodes);

        ChastePoint<3> test_point_contained(9, 4, 7);
        ChastePoint<3> test_point_non_contained(10, 4, 7);

        TS_ASSERT_EQUALS(nodes_list.DoesContain(test_point_contained), true);
        TS_ASSERT_EQUALS(nodes_list.DoesContain(test_point_non_contained), false);

    }

    void TestEllipsoidCreationAndContained() throw(Exception)
    {
    	ChastePoint<3> centre_3D(0, 0, 0);
    	ChastePoint<3> radii_3D(2, 4, 6);
    	ChastePoint<3> bad_radii_3D(-2, 4, 6);
    	ChastePoint<3> point_inside_x_3D(1, 0, 0);
    	ChastePoint<3> point_inside_y_3D(0, 2, 0);
    	ChastePoint<3> point_inside_z_3D(0, 0, 3);
    	ChastePoint<3> point_outside_x_3D(3, 0, 0);
    	ChastePoint<3> point_outside_y_3D(0, 5, 0);
    	ChastePoint<3> point_outside_z_3D(0, 0, 7);

    	ChasteEllipsoid<3> ellipsoid_3D(centre_3D, radii_3D);

    	TS_ASSERT_THROWS_THIS(ChasteEllipsoid<3> bad_ellipsoid_3D(centre_3D, bad_radii_3D),"Attempted to create an ellipsoid with a negative radius");

    	TS_ASSERT_EQUALS(ellipsoid_3D.DoesContain(point_inside_x_3D), true);
    	TS_ASSERT_EQUALS(ellipsoid_3D.DoesContain(point_inside_y_3D), true);
    	TS_ASSERT_EQUALS(ellipsoid_3D.DoesContain(point_inside_z_3D), true);

    	TS_ASSERT_EQUALS(ellipsoid_3D.DoesContain(point_outside_x_3D), false);
    	TS_ASSERT_EQUALS(ellipsoid_3D.DoesContain(point_outside_y_3D), false);
    	TS_ASSERT_EQUALS(ellipsoid_3D.DoesContain(point_outside_z_3D), false);

    	// A point that is just outside the ellipsoid counts as inside to deal with rounding errors
    	ChastePoint<3> just_outside_3D(0,0,6.00000000000000008882); // taken from error in cuboid mesh generation
    	TS_ASSERT_EQUALS(ellipsoid_3D.DoesContain(just_outside_3D), true);

    	ChastePoint<3> returned_centre = ellipsoid_3D.rGetCentre();
    	c_vector<double, 3> diff_centre = returned_centre.rGetLocation() - centre_3D.rGetLocation();
    	TS_ASSERT_DELTA(norm_2(diff_centre),0.0,1e-10);
    	ChastePoint<3> returned_radii=ellipsoid_3D.rGetRadii();
    	c_vector<double, 3> diff_radii = returned_radii.rGetLocation() - radii_3D.rGetLocation();
    	TS_ASSERT_DELTA(norm_2(diff_radii),0.0,1e-10);

    	// Lower dimensional cases
    	ChastePoint<2> centre_2D(0, 0);
    	ChastePoint<2> radii_2D(2, 4);
    	ChastePoint<2> bad_radii_2D(-2, 4);
     	ChastePoint<2> point_inside_x_2D(1, 0);
    	ChastePoint<2> point_inside_y_2D(0, 2);
    	ChastePoint<2> point_outside_x_2D(3, 0);
    	ChastePoint<2> point_outside_y_2D(0, 5);
    	ChasteEllipsoid<2> ellipsoid_2D(centre_2D, radii_2D);
    	TS_ASSERT_EQUALS(ellipsoid_2D.DoesContain(point_inside_x_2D), true);
    	TS_ASSERT_EQUALS(ellipsoid_2D.DoesContain(point_inside_y_2D), true);
    	TS_ASSERT_EQUALS(ellipsoid_2D.DoesContain(point_outside_x_2D), false);
    	TS_ASSERT_EQUALS(ellipsoid_2D.DoesContain(point_outside_y_2D), false);

    	ChastePoint<1> centre_1D(0);
    	ChastePoint<1> radii_1D(2);
    	ChastePoint<1> bad_radii_1D(-2);
    	ChastePoint<1> point_inside_x_1D(1);
    	ChastePoint<1> point_outside_x_1D(3);
    	ChasteEllipsoid<1> ellipsoid_1D(centre_1D, radii_1D);
    	TS_ASSERT_EQUALS(ellipsoid_1D.DoesContain(point_inside_x_1D), true);
    	TS_ASSERT_EQUALS(ellipsoid_1D.DoesContain(point_outside_x_1D), false);

    }

};

#endif /*TESTCHASTEREGIONS_HPP_*/