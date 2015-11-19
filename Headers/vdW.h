#ifndef RMG_vdW_H
#define RMG_vdW_H 1

#include "BaseGrid.h"
#include "Lattice.h"
#include "TradeImages.h"
#include "FiniteDiff.h"

extern "C" {void __vdw_splines_MOD_spline_interpolation (double *x, const int *Nx, double *evaluation_points, const int *Ngrid_points, std::complex<double>  *values);}
extern "C" {void initialize_spline_interpolation (double *x, int *Nx, double *d2y_dx2);}

// Needed to deal with some issues when calling f90 module function from C++
#define  spline_interpolation  __vdw_splines_MOD_spline_interpolation

class Vdw {

private:

    // BaseGrid class
    BaseGrid *G;

    // TradeImages object to use
    TradeImages *T;

    // Lattice object
    Lattice *L;

    //
    int type;

    // Real space basis on this node and grid parameters
    int pbasis;

    double hxgrid;
    double hygrid;
    double hzgrid;
    int dimx;
    int dimy;
    int dimz;

    double *total_rho;
    double *gx;
    double *gy;
    double *gz;
    double *q0;
    double *dq0_drho;
    double *dq0_dgradrho;
    std::complex<double> *thetas;

    // How many terms to include in the sum of SOLER equation 5.
    int m_cut;

    // ----------------------------------------------------------------------
    // The next 2 parameters define the q mesh to be used in the vdW_DF code.
    // These are perhaps the most important to have set correctly. Increasing
    // the number of q points will DRAMATICALLY increase the memory usage of
    // the vdW_DF code because the memory consumption depends quadratically
    // on the number of q points in the mesh. Increasing the number of q
    // points may increase accuracy of the vdW_DF code, although, in testing
    // it was found to have little effect. The largest value of the q mesh
    // is q_cut. All values of q0 (DION equation 11) larger than this value
    // during a run will be saturated to this value using equation 5 of
    // SOLER. In testing, increasing the value of q_cut was found to have
    // little impact on the results, though it is possible that in some
    // systems it may be more important. Always make sure that the variable
    // Nqs is consistent with the number of q points that are actually in the
    // variable q_mesh. Also, do not set any q value to 0. This will cause an
    // infinity in the Fourier transform.
    //
    //
    //                CHANGE THESE VALUES AT YOUR OWN RISK

    static int Nqs;
    static double q_mesh[20];

    // largest value of q_mesh
    double q_cut;

    // smallest value of q_mesh
    double q_min;

    // cutoff value for density
    static const double epsr;

public:
    Vdw (BaseGrid &G, Lattice &L, TradeImages &T, int type, double *rho_valence, double *rho_core, double &etxc, double &vtxc, double *v);
    ~Vdw(void);

    void get_q0_on_grid (void);
    void saturate_q(double q, double q_cut, double &q0, double &dq0_dq);


    double Fs(double s);
    double dFs_ds(double s);
    double kF(double rho);
    double dkF_drho(double rho);
    double ds_drho(double rho, double s);
    double ds_dgradrho(double rho);
    double dqx_drho(double rho, double s);
    void pw(double rs, int iflag, double &ec, double &vc);


};



#endif