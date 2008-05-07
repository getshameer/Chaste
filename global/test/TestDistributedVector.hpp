/*
Copyright (C) University of Oxford, 2008

This file is part of Chaste.

Chaste is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Chaste is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with Chaste.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TESTDISTRIBUTEDVECTOR_HPP_
#define TESTDISTRIBUTEDVECTOR_HPP_

#include <cxxtest/TestSuite.h>
#include <petscvec.h>
#include <petscmat.h>

#include "PetscSetupAndFinalize.hpp"
#include "PetscException.hpp"

#include "DistributedVector.hpp"
#include "PetscTools.hpp"

class TestDistributedVector : public CxxTest::TestSuite
{
public:
    
    void TestRead()
    {
        // WRITE VECTOR
        // create a 10 element petsc vector
        unsigned vec_size = 10u;
        Vec vec;
        VecCreate(PETSC_COMM_WORLD, &vec);
        VecSetSizes(vec, PETSC_DECIDE, vec_size);
        VecSetFromOptions(vec);
        // calculate the range
        PetscInt petsc_lo, petsc_hi;
        VecGetOwnershipRange(vec, &petsc_lo, &petsc_hi);
        unsigned lo=(unsigned)petsc_lo;
        unsigned hi=(unsigned)petsc_hi;   
        // create 20 element petsc vector
        Vec striped;
        VecCreateMPI(PETSC_COMM_WORLD, 2*(hi-lo) , 2*vec_size, &striped);
        // write some values
        double* p_vec;
        VecGetArray(vec, &p_vec);
        double* p_striped;
        VecGetArray(striped, &p_striped);
        for (unsigned global_index=lo; global_index<hi; global_index++)
        {
            unsigned local_index = global_index - lo;
            p_vec[local_index] = local_index*global_index;
            p_striped[2*local_index  ] = local_index;
            p_striped[2*local_index+1] = global_index*global_index;
        }
        VecRestoreArray(vec, &p_vec);
        VecAssemblyBegin(vec);
        VecAssemblyEnd(vec);
        VecRestoreArray(striped, &p_striped);
        VecAssemblyBegin(striped);
        VecAssemblyEnd(striped);
        
        // READ VECTOR
        DistributedVector::SetProblemSize(vec);
        DistributedVector distributed_vector(vec);
        DistributedVector distributed_vector2(striped);
        DistributedVector::Stripe linear(distributed_vector2,0);
        DistributedVector::Stripe quadratic(distributed_vector2,1);
        // check the range
        TS_ASSERT_EQUALS(DistributedVector::GetProblemSize(), vec_size);
        TS_ASSERT_EQUALS(DistributedVector::Begin().Global,lo);
        TS_ASSERT_EQUALS(DistributedVector::End().Global,hi);
        // read some values
        for (DistributedVector::Iterator index = DistributedVector::Begin();
             index!= DistributedVector::End();
             ++index)
        {
            TS_ASSERT_EQUALS(distributed_vector[index], index.Local*index.Global);
            TS_ASSERT_EQUALS(linear[index], index.Local);
            TS_ASSERT_EQUALS(quadratic[index], index.Global * index.Global);
        }
        
        // read the 2nd element of the first vector
        if (lo<=2 && 2<hi)
        {
            TS_ASSERT(DistributedVector::IsGlobalIndexLocal(2));
            TS_ASSERT_EQUALS(distributed_vector[2],2*(2-lo));
        }
        else
        {
            TS_ASSERT(!DistributedVector::IsGlobalIndexLocal(2));
            TS_ASSERT_THROWS_ANYTHING(distributed_vector[2]);
        }
        
        //read the 3rd element of the other vectors
        if (lo<=3 && 3<hi)
        {
            TS_ASSERT(DistributedVector::IsGlobalIndexLocal(3));
            TS_ASSERT_EQUALS(linear[3],(3-lo));
            TS_ASSERT_EQUALS(quadratic[3],3*3);
        }
        else
        {
            TS_ASSERT(!DistributedVector::IsGlobalIndexLocal(3));
            TS_ASSERT_THROWS_ANYTHING(linear[3]);
            TS_ASSERT_THROWS_ANYTHING(quadratic[3]);
        }
        
        VecDestroy(vec);
        VecDestroy(striped);
    }
    
    void TestWrite()
    {
        //WRITE VECTOR
        // create a 10 element petsc vector
        DistributedVector::SetProblemSize(10);
        Vec striped=DistributedVector::CreateVec(2);
        Vec petsc_vec=DistributedVector::CreateVec();

        DistributedVector distributed_vector(petsc_vec);
        DistributedVector distributed_vector2(striped);
        DistributedVector::Stripe linear(distributed_vector2,0);
        DistributedVector::Stripe quadratic(distributed_vector2,1);
        // write some values
        for (DistributedVector::Iterator index = DistributedVector::Begin();
             index!= DistributedVector::End();
             ++index)
        {
            distributed_vector[index] =  -(double)(index.Local*index.Global);
            linear[index] =  1;
            quadratic[index] =  index.Local+1;
        }
        
        distributed_vector.Restore();
        distributed_vector2.Restore();
        
        //READ VECTOR
        // calculate my range
        PetscInt petsc_lo, petsc_hi;
        VecGetOwnershipRange(petsc_vec,&petsc_lo,&petsc_hi);
        unsigned lo=(unsigned)petsc_lo;
        unsigned hi=(unsigned)petsc_hi;   
        // read some values
        double* p_striped;
        VecGetArray(striped, &p_striped);
        double* p_vec;
        VecGetArray(petsc_vec, &p_vec);
        for (unsigned global_index=lo; global_index<hi; global_index++)
        {
            unsigned local_index = global_index - lo;
            TS_ASSERT_EQUALS(p_vec[local_index], -(double)local_index*global_index);
            TS_ASSERT_EQUALS(p_striped[2*local_index], (double)1);
            TS_ASSERT_EQUALS(p_striped[2*local_index+1], local_index+1);
        }
        
        VecDestroy(petsc_vec);
        VecDestroy(striped);
    }
    
    void TestException()
    {
        TS_ASSERT_THROWS_ANYTHING(throw DistributedVectorException() );
    }
    
    void TestUnevenDistribution()
    {
        unsigned my_rank = PetscTools::GetMyRank();
        unsigned num_procs = PetscTools::NumProcs();
        
        //Calculate total number of elements in the vector
        unsigned total_elements = (num_procs+1)*num_procs/2;
        
        DistributedVector::SetProblemSizePerProcessor(total_elements, my_rank+1);
        
        Vec petsc_vec = DistributedVector::CreateVec(1);
        
        PetscInt petsc_lo, petsc_hi;
        VecGetOwnershipRange(petsc_vec,&petsc_lo,&petsc_hi);
        
        unsigned expected_lo = (my_rank+1)*my_rank/2;
        unsigned expected_hi = (my_rank+2)*(my_rank+1)/2;
        
        TS_ASSERT_EQUALS((unsigned)petsc_lo, expected_lo);
        TS_ASSERT_EQUALS((unsigned)petsc_hi, expected_hi);
        
        VecDestroy(petsc_vec);
    }
};

#endif /*TESTDISTRIBUTEDVECTOR_HPP_*/
