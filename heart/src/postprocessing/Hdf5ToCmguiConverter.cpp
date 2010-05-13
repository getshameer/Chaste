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

#include "Hdf5ToCmguiConverter.hpp"
#include "CmguiMeshWriter.hpp"
#include "UblasCustomFunctions.hpp"
#include "HeartConfig.hpp"
#include "PetscTools.hpp"
#include "Exception.hpp"
#include "ReplicatableVector.hpp"
#include "DistributedVector.hpp"
#include "DistributedVectorFactory.hpp"
#include "Version.hpp"
#include "GenericMeshReader.hpp"    


template <unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void Hdf5ToCmguiConverter<ELEMENT_DIM,SPACE_DIM>::Write(std::string type)
{
    assert(type=="Mono" || type=="Bi");
    out_stream p_file=out_stream(NULL);

    unsigned num_nodes = this->mpReader->GetNumberOfRows();
    unsigned num_timesteps = this->mpReader->GetUnlimitedDimensionValues().size();

    DistributedVectorFactory factory(num_nodes);

    Vec data = factory.CreateVec();//for V
    Vec data_phie = factory.CreateVec();//for phi_e

    for (unsigned time_step=0; time_step<num_timesteps; time_step++)
    {
        //create the file for this time step
        std::stringstream time_step_string;
        //unsigned to string
        time_step_string << time_step;
        if (PetscTools::AmMaster())
        {
            p_file = this->mpOutputFileHandler->OpenOutputFile(this->mFileBaseName + "_" + time_step_string.str() + ".exnode");
        }

        //read the data for this time step
        this->mpReader->GetVariableOverNodes(data, "V", time_step);
        ReplicatableVector repl_data(data);
        assert(repl_data.GetSize()==num_nodes);

        //get the data for phie only if needed
        ReplicatableVector repl_data_phie;
        if (type=="Bi")
        {
            repl_data_phie.Resize(num_nodes);
            this->mpReader->GetVariableOverNodes(data_phie, "Phi_e", time_step);
            repl_data_phie.ReplicatePetscVector(data_phie);
        }

        if(PetscTools::AmMaster())
        {
        	//write provenance info
		    std::string comment = "! " + ChasteBuildInfo::GetProvenanceString();
		    *p_file << comment;
            //The header first
            *p_file << "Group name: " << this->mFileBaseName << "\n";

            //we need two fields for bidomain and one only for monodomain
            if(type=="Mono")
            {
               *p_file << "#Fields=1" << "\n" << " 1) " << "V , field, rectangular cartesian, #Components=1" << "\n" << "x.  Value index=1, #Derivatives=0, #Versions=1"<<"\n";
            }
            else
            {
               *p_file << "#Fields=2" << "\n" << " 1) " << "V , field, rectangular cartesian, #Components=1" << "\n" << "x.  Value index=1, #Derivatives=0, #Versions=1"<<"\n";
                //the details of the second field
               *p_file << "\n" << " 2) " << "Phi_e , field, rectangular cartesian, #Components=1" << "\n" << "x.  Value index=1, #Derivatives=0, #Versions=1"<<"\n";
            }

            //write the data
            for(unsigned i=0; i<num_nodes; i++)
            {
                //cmgui counts nodes from 1
                *p_file << "Node: "<< i+1 << "\n" << repl_data[i] << "\n";
                //if it is a bidomain simulation, write the data for phie
                if (type=="Bi")
                {
                    *p_file <<  repl_data_phie[i] << "\n";
                }
            }
        }
    }
    VecDestroy(data);
    VecDestroy(data_phie);

    if(PetscTools::AmMaster())
    {
        p_file->close();
    }
}

template <unsigned ELEMENT_DIM, unsigned SPACE_DIM>
Hdf5ToCmguiConverter<ELEMENT_DIM,SPACE_DIM>::Hdf5ToCmguiConverter(std::string inputDirectory,
                          std::string fileBaseName,
                          AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM> *pMesh,
                          bool hasBath) :
                    AbstractHdf5Converter<ELEMENT_DIM,SPACE_DIM>(inputDirectory, fileBaseName, pMesh, "cmgui_output")
{
    //Used to inform the mesh of the data names
    std::vector<std::string> field_names;
    field_names.push_back("V");
    Write("Mono");
    if(this->mNumVariables==2)
    {
        Write("Bi");
        field_names.push_back("Phi_e");
    }

    //Write mesh in a suitable form for cmgui
    std::string output_directory =  HeartConfig::Instance()->GetOutputDirectory() + "/cmgui_output";
    
    CmguiMeshWriter<ELEMENT_DIM,SPACE_DIM> cmgui_mesh_writer(output_directory, HeartConfig::Instance()->GetOutputFilenamePrefix(), false);
    cmgui_mesh_writer.SetAdditionalFieldNames(field_names);
    if (hasBath)
    {
        std::vector<std::string> names;
        names.push_back("tissue");
        names.push_back("bath");
        cmgui_mesh_writer.SetRegionNames(names);
    }
    
   
    // Normally the in-memory mesh is converted:
    if (HeartConfig::Instance()->GetOutputWithOriginalMeshPermutation() == false )
    {
        cmgui_mesh_writer.WriteFilesUsingMesh(*(this->mpMesh));
    }
    else
    {
        //In this case we expect the mesh to have been read in from file
        ///\todo What if the mesh has been scaled, translated or rotated?
        //Note that the next line will throw if the mesh has not been read from file
        std::string original_file=this->mpMesh->GetMeshFileBaseName();
        GenericMeshReader<ELEMENT_DIM, SPACE_DIM> original_mesh_reader(original_file);
        cmgui_mesh_writer.WriteFilesUsingMeshReader(original_mesh_reader);
    }
    
    PetscTools::Barrier("Hdf5ToCmguiConverter");
}

/////////////////////////////////////////////////////////////////////
// Explicit instantiation
/////////////////////////////////////////////////////////////////////

template class Hdf5ToCmguiConverter<1,1>;
template class Hdf5ToCmguiConverter<1,2>;
template class Hdf5ToCmguiConverter<2,2>;
template class Hdf5ToCmguiConverter<1,3>;
template class Hdf5ToCmguiConverter<2,3>;
template class Hdf5ToCmguiConverter<3,3>;
