#include <catch2/catch_test_macros.hpp>

#include "mt/backtest/signal_rules.hpp"

#include <cmath>
#include <string>

namespace {

mt::TrajectoryFieldContext make_context_no_framework_energy(const std::string& risk_reason) {
    static mt::IntersticeTrajectory traj;
    static mt::RiskField risk;
    static mt::MarketStateColumns state;
    static mt::PiecewiseSection section;

    traj = mt::IntersticeTrajectory{};
    traj.s_count = 1;
    traj.H = 3;
    traj.h_grid = {0.0, 1.0, 2.0};
    traj.T = {mt::cplx(0.0, 0.0), mt::cplx(4.0, 0.0), mt::cplx(7.0, 0.0)};
    traj.T_grid = traj.T;

    risk = mt::RiskField{};
    risk.energy_framework_present = false;
    risk.energy_framework_missing_reason = risk_reason;
    risk.energy_proxy = {0.2, 0.2, 0.2};
    risk.drawdown_proxy = {0.0, 0.0, 0.0};
    risk.curvature = {1.0, 1.0, 1.0};
    risk.uncertainty_proxy = {0.01, 0.01, 0.01};

    state = mt::MarketStateColumns{};
    state.resize(3);
    state.x3[2] = std::log(100.0);

    section = mt::PiecewiseSection{};

    mt::TrajectoryFieldContext ctx{
        traj, risk, state, &section,
        2, 123,
        true, true, true,
        4, 3, 0.75,
        true, true, 4, 2,
        mt::AuthorityRegime::FiniteClosedRegime,
        {}, false, false
    };
    return ctx;
}

}  // namespace

TEST_CASE("production signal propagates framework-energy missing reason from risk layer",
          "[framework][signal][energy_reason_propagation]") {
    mt::SignalConfig cfg;
    cfg.signal_threshold = 0.0;
    cfg.long_threshold = 0.0;
    cfg.short_threshold = 0.0;
    cfg.lambda_energy = 0.0;
    cfg.lambda_drawdown = 0.0;
    cfg.lambda_curvature = 0.0;
    cfg.require_authoritative_anchor = true;
    cfg.require_framework_energy = true;
    cfg.allow_proxy_energy_in_research = false;
    cfg.research_heuristic_signal = false;

    mt::Signal s = mt::build_signal_from_trajectory(
        make_context_no_framework_energy("trajectory_risk_carrier_absent"),
        cfg);

    REQUIRE(s.side == 0);
    // The signal's missing-reason string MUST cite the risk layer's precise
    // cause; a generic reason that drops the upstream cause is forbidden.
    REQUIRE(s.framework_energy_missing_reason.find("trajectory_risk_carrier_absent")
            != std::string::npos);
}
