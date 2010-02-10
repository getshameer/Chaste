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

#include "HeartConfigRelatedCellFactory.hpp"
#include "HeartGeometryInformation.hpp"
#include "ChasteNodesList.hpp"
#include "NonCachedTetrahedralMesh.hpp"

template<unsigned SPACE_DIM>
HeartConfigRelatedCellFactory<SPACE_DIM>::HeartConfigRelatedCellFactory()
    : AbstractCardiacCellFactory<SPACE_DIM>(),
      mDefaultIonicModel(HeartConfig::Instance()->GetDefaultIonicModel())
{
    // Read and store possible region definitions
    HeartConfig::Instance()->GetIonicModelRegions(mIonicModelRegions,
                                                  mIonicModelsDefined);

    // Read and store Stimuli
    try
    {
        HeartConfig::Instance()->GetStimuli(mStimuliApplied, mStimulatedAreas);
    }
    catch (Exception& e)
    {
        // No stimuli provided in XML so we should have hit a parsing exception
        NEVER_REACHED;
    }

    // Read and store Cell Heterogeneities
    try
    {
        HeartConfig::Instance()->GetCellHeterogeneities(mCellHeterogeneityAreas,
                                                        mScaleFactorGks,
                                                        mScaleFactorIto,
                                                        mScaleFactorGkr);
    }
    catch (Exception& e)
    {
        // No cell heterogeneities provided
    }
    
}

template<unsigned SPACE_DIM>
HeartConfigRelatedCellFactory<SPACE_DIM>::~HeartConfigRelatedCellFactory()
{
    for (unsigned i = 0; i<mCellHeterogeneityAreas.size();i++)
    {
        delete mCellHeterogeneityAreas[i];
    }
}

template<unsigned SPACE_DIM>
AbstractCardiacCell* HeartConfigRelatedCellFactory<SPACE_DIM>::CreateCellWithIntracellularStimulus(
        boost::shared_ptr<AbstractStimulusFunction> intracellularStimulus,
        unsigned nodeIndex)
{
    cp::ionic_models_available_type ionic_model = mDefaultIonicModel;

    for (unsigned ionic_model_region_index = 0;
         ionic_model_region_index < mIonicModelRegions.size();
         ++ionic_model_region_index)
    {
        if ( mIonicModelRegions[ionic_model_region_index].DoesContain(this->GetMesh()->GetNode(nodeIndex)->GetPoint()) )
        {
            ionic_model = mIonicModelsDefined[ionic_model_region_index];
            break;
        }
    }

    switch(ionic_model)
    {
        case(cp::ionic_models_available_type::LuoRudyI):
        {
            return new LuoRudyIModel1991OdeSystem(this->mpSolver, intracellularStimulus);
            break;
        }

        case(cp::ionic_models_available_type::LuoRudyIBackwardEuler):
        {
            return new BackwardEulerLuoRudyIModel1991(intracellularStimulus);
            break;
        }

        case(cp::ionic_models_available_type::Fox2002BackwardEuler):
        {
            return new BackwardEulerFoxModel2002Modified(intracellularStimulus);
            break;
        }

        case(cp::ionic_models_available_type::DifrancescoNoble):
        {
            return new DiFrancescoNoble1985OdeSystem(this->mpSolver, intracellularStimulus);
            break;
        }

        case(cp::ionic_models_available_type::MahajanShiferaw):
        {
            return new Mahajan2008OdeSystem(this->mpSolver, intracellularStimulus);
            break;
        }

        case(cp::ionic_models_available_type::tenTusscher2006):
        {
            TenTusscher2006OdeSystem*  p_tt06_instance = new TenTusscher2006OdeSystem(this->mpSolver, intracellularStimulus);

            for (unsigned ht_index = 0;
                 ht_index < mCellHeterogeneityAreas.size();
                 ++ht_index)
            {
                if ( mCellHeterogeneityAreas[ht_index]->DoesContain(this->GetMesh()->GetNode(nodeIndex)->GetPoint()) )
                {
                    p_tt06_instance->SetScaleFactorGks(mScaleFactorGks[ht_index]);
                    p_tt06_instance->SetScaleFactorIto(mScaleFactorIto[ht_index]);
                    p_tt06_instance->SetScaleFactorGkr(mScaleFactorGkr[ht_index]);
                }
            }

            return p_tt06_instance;
            break;
        }
        
        case(cp::ionic_models_available_type::Maleckar):
        {
             Maleckar2009OdeSystem*  p_maleckar_instance = new Maleckar2009OdeSystem(this->mpSolver, intracellularStimulus);

            for (unsigned ht_index = 0;
                 ht_index < mCellHeterogeneityAreas.size();
                 ++ht_index)
            {
                if ( mCellHeterogeneityAreas[ht_index]->DoesContain(this->GetMesh()->GetNode(nodeIndex)->GetPoint()) )
                {
                    p_maleckar_instance->SetScaleFactorGks(mScaleFactorGks[ht_index]);
                    p_maleckar_instance->SetScaleFactorIto(mScaleFactorIto[ht_index]);
                    p_maleckar_instance->SetScaleFactorGkr(mScaleFactorGkr[ht_index]);
                }
            }

            return p_maleckar_instance;
            break;
        }
        
        case(cp::ionic_models_available_type::HodgkinHuxley):
        {
            return new HodgkinHuxleySquidAxon1952OriginalOdeSystem(this->mpSolver, intracellularStimulus);
            break;
        }

        case(cp::ionic_models_available_type::FaberRudy2000):
        {
            FaberRudy2000Version3*  p_faber_rudy_instance = new FaberRudy2000Version3(this->mpSolver, intracellularStimulus);
            for (unsigned ht_index = 0;
                 ht_index < mCellHeterogeneityAreas.size();
                 ++ht_index)
            {
                if ( mCellHeterogeneityAreas[ht_index]->DoesContain(this->GetMesh()->GetNode(nodeIndex)->GetPoint()) )
                {
                    p_faber_rudy_instance->SetScaleFactorGks(mScaleFactorGks[ht_index]);
                    p_faber_rudy_instance->SetScaleFactorIto(mScaleFactorIto[ht_index]);
                    p_faber_rudy_instance->SetScaleFactorGkr(mScaleFactorGkr[ht_index]);
                }
            }

            return p_faber_rudy_instance;
            break;
        }

        case(cp::ionic_models_available_type::FaberRudy2000Optimised):
        {
            return new FaberRudy2000Version3Optimised(this->mpSolver, intracellularStimulus);
            break;
        }

        default:
        {
           //If the ionic model is not in the current enumeration then the XML parser will have picked it up before now!
           NEVER_REACHED;
        }
    }

    return NULL;
}


template<unsigned SPACE_DIM>
AbstractCardiacCell* HeartConfigRelatedCellFactory<SPACE_DIM>::CreateCardiacCellForTissueNode(unsigned nodeIndex)
{
    boost::shared_ptr<MultiStimulus> node_specific_stimulus(new MultiStimulus());

    // Check which of the defined stimuli contain the current node
    for (unsigned stimulus_index = 0;
         stimulus_index < mStimuliApplied.size();
         ++stimulus_index)
    {
        if ( mStimulatedAreas[stimulus_index].DoesContain(this->GetMesh()->GetNode(nodeIndex)->GetPoint()) )
        {
            node_specific_stimulus->AddStimulus(mStimuliApplied[stimulus_index]);
        }
    }

    return CreateCellWithIntracellularStimulus(node_specific_stimulus, nodeIndex);
}

template<unsigned SPACE_DIM>
void HeartConfigRelatedCellFactory<SPACE_DIM>::FillInCellularTransmuralAreas()
{
    NEVER_REACHED;
}

template<>
void HeartConfigRelatedCellFactory<3u>::FillInCellularTransmuralAreas()
{
        std::string mesh_file_name = HeartConfig::Instance()->GetMeshName();
        //files containing list of nodes on each surface
        std::string epi_surface = mesh_file_name + ".epi";
        std::string lv_surface = mesh_file_name + ".lv";
        std::string rv_surface = mesh_file_name + ".rv";
        
        //these vectors will filled and then passed to the HeartGeometryinformation object
        std::vector<unsigned> epi_indices;        
        std::vector<unsigned> lv_indices;
        std::vector<unsigned> rv_indices;
        
        //since, at the moment, our reader is expecting a proper triangulation file
        //we need to create the vectors to be passed to the HeartGeometryInformation 
        ///\todo implement a reader of this type of files in the HeartGeometryinformation class instead of this messy code
        std::ifstream file_epi;
        file_epi.open(epi_surface.c_str());
        assert(file_epi.is_open());
        do
        {
            unsigned value;
            file_epi >> value;
            epi_indices.push_back(value);            
        }
        while(!file_epi.eof());

        std::ifstream file_lv;
        file_lv.open(lv_surface.c_str());
        assert(file_lv.is_open());
        do
        {
            unsigned value;
            file_lv >> value;
            lv_indices.push_back(value);            
        }
        while(!file_lv.eof());

        std::ifstream file_rv;
        file_rv.open(rv_surface.c_str());
        assert(file_rv.is_open());
        do
        {
            unsigned value;
            file_rv >> value;
            rv_indices.push_back(value);            
        }
        while(!file_rv.eof());
        
        TrianglesMeshReader<3,3> mesh_reader(mesh_file_name);
        //TetrahedralMesh<3u,3u>* p_mesh = dynamic_cast<TetrahedralMesh<3u,3u>*> (this->GetMesh());
        NonCachedTetrahedralMesh<3u,3u> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        //create the HeartGeometryInformation object
        HeartGeometryInformation<3u> info(mesh, epi_indices, lv_indices, rv_indices);
        
        //We need the fractions of epi and endo layer supplied by the user
        double epi_fraction = HeartConfig::Instance()->GetEpiLayerFraction();
        double endo_fraction = HeartConfig::Instance()->GetEndoLayerFraction();

        //given the fraction of each layer, compute the distance map and fill in the vector
        info.DetermineLayerForEachNode(epi_fraction,endo_fraction);
        //get the big heterogeneity vector
        std::vector<unsigned> heterogeneity_node_list;
        for (unsigned index=0; index<mesh.GetNumNodes(); index++)
        {
            heterogeneity_node_list.push_back(info.rGetLayerForEachNode()[index]);
        }
        
        std::vector<Node<3u>*> epi_nodes;
        std::vector<Node<3u>*> mid_nodes;
        std::vector<Node<3u>*> endo_nodes;

        //create the list of (pointer to object) nodes in each layer from the heterogeneities vector that was just filled in
        for (unsigned node_index = 0; node_index < this->GetMesh()->GetNumNodes(); node_index++)
        {
            unsigned int unpermuted_index = node_index;
            //now get the old, unpermuted node (this is because the endo_node_list needs it as it will be used in sequential).           
            for (unsigned index=0; index < this->GetMesh()->rGetNodePermutation().size(); index++)
            {
              if (this->GetMesh()->rGetNodePermutation()[index] == node_index)
              {
                unpermuted_index = index;
                //assert(0);
                break;
              }
            }
            //unsigned permuted_index = this->GetMesh()->rGetNodePermutation()[node_index]  ;
//            std::cout<<heterogeneity_node_list[unpermuted_index]<<std::endl;
            switch (heterogeneity_node_list[node_index])
            {
                //epi
                case 2u:
                {
                    epi_nodes.push_back(this->GetMesh()->GetNode(unpermuted_index));
                    break;
                }
                //mid
                case 1u:
                {
                    mid_nodes.push_back(this->GetMesh()->GetNode(unpermuted_index));
                    break;
                }
                //endo
                case 0u:
                {
                    endo_nodes.push_back(this->GetMesh()->GetNode(unpermuted_index));
                    break;
                }
                default:
                NEVER_REACHED;  
            }
                    
        }
        assert((endo_nodes.size()+epi_nodes.size()+mid_nodes.size())==this->GetMesh()->GetNumNodes());
        
        // now the 3 list of pointer to nodes need to be pushed into the mCellHeterogeneityAreas vector, 
        // IN THE ORDER PRESCRIBED BY THE USER IN THE XML FILE! 
        // This is because the corresponding scale factors are already read in that order.
        
        //these three unsigned tell us in which order the user supplied each layer in the XML file
        unsigned user_supplied_epi_index = HeartConfig::Instance()->GetEpiLayerIndex();
        unsigned user_supplied_mid_index = HeartConfig::Instance()->GetMidLayerIndex();
        unsigned user_supplied_endo_index = HeartConfig::Instance()->GetEndoLayerIndex();
        
        //these three should have been set to 0, 1 and 2 by HeartConfig::GetCellHeterogeneities
        assert(user_supplied_epi_index<3);
        assert(user_supplied_mid_index<3);
        assert(user_supplied_endo_index<3);
        
        //pute them in a vector
        std::vector<unsigned> user_supplied_indices;
        user_supplied_indices.push_back(user_supplied_epi_index);
        user_supplied_indices.push_back(user_supplied_mid_index);
        user_supplied_indices.push_back(user_supplied_endo_index);
        
        //figure out who goes first
    
        //loop three times
        for (unsigned layer_index=0; layer_index<3; layer_index++)
        {
            unsigned counter = 0;
            //find the corresponding index 
            for (unsigned supplied_index = 0; supplied_index<user_supplied_indices.size(); supplied_index++)
            {
                if (user_supplied_indices[supplied_index] == layer_index)
                {
                    break;
                }
                counter++;
            }

            //create the node lists based on the calculations above
            if (counter==0)
            {
                mCellHeterogeneityAreas.push_back(new ChasteNodesList<3u>(epi_nodes));
            }
            if (counter==1)
            {
                mCellHeterogeneityAreas.push_back(new ChasteNodesList<3u>(mid_nodes));
            }
            if (counter==2)
            {
                mCellHeterogeneityAreas.push_back(new ChasteNodesList<3u>(endo_nodes));
            }
            assert(counter<3);
        }
        assert(mCellHeterogeneityAreas.size()==3);
}

// Explicit instantiation
template class HeartConfigRelatedCellFactory<1u>;
template class HeartConfigRelatedCellFactory<2u>;
template class HeartConfigRelatedCellFactory<3u>;
