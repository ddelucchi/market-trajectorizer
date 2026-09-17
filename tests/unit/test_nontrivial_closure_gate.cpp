#include <catch2/catch_test_macros.hpp>

#include "mt/engine/pipeline.hpp"

#include <algorithm>

TEST_CASE("authority gate rejects trivial r=1 closure when surrounding structure is rich",
          "[framework][authority][closure_nontrivial]") {
    // Build an AnchorArtifacts that satisfies the surface event-state checks
    // but only has a trivial r=1 single-mode closure.  When the section
    // carries genuine event structure (boundaries + multiple packets), the
    // closure clause must be flagged as non-authoritative.
    mt::AnchorArtifacts art{};
    art.section.event_boundaries = {3};
    art.section.event_packets    = {
        mt::EventPacket{0, 1.0, 0.5, 0.2, 0.8, 0.1, 0.0, 0.4, 0.0, 3.0},
        mt::EventPacket{1, 0.2, 0.1, 0.0, 0.3, 0.0, 0.0, 0.2, 0.0, 0.8},
    };
    art.section.event_boundaries_from_genuine_packets = true;

    art.recurrence.rec.r = 1;
    art.recurrence.rec.c = {mt::cplx(0.5, 0.0)};
    art.recurrence.spectrum.modes.assign(1, mt::SpectralMode{});  // single mode

    art.jet_minus.valid = true;
    art.u_carrier.assign(4, mt::cplx(1.0, 0.0));

    mt::EngineConfig cfg;
    cfg.H = 4;
    cfg.recurrence_window = 4;

    mt::AuthoritativeStatus st = mt::validate_anchor_authoritativeness(art, cfg);

    const bool flagged = std::any_of(st.failed_clauses.begin(), st.failed_clauses.end(),
        [](const std::string& s) { return s == "recurrence.nontrivial_closure"; });
    REQUIRE(flagged);
    REQUIRE_FALSE(st.closure_nontrivial);
    REQUIRE_FALSE(st.production_ready);
}
