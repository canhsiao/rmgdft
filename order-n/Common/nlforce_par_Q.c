/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "md.h"

void nlforce_par_Q(REAL * veff, REAL * rho_nm, int ion, int nh, double *forces)
{
    int idx1, idx2, n, m, count, icount, size;
    int *pidx, max_nl;
    REAL tmp[3], gamma_nm;
    REAL *QnmI_R, *QnmI_x, *QnmI_y, *QnmI_z, *gamma;
    ION *iptr;

    REAL time1, time2;
    time1 = my_crtc();

    gamma = rho_nm + ion * ct.max_nl * ct.max_nl;

    /*Forces array is assumed to be already initialized */
    for (idx1 = 0; idx1 < 3; idx1++)
        forces[idx1] = 0.0;

    count = pct.Qidxptrlen[ion];
    pidx = pct.Qindex[ion];
    iptr = &ct.ions[ion];

    max_nl = ct.max_nl;

    if (count)
    {
        size = (nh * (nh + 1) / 2) * count;
        my_malloc_init( QnmI_R, 3 * size, REAL );
        QnmI_x = QnmI_R;
        QnmI_y = QnmI_x + size;
        QnmI_z = QnmI_y + size;

        for (idx1 = 0; idx1 < 3 * size; idx1++)
            QnmI_R[idx1] = 0.0;

        partial_QI(ion, QnmI_R, iptr);

        for (icount = 0; icount < count; icount++)
        {
            tmp[0] = 0.0;
            tmp[1] = 0.0;
            tmp[2] = 0.0;

            idx2 = 0;
            for (n = 0; n < nh; n++)
            {
                for (m = n; m < nh; m++)
                {
                    idx1 = idx2 * count + icount;
                    if (m == n)
                    {
                        gamma_nm = gamma[n * max_nl + m];
                    }
                    else
                    {
                        gamma_nm = gamma[n * max_nl + m] + gamma[m * max_nl + n];
                    }
                    tmp[0] += QnmI_x[idx1] * gamma_nm;
                    tmp[1] += QnmI_y[idx1] * gamma_nm;
                    tmp[2] += QnmI_z[idx1] * gamma_nm;

                    ++idx2;
                }
            }
            forces[0] += veff[pidx[icount]] * tmp[0];
            forces[1] += veff[pidx[icount]] * tmp[1];
            forces[2] += veff[pidx[icount]] * tmp[2];
        }

        my_free(QnmI_R);
    }

/*    size = 3;
 *   global_sums(forces, &size);
 *   iptr->force[ct.fpt[0]][0] += ct.vel_f * forces[0];
 *   iptr->force[ct.fpt[0]][1] += ct.vel_f * forces[1];
 *   iptr->force[ct.fpt[0]][2] += ct.vel_f * forces[2];
*/

    time2 = my_crtc();
    rmg_timings(NLFORCE_PAR_Q, time2 - time1);

}
