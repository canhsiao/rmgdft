#include "TradeImages.h"
#include "FiniteDiff.h"
#include "common_prototypes.h"
#include "rmg_alloc.h"
#include "rmg_error.h"


template <typename RmgType>
void CPP_app_cir_driver (RmgType * a, RmgType * b, int dimx, int dimy, int dimz, int order)
{

    int sbasis;
    void *allocp;
    RmgType *rptr;
    TradeImages T;
    FiniteDiff FD;

    sbasis = (dimx + 4) * (dimy + 4) * (dimz + 4);
    my_malloc (allocp, sbasis + 64, double);
    rptr = (RmgType *)allocp;

    if(order == APP_CI_FOURTH) {
        T.trade_imagesx (a, rptr, dimx, dimy, dimz, 1, FULL_TRADE);
        FD.app_cir_fourth (rptr, b, dimx, dimy, dimz);
    }
    else if(order == APP_CI_SIXTH) {
        T.trade_imagesx (a, rptr, dimx, dimy, dimz, 2, FULL_TRADE);
        FD.app_cir_sixth (rptr, b, dimx, dimy, dimz);
    }
    else {
        rmg_error_handler (__FILE__, __LINE__, "APP_CIR order not programmed yet in CPP_app_cir_driver.\n");
    }

    my_free(rptr);
    return;

}

extern "C" void app_cir_driver (rmg_double_t * a, rmg_double_t * b, int dimx, int dimy, int dimz, int order)
{
    CPP_app_cir_driver<double>(a, b, dimx, dimy, dimz, order);
}
extern "C" void app_cir_driver_f (rmg_float_t * a, rmg_float_t * b, int dimx, int dimy, int dimz, int order)
{
    CPP_app_cir_driver<float>(a, b, dimx, dimy, dimz, order);
}
