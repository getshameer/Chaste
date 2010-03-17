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

#include "VertexMesh3d.hpp"
#include "RandomNumberGenerator.hpp"
#include "UblasCustomFunctions.hpp"


VertexMesh3d::VertexMesh3d(std::vector<Node<3>*> nodes,
                           std::vector<VertexElement<2,3>*> faces,
                           std::vector<VertexElement3d*> vertexElements)
{
    // Reset member variables and clear mNodes and mElements
    Clear();

    // Populate mNodes mFaces and mElements
    for (unsigned node_index=0; node_index<nodes.size(); node_index++)
    {
        Node<3>* p_temp_node = nodes[node_index];
        this->mNodes.push_back(p_temp_node);
    }
    for (unsigned face_index=0; face_index<faces.size(); face_index++)
    {
    	VertexElement<2,3>* p_temp_face = faces[face_index];
        mFaces.push_back(p_temp_face);
    }

    for (unsigned elem_index=0; elem_index<vertexElements.size(); elem_index++)
    {
		VertexElement3d* p_temp_vertex_element = vertexElements[elem_index];
		mElements.push_back(p_temp_vertex_element);
	}

    // Register elements with nodes
    for (unsigned index=0; index<mElements.size(); index++)
    {
    	VertexElement3d* p_temp_vertex_element = mElements[index];
        for (unsigned node_index=0; node_index<p_temp_vertex_element->GetNumNodes(); node_index++)
        {
            Node<3>* p_temp_node = p_temp_vertex_element->GetNode(node_index);
            p_temp_node->AddElement(p_temp_vertex_element->GetIndex());
        }
    }

    this->mMeshChangesDuringSimulation = false;
}



VertexMesh3d::VertexMesh3d()
{
    this->mMeshChangesDuringSimulation = false;
    Clear();
}



VertexMesh3d::~VertexMesh3d()
{
    Clear();
}


unsigned VertexMesh3d::SolveNodeMapping(unsigned index) const
{
    assert(index < this->mNodes.size());
    return index;
}


unsigned VertexMesh3d::SolveElementMapping(unsigned index) const
{
    assert(index < this->mElements.size());
    return index;
}


unsigned VertexMesh3d::SolveBoundaryElementMapping(unsigned index) const
{
    /// \todo sort out boundary elements in a vertex mesh
//    assert(index < this->mBoundaryElements.size() );
    return index;
}


void VertexMesh3d::Clear()
{


	for (unsigned i=0; i<mElements.size(); i++)
    {
        delete mElements[i];
    }
	for (unsigned i=0; i<mFaces.size(); i++)
	{
		delete mFaces[i];
	}

	for (unsigned i=0; i<this->mNodes.size(); i++)
    {
        delete this->mNodes[i];
    }

    this->mNodes.clear();
    mFaces.clear();
    mElements.clear();
}


unsigned VertexMesh3d::GetNumNodes() const
{
    return this->mNodes.size();
}


unsigned VertexMesh3d::GetNumFaces() const
{
    return mFaces.size();
}

unsigned VertexMesh3d::GetNumElements() const
{
    return mElements.size();
}

unsigned VertexMesh3d::GetNumAllElements() const
{
    return mElements.size();
}

VertexElement3d* VertexMesh3d::GetElement(unsigned index) const
{
    assert(index < mElements.size());
    return mElements[index];
}


double VertexMesh3d::GetAreaOfElement(unsigned index)
{
//    #define COVERAGE_IGNORE
//    assert(SPACE_DIM == 2);
//    #undef COVERAGE_IGNORE
//
//    VertexElement<ELEMENT_DIM, SPACE_DIM>* p_element = GetElement(index);
//
//    c_vector<double, SPACE_DIM> first_node;
//    c_vector<double, SPACE_DIM> current_node;
//    c_vector<double, SPACE_DIM> anticlockwise_node;
//
//    unsigned num_nodes_in_element = p_element->GetNumNodes();
//
    double element_area = 0;
//
//    for (unsigned local_index=0; local_index<num_nodes_in_element; local_index++)
//    {
//        // Find locations of current node and anticlockwise node
//        current_node = p_element->GetNodeLocation(local_index);
//        anticlockwise_node = p_element->GetNodeLocation((local_index+1)%num_nodes_in_element);
//
//        element_area += 0.5*(current_node[0]*anticlockwise_node[1] - anticlockwise_node[0]*current_node[1]);
//    }
//
    return element_area;
}


double VertexMesh3d::GetPerimeterOfElement(unsigned index)
{
//    #define COVERAGE_IGNORE
//    assert(SPACE_DIM == 2);
//    #undef COVERAGE_IGNORE
//
//    VertexElement<ELEMENT_DIM, SPACE_DIM>* p_element = GetElement(index);
//
//    unsigned current_node_index;
//    unsigned anticlockwise_node_index;
//    unsigned num_nodes_in_element = p_element->GetNumNodes();
//
    double element_perimeter = 0;
//
//    for (unsigned local_index=0; local_index<num_nodes_in_element; local_index++)
//    {
//        // Find locations of current node and anticlockwise node
//        current_node_index = p_element->GetNodeGlobalIndex(local_index);
//        anticlockwise_node_index = p_element->GetNodeGlobalIndex((local_index+1)%num_nodes_in_element);
//
//        element_perimeter += this->GetDistanceBetweenNodes(current_node_index, anticlockwise_node_index);
//    }
//
    return element_perimeter;
}


c_vector<double, 3> VertexMesh3d::GetCentroidOfElement(unsigned index)
{
//    #define COVERAGE_IGNORE
//    assert(SPACE_DIM == 2);
//    #undef COVERAGE_IGNORE
//
//    VertexElement<ELEMENT_DIM, SPACE_DIM>* p_element = GetElement(index);
//
    c_vector<double, 3> centroid = zero_vector<double>(3);
//    c_vector<double, SPACE_DIM> current_node;
//    c_vector<double, SPACE_DIM> anticlockwise_node;
//
//    double temp_centroid_x = 0;
//    double temp_centroid_y = 0;
//
//    unsigned num_nodes_in_element = p_element->GetNumNodes();
//
//    for (unsigned local_index=0; local_index<num_nodes_in_element; local_index++)
//    {
//        // Find locations of current node and anticlockwise node
//        current_node = p_element->GetNodeLocation(local_index);
//        anticlockwise_node = p_element->GetNodeLocation((local_index+1)%num_nodes_in_element);
//
//        temp_centroid_x += (current_node[0]+anticlockwise_node[0])*(current_node[0]*anticlockwise_node[1]-current_node[1]*anticlockwise_node[0]);
//        temp_centroid_y += (current_node[1]+anticlockwise_node[1])*(current_node[0]*anticlockwise_node[1]-current_node[1]*anticlockwise_node[0]);
//    }
//
//    double vertex_area = GetAreaOfElement(index);
//    double centroid_coefficient = 1.0/(6.0*vertex_area);
//
//    centroid(0) = centroid_coefficient*temp_centroid_x;
//    centroid(1) = centroid_coefficient*temp_centroid_y;
//
    return centroid;
}

void VertexMesh3d::ConstructFromMeshReader(AbstractMeshReader<3,3>& rMeshReader)
{
//    // Store numbers of nodes and elements
//    unsigned num_nodes = rMeshReader.GetNumNodes();
//    unsigned num_elements = rMeshReader.GetNumElements();
//
//    // Reserve memory for nodes
//    this->mNodes.reserve(num_nodes);
//
//    rMeshReader.Reset();
//
//    // Add nodes
//    std::vector<double> node_data;
//    for (unsigned i=0; i<num_nodes; i++)
//    {
//        node_data = rMeshReader.GetNextNode();
//        unsigned is_boundary_node = (unsigned) node_data[2];
//        node_data.pop_back();
//        this->mNodes.push_back(new Node<SPACE_DIM>(i, node_data, is_boundary_node));
//    }
//
//    rMeshReader.Reset();
//
//    // Reserve memory for nodes
//    mElements.reserve(rMeshReader.GetNumElements());
//
//    // Add elements
//    for (unsigned elem_index=0; elem_index<num_elements; elem_index++)
//    {
//        ElementData element_data = rMeshReader.GetNextElementData();
//        std::vector<Node<SPACE_DIM>*> nodes;
//
//        unsigned num_nodes_in_element = element_data.NodeIndices.size();
//        for (unsigned j=0; j<num_nodes_in_element; j++)
//        {
//            assert(element_data.NodeIndices[j] < this->mNodes.size());
//            nodes.push_back(this->mNodes[element_data.NodeIndices[j]]);
//        }
//
//        VertexElement<ELEMENT_DIM,SPACE_DIM>* p_element = new VertexElement<ELEMENT_DIM,SPACE_DIM>(elem_index, nodes);
//        mElements.push_back(p_element);
//
//        if (rMeshReader.GetNumElementAttributes() > 0)
//        {
//            assert(rMeshReader.GetNumElementAttributes() == 1);
//            unsigned attribute_value = element_data.AttributeValue;
//            p_element->SetRegion(attribute_value);
//        }
//    }
}

// Serialization for Boost >= 1.36
#include "SerializationExportWrapperForCpp.hpp"
