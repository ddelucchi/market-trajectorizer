// Deterministic JSON report serialization for backtest and walk-forward runs.
// Format: pretty-printed top-level object with arrays in line form.

#include "mt/api/json_io.hpp"
#include "mt/api/result_schema.hpp"
#include "mt/backtest/report.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <type_traits>

namespace mt {

namespace {

real json_safe(real v) {
    return std::isfinite(v) ? v : 0.0;
}

void emit_metrics(std::ostringstream& os, const Metrics& m, const std::string& indent) {
    os << indent << "{\n";
    auto kv = [&](const char* k, real v, bool last) {
        os << indent << "  \"" << k << "\": " << std::setprecision(17) << json_safe(v)
           << (last ? "\n" : ",\n");
    };
    kv("cagr", m.cagr, false);
    kv("sharpe", m.sharpe, false);
    kv("sortino", m.sortino, false);
    kv("max_drawdown", m.max_drawdown, false);
    kv("calmar", m.calmar, false);
    kv("hit_rate", m.hit_rate, false);
    kv("profit_factor", m.profit_factor, false);
    kv("turnover", m.turnover, false);
    kv("avg_trade", m.avg_trade, false);
    kv("exposure", m.exposure, false);
    kv("pnl_per_bar", m.pnl_per_bar, false);
    kv("path_deviation", m.path_deviation, true);
    os << indent << "}";
}

void emit_curve(std::ostringstream& os, const EquityCurve& c, const std::string& indent) {
    auto vec = [&](const char* name, auto& v, bool last) {
        os << indent << "  \"" << name << "\": [";
        for (usize i = 0; i < v.size(); ++i) {
            if (i) os << ",";
            using Elem = std::decay_t<decltype(v[i])>;
            if constexpr (std::is_floating_point_v<Elem>) {
                os << std::setprecision(17) << json_safe(static_cast<real>(v[i]));
            } else {
                os << v[i];
            }
        }
        os << "]" << (last ? "\n" : ",\n");
    };
    os << indent << "{\n";
    vec("ts",     c.ts,     false);
    vec("equity", c.equity, false);
    vec("pnl",    c.pnl,    false);
    vec("dd",     c.dd,     true);
    os << indent << "}";
}

void emit_fills(std::ostringstream& os, const Vec<Fill>& fills, const std::string& indent) {
    os << indent << "[";
    for (usize i = 0; i < fills.size(); ++i) {
        if (i) os << ",";
        os << "{\"ts\":" << fills[i].ts
              << ",\"px\":"  << std::setprecision(17) << json_safe(fills[i].px)
              << ",\"qty\":" << json_safe(fills[i].qty)
              << ",\"fee\":" << json_safe(fills[i].fee) << "}";
    }
    os << "]";
}

void emit_fidelity(std::ostringstream& os, const FidelityMetrics& f, const std::string& indent) {
    os << indent << "{\n";
    os << indent << "  \"anchors_evaluated\": " << f.anchors_evaluated << ",\n";
    os << indent << "  \"lead_sign_agreement\": " << std::setprecision(17) << json_safe(f.lead_sign_agreement) << ",\n";
    os << indent << "  \"turning_point_alignment\": " << json_safe(f.turning_point_alignment) << ",\n";
    os << indent << "  \"path_rmse\": " << json_safe(f.path_rmse) << ",\n";
    os << indent << "  \"event_boundary_overlap\": " << json_safe(f.event_boundary_overlap) << ",\n";
    os << indent << "  \"drawdown_shape_similarity\": " << json_safe(f.drawdown_shape_similarity) << "\n";
    os << indent << "}";
}

void emit_stratified(std::ostringstream& os, const StratifiedMetrics& s, const std::string& indent) {
    os << indent << "{\n";
    os << indent << "  \"anchors\": " << s.anchors << ",\n";
    os << indent << "  \"signals\": " << s.signals << ",\n";
    os << indent << "  \"metrics\":\n";
    emit_metrics(os, s.metrics, indent + "  ");
    os << ",\n";
    os << indent << "  \"fidelity\":\n";
    emit_fidelity(os, s.fidelity, indent + "  ");
    os << "\n";
    os << indent << "}";
}

void emit_clause_splits(std::ostringstream& os,
                        const Vec<ClauseStratifiedMetrics>& clauses,
                        const std::string& indent) {
    os << indent << "[\n";
    for (usize i = 0; i < clauses.size(); ++i) {
        const auto& c = clauses[i];
        os << indent << "  {\n";
        os << indent << "    \"clause\": \"" << c.clause << "\",\n";
        os << indent << "    \"anchors\": " << c.anchors << ",\n";
        os << indent << "    \"signals\": " << c.signals << ",\n";
        os << indent << "    \"metrics\":\n";
        emit_metrics(os, c.metrics, indent + "    ");
        os << ",\n";
        os << indent << "    \"fidelity\":\n";
        emit_fidelity(os, c.fidelity, indent + "    ");
        os << "\n";
        os << indent << "  }";
        if (i + 1 < clauses.size()) os << ",";
        os << "\n";
    }
    os << indent << "]";
}

void emit_window_diagnostics(std::ostringstream& os,
                             const WalkForwardWindowDiagnostics& d,
                             const std::string& indent) {
    os << indent << "{\n";

    auto emit_named = [&](const char* name, const StratifiedMetrics& s, bool last) {
        os << indent << "  \"" << name << "\":\n";
        emit_stratified(os, s, indent + "  ");
        os << (last ? "\n" : ",\n");
    };

    emit_named("all", d.all, false);
    emit_named("authoritative", d.authoritative, false);
    emit_named("non_authoritative", d.non_authoritative, false);
    emit_named("local_sector_valid", d.local_sector_valid, false);
    emit_named("local_sector_invalid", d.local_sector_invalid, false);
    emit_named("packet_anchor", d.packet_anchor, false);
    emit_named("no_packet_anchor", d.no_packet_anchor, false);
    emit_named("recurrence_r0", d.recurrence_r0, false);
    emit_named("recurrence_r1_2", d.recurrence_r1_2, false);
    emit_named("recurrence_r3_5", d.recurrence_r3_5, false);
    emit_named("recurrence_r6_plus", d.recurrence_r6_plus, false);
    emit_named("spectral_m0", d.spectral_m0, false);
    emit_named("spectral_m1", d.spectral_m1, false);
    emit_named("spectral_m2_3", d.spectral_m2_3, false);
    emit_named("spectral_m4_plus", d.spectral_m4_plus, false);
    emit_named("theorem_hcount_0", d.theorem_hcount_0, false);
    emit_named("theorem_hcount_1_4", d.theorem_hcount_1_4, false);
    emit_named("theorem_hcount_5_16", d.theorem_hcount_5_16, false);
    emit_named("theorem_hcount_17_plus", d.theorem_hcount_17_plus, false);
    os << indent << "  \"failed_clause_primary\":\n";
    emit_clause_splits(os, d.failed_clause_primary, indent + "  ");
    os << "\n";

    os << indent << "}";
}

}  // namespace

void write_backtest_report(const BacktestResult& br, const Metrics& m, std::string_view out_json) {
    std::ostringstream os;
    os << "{\n";
    os << "  \"schema\": \"backtest_v1\",\n";
    os << "  \"metrics\":\n"; emit_metrics(os, m, "  "); os << ",\n";
    os << "  \"curve\":\n";   emit_curve  (os, br.curve, "  "); os << ",\n";
    os << "  \"fills\": ";    emit_fills  (os, br.fills, "");   os << "\n";
    os << "}\n";
    json_io::write_text(out_json, os.str());
}

void write_walk_forward_report(const WalkForwardResult& wfr, std::string_view out_json) {
    std::ostringstream os;
    os << "{\n";
    os << "  \"schema\": \"walk_forward_v2\",\n";
    os << "  \"authority_mode\": {\"research_non_authoritative\": "
       << (wfr.research_non_authoritative ? "true" : "false")
         << ",\"research_heuristic_signal\": " << (wfr.research_heuristic_signal ? "true" : "false")
       << ",\"authoritative_anchors\": " << wfr.authoritative_anchors
         << ",\"non_authoritative_anchors\": " << wfr.non_authoritative_anchors
         << ",\"rejected_anchors\": " << wfr.rejected_anchors
         << ",\"research_proxy_energy_signals\": " << wfr.research_proxy_energy_signals
         << "},\n";
    os << "  \"windows\": [";
    for (usize i = 0; i < wfr.windows.size(); ++i) {
        const auto& w = wfr.windows[i];
        if (i) os << ",";
        os << "{\"train_start\":" << w.train_start
           << ",\"train_end\":"   << w.train_end
           << ",\"test_start\":"  << w.test_start
           << ",\"test_end\":"    << w.test_end << "}";
    }
    os << "],\n";
    os << "  \"per_window\": [";
    for (usize i = 0; i < wfr.per_window.size(); ++i) {
        if (i) os << ",";
        emit_metrics(os, wfr.per_window[i], "  ");
    }
    os << "],\n";
    os << "  \"per_window_diagnostics\": [\n";
    for (usize i = 0; i < wfr.per_window_diagnostics.size(); ++i) {
        emit_window_diagnostics(os, wfr.per_window_diagnostics[i], "    ");
        if (i + 1 < wfr.per_window_diagnostics.size()) os << ",";
        os << "\n";
    }
    os << "  ],\n";
    os << "  \"aggregate\":\n"; emit_metrics(os, wfr.aggregate, "  "); os << ",\n";
    os << "  \"aggregate_diagnostics\":\n";
    emit_window_diagnostics(os, wfr.aggregate_diagnostics, "  ");
    os << "\n";
    os << "}\n";
    json_io::write_text(out_json, os.str());
}

}  // namespace mt
