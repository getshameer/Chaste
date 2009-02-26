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

#include "VertexMesh.hpp"
#include "RandomNumberGenerator.hpp"


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
VertexMesh<ELEMENT_DIM, SPACE_DIM>::VertexMesh(std::vector<Node<SPACE_DIM>*> nodes, 
                                               std::vector<VertexElement<ELEMENT_DIM,SPACE_DIM>*> vertexElements,
                                               double cellRearrangementThreshold,
                                               double edgeDivisionThreshold)
    : mCellRearrangementThreshold(cellRearrangementThreshold),
      mEdgeDivisionThreshold(edgeDivisionThreshold),
      mAddedNodes(true)
{
    assert(cellRearrangementThreshold > 0.0);
    assert(edgeDivisionThreshold > 0.0);

    Clear();
    for (unsigned node_index=0; node_index<nodes.size(); node_index++)
    {
        Node<SPACE_DIM>* temp_node = nodes[node_index];
        mNodes.push_back(temp_node);
    }
    
    for (unsigned elem_index=0; elem_index<vertexElements.size(); elem_index++)
    {
        VertexElement<ELEMENT_DIM,SPACE_DIM>* temp_vertex_element = vertexElements[elem_index];
        mElements.push_back(temp_vertex_element);
    }
    
    SetupVertexElementsOwnedByNodes();
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
VertexMesh<ELEMENT_DIM, SPACE_DIM>::VertexMesh(double cellRearrangementThreshold, double edgeDivisionThreshold)
    : mCellRearrangementThreshold(cellRearrangementThreshold),
      mEdgeDivisionThreshold(edgeDivisionThreshold),
      mAddedNodes(false)
{
    assert(cellRearrangementThreshold > 0.0);
    assert(edgeDivisionThreshold > 0.0);
    Clear();
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
VertexMesh<ELEMENT_DIM, SPACE_DIM>::VertexMesh(unsigned numAcross,
                                               unsigned numUp,
                                               double cellRearrangementThreshold,
                                               double edgeDivisionThreshold)
    : mCellRearrangementThreshold(cellRearrangementThreshold),
      mEdgeDivisionThreshold(edgeDivisionThreshold),
      mAddedNodes(true)
{
    assert(cellRearrangementThreshold > 0.0);
    assert(edgeDivisionThreshold > 0.0);

    if (SPACE_DIM==2)
    {    
        assert(numAcross > 1);
        unsigned node_index = 0;
        
        // Create the nodes
        for (unsigned j=0; j<=2*numUp+1; j++)
        {
            if (j%2 == 0)
            {
                for (unsigned i=1; i<=3*numAcross+1; i+=2)
                {
                    if (j!=0 || i!= 3*numAcross+1)
                    {
                        if (i%3 != 2)
                        {
                            Node<SPACE_DIM>* p_node = new Node<SPACE_DIM>(node_index, false, i/(2.0*sqrt(3)), j/2.0);
                            
                            if (j==0 || j==2*numUp || i==1 || i==3*numAcross || i==3*numAcross+1)
                            {
                                p_node->SetAsBoundaryNode(true);
                            }
                            mNodes.push_back(p_node);
                            node_index++;
                        }
                    }
                }
            }
            else 
            {
                for (unsigned i=0; i<=3*numAcross+1; i+=2)
                {
                    if ((j!=2*numUp+1 || i != 0) && (j!=2*numUp+1 || i!= 3*numAcross+1))
                    {
                        if (i%3 != 2)
                        {
                            Node<SPACE_DIM>* p_node = new Node<SPACE_DIM>(node_index, false, i/(2.0*sqrt(3)), j/2.0);
                            if (j==1 || j==2*numUp+1 || i==0 || i==3*numAcross || i==3*numAcross+1)
                            {
                                p_node->SetAsBoundaryNode(true);
                            }
                            mNodes.push_back(p_node);
                            node_index++;
                        }
                    }
                }
            }
        }  
        
        // Create the elements. The array node_indices contains the 
        // global node indices from bottom left, going anticlockwise.
        
        unsigned node_indices[6];
        unsigned element_index;
        
        for (unsigned j=0; j<numUp; j++)
        {
            for (unsigned i=0; i<numAcross; i++)
            {
                element_index = j*numAcross + i;
                
                if (numAcross%2==0) // numAcross is even
                {
                    if (j == 0)     // bottom row
                    {
                        if (i%2 == 0) // even
                        {
                            node_indices[0] = i;
                        }
                        else // odd
                        {
                            node_indices[0] = numAcross+i;
                        }                                           
                    }                       
                    else    // not on the bottom row 
                    {
                         if (i%2 == 0) // even
                        {
                            node_indices[0] = (2*numAcross+1)+2*(j-1)*(numAcross+1)+i;
                        }
                        else // odd
                        {
                            node_indices[0] = (2*numAcross+1)+(2*j-1)*(numAcross+1)+i;
                        }                        
                    }
                        
                }
                else // numAcross is odd
                {
                    if (i%2 == 0) // even
                    {
                        node_indices[0] = 2*j*(numAcross+1)+i;
                    }
                    else // odd
                    {
                        node_indices[0] = (2*j+1)*(numAcross+1)+i;
                    }
                }
                node_indices[1] = node_indices[0] + 1;
                node_indices[2] = node_indices[0] + numAcross + 2;
                node_indices[3] = node_indices[0] + 2*numAcross + 3;
                node_indices[4] = node_indices[0] + 2*numAcross + 2;
                node_indices[5] = node_indices[0] + numAcross + 1;
                 
                if ((j==numUp-1)&&(i%2 == 1))
                {
                    // On top row and its an odd column nodes 
                    node_indices[3] -= 1;
                    node_indices[4] -= 1;
                }
                  
                if ((j==0)&&(i%2 == 0)&&(numAcross%2==0))
                {
                    // On bottom row and its an even column and there is
                    // an even number of columns in total, (i.e. the very bottom) 
                    node_indices[2] -= 1;
                    node_indices[3] -= 1;
                    node_indices[4] -= 1;
                    node_indices[5] -= 1;
                }

                std::vector<Node<SPACE_DIM>*> element_nodes;
                
                for (int i=0; i<6; i++)
                {
                   element_nodes.push_back(mNodes[node_indices[i]]);
                }
                VertexElement<ELEMENT_DIM,SPACE_DIM>* p_element = new VertexElement<ELEMENT_DIM,SPACE_DIM>(element_index, element_nodes);
                mElements.push_back(p_element);
            }
        }  
    }
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
VertexMesh<ELEMENT_DIM, SPACE_DIM>::~VertexMesh()
{
    Clear();
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
double VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetCellRearrangementThreshold() const
{
    return mCellRearrangementThreshold;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
double VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetEdgeDivisionThreshold() const
{
    return mEdgeDivisionThreshold;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::SetCellRearrangementThreshold(double cellRearrangementThreshold)
{
    mCellRearrangementThreshold = cellRearrangementThreshold;
}



template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::SetEdgeDivisionThreshold(double edgeDivisionThreshold)
{
    mEdgeDivisionThreshold = edgeDivisionThreshold;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::SetupVertexElementsOwnedByNodes()
{
    for (unsigned index=0; index<mElements.size(); index++)
    {
        VertexElement<ELEMENT_DIM,SPACE_DIM>* p_temp_vertex_element = mElements[index];
        for (unsigned node_index=0; node_index<p_temp_vertex_element->GetNumNodes(); node_index++)
        {
            Node<SPACE_DIM>* p_temp_node = p_temp_vertex_element->GetNode(node_index);
            p_temp_node->AddElement(p_temp_vertex_element->GetIndex());
        }
    }
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::Clear()
{
    mDeletedNodeIndices.clear();
    mDeletedElementIndices.clear();
    mAddedNodes = false;
    
    for (unsigned i=0; i<mElements.size(); i++)
    {
        delete mElements[i];
    }
    for (unsigned i=0; i<mNodes.size(); i++)
    {
        delete mNodes[i];
    }

    mNodes.clear();
    mElements.clear();
}


template <unsigned ELEMENT_DIM, unsigned SPACE_DIM>
double VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetWidth(const unsigned& rDimension)
{
    assert(rDimension < SPACE_DIM);
    c_vector<double,2> extremes = GetWidthExtremes(rDimension);
    return extremes[1] - extremes[0];
}


template <unsigned ELEMENT_DIM, unsigned SPACE_DIM>
c_vector<double, 2> VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetWidthExtremes(const unsigned& rDimension)
{
    assert(rDimension < SPACE_DIM);
    assert(GetNumNodes() > 0);

    double max = -1e200;
    double min = 1e200;
    
    for (unsigned i=0; i<GetNumNodes(); i++)
    {
        if (!mNodes[i]->IsDeleted())
        {
            double this_node_value = mNodes[i]->rGetLocation()[rDimension];
            if (this_node_value>max)
            {
                max = this_node_value;
            }
            if (this_node_value < min)
            {
                min = this_node_value;
            }
        }
    }

    c_vector<double,2> extremes;
    extremes[0] = min;
    extremes[1] = max;

    return extremes;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
unsigned VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetNumNodes() const
{
    return mNodes.size() - mDeletedNodeIndices.size();
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
unsigned VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetNumElements() const
{
    return mElements.size() - mDeletedElementIndices.size();
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
unsigned VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetNumAllElements()
{
    return mElements.size();
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
Node<SPACE_DIM>* VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetNode(unsigned index) const
{
    assert(index < mNodes.size());
    return mNodes[index];
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
VertexElement<ELEMENT_DIM,SPACE_DIM>* VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetElement(unsigned index) const
{
    assert(index < mElements.size());
    return mElements[index];
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>    
double VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetAreaOfElement(unsigned index)
{
    #define COVERAGE_IGNORE
    assert(SPACE_DIM == 2);
    #undef COVERAGE_IGNORE

    VertexElement<ELEMENT_DIM, SPACE_DIM>* p_element = GetElement(index);

    c_vector<double, SPACE_DIM> first_node;
        
    
    c_vector<double, SPACE_DIM> current_node;
    c_vector<double, SPACE_DIM> anticlockwise_node;
  
    unsigned num_nodes_in_element = p_element->GetNumNodes();

    double element_area = 0;

    for (unsigned local_index=0; local_index<num_nodes_in_element; local_index++)
    {
        // Find locations of current node and anticlockwise node
        current_node = p_element->GetNodeLocation(local_index);
        anticlockwise_node = p_element->GetNodeLocation((local_index+1)%num_nodes_in_element);
  
        element_area += 0.5*(current_node[0]*anticlockwise_node[1] 
                           - anticlockwise_node[0]*current_node[1]);
    }

    return element_area;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>    
double VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetPerimeterOfElement(unsigned index)
{
    #define COVERAGE_IGNORE
    assert(SPACE_DIM == 2);
    #undef COVERAGE_IGNORE

    VertexElement<ELEMENT_DIM, SPACE_DIM>* p_element = GetElement(index);

    unsigned current_node_index;
    unsigned anticlockwise_node_index;
    unsigned num_nodes_in_element = p_element->GetNumNodes();

    double element_perimeter = 0;

    for (unsigned local_index=0; local_index<num_nodes_in_element; local_index++)
    {
        // Find locations of current node and anticlockwise node
        current_node_index = p_element->GetNodeGlobalIndex(local_index);
        anticlockwise_node_index = p_element->GetNodeGlobalIndex((local_index+1)%num_nodes_in_element);

        element_perimeter += GetDistanceBetweenNodes(current_node_index, anticlockwise_node_index);
    }

    return element_perimeter;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
c_vector<double, SPACE_DIM> VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetCentroidOfElement(unsigned index)
{
    #define COVERAGE_IGNORE
    assert(SPACE_DIM == 2);
    #undef COVERAGE_IGNORE

    VertexElement<ELEMENT_DIM, SPACE_DIM>* p_element = GetElement(index);

    c_vector<double, SPACE_DIM> centroid = zero_vector<double>(SPACE_DIM);
    c_vector<double, SPACE_DIM> current_node;
    c_vector<double, SPACE_DIM> anticlockwise_node; 

    double temp_centroid_x = 0;
    double temp_centroid_y = 0;

    unsigned num_nodes_in_element = p_element->GetNumNodes();

    for (unsigned local_index=0; local_index<num_nodes_in_element; local_index++)
    {
        // Find locations of current node and anticlockwise node
        current_node = p_element->GetNodeLocation(local_index);
        anticlockwise_node = p_element->GetNodeLocation((local_index+1)%num_nodes_in_element);

        temp_centroid_x += (current_node[0]+anticlockwise_node[0])*(current_node[0]*anticlockwise_node[1]-current_node[1]*anticlockwise_node[0]);
        temp_centroid_y += (current_node[1]+anticlockwise_node[1])*(current_node[0]*anticlockwise_node[1]-current_node[1]*anticlockwise_node[0]);               
    }

    double vertex_area = GetAreaOfElement(index);
    double centroid_coefficient = 1.0/(6.0*vertex_area);

    centroid(0) = centroid_coefficient*temp_centroid_x;
    centroid(1) = centroid_coefficient*temp_centroid_y;

    return centroid;        
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
c_vector<double, SPACE_DIM> VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetAreaGradientOfElementAtNode(VertexElement<ELEMENT_DIM,SPACE_DIM>* pElement, unsigned localIndex)
{
    #define COVERAGE_IGNORE
    assert(SPACE_DIM==2);
    #undef COVERAGE_IGNORE

    unsigned num_nodes_in_element = pElement->GetNumNodes();
    unsigned next_local_index = (localIndex+1)%num_nodes_in_element;

    // We add an extra localIndex-1 in the line below as otherwise this term can be negative, which breaks the % operator    
    unsigned previous_local_index = (num_nodes_in_element+localIndex-1)%num_nodes_in_element;       
    
    c_vector<double, SPACE_DIM> previous_node_location = pElement->GetNodeLocation(previous_local_index);
    c_vector<double, SPACE_DIM> next_node_location = pElement->GetNodeLocation(next_local_index);
    c_vector<double, SPACE_DIM> difference_vector = GetVectorFromAtoB(previous_node_location, next_node_location);

    c_vector<double, SPACE_DIM> area_gradient;

    area_gradient[0] = 0.5*difference_vector[1];
    area_gradient[1] = -0.5*difference_vector[0];

    return area_gradient;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
c_vector<double, SPACE_DIM> VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetPreviousEdgeGradientOfElementAtNode(VertexElement<ELEMENT_DIM,SPACE_DIM>* pElement, unsigned localIndex)
{
    #define COVERAGE_IGNORE
    assert(SPACE_DIM==2);
    #undef COVERAGE_IGNORE

    unsigned num_nodes_in_element = pElement->GetNumNodes();

    // We add an extra localIndex-1 in the line below as otherwise this term can be negative, which breaks the % operator
    unsigned previous_local_index = (num_nodes_in_element+localIndex-1)%num_nodes_in_element;

    unsigned current_global_index = pElement->GetNodeGlobalIndex(localIndex);
    unsigned previous_global_index = pElement->GetNodeGlobalIndex(previous_local_index);

    double previous_edge_length = GetDistanceBetweenNodes(current_global_index, previous_global_index);
    assert(previous_edge_length > DBL_EPSILON);

    c_vector<double, SPACE_DIM> previous_edge_gradient = GetVectorFromAtoB(pElement->GetNodeLocation(previous_local_index), pElement->GetNodeLocation(localIndex))/previous_edge_length;

    return previous_edge_gradient;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
c_vector<double, SPACE_DIM> VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetNextEdgeGradientOfElementAtNode(VertexElement<ELEMENT_DIM,SPACE_DIM>* pElement, unsigned localIndex)
{
    #define COVERAGE_IGNORE
    assert(SPACE_DIM==2);
    #undef COVERAGE_IGNORE

    unsigned num_nodes_in_element = pElement->GetNumNodes();

    unsigned next_local_index = (localIndex+1)%num_nodes_in_element;

    unsigned current_global_index = pElement->GetNodeGlobalIndex(localIndex);
    unsigned next_global_index = pElement->GetNodeGlobalIndex(next_local_index);

    double next_edge_length = GetDistanceBetweenNodes(current_global_index, next_global_index);
    assert(next_edge_length > DBL_EPSILON);

    c_vector<double, SPACE_DIM> next_edge_gradient = GetVectorFromAtoB(pElement->GetNodeLocation(next_local_index), pElement->GetNodeLocation(localIndex))/next_edge_length;

    return next_edge_gradient;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
c_vector<double, SPACE_DIM> VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetPerimeterGradientOfElementAtNode(VertexElement<ELEMENT_DIM,SPACE_DIM>* pElement, unsigned localIndex)
{
    #define COVERAGE_IGNORE
    assert(SPACE_DIM==2);
    #undef COVERAGE_IGNORE

    c_vector<double, SPACE_DIM> previous_edge_gradient = GetPreviousEdgeGradientOfElementAtNode(pElement, localIndex);
    c_vector<double, SPACE_DIM> next_edge_gradient = GetNextEdgeGradientOfElementAtNode(pElement, localIndex);

    return previous_edge_gradient + next_edge_gradient;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>  
c_vector<double, 3> VertexMesh<ELEMENT_DIM, SPACE_DIM>::CalculateMomentsOfElement(unsigned index)
{
    #define COVERAGE_IGNORE
    assert(SPACE_DIM == 2);
    #undef COVERAGE_IGNORE

    VertexElement<ELEMENT_DIM, SPACE_DIM>* p_element = GetElement(index);
    unsigned num_nodes_in_element = p_element->GetNumNodes();

    c_vector<double, 3> moments = zero_vector<double>(3);
    unsigned node_1;
    unsigned node_2;

    for (unsigned i=0; i<num_nodes_in_element; i++)
    {
        node_1 = i;
        node_2 = (i+1)%num_nodes_in_element;

        c_vector<double, 2> pos_1 = p_element->GetNodeLocation(node_1);
        c_vector<double, 2> pos_2 = p_element->GetNodeLocation(node_2);

        // Ixx
        moments(0) += (pos_2(0)-pos_1(0))*(  pos_1(1)*pos_1(1)*pos_1(1)
                                           + pos_1(1)*pos_1(1)*pos_2(1)
                                           + pos_1(1)*pos_2(1)*pos_2(1)
                                           + pos_2(1)*pos_2(1)*pos_2(1));

        // Iyy
        moments(1) += (pos_2(1)-pos_1(1))*(  pos_1(0)*pos_1(0)*pos_1(0)
                                           + pos_1(0)*pos_1(0)*pos_2(0)
                                           + pos_1(0)*pos_2(0)*pos_2(0)
                                           + pos_2(0)*pos_2(0)*pos_2(0));

        // Ixy
        moments(2) +=   pos_1(0)*pos_1(0)*pos_2(1)*(pos_1(1)*2 + pos_2(1))
                      - pos_2(0)*pos_2(0)*pos_1(1)*(pos_1(1) + pos_2(1)*2)
                      + 2*pos_1(0)*pos_2(0)*(pos_2(1)*pos_2(1) - pos_1(1)*pos_1(1));
    }

    moments(0) /= -12;
    moments(1) /= 12;
    moments(2) /= 24;

    return moments;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
c_vector<double, SPACE_DIM> VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetShortAxisOfElement(unsigned index)
{
    #define COVERAGE_IGNORE
    assert(SPACE_DIM == 2);
    #undef COVERAGE_IGNORE

    c_vector<double, SPACE_DIM> short_axis = zero_vector<double>(SPACE_DIM);
    c_vector<double, 3> moments = CalculateMomentsOfElement(index);

    double largest_eigenvalue, discriminant;            

    discriminant = sqrt((moments(0) - moments(1))*(moments(0) - moments(1)) + 4.0*moments(2)*moments(2));

    // This is always the largest eigenvalue as both eigenvalues are real as it is a 
    // symmetric matrix
    largest_eigenvalue = ((moments(0) + moments(1)) + discriminant)*0.5;       

    if (fabs(discriminant) < 1e-10)
    {
        // Return a random unit vector
        short_axis(0) = RandomNumberGenerator::Instance()->ranf();
        short_axis(1) = sqrt(1.0 - short_axis(0)*short_axis(0));
    }
    else
    {                      
        if (moments(2) == 0.0)
        {
             short_axis(0) = 0.0;
             short_axis(1) = 1.0;                       
        }
        else
        {
            short_axis(0) = 1.0;
            short_axis(1) = (moments(0) - largest_eigenvalue)/moments(2);

            double length_short_axis = norm_2(short_axis);

            short_axis /= length_short_axis;
        }     
    }
    return short_axis;        
}


template <unsigned ELEMENT_DIM, unsigned SPACE_DIM>
double VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetDistanceBetweenNodes(unsigned indexA, unsigned indexB)
{
    c_vector<double, SPACE_DIM> vector = GetVectorFromAtoB(mNodes[indexA]->rGetLocation(), mNodes[indexB]->rGetLocation());
    return norm_2(vector);
}


template <unsigned ELEMENT_DIM, unsigned SPACE_DIM>
c_vector<double, SPACE_DIM> VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetVectorFromAtoB(const c_vector<double, SPACE_DIM>& rLocationA, const c_vector<double, SPACE_DIM>& rLocationB)
{
    c_vector<double, SPACE_DIM> vector = rLocationB - rLocationA;
    return vector;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
unsigned VertexMesh<ELEMENT_DIM, SPACE_DIM>::AddNode(Node<SPACE_DIM> *pNewNode)
{
    if (mDeletedNodeIndices.empty())
    {
        pNewNode->SetIndex(mNodes.size());
        mNodes.push_back(pNewNode);
    }
    else
    {
        unsigned index = mDeletedNodeIndices.back();
        pNewNode->SetIndex(index);
        mDeletedNodeIndices.pop_back();
        delete mNodes[index];
        mNodes[index] = pNewNode;
    }
    mAddedNodes = true;
    return pNewNode->GetIndex();
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
unsigned VertexMesh<ELEMENT_DIM, SPACE_DIM>::AddElement(VertexElement<ELEMENT_DIM,SPACE_DIM>* pNewElement)
{
    unsigned new_element_index = pNewElement->GetIndex();
    
    if (new_element_index == mElements.size())
    {
        mElements.push_back(pNewElement);
    }
    else
    {
        mElements[new_element_index] = pNewElement;
    }
    mAddedElements = true;
    pNewElement->RegisterWithNodes();
    
    return pNewElement->GetIndex();
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::SetNode(unsigned nodeIndex, ChastePoint<SPACE_DIM> point)
{
    mNodes[nodeIndex]->SetPoint(point);
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::DeleteElementPriorToReMesh(unsigned index)
{
    #define COVERAGE_IGNORE
    assert(SPACE_DIM == 2);
    #undef COVERAGE_IGNORE

    // Mark any nodes that are contained only in this element as deleted
    for (unsigned i=0; i<this->mElements[index]->GetNumNodes(); i++)
    {
        Node<SPACE_DIM>* p_node = this->mElements[index]->GetNode(i);
        
        if (p_node->rGetContainingElementIndices().size()==1)
        {
            p_node->MarkAsDeleted();
            mDeletedNodeIndices.push_back(p_node->GetIndex());
        }
    }

    // Mark this element as deleted
    this->mElements[index]->MarkAsDeleted();
    mDeletedElementIndices.push_back(index);
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::DivideEdge(Node<SPACE_DIM>* pNodeA, Node<SPACE_DIM>* pNodeB)
{
    // Find the indices of the elements owned by each node
    std::set<unsigned> elements_containing_nodeA = pNodeA->rGetContainingElementIndices();
    std::set<unsigned> elements_containing_nodeB = pNodeB->rGetContainingElementIndices();
    
    // Find common elements
    std::set<unsigned> shared_elements;
    std::set_intersection(elements_containing_nodeA.begin(),
                          elements_containing_nodeA.end(),
                          elements_containing_nodeB.begin(),
                          elements_containing_nodeB.end(),
                          std::inserter(shared_elements, shared_elements.begin()));
    
    // Check that the nodes have a common edge
    assert(shared_elements.size() > 0);

    // Create a new node (position is not important as it will be changed)
    Node<SPACE_DIM>* p_new_node = new Node<SPACE_DIM>(GetNumNodes(), false, 0.0, 0.0);

    // Update the node location
    c_vector<double, SPACE_DIM> new_node_position = pNodeA->rGetLocation() + 0.5*GetVectorFromAtoB(pNodeA->rGetLocation(), pNodeB->rGetLocation());
    ChastePoint<SPACE_DIM> point(new_node_position);    
    p_new_node->SetPoint(new_node_position);
    
    // Add node to mesh
    mNodes.push_back(p_new_node);

    // Iterate over common elements    
    for (std::set<unsigned>::iterator iter=shared_elements.begin();
         iter!=shared_elements.end();
         ++iter)
    {
        // Find which node has the lower local index in this element
        unsigned local_indexA = GetElement(*iter)->GetNodeLocalIndex(pNodeA->GetIndex());
        unsigned local_indexB = GetElement(*iter)->GetNodeLocalIndex(pNodeB->GetIndex());
        
        unsigned index = local_indexB;
        if ( (local_indexA == 0) || (local_indexB == 0) || (local_indexB > local_indexA) )
        {
            index = local_indexA;                 
        }
        if ( (local_indexA == 0) && (local_indexB == GetElement(*iter)->GetNumNodes()-1))
        {
            index = local_indexB;
        }

        // Add new node to this element
        GetElement(*iter)->AddNode(index, p_new_node);
    }
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::ReMesh(NodeMap& elementMap)
{
    // Make sure that we are in the correct dimension - this code will be eliminated at compile time
    #define COVERAGE_IGNORE
    assert(SPACE_DIM==2 || SPACE_DIM==3);
    assert(ELEMENT_DIM == SPACE_DIM);
    #undef COVERAGE_IGNORE

    // Make sure the map is big enough
    elementMap.Resize(GetNumAllElements());
    
    if (SPACE_DIM==2)
    {
        // Remove deleted elements
        std::vector<VertexElement<ELEMENT_DIM, SPACE_DIM>*> live_elements;
        for (unsigned i=0; i<mElements.size(); i++)
        {
            if (mElements[i]->IsDeleted())
            {
                delete this->mElements[i];
            }
            else
            {
                live_elements.push_back(mElements[i]);
                elementMap.SetNewIndex(i, (unsigned)(live_elements.size()-1));
            }
        }

        assert(mDeletedElementIndices.size() == mElements.size() - live_elements.size());
        mDeletedElementIndices.clear();
        mElements = live_elements;

        // Remove deleted nodes
        std::vector<Node<SPACE_DIM>*> live_nodes;
        for (unsigned i=0; i<this->mNodes.size(); i++)
        {
            if (mNodes[i]->IsDeleted())
            {
                delete this->mNodes[i];
            }
            else
            {
                live_nodes.push_back(this->mNodes[i]);
            }
        }

        assert(mDeletedNodeIndices.size() == mNodes.size() - live_nodes.size());
        mNodes = live_nodes;
        mDeletedNodeIndices.clear();

        for (unsigned i=0; i<mElements.size(); i++)
        {
            mElements[i]->ResetIndex(i);
        }

        for (unsigned i=0; i<mNodes.size();i++)
        {
            this->mNodes[i]->SetIndex(i);
        }

        /*
         * We do not need to call Clear() and remove all current data, since
         * cell birth, rearrangement and death result only in local remeshing
         * of a vertex-based mesh.
         * 
         * Instead, we should now remove any deleted nodes and elements.
         * 
         * We should then construct any new nodes, including boundary nodes; 
         * then new elements; then new edges.
         * 
         * Finally (or should this be at the start?), we should perform any 
         * cell rearrangements.
         */

        // Start of element rearrangement code...

        // Restart check after each T1Swap as it changes elements
        bool recheck_mesh = true;
        while (recheck_mesh == true)
        {
            recheck_mesh = false;

            // Loop over elements
            for (unsigned elem_index=0; elem_index<mElements.size(); elem_index++)
            {                
                if (!recheck_mesh)
                {
                    unsigned num_nodes = mElements[elem_index]->GetNumNodes();
                    assert(num_nodes > 0); // if not element should be deleted
                    
                    unsigned new_num_nodes = num_nodes;
                    
                    // Loop over element vertices
                    for (unsigned local_index=0; local_index<num_nodes; local_index++)
                    {
                        // Find locations of current node and anticlockwise node
                        Node<SPACE_DIM>* p_current_node = mElements[elem_index]->GetNode(local_index);
                        unsigned local_index_plus_one = (local_index+1)%new_num_nodes; /// \todo Should use iterators to tidy this up
                        Node<SPACE_DIM>* p_anticlockwise_node = mElements[elem_index]->GetNode(local_index_plus_one);
                        
                        // Find distance between nodes
                        double distance_between_nodes = GetDistanceBetweenNodes(p_current_node->GetIndex(), p_anticlockwise_node->GetIndex());

                        // If the nodes are too close together, perform a swap
                        if (distance_between_nodes < mCellRearrangementThreshold)
                        {
                            // Identify the type of node swap/merge needed then call method to perform swap/merge
                            IdentifySwapType(p_current_node, p_anticlockwise_node);
                            
                            recheck_mesh = true;
                            break;
                        }

                        if (distance_between_nodes > mEdgeDivisionThreshold)
                        {
                            // If the nodes are too far apart, divide the edge
                            DivideEdge(p_current_node, p_anticlockwise_node);
                            new_num_nodes++;
                        }
                    }
                }
                else
                {
                    break;
                }
            }
        }
               
        // ... end of element rearrangement code

        // areas and perimeters of elements are sorted in PerformT1Swap() method

        // Check that no nodes have overlapped elements...

        /// \todo Only need to check this next bit if the element/node is on the boundary (see #933 and #943)

        // Loop over elements
        for (unsigned elem_index=0; elem_index<mElements.size(); elem_index++)
        {
            unsigned num_nodes = mElements[elem_index]->GetNumNodes();

            // Loop over element vertices
            for (unsigned local_index=0; local_index<num_nodes; local_index++)
            {
                // Find locations of current node and anticlockwise node
                Node<SPACE_DIM>* p_current_node = mElements[elem_index]->GetNode(local_index);

                if (p_current_node->IsBoundaryNode())
                {
                    for (unsigned other_elem_index=0; other_elem_index<mElements.size(); other_elem_index++)
                    {
                        if (other_elem_index != elem_index)
                        {
                            if (ElementIncludesPoint(p_current_node->rGetLocation(), other_elem_index))
                            {
                                // \todo this doesnt account for cylindrical meshes
                                std::cout << "Node " << p_current_node->GetIndex() << " has overlapped element " << other_elem_index << "\n" << std::flush;
                                /// \todo Remesh appropriately if an overlap occurs (see #933)
                                // EXCEPTION("A node has overlapped an element");
                            }
                        }
                    }
                }
            }
        }

    }
    else // 3D
    {
        #define COVERAGE_IGNORE
        EXCEPTION("Remeshing has not been implemented in 3D (see #827 and #860)\n");
        #undef COVERAGE_IGNORE
        /// \todo put code for remeshing in 3D here - see #866 and the paper doi:10.1016/j.jtbi.2003.10.001
    }
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::ReMesh()
{
    NodeMap map(GetNumElements());
    ReMesh(map);
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
std::set<unsigned> VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetNeighbouringNodeIndices(unsigned nodeIndex)
{
    // Create a set of neighbouring node indices
    std::set<unsigned> neighbouring_node_indices; 
    
    // Find the indices of the elements owned by this node
    std::set<unsigned> containing_elem_indices = GetNode(nodeIndex)->rGetContainingElementIndices();

    // Iterate over these elements
    for (std::set<unsigned>::iterator elem_iter=containing_elem_indices.begin();
         elem_iter != containing_elem_indices.end();
         ++elem_iter)
    {
        // Find the local index of this node in this element
        unsigned local_index = GetElement(*elem_iter)->GetNodeLocalIndex(nodeIndex);
        
        // Find the global indices of the preceding and successive nodes in this element
        unsigned num_nodes = GetElement(*elem_iter)->GetNumNodes();
        unsigned previous_local_index = (local_index - 1)%num_nodes;
        unsigned next_local_index = (local_index + 1)%num_nodes;
        
        // Add the global indices of these two nodes to the set of neighbouring node indices
        neighbouring_node_indices.insert(GetElement(*elem_iter)->GetNodeGlobalIndex(previous_local_index));
        neighbouring_node_indices.insert(GetElement(*elem_iter)->GetNodeGlobalIndex(next_local_index));
    }
    
    return neighbouring_node_indices;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
std::set<unsigned> VertexMesh<ELEMENT_DIM, SPACE_DIM>::GetNeighbouringNodeNotAlsoInElement(unsigned nodeIndex, unsigned elemIndex)
{
    /// \todo We should probably assert here that the node is in fact contained in the element

    // Create a set of neighbouring node indices
    std::set<unsigned> neighbouring_node_indices_not_in_this_element;
    
    // Get the indices of this node's neighbours
    std::set<unsigned> node_neighbours = GetNeighbouringNodeIndices(nodeIndex);
    
    // Get the indices of the nodes contained in this element
    std::set<unsigned> node_indices_in_this_element;
    for (unsigned local_index=0; local_index<GetElement(elemIndex)->GetNumNodes(); local_index++)
    {
        unsigned global_index = GetElement(elemIndex)->GetNodeGlobalIndex(local_index);
        node_indices_in_this_element.insert(global_index);
    }

    // Check if each neighbour is also in this element; if not, add it to the set
    for (std::set<unsigned>::iterator iter=node_neighbours.begin();
         iter!=node_neighbours.end();
         ++iter)
    {
        if (node_indices_in_this_element.find(*iter) == node_indices_in_this_element.end())
        {
            neighbouring_node_indices_not_in_this_element.insert(*iter);
        }
    }
    
    return neighbouring_node_indices_not_in_this_element;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::IdentifySwapType(Node<SPACE_DIM>* pNodeA, Node<SPACE_DIM>* pNodeB)
{
    // Make sure that we are in the correct dimension - this code will be eliminated at compile time
    #define COVERAGE_IGNORE
    assert(SPACE_DIM == 2); // only works in 2D at present
    assert(ELEMENT_DIM == SPACE_DIM);
    #undef COVERAGE_IGNORE

    // Find the sets of elements containing nodes A and B
    std::set<unsigned> nodeA_elem_indices = pNodeA->rGetContainingElementIndices();
    std::set<unsigned> nodeB_elem_indices = pNodeB->rGetContainingElementIndices();    
    
    // Form the set union
    std::set<unsigned> all_indices, temp_set;
    std::set_union(nodeA_elem_indices.begin(), nodeA_elem_indices.end(), 
                   nodeB_elem_indices.begin(), nodeB_elem_indices.end(), 
                   std::inserter(temp_set, temp_set.begin()));
    all_indices.swap(temp_set); // temp_set will be deleted
    
    if (all_indices.size()==1) // nodes are only in one elment hence on boundary so merge nodes
    {
        /*
         * Looks like 
         * 
         *    A   B
         * ---o---o---
         * 
         * on the boundray of the tissue
         */
        PerformNodeMerge(pNodeA, pNodeB, all_indices);
    }
    else if (all_indices.size()==2) // nodes are in two elments hence on and interior boundary so merge nodes
    {
        if (nodeA_elem_indices.size()==2 && nodeB_elem_indices.size()==2)
        {
            /*
             * Looks like 
             * 
             *    A   B
             * ---o---o---
             * 
             * on an internal edge  
             */
             PerformNodeMerge(pNodeA, pNodeB, all_indices); 
        }
        else
        {
            /*
             * Looks like 
             *
             * Outside
             *         /
             *   --o--o (2)
             *     (1) \
             * 
             * Here we employ a PartialT1Swap 
             */
             PerformT1Swap(pNodeA, pNodeB, all_indices);
        }
    }
    else if (all_indices.size()==3) // nodes are contained in three elments 
    {
       /*
        * Looks like  
        * 
        *     A  B             A  B
        *   \                       /
        *    \  (1)           (1)  /
        * (3) o--o---   or  ---o--o (3)    Element number in brackets
        *    /  (2)           (2)  \
        *   /                       \
        *
        * Perform a PartialT1Swap 
        */
        PerformT1Swap(pNodeA, pNodeB, all_indices); /// \todo this line needs coverage (#821)
    }
    else if (all_indices.size()==4) // Correct set up for T1Swap 
    {
        /*
         * Looks like this
         * 
         *   \(1)/
         *    \ / Node A
         * (2) |   (4)     elements in Brackets
         *    / \ Node B
         *   /(3)\
         * 
         * Perform a T1Swap
         * 
         */  
        PerformT1Swap(pNodeA, pNodeB, all_indices);
    }
    else
    {
        std::cout << "\n nodes are in more than 4 elements so we can't remesh\n"; /// \todo these lines need coverage (#821)
        assert(0);
    }
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::PerformNodeMerge(Node<SPACE_DIM>* pNodeA, 
                                                          Node<SPACE_DIM>* pNodeB,
                                                          std::set<unsigned> elementsContainingNodes)
{
    c_vector<double, SPACE_DIM> node_midpoint = pNodeA->rGetLocation() + 0.5*GetVectorFromAtoB(pNodeA->rGetLocation(), pNodeB->rGetLocation());
    
    if (pNodeA->GetIndex() < pNodeB->GetIndex())
    {
        // Remove node B
        c_vector<double, SPACE_DIM>& r_nodeA_location = pNodeA->rGetModifiableLocation();
        r_nodeA_location = node_midpoint;
    
        for (std::set<unsigned>::const_iterator it = elementsContainingNodes.begin();
             it != elementsContainingNodes.end();
             ++it)
        {
            unsigned nodeB_local_index =  mElements[*it]->GetNodeLocalIndex(pNodeB->GetIndex());
            assert(nodeB_local_index < UINT_MAX); // this element should contain node B
        
            mElements[*it]->DeleteNode(nodeB_local_index); 
        }
    }
    else /// \todo these lines need coverage (#821)
    {
        // Remove node A
        c_vector<double, SPACE_DIM>& r_nodeB_location = pNodeB->rGetModifiableLocation();
        r_nodeB_location = node_midpoint;
    
        for (std::set<unsigned>::const_iterator it = elementsContainingNodes.begin();
             it != elementsContainingNodes.end();
             ++it)
        {
            unsigned nodeA_local_index =  mElements[*it]->GetNodeLocalIndex(pNodeA->GetIndex());
            assert(nodeA_local_index < UINT_MAX); // this element should contain node A
        
            mElements[*it]->DeleteNode(nodeA_local_index); 
        }
    }
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::PerformT1Swap(Node<SPACE_DIM>* pNodeA,
                                                       Node<SPACE_DIM>* pNodeB,
                                                       std::set<unsigned> elementsContainingNodes)
{
    // Make sure that we are in the correct dimension - this code will be eliminated at compile time
    #define COVERAGE_IGNORE
    assert(SPACE_DIM == 2); // only works in 2D at present
    assert(ELEMENT_DIM == SPACE_DIM);
    #undef COVERAGE_IGNORE
    
    /*
     * Restructure elements - remember to update nodes and elements.
     * 
     * We need to implement the following changes:
     * 
     * The element whose index was in nodeA_elem_indices but not nodeB_elem_indices,
     * and the element whose index was in nodeB_elem_indices but not nodeA_elem_indices,
     * should now both contain nodes A and B. 
     * 
     * The element whose index was in nodeA_elem_indices and nodeB_elem_indices, and which 
     * node C lies inside, should now only contain node A. 
     * 
     * The element whose index was in nodeA_elem_indices and nodeB_elem_indices, and which 
     * node D lies inside, should now only contain node B.
     * 
     * Iterate over all elements involved and identify which element they are 
     * in the diagram then update the nodes as necessary.
     * 
     *   \(1)/
     *    \ / Node A
     * (2) |   (4)     elements in brackets
     *    / \ Node B
     *   /(3)\
     * 
     */  

    /*
     * Compute the locations of two new nodes C, D, placed on either side of the 
     * edge E_old formed by nodes current_node and anticlockwise_node, such 
     * that the edge E_new formed by the new nodes is the perpendicular bisector 
     * of E_old, with |E_new| 'just larger' than mThresholdDistance.
     */
          
    // Find the sets of elements containing nodes A and B
    std::set<unsigned> nodeA_elem_indices = pNodeA->rGetContainingElementIndices();
    std::set<unsigned> nodeB_elem_indices = pNodeB->rGetContainingElementIndices();  

    double distance_between_nodes_CD = 2*mCellRearrangementThreshold;

    c_vector<double, SPACE_DIM> nodeA_location = pNodeA->rGetLocation();
    c_vector<double, SPACE_DIM> nodeB_location = pNodeB->rGetLocation();

    c_vector<double, SPACE_DIM> a_to_b = GetVectorFromAtoB(nodeA_location, nodeB_location);    
    c_vector<double, SPACE_DIM> perpendicular_vector;
    perpendicular_vector(0) = -a_to_b(1);
    perpendicular_vector(1) = a_to_b(0);

    c_vector<double, SPACE_DIM> c_to_d = distance_between_nodes_CD / norm_2(a_to_b) * perpendicular_vector;    
    c_vector<double, SPACE_DIM> nodeC_location = nodeA_location + 0.5*a_to_b - 0.5*c_to_d;
    c_vector<double, SPACE_DIM> nodeD_location = nodeC_location + c_to_d;

    /*
     * Move node A to C and node B to D
     */
     
    c_vector<double, SPACE_DIM>& r_nodeA_location = pNodeA->rGetModifiableLocation();
    r_nodeA_location = nodeC_location;
    
    c_vector<double, SPACE_DIM>& r_nodeB_location = pNodeB->rGetModifiableLocation();
    r_nodeB_location = nodeD_location;
    
    for (std::set<unsigned>::const_iterator it = elementsContainingNodes.begin();
         it != elementsContainingNodes.end();
         ++it)
    {
        if (nodeA_elem_indices.find(*it) == nodeA_elem_indices.end()) // not in nodeA_elem_indices so element 3
        {
            /*
             * In this case the element index was not in 
             * nodeA_elem_indices, so this element 
             * does not contain node A. Therefore we must add node A
             * (which has been moved to node C) to this element.
             *
             * Locate local index of node B in element then add node A after 
             * in anticlockwise direction. 
             */  
            
            unsigned nodeB_local_index =  mElements[*it]->GetNodeLocalIndex(pNodeB->GetIndex());
            assert(nodeB_local_index < UINT_MAX); // this element should contain node B
    
            mElements[*it]->AddNode(nodeB_local_index, pNodeA);
        }
        else if (nodeB_elem_indices.find(*it) == nodeB_elem_indices.end()) // not in nodeB_elem_indices so element 1
        {
            /*
             * In this case the element index was not in 
             * nodeB_elem_indices, so this element
             * does not contain node B. Therefore we must add node B
             * (which has been moved to node D) to this element.
             *
             * Locate local index of node A in element then add node B after 
             * in anticlockwise direction. 
             */  
            unsigned nodeA_local_index =  mElements[*it]->GetNodeLocalIndex(pNodeA->GetIndex());
            assert(nodeA_local_index < UINT_MAX); // this element should contain node A
            mElements[*it]->AddNode(nodeA_local_index, pNodeB); 
        }    
        else
        {
            /*
             * In this case the element index was in both nodeB_elem_indices and nodeB_elem_indices
             * so is element 2 or 4 
             */
            
            /*
             * Locate local index of nodeA and nodeB and use the oredering to 
             * identify the element if nodeB_index > nodeA_index then element 4
             * and if nodeA_index > nodeB_index then element 2 
             */  
            unsigned nodeA_local_index =  mElements[*it]->GetNodeLocalIndex(pNodeA->GetIndex());
            assert(nodeA_local_index < UINT_MAX); // this element should contain node A
            
            unsigned nodeB_local_index =  mElements[*it]->GetNodeLocalIndex(pNodeB->GetIndex());
            assert(nodeB_local_index < UINT_MAX); // this element should contain node B
     
            unsigned nodeB_local_index_plus_one = (nodeB_local_index + 1)%(mElements[*it]->GetNumNodes());
            
            if (nodeA_local_index == nodeB_local_index_plus_one)
            {
                /*
                 * In this case the local index of nodeA is the local index of 
                 * nodeB plus one so we are in element 2 so we remove nodeB
                 */
                 mElements[*it]->DeleteNode(nodeB_local_index); 
            }
            else
            {
                assert(nodeB_local_index == (nodeA_local_index + 1)%(mElements[*it]->GetNumNodes())); // as A and B are next to each other
                /*
                 * In this case the local index of nodeA is the local index of 
                 * nodeB minus one so we are in element 4 so we remove nodeA
                 */             
                 mElements[*it]->DeleteNode(nodeA_local_index); 
            }
        }
    }
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
unsigned VertexMesh<ELEMENT_DIM, SPACE_DIM>::DivideElement(VertexElement<ELEMENT_DIM,SPACE_DIM>* pElement)
{
    // Make sure that we are in the correct dimension - this code will be eliminated at compile time
    #define COVERAGE_IGNORE
    assert(SPACE_DIM == 2); // only works in 2D at present
    assert(ELEMENT_DIM == SPACE_DIM);
    #undef COVERAGE_IGNORE

    // Find short axis
    unsigned element_index = pElement->GetIndex();
    c_vector<double, SPACE_DIM> centroid = GetCentroidOfElement(element_index);
    c_vector<double, SPACE_DIM> short_axis = GetShortAxisOfElement(element_index);

    // Find long axis
    c_vector<double, SPACE_DIM> long_axis; // this is perpendicular to the short axis
    long_axis(0) = -short_axis(1);
    long_axis(1) = short_axis(0);

    /// \todo Remove this temporary vector of bools?

    unsigned num_nodes = pElement->GetNumNodes();

    // Store if the node is on the side of the short axis which the long axis points to 
    bool is_on_left[num_nodes];
    
    for (unsigned i=0; i<num_nodes; i++)
    {
        c_vector<double, SPACE_DIM> node_location_from_centroid = pElement->GetNodeLocation(i) - centroid; 
        
        if (inner_prod(node_location_from_centroid, long_axis) >= 0)
        {
            is_on_left[i] = true;
        }
        else // inner_prod(node_location_from_centroid,long_axis)<0
        {
            is_on_left[i] = false;
        }
    }

    std::vector<unsigned> intersecting_nodes;
        
    for (unsigned i=0; i<num_nodes-1; i++)
    {
        if (is_on_left[i]!=is_on_left[i+1])
        {
            intersecting_nodes.push_back(i);
        }
    }
    if (is_on_left[0]!=is_on_left[num_nodes-1])
    {
        intersecting_nodes.push_back(num_nodes-1);
    }

    if (intersecting_nodes.size()!=2) /// \todo this line needs coverage (#821)
    {
        EXCEPTION("Cannot proceed with cell division algorithm - the number of intersecting nodes is not equal to 2");
    }

    std::vector<unsigned> new_node_global_indices;  

    for (unsigned i=0; i<intersecting_nodes.size(); i++)
    {
        // Find intersections between edges and short_axis
        c_vector<double, SPACE_DIM> position_a = pElement->GetNodeLocation(intersecting_nodes[i]);
        c_vector<double, SPACE_DIM> position_b;
        if (intersecting_nodes[i] < num_nodes-1)
        {
            position_b = pElement->GetNodeLocation(intersecting_nodes[i]+1);
        }
        else
        {
            position_b = pElement->GetNodeLocation(0);
        }
        
        c_vector<double, SPACE_DIM> a_to_b = GetVectorFromAtoB(position_a, position_b);
        
        /*
         * Let the first one on edge be a and the second one be b, 
         * then we are interested in position_a + alpha * a_to_b 
         */

        double determinant = a_to_b[0]*short_axis[1] - a_to_b[1]*short_axis[0];
         
        double alpha = (centroid[0]*a_to_b[1] - position_a[0]*a_to_b[1]
                        -centroid[1]*a_to_b[0] + position_a[1]*a_to_b[0])/determinant;

        c_vector<double, SPACE_DIM> intersection = centroid + alpha*short_axis;

        // Add new node to the mesh
        unsigned node_global_index = this->AddNode(new Node<SPACE_DIM>(0, false, intersection[0], intersection[1]));

        // Store index of new node
        new_node_global_indices.push_back(node_global_index);
    }

    for (unsigned i=0; i<intersecting_nodes.size(); i++)
    {
        // Get pointers to the nodes forming the edge into which one new node will be inserted
        
        /* 
         * Note that when we use the first entry of intersecting_nodes to add a node,
         * we change the local index of the second entry of intersecting_nodes in 
         * pElement, so must account for this by moving one entry further on.
         */

        Node<SPACE_DIM>* p_node_A = pElement->GetNode((intersecting_nodes[i]+i)%pElement->GetNumNodes());         
        Node<SPACE_DIM>* p_node_B = pElement->GetNode((intersecting_nodes[i]+i+1)%pElement->GetNumNodes());

        // Find the indices of the elements owned by each node on the edge into which one new node will be inserted
        std::set<unsigned> elems_containing_node1 = p_node_A->rGetContainingElementIndices();
        std::set<unsigned> elems_containing_node2 = p_node_B->rGetContainingElementIndices();
    
        // Find common elements
        std::set<unsigned> shared_elements;
        std::set_intersection(elems_containing_node1.begin(),
                              elems_containing_node1.end(),
                              elems_containing_node2.begin(),
                              elems_containing_node2.end(),
                              std::inserter(shared_elements, shared_elements.begin()));

        // Iterate over common elements
        for (std::set<unsigned>::iterator iter=shared_elements.begin();
             iter!=shared_elements.end();
             ++iter)
        {
            // Find which node has the lower local index in this element
            unsigned local_indexA = GetElement(*iter)->GetNodeLocalIndex(p_node_A->GetIndex());
            unsigned local_indexB = GetElement(*iter)->GetNodeLocalIndex(p_node_B->GetIndex());

            unsigned index = local_indexB;
            if (local_indexB > local_indexA)
            {
                index = local_indexA;                 
            }
            if ( (local_indexA == 0) && (local_indexB == GetElement(*iter)->GetNumNodes()-1))
            {
                index = local_indexB;
            }
    		if ( (local_indexB == 0) && (local_indexA == GetElement(*iter)->GetNumNodes()-1))
            {
                index = local_indexA;
            }

            // Add new node to this element
            GetElement(*iter)->AddNode(index, GetNode(new_node_global_indices[i]));    		
        }
    }

    // Now call DivideElement() to divide the element using the new nodes
    unsigned new_element_index = DivideElement(pElement, pElement->GetNodeLocalIndex(new_node_global_indices[0]), pElement->GetNodeLocalIndex(new_node_global_indices[1]));
    return new_element_index;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
unsigned VertexMesh<ELEMENT_DIM, SPACE_DIM>::DivideElement(VertexElement<ELEMENT_DIM,SPACE_DIM>* pElement, unsigned nodeAIndex, unsigned nodeBIndex)
{
    // Make sure that we are in the correct dimension - this code will be eliminated at compile time
    #define COVERAGE_IGNORE
    assert(SPACE_DIM == 2); // only works in 2D at present
    assert(ELEMENT_DIM == SPACE_DIM);
    #undef COVERAGE_IGNORE

    // Sort nodeA and nodeB such that nodeBIndex>nodeAindex
    assert(nodeBIndex!=nodeAIndex);
    
    unsigned node1Index;
    unsigned node2Index;
    
    if (nodeAIndex < nodeBIndex)
    {
        node1Index = nodeAIndex;
        node2Index = nodeBIndex;
    }
    else
    {
        node1Index = nodeBIndex;
        node2Index = nodeAIndex;
    }

    // Copy element  
    std::vector<Node<SPACE_DIM>*> nodes_elem;
    unsigned num_nodes = pElement->GetNumNodes(); // store this as it changes when you delete nodes from element
    
    for (unsigned i=0; i<num_nodes; i++)
    {
        nodes_elem.push_back(pElement->GetNode(i));
    }

    unsigned new_element_index;
    if (mDeletedElementIndices.empty())
    {
        new_element_index = mElements.size();
    }
    else /// \todo these lines need coverage (#821)
    {
        new_element_index = mDeletedElementIndices.back();
        mDeletedElementIndices.pop_back();
        delete mElements[new_element_index];        
    }

    AddElement(new VertexElement<ELEMENT_DIM,SPACE_DIM>(new_element_index, nodes_elem));

    // Remove nodes  # < node1 and # > node2 from pElement  
    // Remove nodes node1 < # < node2 from new_element
    
    for (unsigned i=num_nodes; i>0; i--)
    {
        if (i-1<node1Index || i-1>node2Index)
        {
            pElement->DeleteNode(i-1);
        }
        else if (i-1>node1Index && i-1<node2Index)
        {
            mElements[new_element_index]->DeleteNode(i-1);
        }
    }
    return new_element_index;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::ConstructFromMeshReader(VertexMeshReader2d& rMeshReader)
{
    // Store numbers of nodes and elements
    unsigned num_nodes = rMeshReader.GetNumNodes();
    unsigned num_elements = rMeshReader.GetNumElements();
    
    // Reserve memory for nodes
    mNodes.reserve(num_nodes);

    rMeshReader.Reset();

    // Add nodes
    std::vector<double> node_data;
    for (unsigned i=0; i<num_nodes; i++)
    {
        node_data = rMeshReader.GetNextNode();
        unsigned is_boundary_node = node_data[2];
        node_data.pop_back();
        mNodes.push_back(new Node<SPACE_DIM>(i, node_data, is_boundary_node));
    }

    rMeshReader.Reset();

    // Reserve memory for nodes
    mElements.reserve(rMeshReader.GetNumElements());

    // Add elements
    for (unsigned elem_index=0; elem_index<num_elements; elem_index++)
    {
        VertexElementData element_data = rMeshReader.GetNextElementData();
        std::vector<Node<SPACE_DIM>*> nodes;

        unsigned num_nodes_in_element = element_data.NodeIndices.size();
        for (unsigned j=0; j<num_nodes_in_element; j++)
        {
            assert(element_data.NodeIndices[j] < mNodes.size());
            nodes.push_back(mNodes[element_data.NodeIndices[j]]);
        }

        VertexElement<ELEMENT_DIM,SPACE_DIM>* p_element = new VertexElement<ELEMENT_DIM,SPACE_DIM>(elem_index, nodes);
        mElements.push_back(p_element);
    
        if (rMeshReader.GetNumElementAttributes() > 0)
        {
            assert(rMeshReader.GetNumElementAttributes() == 1);
            unsigned attribute_value = element_data.AttributeValue;
            p_element->SetRegion(attribute_value);
        }        
    }
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void VertexMesh<ELEMENT_DIM, SPACE_DIM>::Scale(const double xScale, const double yScale, const double zScale)
{
    for (unsigned i=0; i<GetNumNodes(); i++)
    {
        c_vector<double, SPACE_DIM>& r_location = mNodes[i]->rGetModifiableLocation();

        if (SPACE_DIM>=3)
        {
            r_location[2] *= zScale;
        }
        if (SPACE_DIM>=2)
        {
            r_location[1] *= yScale;
        }
        r_location[0] *= xScale;
    }
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
bool VertexMesh<ELEMENT_DIM, SPACE_DIM>::ElementIncludesPoint(const c_vector<double, SPACE_DIM>& testPoint, unsigned elementIndex)
{
    // Make sure that we are in the correct dimension - this code will be eliminated at compile time
    #define COVERAGE_IGNORE
    assert(SPACE_DIM == 2); // only works in 2D at present
    assert(ELEMENT_DIM == SPACE_DIM);
    #undef COVERAGE_IGNORE

    // Initialise boolean
    bool element_includes_point = false;

    // Get the element
    VertexElement<ELEMENT_DIM, SPACE_DIM>* p_element = GetElement(elementIndex);
    unsigned num_nodes = p_element->GetNumNodes();

    // Loop over edges of the element
    for (unsigned local_index=0; local_index<num_nodes; local_index++)
    {
        // Get the end points of this edge
        c_vector<double, SPACE_DIM> vertexA = p_element->GetNodeLocation(local_index);
        c_vector<double, SPACE_DIM> vertexB = p_element->GetNodeLocation((local_index+1)%num_nodes);

        // Check if this edge crosses the ray running out horizontally (increasing x, fixed y) from the test point

        double x = testPoint[0];
        double y = testPoint[1];

        c_vector<double, SPACE_DIM> vector_a_to_point = GetVectorFromAtoB(vertexA, testPoint);
        c_vector<double, SPACE_DIM> vector_b_to_point = GetVectorFromAtoB(vertexB, testPoint);
        c_vector<double, SPACE_DIM> vector_a_to_b = GetVectorFromAtoB(vertexA, vertexB);

        // Pathological case - test point coincides with vertexA or vertexB
        if (    (norm_2(vector_a_to_point) < DBL_EPSILON) 
             || (norm_2(vector_b_to_point) < DBL_EPSILON) )
        {
            return false;
        }
        
        // Pathological case - ray coincides with horizontal edge
        if ( (fabs(vector_a_to_b[1]) < DBL_EPSILON) &&
             (fabs(vector_a_to_point[1]) < DBL_EPSILON) && 
             (fabs(vector_b_to_point[1]) < DBL_EPSILON) )
        {
            if ( (vector_a_to_point[0]>0) != (vector_b_to_point[0]>0) )
            {
                return false;
            }
        }

        /// \todo Need to carefully check all pathological cases (see #933)

        // Non-pathological case        
        if ( (vertexA[1]>y) != (vertexB[1]>y) )
        {
            if (x < vertexA[0] + vector_a_to_b[0]*vector_a_to_point[1]/vector_a_to_b[1])
            {
                element_includes_point = !element_includes_point;
            }
        }
    }
    return element_includes_point;
}


/////////////////////////////////////////////////////////////////////////////////////
// Explicit instantiation
/////////////////////////////////////////////////////////////////////////////////////

template class VertexMesh<1,1>;
template class VertexMesh<1,2>;
template class VertexMesh<2,2>;
template class VertexMesh<2,3>;
template class VertexMesh<3,3>;
