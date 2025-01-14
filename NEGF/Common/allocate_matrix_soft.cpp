#include "negf_prototypes.h"
/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 
/****f* QMD-MGDFT/allocate_matrix_soft.c *****
 * NAME
 *   Ab initio O(n) real space code with 
 *   localized orbitals and multigrid acceleration
 *   Version: 3.0.0
 * COPYRIGHT
 *   Copyright (C) 2001  Wenchang Lu,
 *                       Jerzy Bernholc
 * FUNCTION
 *   void allocate_matrix()   
 *   allocate memory for matrixs
 * INPUTS
 *   nothing
 * OUTPUT
 *   nothing
 * PARENTS
 *   run.c
 * CHILDREN
 *   
 * SOURCE
 */



#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "main.h"
#include "init_var.h"
#include "LCR.h"
#include "init_var.h"
#include "blacs.h"


void allocate_matrix_soft ()
{
    int ispin, sizeofmatrix, item, item1, item2, lwork;
    int ictxt, nproc, myrow, mycol, icrow, iccol;
    int izero = 0, ione = 1, itwo = 2, nb = ct.scalapack_block_factor, nn = ct.num_states;
    int nprow = pct.scalapack_nprow, npcol = pct.scalapack_npcol, npes = pct.grid_npes;
    int locr, qrmem, sizemqrleft, ldc, mpc0, nqc0, nrc;
    

    int sbasis;

    int NB = ct.scalapack_block_factor;
    ispin = ct.spin_flag + 1;

    sbasis = (get_PX0_GRID() +2) * (get_PY0_GRID() +2) * (get_PZ0_GRID() +2);
    my_malloc_init( peaks, 100, double );

    my_malloc_init( rho, get_FP0_BASIS() * ispin, double );
    my_malloc_init( rhoc, get_FP0_BASIS(), double );
    my_malloc_init( vh, get_FP0_BASIS(), double );
    my_malloc_init( vnuc, get_FP0_BASIS(), double );
    my_malloc_init( vext, get_FP0_BASIS(), double );
    my_malloc_init( vcomp, get_FP0_BASIS(), double );
    my_malloc_init( vxc, get_FP0_BASIS() * ispin, double );
    my_malloc_init( vtot, get_FP0_BASIS(), double );
    my_malloc_init( vtot_c, get_P0_BASIS(), double );
    my_malloc_init( rhocore, get_FP0_BASIS(), double );
    my_malloc_init( vtot_global, get_NX_GRID() * get_NY_GRID() * get_NZ_GRID(), double );
    rho_global = vtot_global;
/*  my_malloc_init( nlarray1, get_P0_BASIS()/MAX_FUNC_PE, double );*/
    
    if (ct.num_tfions > 0)
    {
	my_malloc_init( rho_tf, get_FP0_BASIS(), double );
    }

    my_malloc_init( rho_old, get_FP0_BASIS() * ispin, double );

    my_malloc_init( sg_res, sbasis, double );
    sizeofmatrix = ct.num_states * ct.num_states;

#if !GAMMA_PT
    sizeofmatrix *= 2.0;
#endif


    /*  allocate memory for other uses  */

    item = ct.num_kpts * ct.num_states;
    item1 = rmg_max (ct.num_states * ct.num_states, item);
    item2 = 2 * get_P0_BASIS() + sbasis + item1;
    item = rmg_max (13 * sbasis, item2);


    nproc = pct.scalapack_nprow * pct.scalapack_npcol;
    locr = ((ct.num_states / ct.scalapack_block_factor + 1) / nproc + 1) * NB + NB;
    lwork = 10 * (locr * 5 + ct.scalapack_block_factor);
    item1 = rmg_max (lwork, item);

    sl_init_on (&ictxt, pct.scalapack_nprow, pct.scalapack_npcol);
    Cblacs_gridinfo (ictxt, &nprow, &npcol, &myrow, &mycol);
    if (myrow != -1)
        Cblacs_gridexit (ictxt);

    icrow = indxg2p (&itwo, &nb, &myrow, &izero, &nprow);
    iccol = indxg2p (&ione, &nb, &mycol, &izero, &npcol);
    mpc0 = numroc (&nn, &nb, &myrow, &icrow, &nprow);
    nqc0 = numroc (&nn, &nb, &mycol, &iccol, &npcol);
    nrc = numroc (&nn, &nb, &myrow, &izero, &npes);
    ldc = rmg_max (1, nrc);
    sizemqrleft = rmg_max ((ct.scalapack_block_factor * (NB - 1)) / 2, (nqc0 + mpc0) * NB) + NB * NB;
    sizemqrleft *= 2;
    qrmem = 2 * ct.num_states - 2;
    lwork = 5 * ct.num_states + ct.num_states * ldc + rmg_max (sizemqrleft, qrmem) + 1;
    item = rmg_max (lwork, item1);



}                               /* end allocate_matrix */

/********/
