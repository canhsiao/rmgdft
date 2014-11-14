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

    int n, incx;
    int ix, iy, iz, ixx, iyy, izz;
    int xstart, ystart, zstart, xend, yend, zend;
    int ion, idx;
    int ilow, jlow, klow, ihi, jhi, khi;
    int dimx, dimy, dimz;
    int FP0_BASIS;
    int FPX0_GRID, FPY0_GRID, FPZ0_GRID;
    int FPX_OFFSET, FPY_OFFSET, FPZ_OFFSET;
    int FNX_GRID, FNY_GRID, FNZ_GRID;

    rmg_double_t r, Zv, rc, rc2, rcnorm, t1, t2;
    rmg_double_t x[3];
    rmg_double_t hxxgrid, hyygrid, hzzgrid;
    double xside, yside, zside;
    SPECIES *sp;
    ION *iptr;
    int i_r;
    double r1,r2, coef1, coef2, a,b,c;

    hxxgrid = get_hxxgrid();
    hyygrid = get_hyygrid();
    hzzgrid = get_hzzgrid();
    xside = get_xside();
    yside = get_yside();
    zside = get_zside();

    FP0_BASIS = get_FP0_BASIS();
    FPX0_GRID = get_FPX0_GRID();
    FPY0_GRID = get_FPY0_GRID();
    FPZ0_GRID = get_FPZ0_GRID();
    FPX_OFFSET = get_FPX_OFFSET();
    FPY_OFFSET = get_FPY_OFFSET();
    FPZ_OFFSET = get_FPZ_OFFSET();
    FNX_GRID = get_FNX_GRID();
    FNY_GRID = get_FNY_GRID();
    FNZ_GRID = get_FNZ_GRID();


    ilow = FPX_OFFSET;
    jlow = FPY_OFFSET;
    klow = FPZ_OFFSET;
    ihi = ilow + FPX0_GRID;
    jhi = jlow + FPY0_GRID;
    khi = klow + FPZ0_GRID;



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

        b = log((sp->r[2] - sp->r[1])/(sp->r[1] - sp->r[0]));
        c = (sp->r[0] * exp(b) - sp->r[1])/(1.0 -exp(b) );
        a = sp->r[0] + c;

        dimx =  sp->aradius/(hxxgrid*xside);
        dimy =  sp->aradius/(hyygrid*yside);
        dimz =  sp->aradius/(hzzgrid*zside);

        dimx = dimx * 2 + 1;
        dimy = dimy * 2 + 1;
        dimz = dimz * 2 + 1;


        xstart = iptr->xtal[0] / hxxgrid - dimx/2;
        xend = xstart + dimx;
        ystart = iptr->xtal[1] / hyygrid - dimy/2;
        yend = ystart + dimy;
        zstart = iptr->xtal[2] / hzzgrid - dimz/2;
        zend = zstart + dimz;


        for (ix = xstart; ix < xend; ix++)
        {
            // fold the grid into the unit cell
            ixx = (ix + 20 * FNX_GRID) % FNX_GRID;
            if(ixx >= ilow && ixx < ihi)
            {

                for (iy = ystart; iy < yend; iy++)
                {
                    // fold the grid into the unit cell
                    iyy = (iy + 20 * FNY_GRID) % FNY_GRID;
                    if(iyy >= jlow && iyy < jhi)
                    {
                        for (iz = zstart; iz < zend; iz++)
                        {
                            // fold the grid into the unit cell
                            izz = (iz + 20 * FNZ_GRID) % FNZ_GRID;
                            if(izz >= klow && izz < khi)
                            {

                                idx = (ixx-ilow) * FPY0_GRID * FPZ0_GRID + (iyy-jlow) * FPZ0_GRID + izz-klow;
                                x[0] = ix * hxxgrid - iptr->xtal[0];
                                x[1] = iy * hyygrid - iptr->xtal[1];
                                x[2] = iz * hzzgrid - iptr->xtal[2];
                                r = metric (x);
                                if(r <= sp->r[0])
                                     i_r= 0;
                                else
                                    i_r = (int)(log ( (r+c)/a) /b);
                                r1 = a *exp (i_r * b) -c;
                                r2 = a * exp((i_r+1) *b) -c;

                                coef1 = (r2-r)/(r2-r1);
                                coef2 = (r-r1)/(r2-r1);

                                arho_f[idx] += coef1 * sp->atomic_rho[i_r] 
                                    + coef2 * sp->atomic_rho[i_r+1];



                            }                       /* end if */
                        }                           /* end for */
                    }
                }
            }
        }
    }

    /* Check total charge. */
    t2 = 0.0;

    for (idx = 0; idx < get_FP0_BASIS(); idx++)
        t2 += arho_f[idx];

    t2 = get_vel_f() *  real_sum_all (t2, pct.img_comm);

    t1 = ct.nel / t2;
    if (pct.imgpe == 0)
        printf ("\n get_arho: Aggregate initial atomic charge is %f, normalization constant is %f", t2, t1);



    n = get_FP0_BASIS();
    incx = 1;
    QMD_dscal (n, t1, arho_f, incx);

}
