#pragma once
#include "mt/core/config.hpp"
#include "mt/data/dataset.hpp"
#include "mt/engine/extraction_engine.hpp"
#include "mt/engine/recurrence_engine.hpp"
#include "mt/engine/risk_engine.hpp"
#include "mt/engine/trajectorizer_engine.hpp"
#include "mt/math/interstice.hpp"
#include "mt/state/extracted_coeffs.hpp"
#include "mt/state/authority_state.hpp"
#include "mt/state/interstice_state.hpp"
#include "mt/state/jet_state.hpp"
#include "mt/state/market_state.hpp"
#include "mt/state/piecewise_section.hpp"

#include <limits>
#include <string>

namespace mt {

struct AuthoritativeStatus {
    // Mathematical/theorem authority from contract clauses.
    bool             theorem_authoritative                = false;
    // Implementation authority from fully realized runtime path (e.g. full GPU).
    bool             implementation_authoritative         = false;
    // Backward-compatible aggregate flag used by existing artifacts/CLI.
    bool             authoritative                        = false;
    bool             closure_verified                     = false;
    bool             interstice_verified                  = false;
    bool             causality_anchor_truncation_verified = false;
    bool             causality_carrier_verified           = false;
    bool             phase_torus_path_verified            = false;
    bool             jet_normalization_identity_verified  = false;
    bool             jet_normalization_verified           = false;
    bool             di_projection_verified               = false;
    bool             closure_claimed                      = false;
    bool             nontrivial_future_field              = false;
    bool             nontrivial_interstice_field          = false;
    bool             event_state_nonempty                 = false;
    bool             event_state_from_genuine_packet_dynamics = false;
    bool             closure_nontrivial                   = false;
    bool             spectral_realization_nonempty        = false;
    bool             carrier_not_constant_identity        = false;
    bool             production_ready                     = false;
    bool             carrier_continuous_present           = false;
    bool             carrier_jet_present                  = false;
    bool             carrier_newton_present               = false;
    bool             carrier_matrix_present               = false;
    bool             carrier_rational_present             = false;
    bool             carrier_spectral_present             = false;
    // Theorem-path slice descriptor: current theorem carrier is an audited
    // canonical-axis slice through Q=5 (alpha=(n,0,0,0,0)), not a fully
    // coupled multichannel theorem field.
    bool             theorem_slice_single_axis            = true;
    // Local theorem-valid sector certificate (finite exact-sector analogue).
    // H_valid = {0..theorem_valid_h_max} when theorem_valid_h_max >= 0.
    bool             theorem_local_sector_verified        = false;
    int              theorem_valid_h_max                  = -1;
    int              theorem_valid_h_count                = 0;
    real             theorem_valid_h_fraction             = 0.0;
    // Decomposed structural caps composing the local sector certificate.
    int              theorem_valid_h_coeff_cap            = -1;
    int              theorem_valid_h_packet_cap           = -1;
    int              theorem_valid_h_condition_cap        = -1;
    int              theorem_valid_h_residual_cap         = -1;
    real             theorem_coeff_radius_estimate        = 0.0;
    real             theorem_recurrence_condition         = std::numeric_limits<real>::infinity();
    std::string      carrier_rational_reason_missing      = "";
    std::string      carrier_spectral_reason_missing      = "";
    AuthorityRegime  regime                               = AuthorityRegime::RejectedNonAuthoritativeRegime;
    std::string      gpu_mode                             = "cpu_authoritative";
    Vec<std::string> failed_clauses;

    real             max_theorem_residual                = 0.0;
    real             max_jet_normalization_residual      = 0.0;
    real             max_closure_residual                = 0.0;
    real             max_interstice_pde_residual         = 0.0;
    real             max_interstice_recurrence_residual  = 0.0;
    real             err_phase_torus_to_future_section   = 0.0;
    // Reason string set when `theorem.past_jet_invalid` is in failed_clauses.
    // Empty when the FutureSection is valid.
    std::string      past_jet_invalid_reason             = "";
};

struct AnchorArtifacts {
    int                   t_anchor    = 0;
    Timestamp             anchor_ts   = 0;
    MarketStateColumns    market_state;
    ChangePointSet        cps;
    PiecewiseSection      section;
    FutureSection         jet_minus;
    ExtractedCoefficients extracted_phase_torus;
    JetState              normalized_jet;
    bool                  jet_from_phase_torus = false;
    Vec<cplx>             u_carrier;
    RecurrencePipelineOut recurrence;
    EvolutionEquality     evolution;
    bool                  evolution_ok = false;
    IntersticeTrajectory  trajectory;
    Vec<cplx>             future_section_from_phase_torus;
    Vec<real>             interstice_pde_residual_grid;
    Vec<real>             interstice_recurrence_residual_grid;
    RiskField             risk;
    AuthoritativeStatus   authority;
};

AuthoritativeStatus validate_anchor_authoritativeness(const AnchorArtifacts& art,
                                                      const EngineConfig&    cfg);

bool is_anchor_production_authoritative(const AnchorArtifacts& art);

AnchorArtifacts run_anchor(const CandleColumns& candles, int t_anchor, const EngineConfig& cfg);

}  // namespace mt
