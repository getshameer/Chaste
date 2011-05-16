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

#ifndef NODESONLYMESH_HPP_
#define NODESONLYMESH_HPP_

#include "ChasteSerialization.hpp"


#include "MutableMesh.hpp"

/**
 * Mesh class for storing lists of nodes (no elements). This inherits from mutable
 * mesh because we want to be able to add and delete nodes.
 */
template<unsigned SPACE_DIM>
class NodesOnlyMesh: public MutableMesh<SPACE_DIM, SPACE_DIM> 
{
    
template<class Archive>
void serialize(Archive & archive, const unsigned int version)
{
    archive & mCellRadii;    
    archive & boost::serialization::base_object<MutableMesh<SPACE_DIM, SPACE_DIM> >(*this);
   
}
    
private:
///\todo #1762 Add details of the nodes' radii here. 

    /**
     * List of cell radii
     */
    std::vector<double> mCellRadii;
    
public:
    /**
     * Construct the mesh using only nodes.  No mesh is created, but the nodes are stored.
     * The original vector of nodes is deep-copied: new node objects are made with are
     * independent of the pointers in the input so that they can be safely deleted.
     * 
     * If this is the only way of constructing a mesh of this type, then we can be certain that
     * elements and boundary elements are always unused.
     *
     * @param rNodes the vector of nodes
     */
    void ConstructNodesWithoutMesh(const std::vector< Node<SPACE_DIM>*> & rNodes);
    
    /*
     * Get radius of cell associated with a node
     */
    double GetCellRadius(unsigned index);
     
    /*
    * Deletes nodes
    */
     
    void Del(unsigned index); 
     
     /*
      * Set radius of a cell associated with a node
      */
    void SetCellRadius(unsigned index, double radius);
    
};

#endif /*NODESONLYMESH_HPP_*/
