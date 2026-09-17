#include <catch2/catch_test_macros.hpp>

#include "mt/engine/pipeline.hpp"
#include "mt/engine/extraction_engine.hpp"

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

TEST_CASE("future-section identity holds on the actual anchor jet (no scalar surrogate)",
          "[framework][future_section][identity]") {
    mt::EngineConfig cfg;
    cfg.H = 6;
    cfg.recurrence_window = 16;
    cfg.s_count = 6;
    cfg.ds = 0.25;
    cfg.u_imag = 0.0;
    cfg.changepoint.rolling_window = 4;

    const mt::CandleColumns c = make_candles(48);
    mt::AnchorArtifacts art = mt::run_anchor(c, /*t_anchor=*/32, cfg);

    // The carrier we evaluate must be the actual past-jet built from the
    // canonical Q=5 normalized jet, not a scalar x3 fallback.
    REQUIRE(art.jet_minus.valid);
    REQUIRE(art.jet_minus.invalid_reason.empty());

    // The theorem carrier path must use the smooth evaluator for all sampled
    // h >= 0 in u_carrier (not the packet-augmented full evaluator).
    REQUIRE_FALSE(art.u_carrier.empty());
    bool saw_packet_active = false;
    const int h_max = static_cast<int>(std::min<mt::usize>(art.u_carrier.size(), 6));
    for (int h = 0; h < h_max; ++h) {
        const mt::real hh = static_cast<mt::real>(h);
        const mt::cplx u_smooth = mt::evaluate_smooth_future_section(art.jet_minus, hh);
        const mt::cplx u_full   = mt::evaluate_full_future_section(art.jet_minus, hh);
        REQUIRE(std::abs(art.u_carrier[static_cast<mt::usize>(h)] - u_smooth) < 1e-9);
        if (std::abs(u_full - u_smooth) > 1e-9) {
            saw_packet_active = true;
            REQUIRE(std::abs(art.u_carrier[static_cast<mt::usize>(h)] - u_full) > 1e-9);
        }
    }

    if (!saw_packet_active) {
        INFO("packet term inactive on sampled h-window; smooth/full split still verified by explicit evaluator contract test");
    }
}

TEST_CASE("future-section smooth/full evaluators diverge for h>0 when packets activate",
          "[framework][future_section][split]") {
    mt::FutureSection fs;
    fs.valid = true;
    fs.A_phi = {mt::cplx(1.0, 0.0), mt::cplx(0.5, 0.0)};   // 1 + 0.5 h
    fs.sigma = {1.0};
    fs.A_pi  = {mt::Vec<mt::cplx>{mt::cplx(2.0, 0.0)}};    // +2 when h >= 1

    const mt::real h_left  = 0.5;
    const mt::real h_right = 2.0;

    const mt::cplx smooth_left = mt::evaluate_smooth_future_section(fs, h_left);
    const mt::cplx full_left   = mt::evaluate_full_future_section(fs, h_left);
    REQUIRE(std::abs(smooth_left - full_left) < 1e-12);

    const mt::cplx smooth_right = mt::evaluate_smooth_future_section(fs, h_right);
    const mt::cplx full_right   = mt::evaluate_full_future_section(fs, h_right);
    REQUIRE(std::abs(full_right - smooth_right) > 1e-12);
}
