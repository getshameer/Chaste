/*

Copyright (C) University of Oxford, 2005-2009

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


#ifndef ABSTRACTINCOMPRESSIBLEMATERIALLAW_HPP_
#define ABSTRACTINCOMPRESSIBLEMATERIALLAW_HPP_

#include "UblasCustomFunctions.hpp"
#include <cassert>
#include <vector>
#include "Exception.hpp"
#include "FourthOrderTensor2.hpp"

/**
 *  AbstractIncompressibleMaterialLaw
 *
 *  An incompressible hyperelastic material law for finite elastiticy
 *
 *  The law is given by a strain energy function W(E), where E is the strain, such
 *  that the stress T = dW/dE
 */
template<unsigned DIM>
class AbstractIncompressibleMaterialLaw
{
public :
    /**
     *  Compute the (2nd Piola Kirchoff) stress T and the stress derivative dT/dE for
     *  a given strain.
     *
     *  NOTE: the strain E is not expected to be passed in, instead the Lagrangian
     *  deformation tensor C is required (recall, E = 0.5(C-I))
     *
     *  dT/dE is a fourth-order tensor, where dT/dE[M][N][P][Q] = dT^{MN}/dE_{PQ}
     *
     *  @param C The Lagrangian deformation tensor (F^T F)
     *  @param invC The inverse of C. Should be computed by the user. (Change this?)
     *  @param pressure the current pressure
     *  @param T the stress will be returned in this parameter
     *  @param dTdE the stress derivative will be returned in this parameter, assuming
     *    the final parameter is true
     *  @param computeDTdE a boolean flag saying whether the stress derivative is
     *    required or not.
     */
    virtual void ComputeStressAndStressDerivative(c_matrix<double,DIM,DIM>& C,
                                                  c_matrix<double,DIM,DIM>& invC,
                                                  double                    pressure,
                                                  c_matrix<double,DIM,DIM>& T,
                                                  FourthOrderTensor2<DIM>&   dTdE,
                                                  bool                      computeDTdE)=0;


    /**
     *  Compute the Cauchy stress (the true stress), given the deformation gradient
     *  F and the pressure. The Cauchy stress is given by
     *
     *  sigma^{ij} = (1/detF) F^i_M T^{MN} F^j_N
     *
     *  where T is the 2nd Piola Kirchoff stress, dW/dE
     *
     *  @param F the deformation gradient
     *  @param pressure the pressure
     *  @param sigma an empty matrix, which will be filled in with the Cauchy stress
     *
     *  Note: the compute the material part of the stress (the pressure-independent
     *  part), just pass in pressure=0.0
     */
    void ComputeCauchyStress(c_matrix<double,DIM,DIM>& F, double pressure, c_matrix<double,DIM,DIM>& sigma);
;
    /**
     *  Compute the 1st Piola Kirchoff stress, given the deformation gradient F
     *  and the pressure. The 1st Piola Kirchoff stress given by
     *
     *  S^{Mi} = T^{MN} F^i_M,
     *
     *  where T is the 2nd PK stress, dW/dE.
     *
     *  Note that this stress is not symmetric and the least useful of the three
     *  stresses.
     *
     *  @param F the deformation gradient
     *  @param pressure the pressure
     *  @param S an empty matrix, which will be filled in with the stress
     *
     *  Note: the compute the material part of the stress (the pressure-independent
     *  part), just pass in pressure=0.0
     */
    void Compute1stPiolaKirchoffStress(c_matrix<double,DIM,DIM>& F, double pressure, c_matrix<double,DIM,DIM>& S);

    /**
     *  Compute the 2nd Piola Kirchoff stress, given the deformation tensor C
     *  and the pressure. The 2nd Piola Kirchoff stress given by
     *
     *  T^{MN} = dW/dE_{MN} = 2dW/dC_{MN}
     *
     *  @param C the Lagrange deformation tensor (C=F^T F), *not* F, and *not* E
     *  @param pressure the pressure
     *  @param T an empty matrix, which will be filled in with the stress
     *
     *  Note: to compute the material part of the stress (the pressure-independent
     *  part), just pass in pressure=0.0
     */
    void Compute2ndPiolaKirchoffStress(c_matrix<double,DIM,DIM>& C, double pressure, c_matrix<double,DIM,DIM>& T);

    /**
     *  Get the pressure corresponding to E=0, ie C=identity
     */
    virtual double GetZeroStrainPressure()=0;

    /**
     * Destructor.
     */
    virtual ~AbstractIncompressibleMaterialLaw();

    /**
     *  Set a scale factor by which (dimensional) material parameters are scaled. This method
     *  can be optionally implemented in the child class; if no implementation is made an
     *  exception is thrown. A scale factor may be used/needed to improve GMRES convergence.
     *  Note that is a material law is scaled like this any dimensionally equivalent terms
     *  (eg gravity, tractions, active tensions) must also be scaled. Also, computed pressure
     *  will come out scaled.
     * 
     *  @param scaleFactor  the scale factor
     */
    virtual void ScaleMaterialParameters(double scaleFactor);
};

#endif /*ABSTRACTINCOMPRESSIBLEMATERIALLAW_HPP_*/
