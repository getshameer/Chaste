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

#ifndef ABSTRACTCARDIACCELL_HPP_
#define ABSTRACTCARDIACCELL_HPP_

#include "ChasteSerialization.hpp"
#include "ClassIsAbstract.hpp"
#include <boost/serialization/base_object.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/version.hpp>

// This is only needed to prevent compilation errors on PETSc 2.2/Boost 1.33.1 combo
#include "UblasVectorInclude.hpp"

#include "AbstractCardiacCellInterface.hpp"
#include "AbstractOdeSystem.hpp"
#include "AbstractIvpOdeSolver.hpp"
#include "AbstractStimulusFunction.hpp"

#include <vector>

typedef enum _CellModelState
{
    STATE_UNSET = 0,
    FAST_VARS_ONLY,
    ALL_VARS
} CellModelState;

/**
 * This is the base class for ode-based cardiac cell models.
 *
 * It is essentially a cardiac-specific wrapper for ODE systems
 * providing an interface which can interact with the stimulus
 * classes and the voltage in a mono/bidomain simulation.
 *
 * Concrete classes can be autogenerated from CellML files
 * by the PyCml package, and will automatically inherit from this class.
 */
class AbstractCardiacCell : public AbstractCardiacCellInterface, public AbstractOdeSystem
{
private:
    /** Needed for serialization. */
    friend class boost::serialization::access;
    /**
     * Archive the member variables.
     *
     * @param archive
     * @param version
     */
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
        // This calls serialize on the base class.
        archive & boost::serialization::base_object<AbstractOdeSystem>(*this);
        if (version > 0)
        {
            archive & boost::serialization::base_object<AbstractCardiacCellInterface>(*this);
        }
        archive & mDt;
        archive & this->mSetVoltageDerivativeToZero;
        if (version > 0)
        {
            // Note that when loading a version 0 archive, this will be initialised to
            // false by our constructor.  So we should get a consistent (wrong) answer
            // with previous versions of Chaste when in tissue.
            archive & this->mIsUsedInTissue;
            archive & this->mHasDefaultStimulusFromCellML;
        }
        // archive & mVoltageIndex; - always set by constructor - called by concrete class
        // archive & mpOdeSolver; - always set by constructor - called by concrete class
        // archive & mpIntracellularStimulus; - always set by constructor - called by concrete class
        if (version == 0)
        {
            CheckForArchiveFix();
        }
        // Paranoia check
        assert(this->mParameters.size() == this->rGetParameterNames().size());
    }
    
    /**
     * The Luo-Rudy 1991 model saved in previous Chaste versions had a different ordering of state variables.
     * If we're loading that model, we'll need to permute the state vector.
     * This can't (easily) be done in the LR91 code itself, since that is auto-generated!
     */
    void CheckForArchiveFix();

protected:
    /** The timestep to use when simulating this cell.  Set from the HeartConfig object. */
    double mDt;

public:
    /** Create a new cardiac cell. The state variables of the cell will be 
     *  set to AbstractOdeSystemInformation::GetInitialConditions(). Note that
     *  calls to SetDefaultInitialConditions() on a particular instance of this class
     *  will not modify its state variables. You can modify them directly with 
     *  rGetStateVariables(). 
     *
     * @param pOdeSolver  the ODE solver to use when simulating this cell
     * @param numberOfStateVariables  the size of the ODE system modelling this cell
     * @param voltageIndex  the index of the transmembrane potential within the vector of state variables
     * @param pIntracellularStimulus  the intracellular stimulus current
     */
    AbstractCardiacCell(boost::shared_ptr<AbstractIvpOdeSolver> pOdeSolver,
                        unsigned numberOfStateVariables,
                        unsigned voltageIndex,
                        boost::shared_ptr<AbstractStimulusFunction> pIntracellularStimulus);

    /** Virtual destructor */
    virtual ~AbstractCardiacCell();

    /**
     * Initialise the cell:
     *  - set our state variables to the initial conditions,
     *  - resize model parameters vector.
     * 
     * @note Must be called by subclass constructors.
     */
    void Init();

    /**
     * Reset the model's state variables to the default initial conditions.
     */
    void ResetToInitialConditions();
    
    /**
     * Simulate this cell's behaviour between the time interval [tStart, tEnd],
     * with timestemp #mDt, updating the internal state variable values.
     *
     * @param tStart  beginning of the time interval to simulate
     * @param tEnd  end of the time interval to simulate
     */
    virtual void SolveAndUpdateState(double tStart, double tEnd);
    
    /**
     * Simulates this cell's behaviour between the time interval [tStart, tEnd],
     * with timestep #mDt, and return state variable values.
     *
     * @param tStart  beginning of the time interval to simulate
     * @param tEnd  end of the time interval to simulate
     * @param tSamp  sampling interval for returned results (defaults to #mDt)
     */
    virtual OdeSolution Compute(double tStart, double tEnd, double tSamp=0.0);

    /**
     * Simulates this cell's behaviour between the time interval [tStart, tEnd],
     * with timestep #mDt, but does not update the voltage.
     *
     * @param tStart  beginning of the time interval to simulate
     * @param tEnd  end of the time interval to simulate
     */
    virtual void ComputeExceptVoltage(double tStart, double tEnd);

    /** Set the transmembrane potential
     * @param voltage  new value
     */
    void SetVoltage(double voltage);

    /**
     * Get the current value of the transmembrane potential, as given
     * in our state variable vector.
     */
    double GetVoltage();

    /**
     *  [Ca_i] is needed for mechanics, so we explcitly have a Get method (rather than
     *  use a get by name type method, to avoid inefficiency when using different
     *  types of cells). This method by default throws an exception, so should be
     *  implemented in the concrete class if intracellular (cytosolic) calcium concentration is
     *  one of the state variables.
     */
    virtual double GetIntracellularCalciumConcentration();
    
    /**
     *  In electromechanics problems, the stretch is passed back to cell-model in case 
     *  mechano-electric feedback has been modelled. We define an empty method here.
     *  Stretch-dependent cell models should overload this method to use the input 
     *  stretch accordingly.
     *  @param stretch the stretch of the cell in the axial direction
     */
    virtual void SetStretch(double stretch)
    {
    }

    /**
     *  Empty method which can be over-ridden in concrete cell class which should
     *  go through the current state vector and go range checking on the values
     *  (eg check that concentrations are positive and gating variables are between
     *  zero and one). This method is called in the ComputeExceptVoltage method.
     */
    virtual void VerifyStateVariables()
    {
//// This code is for the future, but commented out at the moment due to the memory increase
//// it will introduce. See #794.
////
//// DOXYGEN DESCRIPTION NEEDS CHANGING ONCE THIS IS BROUGHT IN
////
////
//        for(std::set<unsigned>::iterator iter = mGatingVariableIndices.begin();
//            iter != mGatingVariableIndices.end();
//            ++iter)
//        {
//            double value = mStateVariables[*iter];
//            if(value<0.0)
//            {
//                std::stringstream error;
//                error << "State variable " << *iter << ", a gating variable, has gone negative";
//                EXCEPTION(DumpState(error.str()));
//            }
//            if(value>1.0)
//            {
//                std::stringstream error;
//                error << "State variable " << *iter << ", a gating variable, has become greater than one";
//                EXCEPTION(DumpState(error.str()));
//            }
//        }
//
//        for(std::set<unsigned>::iterator iter = mConcentrationIndices.begin();
//            iter != mConcentrationIndices.end();
//            ++iter)
//        {
//            if(mStateVariables[*iter] < 0.0)
//            {
//                std::stringstream error;
//                error << "State variable " << *iter << ", a concentration, has gone negative";
//                EXCEPTION(DumpState(error.str()));
//            }
//        }
    }



    ////////////////////////////////////////////////////////////////////////
    //  METHODS NEEDED BY FAST CARDIAC CELLS
    ////////////////////////////////////////////////////////////////////////

    /**
     * This should be implemented by fast/slow cardiac cell subclasses, and
     *  \li set the state
     *  \li initialise the cell
     *  \li \b SET #mNumberOfStateVariables \b CORRECTLY
     *      (as this would not have been known in the constructor.
     *
     * \note  This \e must be implemented by fast/slow cardiac cell subclasses.
     *
     * @param state  whether this cell is in fast or slow mode.
     */
    virtual void SetState(CellModelState state);

    /**
     * Set the slow variables. Should only be valid in fast mode.
     *
     * \note  This \e must be implemented by fast/slow cardiac cell subclasses.
     *
     * @param rSlowValues  values for the slow variables
     */
    virtual void SetSlowValues(const std::vector<double> &rSlowValues);

    /**
     * Get the current values of the slow variables. Should only be valid in slow mode.
     *
     * \note  This \e must be implemented by fast/slow cardiac cell subclasses.
     *
     * @param rSlowValues  will be filled in with the values of the slow variables on return.
     */
    virtual void GetSlowValues(std::vector<double>& rSlowValues);

    /** Get whether this cell is a fast or slow version.
     *
     * \note  This \e must be implemented by fast/slow cardiac cell subclasses.
     */
    virtual bool IsFastOnly();

    /**
     * In a multiscale simulation a cut-down cell model can be run:
     *  - fast values are calculated according to the CellML definition
     *  - slow values are interpolated on synchronisation time-steps.
     * There's a chance that linear interpolation/extrapolation may push
     * some gating variable out of the range [0, 1].  This method alters
     * any values which are out-of-range.
     *
     * \note  This \e must be implemented by fast/slow cardiac cell subclasses.
     *
     * @param rSlowValues A vector of the slow values for a particular cell after they have been interpolated from nearby coarse cells
     */
    virtual void AdjustOutOfRangeSlowValues(std::vector<double>& rSlowValues);

    /**
     * Get the number of slow variables for the cell model
     * (irrespective of whether in fast or slow mode).
     *
     * \note  This \e must be implemented by fast/slow cardiac cell subclasses.
     */
    virtual unsigned GetNumSlowValues();

};

CLASS_IS_ABSTRACT(AbstractCardiacCell)
BOOST_CLASS_VERSION(AbstractCardiacCell, 1)

#endif /*ABSTRACTCARDIACCELL_HPP_*/
