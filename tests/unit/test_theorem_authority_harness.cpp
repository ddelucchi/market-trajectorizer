#include <catch2/catch_test_macros.hpp>

#include "mt/engine/pipeline.hpp"

#include <algorithm>
#include <cmath>

namespace {

mt::CandleColumns make_candles(int n) {
    mt::CandleColumns c;
    c.resize(static_cast<mt::usize>(n));
    for (int i = 0; i < n; ++i) {
        const double close = 100.0 + 0.1 * static_cast<double>(i)
                           + 0.02 * std::sin(0.07 * static_cast<double>(i));
        c.ts[static_cast<mt::usize>(i)] = static_cast<mt::Timestamp>(1'000'000'000LL * (i + 1));
        c.open[static_cast<mt::usize>(i)] = close - 0.05;
        c.high[static_cast<mt::usize>(i)] = close + 0.12;
        c.low[static_cast<mt::usize>(i)] = close - 0.12;
        c.close[static_cast<mt::usize>(i)] = close;
        c.volume[static_cast<mt::usize>(i)] = 1000.0 + 3.0 * static_cast<double>(i);
    }
    return c;
}

}  // namespace

TEST_CASE("theorem_authority harness scans anchors with split authority semantics", "[theorem_authority][pipeline]") {
    mt::EngineConfig cfg;
    cfg.H = 8;
    cfg.recurrence_window = 16;
    cfg.s_count = 8;
    cfg.ds = 0.25;
    cfg.u_imag = 0.0;
    cfg.changepoint.rolling_window = 4;

    mt::CandleColumns c = make_candles(64);
    int anchors_scanned = 0;

    for (int t = cfg.recurrence_window; t < static_cast<int>(c.n); ++t) {
        mt::AnchorArtifacts art = mt::run_anchor(c, t, cfg);
        ++anchors_scanned;

        const bool extraction_executed = !art.extracted_phase_torus.A.empty()
                                      && (art.extracted_phase_torus.rho_m > 0.0);
        const bool normalized_jet_built = !art.normalized_jet.coeff.empty()
                                       && art.jet_from_phase_torus;
        const bool jet_normalization_identity = art.authority.jet_normalization_identity_verified;
        const bool expected_phase_chain = extraction_executed
                                       && normalized_jet_built
                           && jet_normalization_identity
                                       && art.evolution.jet_semigroup_from_normalized_jet
                                       && art.evolution.theorem_residual_on_phase_torus_path;

        REQUIRE(art.authority.authoritative == art.authority.theorem_authoritative);
        REQUIRE(art.authority.phase_torus_path_verified == expected_phase_chain);
        REQUIRE_FALSE(art.authority.implementation_authoritative);
        REQUIRE(art.authority.gpu_mode == "cpu_authoritative");
        REQUIRE(art.authority.theorem_slice_single_axis);
        REQUIRE(art.authority.theorem_valid_h_count >= 0);
        if (art.authority.theorem_valid_h_count == 0) {
            REQUIRE(art.authority.theorem_valid_h_max < 0);
        } else {
            REQUIRE(art.authority.theorem_valid_h_max == art.authority.theorem_valid_h_count - 1);
        }

        const bool has_local_sector_clause = std::any_of(
            art.authority.failed_clauses.begin(),
            art.authority.failed_clauses.end(),
            [](const std::string& s) { return s == "theorem.local_valid_sector"; });
        REQUIRE(has_local_sector_clause == !art.authority.theorem_local_sector_verified);

        if (art.authority.theorem_authoritative) {
            REQUIRE(art.authority.theorem_local_sector_verified);
            REQUIRE(art.authority.failed_clauses.empty());
        } else {
            REQUIRE_FALSE(art.authority.failed_clauses.empty());
        }
    }

    REQUIRE(anchors_scanned > 0);
}
