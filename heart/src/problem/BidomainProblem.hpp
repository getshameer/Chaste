/*

Copyright (C) University of Oxford, 2008

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


#ifndef BIDOMAINPROBLEM_HPP_
#define BIDOMAINPROBLEM_HPP_



#include "BidomainDg0Assembler.hpp"
#include "BidomainPde.hpp"
#include "AbstractCardiacProblem.hpp"
#include "BidomainMatrixBasedAssembler.hpp"
#include "BidomainWithBathAssembler.hpp"

/**
 * Class which specifies and solves a bidomain problem.
 *
 * The solution vector is of the form:
 * (V_1, phi_1, V_2, phi_2, ......, V_N, phi_N),
 * where V_j is the voltage at node j and phi_j is the
 * extracellular potential at node j.
 */
template<unsigned SPACE_DIM>
class BidomainProblem : public AbstractCardiacProblem<SPACE_DIM, 2>
{

friend class TestBidomainWithBathAssembler;    
    
protected:
    BidomainPde<SPACE_DIM>* mpBidomainPde;

private:
    std::vector<unsigned> mFixedExtracellularPotentialNodes; /** nodes at which the extracellular voltage is fixed to zero (replicated) */
    unsigned mExtracelluarColumnId;
    unsigned mRowMeanPhiEZero;
    bool mHasBath;

    /**
     *  Create normal initial condition but overwrite V to zero for bath nodes, if 
     *  there are any.
     */
    Vec CreateInitialCondition()
    {
        Vec init_cond = AbstractCardiacProblem<SPACE_DIM,2>::CreateInitialCondition();
        if(mHasBath)
        {
            // get the voltage stripe
            DistributedVector ic(init_cond);
            DistributedVector::Stripe voltage_stripe = DistributedVector::Stripe(ic,0);

            for (DistributedVector::Iterator index = DistributedVector::Begin();
                 index!= DistributedVector::End();
                 ++index)
            {
                if(this->mpMesh->GetNode( index.Global )->GetRegion()==BidomainWithBathAssembler<SPACE_DIM,SPACE_DIM>::BATH)
                {
                    voltage_stripe[index] = 0.0;
                }
            }
            ic.Restore();
        }
        
        return init_cond;
    }

protected:
    AbstractCardiacPde<SPACE_DIM> *CreateCardiacPde()
    {
        mpBidomainPde = new BidomainPde<SPACE_DIM>(this->mpCellFactory);

        return mpBidomainPde;
    }

    AbstractDynamicAssemblerMixin<SPACE_DIM, SPACE_DIM, 2>* CreateAssembler()
    {
        BidomainDg0Assembler<SPACE_DIM, SPACE_DIM>* p_bidomain_assembler;
        
        if(mHasBath)
        {
            p_bidomain_assembler
                = new BidomainWithBathAssembler<SPACE_DIM,SPACE_DIM>(this->mpMesh,
                                                                     mpBidomainPde,
                                                                     this->mpBoundaryConditionsContainer,
                                                                     2);
        }
        else
        {
            if(!this->mUseMatrixBasedRhsAssembly)
            {
                p_bidomain_assembler
                    = new BidomainDg0Assembler<SPACE_DIM,SPACE_DIM>(this->mpMesh,
                                                                    mpBidomainPde,
                                                                    this->mpBoundaryConditionsContainer,
                                                                    2);
            }
            else 
            {
                p_bidomain_assembler
                    = new BidomainMatrixBasedAssembler<SPACE_DIM,SPACE_DIM>(this->mpMesh,
                                                                            mpBidomainPde,
                                                                            this->mpBoundaryConditionsContainer,
                                                                            2);
            }
        }

        try
        {
            p_bidomain_assembler->SetFixedExtracellularPotentialNodes(mFixedExtracellularPotentialNodes);
            p_bidomain_assembler->SetRowForMeanPhiEToZero(mRowMeanPhiEZero);
        }
        catch (const Exception& e)
        {
            delete p_bidomain_assembler;
            throw e;
        }

        return p_bidomain_assembler;
    }

public:
    /**
     * Constructor 
     * @param pCellFactory User defined cell factory which shows how the pde should
     * create cells.
     */
    BidomainProblem(AbstractCardiacCellFactory<SPACE_DIM>* pCellFactory, bool hasBath=false)
            : AbstractCardiacProblem<SPACE_DIM, 2>(pCellFactory),
            mpBidomainPde(NULL), 
            mRowMeanPhiEZero(INT_MAX),
            mHasBath(hasBath)
    {
        mFixedExtracellularPotentialNodes.resize(0); 
    }

    /**
     *  Set the nodes at which phi_e (the extracellular potential) is fixed to
     *  zero. This does not necessarily have to be called. If it is not, phi_e
     *  is only defined up to a constant.
     *
     *  @param the nodes to be fixed.
     *
     *  NOTE: currently, the value of phi_e at the fixed nodes cannot be set to be
     *  anything other than zero.
     */
    void SetFixedExtracellularPotentialNodes(std::vector<unsigned> nodes)
    {
        mFixedExtracellularPotentialNodes.resize(nodes.size());
        for (unsigned i=0; i<nodes.size(); i++)
        {
            // the assembler checks that the nodes[i] is less than
            // the number of nodes in the mesh so this is not done here
            mFixedExtracellularPotentialNodes[i] = nodes[i];
        }
    }

    /**
     * Set which row of the linear system should be used to enforce the
     * condition that the mean of phi_e is zero.  If not called, this
     * condition will not be used.
     */
    void SetRowForMeanPhiEToZero(unsigned rowMeanPhiEZero)
    {
        // Row should be odd in C++-like indexing
        if (rowMeanPhiEZero % 2 == 0)
        {
            EXCEPTION("Row for enforcing mean phi_e = 0 should be odd in C++ style indexing");
        }

        mRowMeanPhiEZero = rowMeanPhiEZero;
    }

    /**
     *  Get the pde. Can only be called after Initialise()
     */
    BidomainPde<SPACE_DIM>* GetBidomainPde()
    {
        assert(mpBidomainPde!=NULL);
        return mpBidomainPde;
    }

    /**
     *  Print out time and max/min voltage/phi_e values at current time.
     */
    void WriteInfo(double time)
    {
        std::cout << "Solved to time " << time << "\n" << std::flush;
        ReplicatableVector voltage_replicated;
        voltage_replicated.ReplicatePetscVector(this->mVoltage);
        double v_max = -1e5, v_min = 1e5, phi_max = -1e5, phi_min = 1e5;

        for (unsigned i=0; i<this->mpMesh->GetNumNodes(); i++)
        {
            double v=voltage_replicated[2*i];            
            double phi=voltage_replicated[2*i+1];
  
            #define COVERAGE_IGNORE
            if (isnan(v) || isnan(phi))
            {
                EXCEPTION("Not-a-number encountered");
            }
            #undef COVERAGE_IGNORE
            if ( v > v_max)
            {
                v_max = v;
            }
            if ( v < v_min)
            {
                v_min = v;
            }
            if ( phi > phi_max)
            {
                phi_max = phi;
            }
            if ( phi < phi_min)
            {
                phi_min = phi;
            }
        }
        std::cout << " V; phi_e = " << "[" <<v_min << ", " << v_max << "]" << ";\t"
                  << "[" <<phi_min << ", " << phi_max << "]" << "\n"
                  << std::flush;
    }

    virtual void DefineWriterColumns()
    {
        AbstractCardiacProblem<SPACE_DIM,2>::DefineWriterColumns();
        mExtracelluarColumnId = this->mpWriter->DefineVariable("Phi_e","mV");
    }

    virtual void WriteOneStep(double time, Vec voltageVec)
    {
        this->mpWriter->PutUnlimitedVariable(time);
        this->mpWriter->PutStripedVector(this->mVoltageColumnId, mExtracelluarColumnId, voltageVec);
    }
    
    void PreSolveChecks()
    {
        AbstractCardiacProblem<SPACE_DIM, 2>::PreSolveChecks();
        if (mFixedExtracellularPotentialNodes.empty())
        {
            // We're not pinning any nodes.
            if (mRowMeanPhiEZero==INT_MAX)
            {
                // We're not using the mean phi_e method, hence use a null space
                //Check that the KSP solver isn't going to do anything stupid:
                // phi_e is not bounded, so it'd be wrong to use a relative tolerance
                if (HeartConfig::Instance()->GetUseRelativeTolerance())
                {
                    EXCEPTION("Bidomain external voltage is not bounded in this simulation - use KSP *absolute* tolerance");                      
                }    
            }
        }
    }

};


#endif /*BIDOMAINPROBLEM_HPP_*/
