#ifndef _TESTTIMEDEPENDENTDIFFUSIONEQUATIONPDE_HPP_
#define _TESTTIMEDEPENDENTDIFFUSIONEQUATIONPDE_HPP_

#include <cxxtest/TestSuite.h>
#include "HeatEquation.hpp"

class TestHeatEquation : public CxxTest::TestSuite
{
public:

    void TestHeatEquationMethod()
    {
        ChastePoint<1> zero1(0);
        ChastePoint<2> zero2(0,0);
        ChastePoint<3> zero3(0,0,0);
        double u = 2.0;
        
        HeatEquation<1> pde1;
        HeatEquation<2> pde2;
        HeatEquation<3> pde3;
        
        TS_ASSERT_DELTA(pde1.ComputeNonlinearSourceTerm(zero1,u), 0.0, 1e-12);
        TS_ASSERT_DELTA(pde2.ComputeNonlinearSourceTerm(zero2,u), 0.0, 1e-12);
        TS_ASSERT_DELTA(pde3.ComputeNonlinearSourceTerm(zero3,u), 0.0, 1e-12);
        
        TS_ASSERT_DELTA(pde1.ComputeDuDtCoefficientFunction(zero1), 1.0, 1e-12);
        TS_ASSERT_DELTA(pde2.ComputeDuDtCoefficientFunction(zero2), 1.0, 1e-12);
        TS_ASSERT_DELTA(pde3.ComputeDuDtCoefficientFunction(zero3), 1.0, 1e-12);
        
        // diffusion matrices should be equal to identity
        c_matrix<double, 1, 1> diff1 = pde1.ComputeDiffusionTerm(zero1);
        c_matrix<double, 2, 2> diff2 = pde2.ComputeDiffusionTerm(zero2);
        c_matrix<double, 3, 3> diff3 = pde3.ComputeDiffusionTerm(zero3);
        
        TS_ASSERT_DELTA(diff1(0,0), 1, 1e-12);
        
        TS_ASSERT_DELTA(diff2(0,0), 1, 1e-12);
        TS_ASSERT_DELTA(diff2(1,1), 1, 1e-12);
        TS_ASSERT_DELTA(diff2(0,1), 0, 1e-12);
        
        TS_ASSERT_DELTA(diff3(0,0), 1, 1e-12);
        TS_ASSERT_DELTA(diff3(1,1), 1, 1e-12);
        TS_ASSERT_DELTA(diff3(2,2), 1, 1e-12);
        TS_ASSERT_DELTA(diff3(0,1), 0, 1e-12);
        TS_ASSERT_DELTA(diff3(0,2), 0, 1e-12);
        TS_ASSERT_DELTA(diff3(1,2), 0, 1e-12);

        Node<1> node(0, zero1);
        TS_ASSERT_DELTA(pde1.ComputeLinearSourceTermAtNode(node), 0.0, 1e-12);
        TS_ASSERT_DELTA(pde1.ComputeNonlinearSourceTermAtNode(node,u), 0.0, 1e-12);
    }
};

#endif //_TESTTIMEDEPENDENTDIFFUSIONEQUATIONPDE_HPP_
