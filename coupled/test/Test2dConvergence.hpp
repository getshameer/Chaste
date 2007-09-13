#ifndef TEST2DCONVERGENCE_HPP_
#define TEST2DCONVERGENCE_HPP_

#include <cxxtest/TestSuite.h>
#include "BidomainProblem.hpp"
#include "MonodomainProblem.hpp"
#include <petscvec.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <math.h>

#include "BackwardEulerLuoRudyIModel1991.hpp"
#include "LuoRudyIModel1991OdeSystem.hpp"
#include "TimeConvergenceTester.hpp"
#include "SpaceConvergenceTester.hpp"
#include "KspConvergenceTester.hpp"
#include "OdeConvergenceTester.hpp"


class Test2dConvergence : public CxxTest::TestSuite
{   
public:
    void Test2DSpace() throw(Exception)
    {
        SpaceConvergenceTester<BackwardEulerLuoRudyIModel1991, BidomainProblem<2>, 2> tester;
        tester.Converge();
        TS_ASSERT(tester.Converged);
        TS_ASSERT_EQUALS(tester.MeshNum, 5u); 
    }
};

#endif /*TEST2DCONVERGENCE_HPP_*/
