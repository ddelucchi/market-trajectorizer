#pragma once
#include "mt/state/jet_state.hpp"
#include <string>

namespace mt {

struct ChangePointSet {
    Vec<int> sigma_idx;        // bar indices σ_l
};

struct EventPacket {
    int  index = 0;
    real gap_event = 0.0;
    real range_explosion = 0.0;
    real body_inversion = 0.0;
    real volume_shock = 0.0;
    real wick_rejection = 0.0;
    real compression_break = 0.0;
    real persistent_drift = 0.0;
    real exhaustion = 0.0;
    real joint_score = 0.0;
};

struct PiecewiseSection {
    Vec<JetState>          phi_section; // base smooth sector
    Vec<Vec<JetState>>     pi_jumps;    // [l][n] jump sectors
    ChangePointSet         cps;
    Vec<EventPacket>       event_packets;
    Vec<int>               event_boundaries;
    Vec<real>              packet_transition_distance;
    Vec<real>              sector_scores;
    bool                   single_sector_smooth_regime_only = false;
    // Provenance: true iff event_boundaries arose from genuine OCHLV packet
    // dynamics (sectorizer + packet-event constructor).  False if the only
    // boundary was inserted by the deterministic max-transition fallback;
    // such boundaries are research-only and MUST NOT pass the production gate.
    bool                   event_boundaries_from_genuine_packets = false;
};

// Past-jet J_t^- in the form used by F_t[J_t^-](h).
struct FutureSection {
    Vec<cplx>      A_phi;     // n = 0..N_phi-1 (per-channel pack handled by caller)
    Vec<real>      sigma;     // l = 1..L_max
    Vec<Vec<cplx>> A_pi;      // [l][n], n = 0..N_pi-1

    // Theorem-faithfulness provenance.  `valid` is true only when the smooth
    // coefficients A_phi were built from the canonical Q=5 normalized jet
    // (extract_phase_torus_separable_cpu -> make_jet_from_extracted ->
    // axis_coefficient).  When the normalized jet is absent we leave A_phi
    // empty / zero and set valid=false; downstream authority MUST fail the
    // `theorem.past_jet_invalid` clause and never accept the carrier.
    // `invalid_reason` carries the precise machine-readable cause.
    bool           valid = false;
    std::string    invalid_reason;
};

}  // namespace mt
