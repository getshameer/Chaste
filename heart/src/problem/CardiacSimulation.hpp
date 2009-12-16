/*

Copyright (C) University of Oxford, 2005-2009

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

#ifndef CARDIACSIMULATION_HPP_
#define CARDIACSIMULATION_HPP_

// Must go first
#include "CardiacSimulationArchiver.hpp"

#include <vector>
#include <ctime>
#include <memory>

#include "UblasIncludes.hpp"

#include "MonodomainProblem.hpp"
#include "BidomainProblem.hpp"
#include "PetscTools.hpp"
#include "TimeStepper.hpp"

#include "HeartConfig.hpp"
#include "HeartConfigRelatedCellFactory.hpp"

#include "TetrahedralMesh.hpp"
#include "NonCachedTetrahedralMesh.hpp"
#include "ChastePoint.hpp"
#include "ChasteCuboid.hpp"
#include "MeshalyzerMeshWriter.hpp"
#include "TrianglesMeshWriter.hpp"

#include "OrthotropicConductivityTensors.hpp"

#include "Hdf5ToMeshalyzerConverter.hpp"
#include "PostProcessingWriter.hpp"
#include "Version.hpp"

/**
 * A class which encapsulates the executable functionality.
 * 
 * Takes in a chaste parameters xml file and runs the relevant simulation.
 */
class CardiacSimulation
{
private:
    
    /**
     * Read parameters from the HeartConfig XML file.
     * 
     * @param parameterFileName a string containing the chaste simulation parameters XML file name.
     */
    void ReadParametersFromFile(std::string parameterFileName)
    {
        try
        {
            // Try the hardcoded schema location first
            HeartConfig::Instance()->SetUseFixedSchemaLocation(true);
            HeartConfig::Instance()->SetParametersFile(parameterFileName);
        }
        catch (Exception& e)
        {
            if (e.CheckShortMessageContains("Missing file parsing configuration") == "")
            {
            // Try using the schema location given in the XML
                HeartConfig::Instance()->Reset();
                HeartConfig::Instance()->SetUseFixedSchemaLocation(false);
                HeartConfig::Instance()->SetParametersFile(parameterFileName);
            }
            else
            {
                throw e;
            }
        }    
    }
    
    template<class Problem, unsigned SPACE_DIM>
    void CreateAndRun()
    {
        Problem* p_problem;
        
        if (HeartConfig::Instance()->IsSimulationDefined())
        {
            HeartConfigRelatedCellFactory<SPACE_DIM> cell_factory;
            p_problem = new Problem(&cell_factory);

            p_problem->Initialise();
        }
        else // (HeartConfig::Instance()->IsSimulationResumed())
        {
            p_problem = CardiacSimulationArchiver<Problem>::Load(HeartConfig::Instance()->GetArchivedSimulationDir());                            
        }

        if (HeartConfig::Instance()->GetCheckpointSimulation())
        {    
            TimeStepper checkpoint_stepper(0.0, HeartConfig::Instance()->GetSimulationDuration(), HeartConfig::Instance()->GetCheckpointTimestep());
            while ( !checkpoint_stepper.IsTimeAtEnd() )
            {
                HeartConfig::Instance()->SetSimulationDuration(checkpoint_stepper.GetNextTime());                                
                p_problem->Solve();

                // This is the directory where both archive directory and partial results will live
                std::stringstream checkpoint_dir_basename;
                checkpoint_dir_basename << HeartConfig::Instance()->GetOutputDirectory() << "_checkpoints/" << HeartConfig::Instance()->GetSimulationDuration() << "ms/"; 
        
                // This is a subdirectory of the previous containing the archive for this timestep
                std::stringstream archive_foldername;
                archive_foldername << HeartConfig::Instance()->GetOutputDirectory() << "_" << HeartConfig::Instance()->GetSimulationDuration() << "ms";
                
                CardiacSimulationArchiver<Problem>::Save(*p_problem, checkpoint_dir_basename.str() + archive_foldername.str(), false);
                    
                PetscTools::Barrier(); // Make sure all the processes finished archiving before moving on.

                OutputFileHandler checkpoint_dir_basename_handler(checkpoint_dir_basename.str(), false);
                OutputFileHandler partial_output_dir_handler(HeartConfig::Instance()->GetOutputDirectory(), false);                 

                if (PetscTools::AmMaster())
                {
                    // Copy partial results into checkpoint_dir_basename
                    EXPECT0(system, "cp -r " + partial_output_dir_handler.GetOutputDirectoryFullPath() + " " + checkpoint_dir_basename_handler.GetOutputDirectoryFullPath());                                        
                }
                PetscTools::Barrier();    
    
                checkpoint_stepper.AdvanceOneTimeStep();
            }        
        }
        else
        {
            p_problem->Solve();
        }
        
        delete p_problem;
    }
    
    /**
     * Run the simulation
     */
    void Run()
    {
        switch (HeartConfig::Instance()->GetDomain())
        {
            case cp::domain_type::Mono :
            {
                switch (HeartConfig::Instance()->GetSpaceDimension())
                {
                    case 3:
                    {
                        CreateAndRun<MonodomainProblem<3>,3>();
                        break;
                    }
    
                    case 2:
                    {
                        CreateAndRun<MonodomainProblem<2>,2>();
                        break;
                    }
    
                    case 1:
                    {
                        CreateAndRun<MonodomainProblem<1>,1>();    
                        break;
                    }
                    default :
                        EXCEPTION("Monodomain space dimension not supported: should be 1, 2 or 3");
                }
                break;
            }
    
            case cp::domain_type::Bi :
            {
                switch (HeartConfig::Instance()->GetSpaceDimension())
                {
                    case 3:
                    {
                        CreateAndRun<BidomainProblem<3>,3>();
                        break;
                    }
                    case 2:
                    {
                        CreateAndRun<BidomainProblem<2>,2>();
                        break;
                    }
                    case 1:
                    {
                        CreateAndRun<BidomainProblem<1>,1>();
                        break;
                    }
                    default :
                    {
                        EXCEPTION("Bidomain space dimension not supported: should be 1, 2 or 3");
                    }
                }
                break;
            }
            default :
            {
                // If the domain is not set correctly then the XML parser will have picked it up before now!
                NEVER_REACHED;
            }
        }
    }

public:

    /**
     * Constructor
     * 
     * This also runs the simulation immediately.
     * 
     * @param parameterFileName  The name of the chaste parameters xml file to use to run a simulation (not mandatory since HeartConfig may be set by hand)
     */
    CardiacSimulation(std::string parameterFileName)
    {
        // If we have been passed an XML file then parse the XML file, otherwise throw
        if (parameterFileName == "")
        {
            EXCEPTION("No XML file name given");
        }
        ReadParametersFromFile(parameterFileName);
        Run();
        HeartEventHandler::Headings();
        HeartEventHandler::Report();
    }    
  
};

#endif /*CARDIACSIMULATION_HPP_*/
