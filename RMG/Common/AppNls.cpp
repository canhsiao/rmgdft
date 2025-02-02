/*
 *
 * Copyright 2014 The RMG Project Developers. See the COPYRIGHT file 
 * at the top-level directory of this distribution or in the current
 * directory.
 * 
 * This file is part of RMG. 
 * RMG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * any later version.
 *
 * RMG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
*/


#include "transition.h"
#include "const.h"
#include "BaseThread.h"
#include "TradeImages.h"
#include "RmgTimer.h"
#include "RmgThread.h"
#include "RmgException.h"
#include "rmgtypedefs.h"
#include "params.h"
#include "typedefs.h"
#include "common_prototypes.h"
#include "common_prototypes1.h"
#include "rmg_error.h"
#include "State.h"
#include "Kpoint.h"
#include "RmgGemm.h"
#include "GpuAlloc.h"

#include "GlobalSums.h"
#include "blas.h"

template void AppNls<double>(Kpoint<double> *, double *, double *, double *, double *, double *, int, int, bool);
template void AppNls<std::complex<double> >(Kpoint<std::complex<double>> *, std::complex<double> *, 
                std::complex<double> *, std::complex<double> *, std::complex<double> *, std::complex<double> *, int, int, bool);

template void AppNls<double>(Kpoint<double> *, double *, double *, double *, double *, double *, int, int);
template void AppNls<std::complex<double> >(Kpoint<std::complex<double>> *, std::complex<double> *, 
                std::complex<double> *, std::complex<double> *, std::complex<double> *, std::complex<double> *, int, int);


// Version that computes and returns Bns
template <typename KpointType>
void AppNls(Kpoint<KpointType> *kpoint, KpointType *sintR, 
            KpointType *psi, KpointType *nv, KpointType *ns, KpointType *Bns, 
            int first_state, int num_states)
{
    AppNls(kpoint, sintR, psi, nv, ns, Bns, first_state, num_states, true);
}

// Version that lets you choose to return Bns
template <typename KpointType>
void AppNls(Kpoint<KpointType> *kpoint, KpointType *sintR, 
            KpointType *psi, KpointType *nv, KpointType *ns, KpointType *Bns, 
            int first_state, int num_states, bool need_bns)
{

    // Sanity check
    if(num_states > ct.non_local_block_size)
        throw RmgFatalException() << "AppNls called with num_states > non_local_block_size in " << __FILE__ << " at line " << __LINE__ << "\n";
 
    int P0_BASIS = kpoint->pbasis;
    int num_nonloc_ions = kpoint->BetaProjector->get_num_nonloc_ions();
    int *nonloc_ions_list = kpoint->BetaProjector->get_nonloc_ions_list();
    int num_tot_proj = kpoint->BetaProjector->get_num_tot_proj();

    KpointType ZERO_t(0.0);
    KpointType ONE_t(1.0);

    char *transa = "n";

    double *dnmI;
    double *qqq;
    KpointType *psintR;
    size_t stop = (size_t)num_states * (size_t)P0_BASIS;

    if(num_tot_proj == 0)
    {
        bool need_ns = true;
        if(ct.norm_conserving_pp && (ct.discretization != MEHRSTELLEN_DISCRETIZATION) && ct.is_gamma) need_ns = false;
        if(!ct.norm_conserving_pp) for(size_t i = 0; i < stop; i++) Bns[i] = ZERO_t;
        if(need_ns) for(size_t idx = 0;idx < stop;idx++) ns[idx] = psi[idx];
        if(ct.xc_is_hybrid) 
        {
            for(size_t i = 0; i < stop; i++) nv[i] = ct.exx_fraction * kpoint->vexx[i];
        }
        else
        {
            for(size_t i = 0; i < stop; i++) nv[i] = ZERO_t;
        }
        return;
    }


    size_t alloc = (size_t)num_tot_proj * (size_t)num_states;
    size_t M_cols = 1;
    if(ct.is_ddd_non_diagonal) M_cols = (size_t)num_tot_proj;
    size_t alloc1 = (size_t)num_tot_proj * (size_t)M_cols;

#if GPU_ENABLED
    KpointType *sint_compack = (KpointType *)GpuMallocManaged(sizeof(KpointType) * alloc);
    KpointType *nwork = (KpointType *)GpuMallocManaged(sizeof(KpointType) * alloc);
    KpointType *M_dnm = (KpointType *)GpuMallocManaged(sizeof(KpointType) * alloc1);
    KpointType *M_qqq = (KpointType *)GpuMallocManaged(sizeof(KpointType) * alloc1);
    for(int i = 0;i < alloc;i++) sint_compack[i] = 0.0;
#else
    KpointType *sint_compack = new KpointType[alloc]();
    KpointType *nwork = new KpointType[alloc];
    KpointType *M_dnm = new KpointType [alloc1];
    KpointType *M_qqq = new KpointType [alloc1];
#endif

    for(int istate = 0; istate < num_states; istate++)
    {
        int sindex = (istate + first_state) * num_nonloc_ions * ct.max_nl;
        for (int ion = 0; ion < num_nonloc_ions; ion++)
        {
            int proj_index = ion * ct.max_nl;
            psintR = &sintR[proj_index + sindex];
            //psintI = &sintI[ion * num_states * ct.max_nl + sindex];
            /*Actual index of the ion under consideration*/
            int gion = nonloc_ions_list[ion];
            SPECIES &AtomType = Species[Atoms[gion].species];

            int nh = AtomType.nh;
            for (int i = 0; i < nh; i++)
            {
                sint_compack[istate * num_tot_proj + proj_index + i] = psintR[i];
            }
        }
    }

    for (size_t i = 0; i < alloc1; i++)
    {
        M_dnm[i] = ZERO_t;
        M_qqq[i] = ZERO_t;
    }


    // set up M_qqq and M_dnm, this can be done outside in the
    // init.c or get_ddd get_qqq, we need to check the order
    int proj_index = 0;
    for (int ion = 0; ion < num_nonloc_ions; ion++)
    {

        /*Actual index of the ion under consideration*/
        proj_index = ion * ct.max_nl;
        int gion = nonloc_ions_list[ion];
        SPECIES &AtomType = Species[Atoms[gion].species];

        int nh = AtomType.nh;

        dnmI = pct.dnmI[gion];
        qqq = pct.qqq[gion];

        for (int i = 0; i < nh; i++)
        {
            int inh = i * nh;
            for (int j = 0; j < nh; j++)
            {

                if(ct.is_ddd_non_diagonal) {
                    int idx = (proj_index + i) * num_tot_proj + proj_index + j;
                    M_dnm[idx] = (KpointType)dnmI[inh+j];
                    M_qqq[idx] = (KpointType)qqq[inh+j];
                }
                else {
                    // Diagonal for norm conserving so just save those.
                    if((proj_index + i) == (proj_index + j)) {
                        M_dnm[proj_index + j] = (KpointType)dnmI[inh+j];
                        M_qqq[proj_index + j] = (KpointType)qqq[inh+j];
                    }
                }

            }
        }
    }



    if(!ct.norm_conserving_pp) {

        RmgGemm (transa, transa, num_tot_proj, num_states, num_tot_proj,
                    ONE_t, M_dnm,  num_tot_proj, sint_compack, num_tot_proj,
                    ZERO_t,  nwork, num_tot_proj);

        RmgGemm (transa, transa, P0_BASIS, num_states, num_tot_proj,
                    ONE_t, kpoint->nl_Bweight,  P0_BASIS, nwork, num_tot_proj,
                    ZERO_t,  nv, P0_BASIS);

#if GPU_ENABLED
        cudaMemcpy(ns, psi, stop*sizeof(KpointType), cudaMemcpyDefault);
#else
        memcpy(ns, psi, stop*sizeof(KpointType));
#endif

        RmgGemm (transa, transa, num_tot_proj, num_states, num_tot_proj, 
                ONE_t, M_qqq,  num_tot_proj, sint_compack, num_tot_proj,
                ZERO_t,  nwork, num_tot_proj);

        RmgGemm (transa, transa, P0_BASIS, num_states, num_tot_proj, 
                ONE_t, kpoint->nl_weight,  P0_BASIS, nwork, num_tot_proj,
                ONE_t,  ns, P0_BASIS);

        if(need_bns) {
            RmgGemm (transa, transa, P0_BASIS, num_states, num_tot_proj, 
                    ONE_t, kpoint->nl_Bweight,  P0_BASIS, nwork, num_tot_proj,
                    ZERO_t,  Bns, P0_BASIS);
        }

    }
    else 
    {

        if(ct.is_ddd_non_diagonal)
        {
            RmgGemm (transa, transa, num_tot_proj, num_states, num_tot_proj,
                        ONE_t, M_dnm,  num_tot_proj, sint_compack, num_tot_proj,
                        ZERO_t,  nwork, num_tot_proj);
        }
        else
        {
// Optimize for GPU's!
            for(int jj = 0;jj < num_states;jj++) {
                for(int ii = 0;ii < num_tot_proj;ii++) {
                    nwork[jj*num_tot_proj + ii] = M_dnm[ii] * sint_compack[jj*num_tot_proj + ii];
                }
            }
        }

        RmgGemm (transa, transa, P0_BASIS, num_states, num_tot_proj,
                    ONE_t, kpoint->nl_Bweight,  P0_BASIS, nwork, num_tot_proj,
                    ZERO_t,  nv, P0_BASIS);

#if GPU_ENABLED
// For norm conserving and gamma ns=psi so other parts of code were updated to not require this
        if(!ct.is_gamma)
            cudaMemcpy(ns, psi, stop*sizeof(KpointType), cudaMemcpyDefault);
#else
        if(!ct.is_gamma)
            memcpy(ns, psi, stop*sizeof(KpointType));
#endif

    }

    if(ct.xc_is_hybrid)
    {
        for(size_t i = 0; i < stop; i++) nv[i] += ct.exx_fraction * kpoint->vexx[i];
    }

#if GPU_ENABLED
    GpuFreeManaged(M_qqq);
    GpuFreeManaged(M_dnm);
    GpuFreeManaged(nwork);
    GpuFreeManaged(sint_compack);
#else
    delete [] nwork;
    delete [] sint_compack;
    delete [] M_qqq;
    delete [] M_dnm;
#endif


    // Add in ldaU contributions to nv
    if(ct.ldaU_mode == LDA_PLUS_U_SIMPLE)
    {
        kpoint->ldaU->app_vhubbard(nv, kpoint->orbitalsint_local, first_state, num_states);
    }

}

