#ifndef _SIMPLEDG0PARABOLICASSEMBLER_HPP_
#define _SIMPLEDG0PARABOLICASSEMBLER_HPP_

#include <vector>
#include <iostream>
#include <petscvec.h>

#include "LinearSystem.hpp"
#include "AbstractLinearParabolicPde.hpp"
#include "AbstractLinearDynamicProblemAssembler.hpp"
#include "ConformingTetrahedralMesh.hpp"
#include "BoundaryConditionsContainer.hpp"
#include "AbstractLinearSolver.hpp"
#include "GaussianQuadratureRule.hpp"

/**
 *  SimpleDg0ParabolicAssembler
 *
 *  Assembler for solving AbstractLinearParabolicPdes
 */
template<int ELEMENT_DIM, int SPACE_DIM>
class SimpleDg0ParabolicAssembler : public AbstractLinearDynamicProblemAssembler<ELEMENT_DIM, SPACE_DIM, 1>
{
private :
    AbstractLinearParabolicPde<SPACE_DIM>* mpParabolicPde;
    
protected:

    /**
     *  The term to be added to the element stiffness matrix: 
     *  
     *   grad_phi[row] \dot ( pde_diffusion_term * grad_phi[col]) + 
     *  (1.0/mDt) * pPde->ComputeDuDtCoefficientFunction(rX) * rPhi[row] * rPhi[col]
     */
    virtual c_matrix<double,1*(ELEMENT_DIM+1),1*(ELEMENT_DIM+1)> ComputeLhsTerm(
        const c_vector<double, ELEMENT_DIM+1> &rPhi,
        const c_matrix<double, ELEMENT_DIM, ELEMENT_DIM+1> &rGradPhi,
        const Point<SPACE_DIM> &rX,
        const c_vector<double,1> &u)
    {
        c_matrix<double, ELEMENT_DIM, ELEMENT_DIM> pde_diffusion_term = mpParabolicPde->ComputeDiffusionTerm(rX);
        
        return    prod( trans(rGradPhi), c_matrix<double, ELEMENT_DIM, ELEMENT_DIM+1>(prod(pde_diffusion_term, rGradPhi)) )
                  + this->mDtInverse * mpParabolicPde->ComputeDuDtCoefficientFunction(rX) * outer_prod(rPhi, rPhi);
    }
    
    /**
     *  The term to be added to the element stiffness vector: 
     */
    virtual c_vector<double,1*(ELEMENT_DIM+1)> ComputeRhsTerm(const c_vector<double, ELEMENT_DIM+1> &rPhi,
                                                              const Point<SPACE_DIM> &rX,
                                                              const c_vector<double,1> &u)
    {
        return (mpParabolicPde->ComputeNonlinearSourceTerm(rX, u(0)) + mpParabolicPde->ComputeLinearSourceTerm(rX)
                + this->mDtInverse * mpParabolicPde->ComputeDuDtCoefficientFunction(rX) * u(0)) * rPhi;
    }
    
    
    /**
     *  The term arising from boundary conditions to be added to the element
     *  stiffness vector
     */
    virtual c_vector<double, ELEMENT_DIM> ComputeSurfaceRhsTerm(const BoundaryElement<ELEMENT_DIM-1,SPACE_DIM> &rSurfaceElement,
                                                                const c_vector<double, ELEMENT_DIM> &phi,
                                                                const Point<SPACE_DIM> &x )
    {
        // D_times_gradu_dot_n = [D grad(u)].n, D=diffusion matrix
        double D_times_gradu_dot_n = this->mpBoundaryConditions->GetNeumannBCValue(&rSurfaceElement, x);
        return phi * D_times_gradu_dot_n;
    }
    
    
public:
    /**
     * Constructor stores the mesh, pde and boundary conditons, and calls base constructor.
     */
    SimpleDg0ParabolicAssembler(ConformingTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh,
                                AbstractLinearParabolicPde<SPACE_DIM>* pPde,
                                BoundaryConditionsContainer<ELEMENT_DIM,SPACE_DIM,1>* pBoundaryConditions,
                                int numQuadPoints = 2) :
            AbstractLinearDynamicProblemAssembler<ELEMENT_DIM,SPACE_DIM,1>(numQuadPoints)
    {
        // note - we don't check any of these are NULL here (that is done in Solve() instead),
        // to allow the user or a subclass to set any of these later
        mpParabolicPde = pPde;
        this->mpMesh = pMesh;
        this->mpBoundaryConditions = pBoundaryConditions;
        
        this->mTimesSet = false;
        this->mInitialConditionSet = false;
    }
    
    /**
     * Constructor which also takes in basis functions
     */
    SimpleDg0ParabolicAssembler(ConformingTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh,
                                AbstractLinearParabolicPde<SPACE_DIM>* pPde,
                                BoundaryConditionsContainer<ELEMENT_DIM,SPACE_DIM,1>* pBoundaryConditions,
                                AbstractBasisFunction<ELEMENT_DIM> *pBasisFunction,
                                AbstractBasisFunction<ELEMENT_DIM-1> *pSurfaceBasisFunction,
                                int numQuadPoints = 2) :
            AbstractLinearDynamicProblemAssembler<ELEMENT_DIM,SPACE_DIM,1>(pBasisFunction, pSurfaceBasisFunction, numQuadPoints)
    {
        // note - we don't check any of these are NULL here (that is done in Solve() instead),
        // to allow the user or a subclass to set any of these later
        mpParabolicPde = pPde;
        this->mpMesh = pMesh;
        this->mpBoundaryConditions = pBoundaryConditions;
        
        this->mTimesSet = false;
        this->mInitialConditionSet = false;
    }
    
    /**
     * Called by AbstractLinearDynamicProblemSolver at the beginning of Solve() 
     */
    void PrepareForSolve()
    {
        assert(mpParabolicPde != NULL);
        assert(this->mpMesh != NULL);
        assert(this->mpBoundaryConditions != NULL);
    }
};


#endif //_SIMPLEDG0PARABOLICASSEMBLER_HPP_
