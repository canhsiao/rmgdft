#ifndef RMG_Mgrid_H
#define RMG_Mgrid_H 1

#include "Lattice.h"
#include "TradeImages.h"
#include "const.h"
#include "rmg_error.h"

class Mgrid {

private:
    Lattice *L;
    TradeImages *T;
    int level_flag;

public:
    Mgrid(Lattice *lptr, TradeImages *tptr);
   ~Mgrid(void);

    template <typename RmgType> void mg_restrict (RmgType * full, RmgType * half, int dimx, int dimy, int dimz, int dx2, int dy2, int dz2, int xoffset, int yoffset, int zoffset);

    template <typename RmgType> void mg_prolong (RmgType * full, RmgType * half, int dimx, int dimy, int dimz, int dx2, int dy2, int dz2, int xoffset, int yoffset, int zoffset);

    template <typename RmgType> void eval_residual (RmgType * mat, RmgType * f_mat, int dimx, int dimy, int dimz, double gridhx, double gridhy, double gridhz, RmgType * res);

    template <typename RmgType> void solv_pois (RmgType * vmat, RmgType * fmat, RmgType * work,
                int dimx, int dimy, int dimz, double gridhx, double gridhy, double gridhz, double step, double k);

    int MG_SIZE (int curdim, int curlevel, int global_dim, int global_offset, int global_pdim, int *roffset, int bctype);

    template <typename RmgType> void mgrid_solv (RmgType * v_mat, RmgType * f_mat, RmgType * work,
                 int dimx, int dimy, int dimz,
                 double gridhx, double gridhy, double gridhz,
                 int level, int *nb_ids, int max_levels, int *pre_cyc,
                 int *post_cyc, int mu_cyc, double step, double k,
                 int gxsize, int gysize, int gzsize,
                 int gxoffset, int gyoffset, int gzoffset,
                 int pxdim, int pydim, int pzdim, int boundary_flag);

};

#endif

