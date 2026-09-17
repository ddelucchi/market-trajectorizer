#pragma once
#include "mt/core/types.hpp"
#include <string>
#include <string_view>

namespace mt {

struct ChangePointConfig {
    real k_return  = 3.0;
    real k_range   = 3.0;
    real k_volume  = 4.0;
    int  rolling_window = 32;
    int  min_separation = 4;
    real gap_sigma = 2.5;
    real range_sigma = 2.5;
    real body_sigma = 2.5;
    real volume_sigma = 2.5;
    real wick_sigma = 2.5;
    real compression_sigma = 2.0;
    real drift_sigma = 2.0;
    real exhaustion_sigma = 2.0;
    real packet_transition_eps = 3.5;
    int  packet_persistence = 2;
    real packet_mad_eps = 1e-6;
    real weight_gap = 1.0;
    real weight_range = 1.0;
    real weight_body = 1.0;
    real weight_volume = 1.0;
    real weight_wick = 1.0;
    real weight_compression = 1.0;
    real weight_drift = 0.6;
    real weight_exhaustion = 1.0;
    bool single_sector_smooth_regime_only = false;
    // Research-only escape hatch.  When true, build_piecewise_section_cpu may
    // insert a synthetic deterministic max-transition fallback boundary if no
    // genuine packet-driven boundary survives thresholding.  Such a section
    // MUST fail the production authority clause
    // `event_state.genuine_packet_dynamics`.  Default false: production code
    // never inserts a synthetic boundary; an empty event set leaves the
    // section empty and authority correctly rejects.
    bool allow_synthetic_event_boundary_research_only = false;
};

struct SectionConfig {
    int N_phi = 16;
    int N_pi  = 8;
    int L_max = 8;
};

struct PhaseTorusConfig {
    int  Q          = 5;
    int  order_max  = 8;
    int  M_per_dim  = 16;
    real rho_m      = 0.05;
};

struct EngineConfig {
    ChangePointConfig changepoint{};
    SectionConfig     section{};
    PhaseTorusConfig  torus{};
    int  H                 = 64;     // horizon length
    int  N_phi             = 16;
    int  N_pi              = 8;
    int  L_max             = 8;
    int  recurrence_window = 64;
    real tol_rank          = 1e-9;
    real tol_evolution     = 1e-8;
    real tol_closure       = 1e-8;
    real tol_interstice    = 1e-6;
    int  s_count           = 64;
    real ds                = 1.0;
    real u_imag            = 1.0;
    bool deterministic     = true;
    bool fp32_profile      = false;
};

struct SignalConfig {
    int  h_eval           = 16;
    real signal_threshold = 1e-4;
    real stop_atr_mult    = 2.0;
    real long_threshold   = 0.0;
    real short_threshold  = 0.0;
    real dd_threshold     = 0.05;
    real lambda_energy    = 1.0;
    real lambda_drawdown  = 1.0;
    real lambda_curvature = 1.0;
    bool require_authoritative_anchor = true;
    bool require_framework_energy     = true;
    bool allow_proxy_energy_in_research = false;
    bool research_heuristic_signal      = false;
};

struct BacktestConfig {
    real starting_equity = 1.0e6;
    real fee_bps         = 1.0;
    real slippage_bps    = 1.0;
    int  fill_lag_bars   = 1;     // fill at next bar open
    bool intrabar        = false;
    bool research_non_authoritative = false;
};

struct WalkForwardConfig {
    int train_min   = 252;
    int test_window = 63;
    int step        = 21;
    int horizon_max = 64;
    int anchor_stride = 1;
    int max_windows = 0;
};

EngineConfig    load_engine_config(std::string_view path);
SignalConfig    load_signal_config(std::string_view path);
BacktestConfig  load_backtest_config(std::string_view path);
WalkForwardConfig load_walk_forward_config(std::string_view path);

}  // namespace mt
