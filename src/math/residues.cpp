// Residue extraction at z = Λ_j^{-1}.
//
// γ_{j, μ_j-1} = Λ_j^{μ_j-1} · lim_{z -> Λ_j^{-1}} (1 - Λ_j z)^{μ_j} G(z),
// γ_{j, μ_j-1-k} obtained by successive subtraction of the tail and re-evaluation.

#include "mt/math/residues.hpp"
#include "mt/math/polynomial.hpp"

#include <cmath>

namespace mt {

namespace {

cplx eval_G(const RationalModel& rm, cplx z) {
    return poly_eval(rm.P, z) / poly_eval(rm.Q, z);
}

}  // namespace

Vec<cplx> compute_residue_coefficients(const RationalModel& rm,
                                       cplx lambda_j, int mu_j)
{
    Vec<cplx> out(static_cast<usize>(mu_j), cplx{});
    if (lambda_j == cplx{} || mu_j <= 0) return out;
    const cplx zj = cplx(1.0, 0.0) / lambda_j;

    // gamma_{j,mu-1}: limit via L'Hopital / direct evaluation of P(z) * (1-Λz)^{μ_j} / Q(z).
    // Numerically: evaluate at z slightly off zj is unstable; we use the closed form
    //      γ_{j,μ-1} = Λ^{μ-1} · P(zj) / Q^{(μ)}(zj)/μ! · (-Λ)^{μ}
    // by using the local Laurent expansion: G(z) = N(z) / [(1 - Λ z)^μ · Q_rest(z)].
    // For a deterministic, robust path we use a numerical residue via small-circle
    // Cauchy integral around zj, with M=64 nodes radius r = 1e-3 / max(1,|λ|).
    const int M = 64;
    const real radius = 1e-3 / std::max(1.0, std::abs(lambda_j));
    const real two_pi = 6.28318530717958647692;

    // Compute residue of (1-Λz)^{μ-1-k} G(z) at z = zj for k = 0..μ-1, divided by
    // (-Λ)^{μ-1-k} * (μ-1-k)!  to recover γ_{j,μ-1-k}.  For k=0 this is the leading.
    // Strategy: build h(z) = G(z) · (1 - Λz)^μ; compute h^{(p)}(zj) by Cauchy:
    //   h^{(p)}(zj) = p!/(2πi) ∮ h(z)/(z-zj)^{p+1} dz.
    // Then γ_{j, μ-1-p} = (-Λ)^{ -(p+something)} ...  We use a simpler equivalent:
    // sample the Laurent partial sum coefficients γ_{j,s} so that
    //   Σ_{s=0..μ-1} γ_{j,s} z^s / (1 - Λz)^{s+1}  ≈ G(z) near z=zj.
    // Equivalent linear system on M sample points around zj:
    cplx Lambda = lambda_j;
    Vec<cplx> samples(static_cast<usize>(M));
    Vec<cplx> Gvals(static_cast<usize>(M));
    for (int n = 0; n < M; ++n) {
        real th = two_pi * (static_cast<real>(n) + 0.5) / static_cast<real>(M);
        cplx z = zj + radius * cplx(std::cos(th), std::sin(th));
        samples[static_cast<usize>(n)] = z;
        Gvals  [static_cast<usize>(n)] = eval_G(rm, z);
    }
    // Solve LS: A[n,s] = z_n^s / (1 - Λ z_n)^{s+1}; rhs = Gvals.  Use the normal
    // equations on a small mu_j x mu_j Hermitian system.
    const int K = mu_j;
    Vec<cplx> Anormal(static_cast<usize>(K * K), cplx{});
    Vec<cplx> bnormal(static_cast<usize>(K), cplx{});
    Vec<cplx> Arow(static_cast<usize>(K));
    for (int n = 0; n < M; ++n) {
        cplx z = samples[static_cast<usize>(n)];
        cplx denom = cplx(1.0, 0.0) - Lambda * z;
        cplx zs = cplx(1.0, 0.0);
        cplx ds = denom;            // (1-Λz)^{s+1} starts at s=0 -> denom^1
        for (int s = 0; s < K; ++s) {
            Arow[static_cast<usize>(s)] = zs / ds;
            zs *= z;
            ds *= denom;
        }
        for (int i = 0; i < K; ++i) {
            for (int j = 0; j < K; ++j)
                Anormal[static_cast<usize>(i * K + j)] += std::conj(Arow[static_cast<usize>(i)]) * Arow[static_cast<usize>(j)];
            bnormal[static_cast<usize>(i)] += std::conj(Arow[static_cast<usize>(i)]) * Gvals[static_cast<usize>(n)];
        }
    }
    // Solve K x K (deterministic Gaussian elim).
    for (int col = 0; col < K; ++col) {
        int pivot = col; real best = std::abs(Anormal[static_cast<usize>(col * K + col)]);
        for (int row = col + 1; row < K; ++row) {
            real v = std::abs(Anormal[static_cast<usize>(row * K + col)]);
            if (v > best) { best = v; pivot = row; }
        }
        if (best == 0.0) return out;
        if (pivot != col) {
            for (int c = 0; c < K; ++c)
                std::swap(Anormal[static_cast<usize>(col * K + c)], Anormal[static_cast<usize>(pivot * K + c)]);
            std::swap(bnormal[static_cast<usize>(col)], bnormal[static_cast<usize>(pivot)]);
        }
        cplx d = Anormal[static_cast<usize>(col * K + col)];
        for (int row = col + 1; row < K; ++row) {
            cplx f = Anormal[static_cast<usize>(row * K + col)] / d;
            for (int c = col; c < K; ++c)
                Anormal[static_cast<usize>(row * K + c)] -= f * Anormal[static_cast<usize>(col * K + c)];
            bnormal[static_cast<usize>(row)] -= f * bnormal[static_cast<usize>(col)];
        }
    }
    for (int row = K - 1; row >= 0; --row) {
        cplx s = bnormal[static_cast<usize>(row)];
        for (int c = row + 1; c < K; ++c)
            s -= Anormal[static_cast<usize>(row * K + c)] * bnormal[static_cast<usize>(c)];
        bnormal[static_cast<usize>(row)] = s / Anormal[static_cast<usize>(row * K + row)];
    }
    return bnormal;
}

}  // namespace mt
