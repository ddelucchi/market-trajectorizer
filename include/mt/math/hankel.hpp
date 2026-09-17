#pragma once
#include "mt/core/types.hpp"

namespace mt {

struct HankelMatrix {
    int       N = 0;
    Vec<cplx> a;        // (N+1) x (N+1) row-major: H[i,j] = u[i+j]
};

HankelMatrix build_hankel(const Vec<cplx>& u, int N);

// Smallest m s.t. det H_m = 0  (rank deficiency = recurrence order r).
int detect_min_rank(const Vec<cplx>& u, int N_max, real tol);

}  // namespace mt
