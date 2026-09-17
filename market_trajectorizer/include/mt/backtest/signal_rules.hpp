#pragma once
#include "mt/core/config.hpp"
#include "mt/data/feature_map.hpp"
#include "mt/math/interstice.hpp"
#include "mt/state/authority_state.hpp"
#include "mt/state/interstice_state.hpp"
#include "mt/state/risk_state.hpp"
#include <string>

namespace mt {

struct Signal {
    Timestamp ts        = 0;
    int       side      = 0;     // -1, 0, +1
    real      strength  = 0.0;   // 0..1
    real      entry_px  = 0.0;
    real      stop_px   = 0.0;
    real      take_px   = 0.0;
    real      score_plus = 0.0;
    real      score_minus = 0.0;
    real      score_dominant = 0.0;
    bool      implementation_authoritative = false;
    bool      research_heuristic_signal = false;
    std::string authority_regime;

    // Field-level decomposition that produced the decision.
    Vec<real> projected_log_path;
    Vec<real> projected_return_path;
    Vec<real> framework_energy_path;
    Vec<real> proxy_energy_path;
    Vec<real> curvature_path;
    Vec<real> uncertainty_proxy_path;
    Vec<real> event_joint_score_path;

    int       recurrence_order    = 0;
    int       spectral_mode_count = 0;
    int       event_packet_count  = 0;
    bool      closure_claimed     = false;
    bool      closure_verified    = false;
    bool      theorem_authoritative = false;
    bool      theorem_local_sector_verified = false;
    int       theorem_valid_h_max = -1;
    int       theorem_valid_h_count = 0;
    real      theorem_valid_h_fraction = 0.0;
    bool      research_non_authoritative = false;
    bool      used_proxy_energy   = false;
    std::string framework_energy_missing_reason;
    Vec<std::string> failed_clauses;
};

struct TrajectoryFieldContext {
    const IntersticeTrajectory& trajectory_field;
    const RiskField&            risk_field;
    const MarketStateColumns&   market_state;
    const PiecewiseSection*     event_section = nullptr;
    int                          anchor_idx = 0;
    Timestamp                    anchor_ts  = 0;
    bool                         anchor_theorem_authoritative = false;
    bool                         anchor_theorem_local_sector_verified = false;
    bool                         anchor_implementation_authoritative = false;
    int                          anchor_theorem_valid_h_max   = -1;
    int                          anchor_theorem_valid_h_count = 0;
    real                         anchor_theorem_valid_h_fraction = 0.0;
    bool                         anchor_closure_claimed       = false;
    bool                         anchor_closure_verified      = false;
    int                          recurrence_order             = 0;
    int                          spectral_mode_count          = 0;
    AuthorityRegime              authority_regime = AuthorityRegime::RejectedNonAuthoritativeRegime;
    Vec<std::string>             failed_clauses;
    bool                         research_non_authoritative   = false;
    bool                         research_heuristic_signal    = false;
};

Signal build_signal_from_trajectory(const TrajectoryFieldContext& ctx, const SignalConfig& cfg);

}  // namespace mt
