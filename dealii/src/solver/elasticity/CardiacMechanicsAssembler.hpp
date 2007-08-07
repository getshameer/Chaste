#ifndef CARDIACMECHANICSASSEMBLER_HPP_
#define CARDIACMECHANICSASSEMBLER_HPP_

#include "FiniteElasticityAssembler.cpp"

template<unsigned DIM>
class CardiacMechanicsAssembler : public FiniteElasticityAssembler<DIM>
{
private:
    bool mAllocatedMaterialLawMemory;

    static const unsigned mNumQuadPointsInEachDimension = 3;

    /**
     *  Storage space for dTdE when T and E are in the rotated fibre-sheet frame
     */
    FourthOrderTensor<DIM>  mDTdE_fibre;

    /**
     *  The matrix P using JonW's convention. Orthogonal
     */
    Tensor<2,DIM> mFibreSheetMat;
    
    /**
     *  The transpose of P, which is also the inverse of P
     */
    Tensor<2,DIM> mTransFibreSheetMat;

    /**
     *  Total number of quadrature points in the mesh.
     */  
    unsigned mTotalQuadPoints;

    /** 
     *  Which quad point (out of total number of quad points in the mesh), AssembleOnElement
     *  is currently at. Incremented in AssembleOnElement(). Needed because AssembleOnElement
     *  needs to know which entry of mLambda and mActiveTension corresponds to the current
     *  quad point.
     */ 
    unsigned mCurrentQuadPointGlobalIndex;

    /** 
     *  The active tension, quad point-wise. NOTE: the i-th entry of this vector is
     *  assumed to be the i-th quad point obtained by looping over cells in the obvious
     *  way and then looping over quad points 
     */
    std::vector<double> mActiveTension;

    /** 
     *  The x stretch, quad point-wise. NOTE: the i-th entry of this vector is
     *  assumed to be the i-th quad point obtained by looping over cells in the obvious
     *  way and then looping over quad points 
     */
    std::vector<double> mLambda;
    
    /** Overloaded method for assembling system, which takes into account the active tensions */
    void AssembleOnElement(typename DoFHandler<DIM>::active_cell_iterator  elementIter,
                           Vector<double>&       elementRhs,
                           FullMatrix<double>&   elementMatrix,
                           bool                  assembleResidual,
                           bool                  assembleJacobian);
                           

public:
    /**
     *  Constructor
     *  
     *  @param pMesh. A pointer to the mesh. Should have a surface set as the fixed surface
     *  @param outputDirectory. The output directory, relative to TEST_OUTPUT
     *  @param pMaterialLaw. The material law for the tissue. Defaults to NULL, in which case
     *   a default material law is used.
     */
    CardiacMechanicsAssembler(Triangulation<DIM>* pMesh, 
                              std::string outputDirectory,
                              AbstractIncompressibleMaterialLaw<DIM>* pMaterialLaw = NULL);
    ~CardiacMechanicsAssembler();
    
    /** 
     *  Get the total number of quadrature points (equal to the n.q^d, where n=number of cells
     *  q is the number of quad points in each dimension, and d=DIM). 
     */
    unsigned GetTotalNumQuadPoints();

    /**
     *  Get the total number of quadrature points in each element (ie num_quad_in_each_dir^DIM)
     */
    unsigned GetNumQuadPointsPerElement();

    /** 
     *  Set the current active tensions, by quadrature point. Quad points don't have indices,
     *  so these values should be in the order given by looping over cells and then looping
     *  over quad points
     */
    void SetActiveTension(std::vector<double> activeTension);
    
    /** 
     *  Get lambda (the stretch ratio). 
     * 
     *  NOTE: the i-th entry of this vector is
     *  assumed to be the i-th quad point obtained by looping over cells in the obvious
     *  way and then looping over quad points 
     */
    std::vector<double>& GetLambda();    
    
    
    /**
     *  Specify a constant fibre-sheet rotation matrix
     * 
     *  This is really a temporary method until the fibre-sheet direction can be read in
     */
    void SetFibreSheetMatrix(Tensor<2,DIM> fibreSheetMat)
    {
        // check orthogonal
        Tensor<2,DIM> P_times_transP = fibreSheetMat * transpose(fibreSheetMat);
        for(unsigned i=0; i<DIM; i++)
        {
            for(unsigned j=0; j<DIM; j++)
            {
                double expected = i==j ? 1.0 : 0.0;
                if (fabs(P_times_transP[i][j] - expected) > 1e-9)
                {
                    EXCEPTION("Fibre-sheet matrix passed in does not seem to be orthogonal");
                }
            }
        }
        
        mFibreSheetMat = fibreSheetMat;
        mTransFibreSheetMat = transpose(mFibreSheetMat);
    }
};

#endif /*CARDIACMECHANICSASSEMBLER_HPP_*/
