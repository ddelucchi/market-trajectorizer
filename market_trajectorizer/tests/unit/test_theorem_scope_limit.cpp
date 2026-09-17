#include <catch2/catch_test_macros.hpp>

#include "mt/engine/pipeline.hpp"

#include <cmath>

namespace {

mt::CandleColumns make_candles(int n) {
    mt::CandleColumns c;
    c.resize(static_cast<mt::usize>(n));
    for (int i = 0; i < n; ++i) {
        const double base  = 100.0 + 0.08 * static_cast<double>(i)
                           + 0.015 * std::sin(0.09 * static_cast<double>(i));
        const double range = 0.16 + 0.04 * std::cos(0.15 * static_cast<double>(i));
        c.ts[static_cast<mt::usize>(i)]    = static_cast<mt::Timestamp>(1'000'000'000LL * (i + 1));
        c.open[static_cast<mt::usize>(i)]  = base - 0.04;
        c.high[static_cast<mt::usize>(i)]  = base + range;
        c.low[static_cast<mt::usize>(i)]   = base - range;
        c.close[static_cast<mt::usize>(i)] = base + 0.02 * std::sin(0.17 * static_cast<double>(i));
        c.volume[static_cast<mt::usize>(i)] = 900.0 + 5.0 * static_cast<double>(i);
    }
    return c;
}

}  // namespace

TEST_CASE("theorem authority remains canonical-axis audit slice (explicit interim scope)",
          "[framework][theorem_scope][interim]") {
    mt::EngineConfig cfg;
    cfg.H = 6;
    cfg.recurrence_window = 16;
    cfg.s_count = 6;
    cfg.ds = 0.25;
    cfg.u_imag = 0.0;
    cfg.changepoint.rolling_window = 4;

    const mt::CandleColumns c = make_candles(48);
    mt::AnchorArtifacts art = mt::run_anchor(c, /*t_anchor=*/32, cfg);

    // Explicitly encode the current limitation so we do not over-claim OHLCV
    // completion while the theorem object is still a canonical-axis audit slice.
    REQUIRE(art.authority.theorem_slice_single_axis);
}
