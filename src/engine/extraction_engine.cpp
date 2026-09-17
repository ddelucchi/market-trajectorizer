#include "mt/engine/extraction_engine.hpp"

#include "mt/math/combinatorics.hpp"
#include "mt/math/finite_differences.hpp"

#include <array>
#include <algorithm>
#include <cstddef>
#include <cmath>

namespace mt {

namespace {

struct PacketBuildResult {
    Vec<EventPacket> packets;
    Vec<real> sector_scores;
    Vec<real> transition_distance;
    ChangePointSet cps;
};

real mean_prev_window(const Vec<real>& v, int i, int w) {
    if (i <= 0) return 0.0;
    const int lo = std::max(0, i - w);
    const int hi = i - 1;
    const int n = hi - lo + 1;
    if (n <= 0) return 0.0;
    real acc = 0.0;
    for (int k = lo; k <= hi; ++k) acc += v[static_cast<usize>(k)];
    return acc / static_cast<real>(n);
}

real std_prev_window(const Vec<real>& v, int i, int w, real mean) {
    if (i <= 0) return 0.0;
    const int lo = std::max(0, i - w);
    const int hi = i - 1;
    const int n = hi - lo + 1;
    if (n <= 1) return 0.0;
    real acc = 0.0;
    for (int k = lo; k <= hi; ++k) {
        const real e = v[static_cast<usize>(k)] - mean;
        acc += e * e;
    }
    return std::sqrt(acc / static_cast<real>(n));
}

real normalized_deviation_now(const Vec<real>& v, int i, int w) {
    const real m = mean_prev_window(v, i, w);
    const real s = std_prev_window(v, i, w, m);
    if (s <= 0.0) return 0.0;
    return (v[static_cast<usize>(i)] - m) / s;
}

real median_prev_window(const Vec<real>& v, int i, int w) {
    if (i <= 0) return 0.0;
    const int lo = std::max(0, i - w);
    const int hi = i - 1;
    if (hi < lo) return 0.0;
    Vec<real> win;
    win.reserve(static_cast<usize>(hi - lo + 1));
    for (int k = lo; k <= hi; ++k) win.push_back(v[static_cast<usize>(k)]);
    const usize mid = win.size() / 2;
    std::nth_element(win.begin(), win.begin() + static_cast<std::ptrdiff_t>(mid), win.end());
    if (win.size() % 2 == 1) return win[mid];
    const real a = win[mid];
    std::nth_element(win.begin(), win.begin() + static_cast<std::ptrdiff_t>(mid - 1), win.end());
    const real b = win[mid - 1];
    return 0.5 * (a + b);
}

real mad_prev_window(const Vec<real>& v, int i, int w) {
    if (i <= 1) return 1.0;
    const int lo = std::max(0, i - w);
    const int hi = i - 1;
    if (hi < lo) return 1.0;
    const real med = median_prev_window(v, i, w);
    Vec<real> dev;
    dev.reserve(static_cast<usize>(hi - lo + 1));
    for (int k = lo; k <= hi; ++k) dev.push_back(std::abs(v[static_cast<usize>(k)] - med));
    const usize mid = dev.size() / 2;
    std::nth_element(dev.begin(), dev.begin() + static_cast<std::ptrdiff_t>(mid), dev.end());
    return std::max(1e-9, dev[mid]);
}

real packet_component(real deviation, real sigma) {
    if (sigma <= 0.0) return 0.0;
    const real a = std::abs(deviation);
    if (a <= sigma) return 0.0;
    return (a - sigma) / sigma;
}

real l2_distance(const std::array<real, 8>& a, const std::array<real, 8>& b) {
    real acc = 0.0;
    for (usize i = 0; i < a.size(); ++i) {
        const real d = a[i] - b[i];
        acc += d * d;
    }
    return std::sqrt(acc);
}

real l2_norm(const std::array<real, 8>& a) {
    real acc = 0.0;
    for (real x : a) acc += x * x;
    return std::sqrt(acc);
}

PacketBuildResult build_event_packets(const MarketStateColumns& s,
                                      const ChangePointConfig& cfg) {
    PacketBuildResult out;
    if (s.n == 0) return out;

    const int n = static_cast<int>(s.n);
    const int w = std::max(cfg.rolling_window, 4);

    DerivedChannels d = build_derived_channels(s, w);

    out.packets.resize(s.n);
    out.transition_distance.assign(s.n, 0.0);
    out.sector_scores.assign(s.n, 0.0);

    Vec<real> hist_gap(s.n, 0.0);
    Vec<real> hist_range(s.n, 0.0);
    Vec<real> hist_body(s.n, 0.0);
    Vec<real> hist_volume(s.n, 0.0);
    Vec<real> hist_wick(s.n, 0.0);
    Vec<real> hist_compression(s.n, 0.0);
    Vec<real> hist_drift(s.n, 0.0);
    Vec<real> hist_exhaustion(s.n, 0.0);

    const std::array<real, 8> weights{
        cfg.weight_gap,
        cfg.weight_range,
        cfg.weight_body,
        cfg.weight_volume,
        cfg.weight_wick,
        cfg.weight_compression,
        cfg.weight_drift,
        cfg.weight_exhaustion
    };

    std::array<real, 8> prev_vec{};

    int persistence_hits = 0;
    int last_cp = -cfg.min_separation;

    for (int i = 0; i < n; ++i) {
        EventPacket p;
        p.index = i;

        const real dev_gap = normalized_deviation_now(d.gap, i, w);
        const real dev_range = normalized_deviation_now(d.range, i, w);
        const real dev_body = normalized_deviation_now(d.body, i, w);
        const real dev_wick = normalized_deviation_now(d.wick_asymmetry, i, w);
        const real dev_ret = normalized_deviation_now(d.ret1, i, w);
        const real dev_vol = (i < static_cast<int>(d.volume_shock.size())) ? d.volume_shock[static_cast<usize>(i)] : 0.0;

        p.gap_event = std::max(0.0, packet_component(dev_gap, cfg.gap_sigma));
        p.range_explosion = std::max(0.0, packet_component(dev_range, cfg.range_sigma));

        bool body_flip = false;
        if (i > 0) {
            const real b0 = d.body[static_cast<usize>(i - 1)];
            const real b1 = d.body[static_cast<usize>(i)];
            body_flip = (b0 * b1) < 0.0;
        }
        p.body_inversion = body_flip ? std::max(0.0, packet_component(dev_body, cfg.body_sigma)) : 0.0;

        p.volume_shock = std::max(0.0, packet_component(dev_vol, cfg.volume_sigma));
        p.wick_rejection = std::max(0.0, packet_component(dev_wick, cfg.wick_sigma));

        const real range_mean = mean_prev_window(d.range, i, w);
        const real range_std = std_prev_window(d.range, i, w, range_mean);
        bool compression_break = false;
        if (i > 0 && range_std > 0.0) {
            const real prev_range = d.range[static_cast<usize>(i - 1)];
            const real curr_range = d.range[static_cast<usize>(i)];
            compression_break = prev_range < (range_mean - cfg.compression_sigma * range_std)
                             && curr_range > (range_mean + cfg.compression_sigma * range_std);
        }
        p.compression_break = compression_break ? 1.0 : 0.0;

        const real drift_mean = mean_prev_window(d.ret1, i, w);
        const real drift_std = std_prev_window(d.ret1, i, w, drift_mean);
        p.persistent_drift = (drift_std > 0.0 && std::abs(drift_mean) > cfg.drift_sigma * drift_std)
                   ? std::max(0.0, std::abs(drift_mean) / std::max(1e-12, drift_std))
                   : 0.0;

        bool exhaustion = false;
        if (i > 0) {
            const real ret = d.ret1[static_cast<usize>(i)];
            exhaustion = (ret * drift_mean) < 0.0 && std::abs(dev_ret) > cfg.exhaustion_sigma;
        }
        p.exhaustion = exhaustion ? std::max(0.0, packet_component(dev_ret, cfg.exhaustion_sigma)) : 0.0;

        hist_gap[static_cast<usize>(i)] = p.gap_event;
        hist_range[static_cast<usize>(i)] = p.range_explosion;
        hist_body[static_cast<usize>(i)] = p.body_inversion;
        hist_volume[static_cast<usize>(i)] = p.volume_shock;
        hist_wick[static_cast<usize>(i)] = p.wick_rejection;
        hist_compression[static_cast<usize>(i)] = p.compression_break;
        hist_drift[static_cast<usize>(i)] = p.persistent_drift;
        hist_exhaustion[static_cast<usize>(i)] = p.exhaustion;

        const std::array<real, 8> raw{
            p.gap_event,
            p.range_explosion,
            p.body_inversion,
            p.volume_shock,
            p.wick_rejection,
            p.compression_break,
            p.persistent_drift,
            p.exhaustion
        };

        const std::array<real, 8> mad{
            mad_prev_window(hist_gap, i, w),
            mad_prev_window(hist_range, i, w),
            mad_prev_window(hist_body, i, w),
            mad_prev_window(hist_volume, i, w),
            mad_prev_window(hist_wick, i, w),
            mad_prev_window(hist_compression, i, w),
            mad_prev_window(hist_drift, i, w),
            mad_prev_window(hist_exhaustion, i, w)
        };

        std::array<real, 8> packet_vec{};
        for (usize k = 0; k < packet_vec.size(); ++k) {
            const real normalized = raw[k] / (cfg.packet_mad_eps + mad[k]);
            packet_vec[k] = weights[k] * std::min<real>(20.0, normalized);
        }

        p.joint_score = l2_norm(packet_vec);

        const real transition = (i > 0) ? l2_distance(packet_vec, prev_vec) : 0.0;
        out.transition_distance[static_cast<usize>(i)] = transition;
        prev_vec = packet_vec;

        const real directional = (i < static_cast<int>(d.ret1.size()) ? d.ret1[static_cast<usize>(i)] : 0.0)
                               + (i < static_cast<int>(d.body.size()) ? d.body[static_cast<usize>(i)] : 0.0);
        const real sign = (directional >= 0.0) ? 1.0 : -1.0;
        out.sector_scores[static_cast<usize>(i)] = sign * p.joint_score;

        if (i > 0) {
            if (transition > cfg.packet_transition_eps) {
                ++persistence_hits;
            } else {
                persistence_hits = 0;
            }
            if (persistence_hits >= cfg.packet_persistence && (i - last_cp) >= cfg.min_separation) {
                out.cps.sigma_idx.push_back(i);
                last_cp = i;
                persistence_hits = 0;
            }
        }

        out.packets[static_cast<usize>(i)] = p;
    }

    return out;
}

cplx forward_difference_at_zero(const Vec<cplx>& samples, int order) {
    if (samples.empty() || order < 0 || static_cast<usize>(order) >= samples.size()) return cplx{};
    Vec<cplx> work = samples;
    for (int k = 0; k < order; ++k) {
        const usize n = work.size();
        for (usize i = 0; i + 1 < n; ++i) work[i] = work[i + 1] - work[i];
        work.pop_back();
    }
    return work.empty() ? cplx{} : work.front();
}

cplx axis_coefficient(const JetState& jet, int degree) {
    if (jet.Q <= 0 || jet.order_max < degree || jet.coeff.empty()) return cplx{};
    Vec<int> alpha(static_cast<usize>(jet.Q), 0);
    alpha[0] = degree;
    const auto layout = make_multi_index_layout(jet.Q, jet.order_max);
    const usize idx = flatten_multi_index(alpha, layout);
    if (idx >= jet.coeff.size()) return cplx{};
    return jet.coeff[idx];
}

Vec<int> normalized_event_boundaries(const Vec<int>& boundaries, int t_anchor) {
    Vec<int> out;
    out.reserve(boundaries.size());
    for (int b : boundaries) {
        if (b < 0) continue;
        if (b > t_anchor) continue;
        out.push_back(b);
    }
    std::sort(out.begin(), out.end(), std::greater<int>());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

}  // namespace

ChangePointSet phase_1_sectorizer_cpu(const MarketStateColumns& s, const ChangePointConfig& cfg) {
    return build_event_packets(s, cfg).cps;
}

ChangePointSet detect_changepoints_cpu(const MarketStateColumns& s, const ChangePointConfig& cfg) {
    return phase_1_sectorizer_cpu(s, cfg);
}

PiecewiseSection build_piecewise_section_cpu(const MarketStateColumns& s,
                                             int                        t_anchor,
                                             const ChangePointSet&      cps,
                                             const SectionConfig&       cfg,
                                             const ChangePointConfig&   cp_cfg)
{
    PiecewiseSection sec;
    sec.cps = cps;

    PacketBuildResult packet = build_event_packets(s, cp_cfg);

    sec.event_packets = packet.packets;
    sec.packet_transition_distance = packet.transition_distance;
    sec.sector_scores = packet.sector_scores;
    sec.event_boundaries = cps.sigma_idx.empty() ? packet.cps.sigma_idx : cps.sigma_idx;

    sec.event_boundaries = normalized_event_boundaries(sec.event_boundaries, t_anchor);
    sec.event_boundaries_from_genuine_packets = !sec.event_boundaries.empty();
    // Synthetic deterministic max-transition fallback boundary.  Strict ledger
    // clause 13.5: a synthetic fallback is research-only and MUST NOT pass
    // the production gate `event_state.genuine_packet_dynamics`.  We only
    // attempt the insertion when the operator has explicitly opted in via
    // `cp_cfg.allow_synthetic_event_boundary_research_only`; production
    // (default) leaves the event set empty and lets authority fail closed.
    if (sec.event_boundaries.empty()
        && !cp_cfg.single_sector_smooth_regime_only
        && cp_cfg.allow_synthetic_event_boundary_research_only) {
        int best_idx = -1;
        real best_dist = 0.0;
        for (int i = 1; i <= t_anchor && static_cast<usize>(i) < sec.packet_transition_distance.size(); ++i) {
            const real d = sec.packet_transition_distance[static_cast<usize>(i)];
            if (d > best_dist) {
                best_dist = d;
                best_idx = i;
            }
        }
        if (best_idx >= 0 && best_dist > 0.0) {
            sec.event_boundaries.push_back(best_idx);
            // Synthetic deterministic-fallback boundary: research-only.
            sec.event_boundaries_from_genuine_packets = false;
        }
    }
    sec.phi_section.clear();
    sec.pi_jumps.clear();

    (void)t_anchor;
    (void)cfg;
    return sec;
}

FutureSection build_past_jet_state_cpu(const PiecewiseSection&   sec,
                                       const JetState&           normalized_jet,
                                       const MarketStateColumns& market_state,
                                       int                       t_anchor,
                                       int                       N_phi,
                                       int                       N_pi,
                                       int                       L_max) {
    FutureSection fs;
    fs.A_phi.assign(static_cast<usize>(std::max(1, N_phi)), cplx{});

    // Theorem-faithfulness clause 13.1 (strict_patch_ledger):
    //   The smooth past coefficients A_phi MUST be axis-projected from the
    //   canonical Q=5 normalized jet built by extract_phase_torus_separable_cpu
    //   + make_jet_from_extracted via axis_coefficient(jet, n), which selects
    //   the multi-index alpha = (n, 0, 0, 0, 0) -- i.e. the FIRST canonical
    //   channel x0 = log(open).  Any scalar close-only (x_3) fallback or any
    //   other single-axis surrogate is FORBIDDEN in any path that can
    //   influence theorem authority, including mt_trajectorize / walk_forward
    //   / run_backtest / run_verify_authority.  There is intentionally NO
    //   fallback branch in this function: when the normalized jet is absent
    //   we leave A_phi at zero, mark the FutureSection invalid, and let the
    //   authority gate fail the precise clause `theorem.past_jet_invalid`.
    //   This is fail-closed-by-construction; do not add a fallback.
    const bool have_normalized = !normalized_jet.coeff.empty();
    if (have_normalized) {
        for (int n = 0; n < N_phi; ++n)
            fs.A_phi[static_cast<usize>(n)] = axis_coefficient(normalized_jet, n);
        fs.valid = true;
        fs.invalid_reason.clear();
    } else {
        fs.valid = false;
        fs.invalid_reason = "normalized_jet_absent_scalar_fallback_forbidden";
    }

    Vec<int> boundaries = normalized_event_boundaries(sec.event_boundaries, t_anchor);
    const int L = std::min<int>(L_max, static_cast<int>(boundaries.size()));
    fs.sigma.resize(static_cast<usize>(L));
    fs.A_pi.assign(static_cast<usize>(L), Vec<cplx>(static_cast<usize>(std::max(1, N_pi)), cplx{}));

    // Strict ledger clause 13.1 (canonical Q=5 packet construction):
    //   The discontinuity packets A_pi MUST be built from the canonical
    //   five-channel market state (x0..x4) and projected by the SAME
    //   axis-selection convention used for the smooth coefficients A_phi.
    //   axis_coefficient(jet, n) selects multi-index alpha = (n, 0, 0, 0, 0),
    //   i.e. the first canonical channel x0 = log(open).  We therefore
    //   construct a 5-channel jump sample vector explicitly across all of
    //   (x0..x4) for transparency and audit, then project to the same
    //   canonical axis (x0 = log open).  Close-only (x3) jump samples are
    //   FORBIDDEN: they violate the Q=5 OHLCV channel cardinality of the
    //   framework anchor and contaminate the interstice / packet side of
    //   J_t^- in exactly the same way the removed scalar smooth fallback
    //   did on the A_phi side.
    constexpr int kQ_canonical = 5;
    if (have_normalized) {
        for (int l = 0; l < L; ++l) {
            const int boundary = boundaries[static_cast<usize>(l)];
            const int sigma_steps = std::max(1, t_anchor - boundary + 1);
            fs.sigma[static_cast<usize>(l)] = static_cast<real>(sigma_steps);

            // 5-channel jump samples J_q[eta] = x_q[boundary+eta] - x_q[boundary-1+eta]
            // built from the canonical market state.
            Vec<Vec<real>> J_channel(static_cast<usize>(kQ_canonical),
                                     Vec<real>(static_cast<usize>(std::max(1, N_pi)), 0.0));
            for (int eta = 0; eta < N_pi; ++eta) {
                const int right_idx = std::clamp(boundary + eta, 0, t_anchor);
                const int left_idx  = std::clamp(boundary - 1 + eta, 0, t_anchor);
                J_channel[0][static_cast<usize>(eta)] =
                    market_state.x0[static_cast<usize>(right_idx)] - market_state.x0[static_cast<usize>(left_idx)];
                J_channel[1][static_cast<usize>(eta)] =
                    market_state.x1[static_cast<usize>(right_idx)] - market_state.x1[static_cast<usize>(left_idx)];
                J_channel[2][static_cast<usize>(eta)] =
                    market_state.x2[static_cast<usize>(right_idx)] - market_state.x2[static_cast<usize>(left_idx)];
                J_channel[3][static_cast<usize>(eta)] =
                    market_state.x3[static_cast<usize>(right_idx)] - market_state.x3[static_cast<usize>(left_idx)];
                J_channel[4][static_cast<usize>(eta)] =
                    market_state.x4[static_cast<usize>(right_idx)] - market_state.x4[static_cast<usize>(left_idx)];
            }

            // Canonical axis projection: same axis as axis_coefficient(jet, n)
            // alpha = (n, 0, 0, 0, 0) -> channel 0 (x0 = log open).
            constexpr int kCanonicalProjectionAxis = 0;
            Vec<cplx> jump_samples(static_cast<usize>(std::max(1, N_pi)), cplx{});
            for (int eta = 0; eta < N_pi; ++eta) {
                jump_samples[static_cast<usize>(eta)] = cplx(
                    J_channel[static_cast<usize>(kCanonicalProjectionAxis)][static_cast<usize>(eta)], 0.0);
            }

            fs.A_pi[static_cast<usize>(l)][0] = jump_samples[0];
            for (int n = 1; n < N_pi; ++n) {
                const cplx delta = forward_difference_at_zero(jump_samples, n);
                fs.A_pi[static_cast<usize>(l)][static_cast<usize>(n)] =
                    delta / static_cast<real>(factorial_int(n));
            }
        }
    } else {
        // No normalized jet -> packet side has no canonical anchor either.
        // Fail-closed: leave A_pi/sigma at zero and let authority reject via
        // theorem.past_jet_invalid.
        for (int l = 0; l < L; ++l) {
            const int boundary = boundaries[static_cast<usize>(l)];
            const int sigma_steps = std::max(1, t_anchor - boundary + 1);
            fs.sigma[static_cast<usize>(l)] = static_cast<real>(sigma_steps);
        }
    }

    return fs;
}

cplx evaluate_smooth_future_section(const FutureSection& Jminus, real h) {
    // Strict ledger clause 13.1 / 13.6:
    //   smooth analytic continuation only -- A_phi polynomial; NO packet term.
    //   This is the carrier used by the four-way theorem identity
    //     E_h[F]_n = J_h[F]_n = N_h[b]_n = M_h[Z_n]
    //   which is an identity on the smooth jet shadow J[F], not on the
    //   packet-augmented future section.
    cplx v{};
    for (usize n = 0; n < Jminus.A_phi.size(); ++n)
        v += Jminus.A_phi[n] * std::pow(h, static_cast<real>(n));
    return v;
}

cplx evaluate_full_future_section(const FutureSection& Jminus, real h) {
    // Smooth A_phi shadow PLUS activated discontinuity packets A_pi.
    // For the trajectory / readout / risk layer where packets are intended.
    cplx v = evaluate_smooth_future_section(Jminus, h);
    for (usize l = 0; l < Jminus.sigma.size(); ++l) {
        const real sl = Jminus.sigma[l];
        if (h >= sl) {
            const real dh = h - sl;
            for (usize n = 0; n < Jminus.A_pi[l].size(); ++n)
                v += Jminus.A_pi[l][n] * std::pow(dh, static_cast<real>(n));
        }
    }
    return v;
}

cplx evaluate_future_section(const FutureSection& Jminus, real h) {
    // Back-compat alias = full packet-augmented evaluation.  New theorem-path
    // code MUST call evaluate_smooth_future_section explicitly.
    return evaluate_full_future_section(Jminus, h);
}

}  // namespace mt
