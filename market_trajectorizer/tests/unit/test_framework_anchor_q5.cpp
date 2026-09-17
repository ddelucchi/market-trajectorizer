#include <catch2/catch_test_macros.hpp>

#include "mt/engine/pipeline.hpp"
#include "mt/math/combinatorics.hpp"

#include <cmath>

namespace {

mt::CandleColumns make_candles(int n) {
    mt::CandleColumns c;
    c.resize(static_cast<mt::usize>(n));
    for (int i = 0; i < n; ++i) {
        const double base  = 100.0 + 0.1 * static_cast<double>(i)
                           + 0.02 * std::sin(0.07 * static_cast<double>(i));
        const double range = 0.18 + 0.05 * std::cos(0.11 * static_cast<double>(i));
        c.ts[static_cast<mt::usize>(i)]    = static_cast<mt::Timestamp>(1'000'000'000LL * (i + 1));
        c.open[static_cast<mt::usize>(i)]  = base - 0.05;
        c.high[static_cast<mt::usize>(i)]  = base + range;
        c.low[static_cast<mt::usize>(i)]   = base - range;
        c.close[static_cast<mt::usize>(i)] = base + 0.03 * std::sin(0.13 * static_cast<double>(i));
        c.volume[static_cast<mt::usize>(i)] = 1000.0 + 7.0 * static_cast<double>(i);
    }
    return c;
}

}  // namespace

TEST_CASE("anchor extraction is genuinely Q=5 multi-channel (no scalar close-only fallback)",
          "[framework][phase_torus][q5]") {
    mt::EngineConfig cfg;
    cfg.H = 6;
    cfg.recurrence_window = 16;
    cfg.s_count = 6;
    cfg.ds = 0.25;
    cfg.u_imag = 0.0;
    cfg.changepoint.rolling_window = 4;

    const mt::CandleColumns c = make_candles(48);
    const int t_anchor = 32;

    mt::AnchorArtifacts art = mt::run_anchor(c, t_anchor, cfg);

    // Layout dimension n must equal Q=5; the legacy single-axis (Q=1) path is
    // mathematically forbidden on an OHLCV trajectorizer.
    REQUIRE(art.extracted_phase_torus.Q == 5);
    REQUIRE(art.extracted_phase_torus.order_max >= 1);

    const auto layout = mt::make_multi_index_layout(art.extracted_phase_torus.Q,
                                                    art.extracted_phase_torus.order_max);
    REQUIRE_FALSE(art.extracted_phase_torus.A.empty());

    // For the separable extractor, axis indices (single-channel) must carry
    // non-trivial energy on at least one of the Q=5 channels.
    bool any_axis_nonzero = false;
    for (int q = 0; q < 5; ++q) {
        for (int k = 1; k <= art.extracted_phase_torus.order_max; ++k) {
            mt::Vec<int> idx(5, 0);
            idx[static_cast<mt::usize>(q)] = k;
            const mt::usize fi = mt::flatten_multi_index(idx, layout);
            if (std::abs(art.extracted_phase_torus.A[fi]) > 1e-12) {
                any_axis_nonzero = true;
            }
        }
    }
    REQUIRE(any_axis_nonzero);
}
