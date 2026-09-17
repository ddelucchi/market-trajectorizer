#include "mt/backtest/signal_rules.hpp"

#include <algorithm>
#include <cmath>

namespace mt {

namespace {

inline real clamp01(real v) { return std::max(0.0, std::min(1.0, v)); }
inline real neg_part(real v) { return std::max(0.0, -v); }

struct FieldView {
    usize H = 0;
    int s_count = 0;
    bool have_grid = false;
};

FieldView make_field_view(const IntersticeTrajectory& tr) {
    FieldView fv;
    fv.H = !tr.h_grid.empty() ? tr.h_grid.size() : tr.T.size();
    fv.s_count = std::max(tr.s_count, 1);
    fv.have_grid = (fv.H > 0) && (tr.T_grid.size() >= static_cast<usize>(fv.s_count) * fv.H);
    if (!fv.have_grid) fv.s_count = 1;
    return fv;
}

real field_log_at(const IntersticeTrajectory& tr, const FieldView& fv, int s, usize h) {
    if (h >= fv.H) return 0.0;
    if (fv.have_grid) {
        const usize idx = static_cast<usize>(s) * fv.H + h;
        return tr.T_grid[idx].real();
    }
    if (h < tr.T.size()) return tr.T[h].real();
    return 0.0;
}

void populate_signal_decomposition(const TrajectoryFieldContext& ctx, Signal& sig) {
    const FieldView fv = make_field_view(ctx.trajectory_field);
    if (fv.H == 0) return;

    sig.projected_log_path.assign(fv.H, 0.0);
    sig.projected_return_path.assign(fv.H, 0.0);

    // Production projection functional: average the field log over the full
    // s-grid at each horizon h.  This is a true field functional on the full
    // interstice manifold (not a single s-slice shortcut) and is the canonical
    // diagnostic projection consumed by the field-functional signal builder.
    const real inv_S = 1.0 / static_cast<real>(std::max(1, fv.s_count));
    for (usize h = 0; h < fv.H; ++h) {
        real Lh_avg = 0.0;
        for (int s = 0; s < fv.s_count; ++s)
            Lh_avg += field_log_at(ctx.trajectory_field, fv, s, h);
        sig.projected_log_path[h] = Lh_avg * inv_S;
    }
    real L0 = 0.0;
    for (int s = 0; s < fv.s_count; ++s)
        L0 += field_log_at(ctx.trajectory_field, fv, s, 0);
    L0 *= inv_S;
    for (usize h = 0; h < fv.H; ++h)
        sig.projected_return_path[h] = sig.projected_log_path[h] - L0;

    sig.framework_energy_path = ctx.risk_field.energy_framework;
    sig.proxy_energy_path = ctx.risk_field.energy_proxy;
    sig.curvature_path = ctx.risk_field.curvature;
    sig.uncertainty_proxy_path = ctx.risk_field.uncertainty_proxy;

    if (ctx.event_section != nullptr) {
        sig.event_packet_count = static_cast<int>(ctx.event_section->event_packets.size());
        sig.event_joint_score_path.assign(ctx.event_section->event_packets.size(), 0.0);
        for (usize i = 0; i < ctx.event_section->event_packets.size(); ++i) {
            sig.event_joint_score_path[i] = ctx.event_section->event_packets[i].joint_score;
        }
    }
}

Signal build_research_heuristic_signal(const TrajectoryFieldContext& ctx, const SignalConfig& cfg) {
    Signal sig;
    sig.ts = ctx.anchor_ts;
    sig.theorem_authoritative = ctx.anchor_theorem_authoritative;
    sig.theorem_local_sector_verified = ctx.anchor_theorem_local_sector_verified;
    sig.theorem_valid_h_max = ctx.anchor_theorem_valid_h_max;
    sig.theorem_valid_h_count = ctx.anchor_theorem_valid_h_count;
    sig.theorem_valid_h_fraction = ctx.anchor_theorem_valid_h_fraction;
    sig.implementation_authoritative = ctx.anchor_implementation_authoritative;
    sig.research_non_authoritative = true;
    sig.research_heuristic_signal = true;
    sig.authority_regime = std::string(authority_regime_name(ctx.authority_regime));
    sig.recurrence_order = ctx.recurrence_order;
    sig.spectral_mode_count = ctx.spectral_mode_count;
    sig.closure_claimed = ctx.anchor_closure_claimed;
    sig.closure_verified = ctx.anchor_closure_verified;
    sig.failed_clauses = ctx.failed_clauses;
    sig.framework_energy_missing_reason = "research_heuristic_mode";

    populate_signal_decomposition(ctx, sig);

    if (ctx.trajectory_field.T.empty()) return sig;

    const int H = static_cast<int>(ctx.trajectory_field.T.size());
    const int h = std::clamp(cfg.h_eval, 0, H - 1);

    const real trend = ctx.trajectory_field.T[static_cast<usize>(h)].real();
    const real abs_t = std::abs(trend);
    sig.score_plus = trend;
    sig.score_minus = -trend;
    sig.score_dominant = std::max(sig.score_plus, sig.score_minus);
    if (abs_t < cfg.signal_threshold) return sig;

    real max_dd = 0.0;
    for (int i = 0; i <= h && i < static_cast<int>(ctx.risk_field.drawdown_proxy.size()); ++i)
        max_dd = std::max(max_dd, ctx.risk_field.drawdown_proxy[static_cast<usize>(i)]);
    if (max_dd > cfg.dd_threshold) return sig;

    const bool have_curv = !ctx.risk_field.curvature.empty() && h < static_cast<int>(ctx.risk_field.curvature.size());
    const real curv = have_curv ? ctx.risk_field.curvature[static_cast<usize>(h)] : 0.0;
    if (have_curv && curv <= 0.0) return sig;

    int side = 0;
    if (trend > cfg.long_threshold) side = +1;
    else if (trend < -cfg.short_threshold) side = -1;
    if (side == 0) return sig;

    const usize idx = static_cast<usize>(ctx.anchor_idx);
    if (idx >= ctx.market_state.x3.size()) return sig;
    const real entry_log = ctx.market_state.x3[idx];
    const real entry_px = std::exp(entry_log);

    const real unc = (h < static_cast<int>(ctx.risk_field.uncertainty_proxy.size()))
                   ? std::sqrt(std::max(0.0, ctx.risk_field.uncertainty_proxy[static_cast<usize>(h)]))
                   : 0.0;
    const real band = cfg.stop_atr_mult * unc;

    sig.side = side;
    sig.strength = clamp01(abs_t / (abs_t + cfg.signal_threshold + 1e-12));
    sig.entry_px = entry_px;
    sig.stop_px = (side > 0) ? entry_px * std::exp(-band) : entry_px * std::exp(+band);
    sig.take_px = (side > 0) ? entry_px * std::exp(+band) : entry_px * std::exp(-band);
    sig.used_proxy_energy = true;
    return sig;
}

Signal build_field_functional_signal(const TrajectoryFieldContext& ctx, const SignalConfig& cfg) {
    Signal sig;
    sig.ts = ctx.anchor_ts;
    sig.theorem_authoritative = ctx.anchor_theorem_authoritative;
    sig.theorem_local_sector_verified = ctx.anchor_theorem_local_sector_verified;
    sig.theorem_valid_h_max = ctx.anchor_theorem_valid_h_max;
    sig.theorem_valid_h_count = ctx.anchor_theorem_valid_h_count;
    sig.theorem_valid_h_fraction = ctx.anchor_theorem_valid_h_fraction;
    sig.implementation_authoritative = ctx.anchor_implementation_authoritative;
    sig.research_non_authoritative = ctx.research_non_authoritative;
    sig.research_heuristic_signal = false;
    sig.authority_regime = std::string(authority_regime_name(ctx.authority_regime));
    sig.recurrence_order = ctx.recurrence_order;
    sig.spectral_mode_count = ctx.spectral_mode_count;
    sig.closure_claimed = ctx.anchor_closure_claimed;
    sig.closure_verified = ctx.anchor_closure_verified;
    sig.failed_clauses = ctx.failed_clauses;
    sig.framework_energy_missing_reason = "";

    populate_signal_decomposition(ctx, sig);

    // Hard production gates. Any single failure short-circuits to a zero
    // (no-trade) signal with provenance preserved.  Production code must
    // satisfy ALL of:  theorem authority, implementation authority, framework
    // energy present, no carrier failed clauses, no proxy fallback active.
    const bool authoritative_anchor = ctx.anchor_theorem_authoritative
                                   && ctx.anchor_implementation_authoritative;
    if (cfg.require_authoritative_anchor && !authoritative_anchor && !ctx.research_non_authoritative)
        return sig;

    const FieldView fv = make_field_view(ctx.trajectory_field);
    if (fv.H == 0) return sig;

    const bool have_framework_energy = ctx.risk_field.energy_framework_present
                                    && ctx.risk_field.energy_framework.size() >= fv.H;
    const bool have_proxy_energy = ctx.risk_field.energy_proxy.size() >= fv.H;
    const bool allow_proxy = cfg.allow_proxy_energy_in_research && ctx.research_non_authoritative;

    // In production (non-research), framework energy is REQUIRED.  We never
    // fall back to proxy in production, regardless of allow_proxy_energy_in_research.
    if (!ctx.research_non_authoritative && !have_framework_energy) {
        sig.framework_energy_missing_reason =
            ctx.risk_field.energy_framework_missing_reason.empty()
            ? std::string("framework_energy_absent_blocked_production")
            : ("framework_energy_absent_blocked_production:"
               + ctx.risk_field.energy_framework_missing_reason);
        return sig;
    }
    if (cfg.require_framework_energy && !have_framework_energy && !allow_proxy) {
        sig.framework_energy_missing_reason = "framework_energy_absent_blocked";
        return sig;
    }

    auto energy_at = [&](usize h) -> real {
        if (have_framework_energy) return std::max(0.0, ctx.risk_field.energy_framework[h]);
        if (allow_proxy && have_proxy_energy) {
            sig.used_proxy_energy = true;
            if (sig.framework_energy_missing_reason.empty())
                sig.framework_energy_missing_reason = "framework_energy_absent_using_proxy";
            return std::max(0.0, ctx.risk_field.energy_proxy[h]);
        }
        return 0.0;
    };

    real dd_star = 0.0;
    const usize dd_h = std::min(fv.H, ctx.risk_field.drawdown_proxy.size());
    for (usize h = 0; h < dd_h; ++h) dd_star = std::max(dd_star, ctx.risk_field.drawdown_proxy[h]);

    const real w = 1.0 / static_cast<real>(std::max(1, fv.s_count) * std::max<usize>(1, fv.H));

    real sum_r_plus = 0.0;
    real sum_r_minus = 0.0;
    real sum_e = 0.0;
    real sum_curv_plus = 0.0;
    real sum_curv_minus = 0.0;
    real event_bias_plus = 0.0;
    real event_bias_minus = 0.0;

    for (int s = 0; s < fv.s_count; ++s) {
        const real L0 = field_log_at(ctx.trajectory_field, fv, s, 0);
        for (usize h = 0; h < fv.H; ++h) {
            const real Lh = field_log_at(ctx.trajectory_field, fv, s, h);
            const real Rh = Lh - L0;

            real d2 = 0.0;
            if (h > 0 && (h + 1) < fv.H) {
                const real Lp = field_log_at(ctx.trajectory_field, fv, s, h + 1);
                const real Lm = field_log_at(ctx.trajectory_field, fv, s, h - 1);
                d2 = Lp - 2.0 * Lh + Lm;
            }

            sum_r_plus += w * Rh;
            sum_r_minus += w * (-Rh);
            sum_e += w * energy_at(h);
            sum_curv_plus += w * neg_part(d2);
            sum_curv_minus += w * neg_part(-d2);
        }
    }

    if (ctx.event_section != nullptr && !ctx.event_section->event_packets.empty()) {
        const auto& packets = ctx.event_section->event_packets;
        const usize tail = std::min<usize>(packets.size(), fv.H);
        for (usize k = packets.size() - tail; k < packets.size(); ++k) {
            const auto& p = packets[k];
            const real drift_dir = p.persistent_drift + 0.5 * (p.gap_event + p.body_inversion) - p.exhaustion;
            if (drift_dir >= 0.0) event_bias_plus += drift_dir;
            else event_bias_minus += -drift_dir;
        }
        const real norm = 1.0 / static_cast<real>(std::max<usize>(1, tail));
        event_bias_plus *= norm;
        event_bias_minus *= norm;
    }

    const real score_plus = sum_r_plus
                          + 0.15 * event_bias_plus
                          - cfg.lambda_energy * sum_e
                          - cfg.lambda_drawdown * dd_star
                          - cfg.lambda_curvature * sum_curv_plus
                          - cfg.long_threshold;

    const real score_minus = sum_r_minus
                           + 0.15 * event_bias_minus
                           - cfg.lambda_energy * sum_e
                           - cfg.lambda_drawdown * dd_star
                           - cfg.lambda_curvature * sum_curv_minus
                           - cfg.short_threshold;

    const real dominant = std::max(score_plus, score_minus);
    sig.score_plus = score_plus;
    sig.score_minus = score_minus;
    sig.score_dominant = dominant;
    if (dominant < cfg.signal_threshold) return sig;

    sig.side = (score_plus >= score_minus) ? +1 : -1;
    sig.strength = clamp01(dominant / (dominant + cfg.signal_threshold + 1e-12));

    const usize idx = static_cast<usize>(ctx.anchor_idx);
    if (idx >= ctx.market_state.x3.size()) return sig;
    const real entry_log = ctx.market_state.x3[idx];
    const real entry_px = std::exp(entry_log);

    const usize h_eval = std::clamp<usize>(static_cast<usize>(std::max(0, cfg.h_eval)),
                                           0,
                                           fv.H - 1);
    const real unc = (h_eval < ctx.risk_field.uncertainty_proxy.size())
                   ? std::sqrt(std::max(0.0, ctx.risk_field.uncertainty_proxy[h_eval]))
                   : 0.0;
    const real band = cfg.stop_atr_mult * unc;

    sig.entry_px = entry_px;
    sig.stop_px = (sig.side > 0) ? entry_px * std::exp(-band) : entry_px * std::exp(+band);
    sig.take_px = (sig.side > 0) ? entry_px * std::exp(+band) : entry_px * std::exp(-band);
    return sig;
}

}  // namespace

Signal build_signal_from_trajectory(const TrajectoryFieldContext& ctx, const SignalConfig& cfg) {
    // Production guard: the research-heuristic path may NEVER serve as a
    // production signal.  Routing it here is only allowed when the caller
    // simultaneously declares research_non_authoritative=true on the anchor
    // context.  Otherwise we hard-fall to the field-functional builder, which
    // will itself refuse to produce a trade if the anchor is not authoritative.
    if (cfg.research_heuristic_signal && ctx.research_non_authoritative)
        return build_research_heuristic_signal(ctx, cfg);
    return build_field_functional_signal(ctx, cfg);
}

}  // namespace mt
