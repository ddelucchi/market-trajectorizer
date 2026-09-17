#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "mt/engine/pipeline.hpp"

#include <algorithm>
#include <cmath>

namespace {

mt::CandleColumns make_candles(int n) {
    mt::CandleColumns c;
    c.resize(static_cast<mt::usize>(n));
    for (int i = 0; i < n; ++i) {
        const double close = 100.0 + 0.2 * static_cast<double>(i) + 0.01 * std::sin(static_cast<double>(i));
        c.ts[static_cast<mt::usize>(i)] = static_cast<mt::Timestamp>(1'000'000'000LL * (i + 1));
        c.open[static_cast<mt::usize>(i)] = close - 0.05;
        c.high[static_cast<mt::usize>(i)] = close + 0.10;
        c.low[static_cast<mt::usize>(i)] = close - 0.10;
        c.close[static_cast<mt::usize>(i)] = close;
        c.volume[static_cast<mt::usize>(i)] = 1000.0 + 5.0 * static_cast<double>(i);
    }
    return c;
}

mt::real max_abs_diff(const mt::Vec<mt::cplx>& a, const mt::Vec<mt::cplx>& b) {
    const mt::usize n = std::min(a.size(), b.size());
    mt::real m = 0.0;
    for (mt::usize i = 0; i < n; ++i) m = std::max(m, std::abs(a[i] - b[i]));
    return m;
}

}  // namespace

TEST_CASE("anchor recurrence carrier is causal and independent of post-anchor bars", "[pipeline][causality]") {
    mt::EngineConfig cfg;
    cfg.H = 8;
    cfg.recurrence_window = 16;
    cfg.s_count = 8;
    cfg.ds = 0.25;
    cfg.u_imag = 0.0;
    cfg.changepoint.rolling_window = 4;

    mt::CandleColumns a = make_candles(40);
    mt::CandleColumns b = a;

    const int mutate_from = 25;
    for (int i = mutate_from; i < static_cast<int>(b.n); ++i) {
        b.close[static_cast<mt::usize>(i)] += 500.0 + static_cast<double>(i);
        b.open[static_cast<mt::usize>(i)] = b.close[static_cast<mt::usize>(i)] - 0.25;
        b.high[static_cast<mt::usize>(i)] = b.close[static_cast<mt::usize>(i)] + 0.50;
        b.low[static_cast<mt::usize>(i)] = b.close[static_cast<mt::usize>(i)] - 0.50;
    }

    const int anchor = 20;
    mt::AnchorArtifacts art_a = mt::run_anchor(a, anchor, cfg);
    mt::AnchorArtifacts art_b = mt::run_anchor(b, anchor, cfg);

    REQUIRE(art_a.cps.sigma_idx == art_b.cps.sigma_idx);
    REQUIRE(max_abs_diff(art_a.u_carrier, art_b.u_carrier) < 1e-12);
    REQUIRE(max_abs_diff(art_a.evolution.U_continuous, art_b.evolution.U_continuous) < 1e-12);
}

TEST_CASE("run_anchor is deterministic and reports theorem scaffold cpu mode", "[pipeline][determinism]") {
    mt::EngineConfig cfg;
    cfg.H = 8;
    cfg.recurrence_window = 16;
    cfg.s_count = 8;
    cfg.ds = 0.25;
    cfg.u_imag = 0.0;
    cfg.changepoint.rolling_window = 4;

    mt::CandleColumns c = make_candles(48);

    mt::AnchorArtifacts art1 = mt::run_anchor(c, 30, cfg);
    mt::AnchorArtifacts art2 = mt::run_anchor(c, 30, cfg);

    REQUIRE(max_abs_diff(art1.u_carrier, art2.u_carrier) < 1e-12);
    REQUIRE(max_abs_diff(art1.evolution.U_newton, art2.evolution.U_newton) < 1e-12);
    REQUIRE(art1.authority.gpu_mode == art2.authority.gpu_mode);
    REQUIRE(art1.authority.theorem_authoritative == art2.authority.theorem_authoritative);
    REQUIRE(art1.authority.implementation_authoritative == art2.authority.implementation_authoritative);
    REQUIRE(art1.authority.closure_verified == art2.authority.closure_verified);
    REQUIRE(art1.authority.interstice_verified == art2.authority.interstice_verified);
    REQUIRE(art1.authority.causality_anchor_truncation_verified == art2.authority.causality_anchor_truncation_verified);
    REQUIRE(art1.authority.causality_carrier_verified == art2.authority.causality_carrier_verified);
    REQUIRE(art1.authority.phase_torus_path_verified == art2.authority.phase_torus_path_verified);
    REQUIRE(art1.authority.jet_normalization_identity_verified == art2.authority.jet_normalization_identity_verified);
    REQUIRE(art1.authority.di_projection_verified == art2.authority.di_projection_verified);
    REQUIRE(art1.authority.failed_clauses == art2.authority.failed_clauses);
    REQUIRE(art1.authority.max_theorem_residual == Catch::Approx(art2.authority.max_theorem_residual).margin(1e-14));
    REQUIRE(art1.authority.max_jet_normalization_residual == Catch::Approx(art2.authority.max_jet_normalization_residual).margin(1e-14));
    REQUIRE(art1.authority.max_closure_residual == Catch::Approx(art2.authority.max_closure_residual).margin(1e-14));
    REQUIRE(art1.authority.max_interstice_pde_residual == Catch::Approx(art2.authority.max_interstice_pde_residual).margin(1e-14));
    REQUIRE(art1.authority.max_interstice_recurrence_residual == Catch::Approx(art2.authority.max_interstice_recurrence_residual).margin(1e-14));

    // Path B policy: CPU mode is theorem-scaffold only until implementation authority is available.
    REQUIRE(art1.authority.gpu_mode == "cpu_authoritative");
    REQUIRE_FALSE(art1.authority.implementation_authoritative);

    // Horizon grid must include h=0 as anchor point.
    REQUIRE_FALSE(art1.trajectory.h_grid.empty());
    REQUIRE(art1.trajectory.h_grid.front() == Catch::Approx(0.0).margin(1e-12));
    REQUIRE(static_cast<int>(art1.trajectory.h_grid.size()) == cfg.H + 1);

    // Canonical market-state mapping is authoritative for all downstream paths.
    REQUIRE(art1.market_state.n == static_cast<mt::usize>(31));
    REQUIRE(art1.market_state.x0.back() == Catch::Approx(std::log(c.open[30])).margin(1e-12));
    REQUIRE(art1.market_state.x1.back() == Catch::Approx(std::log(c.high[30])).margin(1e-12));
    REQUIRE(art1.market_state.x2.back() == Catch::Approx(std::log(c.low[30])).margin(1e-12));
    REQUIRE(art1.market_state.x3.back() == Catch::Approx(std::log(c.close[30])).margin(1e-12));
}
