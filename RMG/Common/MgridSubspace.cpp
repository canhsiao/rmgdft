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


#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "transition.h"
#include "const.h"
#include "State.h"
#include "Kpoint.h"
#include "BaseThread.h"
#include "TradeImages.h"
#include "RmgTimer.h"
#include "RmgThread.h"
#include "GlobalSums.h"
#include "Subdiag.h"
#include "rmgthreads.h"
#include "packfuncs.h"
#include "typedefs.h"
#include "common_prototypes.h"
#include "common_prototypes1.h"
#include "rmg_error.h"
#include "Kpoint.h"
#include "GatherScatter.h"
#include "../Headers/prototypes.h"


// Solver that uses multigrid preconditioning and subspace rotations

template <typename OrbitalType> void MgridSubspace (Kpoint<OrbitalType> *kptr, double *);



template void MgridSubspace<double> (Kpoint<double> *, double *);
template void MgridSubspace<std::complex<double> > (Kpoint<std::complex<double>> *, double *);

template <typename OrbitalType> void MgridSubspace (Kpoint<OrbitalType> *kptr, double *vtot_psi)
{
    RmgTimer RT0("3-MgridSubspace"), *RT1;
    BaseThread *T = BaseThread::getBaseThread(0);
    int my_pe_x, my_pe_y, my_pe_z;
    kptr->G->pe2xyz(pct.gridpe, &my_pe_x, &my_pe_y, &my_pe_z);
    int my_pe_offset = my_pe_x % pct.coalesce_factor;

    double mean_occ_res = DBL_MAX;
    double mean_unocc_res = DBL_MAX;
    double max_occ_res = 0.0;
    double max_unocc_res = 0.0;
    double min_occ_res = DBL_MAX;
    double min_unocc_res = DBL_MAX;
    bool potential_acceleration = (ct.potential_acceleration_constant_step > 0.0);
    int pbasis = kptr->pbasis;


    double *nvtot_psi = vtot_psi;;
    if(pct.coalesce_factor > 1)
    {
        nvtot_psi = new double[pbasis * pct.coalesce_factor];
        GatherGrid(kptr->G, pbasis, vtot_psi, nvtot_psi);
    }
    
    // Set trade images coalesce_factor
    kptr->T->set_coalesce_factor(pct.coalesce_factor);
    for(int vcycle = 0;vcycle < ct.eig_parm.mucycles;vcycle++)
    {

        int active_threads = ct.MG_THREADS_PER_NODE;
        if(ct.mpi_queue_mode && (active_threads > 1)) active_threads--;

        // Zero out dvh array if potential acceleration is enabled
        if(potential_acceleration)
        {
           int stop = kptr->ndvh * kptr->pbasis * pct.coalesce_factor;
           for(int i=0;i < stop;i++) kptr->dvh[i] = 0.0;
           PotentialAccelerationReset(my_pe_offset*active_threads + kptr->dvh_skip/pct.coalesce_factor);
        }

        // Update betaxpsi        
        RT1 = new RmgTimer("3-MgridSubspace: Beta x psi");
        Betaxpsi (kptr, 0, kptr->nstates, kptr->newsint_local);
        delete(RT1);

        /* Update the wavefunctions */
        int istop = kptr->nstates / (active_threads * pct.coalesce_factor);
        istop = istop * active_threads * pct.coalesce_factor;

        // Apply the non-local operators to a block of orbitals
        RT1 = new RmgTimer("3-MgridSubspace: AppNls");
        AppNls(kptr, kptr->newsint_local, kptr->Kstates[0].psi, kptr->nv, kptr->ns, kptr->Bns,
               0, std::min(ct.non_local_block_size, kptr->nstates));
        delete(RT1);
        int first_nls = 0;

        int st1 = 0;
        while(st1 < kptr->nstates)
        {

            // Adjust thread count in case num_states is not evenly divisible by the number of threads
            while(active_threads > 1)
            {
                int icheck = st1 + active_threads*pct.coalesce_factor;
                if(icheck > kptr->nstates) 
                {
                    active_threads--;
                }
                else
                {
                    break;
                }
            }

            SCF_THREAD_CONTROL thread_control;

            // Make sure the non-local operators are applied for the next block if needed
            int check = first_nls + active_threads*pct.coalesce_factor;
            if(check > ct.non_local_block_size) 
            {
                RT1 = new RmgTimer("3-MgridSubspace: AppNls");
                AppNls(kptr, kptr->newsint_local, kptr->Kstates[st1].psi, kptr->nv, &kptr->ns[st1 * pbasis], kptr->Bns,
                       st1, std::min(ct.non_local_block_size, kptr->nstates - st1));
                first_nls = 0;
                delete(RT1);
            }
        
            RT1 = new RmgTimer("3-MgridSubspace: Mg_eig");
            int istart = my_pe_offset*active_threads;
            for(int ist = 0;ist < active_threads;ist++) {
                if((st1 + ist + istart) >= kptr->nstates) break;
                thread_control.job = HYBRID_EIG;
                thread_control.vtot = nvtot_psi;
                thread_control.vcycle = vcycle;
                thread_control.sp = &kptr->Kstates[st1 + ist + istart];
                thread_control.p3 = (void *)kptr;
                thread_control.nv = (void *)&kptr->nv[(first_nls + ist + istart) * pbasis];
                thread_control.ns = (void *)&kptr->ns[(st1 + ist + istart) * pbasis];  // ns is not blocked!
                thread_control.basetag = kptr->Kstates[st1 + ist + istart].istate;
                thread_control.extratag1 = active_threads;
                thread_control.extratag2 = st1;
                QueueThreadTask(ist, thread_control);
            }

            // Thread tasks are set up so run them
            if(!ct.mpi_queue_mode) T->run_thread_tasks(active_threads);
            if((check >= ct.non_local_block_size) && ct.mpi_queue_mode) T->run_thread_tasks(active_threads, Rmg_Q);

            delete RT1;

            // Increment index into non-local block and state index
            first_nls += active_threads*pct.coalesce_factor;
            st1+=active_threads*pct.coalesce_factor;
        }

        RT1 = new RmgTimer("3-MgridSubspace: Mg_eig");
        if(ct.mpi_queue_mode) T->run_thread_tasks(active_threads, Rmg_Q);
        delete RT1;
    }

    // Set trade images coalesce factor back to 1 for other routines.
    kptr->T->set_coalesce_factor(1);

    if(pct.coalesce_factor > 1)
    {
        delete [] nvtot_psi;
        // Eigenvalues are not copied to all nodes in MgEigState when using coalesced grids.
        GatherEigs(kptr);
    }

    if(Verify ("freeze_occupied", true, kptr->ControlMap)) {

        // Orbital residual measures (used for some types of calculations
        kptr->max_unocc_res_index = (int)(ct.gw_residual_fraction * (double)kptr->nstates);
        kptr->mean_occ_res = 0.0;
        kptr->min_occ_res = DBL_MAX;
        kptr->max_occ_res = 0.0;
        kptr->mean_unocc_res = 0.0;
        kptr->min_unocc_res = DBL_MAX;
        kptr->max_unocc_res = 0.0;
        kptr->highest_occupied = 0;
        for(int istate = 0;istate < kptr->nstates;istate++) {
            if(kptr->Kstates[istate].occupation[0] > 0.0) {
                kptr->mean_occ_res += kptr->Kstates[istate].res;
                mean_occ_res += kptr->Kstates[istate].res;
                if(kptr->Kstates[istate].res >  kptr->max_occ_res)  kptr->max_occ_res = kptr->Kstates[istate].res;
                if(kptr->Kstates[istate].res <  kptr->min_occ_res)  kptr->min_occ_res = kptr->Kstates[istate].res;
                if(kptr->Kstates[istate].res >  max_occ_res)  max_occ_res = kptr->Kstates[istate].res;
                if(kptr->Kstates[istate].res <  min_occ_res)  min_occ_res = kptr->Kstates[istate].res;
                kptr->highest_occupied = istate;
            }
            else {
                if(istate <= kptr->max_unocc_res_index) {
                    kptr->mean_unocc_res += kptr->Kstates[istate].res;
                    mean_unocc_res += kptr->Kstates[istate].res;
                    if(kptr->Kstates[istate].res >  kptr->max_unocc_res)  kptr->max_unocc_res = kptr->Kstates[istate].res;
                    if(kptr->Kstates[istate].res <  kptr->min_unocc_res)  kptr->min_unocc_res = kptr->Kstates[istate].res;
                    if(kptr->Kstates[istate].res >  max_unocc_res)  max_unocc_res = kptr->Kstates[istate].res;
                    if(kptr->Kstates[istate].res <  min_unocc_res)  min_unocc_res = kptr->Kstates[istate].res;
                }
            }
        }
        kptr->mean_occ_res = kptr->mean_occ_res / (double)(kptr->highest_occupied + 1);
        kptr->mean_unocc_res = kptr->mean_unocc_res / (double)(kptr->max_unocc_res_index -(kptr->highest_occupied + 1));
        mean_occ_res = mean_occ_res / (double)(ct.num_kpts*(kptr->highest_occupied + 1));
        mean_unocc_res = mean_unocc_res / (double)(ct.num_kpts*kptr->max_unocc_res_index -(kptr->highest_occupied + 1));

        rmg_printf("Mean/Min/Max unoccupied wavefunction residual for kpoint %d  =  %10.5e  %10.5e  %10.5e\n", kptr->kidx, kptr->mean_unocc_res, kptr->min_unocc_res, kptr->max_unocc_res);

    }


    /* wavefunctions have changed, projectors have to be recalculated
     * but if we are using potential acceleration and not well converged yet
     * it is counterproductive to do so */
    if(!potential_acceleration || (potential_acceleration && (ct.rms <  5.0e-6))) {
        RT1 = new RmgTimer("3-MgridSubspace: Beta x psi");
        Betaxpsi (kptr, 0, kptr->nstates, kptr->newsint_local);
        delete(RT1);
    }


    RT1 = new RmgTimer("3-MgridSubspace: Diagonalization");
    Subdiag (kptr, vtot_psi, ct.subdiag_driver);
    delete(RT1);

    // wavefunctions have changed, projectors have to be recalculated */
    RT1 = new RmgTimer("3-MgridSubspace: Beta x psi");
    Betaxpsi (kptr, 0, kptr->nstates, kptr->newsint_local);
    delete(RT1);
    if((ct.ldaU_mode != LDA_PLUS_U_NONE) && (ct.num_ldaU_ions > 0))
    {
//kptr->ldaU->calc_ns_occ(kptr->orbitalsint_local, NULL);        
//kptr->ldaU->write_ldaU();
    }

    /* If sorting is requested then sort the states. */
    if (ct.sortflag) {
        kptr->sort_orbitals();
    }


}


