#pragma once
#include "mt/core/types.hpp"
#include <functional>

namespace mt {

// Uniform tensor-product trapezoidal/midpoint quadrature on [0,2π]^Q.
// Used by phase-torus extraction; deterministic node order.
struct TorusQuadrature {
    int      Q = 0;
    int      M_per_dim = 0;
    Vec<real> psi;     // (M^Q) x Q row-major
};

TorusQuadrature build_torus_quadrature(int Q, int M_per_dim);

}  // namespace mt
