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

#ifndef ABSTRACTFEOBJECTASSEMBLER_HPP_
#define ABSTRACTFEOBJECTASSEMBLER_HPP_

#include "LinearBasisFunction.hpp"
#include "GaussianQuadratureRule.hpp"
#include "BoundaryConditionsContainer.hpp"
#include "ReplicatableVector.hpp"
#include "DistributedVector.hpp"
#include "HeartEventHandler.hpp"
#include "AbstractTetrahedralMesh.hpp"
#include "PetscTools.hpp"

///\todo:  #1507

/** 
 *  Enumeration for defining how much interpolation (onto quadrature points) is 
 *  required by the concrete class. 
 *   
 *  CARDIAC: only interpolates the first component of the unknown (ie the voltage)
 *  NORMAL: interpolates the position X and all components of the unknown u
 *  NONLINEAR: interpolates X, u and grad(u). Also computes the gradient of the 
 *   basis functions when assembling vectors.
 */
typedef enum InterpolationLevel_
{
    CARDIAC = 0,
    NORMAL,
    NONLINEAR
} InterpolationLevel;


/**
 * 
 *  An abstract class for creating finite element vectors or matrices that are defined
 *  by integrals over the computational domain of functions of basis functions (for
 *  example, stiffness or mass matrices), that require assembly by looping over
 *  each element in the mesh and computing the elementwise integrals and adding it to
 *  the full matrix or vector.
 * 
 *  This class can be used to assemble a matrix OR a vector OR one of each. The
 *  template booleans CAN_ASSEMBLE_VECTOR and CAN_ASSEMBLE_MATRIX should be chosen
 *  accordingly.
 *  
 *  The class provides the functionality to loop over elements, perform element-wise
 *  integration (using Gaussian quadrature and linear basis functions), and add the
 *  results to the final matrix or vector. The concrete class which inherits from this
 *  must implement either COMPUTE_MATRIX_TERM or COMPUTE_VECTOR_TERM or both, which 
 *  should return the INTERGRAND, as a function of the basis functions.
 * 
 *  Adding integrals of surface elements can also be done (but only to vectors).
 * 
 *  The final template parameter defines how much interpolation (onto quadrature points)
 *  is required by the concrete class. 
 *   
 *  CARDIAC: only interpolates the first component of the unknown (ie the voltage)
 *  NORMAL: interpolates the position X and all components of the unknown u
 *  NONLINEAR: interpolates X, u and grad(u). Also computes the gradient of the 
 *   basis functions when assembling vectors. 
 */
template <unsigned ELEMENT_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM, bool CAN_ASSEMBLE_VECTOR, bool CAN_ASSEMBLE_MATRIX, InterpolationLevel INTERPOLATION_LEVEL>
class AbstractFeObjectAssembler : boost::noncopyable
{
protected:

    /** The vector to be assembled (only used if CAN_ASSEMBLE_VECTOR == true) */
    Vec mVectorToAssemble;

    /** The matrix to be assembled (only used if CAN_ASSEMBLE_MATRIX == true) */
    Mat mMatrixToAssemble;

    /** Whether to assemble the matrix (an assembler may be able to assemble matrices 
     *  (CAN_ASSEMBLE_MATRIX==true), but may not want to do so each timestep, hence
     *  this second boolean
     */
    bool mAssembleMatrix;

    /** Whether to assembler the vector */
    bool mAssembleVector;
    
    /** Whether to zero the given vector before assembly, or just add to it */
    bool mZeroVectorBeforeAssembly;

    /** Whether to add surface integral contributions to the vector */ 
    bool mApplyNeummanBoundaryConditionsToVector;
    
    /** Whether to ONLY assemble of the surface elements (defaults to false) */
    bool mOnlyAssembleOnSurfaceElements;
    
    /** Ownership range of the vector/matrix - lowest component owned */
    PetscInt mOwnershipRangeLo;
    /** Ownership range of the vector/matrix - highest component owned +1 */
    PetscInt mOwnershipRangeHi;

    /** Quadrature rule for use on normal elements */
    GaussianQuadratureRule<ELEMENT_DIM>* mpQuadRule;

    /** Quadrature rule for use on boundary elements */
    GaussianQuadratureRule<ELEMENT_DIM-1>* mpSurfaceQuadRule;

    /** Basis function for use with normal elements */
    typedef LinearBasisFunction<ELEMENT_DIM> BasisFunction;
    /** Basis function for use with boundary elements */
    typedef LinearBasisFunction<ELEMENT_DIM-1> SurfaceBasisFunction;


    /**
     *  If the matrix or vector will be dependent on a current solution, say,
     *  this is where that infomation is put.
     */
    ReplicatableVector mCurrentSolutionOrGuessReplicated;


    /**
     * Compute the derivatives of all basis functions at a point within an element.
     * This method will transform the results, for use within gaussian quadrature
     * for example.
     *
     * This is almost identical to LinearBasisFunction::ComputeTransformedBasisFunctionDerivatives,
     * except that it is also templated over SPACE_DIM and can handle cases such as 1d in 3d space.
     *
     * \todo #1319 Template LinearBasisFunction over SPACE_DIM and remove this method?
     *
     * @param rPoint The point at which to compute the basis functions. The
     *     results are undefined if this is not within the canonical element.
     * @param rInverseJacobian The inverse of the Jacobian matrix mapping the real
     *     element into the canonical element.
     * @param rReturnValue A reference to a vector, to be filled in
     * @return The derivatives of the basis functions, in local index order. Each
     *     entry is a vector (c_vector<double, SPACE_DIM> instance) giving the
     *     derivative along each axis.
     */
    void ComputeTransformedBasisFunctionDerivatives(const ChastePoint<ELEMENT_DIM>& rPoint,
                                                    const c_matrix<double, ELEMENT_DIM, SPACE_DIM>& rInverseJacobian,
                                                    c_matrix<double, SPACE_DIM, ELEMENT_DIM+1>& rReturnValue);


    /**
     *  The main assembly method. Private, should only be called through Assemble(), 
     *  AssembleMatrix() or AssembleVector() which set mAssembleMatrix, mAssembleVector 
     *  accordingly
     */ 
    void DoAssemble();

    /**
     *  Applies surface integral contributions to the vector after integrating
     *  over the elements. Requires CAN_ASSEMBLE_VECTOR==true and ComputeVectorSurfaceTerm
     *  to be implemented. Called if SetApplyNeummanBoundaryConditionsToVector()
     *  has been called. 
     */
    void ApplyNeummanBoundaryConditionsToVector();
    
    /**
     *  Useful inline function for getting an entry from the current solution vector
     *  @param nodeIndex node index
     *  @param indexOfUnknown index of unknown
     */
    virtual double GetCurrentSolutionOrGuessValue(unsigned nodeIndex, unsigned indexOfUnknown)
    {
        return mCurrentSolutionOrGuessReplicated[ PROBLEM_DIM*nodeIndex + indexOfUnknown];
    }

protected:
     /** Mesh to be solved on */
    AbstractTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>* mpMesh;

    /** A boundary conditions container saved if SetApplyNeummanBoundaryConditionsToVector()
     *  is called. Used for determining which surface elements have a Neumann bc and require
     *  integration over it
     */
    BoundaryConditionsContainer<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM>* mpBoundaryConditions;

    /**
     *  The concrete subclass can overload this and IncrementInterpolatedQuantities()
     *  if there are some quantities which need to be computed at each Gauss point.
     *  They are called in AssembleOnElement().
     */
    virtual void ResetInterpolatedQuantities()
    {}

    /**
     * The concrete subclass can overload this and ResetInterpolatedQuantities()
     * if there are some quantities which need to be computed at each Gauss point.
     * They are called in AssembleOnElement().
     *
     * @param phiI
     * @param pNode pointer to a node
     */
    virtual void IncrementInterpolatedQuantities(double phiI, const Node<SPACE_DIM>* pNode)
    {}    


    /**
     *  This method returns the matrix to be added to element stiffness matrix
     *  for a given gauss point, ie, essentially the INTERGRAND in the integral
     *  definition of the matrix. The arguments are the bases, bases gradients,
     *  x and current solution computed at the Gauss point. The returned matrix
     *  will be multiplied by the gauss weight and jacobian determinent and
     *  added to the element stiffness matrix (see AssembleOnElement()).
     *
     *   ** This method has to be implemented in the concrete class if 
     *      CAN_ASSEMBLE_MATRIX is true **
     *
     *  NOTE: for linear problems rGradU is NOT set up correctly because it should
     *  not be needed
     *
     *   @param rPhi The basis functions, rPhi(i) = phi_i, i=1..numBases.
     *   @param rGradPhi Basis gradients, rGradPhi(i,j) = d(phi_j)/d(X_i).
     *   @param rX The point in space.
     *   @param rU The unknown as a vector, u(i) = u_i.
     *   @param rGradU The gradient of the unknown as a matrix, rGradU(i,j) = d(u_i)/d(X_j).
     *   @param pElement Pointer to the element.
     */
    virtual c_matrix<double,PROBLEM_DIM*(ELEMENT_DIM+1),PROBLEM_DIM*(ELEMENT_DIM+1)> ComputeMatrixTerm(
        c_vector<double, ELEMENT_DIM+1>& rPhi,
        c_matrix<double, SPACE_DIM, ELEMENT_DIM+1>& rGradPhi,
        ChastePoint<SPACE_DIM>& rX,
        c_vector<double,PROBLEM_DIM>& rU,
        c_matrix<double, PROBLEM_DIM, SPACE_DIM>& rGradU,
        Element<ELEMENT_DIM,SPACE_DIM>* pElement)
    {
        NEVER_REACHED;
        return zero_matrix<double>(PROBLEM_DIM*(ELEMENT_DIM+1),PROBLEM_DIM*(ELEMENT_DIM+1));
    }

    /**
     *  This method returns the vector to be added to element stiffness vector
     *  for a given gauss point, ie, essentially the INTERGRAND in the integral
     *  definition of the vector. The arguments are the bases,
     *  x and current solution computed at the Gauss point. The returned vector
     *  will be multiplied by the gauss weight and jacobian determinent and
     *  added to the element stiffness matrix (see AssembleOnElement()).
     *
     *   ** This method has to be implemented in the concrete class if 
     *      CAN_ASSEMBLE_VECTOR is true **
     *
     *  NOTE: for linear problems rGradPhi and rGradU are NOT set up correctly because
     *  they should not be needed
     *
     *   @param rPhi The basis functions, rPhi(i) = phi_i, i=1..numBases
     *   @param rGradPhi Basis gradients, rGradPhi(i,j) = d(phi_j)/d(X_i)
     *   @param rX The point in space
     *   @param rU The unknown as a vector, u(i) = u_i
     *   @param rGradU The gradient of the unknown as a matrix, rGradU(i,j) = d(u_i)/d(X_j)
     *   @param pElement Pointer to the element
     */
    virtual c_vector<double,PROBLEM_DIM*(ELEMENT_DIM+1)> ComputeVectorTerm(
        c_vector<double, ELEMENT_DIM+1>& rPhi,
        c_matrix<double, SPACE_DIM, ELEMENT_DIM+1>& rGradPhi,
        ChastePoint<SPACE_DIM>& rX,
        c_vector<double,PROBLEM_DIM>& rU,
        c_matrix<double, PROBLEM_DIM, SPACE_DIM>& rGradU,
        Element<ELEMENT_DIM,SPACE_DIM>* pElement)
    {
        NEVER_REACHED;
        return zero_vector<double>(PROBLEM_DIM*(ELEMENT_DIM+1));
    }

    /**
     *  This method returns the vector to be added to element stiffness vector
     *  for a given gauss point in BoundaryElement, ie, essentially the 
     *  INTERGRAND in the boundary integral part of the definition of the vector.
     *  The arguments are the bases, x and current solution computed at the 
     *  Gauss point. The returned vector
     *  will be multiplied by the gauss weight and jacobian determinent and
     *  added to the element stiffness matrix (see AssembleOnElement()).
     *
     *   ** This method has to be implemented in the concrete class if 
     *      CAN_ASSEMBLE_VECTOR is true and SetApplyNeumannBoundaryConditionsToVector
     *      will be called.**
     *
     *   @param rSurfaceElement the element which is being considered.
     *   @param rPhi The basis functions, rPhi(i) = phi_i, i=1..numBases
     *   @param rX The point in space
     */
    virtual c_vector<double, PROBLEM_DIM*ELEMENT_DIM> ComputeVectorSurfaceTerm(
        const BoundaryElement<ELEMENT_DIM-1,SPACE_DIM>& rSurfaceElement,
        c_vector<double, ELEMENT_DIM>& rPhi,
        ChastePoint<SPACE_DIM>& rX)
    {
        NEVER_REACHED;
        return zero_vector<double>(PROBLEM_DIM*ELEMENT_DIM);
    }
    
    /**
     *  Whether to include this (volume) element when assembling. Returns true 
     *  here but can be overridden by the concrete assembler if not all
     *  elements should be included
     *  @param rElement the element
     */
    virtual bool ElementAssemblyCriterion(Element<ELEMENT_DIM,SPACE_DIM>& rElement)
    {
        return true;
    }

    /**
     *  Calculate the contribution of a single element to the linear system.
     *
     *  @param rElement The element to assemble on.
     *  @param rAElem The element's contribution to the LHS matrix is returned in this
     *     n by n matrix, where n is the no. of nodes in this element. There is no
     *     need to zero this matrix before calling.
     *  @param rBElem The element's contribution to the RHS vector is returned in this
     *     vector of length n, the no. of nodes in this element. There is no
     *     need to zero this vector before calling.
     *
     *  Called by AssembleSystem()
     *  Calls ComputeMatrixTerm() etc
     */
    virtual void AssembleOnElement(Element<ELEMENT_DIM,SPACE_DIM>& rElement,
                                   c_matrix<double, PROBLEM_DIM*(ELEMENT_DIM+1), PROBLEM_DIM*(ELEMENT_DIM+1) >& rAElem,
                                   c_vector<double, PROBLEM_DIM*(ELEMENT_DIM+1)>& rBElem);

    /**
     * Calculate the contribution of a single surface element with Neumann
     * boundary condition to the linear system.
     *
     * @param rSurfaceElement The element to assemble on.
     * @param rBSurfElem The element's contribution to the RHS vector is returned in this
     *     vector of length n, the no. of nodes in this element. There is no
     *     need to zero this vector before calling.
     */
    virtual void AssembleOnSurfaceElement(const BoundaryElement<ELEMENT_DIM-1,SPACE_DIM>& rSurfaceElement,
                                          c_vector<double, PROBLEM_DIM*ELEMENT_DIM>& rBSurfElem);


public:
    /**
     *  Constructor
     *  @param pMesh The mesh
     *  @param numQuadPoints The number of quadratures points (in each dimension) to use
     *   per element. Defaults to 2.
     */
    AbstractFeObjectAssembler(AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh, unsigned numQuadPoints=2);

    /**
     *  Whether to apply surface integrals to the vector.
     *  @param pBcc A boundary conditions container (boundary elements containing a Neumann bc
     *   will be looped over). Requires CAN_ASSEMBLE_VECTOR==true.
     */
    void SetApplyNeummanBoundaryConditionsToVector(BoundaryConditionsContainer<ELEMENT_DIM,SPACE_DIM,PROBLEM_DIM>* pBcc);

    /**
     *  If SetApplyNeummanBoundaryConditionsToVector() has been called, can call
     *  this to only apply the boundary conditions, not to assemble over the
     *  volume elements. Obviously requires CAN_ASSEMBLE_VECTOR==true 
     *  and ComputeVectorSurfaceTerm to be implemented
     *  @param onlyAssembleOnSurfaceElements whether to only assemble on the 
     *   surface elements (defaults to true)
     */
    void OnlyAssembleOnSurfaceElements(bool onlyAssembleOnSurfaceElements = true)
    {
        assert(CAN_ASSEMBLE_VECTOR);
        mOnlyAssembleOnSurfaceElements = onlyAssembleOnSurfaceElements;
    }
    
    /**
     *  Set the matrix that needs to be assembled. Requires CAN_ASSEMBLE_MATRIX==true.
     *  @param rMatToAssemble Reference to the matrix
     */
    void SetMatrixToAssemble(Mat& rMatToAssemble);

    /**
     *  Set the vector that needs to be assembled. Requires CAN_ASSEMBLE_VECTOR==true.
     *  @param rVecToAssemble Reference to the vector
     *  @param zeroVectorBeforeAssembly Whether to zero the vector before assembling 
     *   (otherwise it is just added to)
     */
    void SetVectorToAssemble(Vec& rVecToAssemble, bool zeroVectorBeforeAssembly);

    /** 
     *  Set a current solution vector the will be used in AssembleOnElement can passed
     *  up to ComputeMatrixTerm() or ComputeVectorTerm()
     *  @param currentSolution Current solution vector.
     */
    void SetCurrentSolution(Vec currentSolution);


    /** 
     *  Assemble everything that the class can assemble
     */
    void Assemble()
    {
        mAssembleMatrix = CAN_ASSEMBLE_MATRIX;
        mAssembleVector = CAN_ASSEMBLE_VECTOR;
        DoAssemble();
    }
        
    /** 
     *  Assemble the matrix. Requires CAN_ASSEMBLE_MATRIX==true and ComputeMatrixTerm() to be implemented
     */
    void AssembleMatrix()
    {
        assert(CAN_ASSEMBLE_MATRIX);
        mAssembleMatrix = true;
        mAssembleVector = false;
        DoAssemble();
    }

    /** 
     *  Assemble the vector. Requires CAN_ASSEMBLE_VECTOR==true and ComputeVectorTerm() to be implemented
     */
    void AssembleVector()
    {
        assert(CAN_ASSEMBLE_VECTOR);
        mAssembleMatrix = false;
        mAssembleVector = true;
        DoAssemble();
    }

    /**
     *  Destructor
     */
    virtual ~AbstractFeObjectAssembler()
    {
        delete mpQuadRule;
        if(mpSurfaceQuadRule)
        {
            delete mpSurfaceQuadRule;
        }
    }
};






template <unsigned ELEMENT_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM, bool CAN_ASSEMBLE_VECTOR, bool CAN_ASSEMBLE_MATRIX, InterpolationLevel INTERPOLATION_LEVEL> // class CONCRETE>
AbstractFeObjectAssembler<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM, CAN_ASSEMBLE_VECTOR, CAN_ASSEMBLE_MATRIX, INTERPOLATION_LEVEL>::AbstractFeObjectAssembler(AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh, unsigned numQuadPoints)
    : mVectorToAssemble(NULL),
      mMatrixToAssemble(NULL),
      mZeroVectorBeforeAssembly(true),
      mApplyNeummanBoundaryConditionsToVector(false),
      mOnlyAssembleOnSurfaceElements(false),
      mpMesh(pMesh),
      mpBoundaryConditions(NULL)
{
    assert(pMesh);
    assert(numQuadPoints > 0);
    assert(CAN_ASSEMBLE_VECTOR || CAN_ASSEMBLE_MATRIX);

    mpQuadRule = new GaussianQuadratureRule<ELEMENT_DIM>(numQuadPoints);

    if(CAN_ASSEMBLE_VECTOR)
    {
        mpSurfaceQuadRule = new GaussianQuadratureRule<ELEMENT_DIM-1>(numQuadPoints);
    }
    else
    {
        mpSurfaceQuadRule = NULL;
    }
}  


template <unsigned ELEMENT_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM, bool CAN_ASSEMBLE_VECTOR, bool CAN_ASSEMBLE_MATRIX, InterpolationLevel INTERPOLATION_LEVEL> //, class CONCRETE>
void AbstractFeObjectAssembler<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM, CAN_ASSEMBLE_VECTOR, CAN_ASSEMBLE_MATRIX, INTERPOLATION_LEVEL>::SetMatrixToAssemble(Mat& rMatToAssemble)
{
    assert(rMatToAssemble);
    MatGetOwnershipRange(rMatToAssemble, &mOwnershipRangeLo, &mOwnershipRangeHi);

    mMatrixToAssemble = rMatToAssemble;
}

template <unsigned ELEMENT_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM, bool CAN_ASSEMBLE_VECTOR, bool CAN_ASSEMBLE_MATRIX, InterpolationLevel INTERPOLATION_LEVEL> //, class CONCRETE>
void AbstractFeObjectAssembler<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM, CAN_ASSEMBLE_VECTOR, CAN_ASSEMBLE_MATRIX, INTERPOLATION_LEVEL>::SetVectorToAssemble(Vec& rVecToAssemble, bool zeroVectorBeforeAssembly)
{
    assert(rVecToAssemble);
    VecGetOwnershipRange(rVecToAssemble, &mOwnershipRangeLo, &mOwnershipRangeHi);

    mVectorToAssemble = rVecToAssemble;
    mZeroVectorBeforeAssembly = zeroVectorBeforeAssembly;
}


template <unsigned ELEMENT_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM, bool CAN_ASSEMBLE_VECTOR, bool CAN_ASSEMBLE_MATRIX, InterpolationLevel INTERPOLATION_LEVEL> //, class CONCRETE>
void AbstractFeObjectAssembler<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM, CAN_ASSEMBLE_VECTOR, CAN_ASSEMBLE_MATRIX, INTERPOLATION_LEVEL>::SetCurrentSolution(Vec currentSolution)
{
    assert(currentSolution != NULL);
    // Replicate the current solution and store so can be used in
    // AssembleOnElement
    
    HeartEventHandler::BeginEvent(HeartEventHandler::COMMUNICATION);
    mCurrentSolutionOrGuessReplicated.ReplicatePetscVector(currentSolution);
    HeartEventHandler::EndEvent(HeartEventHandler::COMMUNICATION);
    
    // the AssembleOnElement type methods will determine if a current solution or
    // current guess exists by looking at the size of the replicated vector, so
    // check the size is zero if there isn't a current solution
    assert( mCurrentSolutionOrGuessReplicated.GetSize()>0 );
}


template <unsigned ELEMENT_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM, bool CAN_ASSEMBLE_VECTOR, bool CAN_ASSEMBLE_MATRIX, InterpolationLevel INTERPOLATION_LEVEL> //, class CONCRETE>
void AbstractFeObjectAssembler<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM, CAN_ASSEMBLE_VECTOR, CAN_ASSEMBLE_MATRIX, INTERPOLATION_LEVEL>::DoAssemble()
{
    HeartEventHandler::EventType assemble_event;
    if (mAssembleMatrix)
    {
        assemble_event = HeartEventHandler::ASSEMBLE_SYSTEM;
    }
    else
    {
        assemble_event = HeartEventHandler::ASSEMBLE_RHS;
    }

    if(mAssembleMatrix && mMatrixToAssemble==NULL)
    {
        EXCEPTION("Matrix to be assembled has not been set");
    }
    if(mAssembleVector && mVectorToAssemble==NULL)
    {
        EXCEPTION("Vector to be assembled has not been set");
    }

    // this has to be below PrepareForAssembleSystem as in that
    // method the odes are solved in cardiac problems
    HeartEventHandler::BeginEvent(assemble_event);

    // Zero the matrix/vector if it is to be assembled
    if (mAssembleVector && mZeroVectorBeforeAssembly)
    {
        PetscVecTools::Zero(mVectorToAssemble);
    }
    if (mAssembleMatrix)
    {
        PetscMatTools::Zero(mMatrixToAssemble);
    }

    const size_t STENCIL_SIZE=PROBLEM_DIM*(ELEMENT_DIM+1);
    c_matrix<double, STENCIL_SIZE, STENCIL_SIZE> a_elem;
    c_vector<double, STENCIL_SIZE> b_elem;

    ////////////////////////////////////////////////////////
    // loop over elements
    ////////////////////////////////////////////////////////
    if(mAssembleMatrix || (mAssembleVector && !mOnlyAssembleOnSurfaceElements))
    {
        for (typename AbstractTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::ElementIterator iter = mpMesh->GetElementIteratorBegin();
             iter != mpMesh->GetElementIteratorEnd();
             ++iter)
        {
            Element<ELEMENT_DIM, SPACE_DIM>& r_element = *iter;
    
            if (ElementAssemblyCriterion(r_element)==true && r_element.GetOwnership() == true)
            {
                AssembleOnElement(r_element, a_elem, b_elem);
    
                unsigned p_indices[STENCIL_SIZE];
                r_element.GetStiffnessMatrixGlobalIndices(PROBLEM_DIM, p_indices);
    
                if (mAssembleMatrix)
                {
                    PetscMatTools::AddMultipleValues<STENCIL_SIZE>(mMatrixToAssemble, p_indices, a_elem);
                }
    
                if (mAssembleVector)
                {
                    PetscVecTools::AddMultipleValues<STENCIL_SIZE>(mVectorToAssemble, p_indices, b_elem);
                }
            }
        }
    }

    ////////////////////////////////////////////////////////
    // Apply any Neumann boundary conditions
    //
    // NB. We assume that if an element has a boundary condition on any unknown there is a boundary condition
    // on unknown 0. This can be so for any problem by adding zero constant conditions where required
    // although this is a bit inefficient. Proper solution involves changing BCC to have a map of arrays
    // boundary conditions rather than an array of maps.
    ////////////////////////////////////////////////////////


    if (mApplyNeummanBoundaryConditionsToVector && mAssembleVector)
    {
        HeartEventHandler::EndEvent(assemble_event);
        ApplyNeummanBoundaryConditionsToVector();
        HeartEventHandler::BeginEvent(assemble_event);
    }
    
    HeartEventHandler::EndEvent(assemble_event);
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM, bool CAN_ASSEMBLE_VECTOR, bool CAN_ASSEMBLE_MATRIX, InterpolationLevel INTERPOLATION_LEVEL>
void AbstractFeObjectAssembler<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM, CAN_ASSEMBLE_VECTOR, CAN_ASSEMBLE_MATRIX, INTERPOLATION_LEVEL>::SetApplyNeummanBoundaryConditionsToVector(BoundaryConditionsContainer<ELEMENT_DIM,SPACE_DIM,PROBLEM_DIM>* pBcc)
{
    assert(CAN_ASSEMBLE_VECTOR);
    mApplyNeummanBoundaryConditionsToVector = true;
    mpBoundaryConditions = pBcc;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM, bool CAN_ASSEMBLE_VECTOR, bool CAN_ASSEMBLE_MATRIX, InterpolationLevel INTERPOLATION_LEVEL>
void AbstractFeObjectAssembler<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM, CAN_ASSEMBLE_VECTOR, CAN_ASSEMBLE_MATRIX, INTERPOLATION_LEVEL>::ApplyNeummanBoundaryConditionsToVector()
{
    assert(mAssembleVector);
    assert(mpBoundaryConditions != NULL);
    
    HeartEventHandler::BeginEvent(HeartEventHandler::NEUMANN_BCS);
    if (mpBoundaryConditions->AnyNonZeroNeumannConditions())
    {
        typename BoundaryConditionsContainer<ELEMENT_DIM,SPACE_DIM,PROBLEM_DIM>::NeumannMapIterator
            neumann_iterator = mpBoundaryConditions->BeginNeumann();
        c_vector<double, PROBLEM_DIM*ELEMENT_DIM> b_surf_elem;

        // Iterate over defined conditions
        while (neumann_iterator != mpBoundaryConditions->EndNeumann())
        {
            const BoundaryElement<ELEMENT_DIM-1,SPACE_DIM>& r_surf_element = *(neumann_iterator->first);
            AssembleOnSurfaceElement(r_surf_element, b_surf_elem);

            const size_t STENCIL_SIZE=PROBLEM_DIM*ELEMENT_DIM; // problem_dim*num_nodes_on_surface_element
            unsigned p_indices[STENCIL_SIZE];
            r_surf_element.GetStiffnessMatrixGlobalIndices(PROBLEM_DIM, p_indices);
            PetscVecTools::AddMultipleValues<STENCIL_SIZE>(mVectorToAssemble, p_indices, b_surf_elem);
            ++neumann_iterator;
        }
    }
    HeartEventHandler::EndEvent(HeartEventHandler::NEUMANN_BCS);
}





///////////////////////////////////////////////////////////////////////////////////
// Implementation - AssembleOnElement and smaller
///////////////////////////////////////////////////////////////////////////////////


template <unsigned ELEMENT_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM, bool CAN_ASSEMBLE_VECTOR, bool CAN_ASSEMBLE_MATRIX, InterpolationLevel INTERPOLATION_LEVEL>
void AbstractFeObjectAssembler<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM, CAN_ASSEMBLE_VECTOR, CAN_ASSEMBLE_MATRIX, INTERPOLATION_LEVEL>::ComputeTransformedBasisFunctionDerivatives(
        const ChastePoint<ELEMENT_DIM>& rPoint,
        const c_matrix<double, ELEMENT_DIM, SPACE_DIM>& rInverseJacobian,
        c_matrix<double, SPACE_DIM, ELEMENT_DIM+1>& rReturnValue)
{
    assert(ELEMENT_DIM < 4 && ELEMENT_DIM > 0);
    static c_matrix<double, ELEMENT_DIM, ELEMENT_DIM+1> grad_phi;

    LinearBasisFunction<ELEMENT_DIM>::ComputeBasisFunctionDerivatives(rPoint, grad_phi);
    rReturnValue = prod(trans(rInverseJacobian), grad_phi);
}


template <unsigned ELEMENT_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM, bool CAN_ASSEMBLE_VECTOR, bool CAN_ASSEMBLE_MATRIX, InterpolationLevel INTERPOLATION_LEVEL>
void AbstractFeObjectAssembler<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM, CAN_ASSEMBLE_VECTOR, CAN_ASSEMBLE_MATRIX, INTERPOLATION_LEVEL>::AssembleOnElement(
    Element<ELEMENT_DIM,SPACE_DIM>& rElement,
    c_matrix<double, PROBLEM_DIM*(ELEMENT_DIM+1), PROBLEM_DIM*(ELEMENT_DIM+1) >& rAElem,
    c_vector<double, PROBLEM_DIM*(ELEMENT_DIM+1)>& rBElem)
{
    /**
     * \todo #1320 This assumes that the Jacobian is constant on an element.
     * This is true for linear basis functions, but not for any other type of
     * basis function.
     */
    c_matrix<double, SPACE_DIM, ELEMENT_DIM> jacobian;
    c_matrix<double, ELEMENT_DIM, SPACE_DIM> inverse_jacobian;
    double jacobian_determinant;

    mpMesh->GetInverseJacobianForElement(rElement.GetIndex(), jacobian, jacobian_determinant, inverse_jacobian);

    // With the new signature of GetInverseJacobianForElement, inverse and jacobian are returned at the same time
    //        // Initialise element contributions to zero
    //        if ( mAssembleMatrix || this->ProblemIsNonlinear() ) // don't need to construct grad_phi or grad_u in that case
    //        {
    //            this->mpMesh->GetInverseJacobianForElement(rElement.GetIndex(), inverse_jacobian);
    //        }

    if (mAssembleMatrix)
    {
        rAElem.clear();
    }

    if (mAssembleVector)
    {
        rBElem.clear();
    }

    const unsigned num_nodes = rElement.GetNumNodes();

    // allocate memory for the basis functions values and derivative values
    c_vector<double, ELEMENT_DIM+1> phi;
    c_matrix<double, SPACE_DIM, ELEMENT_DIM+1> grad_phi;

    // loop over Gauss points
    for (unsigned quad_index=0; quad_index < mpQuadRule->GetNumQuadPoints(); quad_index++)
    {
        const ChastePoint<ELEMENT_DIM>& quad_point = mpQuadRule->rGetQuadPoint(quad_index);

        BasisFunction::ComputeBasisFunctions(quad_point, phi);

        if ( mAssembleMatrix || INTERPOLATION_LEVEL==NONLINEAR )
        {
            ComputeTransformedBasisFunctionDerivatives(quad_point, inverse_jacobian, grad_phi);
        }

        // Location of the gauss point in the original element will be stored in x
        // Where applicable, u will be set to the value of the current solution at x
        ChastePoint<SPACE_DIM> x(0,0,0);

        c_vector<double,PROBLEM_DIM> u = zero_vector<double>(PROBLEM_DIM);
        c_matrix<double,PROBLEM_DIM,SPACE_DIM> grad_u = zero_matrix<double>(PROBLEM_DIM,SPACE_DIM);

        // allow the concrete version of the assembler to interpolate any
        // desired quantities
        ResetInterpolatedQuantities();

        /////////////////////////////////////////////////////////////
        // interpolation
        /////////////////////////////////////////////////////////////
        for (unsigned i=0; i<num_nodes; i++)
        {
            const Node<SPACE_DIM>* p_node = rElement.GetNode(i);

            if (INTERPOLATION_LEVEL != CARDIAC) // don't interpolate X if cardiac problem
            {
                const c_vector<double, SPACE_DIM>& r_node_loc = p_node->rGetLocation();
                // interpolate x
                x.rGetLocation() += phi(i)*r_node_loc;
            }

            // interpolate u and grad u if a current solution or guess exists
            unsigned node_global_index = rElement.GetNodeGlobalIndex(i);
            if (mCurrentSolutionOrGuessReplicated.GetSize()>0)
            {
                for (unsigned index_of_unknown=0; index_of_unknown<(INTERPOLATION_LEVEL!=CARDIAC ? PROBLEM_DIM : 1); index_of_unknown++)
                {
                    // If we have a current solution (e.g. this is a dynamic problem)
                    // interpolate the value at this quad point

                    // NOTE - following assumes that, if say there are two unknowns u and v, they
                    // are stored in the current solution vector as
                    // [U1 V1 U2 V2 ... U_n V_n]
                    double u_at_node=GetCurrentSolutionOrGuessValue(node_global_index, index_of_unknown);
                    u(index_of_unknown) += phi(i)*u_at_node;

                    if (INTERPOLATION_LEVEL==NONLINEAR) // don't need to construct grad_phi or grad_u in other cases
                    {
                        for (unsigned j=0; j<SPACE_DIM; j++)
                        {
                            grad_u(index_of_unknown,j) += grad_phi(j,i)*u_at_node;
                        }
                    }
                }
            }

            // allow the concrete version of the assembler to interpolate any
            // desired quantities
            IncrementInterpolatedQuantities(phi(i), p_node);
        }

        double wJ = jacobian_determinant * mpQuadRule->GetWeight(quad_index);

        ////////////////////////////////////////////////////////////
        // create rAElem and rBElem
        ////////////////////////////////////////////////////////////
        if (mAssembleMatrix)
        {
            noalias(rAElem) += ComputeMatrixTerm(phi, grad_phi, x, u, grad_u, &rElement) * wJ;
        }

        if (mAssembleVector)
        {
            noalias(rBElem) += ComputeVectorTerm(phi, grad_phi, x, u, grad_u, &rElement) * wJ;
        }
    }
}


template <unsigned ELEMENT_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM, bool CAN_ASSEMBLE_VECTOR, bool CAN_ASSEMBLE_MATRIX, InterpolationLevel INTERPOLATION_LEVEL>
void AbstractFeObjectAssembler<ELEMENT_DIM, SPACE_DIM, PROBLEM_DIM, CAN_ASSEMBLE_VECTOR, CAN_ASSEMBLE_MATRIX, INTERPOLATION_LEVEL>::AssembleOnSurfaceElement(
            const BoundaryElement<ELEMENT_DIM-1,SPACE_DIM>& rSurfaceElement,
            c_vector<double, PROBLEM_DIM*ELEMENT_DIM>& rBSurfElem)
{
    c_vector<double, SPACE_DIM> weighted_direction;
    double jacobian_determinant;
    mpMesh->GetWeightedDirectionForBoundaryElement(rSurfaceElement.GetIndex(), weighted_direction, jacobian_determinant);

    rBSurfElem.clear();

    // allocate memory for the basis function values
    c_vector<double, ELEMENT_DIM>  phi;

    // loop over Gauss points
    for (unsigned quad_index=0; quad_index<mpSurfaceQuadRule->GetNumQuadPoints(); quad_index++)
    {
        const ChastePoint<ELEMENT_DIM-1>& quad_point = mpSurfaceQuadRule->rGetQuadPoint(quad_index);

        SurfaceBasisFunction::ComputeBasisFunctions(quad_point, phi);

        /////////////////////////////////////////////////////////////
        // interpolation
        /////////////////////////////////////////////////////////////

        // Location of the gauss point in the original element will be
        // stored in x
        ChastePoint<SPACE_DIM> x(0,0,0);

        this->ResetInterpolatedQuantities();
        for (unsigned i=0; i<rSurfaceElement.GetNumNodes(); i++)
        {
            const c_vector<double, SPACE_DIM> node_loc = rSurfaceElement.GetNode(i)->rGetLocation();
            x.rGetLocation() += phi(i)*node_loc;

            // allow the concrete version of the assembler to interpolate any
            // desired quantities
            IncrementInterpolatedQuantities(phi(i), rSurfaceElement.GetNode(i));
        }

        double wJ = jacobian_determinant * mpSurfaceQuadRule->GetWeight(quad_index);

        ////////////////////////////////////////////////////////////
        // create rAElem and rBElem
        ////////////////////////////////////////////////////////////
        ///\todo #1321 Improve efficiency of Neumann BC implementation.
        noalias(rBSurfElem) += ComputeVectorSurfaceTerm(rSurfaceElement, phi, x) * wJ;
    }
}

#endif /*ABSTRACTFEOBJECTASSEMBLER_HPP_*/