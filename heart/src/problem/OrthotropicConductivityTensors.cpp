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

#include "OrthotropicConductivityTensors.hpp"
#include "Exception.hpp"

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void OrthotropicConductivityTensors<ELEMENT_DIM, SPACE_DIM>::Init(AbstractTetrahedralMesh<ELEMENT_DIM, SPACE_DIM> *pMesh) throw (Exception)
{
    this->mpMesh = pMesh;
    //assert(0);
    if (!this->mUseNonConstantConductivities && !this->mUseFibreOrientation)
    {
        // Constant tensor for every element
        c_matrix<double, SPACE_DIM, SPACE_DIM> conductivity_matrix(zero_matrix<double>(SPACE_DIM,SPACE_DIM));

        for (unsigned dim=0; dim<SPACE_DIM; dim++)
        {
            assert(this->mConstantConductivities(dim) != DBL_MAX);
            conductivity_matrix(dim,dim) = this->mConstantConductivities(dim);
        }

        this->mTensors.push_back(conductivity_matrix);
    }
    else
    {
        c_matrix<double,SPACE_DIM,SPACE_DIM> orientation_matrix((identity_matrix<double>(SPACE_DIM)));

        if (this->mUseFibreOrientation)
        {
            // open file
            this->mFileReader.reset(new FibreReader<SPACE_DIM>(this->mFibreOrientationFile, ORTHO));
            this->mNumElements = this->mFileReader->GetNumLinesOfData();
            assert(this->mNumElements == this->mpMesh->GetNumElements());
            
        }
        else
        {
            this->mNumElements = this->mpNonConstantConductivities->size();
            
            assert(this->mNumElements == this->mpMesh->GetNumElements());
        }
        if (this->mUseNonConstantConductivities)
        {
            ///\todo #1342 Most of these checks should be redundant or exceptions
            assert(this->mpNonConstantConductivities->size() == this->mpMesh->GetNumElements());
        }

        // reserve() allocates all the memory at once, more efficient than relying
        // on the automatic reallocation scheme.
        this->mTensors.reserve(this->mNumElements);

        c_matrix<double, SPACE_DIM, SPACE_DIM> conductivity_matrix(zero_matrix<double>(SPACE_DIM,SPACE_DIM));

      for (typename AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>::ElementIterator it = this->mpMesh->GetElementIteratorBegin();
             it != this->mpMesh->GetElementIteratorEnd();
             ++it)
        {
            unsigned global_element_index = it->GetIndex();
            /*
             *  For every element of the mesh we compute its tensor like (from
             * "Laminar Arrangement of VentricularMyocites Influences Electrical
             * Behavior of the Heart", Darren et al. 2007):
             *
             *                         [g_f  0   0 ] [a_f']
             *  tensor = [a_f a_l a_n] [ 0  g_l  0 ] [a_l']
             *                         [ 0   0  g_n] [a_n']
             *
             *              [x_i]
             *  where a_i = [y_i], i={f,l,n} are read from the fibre orientation file and
             *              [z_i]
             *
             *  g_f = fibre/longitudinal conductivity (constant or element specific)
             *  g_l = laminar/transverse conductivity (constant or element specific)
             *  g_n = normal conductivity (constant or element specific)
             *
             */
            if (this->mUseNonConstantConductivities)
            {
                for (unsigned dim=0; dim<SPACE_DIM; dim++)
                {
                    conductivity_matrix(dim,dim) = (*this->mpNonConstantConductivities)[global_element_index][dim];
                }
            }
            else
            {
                for (unsigned dim=0; dim<SPACE_DIM; dim++)
                {
                    assert(this->mConstantConductivities(dim) != DBL_MAX);
                    conductivity_matrix(dim,dim) = this->mConstantConductivities(dim);
                }
            }

            if (this->mUseFibreOrientation)
            {
                this->mFileReader->GetNextFibreSheetAndNormalMatrix(orientation_matrix);
            }

            c_matrix<double,SPACE_DIM,SPACE_DIM> temp;
            noalias(temp) = prod(orientation_matrix, conductivity_matrix);
            this->mTensors.push_back( prod(temp, trans(orientation_matrix) ) );
        }

        if (this->mUseFibreOrientation)
        {
            // close fibre file
            this->mFileReader.reset();
        }
    }

    this->mInitialised = true;
}


/////////////////////////////////////////////////////////////////////
// Explicit instantiation
/////////////////////////////////////////////////////////////////////

template class OrthotropicConductivityTensors<1,1>;
template class OrthotropicConductivityTensors<1,2>;
template class OrthotropicConductivityTensors<1,3>;
template class OrthotropicConductivityTensors<2,2>;
template class OrthotropicConductivityTensors<2,3>;
template class OrthotropicConductivityTensors<3,3>;
