#include "mt/api/artifact_writer.hpp"
#include "mt/api/json_io.hpp"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace mt {

namespace {

std::string escape_json(std::string_view in) {
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

void write_failed_clauses(std::ostringstream& s, const Vec<std::string>& failed) {
    s << "[";
    for (usize i = 0; i < failed.size(); ++i) {
        if (i) s << ",";
        s << "\"" << escape_json(failed[i]) << "\"";
    }
    s << "]";
}

void write_int_array(std::ostringstream& s, const Vec<int>& v) {
    s << "[";
    for (usize i = 0; i < v.size(); ++i) {
        if (i) s << ",";
        s << v[i];
    }
    s << "]";
}

void write_real_array(std::ostringstream& s, const Vec<real>& v) {
    s << "[";
    for (usize i = 0; i < v.size(); ++i) {
        if (i) s << ",";
        s << std::setprecision(17) << v[i];
    }
    s << "]";
}

void write_complex_array(std::ostringstream& s, const Vec<cplx>& v) {
    s << "[";
    for (usize i = 0; i < v.size(); ++i) {
        if (i) s << ",";
        s << "{\"re\":" << std::setprecision(17) << v[i].real()
          << ",\"im\":" << std::setprecision(17) << v[i].imag() << "}";
    }
    s << "]";
}

void write_complex_matrix(std::ostringstream& s, const Vec<Vec<cplx>>& m) {
    s << "[";
    for (usize i = 0; i < m.size(); ++i) {
        if (i) s << ",";
        write_complex_array(s, m[i]);
    }
    s << "]";
}

void write_real_matrix(std::ostringstream& s, const Vec<Vec<real>>& m) {
    s << "[";
    for (usize i = 0; i < m.size(); ++i) {
        if (i) s << ",";
        write_real_array(s, m[i]);
    }
    s << "]";
}

void write_market_state_anchor(std::ostringstream& s,
                               const MarketStateColumns& ms,
                               int anchor_idx) {
    const usize idx = static_cast<usize>(std::max(anchor_idx, 0));
    auto at = [&](const Vec<real>& x) -> real {
        return idx < x.size() ? x[idx] : 0.0;
    };
    s << "{\"idx\":" << anchor_idx
      << ",\"x0\":" << std::setprecision(17) << at(ms.x0)
      << ",\"x1\":" << std::setprecision(17) << at(ms.x1)
      << ",\"x2\":" << std::setprecision(17) << at(ms.x2)
      << ",\"x3\":" << std::setprecision(17) << at(ms.x3)
      << ",\"x4\":" << std::setprecision(17) << at(ms.x4)
      << "}";
}

void write_spectral_modes(std::ostringstream& s, const SpectralDecomposition& sd) {
    s << "[";
    for (usize i = 0; i < sd.modes.size(); ++i) {
        if (i) s << ",";
        const auto& m = sd.modes[i];
        s << "{\"multiplicity\":" << m.multiplicity
          << ",\"lambda\":{\"re\":" << std::setprecision(17) << m.lambda.real()
          << ",\"im\":" << std::setprecision(17) << m.lambda.imag() << "}"
          << ",\"gamma\":";
        write_complex_array(s, m.gamma);
        s << "}";
    }
    s << "]";
}

void write_event_packets(std::ostringstream& s, const Vec<EventPacket>& packets) {
    s << "[";
    for (usize i = 0; i < packets.size(); ++i) {
        if (i) s << ",";
        const auto& p = packets[i];
        s << "{\"index\":" << p.index
          << ",\"gap_event\":" << std::setprecision(17) << p.gap_event
          << ",\"range_explosion\":" << std::setprecision(17) << p.range_explosion
          << ",\"body_inversion\":" << std::setprecision(17) << p.body_inversion
          << ",\"volume_shock\":" << std::setprecision(17) << p.volume_shock
          << ",\"wick_rejection\":" << std::setprecision(17) << p.wick_rejection
          << ",\"compression_break\":" << std::setprecision(17) << p.compression_break
          << ",\"persistent_drift\":" << std::setprecision(17) << p.persistent_drift
          << ",\"exhaustion\":" << std::setprecision(17) << p.exhaustion
          << ",\"joint_score\":" << std::setprecision(17) << p.joint_score
          << "}";
    }
    s << "]";
}

usize count_total_pi_terms(const FutureSection& f) {
    usize total = 0;
    for (const auto& row : f.A_pi) total += row.size();
    return total;
}

}  // namespace

ArtifactPaths build_artifact_paths(std::string_view root,
                                   std::string_view symbol,
                                   std::string_view anchor_label) {
    namespace fs = std::filesystem;

    fs::path base = fs::path(std::string(root))
                  / std::string(symbol)
                  / std::string(anchor_label);
    fs::create_directories(base);

    ArtifactPaths out;
    out.future_section  = (base / "future_section.json").string();
    out.past_jet_state  = (base / "past_jet_state.json").string();
    out.phase_torus_coefficients = (base / "phase_torus_coefficients.json").string();
    out.jet_state = (base / "jet_state.json").string();
    out.future_section_from_phase_torus = (base / "future_section_from_phase_torus.json").string();
    out.phase_torus_residuals = (base / "phase_torus_residuals.json").string();
    out.evolution       = (base / "evolution.json").string();
    out.newton_gregory  = (base / "newton_gregory.json").string();
    out.matrix_carrier  = (base / "matrix_carrier.json").string();
    out.spectral_modes  = (base / "spectral_modes.json").string();
    out.rational_model  = (base / "rational_model.json").string();
    out.interstice_path = (base / "interstice_path.json").string();
    out.interstice_recurrence_residual_grid = (base / "interstice_recurrence_residual_grid.json").string();
    out.interstice_pde_residual_grid = (base / "interstice_pde_residual_grid.json").string();
    out.interstice_mode_transport = (base / "interstice_mode_transport.json").string();
    out.event_packets = (base / "event_packets.json").string();
    out.event_boundaries = (base / "event_boundaries.json").string();
    out.packet_transition_distance = (base / "packet_transition_distance.json").string();
    out.sector_scores = (base / "sector_scores.json").string();
    out.risk_field      = (base / "risk_field.json").string();
    out.signal          = (base / "signal.json").string();
    out.diagnostics     = (base / "diagnostics.json").string();
    return out;
}

void write_anchor_artifacts(const AnchorArtifacts& a,
                            const Signal&          sig,
                            const ArtifactPaths&   paths) {
    auto write_simple = [](std::string_view path, std::string_view payload) {
        json_io::write_text(path, payload);
    };

    {
        std::ostringstream s;
        s << "{";
        s << "\"t_anchor\":" << a.t_anchor
          << ",\"anchor_ts\":" << a.anchor_ts
          << ",\"phi_count\":" << a.jet_minus.A_phi.size()
          << ",\"sigma_count\":" << a.jet_minus.sigma.size()
          << ",\"pi_jump_count\":" << a.jet_minus.A_pi.size()
          << ",\"pi_term_count\":" << count_total_pi_terms(a.jet_minus)
          << ",\"recurrence_order\":" << a.recurrence.rec.r
          << ",\"jet_from_phase_torus\":" << (a.jet_from_phase_torus ? "true" : "false")
          << ",\"anchor_market_state\":";
        write_market_state_anchor(s, a.market_state, a.t_anchor);
        s << "}";
        write_simple(paths.future_section, s.str());
    }

    {
        std::ostringstream s;
        s << "{";
        s << "\"A_phi\":";
        write_complex_array(s, a.jet_minus.A_phi);
        s << ",\"sigma\":";
        write_real_array(s, a.jet_minus.sigma);
        s << ",\"A_pi\":";
        write_complex_matrix(s, a.jet_minus.A_pi);
        s << "}";
        write_simple(paths.past_jet_state, s.str());
    }

    {
        std::ostringstream s;
        s << "{";
        s << "\"Q\":" << a.extracted_phase_torus.Q
          << ",\"order_max\":" << a.extracted_phase_torus.order_max
          << ",\"rho_m\":" << std::setprecision(17) << a.extracted_phase_torus.rho_m
          << ",\"A\":";
        write_complex_array(s, a.extracted_phase_torus.A);
        s << "}";
        write_simple(paths.phase_torus_coefficients, s.str());
    }

    {
        std::ostringstream s;
        s << "{";
        s << "\"Q\":" << a.normalized_jet.Q
          << ",\"order_max\":" << a.normalized_jet.order_max
          << ",\"coeff\":";
        write_complex_array(s, a.normalized_jet.coeff);
        s << "}";
        write_simple(paths.jet_state, s.str());
    }

    {
        std::ostringstream s;
        s << "{\"future_section_from_phase_torus\":";
        write_complex_array(s, a.future_section_from_phase_torus);
        s << "}";
        write_simple(paths.future_section_from_phase_torus, s.str());
    }

    {
        std::ostringstream s;
        s << "{";
        s << "\"err_phase_torus_to_future_section\":" << std::setprecision(17) << a.authority.err_phase_torus_to_future_section
          << ",\"max_jet_normalization_residual\":" << std::setprecision(17) << a.authority.max_jet_normalization_residual
          << "}";
        write_simple(paths.phase_torus_residuals, s.str());
    }

    {
        std::ostringstream s;
        s << "{";
        s << "\"H\":" << a.evolution.U_newton.size()
          << ",\"closed_regime\":" << (a.evolution.closed_regime ? "true" : "false")
          << ",\"ok\":" << (a.evolution_ok ? "true" : "false")
          << ",\"carrier_presence\":{"
          << "\"continuous\":" << (a.authority.carrier_continuous_present ? "true" : "false")
          << ",\"jet\":" << (a.authority.carrier_jet_present ? "true" : "false")
          << ",\"newton\":" << (a.authority.carrier_newton_present ? "true" : "false")
          << ",\"matrix\":" << (a.authority.carrier_matrix_present ? "true" : "false")
          << ",\"rational\":" << (a.authority.carrier_rational_present ? "true" : "false")
          << ",\"spectral\":" << (a.authority.carrier_spectral_present ? "true" : "false")
          << "}"
          << ",\"carrier_reason_missing\":{"
          << "\"rational\":\"" << escape_json(a.authority.carrier_rational_reason_missing) << "\""
          << ",\"spectral\":\"" << escape_json(a.authority.carrier_spectral_reason_missing) << "\""
          << "}"
          << ",\"jet_semigroup_from_normalized_jet\":"
          << (a.evolution.jet_semigroup_from_normalized_jet ? "true" : "false")
          << ",\"theorem_residual_on_phase_torus_path\":"
          << (a.evolution.theorem_residual_on_phase_torus_path ? "true" : "false")
          << ",\"max_pairwise_abs_err\":" << std::setprecision(17) << a.evolution.max_pairwise_abs_err
          << ",\"err_continuous_jet\":" << std::setprecision(17) << a.evolution.err_continuous_jet
          << ",\"err_jet_newton\":" << std::setprecision(17) << a.evolution.err_jet_newton
          << ",\"err_newton_matrix\":" << std::setprecision(17) << a.evolution.err_newton_matrix
          << ",\"err_matrix_rational\":" << std::setprecision(17) << a.evolution.err_matrix_rational
          << ",\"err_rational_spectral\":" << std::setprecision(17) << a.evolution.err_rational_spectral
          << ",\"U_continuous\":";
        write_complex_array(s, a.evolution.U_continuous);
        s << ",\"U_jet\":";
        write_complex_array(s, a.evolution.U_jet);
        s << ",\"U_newton\":";
        write_complex_array(s, a.evolution.U_newton);
        s << ",\"U_matrix\":";
        write_complex_array(s, a.evolution.U_matrix);
        s << ",\"U_rational\":";
        write_complex_array(s, a.evolution.U_rational);
        s << ",\"U_spectral\":";
        write_complex_array(s, a.evolution.U_spectral);
        s << "}";
        write_simple(paths.evolution, s.str());
    }

    {
        std::ostringstream s;
        s << "{";
        s << "\"input_sequence\":";
        write_complex_array(s, a.evolution.newton_input_sequence);
        s << ",\"backward_differences\":";
        write_complex_array(s, a.evolution.newton_backward_differences);
        s << ",\"eta_grid\":";
        write_real_array(s, a.evolution.newton_eta_grid);
        s << ",\"basis_values\":";
        write_real_matrix(s, a.evolution.newton_basis_values);
        s << ",\"partial_sums\":";
        write_complex_matrix(s, a.evolution.newton_partial_sums);
        s << ",\"U_newton\":";
        write_complex_array(s, a.evolution.U_newton);
        s << "}";
        write_simple(paths.newton_gregory, s.str());
    }

    {
        std::ostringstream s;
        s << "{\"U_matrix\":";
        write_complex_array(s, a.evolution.U_matrix);
        s << "}";
        write_simple(paths.matrix_carrier, s.str());
    }

    {
        std::ostringstream s;
        s << "{";
        s << "\"r\":" << a.recurrence.spectrum.r
          << ",\"mode_count\":" << a.recurrence.spectrum.modes.size()
          << ",\"modes\":";
        write_spectral_modes(s, a.recurrence.spectrum);
        s << "}";
        write_simple(paths.spectral_modes, s.str());
    }

    {
        std::ostringstream s;
        s << "{";
        s << "\"deg_p\":" << a.recurrence.rational.P.size()
          << ",\"deg_q\":" << a.recurrence.rational.Q.size()
          << ",\"P\":";
        write_complex_array(s, a.recurrence.rational.P);
        s << ",\"Q\":";
        write_complex_array(s, a.recurrence.rational.Q);
        s << "}";
        write_simple(paths.rational_model, s.str());
    }

    {
        std::ostringstream s;
        s << "{";
        s << "\"H\":" << a.trajectory.H
          << ",\"s_count\":" << a.trajectory.s_count
          << ",\"ds\":" << std::setprecision(17) << a.trajectory.ds
          << ",\"h_grid\":";
        write_real_array(s, a.trajectory.h_grid);
        s << ",\"s_grid\":";
        write_real_array(s, a.trajectory.s_grid);
        s << ",\"kappa\":";
        write_real_array(s, a.trajectory.kappa);
        s << ",\"T\":";
        write_complex_array(s, a.trajectory.T);
        s << ",\"L\":";
        write_complex_array(s, a.trajectory.L);
        s << ",\"P\":";
        write_real_array(s, a.trajectory.P);
        s << ",\"R\":";
        write_complex_array(s, a.trajectory.R);
        s << ",\"T_grid\":";
        write_complex_array(s, a.trajectory.T_grid);
        s << ",\"projected_log_path\":";
        write_real_array(s, sig.projected_log_path);
        s << ",\"projected_return_path\":";
        write_real_array(s, sig.projected_return_path);
        s << ",\"recurrence_order\":" << sig.recurrence_order
          << ",\"spectral_mode_count\":" << sig.spectral_mode_count;
        s << "}";
        write_simple(paths.interstice_path, s.str());
    }

    {
        std::ostringstream s;
        s << "{\"s_count\":" << a.trajectory.s_count
          << ",\"H\":" << a.trajectory.H
          << ",\"residual_grid\":";
        write_real_array(s, a.interstice_recurrence_residual_grid);
        s << "}";
        write_simple(paths.interstice_recurrence_residual_grid, s.str());
    }

    {
        std::ostringstream s;
        s << "{\"s_count\":" << a.trajectory.s_count
          << ",\"H\":" << a.trajectory.H
          << ",\"residual_grid\":";
        write_real_array(s, a.interstice_pde_residual_grid);
        s << "}";
        write_simple(paths.interstice_pde_residual_grid, s.str());
    }

    {
        std::ostringstream s;
        s << "{";
        s << "\"s_grid\":";
        write_real_array(s, a.trajectory.s_grid);
        s << ",\"kappa\":";
        write_real_array(s, a.trajectory.kappa);
        s << ",\"h_grid\":";
        write_real_array(s, a.trajectory.h_grid);
        s << "}";
        write_simple(paths.interstice_mode_transport, s.str());
    }

    {
        std::ostringstream s;
        write_event_packets(s, a.section.event_packets);
        write_simple(paths.event_packets, s.str());
    }

    {
        std::ostringstream s;
        write_int_array(s, a.section.event_boundaries);
        write_simple(paths.event_boundaries, s.str());
    }

    {
        std::ostringstream s;
        write_real_array(s, a.section.packet_transition_distance);
        write_simple(paths.packet_transition_distance, s.str());
    }

    {
        std::ostringstream s;
        write_real_array(s, a.section.sector_scores);
        write_simple(paths.sector_scores, s.str());
    }

    {
        std::ostringstream s;
        s << "{";
        s << "\"energy_framework_H\":" << a.risk.energy_framework.size()
          << ",\"energy_proxy_H\":" << a.risk.energy_proxy.size()
          << ",\"drawdown_proxy_H\":" << a.risk.drawdown_proxy.size()
          << ",\"curvature_H\":" << a.risk.curvature.size()
          << ",\"uncertainty_proxy_H\":" << a.risk.uncertainty_proxy.size()
          << ",\"used_proxy_energy\":" << (sig.used_proxy_energy ? "true" : "false")
                    << ",\"framework_energy_missing_reason\":\"" << escape_json(sig.framework_energy_missing_reason) << "\""
                    << ",\"energy_semantics\":{\"framework\":\"theorem_side_energy\",\"proxy\":\"realized_trajectory_proxy\"}"
          << ",\"energy_framework\":";
        write_real_array(s, a.risk.energy_framework);
        s << ",\"energy_proxy\":";
        write_real_array(s, a.risk.energy_proxy);
        s << ",\"drawdown_proxy\":";
        write_real_array(s, a.risk.drawdown_proxy);
        s << ",\"curvature\":";
        write_real_array(s, a.risk.curvature);
        s << ",\"uncertainty_proxy\":";
        write_real_array(s, a.risk.uncertainty_proxy);
        s << ",\"framework_energy_path\":";
        write_real_array(s, sig.framework_energy_path);
        s << ",\"proxy_energy_path\":";
        write_real_array(s, sig.proxy_energy_path);
        s << ",\"curvature_path\":";
        write_real_array(s, sig.curvature_path);
        s << ",\"uncertainty_proxy_path\":";
        write_real_array(s, sig.uncertainty_proxy_path);
        s << "}";
        write_simple(paths.risk_field, s.str());
    }

    {
        std::ostringstream s;
        s << "{";
        s << "\"side\":" << sig.side
          << ",\"strength\":" << std::setprecision(17) << sig.strength
          << ",\"entry\":" << std::setprecision(17) << sig.entry_px
          << ",\"stop\":" << std::setprecision(17) << sig.stop_px
          << ",\"take\":" << std::setprecision(17) << sig.take_px
          << ",\"score_plus\":" << std::setprecision(17) << sig.score_plus
          << ",\"score_minus\":" << std::setprecision(17) << sig.score_minus
          << ",\"score_dominant\":" << std::setprecision(17) << sig.score_dominant
          << ",\"theorem_authoritative\":" << (sig.theorem_authoritative ? "true" : "false")
          << ",\"implementation_authoritative\":" << (sig.implementation_authoritative ? "true" : "false")
          << ",\"research_non_authoritative\":" << (sig.research_non_authoritative ? "true" : "false")
          << ",\"research_heuristic_signal\":" << (sig.research_heuristic_signal ? "true" : "false")
          << ",\"authority_regime\":\"" << escape_json(sig.authority_regime) << "\""
          << ",\"used_proxy_energy\":" << (sig.used_proxy_energy ? "true" : "false")
          << ",\"framework_energy_missing_reason\":\"" << escape_json(sig.framework_energy_missing_reason) << "\""
          << ",\"closure_claimed\":" << (sig.closure_claimed ? "true" : "false")
          << ",\"closure_verified\":" << (sig.closure_verified ? "true" : "false")
          << ",\"recurrence_order\":" << sig.recurrence_order
          << ",\"spectral_mode_count\":" << sig.spectral_mode_count
          << ",\"event_packet_count\":" << sig.event_packet_count
          << ",\"event_joint_score_path\":";
        write_real_array(s, sig.event_joint_score_path);
        s << ",\"failed_clauses\":";
        write_failed_clauses(s, sig.failed_clauses);
        s << "}";
        write_simple(paths.signal, s.str());
    }

    {
        const bool closure_ok = !a.authority.closure_claimed || a.authority.closure_verified;
        std::ostringstream s;
        s << "{";
        s << "\"authoritative\":" << (a.authority.authoritative ? "true" : "false")
          << ",\"production_ready\":" << (a.authority.production_ready ? "true" : "false")
          << ",\"theorem_authoritative\":" << (a.authority.theorem_authoritative ? "true" : "false")
          << ",\"implementation_authoritative\":" << (a.authority.implementation_authoritative ? "true" : "false")
                    << ",\"theorem_slice_single_axis\":" << (a.authority.theorem_slice_single_axis ? "true" : "false")
                    << ",\"theorem_local_sector_verified\":" << (a.authority.theorem_local_sector_verified ? "true" : "false")
          << ",\"authority_regime\":\"" << escape_json(std::string(authority_regime_name(a.authority.regime))) << "\""
          << ",\"research_non_authoritative\":" << (sig.research_non_authoritative ? "true" : "false")
          << ",\"research_heuristic_signal\":" << (sig.research_heuristic_signal ? "true" : "false")
          << ",\"used_proxy_energy\":" << (sig.used_proxy_energy ? "true" : "false")
          << ",\"evolution_ok\":" << (a.evolution_ok ? "true" : "false")
          << ",\"failed_clauses\":";
        write_failed_clauses(s, a.authority.failed_clauses);
        s << ",\"nontrivial\":{"
          << "\"future_field\":" << (a.authority.nontrivial_future_field ? "true" : "false")
          << ",\"interstice_field\":" << (a.authority.nontrivial_interstice_field ? "true" : "false")
          << ",\"event_state_nonempty\":" << (a.authority.event_state_nonempty ? "true" : "false")
          << ",\"spectral_realization_nonempty\":" << (a.authority.spectral_realization_nonempty ? "true" : "false")
          << ",\"carrier_not_constant_identity\":" << (a.authority.carrier_not_constant_identity ? "true" : "false")
          << "}"
          << ",\"carrier_presence\":{"
          << "\"continuous\":" << (a.authority.carrier_continuous_present ? "true" : "false")
          << ",\"jet\":" << (a.authority.carrier_jet_present ? "true" : "false")
          << ",\"newton\":" << (a.authority.carrier_newton_present ? "true" : "false")
          << ",\"matrix\":" << (a.authority.carrier_matrix_present ? "true" : "false")
          << ",\"rational\":" << (a.authority.carrier_rational_present ? "true" : "false")
          << ",\"spectral\":" << (a.authority.carrier_spectral_present ? "true" : "false")
          << "}"
          << ",\"carrier_reason_missing\":{"
          << "\"rational\":\"" << escape_json(a.authority.carrier_rational_reason_missing) << "\""
          << ",\"spectral\":\"" << escape_json(a.authority.carrier_spectral_reason_missing) << "\""
          << "}"
          << ",\"closure\":{"
          << "\"claimed\":" << (a.authority.closure_claimed ? "true" : "false")
          << ",\"verified\":" << (a.authority.closure_verified ? "true" : "false")
          << ",\"order\":" << a.recurrence.rec.r
          << ",\"max_residual\":" << std::setprecision(17) << a.authority.max_closure_residual
          << "}"
          << ",\"interstice\":{"
          << "\"verified\":" << (a.authority.interstice_verified ? "true" : "false")
          << ",\"max_pde_residual\":" << std::setprecision(17) << a.authority.max_interstice_pde_residual
          << ",\"max_recurrence_residual\":" << std::setprecision(17) << a.authority.max_interstice_recurrence_residual
          << "}"
          << ",\"phase_torus_chain\":{"
          << "\"phase_torus_on_path\":" << (a.authority.phase_torus_path_verified ? "true" : "false")
          << ",\"jet_normalization_identity\":" << (a.authority.jet_normalization_identity_verified ? "true" : "false")
          << ",\"jet_semigroup_from_normalized_jet\":"
          << (a.evolution.jet_semigroup_from_normalized_jet ? "true" : "false")
          << ",\"theorem_residual_evaluated\":"
          << (a.evolution.theorem_residual_on_phase_torus_path ? "true" : "false")
          << "}"
          << ",\"causality\":{"
          << "\"anchor_truncation\":" << (a.authority.causality_anchor_truncation_verified ? "true" : "false")
          << ",\"carrier_from_past_jet\":" << (a.authority.causality_carrier_verified ? "true" : "false")
          << "}"
          << ",\"gpu_mode\":\"" << escape_json(a.authority.gpu_mode) << "\""
          << ",\"theorem_valid_sector\":{"
          << "\"h_max\":" << a.authority.theorem_valid_h_max
          << ",\"h_count\":" << a.authority.theorem_valid_h_count
          << ",\"fraction\":" << std::setprecision(17) << a.authority.theorem_valid_h_fraction
          << ",\"coeff_cap\":" << a.authority.theorem_valid_h_coeff_cap
          << ",\"packet_cap\":" << a.authority.theorem_valid_h_packet_cap
          << ",\"condition_cap\":" << a.authority.theorem_valid_h_condition_cap
          << ",\"residual_cap\":" << a.authority.theorem_valid_h_residual_cap
          << ",\"coeff_radius_estimate\":" << std::setprecision(17) << a.authority.theorem_coeff_radius_estimate
          << ",\"recurrence_condition\":" << std::setprecision(17) << a.authority.theorem_recurrence_condition
          << "}"
          << ",\"max_theorem_residual\":" << std::setprecision(17) << a.authority.max_theorem_residual
          << ",\"max_jet_normalization_residual\":" << std::setprecision(17) << a.authority.max_jet_normalization_residual
          << ",\"err_phase_torus_to_future_section\":" << std::setprecision(17) << a.authority.err_phase_torus_to_future_section
          << ",\"acceptance_clauses\":{"
          << "\"evolution_identity\":" << (a.evolution_ok ? "true" : "false")
          << ",\"theorem_authoritative\":" << (a.authority.theorem_authoritative ? "true" : "false")
          << ",\"theorem_local_valid_sector\":" << (a.authority.theorem_local_sector_verified ? "true" : "false")
          << ",\"implementation_authoritative\":" << (a.authority.implementation_authoritative ? "true" : "false")
          << ",\"closure_verified_or_not_claimed\":" << (closure_ok ? "true" : "false")
          << ",\"interstice_verified\":" << (a.authority.interstice_verified ? "true" : "false")
          << ",\"causality_anchor_truncation\":" << (a.authority.causality_anchor_truncation_verified ? "true" : "false")
          << ",\"causality_carrier\":" << (a.authority.causality_carrier_verified ? "true" : "false")
          << ",\"phase_torus_path\":" << (a.authority.phase_torus_path_verified ? "true" : "false")
          << ",\"jet_normalization_identity\":" << (a.authority.jet_normalization_identity_verified ? "true" : "false")
          << ",\"di_projection\":" << (a.authority.di_projection_verified ? "true" : "false")
          << ",\"nontrivial_future_field\":" << (a.authority.nontrivial_future_field ? "true" : "false")
          << ",\"nontrivial_interstice_field\":" << (a.authority.nontrivial_interstice_field ? "true" : "false")
          << ",\"event_state_nonempty\":" << (a.authority.event_state_nonempty ? "true" : "false")
          << ",\"spectral_realization_nonempty\":" << (a.authority.spectral_realization_nonempty ? "true" : "false")
          << ",\"carrier_not_constant_identity\":" << (a.authority.carrier_not_constant_identity ? "true" : "false")
          << "}"
          << "}";
        write_simple(paths.diagnostics, s.str());
    }
}

}  // namespace mt
