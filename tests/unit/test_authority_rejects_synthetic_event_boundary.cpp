#include <catch2/catch_test_macros.hpp>

#include "mt/core/config.hpp"
#include "mt/engine/extraction_engine.hpp"

#include <cmath>

namespace {

// Volatile non-smooth path so the section is not flagged single_sector_smooth.
mt::MarketStateColumns make_volatile_state(int n) {
    mt::MarketStateColumns s;
    s.resize(static_cast<mt::usize>(n));
    for (int i = 0; i < n; ++i) {
        const double base = std::log(100.0)
                          + 0.05 * std::sin(0.31 * static_cast<double>(i))
                          + 0.03 * std::sin(0.97 * static_cast<double>(i));
        s.x0[static_cast<mt::usize>(i)] = base;
        s.x1[static_cast<mt::usize>(i)] = base + 0.001;
        s.x2[static_cast<mt::usize>(i)] = base - 0.001;
        s.x3[static_cast<mt::usize>(i)] = base;
        s.x4[static_cast<mt::usize>(i)] = 1.0 + 0.1 * static_cast<double>(i);
    }
    return s;
}

}  // namespace

TEST_CASE("synthetic event-boundary fallback is gated off by default in production",
          "[framework][authority][synthetic_event_boundary]") {
    mt::ChangePointConfig cp_cfg;
    // Production default: synthetic fallback MUST be disabled.
    REQUIRE_FALSE(cp_cfg.allow_synthetic_event_boundary_research_only);

    mt::SectionConfig sec_cfg;
    mt::ChangePointSet empty_cps;
    const mt::MarketStateColumns s = make_volatile_state(64);

    // Production path: no synthetic rescue.  Either boundaries are empty
    // (production fails closed) OR they come from genuine packet detection.
    mt::PiecewiseSection prod_sec = mt::build_piecewise_section_cpu(s, /*t_anchor=*/40,
                                                                    empty_cps, sec_cfg, cp_cfg);
    if (!prod_sec.event_boundaries.empty()) {
        REQUIRE(prod_sec.event_boundaries_from_genuine_packets);
    } else {
        REQUIRE_FALSE(prod_sec.event_boundaries_from_genuine_packets);
    }

    // Research path: synthetic fallback may insert a boundary; if so, the
    // provenance flag MUST be false so the authority gate rejects it.
    mt::ChangePointConfig research_cp_cfg = cp_cfg;
    research_cp_cfg.allow_synthetic_event_boundary_research_only = true;
    research_cp_cfg.single_sector_smooth_regime_only = false;

    mt::PiecewiseSection research_sec = mt::build_piecewise_section_cpu(s, /*t_anchor=*/40,
                                                                        empty_cps, sec_cfg,
                                                                        research_cp_cfg);
    if (research_sec.event_boundaries.size() > prod_sec.event_boundaries.size()) {
        // Synthetic insertion happened.  Must NOT be marked genuine.
        REQUIRE_FALSE(research_sec.event_boundaries_from_genuine_packets);
    }
}
