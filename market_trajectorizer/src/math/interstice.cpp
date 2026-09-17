// Interstice transport along a deterministic parameter line s ∈ [0, S].
//
// Differential law (real-axis specialization with imaginary unit u stored as scalar):
//      δ'(s) = u κ(s) δ(s),     γ'(s) = δ(s),     δ(0)=1, γ(0)=0.
//
// Discrete deterministic integrator (fixed step ∆s):
//      δ_{j+1} = δ_j · exp(u κ_j ∆s),
//      γ_{j+1} = γ_j +     δ_j ∆s,
//      θ_{j+1} = θ_j +       κ_j ∆s.
//
// κ-profile: in the absence of a learned curvature signal we use the
// deterministic constant κ(s) ≡ u_imag (named after the framework's
// imaginary-unit convention).  Callers may swap by writing into dyn.kappa
// before propagation.
//
// Interstice field (theorem object):
//      Λ_j(s) = λ_j^{δ(s)},
//      T_t^{int}(s,h) = Σ_j Σ_{m=0}^{μ_j-1} Γ̃_{j,m}(t,s) · C(h,m) · Λ_j(s)^{h-m}.
//
// We use Γ̃_{j,m}(t,s) = γ_{j,m} (i.e. transported in the spectrum but not
// yet remixed in m); this matches the leading framework formula and is
// exactly correct when κ(s)=0 and asymptotically correct for small κ.

#include "mt/math/interstice.hpp"
#include "mt/math/combinatorics.hpp"

#include <algorithm>
#include <cmath>

namespace mt {

IntersticeDynamics build_interstice_dynamics(int s_count, real ds, real u_imag) {
    IntersticeDynamics dyn;
    if (s_count <= 0) return dyn;
    dyn.s_grid.assign(static_cast<usize>(s_count), 0.0);
    dyn.theta .assign(static_cast<usize>(s_count), 0.0);
    dyn.kappa .assign(static_cast<usize>(s_count), 0.0);
    dyn.delta .assign(static_cast<usize>(s_count), 1.0);
    dyn.gamma_path.assign(static_cast<usize>(s_count), 0.0);

    const real base_kappa = std::max<real>(std::abs(u_imag), 0.25);
    for (int i = 0; i < s_count; ++i) {
        dyn.s_grid[static_cast<usize>(i)] = static_cast<real>(i) * ds;
        const real undulation = 1.0 + 0.15 * std::sin(0.5 * static_cast<real>(i));
        dyn.kappa[static_cast<usize>(i)] = base_kappa * undulation;
    }

    // Forward Euler on (γ, δ, θ) with the multiplicative δ update.
    for (int j = 0; j + 1 < s_count; ++j) {
        real dlt = dyn.delta[static_cast<usize>(j)];
        real kap = dyn.kappa[static_cast<usize>(j)];
        // δ' = u κ δ   ->  δ_{j+1} = δ_j · exp(u_imag κ ∆s)   (real-axis run: u≡real).
        dyn.delta[static_cast<usize>(j + 1)]      = dlt * std::exp(kap * ds);
        dyn.gamma_path[static_cast<usize>(j + 1)] = dyn.gamma_path[static_cast<usize>(j)] + dlt * ds;
        dyn.theta[static_cast<usize>(j + 1)]      = dyn.theta[static_cast<usize>(j)]      + kap * ds;
    }
    return dyn;
}

Vec<cplx> eval_interstice_field(const SpectralDecomposition& sd,
                                const IntersticeDynamics& dyn,
                                int s_index,
                                const HorizonGrid& hg)
{
    Vec<cplx> out(hg.h.size(), cplx{});
    if (s_index < 0 || s_index >= static_cast<int>(dyn.delta.size())) return out;
    const real delta_s = dyn.delta[static_cast<usize>(s_index)];
    for (usize i = 0; i < hg.h.size(); ++i) {
        const real h = hg.h[i];
        cplx acc{};
        for (const auto& m : sd.modes) {
            cplx Lambda = std::pow(m.lambda, delta_s);   // Λ_j(s) = λ_j^{δ(s)}
            for (int s = 0; s < m.multiplicity; ++s) {
                acc += m.gamma[static_cast<usize>(s)]
                     * binom_real(h, s)
                     * std::pow(Lambda, h - static_cast<real>(s));
            }
        }
        out[i] = acc;
    }
    return out;
}

IntersticeTrajectory build_interstice_trajectory(const SpectralDecomposition& sd,
                                                 const IntersticeDynamics& dyn,
                                                 int s_index,
                                                 const HorizonGrid& hg)
{
    IntersticeTrajectory tr;
    tr.s_count = static_cast<int>(dyn.s_grid.size());
    tr.H       = static_cast<int>(hg.h.size());
    tr.s_grid  = dyn.s_grid;
    tr.h_grid  = hg.h;
    tr.kappa   = dyn.kappa;
    tr.ds      = (dyn.s_grid.size() >= 2) ? (dyn.s_grid[1] - dyn.s_grid[0]) : 0.0;

    if (tr.s_count <= 0 || tr.H <= 0) return tr;

    tr.T_grid.assign(static_cast<usize>(tr.s_count) * static_cast<usize>(tr.H), cplx{});
    for (int si = 0; si < tr.s_count; ++si) {
        Vec<cplx> row = eval_interstice_field(sd, dyn, si, hg);
        for (int hi = 0; hi < tr.H; ++hi) {
            tr.T_grid[static_cast<usize>(si) * static_cast<usize>(tr.H) + static_cast<usize>(hi)] =
                row[static_cast<usize>(hi)];
        }
    }

    const int s_sel = std::clamp(s_index, 0, tr.s_count - 1);
    tr.T.assign(static_cast<usize>(tr.H), cplx{});
    for (int hi = 0; hi < tr.H; ++hi) {
        tr.T[static_cast<usize>(hi)] =
            tr.T_grid[static_cast<usize>(s_sel) * static_cast<usize>(tr.H) + static_cast<usize>(hi)];
    }

    tr.L.resize(tr.T.size());
    tr.P.resize(tr.T.size());
    tr.R.resize(tr.T.size());
    tr.Risk.assign(tr.T.size(), 0.0);
    cplx L0 = tr.T.empty() ? cplx{} : tr.T[0];
    for (usize i = 0; i < tr.T.size(); ++i) {
        tr.L[i] = tr.T[i];                       // Π_3 projection (price-axis)
        tr.P[i] = std::exp(tr.L[i].real());      // exp(Re L)
        tr.R[i] = tr.L[i] - L0;                  // L(s,h) - L(s,0)
        tr.Risk[i] = std::norm(tr.R[i]);
    }
    return tr;
}

}  // namespace mt
