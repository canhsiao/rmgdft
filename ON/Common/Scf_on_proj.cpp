/************************** SVN Revision Information **************************
 **    $Id$    **
 ******************************************************************************/

/*
   scf.c
   Performs a single self consistent step.
 */

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
//#include "main.h"
#include "rmgtypedefs.h"
#include "params.h"
#include "typedefs.h"
#include "RmgTimer.h"
#include "transition.h"
#include "blas.h"
#include "RmgSumAll.h"

#include "prototypes_on.h"
#include "init_var.h"
#include "PulayMixing.h"
#include "LocalObject.h"
#include "Kbpsi.h"
#include "LdaU_on.h"
#include "GpuAlloc.h"

#define DELTA_V_MAX 1.0

void update_pot(double *, double *, double *, double *, double *, double *, double *,
        double *, double *, int *, STATE * states);


void Scf_on_proj(STATE * states, double *vxc, double *vh,
        double *vnuc, double *rho, double *rho_oppo, double *rhoc, double *rhocore,
        double * vxc_old, double * vh_old, int *CONVERGENCE)
{
    int numst = ct.num_states;
    int idx, ione = 1;
    double tem;
    int nfp0 = Rmg_G->get_P0_BASIS(Rmg_G->default_FG_RATIO);
    int pbasis= Rmg_G->get_P0_BASIS(1);
    double *rho_pre;

    RmgTimer *RT0 = new RmgTimer("init orbital");
    if(ct.scf_steps == 0)
    {
        Pulay_rho = new PulayMixing(nfp0, ct.charge_pulay_order, ct.charge_pulay_refresh, 
                ct.mix, ct.charge_pulay_scale, pct.grid_comm); 
        int tot_size = LocalOrbital->num_thispe * pbasis;
        Pulay_orbital = new PulayMixing(tot_size, ct.orbital_pulay_order, ct.orbital_pulay_refresh, 
                ct.orbital_pulay_mixfirst, ct.orbital_pulay_scale, pct.grid_comm); 
        Pulay_orbital->SetPrecond(Preconditioner);
        Pulay_orbital->SetNstates(LocalOrbital->num_thispe);
        write_data (ct.outfile, vh, vxc, vh_old, vxc_old, rho, vh_corr, states);
        MPI_Barrier(pct.img_comm);
        LocalOrbital->ReadOrbitals(std::string(ct.outfile), *Rmg_G);
    }

    rho_pre = new double[nfp0];
    double *trho = new double[nfp0];

    MPI_Barrier(pct.img_comm);
    delete RT0;

    RmgTimer *RT = new RmgTimer("2-SCF");
    RT0 = new RmgTimer("2-SCF: set zero boundary");
    
    for(int st = 0; st < LocalOrbital->num_thispe; st++)
    {
        for(int idx = 0; idx < pbasis; idx++) if (!LocalOrbital->mask[st * pbasis + idx])
        LocalOrbital->storage_proj[st * pbasis + idx] = 0.0;
    }

    delete RT0;

    RT0 = new RmgTimer("2-SCF: Kbpsi");


    LO_x_LO(*LocalProj, *LocalOrbital, Kbpsi_mat, *Rmg_G);
    delete RT0;

    RT0 = new RmgTimer("2-SCF: HS mat");
    int num_tot = LocalOrbital->num_tot;
    double *Hij_glob = new double[num_tot * num_tot];
    double *Sij_glob = new double[num_tot * num_tot];
    GetHS_proj(*LocalOrbital, *H_LocalOrbital, vtot_c, Hij_glob, Sij_glob, Kbpsi_mat);

    mat_global_to_dist(Hij, pct.desca, Hij_glob);
    mat_global_to_dist(matB, pct.desca, Sij_glob);
    delete [] Hij_glob;
    delete [] Sij_glob;

    delete RT0;

    RT0 = new RmgTimer("2-SCF: DiagScalapack");

    DiagScalapack(states, ct.num_states, Hij, matB);
    // mat_X charge density matrix in distributed way
    // uu_dis theta = (S^-1 H) in distributed way.
    double *rho_matrix_local, *theta_local;

    int num_orb = LocalOrbital->num_thispe;
    rho_matrix_local = (double *)GpuMallocManaged(num_orb * num_orb*sizeof(double));
    theta_local = (double *)GpuMallocManaged(num_orb * num_orb*sizeof(double));

    mat_dist_to_local(mat_X, pct.desca, rho_matrix_local, *LocalOrbital);
    mat_dist_to_local(uu_dis, pct.desca, theta_local, *LocalOrbital);
    delete RT0;


    if(ct.num_ldaU_ions > 0)
    {
        RT0 = new RmgTimer("2-SCF: LDA+U");
        ldaU_on->calc_ns_occ(*LocalOrbital, mat_X, *Rmg_G);
        delete RT0;

    }

    if(ct.spin_flag)
    {
        get_opposite_eigvals( states );
    }
    /* Generate new density */

    ct.efermi = Fill_on(states, ct.occ_width, ct.nel, ct.occ_mix, numst, ct.occ_flag, ct.mp_order);

    get_te(rho, rho_oppo, rhocore, rhoc, vh, vxc, states, !ct.scf_steps);
    if(pct.gridpe == 0) write_eigs(states);

    if (pct.gridpe == 0 && ct.occ_flag == 1)
        rmg_printf("FERMI ENERGY = %15.8f\n", ct.efermi * Ha_eV);

    dcopy(&nfp0, rho, &ione, rho_old, &ione);
    dcopy(&nfp0, rho, &ione, rho_pre, &ione);

    RT0 = new RmgTimer("2-SCF: GetNewRho");
    for (idx = 0; idx < nfp0; idx++)trho[idx] = rho[idx];

    if(ct.scf_steps >= ct.freeze_rho_steps)
    {
        GetNewRho_proj(*LocalOrbital, *H_LocalOrbital, rho, rho_matrix_local);
    }
    delete RT0;

    RT0 = new RmgTimer("2-SCF: Rho mixing");
    //BroydenPotential(rho_old, rho, rhoc, vh_old, vh, ct.charge_broyden_order, false);

    int iii = get_FP0_BASIS();

    double tcharge = 0.0;
    for (idx = 0; idx < get_FP0_BASIS(); idx++)
        tcharge += rho[idx];
    ct.tcharge = real_sum_all(tcharge, pct.grid_comm);
    ct.tcharge = real_sum_all(ct.tcharge, pct.spin_comm);
    ct.tcharge *= get_vel_f();

    double t2 = ct.nel / ct.tcharge;
    dscal(&iii, &t2, &rho[0], &ione);

    if(ct.spin_flag)
        get_rho_oppo(rho, rho_oppo);

    if(fabs(t2 -1.0) > 1.0e-6 && pct.gridpe == 0)
        printf("\n Warning: total charge Normalization constant = %15.12e  \n", t2-1.0);


    if(ct.charge_mixing_type == 0)
    {
        if(ct.spin_flag)
            get_rho_oppo(rho, rho_oppo);
        //     get_te(rho, rho_oppo, rhocore, rhoc, vh, vxc, states, !ct.scf_steps);
        for(int idx=0;idx < nfp0;idx++)rho[idx] = ct.mix*rho[idx] + (1.0-ct.mix)*trho[idx];
    }
    else
    {

        for (idx = 0; idx < nfp0; idx++)
        {
            tem = rho_old[idx];
            rho_old[idx] = rho[idx] - rho_old[idx];
            rho[idx] = tem;
        }

        if(ct.spin_flag)
            get_rho_oppo(rho, rho_oppo);
        //get_te(rho, rho_oppo, rhocore, rhoc, vh, vxc, states, !ct.scf_steps);
        if(ct.scf_steps >=ct.freeze_orbital_step)
        {
            if(ct.charge_pulay_order ==1 )  ct.charge_pulay_order++;
            Pulay_rho->Refresh();
        }

        if(ct.scf_steps >= ct.freeze_rho_steps)
            Pulay_rho->Mixing(rho, rho_old);

    }

    delete RT0;

    if(ct.spin_flag) get_rho_oppo(rho, rho_oppo);

    /* Update potential */
    RmgTimer *RT4 = new RmgTimer("2-SCF: update_pot");
    if(ct.scf_steps >= ct.freeze_rho_steps)
        UpdatePot(vxc, vh, vxc_old, vh_old, vnuc, rho, rho_oppo, rhoc, rhocore);
    delete(RT4);

    CheckConvergence(vxc, vh, vxc_old, vh_old, rho, rho_pre, CONVERGENCE);

    /* Update the orbitals */
    if(ct.scf_steps < ct.freeze_orbital_step)
    {
        if(ct.scf_steps == ct.freeze_rho_steps ) 
            ct.restart_mix = 1;
        else
            ct.restart_mix = 0;

        RT0 = new RmgTimer("2-SCF: Residual calculation");
        CalculateResidual(*LocalOrbital, *H_LocalOrbital, *LocalProj,  vtot_c, theta_local, Kbpsi_mat);
        delete RT0;

        for(int st = 0; st < LocalOrbital->num_thispe; st++)
        {
            for(int idx = 0; idx < pbasis; idx++) if (!LocalOrbital->mask[st * pbasis + idx])
                H_LocalOrbital->storage_proj[st * pbasis + idx] = 0.0;
        }
        RT0 = new RmgTimer("2-SCF: orbital precond and mixing");

        Pulay_orbital->Mixing(LocalOrbital->storage_proj, H_LocalOrbital->storage_proj);
        delete RT0;

    }
    delete(RT);

    delete [] trho;
    delete [] rho_pre;
    GpuFreeManaged(rho_matrix_local);
    GpuFreeManaged(theta_local);

}                               /* end scf */



