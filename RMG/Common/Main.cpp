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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h> 
#include <unistd.h>
#include <unordered_map>
#include <csignal>

#include "const.h"
#include "RmgTimer.h"
#include "RmgException.h"
#include "rmgtypedefs.h"
#include "params.h"
#include "typedefs.h"
#include "rmg_error.h"
#include "transition.h"
#include "Kpoint.h"
#include "InputKey.h"
#include "blas.h"
#include "RmgThread.h"
#include "rmgthreads.h"


#include "../Headers/common_prototypes.h"
#include "../Headers/common_prototypes1.h"
#include "prototypes_tddft.h"
#include "Exxbase.h"
#include "Neb.h"


void initialize (int argc, char **argv);

template <typename OrbitalType> void run (Kpoint<OrbitalType> **Kptr);

void report (void);

void finish (void);

std::vector<ION> Atoms;
std::vector<SPECIES> Species;


/* Electronic charge density or charge density of own spin in polarized case */
double *rho;

/*  Electronic charge density of pposite spin density*/
double *rho_oppo;  


/* Core Charge density */
double *rhocore;


/* Compensating charge density */
double *rhoc;


/* Hartree potential */
double *vh;

/* Nuclear local potential */
double *vnuc;

/* Exchange-correlation potential */
double *vxc;

// Pointer to Kpoint class arrays for gamma and non-gamma
Kpoint<double> **Kptr_g;
Kpoint<std::complex<double> > **Kptr_c;

double *tau;
/* Main control structure which is declared extern in main.h so any module */
/* may access it.					                 */
CONTROL ct;

/* PE control structure which is also declared extern in main.h */
PE_CONTROL pct;

std::unordered_map<std::string, InputKey *> ControlMap;

std::atomic<bool> shutdown_request(false);

extern "C" void term_handler(int signal)
{
    shutdown_request.store(true); 
}

void CheckShutdown(void)
{
    if(shutdown_request.load())
    {
        DeleteNvmeArrays();
        for (int kpt = 0; kpt < ct.num_kpts_pe; kpt++)
        {

            int kpt1 = kpt + pct.kstart;
            if(ct.is_gamma)
            {
                Kptr_g[kpt1]->DeleteNvmeArrays();
            }
            else
            {
                Kptr_c[kpt1]->DeleteNvmeArrays();
            }
        }
        MPI_Abort( MPI_COMM_WORLD, 0 );
        kill(getpid(), SIGKILL);
    }
}

int main (int argc, char **argv)
{

    // Set branch type and save argc and argv in control structure
    ct.rmg_branch = RMG_BASE;
    ct.save_args(argc, argv);

    // Signal handlers to cleanup in case user terminates
    std::signal(SIGTERM, term_handler);
    std::signal(SIGINT, term_handler);

    RmgTimer *RT = new RmgTimer("1-TOTAL");
    char *tptr;

    /* Define a default output stream, gets redefined to log file later */

    ct.logfile = stdout;

// for RMG, the projectors |beta> are multiplied by exp(-ik.r) for non-gamma point
// for ON and NEGF, the phase exp(-ik.r) is in the matrix separtion.
    ct.proj_nophase = 0; 


    // Get RMG_MPI_THREAD_LEVEL environment variable
    ct.mpi_threadlevel = MPI_THREAD_SERIALIZED;
    if(NULL != (tptr = getenv("RMG_MPI_THREAD_LEVEL"))) {
        ct.mpi_threadlevel = atoi(tptr);
    }

    try {

        RmgTimer *RT1 =  new RmgTimer("1-TOTAL: Init");
        initialize (argc, argv);
        delete(RT1);



        RmgTimer *RT2 = new RmgTimer("1-TOTAL: run");
        if(ct.is_gamma)
            run<double> ((Kpoint<double> **)Kptr_g);
        else
            run<std::complex<double> >((Kpoint<std::complex<double>> **)Kptr_c);
        delete(RT2);

    }

    // Catch exceptions issued by us.
    catch(RmgFatalException const &e) {
        std::cout << e.rwhat() << std::endl;
        finish ();
        exit(0);
    }

    // By std
    catch (std::exception &e) {
        std::cout << "Caught a std exception: " << e.what () << std::endl;
        finish ();
        exit(0);
    } 

    // Catchall
    catch (...) {
        std::cout << "Caught an unknown exception of some type." << std::endl;
        finish ();
        exit(0);
    } 

    delete(RT);   // Destructor has to run before report
    report ();

    finish ();

    // Shutdown threads gracefully otherwise Cray perftools has issues
    RmgTerminateThreads();

}


void initialize(int argc, char **argv) 
{

    int FP0_BASIS;

    /* start the benchmark clock */
    ct.time0 = my_crtc ();
    RmgTimer *RT0 = new RmgTimer("2-Init");
    RmgTimer *RT = new RmgTimer("2-Init: KpointClass");

    /* Initialize all I/O including MPI group comms */
    /* Also reads control and pseudopotential files*/
    InitIo (argc, argv, ControlMap);

    FP0_BASIS = Rmg_G->get_P0_BASIS(Rmg_G->default_FG_RATIO);

    int num_images = pct.images;
    num_images = 1;
    lbfgs_init(Atoms.size(), num_images);

    int spinfac = 1;
    if(ct.spin_flag) spinfac = 2;
    rho = new double[spinfac * FP0_BASIS]();
    rhocore = new double[FP0_BASIS];
    rhoc = new double[FP0_BASIS];
    vh = new double[FP0_BASIS];
    vnuc = new double[FP0_BASIS];
    vxc = new double[spinfac * FP0_BASIS];
    if (ct.xctype == MGGA_TB09) 
    	tau = new double[FP0_BASIS];

    /* for spin polarized calculation set pointer to memory for density of the opposite spin */
    rho_oppo = rho + FP0_BASIS;

    /* Check if Bweight is needed */
    ct.need_Bweight = true;
//    if((ct.discretization == CENTRAL_DISCRETIZATION) && ct.norm_conserving_pp) ct.need_Bweight = false;
    if(ct.discretization == CENTRAL_DISCRETIZATION) ct.need_Bweight = false;

    /* Initialize some k-point stuff */
    Kptr_g = new Kpoint<double> * [ct.num_kpts_pe];
    Kptr_c = new Kpoint<std::complex<double> > * [ct.num_kpts_pe];

    ct.is_gamma = true;
    for (int kpt = 0; kpt < ct.num_kpts; kpt++) {
        double v1, v2, v3;
        v1 = twoPI * ct.kp[kpt].kpt[0] / Rmg_L.get_xside();
        v2 = twoPI * ct.kp[kpt].kpt[1] / Rmg_L.get_yside();
        v3 = twoPI * ct.kp[kpt].kpt[2] / Rmg_L.get_zside();

        ct.kp[kpt].kvec[0] = v1;
        ct.kp[kpt].kvec[1] = v2;
        ct.kp[kpt].kvec[2] = v3;
        ct.kp[kpt].kmag = v1 * v1 + v2 * v2 + v3 * v3;

        if(ct.kp[kpt].kmag != 0.0) ct.is_gamma = false;
    }

    if(ct.is_gamma) 
    {
        ct.is_use_symmetry = 0;
    }

    for (int kpt = 0; kpt < ct.num_kpts_pe; kpt++)
    {

        int kpt1 = kpt + pct.kstart;
        if(ct.is_gamma) {

            // Gamma point
            Kptr_g[kpt] = new Kpoint<double> (ct.kp[kpt1], kpt, pct.grid_comm, Rmg_G, Rmg_T, &Rmg_L, ControlMap);

        }
        else {

            // General case
            Kptr_c[kpt] = new Kpoint<std::complex<double>> (ct.kp[kpt1], kpt, pct.grid_comm, Rmg_G, Rmg_T, &Rmg_L, ControlMap);

        }
        ct.kp[kpt].kidx = kpt;
    }


    MPI_Barrier (pct.img_comm);

    /* Record the time it took from the start of run until we hit init */
    delete(RT);

    /* Perform any necessary initializations */
    if(ct.is_gamma) {
        Init (vh, rho, rho_oppo, rhocore, rhoc, vnuc, vxc, Kptr_g);
    }
    else {
        Init (vh, rho, rho_oppo, rhocore, rhoc, vnuc, vxc, Kptr_c);
    }


    /* Flush the results immediately */
    fflush (NULL);


    /* Wait until everybody gets here */
    /* MPI_Barrier(MPI_COMM_WORLD); */
    MPI_Barrier(pct.img_comm);

    delete(RT0);

}

template <typename OrbitalType> void run (Kpoint<OrbitalType> **Kptr)
{


    /* Dispatch to the correct driver routine */
    switch (ct.forceflag)
    {

        case MD_QUENCH:            /* Quench the electrons */
            if (ct.xctype == MGGA_TB09)
                //relax_tau (0, states, vxc, vh, vnuc, rho, rho_oppo, rhocore, rhoc, tau);
                ;
            else 
            Relax (0, vxc, vh, vnuc, rho, rho_oppo, rhocore, rhoc, Kptr);
            break;

        case MD_FASTRLX:           /* Fast relax */
            Relax (ct.max_md_steps, vxc, vh, vnuc, rho, rho_oppo, rhocore, rhoc, Kptr);
            break;

        case NEB_RELAX:           /* nudged elastic band relax */
            {
                Neb<OrbitalType> NEB (*Rmg_G, pct.images,ct.max_neb_steps, ct.input_initial, ct.input_final, ct.totale_initial, ct.totale_final); 
                NEB.relax(vxc, vh, vnuc, rho, rho_oppo, rhocore, rhoc, Kptr);
            }
            break;

        case MD_CVE:               /* molecular dynamics */
        case MD_CVT:
        case MD_CPT:
            ct.fpt[0] = 0;  // Eventually fix all references to fpt in the code and this will not be needed
            ct.fpt[1] = 1;
            ct.fpt[2] = 2;
            ct.fpt[3] = 3;
            MolecularDynamics (Kptr, vxc, vh, vnuc, rho, rho_oppo, rhoc, rhocore);
            break;

        case BAND_STRUCTURE:
            BandStructure (Kptr, vxc, vh, vnuc);
            if(ct.rmg2bgw) WriteBGW_Rhog(rho, rho_oppo);
            OutputBandPlot(Kptr);
            return;

        case TDDFT:
            if(!ct.restart_tddft) Relax (0, vxc, vh, vnuc, rho, rho_oppo, rhocore, rhoc, Kptr);
            RmgTddft (vxc, vh, vnuc, rho, rho_oppo, rhocore, rhoc, Kptr);
            break;
        
        case Exx_only:
            {
                std::vector<double> occs;
                occs.resize(Kptr[0]->nstates);
                for(int i=0;i < Kptr[0]->nstates;i++) occs[i] = Kptr[0]->Kstates[i].occupation[0];
                Exxbase<OrbitalType> Exx(*Kptr[0]->G, *Kptr[0]->L, "tempwave", Kptr[0]->nstates, occs.data(), 
                        Kptr[0]->orbital_storage, ct.exx_mode);
                if(ct.exx_mode == EXX_DIST_FFT)
                    Exx.ReadWfsFromSingleFile();
                Exx.Vexx_integrals(ct.exx_int_file);
                break;
            }

        default:
            rmg_error_handler (__FILE__, __LINE__, "Undefined MD method");


    }


    if(Verify ("output_rho_xsf", true, Kptr[0]->ControlMap))
        Output_rho_xsf(rho, pct.grid_comm);

}                               /* end run */

void report ()
{

    /* write planar averages of quantities */
    if (ct.zaverage == 1)
    {
        /* output the average potential */
        write_avgv (vh, vnuc);
        write_avgd (rho);
    }
    else if (ct.zaverage == 2)
    {
        //write_zstates (states);
        ;
    }


    /* If milliken population info is requested then compute and output it */
    /*if (ct.domilliken)
      mulliken (states);*/


    if (ct.xctype == MGGA_TB09) 
        delete [] tau;

    /* Write timing information */
    if(pct.imgpe == 0) fclose(ct.logfile);
    int override_rank = 0;
    if(pct.imgpe==0) MPI_Comm_rank (pct.img_comm, &override_rank);
    int num_owned_ions;
    if(ct.is_gamma)
    {
        num_owned_ions = Kptr_g[0]->BetaProjector->get_num_owned_ions();
    }
    else
    {
        num_owned_ions = Kptr_c[0]->BetaProjector->get_num_owned_ions();
    }
    RmgPrintTimings(pct.img_comm, ct.logname, ct.scf_steps, num_owned_ions * ct.num_kpts_pe, override_rank);


}                               /* end report */


void finish ()
{

    DeleteNvmeArrays();
    MPI_Barrier(MPI_COMM_WORLD);
    for (int kpt = 0; kpt < ct.num_kpts_pe; kpt++)
    {

        if(ct.is_gamma)
        {
            Kptr_g[kpt]->DeleteNvmeArrays();
        }
        else
        {
            Kptr_c[kpt]->DeleteNvmeArrays();
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    /*Exit MPI */
    MPI_Finalize ();

#if GPU_ENABLED
    //cublasDestroy(ct.cublas_handle);
    //cudaDeviceReset();
#endif

}                               /* end finish */

