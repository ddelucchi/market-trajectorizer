#include "mt/math/quadrature.hpp"
#include "mt/core/constants.hpp"

namespace mt {

TorusQuadrature build_torus_quadrature(int Q, int M) {
    TorusQuadrature tq; tq.Q = Q; tq.M_per_dim = M;
    // Tensor-product grid on [0,2π)^Q with M nodes per dim, fixed lexicographic order.
    usize total = 1;
    for (int q = 0; q < Q; ++q) total *= static_cast<usize>(M);
    tq.psi.assign(total * static_cast<usize>(Q), 0.0);
    Vec<int> idx(static_cast<usize>(Q), 0);
    const real h = constants::TWO_PI / static_cast<real>(M);
    for (usize n = 0; n < total; ++n) {
        for (int q = 0; q < Q; ++q)
            tq.psi[n * static_cast<usize>(Q) + static_cast<usize>(q)]
                = static_cast<real>(idx[static_cast<usize>(q)]) * h;
        // odometer increment
        for (int q = Q - 1; q >= 0; --q) {
            if (++idx[static_cast<usize>(q)] < M) break;
            idx[static_cast<usize>(q)] = 0;
        }
    }
    return tq;
}

}  // namespace mt
