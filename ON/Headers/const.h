/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 
/****f* QMD-MGDFT/const.h *****
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


/* Boolean definitions */
#define     TRUE        1
#define     FALSE       0



#define     PI          3.14159265359
#define		twoPI       6.28318530718
#define		threePI     9.42477796077
#define		fourPI     12.56637061436

/* Physical constants and conversion factors obtained from NIST
 *
 *  http://physics.nist.gov/cgi-bin/cuu/Value?angstar|search_for=all!
 *
 */

/* electron charge in Coulomb */
#define     e_C         1.60217646e-19

/* Planck constant in SI */
#define     h_SI        6.626068e-34

/* unit conversions */
/* the numbers with higher precision were obtained by '1/const' */

/* conversions between hartree and eV */
#define     Ha_eV      27.2113845
#define     eV_Ha       0.0367493245336341

/* conversions between angstrom and bohr */
#define     a0_A        0.5291772108
#define     A_a0         1.88972612499359


/* conversion factors for atomic masses */
#define     mu_me    1822.88847984055
#define     me_mu       0.000548579910981427





/* Angular momentum states */
#define     S_STATE     0
#define     P_STATE     1
#define     D_STATE     2
#define     F_STATE     3
#define     G_STATE     4
#define     PX_STATE    1
#define     PY_STATE    2
#define     PZ_STATE    3


/* Molecular dynamics flags */
#define      MD_QUENCH       0
#define	MD_FASTRLX	1
#define      MD_CVE          2
#define      MD_CVT          3
#define      MD_CPT          4  /* not working yet */
#define	CD_FASTRLX	5
#define	PLOT     	6
#define	PSIPLOT     	7
#define	BAND_STRUCTURE 	8


/* MD integration flags */
#define      ORDER_2         0
#define      ORDER_3         1
#define      ORDER_5         2

/* Temperature control flags */
#define      T_NOSE_CHAIN    0
#define      T_AND_SCALE     1

/* Boundary condition flags */
#define      PERIODIC        0
#define      CLUSTER         1
#define      SURFACE         2
#define      ZPERIODIC       3

/* Exchange-correlation flags */
#define       LDA_PZ81        0
#define       GGA_BLYP        1
#define       GGA_XB_CP       2
#define       GGA_XP_CP       3
#define       GGA_PBE         4

#define INIT_FIREBALL  2
#define INIT_GAUSSIAN  3

/* Neighbor list indices */
#define NB_N 0
#define NB_S 1
#define NB_E 2
#define NB_W 3
#define NB_U 4
#define NB_D 5
#define NB_SELF 7



/* Some stuff for timing and performance measurements */
#define TOTAL_TIME (0)
#define ORTHO_TIME (1)
#define NL_TIME (2)
#define MG_TIME (3)
#define IMAGE_TIME (4)
#define APPCIL_TIME (5)
#define APPCIR_TIME (6)
#define RESTRICT_TIME (7)
#define EXPAND_TIME (8)
#define PACK_TIME (9)
#define INIT_TIME (10)
#define HARTREE_TIME (11)
#define DIAG_TIME (12)
#define APPGRAD_TIME (15)
#define RHO_TIME (16)
#define MASK_TIME (17)
#define MATB_TIME (18)
#define PRECOND_TIME (19)
#define GET_TE_TIME (20)
#define VXC_TIME (21)
#define SCF_TIME (22)
#define SX_TIME (23)
#define RES_TIME (24)
#define ORTHONORM_TIME (25)
#define COND_S_TIME (26)
#define GET_ORBIT (27)
#define OVERLAP_TIME (28)
#define CHOLESKY_TIME (29)
#define PSSYEV_TIME (30)
#define PSSYGST_TIME (31)
#define PSTRTRS_TIME (32)
#define GATHER_TIME  (33)
#define LINE_MINTIME   (34)
#define ORBIT_DOT_ORBIT  (35)
#define ORBIT_DOT_ORBIT_H  (36)
#define DOT_PRODUCT (37)
#define GET_NEW_RHO (38)


#define MATDIAG_TIME (40)
#define UPDATEPOT_TIME  (41)
#define POTFC_TIME (42)
#define THETA_TIME (43)
#define QNMPSI_TIME (44)
#define NONRES_TIME (45)
#define HPSI_TIME (46)
#define DNMPSI_TIME (47)
#define MIXPSI_TIME (48)
#define HIJ_TIME (49)
#define INVMATB_TIME (50)
#define QUENCH_TIME (51)
#define RHO_PHI_TIME (52)
#define RHO_SUM_TIME (53)
#define RHO_CTOF_TIME (54)
#define RHO_AUG_TIME (55)
#define RHO_QNM_MAT (95)
#define matB_QNM (96)

#define FORCE_TIME (56)
#define IIFORCE_TIME (57)
#define LFORCE_TIME (13)
#define NLFORCE_TIME (14)
#define NLCCFORCE_TIME (58)
#define PARTIAL_KBPSI (59)
#define RHO_NM_MAT (60)
#define PARTIAL_MAT_NM (61)
#define NLFORCE_PAR_Q (62)
#define NLFORCE_PAR_RHO (63)
#define NLFORCE_PAR_OMEGA (64)


#define ALLOC_TIME (70)
#define READ_PSEUDO_TIME 71

#define LAST_TIME     100

/* Boolean definitions */
#define     TRUE        1
#define     FALSE       0

/* Symbolic numeric constants */
#define		ZERO        0.0
#define		ONE         1.0
#define		TWO         2.0
#define		THREE       3.0
#define		FOUR        4.0
#define		FIVE        5.0
#define		SIX         6.0
#define		SEVEN       7.0
#define		EIGHT       8.0
#define		NINE        9.0
#define		TEN        10.0


#define		PI          3.14159265359
#define		twoPI       6.28318530718
#define		threePI     9.42477796077
#define		fourPI     12.56637061436


/* Physical constants and conversion factors obtained from NIST
 *
 *  http://physics.nist.gov/cgi-bin/cuu/Value?angstar|search_for=all!
 *
 */

/* electron charge in Coulomb */
#define     e_C         1.60217646e-19

/* Planck constant in SI */
#define     h_SI        6.626068e-34

/* unit conversions */
/* the numbers with higher precision were obtained by '1/const' */

/* conversions between hartree and eV */
#define     Ha_eV      27.2113845
#define     eV_Ha       0.0367493245336341

/* conversions between angstrom and bohr */
#define     a0_A        0.5291772108
#define     A_a0         1.88972612499359


/* conversion factor for atomic masses */
#define     mu_me    1822.88847984055
#define     me_mu       0.000548579910981427






/* Angular momentum states */
#define		S_STATE     0
#define		P_STATE     1
#define		D_STATE     2
#define		F_STATE     3
#define		G_STATE     4
#define		PX_STATE    1
#define		PY_STATE    2
#define		PZ_STATE    3


/* Neighbor list indices */
#define NB_N 0
#define NB_S 1
#define NB_E 2
#define NB_W 3
#define NB_U 4
#define NB_D 5
#define NB_SELF 7

/* Molecular dynamics flags */
#define		MD_QUENCH       0
#define		MD_FASTRLX	1
#define		MD_CVE          2
#define		MD_CVT          3
#define		MD_CPT          4       /* not working yet */
#define		CD_FASTRLX	5
#define		PLOT     	6
#define		PSIPLOT     	7
#define		BAND_STRUCTURE 	8

/* MD integration flags */
#define		ORDER_2         0
#define		ORDER_3         1
#define		ORDER_5         2

/* Temperature control flags */
#define		T_NOSE_CHAIN    0
#define		T_AND_SCALE     1

/* Boundary condition flags */
#define		PERIODIC        0
#define		CLUSTER         1
#define		SURFACE         2

/* Exchange-correlation flags */
#define       	LDA_PZ81        0
#define       	GGA_BLYP        1
#define       	GGA_XB_CP       2
#define       	GGA_XP_CP       3
#define       	GGA_PBE         4

/* Types of finite differencing */
#define FULL_FD 1
#define CENTRAL_FD 2


/******/
