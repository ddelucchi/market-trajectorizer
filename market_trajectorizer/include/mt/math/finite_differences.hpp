#pragma once
#include "mt/core/types.hpp"

namespace mt {

// ∇^k x_n, backward differences. Output sized to K+1.
void finite_backward_differences(const Vec<cplx>& x, int n, int K, Vec<cplx>& nabla_out);

// Convenience: ∇^0..∇^K evaluated at the last sample (n = u.size()-1).
inline Vec<cplx> nabla_k(const Vec<cplx>& u, int K) {
    Vec<cplx> out;
    if (u.empty()) return out;
    finite_backward_differences(u, static_cast<int>(u.size()) - 1, K, out);
    return out;
}

}  // namespace mt
