#pragma once
#include "mt/core/types.hpp"

namespace mt {

// Smooth Taylor-jet representation of F at z (over Q channels).
struct JetState {
    int Q          = 0;
    int order_max  = 0;
    Vec<cplx> coeff;       // J_α[F](z,κ), flattened in MultiIndexLayout order
};

struct DeviceJetState {
    int   Q         = 0;
    int   order_max = 0;
    cplx* coeff_d   = nullptr;
    usize n_coeff   = 0;
};

struct JetOperator {       // L_jet,κ; sparse-by-default
    Vec<cplx> matrix;      // dense for now (n_coeff x n_coeff)
    usize     dim = 0;
};

struct DeviceJetOperator {
    cplx* matrix_d = nullptr;
    usize dim      = 0;
};

}  // namespace mt
