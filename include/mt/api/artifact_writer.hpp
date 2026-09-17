#pragma once
#include "mt/backtest/signal_rules.hpp"
#include "mt/engine/pipeline.hpp"
#include <string>
#include <string_view>

namespace mt {

struct ArtifactPaths {
    std::string future_section;
    std::string past_jet_state;
    std::string phase_torus_coefficients;
    std::string jet_state;
    std::string future_section_from_phase_torus;
    std::string phase_torus_residuals;
    std::string evolution;
    std::string newton_gregory;
    std::string matrix_carrier;
    std::string spectral_modes;
    std::string rational_model;
    std::string interstice_path;
    std::string interstice_recurrence_residual_grid;
    std::string interstice_pde_residual_grid;
    std::string interstice_mode_transport;
    std::string event_packets;
    std::string event_boundaries;
    std::string packet_transition_distance;
    std::string sector_scores;
    std::string risk_field;
    std::string signal;
    std::string diagnostics;
};

ArtifactPaths build_artifact_paths(std::string_view root,
                                   std::string_view symbol,
                                   std::string_view anchor_label);

void write_anchor_artifacts(const AnchorArtifacts& a,
                            const Signal&          sig,
                            const ArtifactPaths&   paths);

}  // namespace mt
