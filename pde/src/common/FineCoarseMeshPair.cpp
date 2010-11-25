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


#include "FineCoarseMeshPair.hpp"


template<unsigned DIM>
FineCoarseMeshPair<DIM>::FineCoarseMeshPair(TetrahedralMesh<DIM,DIM>& rFineMesh, QuadraticMesh<DIM>& rCoarseMesh)
    : mrFineMesh(rFineMesh),
      mrCoarseMesh(rCoarseMesh)
{
    mpFineMeshBoxCollection = NULL;
    mpCoarseMeshBoxCollection = NULL;
    ResetStatisticsVariables();
}



template<unsigned DIM>
FineCoarseMeshPair<DIM>::~FineCoarseMeshPair()
{
    DeleteFineBoxCollection();
    DeleteCoarseBoxCollection();
}

template<unsigned DIM>
void FineCoarseMeshPair<DIM>::DeleteFineBoxCollection()
{
    if(mpFineMeshBoxCollection != NULL)
    {
        delete mpFineMeshBoxCollection;
        mpFineMeshBoxCollection = NULL;
    }
}


template<unsigned DIM>
void FineCoarseMeshPair<DIM>::DeleteCoarseBoxCollection()
{
    if(mpCoarseMeshBoxCollection != NULL)
    {
        delete mpCoarseMeshBoxCollection;
        mpCoarseMeshBoxCollection = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////
//
//
//   Setting up boxes methods
//
//
////////////////////////////////////////////////////////////////////////////////////

template<unsigned DIM>
void FineCoarseMeshPair<DIM>::SetUpBoxesOnFineMesh(double boxWidth)
{
    SetUpBoxes(mrFineMesh, boxWidth, mpFineMeshBoxCollection);
}

template<unsigned DIM>
void FineCoarseMeshPair<DIM>::SetUpBoxesOnCoarseMesh(double boxWidth)
{
    SetUpBoxes(mrCoarseMesh, boxWidth, mpCoarseMeshBoxCollection);
}


template<unsigned DIM>
void FineCoarseMeshPair<DIM>::SetUpBoxes(TetrahedralMesh<DIM,DIM>& rMesh,
                                         double boxWidth,
                                         BoxCollection<DIM>*& rpBoxCollection)
{
    if(rpBoxCollection)
    {
        delete rpBoxCollection;
        rpBoxCollection = NULL;
    }
    
    // compute min and max values for the fine mesh nodes
    ChasteCuboid<DIM> bounding_box = rMesh.CalculateBoundingBox();
    
    // set up the boxes. Use a domain which is a touch larger than the fine mesh
    c_vector<double,2*DIM> extended_min_and_max;
    for(unsigned i=0; i<DIM; i++)
    {
        double width = bounding_box.GetWidth(i);
 
        // subtract from the minima
        extended_min_and_max(2*i) = bounding_box.rGetLowerCorner()[i] - 0.05*width;
        // add to the maxima
        extended_min_and_max(2*i+1) = bounding_box.rGetUpperCorner()[i] + 0.05*width;
    }



    if(boxWidth < 0)
    {
        // use default value = max(max_edge_length, w20),  where w20 is the width corresponding to 
        // 20 boxes in the x-direction 

        // BoxCollection creates an extra box so divide by 19 not 20.  Add a little bit on to ensure
        // minor numerical fluctuations don't change the answer.
        boxWidth = (extended_min_and_max(1) - extended_min_and_max(0))/19.000000001;

        // determine the maximum edge length
        double max_edge_length = -1;

        for (typename TetrahedralMesh<DIM,DIM>::EdgeIterator edge_iterator = rMesh.EdgesBegin();
             edge_iterator!=rMesh.EdgesEnd();
             ++edge_iterator)
        {
            c_vector<double, 3> location1 = edge_iterator.GetNodeA()->rGetLocation();
            c_vector<double, 3> location2 = edge_iterator.GetNodeB()->rGetLocation();
            double edge_length = norm_2(location1-location2);

            if(edge_length>max_edge_length)
            {
                max_edge_length = edge_length;
            }
        }
        
        if(boxWidth < max_edge_length)
        {
            boxWidth = 1.1*max_edge_length;
        }
    }
 
    rpBoxCollection = new BoxCollection<DIM>(boxWidth, extended_min_and_max);
    rpBoxCollection->SetupAllLocalBoxes();

    // for each element, if ANY of its nodes are physically in a box, put that element
    // in that box
    for(unsigned i=0; i<rMesh.GetNumElements(); i++)
    {
        Element<DIM,DIM>* p_element = rMesh.GetElement(i);

        std::set<unsigned> box_indices_each_node_this_elem;
        for(unsigned j=0; j<DIM+1; j++) // num vertices per element
        {
            Node<DIM>* p_node = p_element->GetNode(j);
            unsigned box_index = rpBoxCollection->CalculateContainingBox(p_node);
            box_indices_each_node_this_elem.insert(box_index);
        }

        for(std::set<unsigned>::iterator iter = box_indices_each_node_this_elem.begin();
            iter != box_indices_each_node_this_elem.end();
            ++iter)
        {
            rpBoxCollection->rGetBox( *iter ).AddElement(p_element);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////
// 
// 
// ComputeFineElementsAndWeightsForCoarseQuadPoints() 
// and 
// ComputeFineElementsAndWeightsForCoarseNodes()
// and 
// common method
//
//
////////////////////////////////////////////////////////////////////////////////////

template<unsigned DIM>
void FineCoarseMeshPair<DIM>::ComputeFineElementsAndWeightsForCoarseQuadPoints(GaussianQuadratureRule<DIM>& rQuadRule,
                                                                               bool safeMode)
{
    if(mpFineMeshBoxCollection==NULL)
    {
        EXCEPTION("Call SetUpBoxesOnFineMesh() before ComputeFineElementsAndWeightsForCoarseQuadPoints()");
    }

    // get the quad point (physical) positions
    QuadraturePointsGroup<DIM> quad_point_posns(mrCoarseMesh, rQuadRule);

    // resize the elements and weights vector.
    mFineMeshElementsAndWeights.resize(quad_point_posns.Size());

    #ifdef FINECOARSEMESHPAIR_VERBOSE
    std::cout << "\nComputing fine elements and weights for coarse quad points\n";
    #endif

    ResetStatisticsVariables();
    for(unsigned i=0; i<quad_point_posns.Size(); i++)
    {
        #ifdef FINECOARSEMESHPAIR_VERBOSE
        std::cout << "\r " << i << " of " << quad_point_posns.Size() << std::flush;
        #endif
        
        // get the box this point is in
        unsigned box_for_this_point = mpFineMeshBoxCollection->CalculateContainingBox( quad_point_posns.Get(i) );

        // a chaste point version of the c-vector is needed for the GetContainingElement call.
        ChastePoint<DIM> point(quad_point_posns.Get(i));

        ComputeFineElementAndWeightForGivenPoint(point, safeMode, box_for_this_point, i);
    }

    if(mStatisticsCounters[1]>0)
    {
        std::stringstream stream;
        stream << mStatisticsCounters[1] << " of " << quad_point_posns.Size() << " coarse-mesh quadrature points were outside the fine mesh"; 
        WARNING(stream.str());
    }
}


template<unsigned DIM>
void FineCoarseMeshPair<DIM>::ComputeFineElementsAndWeightsForCoarseNodes(bool safeMode)
{
    if(mpFineMeshBoxCollection==NULL)
    {
        EXCEPTION("Call SetUpBoxesOnFineMesh() before ComputeFineElementsAndWeightsForCoarseNodes()");
    }

    // resize the elements and weights vector.
    mFineMeshElementsAndWeights.resize(mrCoarseMesh.GetNumNodes());

    #ifdef FINECOARSEMESHPAIR_VERBOSE
    std::cout << "\nComputing fine elements and weights for coarse nodes\n";
    #endif

    ResetStatisticsVariables();
    for(unsigned i=0; i<mrCoarseMesh.GetNumNodes(); i++)
    {
        #ifdef FINECOARSEMESHPAIR_VERBOSE
        std::cout << "\r " << i << " of " << mrCoarseMesh.GetNumNodes() << std::flush;
        #endif
        
        Node<DIM>* p_node = mrCoarseMesh.GetNode(i);
        
        // get the box this point is in
        unsigned box_for_this_point = mpFineMeshBoxCollection->CalculateContainingBox( p_node->rGetModifiableLocation() );

        // a chaste point version of the c-vector is needed for the GetContainingElement call.
        ChastePoint<DIM> point(p_node->rGetLocation());

        ComputeFineElementAndWeightForGivenPoint(point, safeMode, box_for_this_point, i);
    }
}


///\todo: could possibly merge with ComputeCoarseElementForGivenPoint(). Difference between the
/// methods are: this uses fine mesh and fine mesh box, computes weights as well (and sets
/// the element and weight in the vec), rather than returning the element, and this method 
/// saves information in mStatisticsCounters
template<unsigned DIM>
void FineCoarseMeshPair<DIM>::ComputeFineElementAndWeightForGivenPoint(ChastePoint<DIM>& rPoint, 
                                                                       bool safeMode,
                                                                       unsigned boxForThisPoint,
                                                                       unsigned index)
{
    std::set<unsigned> test_element_indices;

    // the elements to try (initially) are those contained in the box the point is in
    // NOTE: it is possible the point to be in an element inot 'in' this box, as it is possible
    // for all element nodes to be in different boxes.
    CollectElementsInContainingBox(mpFineMeshBoxCollection, boxForThisPoint, test_element_indices);

    unsigned elem_index;
    c_vector<double,DIM+1> weight;

    try
    {
        //std::cout << "\n" << "# test elements initially " << test_element_indices.size() << "\n";
        // try these elements only, initially
        elem_index = mrFineMesh.GetContainingElementIndex(rPoint,
                                                          false,
                                                          test_element_indices,
                                                          true /* quit if not in test_elements */);
        weight = mrFineMesh.GetElement(elem_index)->CalculateInterpolationWeights(rPoint);

        mStatisticsCounters[0]++;
    }
    catch(Exception& e)
    {
        // now try all the elements, but trying the elements contained in the boxes local to this
        // element first
        std::set<unsigned> test_element_indices;

        CollectElementsInLocalBoxes(mpFineMeshBoxCollection, boxForThisPoint, test_element_indices);

        try
        {
            elem_index = mrFineMesh.GetContainingElementIndex(rPoint,
                                                              false,
                                                              test_element_indices,
                                                              true);
            weight = mrFineMesh.GetElement(elem_index)->CalculateInterpolationWeights(rPoint);

            mStatisticsCounters[0]++;

        }
        catch(Exception& e)
        {
            if(safeMode)
            {
                // try the remaining elements
                try
                {
                    elem_index = mrFineMesh.GetContainingElementIndex(rPoint,
                                                                      false);
                    weight = mrFineMesh.GetElement(elem_index)->CalculateInterpolationWeights(rPoint);
                    mStatisticsCounters[0]++;

                }
                catch (Exception& e)
                {
                    // the point is not in ANY element, store the nearest element and corresponding weights
                    elem_index = mrFineMesh.GetNearestElementIndexFromTestElements(rPoint,test_element_indices);
                    weight = mrFineMesh.GetElement(elem_index)->CalculateInterpolationWeights(rPoint);

                    mNotInMesh.push_back(index);
                    mNotInMeshNearestElementWeights.push_back(weight);
                    mStatisticsCounters[1]++;
                }
            }
            else
            {
                assert(test_element_indices.size()>0); // boxes probably too small if this fails
                
                // immediately assume it isn't in the rest of the mesh - this should be the 
                // case assuming the box width was chosen suitably.
                // Store the nearest element and corresponding weights
                elem_index = mrFineMesh.GetNearestElementIndexFromTestElements(rPoint,test_element_indices);
                weight = mrFineMesh.GetElement(elem_index)->CalculateInterpolationWeights(rPoint);

                mNotInMesh.push_back(index);
                mNotInMeshNearestElementWeights.push_back(weight);
                mStatisticsCounters[1]++;
            }
        }
    }

    mFineMeshElementsAndWeights[index].ElementNum = elem_index;
    mFineMeshElementsAndWeights[index].Weights = weight;    
}

////////////////////////////////////////////////////////////////////////////////////
//
//
// ComputeCoarseElementsForFineNodes
// and 
// ComputeCoarseElementsForFineElementCentroids
// and
// common method
//
//
////////////////////////////////////////////////////////////////////////////////////



template<unsigned DIM>
void FineCoarseMeshPair<DIM>::ComputeCoarseElementsForFineNodes(bool safeMode)
{
    if(mpCoarseMeshBoxCollection==NULL)
    {
        EXCEPTION("Call SetUpBoxesOnCoarseMesh() before ComputeCoarseElementsForFineNodes()");
    }

    #ifdef FINECOARSEMESHPAIR_VERBOSE
    std::cout << "\nComputing coarse elements for fine nodes\n";
    #endif

    mCoarseElementsForFineNodes.resize(mrFineMesh.GetNumNodes());
    
    ResetStatisticsVariables();
    for(unsigned i=0; i<mCoarseElementsForFineNodes.size(); i++)
    {
        #ifdef FINECOARSEMESHPAIR_VERBOSE
        std::cout << "\r " << i << " of " << mCoarseElementsForFineNodes.size() << std::flush;
        #endif
        
        ChastePoint<DIM> point = mrFineMesh.GetNode(i)->GetPoint();
        // get the box this point is in
        unsigned box_for_this_point = mpCoarseMeshBoxCollection->CalculateContainingBox( mrFineMesh.GetNode(i)->rGetModifiableLocation() );

        mCoarseElementsForFineNodes[i] = ComputeCoarseElementForGivenPoint(point, safeMode, box_for_this_point);
    }
}


template<unsigned DIM>
void FineCoarseMeshPair<DIM>::ComputeCoarseElementsForFineElementCentroids(bool safeMode)
{
    if(mpCoarseMeshBoxCollection==NULL)
    {
        EXCEPTION("Call SetUpBoxesOnCoarseMesh() before ComputeCoarseElementsForFineElementCentroids()");
    }    

    #ifdef FINECOARSEMESHPAIR_VERBOSE
    std::cout << "\nComputing coarse elements for fine element centroids\n";
    #endif
    
    mCoarseElementsForFineElementCentroids.resize(mrFineMesh.GetNumElements());
    
    ResetStatisticsVariables();
    for(unsigned i=0; i<mrFineMesh.GetNumElements(); i++)
    {
        #ifdef FINECOARSEMESHPAIR_VERBOSE
        std::cout << "\r " << i << " of " << mrFineMesh.GetNumElements() << std::flush;
        #endif
        
        c_vector<double,DIM> point_cvec = mrFineMesh.GetElement(i)->CalculateCentroid();
        ChastePoint<DIM> point(point_cvec);

        // get the box this point is in
        unsigned box_for_this_point = mpCoarseMeshBoxCollection->CalculateContainingBox( point_cvec );
        
        mCoarseElementsForFineElementCentroids[i] = ComputeCoarseElementForGivenPoint(point, safeMode, box_for_this_point);
    }
}


///\todo: could possibly merge with ComputeFineElementAndWeightForGivenPoint(). Differences between the
/// methods are: the other method uses fine mesh and fine mesh box, computes weights as well (and sets
/// the element and weight in the vec), rather than returning the element, and that method 
/// saves information in mStatisticsCounters
template<unsigned DIM>
unsigned FineCoarseMeshPair<DIM>::ComputeCoarseElementForGivenPoint(ChastePoint<DIM>& rPoint, 
                                                                    bool safeMode,
                                                                    unsigned boxForThisPoint)
{    
    std::set<unsigned> test_element_indices;
    CollectElementsInContainingBox(mpCoarseMeshBoxCollection, boxForThisPoint, test_element_indices);

    unsigned elem_index;

    try
    {
        elem_index = mrCoarseMesh.GetContainingElementIndex(rPoint,
                                                            false,
                                                            test_element_indices,
                                                            true /* quit if not in test_elements */);

        mStatisticsCounters[0]++;
    }
    catch(Exception& e)
    {
        // now try all the elements, but trying the elements contained in the boxes local to this
        // element first
        std::set<unsigned> test_element_indices;
        CollectElementsInLocalBoxes(mpCoarseMeshBoxCollection, boxForThisPoint, test_element_indices);

        try
        {
            elem_index = mrCoarseMesh.GetContainingElementIndex(rPoint,
                                                                false,
                                                                test_element_indices,
                                                                true);
            
            mStatisticsCounters[0]++;
        }
        catch(Exception& e)
        {
            if(safeMode)
            {
                // try the remaining elements
                try
                {
                    elem_index = mrCoarseMesh.GetContainingElementIndex(rPoint,
                                                                        false);

                    mStatisticsCounters[0]++;
                }
                catch (Exception& e)
                {
                    // the point is not in ANY element, store the nearest element and corresponding weights
                    elem_index = mrCoarseMesh.GetNearestElementIndexFromTestElements(rPoint,test_element_indices);
                    mStatisticsCounters[1]++;
                }
            }
            else
            {
                assert(test_element_indices.size()>0); // boxes probably too small if this fails
                
                // immediately assume it isn't in the rest of the mesh - this should be the 
                // case assuming the box width was chosen suitably.
                // Store the nearest element and corresponding weights
                elem_index = mrCoarseMesh.GetNearestElementIndexFromTestElements(rPoint,test_element_indices);
                mStatisticsCounters[1]++;
            }
        }
    }

    return elem_index;
}




////////////////////////////////////////////////////////////////////////////////////
// 
// helper methods for code
//
////////////////////////////////////////////////////////////////////////////////////

template<unsigned DIM>
void FineCoarseMeshPair<DIM>::CollectElementsInContainingBox(BoxCollection<DIM>*& rpBoxCollection, 
                                                             unsigned boxIndex,
                                                             std::set<unsigned>& rElementIndices)
{
    for(typename std::set<Element<DIM,DIM>*>::iterator elem_iter = rpBoxCollection->rGetBox(boxIndex).rGetElementsContained().begin();
        elem_iter != rpBoxCollection->rGetBox(boxIndex).rGetElementsContained().end();
        ++elem_iter)
    {
        rElementIndices.insert((*elem_iter)->GetIndex());
    }
}


template<unsigned DIM>
void FineCoarseMeshPair<DIM>::CollectElementsInLocalBoxes(BoxCollection<DIM>*& rpBoxCollection, 
                                                          unsigned boxIndex,
                                                          std::set<unsigned>& rElementIndices)
{
    std::set<unsigned> local_boxes = rpBoxCollection->GetLocalBoxes(boxIndex);
    for(std::set<unsigned>::iterator local_box_iter = local_boxes.begin();
        local_box_iter != local_boxes.end();
         ++local_box_iter)
    {
        for(typename std::set<Element<DIM,DIM>*>::iterator elem_iter = rpBoxCollection->rGetBox(*local_box_iter).rGetElementsContained().begin();
            elem_iter != rpBoxCollection->rGetBox(*local_box_iter).rGetElementsContained().end();
            ++elem_iter)
        {
            rElementIndices.insert((*elem_iter)->GetIndex());
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////
// 
// statistics related methods
//
////////////////////////////////////////////////////////////////////////////////////


template<unsigned DIM> 
void FineCoarseMeshPair<DIM>::ResetStatisticsVariables()
{
    mNotInMesh.clear();
    mNotInMeshNearestElementWeights.clear();
    mStatisticsCounters.resize(2, 0u);
}

template<unsigned DIM>
void FineCoarseMeshPair<DIM>::PrintStatistics()
{
    std::cout << "\nFineCoarseMeshPair statistics for the last-called method:\n";

//    std::cout << "\tNum points for which containing element was found, using box containing that point = " << mStatisticsCounters[0] << "\n";
//    std::cout << "\tNum points for which containing element was in local box = " << mStatisticsCounters[1] << "\n";
//    std::cout << "\tNum points for which containing element was in an element in a non-local box = " << mStatisticsCounters[2] << "\n";
//    std::cout << "\tNum points for which no containing element was found = " << mStatisticsCounters[3] << "\n";

    std::cout << "\tNum points for which containing element was found: " << mStatisticsCounters[0] << "\n";
    std::cout << "\tNum points for which no containing element was found = " << mStatisticsCounters[1] << "\n";

    if(mNotInMesh.size()>0)
    {
        std::cout << "\tIndices and weights for points (nodes/quad points) for which no containing element was found:\n";
        for(unsigned i=0; i<mNotInMesh.size(); i++)
        {
            std::cout << "\t\t" << mNotInMesh[i] << ", " << mNotInMeshNearestElementWeights[i] << "\n";
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// Explicit instantiation
/////////////////////////////////////////////////////////////////////////////


template class FineCoarseMeshPair<1>;
template class FineCoarseMeshPair<2>;
template class FineCoarseMeshPair<3>;