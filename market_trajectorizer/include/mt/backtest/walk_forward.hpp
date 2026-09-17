#pragma once
#include "mt/backtest/metrics.hpp"
#include "mt/backtest/simulator.hpp"
#include "mt/core/config.hpp"
#include "mt/data/dataset.hpp"
#include <string>

namespace mt {

struct WalkForwardWindow {
    int train_start = 0;
    int train_end   = 0;
    int test_start  = 0;
    int test_end    = 0;
};

struct FidelityMetrics {
    int  anchors_evaluated = 0;
    real lead_sign_agreement = 0.0;
    real turning_point_alignment = 0.0;
    real path_rmse = 0.0;
    real event_boundary_overlap = 0.0;
    real drawdown_shape_similarity = 0.0;
};

struct StratifiedMetrics {
    int             anchors = 0;
    int             signals = 0;
    Metrics         metrics;
    FidelityMetrics fidelity;
};

struct ClauseStratifiedMetrics {
    std::string     clause;
    int             anchors = 0;
    int             signals = 0;
    Metrics         metrics;
    FidelityMetrics fidelity;
};

struct WalkForwardWindowDiagnostics {
    StratifiedMetrics all;
    StratifiedMetrics authoritative;
    StratifiedMetrics non_authoritative;
    StratifiedMetrics local_sector_valid;
    StratifiedMetrics local_sector_invalid;
    StratifiedMetrics packet_anchor;
    StratifiedMetrics no_packet_anchor;
    StratifiedMetrics recurrence_r0;
    StratifiedMetrics recurrence_r1_2;
    StratifiedMetrics recurrence_r3_5;
    StratifiedMetrics recurrence_r6_plus;
    StratifiedMetrics spectral_m0;
    StratifiedMetrics spectral_m1;
    StratifiedMetrics spectral_m2_3;
    StratifiedMetrics spectral_m4_plus;
    StratifiedMetrics theorem_hcount_0;
    StratifiedMetrics theorem_hcount_1_4;
    StratifiedMetrics theorem_hcount_5_16;
    StratifiedMetrics theorem_hcount_17_plus;
    Vec<ClauseStratifiedMetrics> failed_clause_primary;
};

struct WalkForwardResult {
    Vec<WalkForwardWindow> windows;
    Vec<Metrics>           per_window;
    Vec<WalkForwardWindowDiagnostics> per_window_diagnostics;
    Metrics                aggregate;
    WalkForwardWindowDiagnostics aggregate_diagnostics;
    int                    authoritative_anchors = 0;
    int                    non_authoritative_anchors = 0;
    int                    rejected_anchors = 0;
    int                    research_proxy_energy_signals = 0;
    bool                   research_non_authoritative = false;
    bool                   research_heuristic_signal = false;
};

WalkForwardResult run_walk_forward(const Dataset& ds,
                                   const WalkForwardConfig& wf,
                                   const EngineConfig&      ec,
                                   const SignalConfig&      sc,
                                   const BacktestConfig&    bc);

}  // namespace mt
