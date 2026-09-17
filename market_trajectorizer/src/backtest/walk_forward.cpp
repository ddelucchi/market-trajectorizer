// Walk-forward orchestrator: rolling [train | test] partitions over a single
// dataset.  STRICT NO-LOOKAHEAD: the engine sees only ts <= train_end - 1
// when generating signals consumed in the test window.
//
//   anchor_idx in [test_start, test_end)
//   for each anchor t:
//     run_anchor(candles[0..t], t)             // signals only use [0,t]
//     -> Signal at t -> backtest fills candles[t+1].open at fill_lag = 1
//
// Window stride: train_min, test_window, step (all in bars).

#include "mt/backtest/walk_forward.hpp"
#include "mt/engine/pipeline.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace mt {

namespace {

CandleColumns slice_candles(const CandleColumns& src, int start, int end) {
    CandleColumns c;
    if (end <= start) { c.n = 0; return c; }
    const usize n = static_cast<usize>(end - start);
    c.resize(n);
    for (usize i = 0; i < n; ++i) {
        usize j = static_cast<usize>(start) + i;
        c.ts[i]     = src.ts[j];
        c.open[i]   = src.open[j];
        c.high[i]   = src.high[j];
        c.low[i]    = src.low[j];
        c.close[i]  = src.close[j];
        c.volume[i] = src.volume[j];
    }
    return c;
}

Metrics aggregate_metrics(const Vec<Metrics>& m) {
    Metrics agg;
    if (m.empty()) return agg;
    real n = static_cast<real>(m.size());
    for (const auto& x : m) {
        agg.cagr           += x.cagr;
        agg.sharpe         += x.sharpe;
        agg.sortino        += x.sortino;
        agg.max_drawdown    = std::max(agg.max_drawdown, x.max_drawdown);
        agg.calmar         += x.calmar;
        agg.hit_rate       += x.hit_rate;
        agg.profit_factor  += x.profit_factor;
        agg.turnover       += x.turnover;
        agg.avg_trade      += x.avg_trade;
        agg.exposure       += x.exposure;
        agg.pnl_per_bar    += x.pnl_per_bar;
        agg.path_deviation += x.path_deviation;
    }
    agg.cagr /= n; agg.sharpe /= n; agg.sortino /= n; agg.calmar /= n;
    agg.hit_rate /= n; agg.profit_factor /= n; agg.turnover /= n;
    agg.avg_trade /= n; agg.exposure /= n; agg.pnl_per_bar /= n;
    agg.path_deviation /= n;
    return agg;
}

struct FidelityAccumulator {
    int  n = 0;
    real lead_sign_agreement_sum = 0.0;
    real turning_point_alignment_sum = 0.0;
    real path_rmse_sum = 0.0;
    real event_boundary_overlap_sum = 0.0;
    real drawdown_shape_similarity_sum = 0.0;
};

struct SplitAccumulator {
    int         anchors = 0;
    int         signals = 0;
    Vec<Signal> signal_list;
    FidelityAccumulator fidelity;
};

real sign_eps(real x) {
    constexpr real eps = 1e-12;
    if (x > eps) return 1.0;
    if (x < -eps) return -1.0;
    return 0.0;
}

void add_fidelity(FidelityAccumulator& acc, const FidelityMetrics& m) {
    if (m.anchors_evaluated <= 0) return;
    acc.n += m.anchors_evaluated;
    acc.lead_sign_agreement_sum += m.lead_sign_agreement * static_cast<real>(m.anchors_evaluated);
    acc.turning_point_alignment_sum += m.turning_point_alignment * static_cast<real>(m.anchors_evaluated);
    acc.path_rmse_sum += m.path_rmse * static_cast<real>(m.anchors_evaluated);
    acc.event_boundary_overlap_sum += m.event_boundary_overlap * static_cast<real>(m.anchors_evaluated);
    acc.drawdown_shape_similarity_sum += m.drawdown_shape_similarity * static_cast<real>(m.anchors_evaluated);
}

FidelityMetrics finalize_fidelity(const FidelityAccumulator& acc) {
    FidelityMetrics m;
    if (acc.n <= 0) return m;
    const real inv_n = 1.0 / static_cast<real>(acc.n);
    m.anchors_evaluated = acc.n;
    m.lead_sign_agreement = acc.lead_sign_agreement_sum * inv_n;
    m.turning_point_alignment = acc.turning_point_alignment_sum * inv_n;
    m.path_rmse = acc.path_rmse_sum * inv_n;
    m.event_boundary_overlap = acc.event_boundary_overlap_sum * inv_n;
    m.drawdown_shape_similarity = acc.drawdown_shape_similarity_sum * inv_n;
    return m;
}

void add_anchor(SplitAccumulator& acc, const FidelityMetrics& fidelity) {
    ++acc.anchors;
    add_fidelity(acc.fidelity, fidelity);
}

void add_signal(SplitAccumulator& acc, const Signal& s) {
    if (s.side == 0) return;
    ++acc.signals;
    acc.signal_list.push_back(s);
}

Vec<int> turning_points(const Vec<real>& path) {
    Vec<int> out;
    if (path.size() < 3) return out;
    for (usize i = 1; i + 1 < path.size(); ++i) {
        const real left = path[i] - path[i - 1];
        const real right = path[i + 1] - path[i];
        const real s_left = sign_eps(left);
        const real s_right = sign_eps(right);
        if (s_left == 0.0 || s_right == 0.0) continue;
        if (s_left != s_right) out.push_back(static_cast<int>(i));
    }
    return out;
}

real jaccard_overlap(const Vec<int>& a, const Vec<int>& b) {
    if (a.empty() && b.empty()) return 1.0;
    std::set<int> sa(a.begin(), a.end());
    std::set<int> sb(b.begin(), b.end());
    usize intersection = 0;
    for (int x : sa) {
        if (sb.find(x) != sb.end()) ++intersection;
    }
    const usize uni = sa.size() + sb.size() - intersection;
    if (uni == 0) return 0.0;
    return static_cast<real>(intersection) / static_cast<real>(uni);
}

Vec<real> drawdown_curve_from_log_path(const Vec<real>& log_path) {
    Vec<real> dd(log_path.size(), 0.0);
    if (log_path.empty()) return dd;
    real peak = std::exp(log_path[0]);
    for (usize i = 0; i < log_path.size(); ++i) {
        const real px = std::exp(log_path[i]);
        peak = std::max(peak, px);
        if (peak > 0.0) dd[i] = (px / peak) - 1.0;
    }
    return dd;
}

FidelityMetrics compute_anchor_fidelity(const Signal& s,
                                        const CandleColumns& candles,
                                        int t,
                                        int horizon_max,
                                        const FutureSection& jet_minus) {
    FidelityMetrics out;
    if (s.projected_return_path.size() < 2) return out;
    if (t < 0 || static_cast<usize>(t) >= candles.n) return out;
    if (candles.close[static_cast<usize>(t)] <= 0.0) return out;

    const int max_proj_h = static_cast<int>(s.projected_return_path.size()) - 1;
    const int max_data_h = static_cast<int>(candles.n) - t - 1;
    int max_h = std::min(max_proj_h, max_data_h);
    if (horizon_max > 0) max_h = std::min(max_h, horizon_max);
    if (max_h < 1) return out;

    Vec<real> proj(static_cast<usize>(max_h) + 1, 0.0);
    Vec<real> realized(static_cast<usize>(max_h) + 1, 0.0);
    for (int h = 0; h <= max_h; ++h)
        proj[static_cast<usize>(h)] = s.projected_return_path[static_cast<usize>(h)];

    const real anchor_log = std::log(candles.close[static_cast<usize>(t)]);
    for (int h = 0; h <= max_h; ++h) {
        const usize idx = static_cast<usize>(t + h);
        const real c = candles.close[idx];
        if (c <= 0.0) return out;
        realized[static_cast<usize>(h)] = std::log(c) - anchor_log;
    }

    int sign_pairs = 0;
    int sign_matches = 0;
    for (int h = 1; h <= max_h; ++h) {
        const real ps = sign_eps(proj[static_cast<usize>(h)]);
        const real rs = sign_eps(realized[static_cast<usize>(h)]);
        if (ps == 0.0 || rs == 0.0) continue;
        ++sign_pairs;
        if (ps == rs) ++sign_matches;
    }
    out.lead_sign_agreement = (sign_pairs > 0)
        ? static_cast<real>(sign_matches) / static_cast<real>(sign_pairs)
        : 0.0;

    real rmse = 0.0;
    for (int h = 1; h <= max_h; ++h) {
        const real e = proj[static_cast<usize>(h)] - realized[static_cast<usize>(h)];
        rmse += e * e;
    }
    rmse = std::sqrt(rmse / static_cast<real>(max_h));
    out.path_rmse = rmse;

    const Vec<int> proj_turn = turning_points(proj);
    const Vec<int> real_turn = turning_points(realized);
    out.turning_point_alignment = jaccard_overlap(proj_turn, real_turn);

    Vec<int> pred_boundaries;
    pred_boundaries.reserve(jet_minus.sigma.size());
    for (const real sigma : jet_minus.sigma) {
        const int h = static_cast<int>(std::llround(sigma));
        if (h >= 1 && h <= max_h)
            pred_boundaries.push_back(h);
    }
    std::sort(pred_boundaries.begin(), pred_boundaries.end());
    pred_boundaries.erase(std::unique(pred_boundaries.begin(), pred_boundaries.end()), pred_boundaries.end());
    out.event_boundary_overlap = jaccard_overlap(pred_boundaries, real_turn);

    const Vec<real> dd_proj = drawdown_curve_from_log_path(proj);
    const Vec<real> dd_real = drawdown_curve_from_log_path(realized);
    real dd_rmse = 0.0;
    for (usize i = 0; i < dd_proj.size(); ++i) {
        const real e = dd_proj[i] - dd_real[i];
        dd_rmse += e * e;
    }
    dd_rmse = std::sqrt(dd_rmse / static_cast<real>(std::max<usize>(1, dd_proj.size())));
    out.drawdown_shape_similarity = 1.0 / (1.0 + dd_rmse);

    out.anchors_evaluated = 1;
    return out;
}

StratifiedMetrics finalize_split(const SplitAccumulator& acc,
                                const CandleColumns& test_slice,
                                const BacktestConfig& bc,
                                const Metrics* precomputed_metrics = nullptr) {
    StratifiedMetrics out;
    out.anchors = acc.anchors;
    out.signals = acc.signals;
    out.fidelity = finalize_fidelity(acc.fidelity);
    if (precomputed_metrics != nullptr) {
        out.metrics = *precomputed_metrics;
    } else if (!acc.signal_list.empty()) {
        out.metrics = compute_metrics(run_backtest(test_slice, acc.signal_list, bc));
    }
    return out;
}

std::string primary_failed_clause(const Vec<std::string>& failed_clauses) {
    if (failed_clauses.empty()) return "none";
    return failed_clauses.front();
}

StratifiedMetrics aggregate_stratified(const Vec<StratifiedMetrics>& rows) {
    StratifiedMetrics out;
    if (rows.empty()) return out;

    Vec<Metrics> metric_rows;
    metric_rows.reserve(rows.size());
    FidelityAccumulator fidelity_acc;
    for (const auto& r : rows) {
        out.anchors += r.anchors;
        out.signals += r.signals;
        metric_rows.push_back(r.metrics);
        add_fidelity(fidelity_acc, r.fidelity);
    }
    out.metrics = aggregate_metrics(metric_rows);
    out.fidelity = finalize_fidelity(fidelity_acc);
    return out;
}

Vec<ClauseStratifiedMetrics> aggregate_clause_splits(const Vec<WalkForwardWindowDiagnostics>& windows) {
    struct ClauseAcc {
        int anchors = 0;
        int signals = 0;
        Vec<Metrics> metrics;
        FidelityAccumulator fidelity;
    };

    std::map<std::string, ClauseAcc> table;
    for (const auto& w : windows) {
        for (const auto& c : w.failed_clause_primary) {
            auto& acc = table[c.clause];
            acc.anchors += c.anchors;
            acc.signals += c.signals;
            acc.metrics.push_back(c.metrics);
            add_fidelity(acc.fidelity, c.fidelity);
        }
    }

    Vec<ClauseStratifiedMetrics> out;
    out.reserve(table.size());
    for (const auto& kv : table) {
        ClauseStratifiedMetrics row;
        row.clause = kv.first;
        row.anchors = kv.second.anchors;
        row.signals = kv.second.signals;
        row.metrics = aggregate_metrics(kv.second.metrics);
        row.fidelity = finalize_fidelity(kv.second.fidelity);
        out.push_back(row);
    }
    std::sort(out.begin(), out.end(), [](const ClauseStratifiedMetrics& a, const ClauseStratifiedMetrics& b) {
        if (a.anchors != b.anchors) return a.anchors > b.anchors;
        return a.clause < b.clause;
    });
    return out;
}

WalkForwardWindowDiagnostics aggregate_window_diagnostics(const Vec<WalkForwardWindowDiagnostics>& windows) {
    WalkForwardWindowDiagnostics agg;
    if (windows.empty()) return agg;

    auto collect = [&](auto member_ptr) {
        Vec<StratifiedMetrics> rows;
        rows.reserve(windows.size());
        for (const auto& w : windows) rows.push_back(w.*member_ptr);
        return aggregate_stratified(rows);
    };

    agg.all = collect(&WalkForwardWindowDiagnostics::all);
    agg.authoritative = collect(&WalkForwardWindowDiagnostics::authoritative);
    agg.non_authoritative = collect(&WalkForwardWindowDiagnostics::non_authoritative);
    agg.local_sector_valid = collect(&WalkForwardWindowDiagnostics::local_sector_valid);
    agg.local_sector_invalid = collect(&WalkForwardWindowDiagnostics::local_sector_invalid);
    agg.packet_anchor = collect(&WalkForwardWindowDiagnostics::packet_anchor);
    agg.no_packet_anchor = collect(&WalkForwardWindowDiagnostics::no_packet_anchor);
    agg.recurrence_r0 = collect(&WalkForwardWindowDiagnostics::recurrence_r0);
    agg.recurrence_r1_2 = collect(&WalkForwardWindowDiagnostics::recurrence_r1_2);
    agg.recurrence_r3_5 = collect(&WalkForwardWindowDiagnostics::recurrence_r3_5);
    agg.recurrence_r6_plus = collect(&WalkForwardWindowDiagnostics::recurrence_r6_plus);
    agg.spectral_m0 = collect(&WalkForwardWindowDiagnostics::spectral_m0);
    agg.spectral_m1 = collect(&WalkForwardWindowDiagnostics::spectral_m1);
    agg.spectral_m2_3 = collect(&WalkForwardWindowDiagnostics::spectral_m2_3);
    agg.spectral_m4_plus = collect(&WalkForwardWindowDiagnostics::spectral_m4_plus);
    agg.theorem_hcount_0 = collect(&WalkForwardWindowDiagnostics::theorem_hcount_0);
    agg.theorem_hcount_1_4 = collect(&WalkForwardWindowDiagnostics::theorem_hcount_1_4);
    agg.theorem_hcount_5_16 = collect(&WalkForwardWindowDiagnostics::theorem_hcount_5_16);
    agg.theorem_hcount_17_plus = collect(&WalkForwardWindowDiagnostics::theorem_hcount_17_plus);
    agg.failed_clause_primary = aggregate_clause_splits(windows);
    return agg;
}

}  // namespace

WalkForwardResult run_walk_forward(const Dataset& ds,
                                   const WalkForwardConfig& wf,
                                   const EngineConfig&      ec,
                                   const SignalConfig&      sc,
                                   const BacktestConfig&    bc)
{
    WalkForwardResult res;
    res.research_non_authoritative = bc.research_non_authoritative;
    res.research_heuristic_signal = sc.research_heuristic_signal;
    const int N = static_cast<int>(ds.candles.n);
    if (wf.step <= 0 || N <= wf.train_min + wf.test_window) return res;

    int train_end = wf.train_min;
    int windows_emitted = 0;
    while (train_end + wf.test_window <= N) {
        if (wf.max_windows > 0 && windows_emitted >= wf.max_windows) break;
        WalkForwardWindow w;
        w.train_start = 0;
        w.train_end   = train_end;
        w.test_start  = train_end;
        w.test_end    = std::min(train_end + wf.test_window, N);

        // Generate signals for each anchor in [test_start, test_end - 1].
        // The pipeline truncates candles[0..t] internally (causality fence).
        SplitAccumulator all_acc;
        all_acc.signal_list.reserve(static_cast<usize>(w.test_end - w.test_start));
        SplitAccumulator authoritative_acc;
        SplitAccumulator non_authoritative_acc;
        SplitAccumulator local_sector_valid_acc;
        SplitAccumulator local_sector_invalid_acc;
        SplitAccumulator packet_anchor_acc;
        SplitAccumulator no_packet_anchor_acc;
        SplitAccumulator recurrence_r0_acc;
        SplitAccumulator recurrence_r1_2_acc;
        SplitAccumulator recurrence_r3_5_acc;
        SplitAccumulator recurrence_r6_plus_acc;
        SplitAccumulator spectral_m0_acc;
        SplitAccumulator spectral_m1_acc;
        SplitAccumulator spectral_m2_3_acc;
        SplitAccumulator spectral_m4_plus_acc;
        SplitAccumulator theorem_hcount_0_acc;
        SplitAccumulator theorem_hcount_1_4_acc;
        SplitAccumulator theorem_hcount_5_16_acc;
        SplitAccumulator theorem_hcount_17_plus_acc;
        std::map<std::string, SplitAccumulator> clause_acc;

        for (int t = w.test_start; t < w.test_end; t += std::max(1, wf.anchor_stride)) {
            CandleColumns view = slice_candles(ds.candles, 0, t + 1);
            try {
                AnchorArtifacts art = run_anchor(view, t, ec);
                const bool production_ready = is_anchor_production_authoritative(art);
                if (!production_ready) {
                    ++res.non_authoritative_anchors;
                }
                if (!production_ready && !bc.research_non_authoritative) {
                    ++res.rejected_anchors;
                    continue;
                }
                if (production_ready) ++res.authoritative_anchors;

                TrajectoryFieldContext ctx{
                    art.trajectory,
                    art.risk,
                    art.market_state,
                    &art.section,
                    t,
                    art.anchor_ts,
                    art.authority.theorem_authoritative,
                    art.authority.theorem_local_sector_verified,
                    art.authority.implementation_authoritative,
                    art.authority.theorem_valid_h_max,
                    art.authority.theorem_valid_h_count,
                    art.authority.theorem_valid_h_fraction,
                    art.authority.closure_claimed,
                    art.authority.closure_verified,
                    art.recurrence.rec.r,
                    static_cast<int>(art.recurrence.spectrum.modes.size()),
                    art.authority.regime,
                    art.authority.failed_clauses,
                    !production_ready && bc.research_non_authoritative,
                    sc.research_heuristic_signal
                };
                Signal s = build_signal_from_trajectory(ctx, sc);
                if (s.used_proxy_energy) ++res.research_proxy_energy_signals;

                const FidelityMetrics fidelity = compute_anchor_fidelity(
                    s,
                    ds.candles,
                    t,
                    wf.horizon_max,
                    art.jet_minus
                );

                auto update_split = [&](SplitAccumulator& acc) {
                    add_anchor(acc, fidelity);
                    add_signal(acc, s);
                };

                update_split(all_acc);

                if (production_ready) update_split(authoritative_acc);
                else update_split(non_authoritative_acc);

                if (art.authority.theorem_local_sector_verified) update_split(local_sector_valid_acc);
                else update_split(local_sector_invalid_acc);

                if (!art.section.event_packets.empty()) update_split(packet_anchor_acc);
                else update_split(no_packet_anchor_acc);

                const int rec_order = std::max(0, art.recurrence.rec.r);
                if (rec_order == 0) update_split(recurrence_r0_acc);
                else if (rec_order <= 2) update_split(recurrence_r1_2_acc);
                else if (rec_order <= 5) update_split(recurrence_r3_5_acc);
                else update_split(recurrence_r6_plus_acc);

                const int spectral_modes = static_cast<int>(art.recurrence.spectrum.modes.size());
                if (spectral_modes <= 0) update_split(spectral_m0_acc);
                else if (spectral_modes == 1) update_split(spectral_m1_acc);
                else if (spectral_modes <= 3) update_split(spectral_m2_3_acc);
                else update_split(spectral_m4_plus_acc);

                const int h_count = std::max(0, art.authority.theorem_valid_h_count);
                if (h_count == 0) update_split(theorem_hcount_0_acc);
                else if (h_count <= 4) update_split(theorem_hcount_1_4_acc);
                else if (h_count <= 16) update_split(theorem_hcount_5_16_acc);
                else update_split(theorem_hcount_17_plus_acc);

                const std::string clause = primary_failed_clause(art.authority.failed_clauses);
                update_split(clause_acc[clause]);
            } catch (...) {
                // Skip degenerate anchors deterministically; do not pollute results.
                ++res.rejected_anchors;
            }
        }

        // Replay the test slice with the generated signals.
        CandleColumns test_slice = slice_candles(ds.candles, w.test_start, w.test_end);
        BacktestResult br = run_backtest(test_slice, all_acc.signal_list, bc);
        Metrics mm = compute_metrics(br);

        WalkForwardWindowDiagnostics diag;
        diag.all = finalize_split(all_acc, test_slice, bc, &mm);
        diag.authoritative = finalize_split(authoritative_acc, test_slice, bc);
        diag.non_authoritative = finalize_split(non_authoritative_acc, test_slice, bc);
        diag.local_sector_valid = finalize_split(local_sector_valid_acc, test_slice, bc);
        diag.local_sector_invalid = finalize_split(local_sector_invalid_acc, test_slice, bc);
        diag.packet_anchor = finalize_split(packet_anchor_acc, test_slice, bc);
        diag.no_packet_anchor = finalize_split(no_packet_anchor_acc, test_slice, bc);
        diag.recurrence_r0 = finalize_split(recurrence_r0_acc, test_slice, bc);
        diag.recurrence_r1_2 = finalize_split(recurrence_r1_2_acc, test_slice, bc);
        diag.recurrence_r3_5 = finalize_split(recurrence_r3_5_acc, test_slice, bc);
        diag.recurrence_r6_plus = finalize_split(recurrence_r6_plus_acc, test_slice, bc);
        diag.spectral_m0 = finalize_split(spectral_m0_acc, test_slice, bc);
        diag.spectral_m1 = finalize_split(spectral_m1_acc, test_slice, bc);
        diag.spectral_m2_3 = finalize_split(spectral_m2_3_acc, test_slice, bc);
        diag.spectral_m4_plus = finalize_split(spectral_m4_plus_acc, test_slice, bc);
        diag.theorem_hcount_0 = finalize_split(theorem_hcount_0_acc, test_slice, bc);
        diag.theorem_hcount_1_4 = finalize_split(theorem_hcount_1_4_acc, test_slice, bc);
        diag.theorem_hcount_5_16 = finalize_split(theorem_hcount_5_16_acc, test_slice, bc);
        diag.theorem_hcount_17_plus = finalize_split(theorem_hcount_17_plus_acc, test_slice, bc);
        for (const auto& kv : clause_acc) {
            ClauseStratifiedMetrics row;
            row.clause = kv.first;
            row.anchors = kv.second.anchors;
            row.signals = kv.second.signals;
            row.fidelity = finalize_fidelity(kv.second.fidelity);
            if (!kv.second.signal_list.empty())
                row.metrics = compute_metrics(run_backtest(test_slice, kv.second.signal_list, bc));
            diag.failed_clause_primary.push_back(row);
        }
        std::sort(diag.failed_clause_primary.begin(), diag.failed_clause_primary.end(),
            [](const ClauseStratifiedMetrics& a, const ClauseStratifiedMetrics& b) {
                if (a.anchors != b.anchors) return a.anchors > b.anchors;
                return a.clause < b.clause;
            });

        res.windows   .push_back(w);
        res.per_window.push_back(mm);
        res.per_window_diagnostics.push_back(diag);
        ++windows_emitted;

        train_end += wf.step;
    }
    res.aggregate = aggregate_metrics(res.per_window);
    res.aggregate_diagnostics = aggregate_window_diagnostics(res.per_window_diagnostics);
    return res;
}

}  // namespace mt
