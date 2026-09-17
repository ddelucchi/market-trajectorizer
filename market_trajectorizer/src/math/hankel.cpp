#include "mt/math/hankel.hpp"

#include <algorithm>
#include <cmath>

namespace mt {

HankelMatrix build_hankel(const Vec<cplx>& u, int N) {
    HankelMatrix H; H.N = N;
    const usize sz = static_cast<usize>(N + 1);
    H.a.assign(sz * sz, cplx{});
    for (usize i = 0; i <= static_cast<usize>(N); ++i)
        for (usize j = 0; j <= static_cast<usize>(N); ++j)
            H.a[i * sz + j] = (i + j < u.size()) ? u[i + j] : cplx{};
    return H;
}

namespace {

// Deterministic complex Gaussian elimination with partial pivoting on an mxm
// matrix; returns the rank within tolerance and (optionally) the upper
// triangular factor in `M` so callers can read pivots.  In-place.
int gaussian_rank_inplace(Vec<cplx>& M, int m, real tol) {
    int rank = 0;
    Vec<int> piv_row(static_cast<usize>(m), -1);
    for (int col = 0; col < m; ++col) {
        int pivot = -1;
        real best = 0.0;
        for (int row = rank; row < m; ++row) {
            real v = std::abs(M[static_cast<usize>(row) * static_cast<usize>(m) + static_cast<usize>(col)]);
            if (v > best) { best = v; pivot = row; }
        }
        if (pivot < 0 || best < tol) continue;
        // swap rows rank and pivot (lex order is deterministic given input)
        if (pivot != rank) {
            for (int c = 0; c < m; ++c) {
                std::swap(M[static_cast<usize>(rank)  * static_cast<usize>(m) + static_cast<usize>(c)],
                          M[static_cast<usize>(pivot) * static_cast<usize>(m) + static_cast<usize>(c)]);
            }
        }
        cplx d = M[static_cast<usize>(rank) * static_cast<usize>(m) + static_cast<usize>(col)];
        for (int row = rank + 1; row < m; ++row) {
            cplx f = M[static_cast<usize>(row) * static_cast<usize>(m) + static_cast<usize>(col)] / d;
            for (int c = col; c < m; ++c) {
                M[static_cast<usize>(row) * static_cast<usize>(m) + static_cast<usize>(c)] -=
                    f * M[static_cast<usize>(rank) * static_cast<usize>(m) + static_cast<usize>(c)];
            }
        }
        piv_row[static_cast<usize>(rank)] = col;
        ++rank;
    }
    return rank;
}

}  // namespace

// Smallest m >= 1 with rank(H_m) < m+1 (we test the (m+1)x(m+1) leading
// principal minor of H built from u_0..u_{2m}).  By the framework H_m has rank
// exactly r once m >= r-1; we report r = first m at which the minor drops.
int detect_min_rank(const Vec<cplx>& u, int N_max, real tol) {
    if (u.empty()) return 0;
    int Nmax = std::min<int>(N_max, static_cast<int>(u.size()) / 2);
    if (Nmax < 1) Nmax = 1;
    for (int m = 1; m <= Nmax; ++m) {
        const int dim = m + 1;
        Vec<cplx> M(static_cast<usize>(dim * dim), cplx{});
        for (int i = 0; i < dim; ++i)
            for (int j = 0; j < dim; ++j)
                M[static_cast<usize>(i * dim + j)] =
                    (static_cast<usize>(i + j) < u.size()) ? u[static_cast<usize>(i + j)] : cplx{};
        int r = gaussian_rank_inplace(M, dim, tol);
        if (r < dim) return m;       // first deficiency
    }
    return Nmax;
}

}  // namespace mt
