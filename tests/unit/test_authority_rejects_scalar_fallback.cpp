#include <catch2/catch_test_macros.hpp>

#include "mt/engine/extraction_engine.hpp"
#include "mt/engine/pipeline.hpp"

#include <algorithm>

TEST_CASE("authority gate rejects past-jet that would have used scalar x3 fallback",
          "[framework][authority][past_jet_invalid]") {
    // Construct a degenerate normalized jet (empty coefficients) to simulate
    // the case where the canonical Q=5 normalization failed.  The
    // post-fallback-removal contract says build_past_jet_state_cpu MUST mark
    // the resulting FutureSection invalid and the authority gate MUST then
    // fail the `theorem.past_jet_invalid` clause.
    mt::PiecewiseSection sec{};
    sec.event_packets.clear();
    sec.event_boundaries.clear();
    sec.event_boundaries_from_genuine_packets = false;

    mt::JetState empty_jet{};  // empty coeff -> normalized jet absent
    REQUIRE(empty_jet.coeff.empty());

    mt::MarketStateColumns ms{};
    ms.resize(8);

    mt::FutureSection fs = mt::build_past_jet_state_cpu(sec, empty_jet, ms,
                                                       /*t_anchor=*/4,
                                                       /*N_phi=*/4,
                                                       /*N_pi=*/4,
                                                       /*L_max=*/2);
    REQUIRE_FALSE(fs.valid);
    REQUIRE(fs.invalid_reason == "normalized_jet_absent_scalar_fallback_forbidden");

    // Plug into a synthetic AnchorArtifacts and run the authority gate.
    mt::AnchorArtifacts art{};
    art.jet_minus = fs;
    art.section   = sec;
    // Provide a non-empty u_carrier so other clauses don't preempt the test.
    art.u_carrier.assign(4, mt::cplx(1.0, 0.0));

    mt::EngineConfig cfg;
    cfg.H = 4;
    cfg.recurrence_window = 4;
    mt::AuthoritativeStatus st = mt::validate_anchor_authoritativeness(art, cfg);

    const bool flagged = std::any_of(st.failed_clauses.begin(), st.failed_clauses.end(),
        [](const std::string& s) { return s == "theorem.past_jet_invalid"; });
    REQUIRE(flagged);
    REQUIRE_FALSE(st.production_ready);
    REQUIRE_FALSE(st.theorem_authoritative);
    REQUIRE(st.past_jet_invalid_reason == "normalized_jet_absent_scalar_fallback_forbidden");
}
