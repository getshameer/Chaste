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

#ifndef MATRIXBASEDMONODOMAINSOLVER_HPP_
#define MATRIXBASEDMONODOMAINSOLVER_HPP_

#include "AbstractMonodomainSolver.hpp"
#include "MassMatrixAssembler.hpp"
#include "MonodomainCorrectionTermAssembler.hpp"

/**
 *  A monodomain solver, which uses various assemblers to set up the 
 *  monodomain linear system.
 * 
 *  Also allows state variable interpolation (SVI) to be used on elements
 *  for which it will be needed, if the appropriate HeartConfig boolean
 *  is set. See wiki page ChasteGuides/StateVariableInterpolation for more 
 *  details.
 */ 
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
class MatrixBasedMonodomainSolver
   : public AbstractMonodomainSolver<ELEMENT_DIM,SPACE_DIM> 
{
private:
    /** The mass matrix, used to computing the RHS vector */
    Mat mMassMatrix;
    
    /** The vector multiplied by the mass matrix. Ie, if the linear system to
     *  be solved is Ax=b, this vector is z where b=Mz.
     */
    Vec mVecForConstructingRhs;

    /**
     * If using state variable interpolation, points to an assembler to use in 
     * computing the correction term to apply to the RHS.
     */
    MonodomainCorrectionTermAssembler<ELEMENT_DIM,SPACE_DIM>* mpMonodomainCorrectionTermAssembler;

    /** 
     *  Implementation of SetupLinearSystem() which uses the assembler to compute the
     *  LHS matrix, but sets up the RHS vector using the mass-matrix (constructed 
     *  using a separate assembler) multiplied by a vector
     * 
     *  @param currentSolution  Solution at current time
     *  @param computeMatrix  Whether to compute the matrix of the linear system
     */
    void SetupLinearSystem(Vec currentSolution, bool computeMatrix);
    
    
public:
  
    /** Overloaded InitialiseForSolve() which calls base version but also
     *  initialises #mMassMatrix and #mVecForConstructingRhs.
     * 
     *  @param initialSolution  initial solution
     */
    void InitialiseForSolve(Vec initialSolution);


    /**
     * Constructor
     *
     * @param pMesh pointer to the mesh
     * @param pTissue pointer to the tissue
     * @param pBoundaryConditions pointer to the boundary conditions
     * @param numQuadPoints number of quadrature points (defaults to 2)
     */
    MatrixBasedMonodomainSolver(AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh,
                                MonodomainTissue<ELEMENT_DIM,SPACE_DIM>* pTissue,
                                BoundaryConditionsContainer<ELEMENT_DIM,SPACE_DIM,1>* pBoundaryConditions,
                                unsigned numQuadPoints = 2);
                     
    /**
     *  Destructor
     */
    ~MatrixBasedMonodomainSolver();
};



#endif /*MATRIXBASEDMONODOMAINSOLVER_HPP_*/
