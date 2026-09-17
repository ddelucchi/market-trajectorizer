#pragma once
#include "mt/engine/recurrence_engine.hpp"
#include "mt/state/interstice_state.hpp"   // FutureSection (J_t^-)
#include "mt/state/jet_state.hpp"

namespace mt {

// The evolution equality (theorem object).
//
//      E_h[F]_n  =  J_h[F]_n  =  N_h[b]_n  =  M_h[Z_n]
//
// (continuous formation = jet semigroup = Newton-Gregory = matrix propagator).
// Rational and spectral are corollaries available only in the finite-dimensional
// closure regime; they are reported separately so the theorem does not depend
// on their availability.
struct EvolutionEquality {
    Vec<cplx> U_continuous;     // E_h[F]_n  =  F_t[J_t^-](h)
    Vec<cplx> U_jet;            // J_h[F]_n  =  (e^{h L_jet,κ} J_n^-)_0
    Vec<cplx> U_newton;         // N_h[b]_n  =  Σ β_k(h) ∇^k u
    Vec<cplx> U_matrix;         // M_h[Z_n]  =  R · M_*^h · Z_n  (companion orbit)

    // Newton decomposition diagnostics for runtime authority debugging.
    Vec<cplx>      newton_input_sequence;
    Vec<cplx>      newton_backward_differences;
    Vec<real>      newton_eta_grid;
    Vec<Vec<real>> newton_basis_values;
    Vec<Vec<cplx>> newton_partial_sums;

    // Provenance flags for theorem-path authority checks.
    bool      jet_semigroup_from_normalized_jet      = false;
    bool      theorem_residual_on_phase_torus_path   = false;

    // Corollaries (populated only when closed_regime == true)
    Vec<cplx> U_rational;       // [z^h] P(z)/Q(z)
    Vec<cplx> U_spectral;       // Σ γ_{j,s} C(h,s) λ_j^{h-s}
    bool      closed_regime  = false;

    real      err_continuous_jet     = 0.0;
    real      err_jet_newton         = 0.0;
    real      err_newton_matrix      = 0.0;
    real      err_matrix_rational    = 0.0;
    real      err_rational_spectral  = 0.0;
    real      max_pairwise_abs_err   = 0.0;
};

// Compute U_continuous from F_t[J_t^-](h), U_jet from the normalized extracted
// phase-torus jet under the truncated semigroup, U_newton from backward
// differences of u, and U_matrix from companion-orbit propagation.
EvolutionEquality compute_evolution_equality(const FutureSection&         jet_minus,
                                             const JetState&              normalized_jet,
                                             const RecurrencePipelineOut& rp,
                                             const Vec<cplx>&             u_history,
                                             const HorizonGrid&           hg);

// True iff every implemented identity holds within tol; rational/spectral
// identities are only required when ee.closed_regime is true.
bool assert_evolution_equality(const EvolutionEquality& ee, real tol);

}  // namespace mt
