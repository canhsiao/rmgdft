/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 
/****f* QMD-MGDFT/md.h *****
 * NAME
 *   Ab initio real space code with multigrid acceleration
 *   Quantum molecular dynamics package.
 *   Version: 2.1.5
 * COPYRIGHT
 *   Copyright (C) 1995  Emil Briggs
 *   Copyright (C) 1998  Emil Briggs, Charles Brabec, Mark Wensell, 
 *                       Dan Sullivan, Chris Rapcewicz, Jerzy Bernholc
 *   Copyright (C) 2001  Emil Briggs, Wenchang Lu,
 *                       Marco Buongiorno Nardelli,Charles Brabec, 
 *                       Mark Wensell,Dan Sullivan, Chris Rapcewicz,
 *                       Jerzy Bernholc
 * FUNCTION
 *   
 * INPUTS
 *
 * OUTPUT
 *  
 * PARENTS
 *
 * CHILDREN
 * 
 * SEE ALSO
 *  
 * SOURCE
 */


#define ORDER_N 1

/* Version information */
#include    "version.h"
#include    "input.h"


/* Compile time parameters */
#include    "params.h"


/* Constants and symbolic definitions */
#include    "const.h"

/* Fourier transformation structure definition */
#include    "fftw.h"

#include    "my_finegrid.h"
#include    "twoParts.h"


#include "my_scalapack.h"

#include "my_mpi.h"


/** Size of floating point variables used in QMD */
#define     REAL    double


int MXLLDA, MXLCOL;
REAL *rho, *rho_old, *rhoc, *vh, *vnuc, *vcomp, *vxc, *rhocore, *vtot,
    *vtot_c;
REAL *vh_old, *vxc_old;
REAL *statearray, *l_s, *matB, *mat_hb, *mat_X, *Hij, *theta, *work_dis;
double *Hij_00, *Bij_00;
REAL *work_dis2, *zz_dis, *gamma_dis, *uu_dis, *mat_Omega;
REAL *work_matrix_row, *nlarray1;
REAL *projectors, *projectors_x, *projectors_y, *projectors_z;
REAL *sg_twovpsi, *sg_res;
int *nlindex;
REAL *work_memory;
REAL *sg_orbit;
REAL *sg_orbit_res;
REAL *orbit_tem;
REAL *vtot_global;


int NPES;
int NX_GRID, NY_GRID, NZ_GRID;
int P0_BASIS;
int S0_BASIS;
int PX0_GRID;
int PY0_GRID;
int PZ0_GRID;
int state_to_ion[MAX_STATES];
int state_to_proc[MAX_STATES];
char num_loc_st[MAX_IONS];
short int overlap_or_not[MAX_IONS * MAX_IONS];
short int state_overlap_or_not[MAX_STATES * MAX_STATES];

struct b_list
{
    char *TagName;
    int Flag;
};

/* option tags list */
struct t_list
{
    char *TagName;
    char *OptName;
    int OptCount;
    char *Opt[MAX_OPTS];
};


/* Array telling for each pair of ions if the localization regions
   around touch both a same region (anywhere on any PE) */
int array_both_not_zero[MAX_IONS][MAX_IONS];

/** @name PE_CONTROL
 *
 * @memo Processor control structure.
 * 
 * This is a global structure declared as extern PE_CONTROL pct.
 * 
 */

typedef struct
{


   /** Number (rank in MPI terminology) of this processor in this image
 * grid */
    int gridpe, imgpe, thisimg, spinpe;

    /** Number of grids (typically 1) per image to be run simultaneously
 * **/
    int images, grids;

    /* MPI communicators for each code grid (grid_comm) and one
 * (rmg_comm)
 *      * for all group rank 0 pe's. The later effectively replaces
 *      MPI_COMM_WORLD
 *           * unless you really need all-to-all, even across grids,
 *           communication. */
    MPI_Comm rmg_comm, img_topo_comm, grid_topo_comm, grid_comm, img_comm, spin_comm;


    int instances;
    /** Neighboring processors in three-dimensional space */
    int neighbors[6];

    /** Processor kpoint- coordinate for domain decomposition */
    /*  paralleled for kpoint */
    int pe_kpoint;

    /** Processor x-coordinate for domain decomposition */
    int pe_x;
    /** Processor y-coordinate for domain decomposition */
    int pe_y;
    /** Processor z-coordinate for domain decomposition */
    int pe_z;

    /** An index array which maps the projectors onto the 3-d grid associated 
     * whith each processor */
    int *Qindex[MAX_IONS];

    /** An index array which indicate whether the grid map on the current processor */
    int *Qdvec[MAX_IONS];

    /** Number of points in the Qindex array for each ion */
    int Qidxptrlen[MAX_IONS];

    /** Number of points in the circle of local projector for eac processor*/
    int lptrlen[MAX_IONS];

    /** Point to start of storage for the augument function */
    REAL *augfunc[MAX_IONS];

    /** Point to start of DnmI function storage for this ion */
    REAL *dnmI[MAX_IONS];

    /** Point to start of qqq storage for this ion */
    REAL *qqq[MAX_IONS];

    /** local grid size for x,y,z **/
    int nx_grid;
    int ny_grid;
    int nz_grid;

    /* kpoint index for start and end for a subdomain processors */
    int kstart;
    int kend;

    /*  processor coordinates in COMM_KP communicator */
    int coords[2];

    /* Number of ions centered on this processor */
    int n_ion_center;

    /* Indices of the ions centered on this PE */
    int ionidxcenter[IONS_PER_PE];

    /* Projectors per ion in a given region */
    int prj_per_ion[MAX_IONS];

    /* Indices of the ions within non-local range */
    int ionidx[IONS_PER_PE];

    /* Points to start of projectors for this ion in projector space */
    /* All projectors are stored consecutively.                      */
    int prj_ptr[IONS_PER_PE];

    /* Pointers to the index array for the non-local potentials */
    int idxptr[IONS_PER_PE];




    /* The size of local orbitals on this PE */
    int psi_size;
    /* pointer to former step solution, used in pulay and KAIN mixing  */
    int desca[DLEN];
    int descb[DLEN];
    int mycol;
    int myrow;
    int nprow;
    int npcol;


} PE_CONTROL;


/**@name STATE
 *
 * @memo Wavefunction storage structure */
typedef struct 
{

    /** First iteration flag */
    int firstflag;

    /** Current estimate of the eigenvalue for this orbital (state). */
    REAL eig;

    /** Wavefunction residual error computed by multigrid solver */
    REAL res;

    /** Points to the storage area for the real part of the orbital */
    REAL *psiR;
    /** Points to the storage area for the imaginary part of the orbital */
    REAL *psiI;

    /** Occupation of the orbital */
    REAL occupation;
    REAL oldeig;

    /** Index showing which k-point this orbital is associated with */
    int kidx;
    int istate;

    /* The ion on which this state is localized */
    int inum;

    /* index for functions with same localization */
    int loc_index;

    /* Actual Physical coordinates at current time step */
    int pe;
    REAL crds[3];
    REAL radius;
    int movable;
    int frozen;
    int index;

    int ixmin;
    int ixmax;
    int iymin;
    int iymax;
    int izmin;
    int izmax;
    int xfold;
    int yfold;
    int zfold;
    int ixstart;
    int iystart;
    int izstart;
    int ixend;
    int iyend;
    int izend;
    int orbit_nx, orbit_ny, orbit_nz;
    int size;
    /* Localization mask */
    char *lmask[4];

    int atomic_orbital_index;


    int n_orbital_same_center;
    int gaussian_orbital_index;


} STATE;



/**@name SPECIES
 * @memo Species (pseudopotential) control structure
 * @doc Structure holds data about the pseudopotentials used to
 * represent different types of atomic species. 
 */
typedef struct 
{

    /** Description of the species (e.g Atomic carbon generated using 
     * hamann's code with  rcs=0.80 rcp=0.85 bohr
     */

    /* symbol read from control file */
    char pseudo_symbol[32];

    /* pseudopotential filename */
    char pseudo_filename[MAX_PATH];

    char description[256];

    /** Atomic number */
    int atomic_number;

    /** Atomic symbol */
    char *atomic_symbol;

    /** Atomic mass */
    REAL atomic_mass;

    /** Number of valence electrons */
    REAL zvalence;

    /** Gaussian charge parameter used for compensating the 1/r Coulomb
     * tail of the pseudopotentials
     */

    REAL rc;

    /* Number of grid points in the local and non-local potential localized grids
     * in each coordinate direction. These used to be L0_LDIM and L0_NLDIM.
     */
    int ldim;
    int nldim;
    int nlfdim;
    int qdim;


    /* These are input parameters in the pseudopotential file. They represent the
     * real radii that are used in generating ldim and nldim.
     */
    REAL lradius;
    REAL nlradius;
    REAL qradius;

    /*Radius for milliken analysis*/
    REAL mill_radius;
    /*Radius in number of grid points*/
    int mill_dim;
    /*Number of radial atomic wave functions - these depend on l only, not on m*/
    int num_atomic_waves;
    /*l-numbers for states for which we have atomic orbitals*/
    int lstate_atomic_wave[5];
    /*Sum of all atomic states (with different l or m numbers*/
    int sum_atomic_waves;

    /* Pseudo atomic valence density */
    /*REAL avdens[MAX_RGRID];*/
    REAL **atomic_wave;


    /*This will store name of atomic wavefunctions, such as s, px, py, pz, dxx etc*/
    char atomic_wave_symbol[20][12];




    /** Number of radial grid points in the pseudopotential file */
    int rg_points;

    /* Log mesh parameter */
    REAL almesh;
    /* Log mesh parameter, where aa=exp(-aasf)/Z, bb=1.0/bbsf */
    REAL aa, bb;

    /** Non-linear core correction flag */
    int nlccflag;

    /* Number of potentials */
    int num_potentials;

    /* L-values for the reference states */
    int lval[10];

    /* L-value for local pseudopotential state */
    int local;

    /* Index for local pseudopotential state */
    int localidx;

    /* Radial grids */
    /*  REAL r[MAX_L+1][MAX_RGRID]; */

    /* Reference states */
    REAL psi[MAX_L + 1][MAX_RGRID];


    /* Pseudopotentials */
    REAL psp[MAX_L + 1][MAX_RGRID];

    /* Number of grid points in the beta function */
    int kkbeta;

    /* Matrix ddd0(nbeta,nbeta) */
    REAL ddd0[18][18];
    REAL ddd[18][18];

    /* Matrix qqq(nbeta,nbeta) */
    REAL qqq[18][18];

    /* The number of L=|l1-l2|.....|l1+l2|, we limit nlc <=5 */
    int nlc;

    /* The number of component in polynomial of the pseudized Q_I(r) function we limit nqf<=10 */
    int nqf;

    /* L-independent inner cutoff radii rinner for Q_I(r) function */
    REAL rinner[5];

    /* ultrosoft Vanderbilt Qnm_rad(r) function and */
    REAL *qnm;
    REAL *qnmlig;
    REAL *drqnmlig;

    /* the coefficient for pseudosation of Qnm_L(r) */
    REAL *qfcoef;

    /* logarithmic radial mesh information */
    REAL r[MAX_RGRID];
    REAL rab[MAX_RGRID];

    /* Local Pseudopotentials */
    REAL vloc0[MAX_RGRID];


    /* Core charge radial grids */
    REAL cr[MAX_RGRID];


    /* Pseudo atomic valence density */
    REAL avdens[MAX_RGRID];


    /* Pseudo atomic core density */
    REAL rspsco[MAX_RGRID];


    /* The L-value for the beta function */
    int llbeta[MAX_NB];

    /* Ultrasoft Vanderbilt beta_n(r) function on radial grid */
    REAL beta[MAX_NB][MAX_RGRID];


    /* Total number of projectors */
    int nbeta;

    /* Pseudo atomic core density */
    REAL acdens[MAX_RGRID];


    /* Kleinman-Bylander Projectors on radial grid */
    REAL kbp[MAX_L + 1][MAX_RGRID];


    /* Radial derivative terms for Kleinman-Bylander projectors */
    REAL drkbp[MAX_L + 1][MAX_RGRID];


    /* Local difference potential on radial grid */
    REAL difflocal[MAX_RGRID];


    /* Radial derivative of local difference potential */
    REAL drdifflocal[MAX_RGRID];


    /* Total number of projectors */
    int ipcount;


    /* Normalization coefficients for the KB projectors */
    REAL kbnorm[MAX_L + 1];


    /* Milliken projector normalization coefficients */
    REAL mnorm[MAX_L + 1];


    /* Linear interpolation storage for the compensated local potential
     * and for it's radial derivative.
     */
    REAL localig[MAX_LOCAL_LIG];
    REAL drlocalig[MAX_LOCAL_LIG];

    /* Linear interpolation storage for the core charge density */
    REAL rhocorelig[MAX_LOCAL_LIG];

    /* Kleinman-Bylander Projectors on linear interpolation grid */
    REAL betalig[MAX_NB][MAX_LOCAL_LIG];

    /* Radial derivatives of the Kleinman-Bylander Projectors on linear interpolation grid */
    REAL drbetalig[MAX_NB][MAX_LOCAL_LIG];
    /* Kleinman-Bylander Projectors on linear interpolation grid */
    REAL kbplig[MAX_L + 1][MAX_LOCAL_LIG];

    /* Radial derivatives of the Kleinman-Bylander Projectors on linear interpolation grid */
    REAL drkbplig[MAX_L + 1][MAX_LOCAL_LIG];

    /* Reference states on linear interpolation grid */
    REAL psilig[MAX_L + 1][MAX_LOCAL_LIG];


    /* Local potential linear interpolation grid spacing */
    REAL drlig;


    /* Non-local linear interpolation grid spacing */
    REAL drnlig;

    /* Qfunction linear interpolation grid spacing */
    REAL drqlig;

    /* Pseudopotential filtering parameters */
    REAL lrcut;                 /* Real space local cutoff */
    REAL nlrcut[5];             /* Real space non-local cutoff */
    REAL rwidth;                /* Real-space width parameter */
    REAL gwidth;                /* G-space width parameter */

    int num_projectors;

    fftw_complex *forward_beta;
    fftw_complex *forward_derbeta_x;
    fftw_complex *forward_derbeta_y;
    fftw_complex *forward_derbeta_z;

    char *backward_wisdom;



} SPECIES;


/* Ion structure */
typedef struct
{

    /* Initial physical coordinates at start of run */
    REAL icrds[3];

    /* Actual Physical coordinates at current time step */
    REAL crds[3];

    /* Positions at the previous time step */
    REAL ocrds[3];

    /* Initial crystal coordinates at start of run */
    REAL ixtal[3];

    /* Actual crystal coordinates at current time step */
    REAL xtal[3];

    /* Crystal coordinates  at the previous time step */
    REAL oxtal[3];

    /* Coordinates of the corner of the grid that the local */
    /* difference potential is nonzero on.                  */
    REAL lxcstart;
    REAL lycstart;
    REAL lzcstart;


    /* Coordinates of the corner of the grid that the non-local */
    /* potential is nonzero on.                                 */
    REAL nlxcstart;
    REAL nlycstart;
    REAL nlzcstart;


    /* Coordinates of the corner of the grid that the non-local */
    /* potential is nonzero on.                                 */
    REAL Qxcstart;
    REAL Qycstart;
    REAL Qzcstart;

    /* Integer species type when using a raw pseudopotential */
    int species;


    /* Forces on the ion */
    REAL force[4][3];

    /* Current velocity of the ion */
    REAL velocity[3];

    /* Kleinman-Bylander normalization coefficients */
    REAL pd[(MAX_L + 1) * (MAX_L + 1)];

    /* Milliken normalization coefficients */
    REAL mnorm[(MAX_L + 1) * (MAX_L + 1)];

    /* Total number of projectors */
    int prjcount;

    /* Movable flag */
    int movable;

    int frozen;

    /*  number of local orbitals on the ion */
    int n_loc_states;

    /* Localization mask */
    char *lmask[4];

    int ixstart;
    int iystart;
    int izstart;
    int ixend;
    int iyend;
    int izend;

    int first_state_index;
    double *fftw_phase_sin;
    double *fftw_phase_cos;

 /* Force modifier parameters */
    struct {
        REAL setA_weight;
        REAL setA_coord[3];
        REAL setB_weight;
        REAL setB_coord[3];
    } constraint;



} ION;



/* multigrid-parameter structure */
typedef struct
{

    /* number of global-grid pre/post smoothings and timestep */
    REAL gl_step;
    int gl_pre;
    int gl_pst;

    /* timestep for the subiteration */
    REAL sb_step;

    /* timestep for the Richardson-Iteration */
    REAL ri_step;

    /* lowest MG level */
    int levels;


} MG_PARM;

/* Nose control structure */
typedef struct
{

    /* number of atoms allowed to move */
    int N;

    /* ionic target temperature in Kelvin */
    REAL temp;

    /* ionic target kinetic energy */
    REAL k0;

    /* randomize velocity flag */
    int randomvel;

    /* Nose oscillation frequency */
    REAL fNose;

    /* number of thermostats used */
    int m;

    /* thermostat positions,velocities,masses and forces */
    REAL xx[10];
    REAL xv[10];
    REAL xq[10];
    REAL xf[4][10];

} FINITE_T_PARM;

/** @name KPOINT
 * @memo Holds data specific to individual k-points.
 */
typedef struct 
{

    /** The index of the k-point for backreferencing */
    int kidx;

    /** The k-point */
    REAL kpt[3];

    /** The corresponding vector */
    REAL kvec[3];

    /** The weight associated with the k-point */
    REAL kweight;

    /** The magnitude of the k-vector */
    REAL kmag;

    STATE *kstate;

    /* Mean min, and max wavefunction residuals for occupied space */
    REAL meanres;
    REAL minres;
    REAL maxres;

} KPOINT;


/** @name CONTROL
  @memo Main control structure

  This is a global structure declared as extern CONTROL ct

 */
typedef struct
{

    /** Description of the run. */
    char description[200];

    /** time at which run started */
    REAL time0;

    int spin_flag;
    /** Name of the input control file. Passed as a command line argument
     *
     *  Example:
     *  bash$  md in.diamond8
     */
    char cfile[MAX_PATH];
    char basename[MAX_PATH];

    FILE *logfile;

    /** Input file name to read wavefunctions from when doing a restart */
    char infile[MAX_PATH];

    /** Input file name to write wavefunctions to */
    /* Output file name */
    char outfile[MAX_PATH];

    /** File to read the pseudopotentials from */
    char pspfile[MAX_PATH];

    /** Initial run flag. Read from the input file.
      0=initial run otherwise a restart */
    int runflag;

    /*  spin polarize flag, 0 = no spin, 1 = spin polarized */
    int spin;

    /* output z-average of states */
    int zaverage;

    /* number of state to output */
    int plot_state;

    /* Exchage-Correlation flag */
    int xctype;

    /** Boundary condition flag. Read from the input file. 
      0=periodic, 1=cluster, 2=surface */
    int boundaryflag;

    /* Coordinate input flag: crystal or cartesian */
    int crd_flag;

    /* Maximum number of MD steps */
    int max_md_steps;

    /* MD steps iterator */
    int md_steps;

    /* Maxium number of SCF steps in an MD step */
    int max_scf_steps;
    int freeze_orbital_step;

    /* Actual number of steps done */
    int scf_steps;

    /* Total number of SCF steps done */
    int total_scf_steps;

    /* override occupations */
    int override_occ;

    /* Override of positions (during restart) */
    int override_atoms;

    char occupation_str[256];

    /* convergence criterion */
    REAL thr_rms;

    /* force convergence criterion */
    REAL thr_frc;

    /* Number of steps after which to perform checkpointing */
    int checkpoint;

    /* Number of steps after which to output results */
    int outcount;

    /** Sorting flag for wavefunctions. Read from input file. 0=no sort, 1=sort */
    int sortflag;

    /** Number of states */
    int num_states;

    /** Number of ions */
    int num_ions;

    /** Ion structure */
    ION *ions;

    /** Number of species */
    int num_species;

    /* Cutoff parameter */
    REAL cparm;
    REAL betacparm;
    REAL qcparm;

    /** Total conpensating charge density */
    REAL crho;

    /** Total charge in supercell */
    REAL tcharge;

    /** Species structure 
     * @see SPECIES */
    SPECIES *sp;

    /** Global uniform grid spacing in x */
    REAL hxgrid;

    /** Global uniform grid spacing in y */
    REAL hygrid;

    /** Global uniform grid spacing in z */
    REAL hzgrid;

    /* The fine uniform grid spacing in x */
    REAL hxxgrid;

    /* The fine uniform grid spacing in y */
    REAL hyygrid;

    /* The fine uniform grid spacing in z */
    REAL hzzgrid;

    /** bravais lattice type */
    int ibrav;

    /** Lattice information */
    REAL celldm[6];

    /* lattice vectors */
    REAL a0[3];
    REAL a1[3];
    REAL a2[3];

    /** Total cell volume */
    REAL omega;

    /* lengths of the sides of the supercell */
    REAL xside;
    REAL yside;
    REAL zside;

    /* This is the max of nldim for any species cubed */
    int max_nlpoints;
    int max_Qpoints;

    /** Maximum grid spacing in any coordinate direction */
    REAL hmaxgrid;


    /** Minimum grid spacing in any coordinate direction */
    REAL hmingrid;


    /** Grid anisotropy defined as the ratio of hmaxgrid to hmingrid. 
      A value larger than 1.05 can lead to convergence problems. */
    REAL anisotropy;


    /** Volume element associated with each grid point */
    REAL vel;
    REAL vel_f;


    /** Physical grid basis size */
    int nbasis;


    /** Density mixing parameter. Typical values range from 0.2 to 0.9, while larger values provide faster convergence as long as they are stable. */
    REAL mix;


    /* Projector mixing parameter */
    REAL prjmix;


 /*Order of Pulay mixing for charge density*/
    int charge_pulay_order;

    /*How often to refresh Pulay history*/
    int charge_pulay_refresh;

    /*Flag to test whether or not the modified metrics should be used in * Pulay mixing*/
    int charge_pulay_special_metrics;

    /*Weight for Pulay special metrics*/
    REAL charge_pulay_special_metrics_weight;


    /* Global uniform grid corner */
    REAL xcstart;
    REAL ycstart;
    REAL zcstart;


    /* Hartree potential grid sizes per domain */
    int vh_pxgrid;
    int vh_pygrid;
    int vh_pzgrid;

    /* Potential grid sizes */
    int vh_nxgrid;
    int vh_nygrid;
    int vh_nzgrid;


    /* Total points in hartree potential per domain */
    int vh_pbasis;


    /* Wavefunction grid sizes */
    int psi_nxgrid;
    int psi_nygrid;
    int psi_nzgrid;

    int psi_fnxgrid;
    int psi_fnygrid;
    int psi_fnzgrid;
    /* Total points for wavefunctions */
    int psi_nbasis;
    int psi_fnbasis;

    /* Total points for potential */
    int vh_nbasis;

    /* Decoupled hartree potential */
    REAL *vh_ext;


    /* Mean min, and max wavefunction residuals for occupied space */
    REAL meanres;
    REAL minres;
    REAL maxres;

    /* total ionic charge */
    REAL ionic_charge;

    /* Variable occupation stuff */
    REAL nel;

    int occ_flag;

    REAL occ_width;

    REAL occ_mix;

    /** total background smearing charge -- for charged supercells */
    REAL background_charge;


    /** Multigrid parameters for the eigenvalue solver */
    MG_PARM eig_parm;

    /** Multigrid parameters for the poisson solver */
    MG_PARM poi_parm;


    /* Total number of electrons */
    REAL num_el;

    /** Nose paramters */
    FINITE_T_PARM nose;

    /* force pointer array */
    int fpt[4];

    /* temperature control */
    int tcontrol;

    /* md integrate level */
    int mdorder;

    /* movie flags */
    int rmvmovie, chmovie, xbsmovie;
    
    int domilliken, milliken;


    /* Diagonalization flag and period */
    int initdiag;
    int diag;
    int end_diag;

    /** Force flag. 0=don't compute forces, 1=compute forces */
    int forceflag;

    /* Desired vector for constrained dynamics */
    REAL cd_vector[3];

    /* number of velocity in constrained dynamics */
    REAL cd_velocity;


    /** Ionic motion timestep */
    REAL iondt;


    /** Ionic motion energy */
    REAL ionke;


    /* Total energies */
    REAL ES;
    REAL NUC;
    REAL KE;
    REAL XC;
    REAL NL;
    REAL II;
    REAL TOTAL;
    REAL Evxcold_rho;
    REAL Evhold_rho;
    REAL Evh_rho;
    REAL Evh_rhoc;

    REAL TOTAL_former;
    REAL dE;

    REAL *energies;

    int restart_mix;

    int move_centers_at_this_step;


    /* fermi energy */
    REAL efermi;

    /** Total number of k-points being used in the calculation */
    int num_kpts;


    /** K-point control structure */
    KPOINT *kp;

    /** The maximum number of projectors for any species */
    int max_nl;

    int num_storage_st;


    /* Index telling if an orbital has been allocated */
    char allocate_st[MAX_STATES];

    /* Number of orbital in each global function */
    int nb_func[MAX_STATES];

    /* Initial storage of each orbital
       (to restart with same function) */
    int alloc_ini[MAX_STATES];

    /* "Center" of global function */
    REAL center[MAX_STATES][3];

    REAL Bc;
    REAL Bx;
    REAL By;
    REAL Bz;
    REAL Ac;
    REAL Ax;
    REAL Ay;
    REAL Az;
    REAL Axy;
    REAL Axz;
    REAL Ayz;

    int state_per_proc;
    int state_begin;
    int state_end;

    int max_orbit_size;
    int max_orbit_nx;
    int max_orbit_ny;
    int max_orbit_nz;

    int movingCenter;
    int movingSteps;
    int mg_method;
    int mg_steps;
    STATE *states;


    /*This keeps track whether ct.fftw_wisdom_setup was setup or not so that
     *      * we know whether to release wisdom memory at the end or not*/
    int fftw_wisdom_setup;


    char file_atomic_orbit[MAX_SPECIES][MAX_PATH];

    int nxfgrid;
    int nyfgrid;
    int nzfgrid;

    /* output information for GW calculations.  --Xiaohong */
    int flag_gw;

        /* the external electric field */
    REAL e_field;

    REAL x_field_0;

    REAL y_field_0;

    REAL z_field_0;

     /* Should we process constraints on forces flag */
    int constrainforces;

    REAL neb_spring_constant;

} CONTROL;


/* Extern declaration for the main control structure */
extern CONTROL ct;
STATE states[MAX_STATES];
STATE states1[MAX_STATES];
STATE states_res[MAX_STATES];
STATE states_res1[MAX_STATES];
STATE states_tem[MAX_STATES];


/* Extern declaration for the processor control structure */
extern PE_CONTROL pct;


/* Header file for blas routines */
#include "blas.h"
#include "blas3_def.h"
#include "lapack_def.h"



/* Function prototypes */
void app_4del2 (REAL * f, REAL * work);
REAL app_del2c (REAL * a, REAL * b, int dimx, int dimy, int dimz,
        REAL gridhx, REAL gridhy, REAL gridhz);

void app_smooth (REAL * f, int dimx, int dimy, int dimz);
void app_cir (REAL * a, REAL * b, int dimx, int dimy, int dimz);
void app_cir_0 (REAL * a, REAL * b, int dimx, int dimy, int dimz);
void app_cir_ortho_0 (REAL * a, REAL * b, int dimx, int dimy, int dimz);
void app_cir_ortho (REAL * a, REAL * b, int dimx, int dimy, int dimz);
void app_cir_bcc (REAL * a, REAL * b, int dimx, int dimy, int dimz);
void app_cir_fcc (REAL * a, REAL * b, int dimx, int dimy, int dimz);
void app_cir_hex (REAL * a, REAL * b, int dimx, int dimy, int dimz);
REAL app_cilr (REAL * a, REAL * b, REAL * c, int dimx, int dimy, int dimz,
        REAL gridhx, REAL gridhy, REAL gridhz);
REAL app_cilr_bcc (REAL * a, REAL * b, REAL * c, int dimx, int dimy, int dimz,
        REAL gridhx, REAL gridhy, REAL gridhz);
REAL app_cilr_fcc (REAL * a, REAL * b, REAL * c, int dimx, int dimy, int dimz,
        REAL gridhx, REAL gridhy, REAL gridhz);
REAL app_cilr_hex (REAL * a, REAL * b, REAL * c, int dimx, int dimy, int dimz,
        REAL gridhx, REAL gridhy, REAL gridhz);
REAL app_cilr_ortho (REAL * a, REAL * b, REAL * c, int dimx, int dimy,
        int dimz, REAL gridhx, REAL gridhy, REAL gridhz);
REAL app_cil (REAL * a, REAL * b, int dimx, int dimy, int dimz, REAL gridhx,
        REAL gridhy, REAL gridhz);
void app_nl (REAL * psiR, REAL * psiI, REAL * workR, REAL * workI, int state,
        int flag, int kidx, int tid);
void cholesky (REAL * a, int n);
void cross_product (REAL * a, REAL * b, REAL * c);
void eval_residual (REAL * mat, REAL * f_mat, int dimx, int dimy, int dimz,
        REAL gridhx, REAL gridhy, REAL gridhz, REAL * res);
void solv_pois (REAL * vmat, REAL * fmat, REAL * work,
        int dimx, int dimy, int dimz, REAL gridhx,
        REAL gridhy, REAL gridhz, double step);
REAL fill (STATE * states, REAL width, REAL nel, REAL mix,
        int num_st, int occ_flag);
void force (REAL * rho, REAL * rho_opps, REAL * rhoc, REAL * vh, REAL * vxc, REAL * vnuc, STATE * states);
void genvpsi (REAL * psi, REAL * twovpsi, REAL * pvtot, REAL * pvnl,
        REAL * kd, REAL kmag, int dimx, int dimy, int dimz);
void get_index_loc (STATE *);
void get_nlop (void);
void get_phase (ION * iptr, REAL * rtptr, int ip, int icount, int *dvec);
void get_nlop_smp (int tid);
void get_nlop_f (ION * iptr, REAL * rtptr, int ip);
void get_eig (STATE * states, REAL * vxc, REAL * vh, REAL * vnuc);
char *get_num (char *str);
void get_rho (STATE * states, REAL * rho);
void get_te (REAL * rho, REAL * rhocore, REAL * rhoc, REAL * vh,
        REAL * vxc, STATE * states);
void get_vxc (REAL * rho, REAL * rhocore, REAL * vxc);
void get_zdens (STATE * states, int state, REAL * zvec);
void xclda_pz81 (REAL * rho, REAL * vxc);
void exclda_pz81 (REAL * rho, REAL * exc);
void xcgga (REAL * rho, REAL * vxc, REAL * exc, int flag);
void gram (KPOINT * kpoint, REAL h, int numst, int maxst, int numpt,
        int maxpt);
REAL get_ke (STATE * sp, int tid);
void get_vh (REAL * rho, REAL * rhoc, REAL * vh, int cycles, int maxlevel);
void global_sums (REAL * vect, int *length, MPI_Comm comm);
void iiforce (void);
void init (REAL * vh, REAL * rho, REAL * rhocore, REAL * rhoc, STATE * states,
        STATE * states1, REAL * vnuc, REAL * vxc, REAL * vh_old,
        REAL * vxc_old);
void init_kbr (void);
void init_pe_on (void);
void init_pegrid (void);
void init_wf (STATE * states);
void init_wflcao (STATE * states);
void init_nuc (REAL * vnuc, REAL * rhoc, REAL * rhocore);
void init_pos ();
void init_sym (void);
void symmetrize_rho (REAL * rho);
void symforce (void);
void lforce (REAL * rho, REAL * vh);
void nlccforce (REAL * rho, REAL * vxc);
void cforce (REAL * rho, REAL * vh);
void rmg_timings (int what, REAL time);
void mg_eig_state (STATE * states, int tid, REAL * vtot);
REAL minimage (ION * ip1, ION * ip2, REAL * xtal_r);
REAL my_crtc (void);
void nlforce (REAL *veff);
/* Local function prototypes */
void nlforce_s (int ion, int ip, STATE * states, STATE * states1);
void nlforce_p (int ion, int ip, STATE * states, STATE * states1);
void nlforce_d (int ion, int ip, STATE * states, STATE * states1);
void norm_psi (STATE * psi, REAL bv);
void ortho_full (STATE * states);
void ortho_half (STATE * states);
void ortho_bcc (STATE * states);
void output (STATE * states, int its);
void pe2xyz (int pe, int *x, int *y, int *z);
void pack_ptos (REAL * sg, REAL * pg, int dimx, int dimy, int dimz);
void pack_stop (REAL * sg, REAL * pg, int dimx, int dimy, int dimz);
void pack_stop_axpy (REAL * sg, REAL * pg, REAL alpha, int dimx, int dimy,
        int dimz);
void pack_ptos_trade (REAL * sg, REAL * pg, int dimx, int dimy, int dimz);
void pack_vhstod (REAL * s, REAL * d, int dimx, int dimy, int dimz);
void pack_vhdtos (REAL * s, REAL * d, int dimx, int dimy, int dimz);
double radint (double *f, double *r, int n, double al);
void radiff (double *f, double *df, double *r, int n, double al);
void ra2diff (double *f, double *df, double *r, int n, double al);
void ranv (void);
void read_control (void);
void read_data (char *name, REAL * vh, REAL * rho, REAL * vxc, REAL * vh_old,
        REAL * vxc_old, STATE * states);
void read_pseudo (void);
REAL real_sum_all (REAL x, MPI_Comm comm);
REAL real_max_all (REAL x);
void rft (REAL * f, REAL * r, REAL * ffil, REAL al, int rg_points, int lval,
        REAL dr, REAL width, int lrg_points);
void scf (STATE * states, STATE * states1, REAL * vxc, REAL * vh, REAL * vnuc,
        REAL * rho, REAL * rhocore, REAL * rhoc, REAL * vxc_old,
        REAL * vh_old, int *CONVERGENCE);
void sortpsi (STATE * states);
void subdiag (STATE * states, REAL * vh, REAL * vnuc, REAL * vxc);
void trade_images (REAL * mat, int dimx, int dimy, int dimz, int *nb_ids);
void trade_images_mpi (REAL * mat, int dimx, int dimy, int dimz, int *nb_ids);
void trade_images_smp (REAL * mat, int dimx, int dimy, int dimz, int *nb_ids);
void set_bc (REAL * mat, int dimx, int dimy, int dimz, int images, REAL val);
void getpoi_bc (REAL * rho, REAL * vh_bc, int dimx, int dimy, int dimz);
void vol_rho (REAL * rho, int step);
void vol_wf (STATE * states, int state, int step);
void write_avgd (REAL * rho);
void write_avgv (REAL * vh, REAL * vnuc);
void write_zstates (STATE * states);
void write_data (char *name, REAL * vh, REAL * rho, REAL * vxc, REAL * vh_old,
        REAL * vxc_old, STATE * states);
void write_header (void);
void write_pos (void);
void write_eigs (STATE * states);
void write_occ (STATE * states);
void write_force (void);
void write_timings (void);
REAL rand0 (long *idum);

void mg_restrict (REAL * full, REAL * half, int dimx, int dimy, int dimz);
void mg_prolong (REAL * full, REAL * half, int dimx, int dimy, int dimz);
void gather_psi (REAL * tmp_psiR, REAL * tmp_psiI, STATE * sp, int tid);
void scatter_psi (REAL * tmp_psiR, REAL * tmp_psiI, STATE * sp, int tid);
void get_milliken (STATE * states);

void bandstructure (STATE * states, REAL * vxc, REAL * vh, REAL * vnuc,
        REAL * rho, REAL * rhocore, REAL * rhoc);
void output_wave (STATE * states, int kpt, int fhand);

/* Blas wrappers */
void QMD_saxpy (int n, REAL alpha, REAL * x, int incx, REAL * y, int incy);
void QMD_sscal (int n, REAL alpha, REAL * x, int incx);
void QMD_scopy (int n, REAL * x, int incx, REAL * y, int incy);
REAL QMD_sdot (int n, REAL * x, int incx, REAL * y, int incy);



/* Conversion between crystal and cartesian coordinate prototypes */
void latgen (int *ibrav, REAL * celldm, REAL * A0I, REAL * A1I, REAL * A2I,
        REAL * OMEGAI, int *flag);
void recips (void);
void to_cartesian (REAL crystal[], REAL cartesian[]);
void to_crystal (REAL crystal[], REAL cartesian[]);
REAL metric (REAL * crystal);

/* Md run types */
void run (STATE * states, STATE * states1);
void quench (STATE * states, STATE * states1, REAL * vxc, REAL * vh,
        REAL * vnuc, REAL * vh_old, REAL * vxc_old, REAL * rho,
        REAL * rhocore, REAL * rhoc);
void fastrlx( STATE *states, STATE *states1, REAL *vxc, REAL *vh, REAL *vnuc, REAL *vh_old, REAL *vxc_old,
        REAL *rho, REAL *rhocore, REAL *rhoc );
void cdfastrlx (STATE * states, REAL * vxc, REAL * vh, REAL * vnuc,
        REAL * rho, REAL * rhocore, REAL * rhoc);
/*void moldyn (STATE * states, REAL * vxc, REAL * vh, REAL * vnuc, REAL * rho, REAL * rhoc, REAL * rhocore);*/
void dx (STATE * states, REAL * vxc, REAL * vh, REAL * vnuc, REAL * rho,
        REAL * rhoc);
void psidx (STATE * states, REAL * vxc, REAL * vh, REAL * vnuc, REAL * rho,
        REAL * rhoc);
void cholesky (REAL * a, int n);


REAL minimage1 (REAL aa[3], REAL bb[3]);
void init_parameter (STATE * states);
void get_mehr (void);
void make_mask_grid (REAL rcut, int level, STATE * states);
void make_mask_grid_state (int level, STATE * states);
void allocate_func (STATE * states, int inist);
void allocate_psi (STATE * states, STATE * states1);
void allocate_matrix ();
void xyz2pe (int x, int y, int z, int *pe);
void get_normKB (SPECIES * sp, double *pd);
void get_start (int cdim, double crds, double cstart, double hgrid,
        int *istart, double *nlcstart);
void get_index_array (int *pvec, int *dvec, int dim,
        int *Aix, int *Aiy, int *Aiz, int *icount,
        int index_low[3], int index_high[3]);
void get_Ai (int cdim, int *Ai, int ngrid, int istart);
void xclsd_pz81 (REAL * rho, REAL * vxc);
void global_sums_int (int *, int *);
REAL linint (REAL * y, REAL r, REAL invdr);
void ortho_norm_local (STATE * states);
void global_sums_int (int *vect, int *length);
void my_barrier (void);
int int_sum_all (int isum);
void matrix_and_diag (STATE * states, STATE * states1, REAL * vxc, int flag);
void get_kbpsi (STATE *sp1, double *kbpsi);
void precond_mg (double *res, double *work1, double *work2, int istate);

void matS_cholesky_real (STATE * states);
void get_invBmat (STATE * states, double *matB);
void print_matrix (double *b, int n, int ldb);
void get_overlap_real (double *aa, int numst, int numpt,
        int lda, double *ss, int lds);
void get_cholesky_real (double *matS);
void get_all_kbpsi (STATE * states1, STATE * states);
void get_Hvnlij (double *Aij);
void genvlocpsi (REAL * psi, int st1, REAL * work1, REAL * vtot_global, STATE * states);
void genvnlpsi (double *sg_twovpsi, double *vnl,
        int dimx, int dimy, int dimz);
void get_new_rho (STATE * states, double *rho);
void mg_eig (STATE * states, STATE * states1, double *vxc, double *vh,
        double *vnuc, double *rho, double *rhoc, REAL * vxc_old,
        REAL * vh_old);
void fsymforces (REAL * force, int *s, int *irg, int *irt, int *nat,
        int *ibrav, int *nsym, REAL * celldm, int *nr1, int *nr2,
        int *nr3);
void symmetry (int *ibrav, int *s, int *nsym, int *irg, int *irt, int *ftau,
        int *nat, REAL * tau, int *ityp, int *nks, REAL * xk,
        REAL * wk, REAL * celldm, int *nr1, int *nr2, int *nr3,
        int *wflag);
void symrho (REAL * rho, int *nr1, int *nr2, int *nr3, int *nsym, int *s,
        int *irg, int *ftau);

void sgama (int *nrot,int *nat,double *s,double *at,double *bg,double *tau,
            int *ityp,int *nsym,int *nr1,int *nr2,int *nr3,int *irg,
            int *irt,double *ftau,double *rtau,int *npk,int *nks,double *xk,
            double *wk,double *xau,double *rau,bool *invsym, int *wflag);

void line_min_three_point (STATE *, STATE *, REAL, REAL, REAL *, REAL *,
        REAL *, REAL *, REAL *);

void dot_product_orbit_orbit (STATE *orbit1, STATE *orbit2, STATE
*orbit3, double *H, double *S);

void orbit_dot_orbit (STATE * states, STATE * states1, REAL *Hij_row, REAL * Bij_row);

void app_mask (int istate, double *u, int level);
void allocate_masks (STATE * states);

void mgrid_solv (REAL * v_mat, REAL * f_mat, REAL * work, int dimx, int dimy,
        int dimz, REAL gridhx, REAL gridhy, REAL gridhz, int level,
        int *nb_ids, int max_levels, int *pre_cyc, int *post_cyc,
        int mu_cyc, int istate, int *iion, int flag_local);


void state_corner_xyz (STATE * states);

void density_orbit_X_orbit (int st1, int st2, REAL scale, REAL * psi1,
        REAL * psi2, REAL * rho_global, int mode,
        STATE * states);

char *get_symbol (int atomic_number);

/*begin shuchun wang */
void get_nlop_s (ION * iptr, REAL * rtptr, int ip, fftwnd_plan p1,
        fftwnd_plan p2);
void get_nlop_p (ION * iptr, REAL * rtptr, int ip, fftwnd_plan p1,
        fftwnd_plan p2);
void get_nlop_d (ION * iptr, REAL * rtptr, int ip, fftwnd_plan p1,
        fftwnd_plan p2);
void get_matB_qnm (double *Aij);
void pack_vtot_ftoc (REAL * vtot, REAL * vtot_c);
void get_qnmpsi (STATE *sp, double *kbpsi_one_state, double *work);
void qnm_beta_betapsi (STATE *sp, int ion2, REAL * prjptr, REAL * work);
void get_dnmpsi (STATE *sp, double *kbpsi_one_state, double *work);
void dnm_beta_betapsi (STATE *sp, int ion2, REAL scale, int ip2, REAL * prjptr,
        REAL * work);
void pack_rho_ctof (REAL * rho1, REAL * rho_f);
void rho_augmented (REAL * rho, REAL * global_mat_X);
void rho_Qnm_mat (double *Aij, REAL * global_mat_X);
void rho_nm_mat (double *Aij, REAL * global_mat_X);
void trade_images2 (REAL * f, REAL * w, int dimx, int dimy, int dimz);
void trade_images3 (REAL * f, REAL * w, int dimx, int dimy, int dimz);
void app_grad (REAL * f, REAL * wx, REAL * wy, REAL * wz, int dimx, int dimy,
        int dimz);
void app6_del2 (REAL * f, REAL * work, int dimx, int dimy, int dimz,
        REAL hxgrid, REAL hygrid, REAL hzgrid);
void app8_del2 (REAL * f, REAL * work, int dimx, int dimy, int dimz,
        REAL hxgrid, REAL hygrid, REAL hzgrid);
void app10_del2 (REAL * f, REAL * work, int dimx, int dimy, int dimz,
        REAL hxgrid, REAL hygrid, REAL hzgrid);
void get_ddd (REAL * veff);
void get_qqq ();
void init_qfunct (void);
void rft1 (REAL cparm, REAL * f, REAL * r, REAL * ffil, REAL * rab,
        int rg_points, int lval, REAL dr, REAL width, int lrg_points);
void get_QI (void);
void aainit (int lli, int mix, int lx, int mx, int nlx, REAL ap[][9][9],
        int lpx[][9], int lpl[][9][9]);
void ylmr2 (double *r, double *ylm);
REAL qval (int ih, int jh, REAL r, REAL invdr, REAL * ptpr, int *nhtol,
        int *nhtom, int *indv, REAL * ylm, REAL ap[][9][9], int lpx[][9],
        int lpl[][9][9], SPECIES * sp);
int get_index (int gridpe, ION * iptr, int *Aix, int *Aiy, int *Aiz, int *ilow, int *ihi,
        int *jlow, int *jhi, int *klow, int *khi, int cdim, int pxgrid,
        int pygrid, int pzgrid, int nxgrid, int nygrid, int nzgrid,
        REAL * xcstart, REAL * ycstart, REAL * zcstart);
REAL get_QnmL (int idx, int ltot, REAL r, SPECIES * sp);
void assign_weight (SPECIES * sp, fftw_complex * weptr, REAL * rtptr);
void pack_gftoc (SPECIES * sp, fftw_complex * gwptr, fftw_complex * gbptr);
void xcgga (REAL * rho, REAL * vxc, REAL * exc, int mode);
REAL radint1 (REAL * func, REAL * r, REAL * rab, int n);
void get_all_partial_kbpsi (STATE *states);
void partial_Mat_nm_R (double *partial_x, double *partial_y, double *partial_z, REAL * global_mat_X);
void nlforce_par_Q(REAL *veff, REAL *rho_nm, int ion, int nh, double
*force_tem);
void nlforce_par_rho(REAL *par_gamma_x, REAL *par_gamma_y, REAL *par_gamma_z, int ion, int nh);
void nlforce_par_omega(REAL *par_omega_x, REAL *par_omega_y, REAL *par_omega_z, int ion, int nh);
void partial_QI (int ion, REAL * QI_R, ION *iptr);
void partial_nlop_s (ION * iptr, REAL * betax, REAL * betay, REAL * betaz,
        int ip, fftwnd_plan p1, fftwnd_plan p2);
void partial_nlop_p (ION * iptr, REAL * betax, REAL * betay, REAL * betaz,
        int ip, fftwnd_plan p1, fftwnd_plan p2);
void partial_nlop_d (ION * iptr, REAL * betax, REAL * betay, REAL * betaz,
        int ip, fftwnd_plan p1, fftwnd_plan p2);
void get_mat_Omega (STATE * states, double Omega[]);
void ylmr2_x (double *r, double *ylm_x);
void ylmr2_y (double *r, double *ylm_y);
void ylmr2_z (double *r, double *ylm_z);
void qval_R (int ih, int jh, REAL r, REAL * x, REAL * qlig, REAL * drqlig,
        REAL invdr, int *nhtol, int *nhtom, int *indv, REAL * ylm,
        REAL * ylm_x, REAL * ylm_y, REAL * ylm_z, REAL ap[][9][9],
        int lpx[][9], int lpl[][9][9], REAL * Q_x, REAL * Q_y,
        REAL * Q_z, SPECIES * sp);
void md_fastrelax(void);
void change_states_crds (STATE * states);

/*end shuchun wang */

FILE *open_xbs_movie (char *filename);


/* Occupation flags */
#define OCC_NONE 0
#define OCC_FD 1
#define OCC_GS 2
#define OCC_EF 3



/* Crystal lattice types */
/** Simple cubic lattice type.
 *  @doc Set input file value = 1 */
#define CUBIC_PRIMITIVE 	1

/** Face centered cubic lattice type. 
 *  @doc Set input file value = 2 */
#define CUBIC_FC		2

/** Bodycentered cubic lattice type. 
 *  @doc Set input file value = 3 */
#define CUBIC_BC		3

/** Hexagonal lattice type. 
 *  @doc Set input file value = 4 */
#define HEXAGONAL		4

#define TRIGONAL_PRIMITIVE	5
#define TETRAGONAL_PRIMITIVE	6
#define TETRAGONAL_BC           7

/** Orthorhombic lattice type. 
 *  @doc Set input file value = 8 */
#define ORTHORHOMBIC_PRIMITIVE  8

#define ORTHORHOMBIC_BASE_CENTRED 9
#define ORTHORHOMBIC_BC         10
#define ORTHORHOMBIC_FC 11
#define MONOCLINIC_PRIMITIVE 12
#define MONOCLINIC_BASE_CENTRED 13
#define TRICLINIC_PRIMITIVE 14

/* The number of possible point symmetries */
#define MAX_SYMMETRY	48

/******/






REAL get_te_ion_ion ();
REAL get_sum_eig (STATE * states);
REAL get_Exc (REAL * rho, REAL * rhocore);
void correct_res (STATE *, REAL *);
void mix_rho (REAL * rho_out, REAL * rho_in, REAL mix, int steps, int mode);
void get_state_to_proc (STATE * states);


#include "overlap.h"



/* different methods to update orbitals */
void kain (int step, int N, double *xm, double *fm, int NsavedSteps);
void pulay (int step, int N, double *xm, double *fm, int NsavedSteps,
        int preconditioning);
void sd (int step, int N, double *xm, double *fm);
void pulay_mixing (int size, double *rho, int NsavedSteps);
void charge_pulay (int step, int N, double *xm, double *fm, int NsavedSteps);
void pulay_kain (int step, int N, double *xm, double *fm, int NsavedSteps);

/*  Moving center stuff  */
void get_orbit_center (STATE *state, REAL * x, REAL * y, REAL * z);
void update_orbit_centers (STATE * states);
int if_update_centers (STATE * states);

void write_states_info (char *outfile, STATE * states);
void read_states_info (char *outfile, STATE * states);

double get_gamma (double *vtot, double small_eig);
void init_state_size (STATE * states);





#include "salloc.h"

#include "macros.h"
