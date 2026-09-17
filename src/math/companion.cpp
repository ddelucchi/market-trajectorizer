// Recurrence solve via Hankel system + companion matrix construction +
// horner-style companion-orbit propagation.
//
// Given u_0..u_{N-1} with finite-dimensional closure of order r, solve
//      H_{r-1} c = -v_r,        v_r = (u_r, u_{r+1}, ..., u_{2r-1})^T
// for c = (c_0, c_1, ..., c_{r-1})^T such that
//      u_{n+r} + c_{r-1} u_{n+r-1} + ... + c_0 u_n = 0.
//
// Companion matrix in row-major:
//      C[i, i+1] = 1            for i = 0..r-2
//      C[r-1, j] = -c_j         for j = 0..r-1.

#include "mt/math/companion.hpp"
#include "mt/core/errors.hpp"

#include <algorithm>
#include <cmath>

namespace mt {

namespace {

// Solve A x = b in place via partial-pivoted Gaussian elimination (deterministic).
// A is m x m row-major; b length m; on exit b holds x.
void solve_linear(Vec<cplx>& A, Vec<cplx>& b, int m) {
    for (int col = 0; col < m; ++col) {
        int pivot = col;
        real best = std::abs(A[static_cast<usize>(col * m + col)]);
        for (int row = col + 1; row < m; ++row) {
            real v = std::abs(A[static_cast<usize>(row * m + col)]);
            if (v > best) { best = v; pivot = row; }
        }
        if (best == 0.0) throw NumericError("solve_linear: singular Hankel system");
        if (pivot != col) {
            for (int c = 0; c < m; ++c) {
                std::swap(A[static_cast<usize>(col * m + c)], A[static_cast<usize>(pivot * m + c)]);
            }
            std::swap(b[static_cast<usize>(col)], b[static_cast<usize>(pivot)]);
        }
        cplx d = A[static_cast<usize>(col * m + col)];
        for (int row = col + 1; row < m; ++row) {
            cplx f = A[static_cast<usize>(row * m + col)] / d;
            for (int c = col; c < m; ++c)
                A[static_cast<usize>(row * m + c)] -= f * A[static_cast<usize>(col * m + c)];
            b[static_cast<usize>(row)] -= f * b[static_cast<usize>(col)];
        }
    }
    for (int row = m - 1; row >= 0; --row) {
        cplx s = b[static_cast<usize>(row)];
        for (int c = row + 1; c < m; ++c) s -= A[static_cast<usize>(row * m + c)] * b[static_cast<usize>(c)];
        b[static_cast<usize>(row)] = s / A[static_cast<usize>(row * m + row)];
    }
}

}  // namespace

RecurrenceCoefficients solve_recurrence_from_hankel(const Vec<cplx>& u, int r) {
    RecurrenceCoefficients rc; rc.r = r;
    rc.c.assign(static_cast<usize>(r), cplx{});
    if (r <= 0) return rc;
    if (static_cast<int>(u.size()) < 2 * r)
        throw NumericError("solve_recurrence_from_hankel: need at least 2r samples");

    // Build H_{r-1}  (rxr Hankel  [u_{i+j}], i,j=0..r-1) and v_r = (u_r..u_{2r-1}).
    Vec<cplx> A(static_cast<usize>(r * r), cplx{});
    Vec<cplx> b(static_cast<usize>(r), cplx{});
    for (int i = 0; i < r; ++i) {
        for (int j = 0; j < r; ++j)
            A[static_cast<usize>(i * r + j)] = u[static_cast<usize>(i + j)];
        b[static_cast<usize>(i)] = -u[static_cast<usize>(i + r)];
    }
    solve_linear(A, b, r);
    rc.c = b;
    return rc;
}

CompanionState build_companion_state(const RecurrenceCoefficients& rc, const Vec<cplx>& u_init) {
    CompanionState cs; cs.r = rc.r;
    const usize r = static_cast<usize>(rc.r);
    cs.C.assign(r * r, cplx{});
    for (usize i = 0; i + 1 < r; ++i) cs.C[i * r + (i + 1)] = cplx(1.0, 0.0);
    for (usize j = 0; j < r; ++j)     cs.C[(r - 1) * r + j] = -rc.c[j];
    cs.Y0.assign(r, cplx{});
    for (usize i = 0; i < r && i < u_init.size(); ++i) cs.Y0[i] = u_init[i];
    return cs;
}

cplx eval_companion(const CompanionState& cs, int h) {
    const usize r = static_cast<usize>(cs.r);
    if (r == 0) return cplx{};
    Vec<cplx> Y = cs.Y0;
    Vec<cplx> tmp(r);
    for (int step = 0; step < h; ++step) {
        for (usize i = 0; i < r; ++i) {
            cplx acc{};
            for (usize j = 0; j < r; ++j) acc += cs.C[i * r + j] * Y[j];
            tmp[i] = acc;
        }
        Y.swap(tmp);
    }
    return Y[0];
}

Vec<cplx> eval_companion(const CompanionState& cs, const HorizonGrid& hg) {
    Vec<cplx> out(hg.h.size());
    for (usize i = 0; i < hg.h.size(); ++i)
        out[i] = eval_companion(cs, static_cast<int>(hg.h[i]));
    return out;
}

}  // namespace mt
