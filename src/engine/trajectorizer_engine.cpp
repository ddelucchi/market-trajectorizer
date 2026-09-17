#include "mt/engine/trajectorizer_engine.hpp"
#include "mt/engine/extraction_engine.hpp"   // evaluate_future_section
#include "mt/math/finite_differences.hpp"
#include "mt/math/companion.hpp"
#include "mt/math/newton_gregory.hpp"
#include "mt/math/resolvent.hpp"
#include "mt/math/spectral.hpp"
#include "mt/math/semigroup.hpp"

#include <algorithm>
#include <cmath>

namespace mt {

namespace {

real diff_max(const Vec<cplx>& a, const Vec<cplx>& b) {
    const usize n = std::min(a.size(), b.size());
    real m = 0.0;
    for (usize i = 0; i < n; ++i) m = std::max(m, std::abs(a[i] - b[i]));
    return m;
}

// Theorem-faithful Newton carrier (math_contract clause N_h[b]_n).
//
// Input: u_past[j] = u_h evaluated at past offset h = -j, j = 0..K-1, where
// u_past[0] = U(0) is the anchor sample.  The Newton-Gregory backward formula
//
//      U(h)  =  Σ_{k=0..K-1}  β_k(h) · ∇^k u_0
//
// requires ∇^k u_0 = Σ_j (-1)^j C(k,j) u_{-j}.  We expose this by reversing
// u_past so that rev[K-1] = u_past[0] = U(0) and rev[K-1-j] = u_past[j] =
// U(-j); ∇^k rev_{K-1} = Σ_j (-1)^j C(k,j) rev_{K-1-j} = Σ_j (-1)^j C(k,j)
// U(-j) = ∇^k u_0.  We then evaluate at η := h on the horizon grid (NOT at the
// shifted η = h - (K-1)).  This is the carrier required by the theorem path.
Vec<cplx> eval_newton_at_h(const Vec<cplx>&   u_past,
                           int                K,
                           int                r_recurrence,
                           const Vec<real>&   h_grid,
                           EvolutionEquality& ee) {
    const int K_avail = static_cast<int>(u_past.size());
    if (K_avail <= 0 || K <= 0) {
        ee.newton_input_sequence.clear();
        ee.newton_backward_differences.clear();
        ee.newton_eta_grid.assign(h_grid.size(), 0.0);
        ee.newton_basis_values.assign(h_grid.size(), Vec<real>{});
        ee.newton_partial_sums.assign(h_grid.size(), Vec<cplx>{});
        return Vec<cplx>(h_grid.size(), cplx{});
    }

    // Cap order at recurrence-implied order (memory: K <= r+1 avoids high-order
    // divergence on real data) when a finite recurrence is available.
    int K_use = std::max(1, std::min(K, K_avail));
    if (r_recurrence > 0) K_use = std::min(K_use, r_recurrence + 1);

    // rev[i] = u_past[K_use-1-i] : rev now indexed in chronological order
    // (deepest past at rev[0], anchor at rev[K_use-1]).
    Vec<cplx> rev(static_cast<usize>(K_use), cplx{});
    for (int i = 0; i < K_use; ++i)
        rev[static_cast<usize>(i)] = u_past[static_cast<usize>(K_use - 1 - i)];

    // ∇^k rev_{K_use-1} = ∇^k u_0 of the underlying carrier (anchored at h=0).
    Vec<cplx> nabla;
    finite_backward_differences(rev, K_use - 1, K_use - 1, nabla);

    ee.newton_input_sequence       = rev;
    ee.newton_backward_differences = nabla;
    ee.newton_eta_grid.assign(h_grid.size(), 0.0);
    ee.newton_basis_values.assign(h_grid.size(), Vec<real>(static_cast<usize>(K_use), 0.0));
    ee.newton_partial_sums.assign(h_grid.size(), Vec<cplx>(static_cast<usize>(K_use), cplx{}));

    Vec<cplx> out(h_grid.size(), cplx{});
    for (usize i = 0; i < h_grid.size(); ++i) {
        const real eta = h_grid[i];     // theorem path: η := h
        ee.newton_eta_grid[i] = eta;
        cplx acc{};
        for (int k = 0; k < K_use; ++k) {
            const real beta = beta_k(k, eta);
            ee.newton_basis_values[i][static_cast<usize>(k)] = beta;
            acc += beta * nabla[static_cast<usize>(k)];
            ee.newton_partial_sums[i][static_cast<usize>(k)] = acc;
        }
        out[i] = acc;
    }
    return out;
}

// Legacy shifted-basis evaluator preserved as a strictly-debugging helper for
// change-of-basis comparisons.  NEVER USE THIS AS THE THEOREM CARRIER.
[[maybe_unused]]
Vec<cplx> eval_newton_change_of_basis_debug(const Vec<cplx>& x,
                                            int              K,
                                            const Vec<real>& h_grid) {
    if (x.empty() || K <= 0) return Vec<cplx>(h_grid.size(), cplx{});
    const int K_use = std::max(1, std::min(K, static_cast<int>(x.size())));
    const int n_anchor = K_use - 1;
    Vec<cplx> seq(K_use, cplx{});
    for (int i = 0; i < K_use; ++i) seq[static_cast<usize>(i)] = x[static_cast<usize>(i)];
    Vec<cplx> nabla;
    finite_backward_differences(seq, n_anchor, K_use - 1, nabla);
    Vec<cplx> out(h_grid.size(), cplx{});
    for (usize i = 0; i < h_grid.size(); ++i) {
        const real eta = h_grid[i] - static_cast<real>(n_anchor);
        cplx acc{};
        for (int k = 0; k < K_use; ++k)
            acc += beta_k(k, eta) * nabla[static_cast<usize>(k)];
        out[i] = acc;
    }
    return out;
}

}  // namespace

EvolutionEquality compute_evolution_equality(const FutureSection&         jet_minus,
                                             const JetState&              normalized_jet,
                                             const RecurrencePipelineOut& rp,
                                             const Vec<cplx>&             u_history,
                                             const HorizonGrid&           hg)
{
    EvolutionEquality ee;

    // Theorem-path invariants: horizon grid must be anchored at h=0 so that the
    // four carriers U_continuous, U_jet, U_newton, U_matrix all share a common
    // origin sample.  A non-anchored grid silently invalidates the equality
    // identity, so we fail closed by returning an empty equality object.
    if (hg.h.empty() || hg.h.front() != 0.0) {
        ee.U_continuous.assign(hg.h.size(), cplx{});
        ee.U_jet.assign(hg.h.size(), cplx{});
        ee.U_newton.assign(hg.h.size(), cplx{});
        ee.U_matrix.assign(hg.h.size(), cplx{});
        ee.theorem_residual_on_phase_torus_path = false;
        return ee;
    }

    // ----- E_h[F]_n  : continuous formation off the SMOOTH J_t^- ----------
    // Strict ledger 13.6: the four-way identity E_h = J_h = N_h = M_h is an
    // identity on the smooth analytic continuation / smooth-jet shadow, NOT
    // on the packet-augmented future section.  Mixing A_pi packets here
    // while U_jet is computed from the smooth normalized jet alone would
    // make err_continuous_jet nonzero by construction whenever any boundary
    // is active.  Packet/interstice contributions live in the trajectory /
    // readout / risk layer, not in this identity.
    ee.U_continuous.resize(hg.h.size());
    for (usize i = 0; i < hg.h.size(); ++i)
        ee.U_continuous[i] = evaluate_smooth_future_section(jet_minus, hg.h[i]);

    // ----- J_h[F]_n  : jet semigroup on normalized extracted jet ------------
    JetOperator L{};   // canonical translation operator on the truncated jet lattice
    ee.U_jet.resize(hg.h.size());
    ee.jet_semigroup_from_normalized_jet = !normalized_jet.coeff.empty();
    if (ee.jet_semigroup_from_normalized_jet) {
        for (usize i = 0; i < hg.h.size(); ++i) {
            JetState Jh = shift_jet_semigroup_cpu(normalized_jet, hg.h[i], L);
            ee.U_jet[i] = Jh.coeff.empty() ? cplx{} : Jh.coeff.front();
        }
    } else {
        std::fill(ee.U_jet.begin(), ee.U_jet.end(), cplx{});
    }

    // ----- N_h[b]_n  : Newton-Gregory  (theorem path: η := h) ----------------
    // u_history MUST be the past anchor samples u_past[j] = U(-j), j = 0..K-1.
    // The pipeline now feeds this directly; the carrier is the same anchor
    // object F_t[J_t^-] sampled at past offsets.
    int K_newton = static_cast<int>(u_history.size());
    ee.U_newton = eval_newton_at_h(u_history,
                                   K_newton,
                                   rp.rec.r,
                                   hg.h,
                                   ee);

    // ----- M_h[Z_n]  : companion orbit --------------------------------------
    ee.U_matrix = eval_companion(rp.comp, hg);

    // ----- closed-regime corollaries ---------------------------------------
    ee.closed_regime = (rp.rec.r > 0) && !rp.rational.Q.empty() && !rp.spectrum.modes.empty();
    if (ee.closed_regime) {
        ee.U_rational = eval_resolvent_grid(rp.rational, hg);
        ee.U_spectral = eval_spectral_modes(rp.spectrum, hg);
    }

    ee.err_continuous_jet  = diff_max(ee.U_continuous, ee.U_jet);
    ee.theorem_residual_on_phase_torus_path = ee.jet_semigroup_from_normalized_jet
                                           && !ee.U_continuous.empty()
                                           && (ee.U_continuous.size() == ee.U_jet.size())
                                           && std::isfinite(ee.err_continuous_jet);
    ee.err_jet_newton      = diff_max(ee.U_jet,        ee.U_newton);
    ee.err_newton_matrix   = diff_max(ee.U_newton,     ee.U_matrix);
    if (ee.closed_regime) {
        ee.err_matrix_rational   = diff_max(ee.U_matrix,   ee.U_rational);
        ee.err_rational_spectral = diff_max(ee.U_rational, ee.U_spectral);
    }
    ee.max_pairwise_abs_err = std::max({ee.err_continuous_jet,
                                        ee.err_jet_newton,
                                        ee.err_newton_matrix,
                                        ee.err_matrix_rational,
                                        ee.err_rational_spectral});
    return ee;
}

bool assert_evolution_equality(const EvolutionEquality& ee, real tol) {
    bool ok = (ee.err_continuous_jet < tol)
           && (ee.err_jet_newton     < tol)
           && (ee.err_newton_matrix  < tol);
    if (ee.closed_regime) {
        ok = ok && (ee.err_matrix_rational   < tol)
                && (ee.err_rational_spectral < tol);
    }
    return ok;
}

}  // namespace mt
