#ifndef TYSONNOVAKCELLCYCLEMODEL_HPP_
#define TYSONNOVAKCELLCYCLEMODEL_HPP_

#include <boost/serialization/vector.hpp>

#include <iostream>

#include "AbstractOdeBasedCellCycleModel.hpp"
#include "TysonNovak2001OdeSystem.hpp"
#include "BackwardEulerIvpOdeSolver.hpp"
#include "Exception.hpp"

/**
 *  Tyson Novak cell cycle model
 *
 *  Time taken to progress through the cycle is actually deterministic as ODE system
 *  independent of external factors.
 * 
 * Note that this class uses C++'s default copying semantics, and so doesn't implement a copy constructor
 * or operator=.
 */
class TysonNovakCellCycleModel : public AbstractOdeBasedCellCycleModel
{
private:
    static BackwardEulerIvpOdeSolver msSolver;
    
    TysonNovakCellCycleModel(std::vector<double> parentProteinConcentrations, double divideTime, unsigned generation);
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
        archive & boost::serialization::base_object<AbstractOdeBasedCellCycleModel>(*this);
    }
    
public:

    TysonNovakCellCycleModel();
    
    void ResetForDivision();
    
    AbstractCellCycleModel *CreateDaughterCellCycleModel();
    
    bool SolveOdeToTime(double currentTime);
    
    double GetOdeStopTime();
    
    double GetSDuration();
    double GetG2Duration();
    double GetMDuration();
    
    void InitialiseDaughterCell();
    
};


// declare identifier for the serializer
BOOST_CLASS_EXPORT(TysonNovakCellCycleModel)


#endif /*TYSONNOVAKCELLCYCLEMODEL_HPP_*/
