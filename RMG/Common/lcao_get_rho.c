/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "grid.h"
#include "common_prototypes.h"
#include "main.h"


void lcao_get_rho (rmg_double_t * arho_f)
{

    int ix, iy, iz;
    int ion, idx;
    int ilow, jlow, klow, ihi, jhi, khi, map;
    int *Aix, *Aiy, *Aiz;
    int icount, n, incx;
    int *pvec;
    int FPX0_GRID, FPY0_GRID, FPZ0_GRID;
    int FPX_OFFSET, FPY_OFFSET, FPZ_OFFSET;

    rmg_double_t r, xc, yc, zc;;
    rmg_double_t x[3], invdr, t1, t2, xstart, ystart, zstart;;
    SPECIES *sp;
    ION *iptr;

    FPX0_GRID = get_FPX0_GRID();
    FPY0_GRID = get_FPY0_GRID();
    FPZ0_GRID = get_FPZ0_GRID();
    FPX_OFFSET = get_FPX_OFFSET();
    FPY_OFFSET = get_FPY_OFFSET();
    FPZ_OFFSET = get_FPZ_OFFSET();

    /* Grab some memory for temporary storage */
    my_malloc (pvec, get_FP0_BASIS(), int);
    my_malloc (Aix, get_FNX_GRID(), int);
    my_malloc (Aiy, get_FNY_GRID(), int);
    my_malloc (Aiz, get_FNZ_GRID(), int);


    /* Initialize the compensating charge array and the core charge array */
    for (idx = 0; idx < get_FP0_BASIS(); idx++)
        arho_f[idx] = 0.0;


    /* Loop over ions */
    for (ion = 0; ion < ct.num_ions; ion++)
    {
        /* Generate ion pointer */
        iptr = &ct.ions[ion];

        /* Get species type */
        sp = &ct.sp[iptr->species];

        /* Determine mapping indices or even if a mapping exists */
        map = get_index (pct.gridpe, iptr, Aix, Aiy, Aiz, &ilow, &ihi, &jlow, &jhi, &klow, &khi,
                         sp->adim_rho, FPX0_GRID, FPY0_GRID, FPZ0_GRID,
                         ct.psi_fnxgrid, ct.psi_fnygrid, ct.psi_fnzgrid,
                         &xstart, &ystart, &zstart);



        /* If there is any overlap then we have to generate the mapping */
        if (map)
        {
            invdr = 1.0 / sp->drlig_arho;
            icount = 0;

            xc = xstart;
            for (ix = 0; ix < sp->adim_rho; ix++)
            {
                yc = ystart;
                for (iy = 0; iy < sp->adim_rho; iy++)
                {
                    zc = zstart;
                    for (iz = 0; iz < sp->adim_rho; iz++)
                    {
                        if (((Aix[ix] >= ilow) && (Aix[ix] <= ihi)) &&
                            ((Aiy[iy] >= jlow) && (Aiy[iy] <= jhi)) &&
                            ((Aiz[iz] >= klow) && (Aiz[iz] <= khi)))
                        {
                            pvec[icount] =
                                FPY0_GRID * FPZ0_GRID * ((Aix[ix]-FPX_OFFSET) % FPX0_GRID) +
                                FPZ0_GRID * ((Aiy[iy]-FPY_OFFSET) % FPY0_GRID) +
                                ((Aiz[iz]-FPZ_OFFSET) % FPZ0_GRID);

                            x[0] = xc - iptr->xtal[0];
                            x[1] = yc - iptr->xtal[1];
                            x[2] = zc - iptr->xtal[2];
                            r = metric (x);

			    if (r <= sp->aradius)
				arho_f[pvec[icount]] += linint (&sp->arho_lig[0], r, invdr);

                            icount++;
                        }

                        zc += ct.hzzgrid;

                    }           /* end for */

                    yc += ct.hyygrid;

                }               /* end for */

                xc += ct.hxxgrid;

            }                   /* end for */

        }                       /* end if */

    }                           /* end for */
    
    /* Check total charge. */
    t2 = 0.0;
    
    for (idx = 0; idx < get_FP0_BASIS(); idx++)
	t2 += arho_f[idx];

    t2 = ct.vel_f *  real_sum_all (t2, pct.img_comm);
    
    t1 = ct.nel / t2;
    if (pct.imgpe == 0)
        printf ("\n get_arho: Aggregate initial atomic charge is %f, normalization constant is %f", t2, t1);
    

    
    n = get_FP0_BASIS();
    incx = 1;
    QMD_dscal (n, t1, arho_f, incx);



    /* Release our memory */
    my_free(Aiz);
    my_free(Aiy);
    my_free(Aix);
    my_free (pvec);

}                               /* end init_nuc */

/******/
