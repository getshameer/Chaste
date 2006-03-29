#ifndef _NONLINEARHEATEQUATION2PDE_HPP_
#define _NONLINEARHEATEQUATION2PDE_HPP_

#include "AbstractNonlinearEllipticPde.hpp"

/**
 *  A simple nonlinear elliptic PDE used by tests; Grad.(1/u Grad u) + 1 = 0
 */

template <int SPACE_DIM>
class NonlinearHeatEquation2Pde : public AbstractNonlinearEllipticPde<SPACE_DIM>
{
public:

    double ComputeLinearSourceTerm(Point<SPACE_DIM> )
    {
    	return 1.0;
    }
    
    double ComputeNonlinearSourceTerm(Point<SPACE_DIM> , double )
    {
    	return 0.0;
    }

    MatrixDouble ComputeDiffusionTerm(Point<SPACE_DIM> , double u)
    {
		return MatrixDouble::Identity(SPACE_DIM) * (1.0/u);
    }
    
    MatrixDouble ComputeDiffusionTermPrime(Point<SPACE_DIM> , double u)
    {
		return MatrixDouble::Identity(SPACE_DIM) * (-1.0/pow(u,2));
    }
    
    double ComputeNonlinearSourceTermPrime(Point<SPACE_DIM> , double )
    {
    	return 0.0;
    }
};

#endif //_NONLINEARHEATEQUATION2PDE_HPP_
