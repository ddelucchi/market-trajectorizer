#include "mt/math/phase_torus.hpp"
#include "mt/math/combinatorics.hpp"
#include "mt/core/constants.hpp"
#include "mt/core/errors.hpp"

#include <cmath>

namespace mt {

namespace {
inline usize dense_size(int Q, int order_max) {
    usize n = 1;
    for (int q = 0; q < Q; ++q) n *= static_cast<usize>(order_max + 1);
    return n;
}
}  // namespace

ExtractedCoefficients extract_phase_torus_cpu(
    const FieldCpu& F,
    const Vec<real>& z_real, const Vec<real>& z_imag,
    real rho_m, int Q, int order_max, int M)
{
    if (rho_m <= 0.0) throw NumericError("extract_phase_torus_cpu: rho_m must be > 0");
    if (M     <  2 ) throw NumericError("extract_phase_torus_cpu: M must be >= 2");

    ExtractedCoefficients out;
    out.Q = Q; out.order_max = order_max; out.rho_m = rho_m;

    Vec<Vec<int>> idx; enumerate_multi_indices(Q, order_max, idx);
    auto layout = make_multi_index_layout(Q, order_max);
    out.A.assign(dense_size(Q, order_max), cplx{});

    auto torus = build_torus_quadrature(Q, M);
    usize nodes = 1;
    for (int q = 0; q < Q; ++q) nodes *= static_cast<usize>(M);
    const real norm = std::pow(constants::TWO_PI, static_cast<real>(Q));
    const real dvol = norm / static_cast<real>(nodes);

    Vec<real> zp_re(static_cast<usize>(Q));
    Vec<real> zp_im(static_cast<usize>(Q));

    for (const auto& alpha : idx) {
        cplx acc{};
        for (usize n = 0; n < nodes; ++n) {
            for (int q = 0; q < Q; ++q) {
                const real psi = torus.psi[n * static_cast<usize>(Q) + static_cast<usize>(q)];
                zp_re[static_cast<usize>(q)] = z_real[static_cast<usize>(q)] + rho_m * std::cos(psi);
                zp_im[static_cast<usize>(q)] = z_imag[static_cast<usize>(q)] + rho_m * std::sin(psi);
            }
            cplx Fv = F(zp_re, zp_im);
            real apsi = 0.0;
            for (int q = 0; q < Q; ++q)
                apsi += static_cast<real>(alpha[static_cast<usize>(q)])
                      * torus.psi[n * static_cast<usize>(Q) + static_cast<usize>(q)];
            acc += Fv * std::exp(cplx(0.0, -apsi));
        }
        out.A[flatten_multi_index(alpha, layout)] = acc * (dvol / norm);
    }
    return out;
}

// GPU path: deferred under "Path B - honest CPU-first".  The function is kept
// as a concrete-typed entry point that explicitly signals deferred status; it
// MUST NOT silently succeed.
void extract_phase_torus_gpu(const DeviceFieldView&, const DeviceVector&, real, int, int, int,
                             DeviceExtractedCoefficients&, cudaStream_t) {
    throw NumericError("extract_phase_torus_gpu: GPU path deferred; call extract_phase_torus_cpu");
}

ExtractedCoefficients extract_phase_torus_separable_cpu(
    const Vec<FieldCpu1D>& G,
    const Vec<real>&       z_real,
    const Vec<real>&       z_imag,
    real                   rho_m,
    int                    order_max,
    int                    M_per_dim)
{
    const int Q = static_cast<int>(G.size());
    if (Q <= 0)        throw NumericError("extract_phase_torus_separable_cpu: Q must be > 0");
    if (rho_m <= 0.0)  throw NumericError("extract_phase_torus_separable_cpu: rho_m must be > 0");
    if (M_per_dim < 2) throw NumericError("extract_phase_torus_separable_cpu: M must be >= 2");
    if (static_cast<int>(z_real.size()) != Q || static_cast<int>(z_imag.size()) != Q)
        throw NumericError("extract_phase_torus_separable_cpu: z_real/z_imag length must equal Q");

    ExtractedCoefficients out;
    out.Q = Q; out.order_max = order_max; out.rho_m = rho_m;
    out.A.assign(dense_size(Q, order_max), cplx{});

    // Per-axis 1D Cauchy ring quadrature.
    //     A_q[α] = (1/M) Σ_{m=0..M-1} G_q(z_q + ρ e^{iψ_m}) · e^{-i α ψ_m}
    // where ψ_m = 2π m / M.  Result is exact for analytic germs (trapezoid rule
    // on a closed loop is spectrally accurate).
    const real two_pi = 2.0 * std::acos(-1.0);
    Vec<Vec<cplx>> A_axis(static_cast<usize>(Q));
    for (int q = 0; q < Q; ++q) {
        Vec<cplx>& Aq = A_axis[static_cast<usize>(q)];
        Aq.assign(static_cast<usize>(order_max + 1), cplx{});
        const real x = z_real[static_cast<usize>(q)];
        const real y = z_imag[static_cast<usize>(q)];
        for (int m = 0; m < M_per_dim; ++m) {
            const real psi = two_pi * static_cast<real>(m) / static_cast<real>(M_per_dim);
            const cplx Fv = G[static_cast<usize>(q)](x + rho_m * std::cos(psi),
                                                     y + rho_m * std::sin(psi));
            for (int a = 0; a <= order_max; ++a)
                Aq[static_cast<usize>(a)] += Fv * std::exp(cplx(0.0, -static_cast<real>(a) * psi));
        }
        const real inv_M = 1.0 / static_cast<real>(M_per_dim);
        for (int a = 0; a <= order_max; ++a)
            Aq[static_cast<usize>(a)] *= inv_M;
    }

    // Assemble the full dense lattice.  For separable F = Σ_q G_q:
    //   - α = 0           : Σ_q G_q(z_q)  (constants sum)
    //   - α = α_q · e_q   : A_axis[q][α_q]
    //   - mixed support   : 0
    auto layout = make_multi_index_layout(Q, order_max);
    Vec<Vec<int>> idx;
    enumerate_multi_indices(Q, order_max, idx);

    for (const auto& alpha : idx) {
        int support_axis = -1;
        int support_count = 0;
        int support_order = 0;
        for (int q = 0; q < Q; ++q) {
            if (alpha[static_cast<usize>(q)] > 0) {
                support_axis  = q;
                support_order = alpha[static_cast<usize>(q)];
                if (++support_count > 1) break;
            }
        }
        const usize k = flatten_multi_index(alpha, layout);
        if (support_count == 0) {
            // α = 0: A_0 = Σ_q A_axis[q][0] = Σ_q (1/2π)∫G_q dψ = Σ_q G_q(z_q).
            cplx acc{};
            for (int q = 0; q < Q; ++q) acc += A_axis[static_cast<usize>(q)][0];
            out.A[k] = acc;
        } else if (support_count == 1) {
            out.A[k] = A_axis[static_cast<usize>(support_axis)][static_cast<usize>(support_order)];
        } else {
            out.A[k] = cplx{};   // mixed multi-indices vanish for separable F.
        }
    }
    return out;
}

}  // namespace mt
