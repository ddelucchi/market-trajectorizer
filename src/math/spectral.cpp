// Spectral factorization of the rational model and mode evaluation.
//
// Q(z) = Π_j (1 - λ_j z)^{μ_j}.  Roots of Q in z are 1/λ_j; we recover the
// λ_j by reversing Q to monomial form q_r(λ) = Σ Q[r-k] λ^k and root-finding,
// then cluster duplicates to multiplicities by tolerance.
//
// γ_{j,s} are determined so that
//      u_h = Σ_j Σ_{s=0}^{μ_j-1} γ_{j,s} C(h,s) λ_j^{h-s}
// matches u_0..u_{N-1} (linear least squares with N >= Σ μ_j; we use exact
// solve with N = Σ μ_j).

#include "mt/math/spectral.hpp"
#include "mt/math/combinatorics.hpp"
#include "mt/math/complex_roots.hpp"
#include "mt/math/resolvent.hpp"
#include "mt/core/errors.hpp"

#include <algorithm>
#include <cmath>

namespace mt {

namespace {

// Solve A x = b in place via partial-pivoted Gaussian elimination.
void solve_linear(Vec<cplx>& A, Vec<cplx>& b, int m) {
    for (int col = 0; col < m; ++col) {
        int pivot = col; real best = std::abs(A[static_cast<usize>(col * m + col)]);
        for (int row = col + 1; row < m; ++row) {
            real v = std::abs(A[static_cast<usize>(row * m + col)]);
            if (v > best) { best = v; pivot = row; }
        }
        if (best == 0.0) throw NumericError("spectral solve: singular Vandermonde-like system");
        if (pivot != col) {
            for (int c = 0; c < m; ++c)
                std::swap(A[static_cast<usize>(col * m + c)], A[static_cast<usize>(pivot * m + c)]);
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

// Cluster λ values into (lambda, multiplicity) pairs by absolute tolerance.
struct LambdaCluster { cplx lambda; int mult; };
Vec<LambdaCluster> cluster_lambdas(const Vec<cplx>& roots, real tol) {
    Vec<LambdaCluster> out;
    Vec<int> used(roots.size(), 0);
    for (usize i = 0; i < roots.size(); ++i) {
        if (used[i]) continue;
        cplx sum = roots[i]; int cnt = 1; used[i] = 1;
        for (usize j = i + 1; j < roots.size(); ++j) {
            if (!used[j] && std::abs(roots[j] - roots[i]) < tol) {
                sum += roots[j]; ++cnt; used[j] = 1;
            }
        }
        out.push_back({sum / static_cast<real>(cnt), cnt});
    }
    return out;
}

}  // namespace

SpectralDecomposition factor_rational_model(const RationalModel& rm, real tol) {
    SpectralDecomposition sd;
    const int r = static_cast<int>(rm.Q.size()) - 1;
    sd.r = r;
    if (r <= 0) return sd;

    // q_r(λ) = z^r Q(1/z) reversed: coefficient of λ^k is Q[r - k].
    // In low-to-high order for find_polynomial_roots: c[k] = Q[r-k].
    Vec<cplx> q_lambda(static_cast<usize>(r + 1));
    for (int k = 0; k <= r; ++k) q_lambda[static_cast<usize>(k)] = rm.Q[static_cast<usize>(r - k)];

    Vec<cplx> roots = find_polynomial_roots(q_lambda, tol, 200);
    real cluster_tol = std::max(tol * 100.0, 1e-7);
    auto clusters = cluster_lambdas(roots, cluster_tol);

    int sum_mu = 0;
    for (const auto& cl : clusters) sum_mu += cl.mult;
    if (sum_mu != r) {
        // Multiplicities did not sum to r; treat all as simple roots in a fallback.
        clusters.clear();
        for (const auto& z : roots) clusters.push_back({z, 1});
        sum_mu = static_cast<int>(roots.size());
        if (sum_mu == 0) return sd;
    }

    // Assemble γ-coefficients via linear solve:
    //      Σ_j Σ_{s=0}^{μ_j-1} γ_{j,s} · C(h,s) · λ_j^{h-s}  =  u_h, h = 0..sum_mu-1
    // For that we need u_0..u_{sum_mu-1}; recover from rational long-division.
    Vec<cplx> u_seed = eval_rational_coeffs(rm, sum_mu - 1);
    const int M = sum_mu;
    Vec<cplx> A(static_cast<usize>(M * M), cplx{});
    Vec<cplx> b(static_cast<usize>(M), cplx{});
    for (int h = 0; h < M; ++h) {
        b[static_cast<usize>(h)] = u_seed[static_cast<usize>(h)];
        int col = 0;
        for (const auto& cl : clusters) {
            for (int s = 0; s < cl.mult; ++s) {
                cplx w = binom_real(static_cast<real>(h), s)
                       * std::pow(cl.lambda, static_cast<real>(h - s));
                A[static_cast<usize>(h * M + col)] = w;
                ++col;
            }
        }
    }
    solve_linear(A, b, M);

    int col = 0;
    for (const auto& cl : clusters) {
        SpectralMode m; m.lambda = cl.lambda; m.multiplicity = cl.mult;
        m.gamma.assign(static_cast<usize>(cl.mult), cplx{});
        for (int s = 0; s < cl.mult; ++s) m.gamma[static_cast<usize>(s)] = b[static_cast<usize>(col++)];
        sd.modes.push_back(m);
    }
    return sd;
}

Vec<cplx> eval_spectral_modes(const SpectralDecomposition& sd, int H) {
    Vec<cplx> u(static_cast<usize>(H + 1), cplx{});
    for (int h = 0; h <= H; ++h) {
        cplx acc{};
        for (const auto& m : sd.modes) {
            for (int s = 0; s < m.multiplicity; ++s) {
                cplx pw = std::pow(m.lambda, static_cast<real>(h - s));
                acc += m.gamma[static_cast<usize>(s)] * binom_real(static_cast<real>(h), s) * pw;
            }
        }
        u[static_cast<usize>(h)] = acc;
    }
    return u;
}

Vec<cplx> eval_spectral_modes(const SpectralDecomposition& sd, const HorizonGrid& hg) {
    Vec<cplx> out(hg.h.size(), cplx{});
    for (usize i = 0; i < hg.h.size(); ++i) {
        const real h = hg.h[i];
        cplx acc{};
        for (const auto& m : sd.modes) {
            for (int s = 0; s < m.multiplicity; ++s)
                acc += m.gamma[static_cast<usize>(s)]
                     * binom_real(h, s)
                     * std::pow(m.lambda, h - static_cast<real>(s));
        }
        out[i] = acc;
    }
    return out;
}

}  // namespace mt
