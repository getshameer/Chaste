#ifndef _WNTCELLCYCLEODESYSTEM_HPP_
#define _WNTCELLCYCLEODESYSTEM_HPP_

#include <vector>
#include <cmath>

#include "AbstractOdeSystem.hpp"
#include "CellMutationStates.hpp"

/**
 * Represents the Mirams et al. (2007-with any luck) system of ODEs.
 * Based on Swat et al. (2004) and a simple Wnt model.
 *
 * The variables are
 *
 0. r = pRb
 1. e = E2F1
 2. i = CycD (inactive)
 3. j = CycD (active)
 4. p = pRb-p
 5. c = destruction complex (Active)
 6. b1 = Beta-Catenin (from 1st allele)
 7. b2 = Beta-Catenin (from 1st allele)
 8. WntLevel
 *
 */
class WntCellCycleOdeSystem : public AbstractOdeSystem
{
private:
    // Constants for the Swat et al. (2004) Model
    double mk2d;
    double mk3d;
    double mk34d;
    double mk43d;
    double mk23d;
    double mad;
    double mJ11d;
    double mJ12d;
    double mJ13d;
    double mJ61d;
    double mJ62d;
    double mJ63d;
    double mKm1d;
    double mkpd;
    double mphi_r;
    double mphi_i;
    double mphi_j;
    double mphi_p;
    double ma2d;
    double ma3d;
    double ma4d;
    double ma5d;
    double mk16d;
    double mk61d;
    double mPhiE2F1;    
    CellMutationState mMutationState;
        
public:
    // Constructor
    WntCellCycleOdeSystem(double WntStimulus = 0.0, const CellMutationState& rMutationState = HEALTHY);
    
    // Destructor
    ~WntCellCycleOdeSystem();
    
    void Init(); //Sets up the parameter values
    
    void SetMutationState(const CellMutationState &rMutationState);
    
    /** 
     * Called by the archive function on the wnt cell cycle model.
     * @return mMutationState the mutation state of the cell defined by 
     * CellMutationStates.hpp
     */
    CellMutationState& rGetMutationState();
    
    // Compute the RHS of the WntCellCycle system of ODEs
    void EvaluateYDerivatives(double time, const std::vector<double> &rY, std::vector<double> &rDY);
    
    /**
     * This also contains a calculation of dY[1], copied from EvaluateYDerivatives.
     * Ensure they do not get out of sync!
     */
    bool CalculateStoppingEvent(double time, const std::vector<double> &rY);
    
};
#endif //_WNTCELLCYCLEODESYSTEM_HPP_
