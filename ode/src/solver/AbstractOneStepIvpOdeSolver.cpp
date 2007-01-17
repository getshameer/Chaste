#include "AbstractOneStepIvpOdeSolver.hpp"
#include <cassert>
#include <iostream>


OdeSolution AbstractOneStepIvpOdeSolver::Solve(AbstractOdeSystem* pAbstractOdeSystem,
                                               std::vector<double>& rYValues,
                                               double startTime,
                                               double endTime,
                                               double timeStep,
                                               double timeSampling)
{
    // assert the size of the rYValues vector is correct
    assert(rYValues.size()==pAbstractOdeSystem->GetNumberOfStateVariables());
    
    // Assert that we actually have a time interval > 0 .
    assert(endTime > startTime);
    
    // Assert that we  have a timestep > 0 .
    assert(timeStep > 0.0);
    
    // Assert that we  have a timesampling > 0 and >= timestep
    assert(timeSampling >= timeStep);
    
    // Determine the number of time steps that will be required to solve the
    // ODE system (note that the current algorithm accounts for any potential
    // floating point error)
    
    mStoppingEventOccured = false;
    if ( pAbstractOdeSystem->CalculateStoppingEvent(startTime, rYValues) == true )
    {
        EXCEPTION("Stopping event is true for initial condition");
    }
    
    int number_of_time_samples;
    double current_time;
    
    number_of_time_samples = 0;
    
    current_time = startTime;
    
    while (current_time < endTime)
    {
        number_of_time_samples++;
        
        if (startTime+number_of_time_samples*timeSampling >= endTime)
        {
            current_time = endTime;
        }
        else
        {
            current_time = startTime+number_of_time_samples*timeSampling;
        }
    }
    
    // setup solutions if output is required
    
    OdeSolution solutions;
    
    solutions.SetNumberOfTimeSteps(number_of_time_samples);
    solutions.rGetSolutions().push_back(rYValues);
    solutions.rGetTimes().push_back(startTime);
    
    // Allocate working memory
    std::vector<double> working_memory(rYValues.size());
    
    // Solve the ODE system
    
    int time_step_number = 0;
    
    current_time = startTime;
    
    double to_time;
    
    
    while ( (current_time < endTime) && (!mStoppingEventOccured) )
    {
        time_step_number++;
        
        to_time = startTime+time_step_number*timeSampling;
        
        if (to_time >= endTime)
        {
            to_time = endTime;
        }
        
        InternalSolve(pAbstractOdeSystem, rYValues, working_memory, current_time, to_time, timeStep);
        
        current_time = to_time;
        if ( mStoppingEventOccured == true )
        {
            current_time = mStoppingTime;
            endTime = current_time;
            solutions.SetNumberOfTimeSteps(time_step_number);
        }
        
        // write current solution into solutions
        solutions.rGetSolutions().push_back(rYValues);
        // Push back new time into the time solution vector
        solutions.rGetTimes().push_back(current_time);
    }
    
    return solutions;
}

void AbstractOneStepIvpOdeSolver::Solve(AbstractOdeSystem* pAbstractOdeSystem,
                                        std::vector<double>& rYValues,
                                        double startTime,
                                        double endTime,
                                        double timeStep)
{
    // Allocate working memory
    std::vector<double> working_memory(rYValues.size());
    // And solve...
    InternalSolve(pAbstractOdeSystem, rYValues, working_memory, startTime, endTime, timeStep);
}


void AbstractOneStepIvpOdeSolver::InternalSolve(AbstractOdeSystem* pAbstractOdeSystem,
                       std::vector<double>& rYValues,
                       std::vector<double>& rWorkingMemory,
                       double startTime,
                       double endTime,
                       double timeStep)
{
    // Solve the ODE system
    
    double real_time_step = timeStep;
    int time_step_number = 0;
    double current_time = startTime;
    
    // Which of our vectors holds the current solution?
    // If this is true, it's in rYValues, otherwise it's in rWorkingMemory.
    bool curr_is_curr = false;
    
    // should never get here if this bool has been set to true;
    assert(!mStoppingEventOccured);
    
    while ( (current_time < endTime) && (!mStoppingEventOccured) )
    {
        time_step_number++;
        
        // Determine what the value time step should really be like
        double to_time = startTime+time_step_number*timeStep; 
        if (to_time >= endTime)
        {
            real_time_step = endTime - current_time;
            to_time = endTime;
        }
        
        curr_is_curr = not curr_is_curr;
        
        // Function that calls the appropriate one-step solver
//        std::cout << rYValues[0] << "\n";
//        std::cout << pAbstractOdeSystem->GetNumberOfStateVariables()<<"\n";
//        std::cout.flush();
        CalculateNextYValue(pAbstractOdeSystem,
                            real_time_step,
                            current_time,
                            curr_is_curr ? rYValues : rWorkingMemory,
                            curr_is_curr ? rWorkingMemory : rYValues);
                                       
        // Determine the new current time
        current_time = to_time;
        
        if ( pAbstractOdeSystem->CalculateStoppingEvent(current_time,
                                                        curr_is_curr ? rWorkingMemory : rYValues) == true )
        {
            mStoppingTime = current_time;
            mStoppingEventOccured = true;
        }
    }
    
    // Final answer must be in rYValues
    if (curr_is_curr)
    {
        rYValues.assign(rWorkingMemory.begin(), rWorkingMemory.end());
    }
}
