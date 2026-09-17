#pragma once
#include "mt/core/types.hpp"

namespace mt {

// A_F_α^(m)(z) for fixed z, multi-index α with |α| <= order_max.
struct ExtractedCoefficients {
    int       Q          = 0;
    int       order_max  = 0;
    real      rho_m      = 0.0;
    Vec<cplx> A;          // flattened by MultiIndexLayout
};

struct DeviceExtractedCoefficients {
    int   Q          = 0;
    int   order_max  = 0;
    real  rho_m      = 0.0;
    cplx* A_d        = nullptr;
    usize n_coeff    = 0;
};

}  // namespace mt
