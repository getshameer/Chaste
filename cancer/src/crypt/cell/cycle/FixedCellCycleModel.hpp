#ifndef FIXEDCELLCYCLEMODEL_HPP_
#define FIXEDCELLCYCLEMODEL_HPP_

#include "AbstractCellCycleModel.hpp"

/**
 *  Fixed cell cycle model
 *
 *  Cell cycle time is deterministic for stem and transit cells (with values
 *  CancerParameters::StemCellCycleTime and CancerParameters::TransitCellCycleTime)
 */
class FixedCellCycleModel : public AbstractCellCycleModel
{
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
        archive & boost::serialization::base_object<AbstractCellCycleModel>(*this);
    }
public:
    virtual bool ReadyToDivide();
    
    virtual void ResetModel();
    
    virtual void SetBirthTime(double birthTime);
    
    AbstractCellCycleModel *CreateCellCycleModel();
    
    FixedCellCycleModel();
};

// declare identifier for the serializer
BOOST_CLASS_EXPORT(FixedCellCycleModel)


#endif /*FIXEDCELLCYCLEMODEL_HPP_*/
