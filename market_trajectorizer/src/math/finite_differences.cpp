#include "mt/math/finite_differences.hpp"
namespace mt {
void finite_backward_differences(const Vec<cplx>& x, int n, int K, Vec<cplx>& nabla_out) {
    nabla_out.assign(static_cast<usize>(K + 1), cplx{});
    // ∇^k x_n = Σ_{j=0}^{k} (-1)^j C(k,j) x_{n-j}
    Vec<cplx> tmp(static_cast<usize>(K + 1));
    for (int k = 0; k <= K; ++k) {
        cplx acc{};
        real sign = 1.0;
        real ck = 1.0;  // C(k,0)
        for (int j = 0; j <= k; ++j) {
            int idx = n - j;
            if (idx < 0 || idx >= static_cast<int>(x.size())) break;
            acc += sign * ck * x[static_cast<usize>(idx)];
            // update C(k,j+1) = C(k,j) * (k - j) / (j + 1)
            ck = ck * static_cast<real>(k - j) / static_cast<real>(j + 1);
            sign = -sign;
        }
        nabla_out[static_cast<usize>(k)] = acc;
    }
}
}
