/************************** SVN Revision Information **************************
 **    $Id: lcao_get_awave.c 2147 2014-02-26 18:46:25Z ebriggs $    **
******************************************************************************/

#include <complex>
#include "const.h"
#include "params.h"
#include "rmgtypedefs.h"
#include "typedefs.h"
#include "rmg_error.h"
#include "State.h"
#include "Kpoint.h"
#include "transition.h"

/*This function calculates atomic wavefunctions using wavefunctions read from PP files
 * with angular part added. The result is in psi, which is assumed to be initialized to zero*/

template void LcaoGetAwave(double *, ION *, int, int, int, double);
template void LcaoGetAwave(std::complex<double>  *, ION *, int, int, int, double);

template <typename StateType>
void LcaoGetAwave (StateType *psi, ION *iptr, int awave_idx, int l, int m, double coeff)
{

    int ix, iy, iz;
    int yindex;
    int ilow, jlow, klow, ihi, jhi, khi, map;
    int icount;
    int PX0_GRID, PY0_GRID, PZ0_GRID;
    int PX_OFFSET, PY_OFFSET, PZ_OFFSET;

    double r, xc, yc, zc, vector[3];
    double x[3], invdr, xstart, ystart, zstart;;
    double hxgrid, hygrid, hzgrid;
    SPECIES *sp;

    PX0_GRID = get_PX0_GRID();
    PY0_GRID = get_PY0_GRID();
    PZ0_GRID = get_PZ0_GRID();
    PX_OFFSET = get_PX_OFFSET();
    PY_OFFSET = get_PY_OFFSET();
    PZ_OFFSET = get_PZ_OFFSET();
    hxgrid = get_hxgrid();
    hygrid = get_hygrid();
    hzgrid = get_hzgrid();


    /* Grab some memory for temporary storage */
    int *pvec = new int[get_P0_BASIS()];
    int *Aix = new int[get_P0_BASIS()];
    int *Aiy = new int[get_P0_BASIS()];
    int *Aiz = new int[get_P0_BASIS()];


    /* Get species type */
    sp = &ct.sp[iptr->species];


    /* Determine mapping indices or even if a mapping exists */
    map = get_index (pct.gridpe, iptr, Aix, Aiy, Aiz, &ilow, &ihi, &jlow, &jhi, &klow, &khi,
	    sp->adim_wave, PX0_GRID, PY0_GRID, PZ0_GRID,
	    get_NX_GRID(), get_NY_GRID(), get_NZ_GRID(),
	    &xstart, &ystart, &zstart);


    /* If there is any overlap then we have to generate the mapping */
    if (map)
    {
	
	/*Starting index for ylm function: Indexing is as follows: 0:s, 1:px, 2:py, 3:pz, 4:dxx, etc.*/
	yindex = l*l + m;
	
	invdr = 1.0 / sp->drlig_awave;
	icount = 0;

	xc = xstart;
	for (ix = 0; ix < sp->adim_wave; ix++)
	{
	    yc = ystart;
	    for (iy = 0; iy < sp->adim_wave; iy++)
	    {
		zc = zstart;
		for (iz = 0; iz < sp->adim_wave; iz++)
		{
		    if (((Aix[ix] >= ilow) && (Aix[ix] <= ihi)) &&
			    ((Aiy[iy] >= jlow) && (Aiy[iy] <= jhi)) &&
			    ((Aiz[iz] >= klow) && (Aiz[iz] <= khi)))
		    {
                        pvec[icount] =
                            PY0_GRID * PZ0_GRID * ((Aix[ix]-PX_OFFSET) % PX0_GRID) +
                            PZ0_GRID * ((Aiy[iy]-PY_OFFSET) % PY0_GRID) +
                            ((Aiz[iz]-PZ_OFFSET) % PZ0_GRID);



			x[0] = xc - iptr->xtal[0];
			x[1] = yc - iptr->xtal[1];
			x[2] = zc - iptr->xtal[2];
			r = metric (x);

			to_cartesian(x, vector);

			if (r <= sp->aradius)
			    psi[pvec[icount]] = psi[pvec[icount]] + coeff * linint (&sp->awave_lig[awave_idx][0], r, invdr) * ylm(yindex, vector);
			    

			icount++;
		    }

		    zc += hzgrid;

		}           /* end for */

		yc += hygrid;

	    }               /* end for */

	    xc += hxgrid;

	}                   /* end for */

    }                       /* end if */


    /* Release our memory */
    delete [] Aiz;
    delete [] Aiy;
    delete [] Aix;
    delete [] pvec;

}
