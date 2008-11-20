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


#ifndef TESTUBLASCUSTOMFUNCTIONS_HPP_
#define TESTUBLASCUSTOMFUNCTIONS_HPP_

#include <cxxtest/TestSuite.h>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include "UblasCustomFunctions.hpp"
#include "PetscSetupAndFinalize.hpp"
#include "PetscTools.hpp"

class TestUblasCustomFunctions : public CxxTest::TestSuite
{
public:

    void TestDeterminant()
    {
        using namespace boost::numeric::ublas;
        c_matrix<double, 1, 1> C;
        double OneOneDeterminant;
        C(0,0)=5.6;
        OneOneDeterminant = Determinant(C);
        TS_ASSERT_DELTA( OneOneDeterminant, 5.6, 0.0000000001);

        c_matrix<double, 3, 3> A;
        A(0,0) = 2.4;
        A(0,1) = 5;
        A(0,2) = 5;
        A(1,0) = 5;
        A(1,1) = 6;
        A(1,2) = 7;
        A(2,0) = 6;
        A(2,1) = 8;
        A(2,2) = 9;
        double ThreeThreeDeterminant = Determinant(A);
        TS_ASSERT_DELTA( ThreeThreeDeterminant, 0.2, 0.0000000001);

        c_matrix<double, 2, 2> B;
        B(0,0) = 2.4;
        B(0,1) = 5;
        B(1,0) = 5;
        B(1,1) = 6;
        double TwoTwoDeterminant = Determinant(B);
        TS_ASSERT_DELTA( TwoTwoDeterminant, -10.6, 0.0000000001);
    }

    void TestSubDeterminant()
    {
        using namespace boost::numeric::ublas;

        c_matrix<double, 1, 1> C;
        double OneOneDeterminant;
        C(0,0)=5.6;
        OneOneDeterminant = Determinant(C);
        TS_ASSERT_DELTA( OneOneDeterminant,
                         C(0,0)*SubDeterminant(C,0,0),
                         1e-10);

        c_matrix<double, 2, 2> B;
        B(0,0) = 2.4;
        B(0,1) = 5;
        B(1,0) = 5;
        B(1,1) = 6;
        double TwoTwoDeterminant = Determinant(B);


        TS_ASSERT_DELTA( TwoTwoDeterminant,
                         B(0,0)*SubDeterminant(B,0,0)-B(0,1)*SubDeterminant(B,0,1), 1e-10);
        TS_ASSERT_DELTA( TwoTwoDeterminant,
                         B(0,0)*SubDeterminant(B,0,0)-B(1,0)*SubDeterminant(B,1,0), 1e-10);
        c_matrix<double, 3, 3> A;
        A(0,0) = 2.4;
        A(0,1) = 5;
        A(0,2) = 5;
        A(1,0) = 5;
        A(1,1) = 6;
        A(1,2) = 7;
        A(2,0) = 6;
        A(2,1) = 8;
        A(2,2) = 9;
        double ThreeThreeDeterminant = Determinant(A);
        TS_ASSERT_DELTA( ThreeThreeDeterminant, 0.2, 1e-10);
        TS_ASSERT_DELTA( ThreeThreeDeterminant,
                         A(0,0)*SubDeterminant(A,0,0)
                         - A(1,0)*SubDeterminant(A,1,0)
                         + A(2,0)*SubDeterminant(A,2,0),
                         1e-10);
        TS_ASSERT_DELTA( ThreeThreeDeterminant,
                         - A(0,1)*SubDeterminant(A,0,1)
                         + A(1,1)*SubDeterminant(A,1,1)
                         - A(2,1)*SubDeterminant(A,2,1),
                         1e-10);
        TS_ASSERT_DELTA( ThreeThreeDeterminant,
                         A(0,0)*SubDeterminant(A,0,0)
                         - A(0,1)*SubDeterminant(A,0,1)
                         + A(0,2)*SubDeterminant(A,0,2),
                         1e-10);
        TS_ASSERT_DELTA( ThreeThreeDeterminant,
                         - A(1,0)*SubDeterminant(A,1,0)
                         + A(1,1)*SubDeterminant(A,1,1)
                         - A(1,2)*SubDeterminant(A,1,2),
                         1e-10);



    }

    void TestInverse( void )
    {
        using namespace boost::numeric::ublas;
        c_matrix<double, 1, 1> C;
        C(0,0) = 8;
        c_matrix<double, 1, 1> invC;
        invC = Inverse(C);
        TS_ASSERT_DELTA(invC(0,0), 0.125, 0.0000000001);
        c_matrix<double, 3, 3> A;
        A(0,0) = 2.4;
        A(0,1) = 5;
        A(0,2) = 5;
        A(1,0) = 5;
        A(1,1) = 6;
        A(1,2) = 7;
        A(2,0) = 6;
        A(2,1) = 8;
        A(2,2) = 9;
        c_matrix<double, 3, 3> invA;
        c_matrix<double, 3, 3> invAMatlab;
        invAMatlab(0,0) = -10.00;
        invAMatlab(0,1) = -25.00;
        invAMatlab(0,2) = 25.00;
        invAMatlab(1,0) = -15.00;
        invAMatlab(1,1) = -42.00;
        invAMatlab(1,2) = 41.00;
        invAMatlab(2,0) = 20.00;
        invAMatlab(2,1) = 54.00;
        invAMatlab(2,2) = -53.00;
        invA = Inverse(A);
        for ( int i = 0; i < 3; i++)
        {
            for ( int j = 0; j < 3; j++)
            {
                TS_ASSERT_DELTA( invA(i,j), invAMatlab(i,j), 0.0000000001);
            }
        }
        c_matrix<double, 2, 2> B;
        B(0,0) = 2.4;
        B(0,1) = 5;
        B(1,0) = 5;
        B(1,1) = 6;
        c_matrix<double, 2, 2> invB;
        invB = Inverse(B);
        c_matrix<double, 2, 2> invBMatlab;
        invBMatlab(0,0) = -0.5660;
        invBMatlab(0,1) = 0.4717;
        invBMatlab(1,0) = 0.4717;
        invBMatlab(1,1) = -0.2264;
        for ( int i = 0; i < 2; i++)
        {
            for ( int j = 0; j < 2; j++)
            {
                TS_ASSERT_DELTA( invB(i,j), invBMatlab(i,j), 0.0001);
            }
        }
    }


    void TestTraceAndSecondInvariant()
    {
        c_matrix<double, 1,1> a;
        a(0,0) = 13.03;
        TS_ASSERT_DELTA(Trace(a),13.03,1e-10);

        c_matrix<double, 2,2> b;
        b(0,0) = 13.03;
        b(1,0) = 3.03;
        b(0,1) = 3.03;
        b(1,1) = 165;
        TS_ASSERT_DELTA(Trace(b),13.03+165,1e-10);
        
        TS_ASSERT_DELTA(SecondInvariant(b), Determinant(b), 1e-12);

        // symmetric 3 by 3 matrix.
        c_matrix<double, 3,3> c;
        c(0,0) = 13.03;
        c(1,0) = 1.3;
        c(2,0) = 2.3;
        c(0,1) = 1.3;
        c(1,1) = 45;
        c(2,1) = 9.9;
        c(0,2) = 2.3;
        c(1,2) = 9.9;
        c(2,2) = 34;

        c_matrix<double,3,3> c_squared = prod(trans(c),c);

        TS_ASSERT_DELTA(Trace(c),13.03+45+34,1e-10);
        TS_ASSERT_DELTA(SecondInvariant(c),0.5*(Trace(c)*Trace(c)-Trace(c_squared)),1e-10);

        c_matrix<double, 4,4> d = identity_matrix<double>(4);
        TS_ASSERT_DELTA(Trace(d),4,1e-10);
    }



    // Get a row from a matrix
    void TestUblasMatrixRow()
    {
        c_matrix<double,2,3> a;
        a(0,0) = 1;
        a(0,1) = 2;
        a(0,2) = 3;
        a(1,0) = 4;
        a(1,1) = 5;
        a(1,2) = 6;

        matrix_row< c_matrix<double,2,3> > row0(a, 0);
        matrix_row< c_matrix<double,2,3> > row1(a, 1);

        TS_ASSERT_EQUALS(a(0,0), row0(0));
        TS_ASSERT_EQUALS(a(0,1), row0(1));
        TS_ASSERT_EQUALS(a(0,2), row0(2));
        TS_ASSERT_EQUALS(a(1,0), row1(0));
        TS_ASSERT_EQUALS(a(1,1), row1(1));
        TS_ASSERT_EQUALS(a(1,2), row1(2));
    }

    void TestCreate_c_vector()
    {
        c_vector<double, 1> v1 = Create_c_vector(1);
        TS_ASSERT_EQUALS( v1[0], 1);

        c_vector<double, 2> v2 = Create_c_vector(1,2);
        TS_ASSERT_EQUALS( v2[0], 1);
        TS_ASSERT_EQUALS( v2[1], 2);

        c_vector<double, 3> v3 = Create_c_vector(1,2,3);
        TS_ASSERT_EQUALS( v3[0], 1);
        TS_ASSERT_EQUALS( v3[1], 2);
        TS_ASSERT_EQUALS( v3[2], 3);
    }
    
    void TestEigenVectorValueCalculation() throw(Exception)
    {
        c_matrix<double, 3, 3> A;
        A(0,0) = 2.4;
        A(0,1) = 5;
        A(0,2) = 5;
        A(1,0) = 5;
        A(1,1) = 6;
        A(1,2) = 7;
        A(2,0) = 6;
        A(2,1) = 8;
        A(2,2) = 9;
        
        double smallest_eigenvalue = -0.0096002162399060342324;
        
        c_vector<double, 3> eigenvector;
        
        eigenvector = CalculateEigenvectorForSmallestEigenvalue(A);
        
        c_vector<double, 3> a_times_eigenvector = prod(A, eigenvector);              

        double delta = 1e-12; 
        TS_ASSERT_DELTA( a_times_eigenvector[0], smallest_eigenvalue*eigenvector[0], delta);  
        TS_ASSERT_DELTA( a_times_eigenvector[1], smallest_eigenvalue*eigenvector[1], delta);
        TS_ASSERT_DELTA( a_times_eigenvector[2], smallest_eigenvalue*eigenvector[2], delta);
        
        // set up B = [1  0 0] 
        //            [0  0 1] 
        //            [0 -1 0]
        c_matrix<double,3,3> B = zero_matrix<double>(3,3);
        B(0,0) = B(1,2) = 1.0;
        B(2,1) = -1.0;
        TS_ASSERT_THROWS_ANYTHING(CalculateEigenvectorForSmallestEigenvalue(B));  // eigenvalues are 1,i,-i, ie complex
    }
};

#endif /*TESTUBLASCUSTOMFUNCTIONS_HPP_*/
