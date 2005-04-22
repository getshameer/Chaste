#include "RungeKutta2IvpOdeSolver.hpp"
#include "AbstractIvpOdeSolver.hpp"
#include "AbstractOdeSystem.hpp"
#include "OdeSolution.hpp"

#include <iostream>
#include <vector>
#include <cassert>

/**
 * Solves a system of ODEs using the Runge Kutta 2nd Order Initial Value Problem Ordinary Differential Equation Solver
 *  
 * @param pAbstractOdeSystem points to the concrete ODE system to be solved
 * @param startTime the time at which the initial conditions are specified
 * @param endTime the time to which the system should be solved and the solution 
 * returned
 * @param timeStep the time interval to be used by the solver
 * @param initialConditions a standard vector specifying the intial condition 
 * of each solution variable in the system 
 * 
 * 
 * @return OdeSolution is an object containing an integer of the number of 
 * equations, a std::vector of times and a std::vector of std::vectors where 
 * each of those vectors contains the solution for one variable of the ODE 
 * system at those times
 * 
 * To be used in the form:
 * 
 * RungeKutta2IvpOdeSolver mySolver
 * OdeSolution solution=mySolver->Solve(pMyOdeSystem, StartTime, EndTime, TimeStep, yInit);
 *  
*/

OdeSolution RungeKutta2IvpOdeSolver::Solve(AbstractOdeSystem* pAbstractOdeSystem, 
				double startTime,
				double endTime,
				double timeStep,
				std::vector<double> initialConditions)
{

    int num_equations = pAbstractOdeSystem->mNumberOfEquations;
    
    // Assert that the size of Initial Conditions vector = number of equations.
    assert(initialConditions.size()==num_equations);	
    
    // Assert that the timestep does not exceed the time interval.
    assert(timeStep <= endTime - startTime);
    
    // Assert that we actually have a time interval > 0 .
    assert(endTime > startTime);
    
    // Assert that we  have a timestep > 0 .
    assert(timeStep > 0.0);
    
    int num_timesteps = ((int) ((endTime - startTime)/timeStep));
    
    double last_timestep = endTime - ((double) num_timesteps)*timeStep;
    
	OdeSolution solutions;
	solutions.mNumberOfTimeSteps = num_timesteps;
	// (num_timesteps)(num_equations)
		
	solutions.mSolutions.push_back(initialConditions);
	solutions.mTime.push_back(startTime);
	
	std::vector<double> row(num_equations);	
	std::vector<double> dy(num_equations);
	
	row=initialConditions;
	
	for(int timeindex=0;timeindex<num_timesteps;timeindex++)
	{
		// Apply Runge-Kutta 2nd Order method
        
        // Work out k1
        std::vector<double> k1(num_equations);
        dy = pAbstractOdeSystem->EvaluateYDerivatives(solutions.mTime[timeindex],row);
		for(int i=0;i<num_equations; i++) 
		{
			k1[i] = timeStep*dy[i];
			k1[i] = k1[i]/2.0+row[i];
		}
		// Work out k2				
		std::vector<double> k2(num_equations);
		dy = pAbstractOdeSystem->EvaluateYDerivatives(solutions.mTime[timeindex]+timeStep/2.0,k1);
		for(int i=0;i<num_equations; i++) 
		{
			k2[i] = timeStep*dy[i];
			row[i]=row[i]+k2[i];
		}
		
		// Send out solution and time
		solutions.mSolutions.push_back(row);
		solutions.mTime.push_back(solutions.mTime[timeindex]+timeStep);
		//std::cout<<solutions.mTime[timeindex]<<"\t"<<solutions.mSolutions[timeindex][0]<<std::endl;
	}
	
	// Extra step to get to exactly endTime
	if(last_timestep>0.00001)
	{	
		solutions.mNumberOfTimeSteps=num_timesteps+1;
		
		std::vector<double> k1(num_equations);
        dy = pAbstractOdeSystem->EvaluateYDerivatives(solutions.mTime[num_timesteps],row);
		for(int i=0;i<num_equations; i++) 
		{
			k1[i] = last_timestep*dy[i];
			k1[i] = k1[i]/2.0+row[i];
		}
		// Work out k2				
		std::vector<double> k2(num_equations);
		dy = pAbstractOdeSystem->EvaluateYDerivatives(solutions.mTime[num_timesteps]+last_timestep/2.0,k1);
		for(int i=0;i<num_equations; i++) 
		{
			k2[i] = last_timestep*dy[i];
			row[i]=row[i]+k2[i];
		}
		
		solutions.mSolutions.push_back(row);
		
		solutions.mTime.push_back(solutions.mTime[num_timesteps]+last_timestep);
		//std::cout<<solutions.mTime[num_timesteps+1]<<"\t"<<solutions.mSolutions[num_timesteps+1][0]<<std::endl;
	}
	
			
	return solutions;
}
