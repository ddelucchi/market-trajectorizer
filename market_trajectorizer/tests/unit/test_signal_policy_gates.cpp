#include <catch2/catch_test_macros.hpp>

#include "mt/backtest/signal_rules.hpp"

#include <cmath>

namespace {

mt::TrajectoryFieldContext make_context(bool theorem_authoritative,
                                        bool implementation_authoritative,
                                        bool research_non_authoritative,
                                        bool with_framework_energy,
                                        bool with_proxy_energy) {
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
    if (with_framework_energy) risk.energy_framework = {0.1, 0.1, 0.1};
    risk.energy_framework_present = with_framework_energy;
    if (with_proxy_energy) risk.energy_proxy = {0.2, 0.2, 0.2};
    risk.drawdown_proxy = {0.0, 0.0, 0.0};
    risk.curvature = {1.0, 1.0, 1.0};
    risk.uncertainty_proxy = {0.01, 0.01, 0.01};

    state = mt::MarketStateColumns{};
    state.resize(3);
    state.x3[2] = std::log(100.0);

    section = mt::PiecewiseSection{};
    section.event_packets = {
        mt::EventPacket{0, 1.0, 0.5, 0.2, 0.8, 0.1, 0.0, 0.4, 0.0, 3.0},
        mt::EventPacket{1, 0.2, 0.1, 0.0, 0.3, 0.0, 0.0, 0.2, 0.0, 0.8},
        mt::EventPacket{2, 0.3, 0.2, 0.0, 0.4, 0.0, 0.1, 0.3, 0.1, 1.4}
    };
    section.event_boundaries = {1, 2};

    mt::TrajectoryFieldContext ctx{
        traj,
        risk,
        state,
        &section,
        2,
        123,
        theorem_authoritative,
        true,
        implementation_authoritative,
        4,
        3,
        0.75,
        true,
        true,
        4,
        2,
        mt::AuthorityRegime::FiniteClosedRegime,
        {},
        research_non_authoritative,
        false
    };
    return ctx;
}

}  // namespace

TEST_CASE("production signal path fails closed without framework energy", "[signal][policy]") {
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
        make_context(/*theorem_authoritative*/ true,
                     /*implementation_authoritative*/ true,
                     /*research_non_authoritative*/ false,
                     /*with_framework_energy*/ false,
                     /*with_proxy_energy*/ true),
        cfg);

    REQUIRE(s.side == 0);
    REQUIRE_FALSE(s.used_proxy_energy);
}

TEST_CASE("production path enforces authoritative anchor gate", "[signal][policy]") {
    mt::SignalConfig cfg;
    cfg.signal_threshold = 0.0;
    cfg.long_threshold = 0.0;
    cfg.short_threshold = 0.0;
    cfg.lambda_energy = 0.0;
    cfg.lambda_drawdown = 0.0;
    cfg.lambda_curvature = 0.0;
    cfg.require_authoritative_anchor = true;
    cfg.require_framework_energy = true;
    cfg.research_heuristic_signal = false;

    mt::Signal s = mt::build_signal_from_trajectory(
        make_context(/*theorem_authoritative*/ false,
                     /*implementation_authoritative*/ false,
                     /*research_non_authoritative*/ false,
                     /*with_framework_energy*/ true,
                     /*with_proxy_energy*/ true),
        cfg);

    REQUIRE(s.side == 0);
}

TEST_CASE("legacy heuristic path requires explicit research flag", "[signal][policy][legacy_ban]") {
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

    mt::SignalConfig production_cfg = cfg;
    production_cfg.research_heuristic_signal = false;

    mt::SignalConfig research_cfg = cfg;
    research_cfg.research_heuristic_signal = true;

    mt::Signal production = mt::build_signal_from_trajectory(
        make_context(/*theorem_authoritative*/ true,
                     /*implementation_authoritative*/ true,
                     /*research_non_authoritative*/ true,
                     /*with_framework_energy*/ false,
                     /*with_proxy_energy*/ true),
        production_cfg);

    mt::Signal research = mt::build_signal_from_trajectory(
        make_context(/*theorem_authoritative*/ true,
                     /*implementation_authoritative*/ true,
                     /*research_non_authoritative*/ true,
                     /*with_framework_energy*/ false,
                     /*with_proxy_energy*/ true),
        research_cfg);

    REQUIRE(production.side == 0);
    REQUIRE(research.side != 0);
    REQUIRE(research.research_non_authoritative);
    REQUIRE(research.used_proxy_energy);
}
