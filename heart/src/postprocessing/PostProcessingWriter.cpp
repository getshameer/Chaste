/*

Copyright (C) University of Oxford, 2005-2011

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

#include "UblasCustomFunctions.hpp"
#include "HeartConfig.hpp"
#include "PostProcessingWriter.hpp"
#include "PetscTools.hpp"
#include "OutputFileHandler.hpp"
#include "DistanceMapCalculator.hpp"
#include "PseudoEcgCalculator.hpp"
#include "Version.hpp"
#include "HeartEventHandler.hpp"

#include <iostream>

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
PostProcessingWriter<ELEMENT_DIM, SPACE_DIM>::PostProcessingWriter(AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>& rMesh,
                                                                   std::string directory,
                                                                   std::string hdf5File,
                                                                   bool makeAbsolute,
                                                                   std::string voltageName)
        : mDirectory(directory),
          mHdf5File(hdf5File),
          mMakeAbsolute(makeAbsolute),
          mVoltageName(voltageName),
          mrMesh(rMesh)
{
    mLo = mrMesh.GetDistributedVectorFactory()->GetLow();
    mHi = mrMesh.GetDistributedVectorFactory()->GetHigh();
    mpDataReader = new Hdf5DataReader(directory, hdf5File, makeAbsolute);
    mpCalculator = new PropagationPropertiesCalculator(mpDataReader,voltageName);
    //check that the hdf file was generated by simulations from the same mesh
    assert(mpDataReader->GetNumberOfRows() == mrMesh.GetNumNodes());
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void PostProcessingWriter<ELEMENT_DIM, SPACE_DIM>::WritePostProcessingFiles()
{
// Please note that only the master processor should write to file.
// Each of the private methods called here takes care of checking.

    if (HeartConfig::Instance()->IsApdMapsRequested())
    {
        std::vector<std::pair<double,double> > apd_maps;
        HeartConfig::Instance()->GetApdMaps(apd_maps);
        for (unsigned i=0; i<apd_maps.size(); i++)
        {
            WriteApdMapFile(apd_maps[i].first, apd_maps[i].second);
        }
    }

    if (HeartConfig::Instance()->IsUpstrokeTimeMapsRequested())
    {
        std::vector<double> upstroke_time_maps;
        HeartConfig::Instance()->GetUpstrokeTimeMaps(upstroke_time_maps);
        for (unsigned i=0; i<upstroke_time_maps.size(); i++)
        {
            WriteUpstrokeTimeMap(upstroke_time_maps[i]);
        }
    }

    if (HeartConfig::Instance()->IsMaxUpstrokeVelocityMapRequested())
    {
        std::vector<double> upstroke_velocity_maps;
        HeartConfig::Instance()->GetMaxUpstrokeVelocityMaps(upstroke_velocity_maps);
        for (unsigned i=0; i<upstroke_velocity_maps.size(); i++)
        {
            WriteMaxUpstrokeVelocityMap(upstroke_velocity_maps[i]);
        }
    }

    if (HeartConfig::Instance()->IsConductionVelocityMapsRequested())
    {
        std::vector<unsigned> conduction_velocity_maps;
        HeartConfig::Instance()->GetConductionVelocityMaps(conduction_velocity_maps);

        //get the mesh here
        DistanceMapCalculator<ELEMENT_DIM, SPACE_DIM> dist_map_calculator(mrMesh);

        for (unsigned i=0; i<conduction_velocity_maps.size(); i++)
        {
            std::vector<double> distance_map;
            std::vector<unsigned> origin_surface;
            origin_surface.push_back(conduction_velocity_maps[i]);
            dist_map_calculator.ComputeDistanceMap(origin_surface, distance_map);
            WriteConductionVelocityMap(conduction_velocity_maps[i], distance_map);
        }
    }
    
    if (HeartConfig::Instance()->IsAnyNodalTimeTraceRequested())
    {
        std::vector<unsigned> requested_nodes;
        HeartConfig::Instance()->GetNodalTimeTraceRequested(requested_nodes);
        WriteVariablesOverTimeAtNodes(requested_nodes);
    }

    if (HeartConfig::Instance()->IsPseudoEcgCalculationRequested())
    {
        std::vector<ChastePoint<SPACE_DIM> > electrodes;
        HeartConfig::Instance()->GetPseudoEcgElectrodePositions(electrodes);
        
        for (unsigned i=0; i<electrodes.size(); i++)
        {
            PseudoEcgCalculator<ELEMENT_DIM,SPACE_DIM,1> calculator(mrMesh, electrodes[i], mDirectory, mHdf5File, mVoltageName, mMakeAbsolute);
            calculator.WritePseudoEcg();
        }
    }
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
PostProcessingWriter<ELEMENT_DIM, SPACE_DIM>::~PostProcessingWriter()
{
    delete mpDataReader;
    delete mpCalculator;
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void PostProcessingWriter<ELEMENT_DIM, SPACE_DIM>::WriteApdMapFile(double repolarisationPercentage, double threshold)
{
    std::vector<std::vector<double> > output_data = mpCalculator->CalculateAllActionPotentialDurationsForNodeRange(repolarisationPercentage, mLo, mHi, threshold);
    std::stringstream filename_stream;
    filename_stream << "Apd_" << repolarisationPercentage << "_" << threshold << "_Map.dat";
    WriteGenericFile(output_data, filename_stream.str());
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void PostProcessingWriter<ELEMENT_DIM, SPACE_DIM>::WriteUpstrokeTimeMap(double threshold)
{
    std::vector<std::vector<double> > output_data;
    //Fill in data
    for (unsigned node_index = mLo; node_index < mHi; node_index++)
    {
        std::vector<double> upstroke_times;
        try
        {
            upstroke_times = mpCalculator->CalculateUpstrokeTimes(node_index, threshold);
            assert(upstroke_times.size() != 0);
        }
        catch(Exception& e)
        {
            upstroke_times.push_back(0);
            assert(upstroke_times.size() == 1);
        }
        output_data.push_back(upstroke_times);
    }
    std::stringstream filename_stream;
    filename_stream << "UpstrokeTimeMap_" << threshold << ".dat";
    WriteGenericFile(output_data, filename_stream.str());
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void PostProcessingWriter<ELEMENT_DIM, SPACE_DIM>::WriteMaxUpstrokeVelocityMap(double threshold)
{
    std::vector<std::vector<double> > output_data;
    //Fill in data
    for (unsigned node_index = mLo; node_index < mHi; node_index++)
    {
        std::vector<double> upstroke_velocities;
        try
        {
            upstroke_velocities = mpCalculator->CalculateAllMaximumUpstrokeVelocities(node_index, threshold);
            assert(upstroke_velocities.size() != 0);
        }
        catch(Exception& e)
        {
            upstroke_velocities.push_back(0);
            assert(upstroke_velocities.size() ==1);
        }
        output_data.push_back(upstroke_velocities);
    }
    std::stringstream filename_stream;
    filename_stream << "MaxUpstrokeVelocityMap_" << threshold << ".dat";
    WriteGenericFile(output_data, filename_stream.str());
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void PostProcessingWriter<ELEMENT_DIM, SPACE_DIM>::WriteConductionVelocityMap(unsigned originNode, std::vector<double> distancesFromOriginNode)
{
    //Fill in data
    std::vector<std::vector<double> > output_data;
    for (unsigned dest_node = mLo; dest_node < mHi; dest_node++)
    {
        std::vector<double> conduction_velocities;
        try
        {
            conduction_velocities = mpCalculator->CalculateAllConductionVelocities(originNode, dest_node, distancesFromOriginNode[dest_node]);
            assert(conduction_velocities.size() != 0);
        }
        catch(Exception& e)
        {
            conduction_velocities.push_back(0);
            assert(conduction_velocities.size() == 1);
        }
        output_data.push_back(conduction_velocities);
    }
    std::stringstream filename_stream;
    filename_stream << "ConductionVelocityFromNode" << originNode << ".dat";
    WriteGenericFile(output_data, filename_stream.str());
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void PostProcessingWriter<ELEMENT_DIM, SPACE_DIM>::WriteAboveThresholdDepolarisationFile(double threshold )
{
    std::vector<std::vector<double> > output_data;

    //Fill in data
    for (unsigned node_index = mLo; node_index < mHi; node_index++)
    {
        std::vector<double> upstroke_velocities;
        std::vector<unsigned> above_threshold_depolarisations;
        std::vector<double> output_item;
        bool no_upstroke_occurred = false;

        try
        {
            upstroke_velocities = mpCalculator->CalculateAllMaximumUpstrokeVelocities(node_index, threshold);
            assert(upstroke_velocities.size() != 0);
        }
        catch(Exception& e)
        {
            upstroke_velocities.push_back(0);
            assert(upstroke_velocities.size() ==1);
            no_upstroke_occurred = true;
        }
        //this method won't throw any exception, so there is no need to put it into the try/catch
        above_threshold_depolarisations =  mpCalculator->CalculateAllAboveThresholdDepolarisations(node_index, threshold);

        //count the total above threshold depolarisations
        unsigned total_number_of_above_threshold_depolarisations = 0;
        for (unsigned ead_index = 0; ead_index< above_threshold_depolarisations.size();ead_index++)
        {
            total_number_of_above_threshold_depolarisations = total_number_of_above_threshold_depolarisations + above_threshold_depolarisations[ead_index];
        }

        //for this item, puch back the number of upstrokes...
        if (no_upstroke_occurred)
        {
            output_item.push_back(0);
        }
        else
        {
            output_item.push_back(upstroke_velocities.size());
        }
        //... and the number of above thrshold depolarisations
        output_item.push_back((double) total_number_of_above_threshold_depolarisations);

        output_data.push_back(output_item);
    }
    std::stringstream filename_stream;
    filename_stream << "AboveThresholdDepolarisations" << threshold << ".dat";
    WriteGenericFile(output_data, filename_stream.str());
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void PostProcessingWriter<ELEMENT_DIM, SPACE_DIM>::WriteVariablesOverTimeAtNodes(std::vector<unsigned>& rNodeIndices)
{
    std::vector<std::string> variable_names = mpDataReader->GetVariableNames();

    //we will write one file per variable in the hdf5 file
    for (unsigned name_index=0; name_index < variable_names.size(); name_index++)
    {
        std::vector<std::vector<double> > output_data;
        if (PetscTools::AmMaster())//only master process fills the data structure
        {
            //allocate memory: NXM matrix where N = numbe rof time stpes and M number of requested nodes
            output_data.resize( mpDataReader->GetUnlimitedDimensionValues().size() );
            for (unsigned j = 0; j < mpDataReader->GetUnlimitedDimensionValues().size(); j++)
            {
                output_data[j].resize(rNodeIndices.size());
            }

            for (unsigned requested_index = 0; requested_index < rNodeIndices.size(); requested_index++)
            {
                unsigned node_index = rNodeIndices[requested_index];

                //handle permutation, if any
                if ( (mrMesh.rGetNodePermutation().size() != 0) &&
                      !HeartConfig::Instance()->GetOutputUsingOriginalNodeOrdering() )
                {
                    node_index = mrMesh.rGetNodePermutation()[ rNodeIndices[requested_index] ];
                }

                //grab the data from the hdf5 file.
                std::vector<double> time_series =  mpDataReader->GetVariableOverTime(variable_names[name_index], node_index);
                assert ( time_series.size() == mpDataReader->GetUnlimitedDimensionValues().size());

                //fill the output_data data structure
                for (unsigned time_step = 0; time_step < time_series.size(); time_step++)
                {
                    output_data[time_step][requested_index] = time_series[time_step];
                }
            }
        }
        std::stringstream filename_stream;
        filename_stream << "NodalTraces_" << variable_names[name_index] << ".dat";
        WriteGenericFile(output_data, filename_stream.str());
    }
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void PostProcessingWriter<ELEMENT_DIM, SPACE_DIM>::WriteGenericFile(std::vector<std::vector<double> >& rDataPayload, std::string fileName)
{
    OutputFileHandler output_file_handler(HeartConfig::Instance()->GetOutputDirectory() + "/output", false);
    for (unsigned writing_process=0; writing_process<PetscTools::GetNumProcs(); writing_process++)
    {
        if(PetscTools::GetMyRank() == writing_process)
        {
            out_stream p_file=out_stream(NULL);
            if (PetscTools::AmMaster())
            {
                //Open the file for the first time
                p_file = output_file_handler.OpenOutputFile(fileName);
                //write provenance info
			    std::string comment = "# " + ChasteBuildInfo::GetProvenanceString();
			    *p_file << comment;
            }
            else
            {
                //Append to the existing file
                p_file = output_file_handler.OpenOutputFile(fileName, std::ios::app);
            }
            for (unsigned line_number=0; line_number<rDataPayload.size(); line_number++)
            {
                for (unsigned i = 0; i < rDataPayload[line_number].size(); i++)
                {
                    *p_file << rDataPayload[line_number][i] << "\t";
                }
                *p_file << std::endl;
            }
            p_file->close();
        }
        //Process i+1 waits for process i to close the file
        PetscTools::Barrier();
    }//Loop in writing_process
}

/////////////////////////////////////////////////////////////////////
// Explicit instantiation
/////////////////////////////////////////////////////////////////////

template class PostProcessingWriter<1,1>;
template class PostProcessingWriter<1,2>;
template class PostProcessingWriter<2,2>;
template class PostProcessingWriter<1,3>;
//template class PostProcessingWriter<2,3>;
template class PostProcessingWriter<3,3>;
