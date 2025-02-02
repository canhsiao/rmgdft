/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 

#ifndef LocalObject_H
#define LocalObject_H 1

//  localized objects, such orbitals, projectors, or atomic wavefunctions, can distributed
//  in two methods. One way is to project the localized object onto the whole real space 
//  with processor domain decomposition. Another way is that localized objects are distributed
// over proc.

#include <vector>
#include <iterator>
#include "BaseGrid.h"

template <typename KpointType> class LocalObject {

public:
//    LocalObject(int num_objects, double *center_crd, double *radius, 
//                BaseGrid *Rmg_G, int coarse_fine, MPI_Comm comm);
    LocalObject(int num_objects, int *ixmin, int *iymin, int *izmin, 
                int *dimx, int *dimy, int *dimz, bool delocalized,
                BaseGrid &Rmg_G, int density, MPI_Comm comm);
    ~LocalObject(void);

    KpointType *storage_proj;
    char *mask;

    int num_thispe;
    int num_tot;
    int density;
    int *index_proj_to_global;
    int *index_global_to_proj;
    bool delocalized;
    MPI_Comm comm;

    // Type LOCALIZED or DELOCALIZED
    int type;
    void ReadOrbitals(std::string filename, BaseGrid &Rmg_G);
    void WriteOrbitals(std::string filename, BaseGrid &Rmg_G);
    void ReadProjectors(int num_ions, int max_nlpoint, int *num_proj_perion, BaseGrid &Rmg_G);
    void GetAtomicOrbitals(int num_ions, BaseGrid &Rmg_G);
    void SetZeroBoundary(BaseGrid &Rmg_G, int multi_grid_level, int fd_order);
    void ReAssign(BaseGrid &BG);



private:
    int *ixmin, *iymin, *izmin, *dimx, *dimy, *dimz;

};

extern LocalObject<double> *LocalOrbital;
extern LocalObject<double> *H_LocalOrbital;
extern LocalObject<double> *LocalProj;
extern LocalObject<double> *LocalAtomicOrbital;

#endif
