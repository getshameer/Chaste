/*

Copyright (c) 2005-2012, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "SemMeshWriter.hpp"
#include "Version.hpp"

/**
 * Convenience collection of iterators, primarily to get compilation to happen.
 */
template<unsigned SPACE_DIM>
struct MeshSemWriterIterators
{
    /** Iterator over nodes */
    typename AbstractMesh<SPACE_DIM,SPACE_DIM>::NodeIterator* pNodeIter;
    /** Iterator over potts elements */
    typename SemMesh<SPACE_DIM>::SemElementIterator* pElemIter;
};

///////////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////////

template<unsigned SPACE_DIM>
SemMeshWriter<SPACE_DIM>::SemMeshWriter(const std::string& rDirectory,
                                                           const std::string& rBaseName,
                                                           const bool clearOutputDir)
    : AbstractMeshWriter<SPACE_DIM, SPACE_DIM>(rDirectory, rBaseName, clearOutputDir),
      mpMesh(NULL),
      mpIters(new MeshSemWriterIterators<SPACE_DIM>),
      mpNodeMap(NULL),
      mNodeMapCurrentIndex(0)
{
    mpIters->pNodeIter = NULL;
    mpIters->pElemIter = NULL;
}

template<unsigned SPACE_DIM>
SemMeshWriter<SPACE_DIM>::~SemMeshWriter()
{
    if (mpIters->pNodeIter)
    {
        delete mpIters->pNodeIter;
        delete mpIters->pElemIter;
    }

    delete mpIters;

    if (mpNodeMap)
    {
        delete mpNodeMap;
    }
}

template<unsigned SPACE_DIM>
std::vector<double> SemMeshWriter<SPACE_DIM>::GetNextNode()
{
    if (mpMesh)
    {
        // Sanity check
        assert(this->mNumNodes == mpMesh->GetNumNodes());

        std::vector<double> coordinates(SPACE_DIM+1);

        // Get the node coordinates using the node iterator (thus skipping deleted nodes)
        for (unsigned j=0; j<SPACE_DIM; j++)
        {
            coordinates[j] = (*(mpIters->pNodeIter))->GetPoint()[j];
        }
        coordinates[SPACE_DIM] = (*(mpIters->pNodeIter))->IsBoundaryNode();

        ++(*(mpIters->pNodeIter));

        return coordinates;
    }
    else
    {
        return AbstractMeshWriter<SPACE_DIM,SPACE_DIM>::GetNextNode();
    }
}

template<unsigned SPACE_DIM>
ElementData SemMeshWriter<SPACE_DIM>::GetNextElement()
{
    if (mpMesh)
    {
        assert(this->mNumElements == mpMesh->GetNumElements());

        ElementData elem_data;
        elem_data.NodeIndices.resize((*(mpIters->pElemIter))->GetNumNodes());
        for (unsigned j=0; j<elem_data.NodeIndices.size(); j++)
        {
            unsigned old_index = (*(mpIters->pElemIter))->GetNodeGlobalIndex(j);
            elem_data.NodeIndices[j] = mpMesh->IsMeshChanging() ? mpNodeMap->GetNewIndex(old_index) : old_index;
        }

        // Set attribute
        elem_data.AttributeValue = (*(mpIters->pElemIter))->GetAttribute();
        ++(*(mpIters->pElemIter));

        return elem_data;
    }
    else
    {
        return AbstractMeshWriter<SPACE_DIM, SPACE_DIM>::GetNextElement();
    }
}

///\todo Mesh should be const (#1663)
template<unsigned SPACE_DIM>
void SemMeshWriter<SPACE_DIM>::WriteFilesUsingMesh(SemMesh<SPACE_DIM>& rMesh)
{
    this->mpMeshReader = NULL;
    mpMesh = &rMesh;

    this->mNumNodes = mpMesh->GetNumNodes();
    this->mNumElements = mpMesh->GetNumElements();

    typedef typename AbstractMesh<SPACE_DIM,SPACE_DIM>::NodeIterator NodeIterType;
    mpIters->pNodeIter = new NodeIterType(mpMesh->GetNodeIteratorBegin());

    typedef typename SemMesh<SPACE_DIM>::SemElementIterator ElemIterType;
    mpIters->pElemIter = new ElemIterType(mpMesh->GetElementIteratorBegin());

    // Set up node map if we might have deleted nodes
    mNodeMapCurrentIndex = 0;
    if (mpMesh->IsMeshChanging())
    {
        mpNodeMap = new NodeMap(mpMesh->GetNumAllNodes());
        for (NodeIterType it = mpMesh->GetNodeIteratorBegin(); it != mpMesh->GetNodeIteratorEnd(); ++it)
        {
            mpNodeMap->SetNewIndex(it->GetIndex(), mNodeMapCurrentIndex++);
        }
    }
    WriteFiles();
}

template<unsigned SPACE_DIM>
void SemMeshWriter<SPACE_DIM>::WriteFiles()
{
    std::string comment = "# " + ChasteBuildInfo::GetProvenanceString();

    // Write node file
    std::string node_file_name = this->mBaseName + ".node";
    out_stream p_node_file = this->mpOutputFileHandler->OpenOutputFile(node_file_name);

    // Write the node header
    unsigned num_attr = 0;
    unsigned max_bdy_marker = 1; // as we include boundary node information in the node file
    unsigned num_nodes = this->GetNumNodes();

    *p_node_file << num_nodes << "\t";
    *p_node_file << SPACE_DIM << "\t";
    *p_node_file << num_attr << "\t";
    *p_node_file << max_bdy_marker << "\n";
    *p_node_file << std::setprecision(6);

    // Write each node's data
    for (unsigned item_num=0; item_num<num_nodes; item_num++)
    {
        std::vector<double> current_item = this->GetNextNode();
        *p_node_file << item_num;
        for (unsigned i=0; i<SPACE_DIM+1; i++)
        {
            *p_node_file << "\t" << current_item[i];
        }
        *p_node_file << "\n";
    }
    *p_node_file << comment << "\n";
    p_node_file->close();

    // Write element file
    std::string element_file_name = this->mBaseName + ".cell";
    out_stream p_element_file = this->mpOutputFileHandler->OpenOutputFile(element_file_name);

    // Write the element header
    unsigned num_elements = this->GetNumElements();

    /// \todo #2146 - this looks like a bug in the making - what if the attribute IS zero!?!?
    double first_elem_attribute_value = (*(mpIters->pElemIter))->GetAttribute();
    if (first_elem_attribute_value != 0)
    {
        num_attr = 1;
    }

    *p_element_file << num_elements << "\t";
    *p_element_file << num_attr << "\n";

    // Write each element's data
    for (unsigned item_num=0; item_num<num_elements; item_num++)
    {
        // Get data for this element
        ElementData elem_data = this->GetNextElement();

        // Get the node indices owned by this element
        std::vector<unsigned> node_indices = elem_data.NodeIndices;

        // Write this element's index and the number of nodes owned by it to file
        *p_element_file << item_num <<  "\t" << node_indices.size();

        // Write the node indices owned by this element to file
        for (unsigned i=0; i<node_indices.size(); i++)
        {
            *p_element_file << "\t" << node_indices[i];
        }

        // Write the element attribute if necessary
        if (elem_data.AttributeValue != 0)
        {
            *p_element_file << "\t" << elem_data.AttributeValue;
        }

        // New line
        *p_element_file << "\n";
    }

    *p_element_file << comment << "\n";
    p_element_file->close();
}

/////////////////////////////////////////////////////////////////////////////
// Explicit instantiation
/////////////////////////////////////////////////////////////////////////////

template class SemMeshWriter<1>;
template class SemMeshWriter<2>;
template class SemMeshWriter<3>;