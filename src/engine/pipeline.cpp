#include "mt/engine/pipeline.hpp"
#include "mt/engine/extraction_engine.hpp"
#include "mt/engine/jet_engine.hpp"
#include "mt/engine/recurrence_engine.hpp"
#include "mt/engine/interstice_engine.hpp"
#include "mt/engine/trajectorizer_engine.hpp"
#include "mt/engine/risk_engine.hpp"
#include "mt/data/feature_map.hpp"
#include "mt/math/combinatorics.hpp"
#include "mt/math/companion.hpp"
#include "mt/math/finite_differences.hpp"
#include "mt/math/phase_torus.hpp"
#include "mt/math/taylor.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace mt {

namespace {

struct ContractSelfTest {
    bool jet_normalization_verified = false;
    real jet_normalization_err      = std::numeric_limits<real>::infinity();
    bool di_projection_verified     = false;
    real di_projection_err          = std::numeric_limits<real>::infinity();
};

real diff_max(const Vec<cplx>& a, const Vec<cplx>& b) {
    const usize n = std::min(a.size(), b.size());
    real m = 0.0;
    for (usize i = 0; i < n; ++i) m = std::max(m, std::abs(a[i] - b[i]));
    return m;
}

inline usize dense_size(int Q, int order_max) {
    usize n = 1;
    for (int q = 0; q < Q; ++q) n *= static_cast<usize>(order_max + 1);
    return n;
}

real max_jet_normalization_residual(const ExtractedCoefficients& extracted,
                                    const JetState&             normalized_jet) {
    if (extracted.A.empty() || normalized_jet.coeff.empty())
        return std::numeric_limits<real>::infinity();
    if (extracted.Q <= 0 || extracted.order_max < 0 || extracted.rho_m <= 0.0)
        return std::numeric_limits<real>::infinity();
    if (normalized_jet.Q != extracted.Q || normalized_jet.order_max != extracted.order_max)
        return std::numeric_limits<real>::infinity();

    const usize n_expected = dense_size(extracted.Q, extracted.order_max);
    if (extracted.A.size() < n_expected || normalized_jet.coeff.size() < n_expected)
        return std::numeric_limits<real>::infinity();

    auto layout = make_multi_index_layout(extracted.Q, extracted.order_max);
    Vec<Vec<int>> idx;
    enumerate_multi_indices(extracted.Q, extracted.order_max, idx);

    real max_res = 0.0;
    for (const auto& alpha : idx) {
        const usize k = flatten_multi_index(alpha, layout);
        const int degree = multi_degree(alpha);
        const cplx expected = extracted.A[k] * std::pow(extracted.rho_m, -static_cast<real>(degree));
        max_res = std::max(max_res, std::abs(normalized_jet.coeff[k] - expected));
    }
    return max_res;
}

// Per-channel anchor germ G_q(z) for the canonical OHLCV state.  Each axis
// q ∈ {0,1,2,3,4} maps to x_q[t] = (log open, log high, log low, log close,
// log(1+volume/volume_norm)).  We build a 1D Newton-Gregory analytic germ from
// the per-channel past sequence anchored at t = t_anchor:
//
//      G_q(z)  =  Σ_n (∇^n x_q[t]/n!) · z^n
//
// The full multivariate analytic germ feeding the phase-torus extractor is the
// separable sum F(z_0,...,z_{Q-1}) = Σ_q G_q(z_q).  This recovers each
// channel's Newton expansion on its own axis (diagonal multi-indices) while
// keeping mixed multi-indices identically zero — the simplest theorem-faithful
// multichannel construction over the canonical state.
Vec<Vec<cplx>> build_anchor_local_coeffs_per_channel(const MarketStateColumns& s,
                                                     int                       t_anchor,
                                                     int                       order_max) {
    const int Q = 5;
    Vec<Vec<cplx>> coeffs(static_cast<usize>(Q),
                          Vec<cplx>(static_cast<usize>(order_max + 1), cplx{}));
    if (s.n == 0) return coeffs;
    const int t = std::clamp(t_anchor, 0, static_cast<int>(s.n) - 1);

    const Vec<real>* xs[5] = {&s.x0, &s.x1, &s.x2, &s.x3, &s.x4};
    for (int q = 0; q < Q; ++q) {
        const Vec<real>& x = *xs[q];
        Vec<cplx> hist;
        hist.reserve(static_cast<usize>(t + 1));
        for (int i = 0; i <= t && static_cast<usize>(i) < x.size(); ++i)
            hist.push_back(cplx(x[static_cast<usize>(i)], 0.0));
        Vec<cplx> nabla;
        finite_backward_differences(hist, static_cast<int>(hist.size()) - 1, order_max, nabla);
        Vec<cplx>& C = coeffs[static_cast<usize>(q)];
        for (int n = 0; n <= order_max && static_cast<usize>(n) < nabla.size(); ++n)
            C[static_cast<usize>(n)] = nabla[static_cast<usize>(n)]
                                     / static_cast<real>(factorial_int(n));
    }
    return coeffs;
}

ExtractedCoefficients extract_anchor_phase_torus_coeffs(const MarketStateColumns& s,
                                                        int                  t_anchor,
                                                        const EngineConfig&  cfg) {
    const int max_data_order = std::max(1, std::min(t_anchor, cfg.torus.order_max));
    const int order_max = std::max(1, std::min(cfg.torus.order_max, max_data_order));
    const int M_per_dim = std::max(8, cfg.torus.M_per_dim);
    const real rho_m = cfg.torus.rho_m;
    // Full canonical-state extraction over Q = 5 channels (x_0..x_4).
    // The legacy single-axis (Q = 1) close-only path has been removed; it is
    // mathematically incompatible with an OHLCV/OCHLV trajectorizer.
    const int Q = 5;

    const Vec<Vec<cplx>> per_channel = build_anchor_local_coeffs_per_channel(s, t_anchor, order_max);

    Vec<FieldCpu1D> G(static_cast<usize>(Q));
    for (int q = 0; q < Q; ++q) {
        const Vec<cplx> Cq = per_channel[static_cast<usize>(q)];
        G[static_cast<usize>(q)] = [Cq](real x, real y) -> cplx {
            const cplx z(x, y);
            cplx acc{};
            cplx zp(1.0, 0.0);
            for (const cplx& c : Cq) {
                acc += c * zp;
                zp *= z;
            }
            return acc;
        };
    }

    // Anchor the extraction at the origin of each channel's local Taylor germ:
    // (z_q = 0).  The MarketState log-coordinates already absorb level shifts,
    // so the Cauchy ring of radius rho_m around the origin captures the local
    // analytic structure of every channel.
    Vec<real> z_real(static_cast<usize>(Q), 0.0);
    Vec<real> z_imag(static_cast<usize>(Q), 0.0);
    return extract_phase_torus_separable_cpu(G, z_real, z_imag, rho_m,
                                             order_max, M_per_dim);
}

real max_carrier_residual(const AnchorArtifacts& art) {
    // Strict ledger 13.6: u_carrier feeds the four-way theorem identity, so
    // the comparison MUST use the smooth-only future section.  Using the
    // full (packet-augmented) evaluation here would silently make the
    // residual nonzero whenever any sigma_l <= n, contaminating
    // causality.carrier_from_past_jet by construction.
    if (art.u_carrier.empty()) return std::numeric_limits<real>::infinity();
    real m = 0.0;
    for (usize n = 0; n < art.u_carrier.size(); ++n) {
        const cplx expected = evaluate_smooth_future_section(art.jet_minus, static_cast<real>(n));
        m = std::max(m, std::abs(art.u_carrier[n] - expected));
    }
    return m;
}

real max_interstice_pde_residual(const IntersticeTrajectory& tr) {
    if (tr.s_count < 3 || tr.H < 3) return std::numeric_limits<real>::infinity();
    const usize S = static_cast<usize>(tr.s_count);
    const usize H = static_cast<usize>(tr.H);
    if (tr.T_grid.size() < S * H || tr.s_grid.size() < S || tr.h_grid.size() < H || tr.kappa.size() < S)
        return std::numeric_limits<real>::infinity();

    auto at = [&](int s, int h) -> const cplx& {
        return tr.T_grid[static_cast<usize>(s) * H + static_cast<usize>(h)];
    };

    real max_res = 0.0;
    bool sampled = false;
    for (int si = 1; si + 1 < tr.s_count; ++si) {
        const real ds = tr.s_grid[static_cast<usize>(si + 1)] - tr.s_grid[static_cast<usize>(si - 1)];
        if (ds == 0.0) continue;
        for (int hi = 1; hi + 1 < tr.H; ++hi) {
            const real dh = tr.h_grid[static_cast<usize>(hi + 1)] - tr.h_grid[static_cast<usize>(hi - 1)];
            if (dh == 0.0) continue;
            const cplx dTds = (at(si + 1, hi) - at(si - 1, hi)) / ds;
            const cplx dTdh = (at(si, hi + 1) - at(si, hi - 1)) / dh;
            const cplx rhs  = (1.0 + tr.kappa[static_cast<usize>(si)] * tr.h_grid[static_cast<usize>(hi)]) * dTdh;
            max_res = std::max(max_res, std::abs(dTds - rhs));
            sampled = true;
        }
    }
    return sampled ? max_res : std::numeric_limits<real>::infinity();
}

real max_interstice_recurrence_residual(const IntersticeTrajectory& tr,
                                        const RecurrenceCoefficients& rc) {
    if (rc.r <= 0) return 0.0;
    if (tr.H <= rc.r) return std::numeric_limits<real>::infinity();

    // Authority clause is enforced on the deployed signal path T(h), not across
    // all transported s-slices that may obey an s-modulated characteristic law.
    const usize H = static_cast<usize>(tr.H);
    const Vec<cplx>* seq = nullptr;
    if (tr.T.size() >= H) {
        seq = &tr.T;
    } else if (tr.T_grid.size() >= H) {
        seq = &tr.T_grid;
    }
    if (seq == nullptr) return std::numeric_limits<real>::infinity();

    real max_res = 0.0;
    bool sampled = false;
    for (int n = 0; n + rc.r < tr.H; ++n) {
        cplx res = (*seq)[static_cast<usize>(n + rc.r)];
        for (int j = 0; j < rc.r; ++j)
            res += rc.c[static_cast<usize>(j)] * (*seq)[static_cast<usize>(n + j)];
        max_res = std::max(max_res, std::abs(res));
        sampled = true;
    }

    // Strict mode: verify recurrence on each transported s-slice with locally
    // fitted coefficients of the same order, then aggregate worst residual.
    if (tr.s_count > 0 && tr.T_grid.size() >= static_cast<usize>(tr.s_count) * H && tr.H >= 2 * rc.r) {
        for (int si = 0; si < tr.s_count; ++si) {
            Vec<cplx> slice(H, cplx{});
            for (usize h = 0; h < H; ++h)
                slice[h] = tr.T_grid[static_cast<usize>(si) * H + h];

            try {
                RecurrenceCoefficients local_rc = solve_recurrence_from_hankel(slice, rc.r);
                for (int n = 0; n + local_rc.r < tr.H; ++n) {
                    cplx res = slice[static_cast<usize>(n + local_rc.r)];
                    for (int j = 0; j < local_rc.r; ++j)
                        res += local_rc.c[static_cast<usize>(j)] * slice[static_cast<usize>(n + j)];
                    max_res = std::max(max_res, std::abs(res));
                    sampled = true;
                }
            } catch (...) {
                return std::numeric_limits<real>::infinity();
            }
        }
    }

    return sampled ? max_res : std::numeric_limits<real>::infinity();
}

real max_abs_complex(const Vec<cplx>& v) {
    real m = 0.0;
    for (const auto& x : v) m = std::max(m, std::abs(x));
    return m;
}

real max_abs_real(const Vec<real>& v) {
    real m = 0.0;
    for (const auto& x : v) m = std::max(m, std::abs(x));
    return m;
}

bool price_path_is_identity(const Vec<real>& p) {
    if (p.empty()) return true;
    for (real x : p) {
        if (std::abs(x - 1.0) > 1e-12) return false;
    }
    return true;
}

struct LocalTheoremSectorCertificate {
    int  h_coeff_cap      = -1;
    int  h_packet_cap     = -1;
    int  h_condition_cap  = -1;
    int  h_structural_cap = -1;
    int  h_residual_cap   = -1;
    int  h_valid_max      = -1;
    int  h_valid_count    = 0;
    real h_valid_fraction = 0.0;
    real coeff_radius_estimate = 0.0;
    real recurrence_condition  = std::numeric_limits<real>::infinity();
    bool verified = false;
};

struct LocalResidualSummary {
    real err_continuous_jet   = std::numeric_limits<real>::infinity();
    real err_jet_newton       = std::numeric_limits<real>::infinity();
    real err_newton_matrix    = std::numeric_limits<real>::infinity();
    real err_matrix_rational  = std::numeric_limits<real>::infinity();
    real err_rational_spectral= std::numeric_limits<real>::infinity();
    bool finite = false;
};

constexpr int kMinTheoremValidHCount = 2;

int clamp_h(int h, int H) {
    if (H < 0) return -1;
    if (h < 0) return -1;
    return std::min(h, H);
}

real estimate_smooth_radius(const FutureSection& jet_minus) {
    if (jet_minus.A_phi.empty()) return 0.0;

    const real a0 = std::max<real>(std::abs(jet_minus.A_phi.front()), 1.0);
    real r_est = std::numeric_limits<real>::infinity();
    for (usize n = 1; n < jet_minus.A_phi.size(); ++n) {
        const real an = std::abs(jet_minus.A_phi[n]);
        if (an <= 1e-18) continue;
        const real rn = std::pow(a0 / an, 1.0 / static_cast<real>(n));
        if (std::isfinite(rn) && rn > 0.0)
            r_est = std::min(r_est, rn);
    }

    if (!std::isfinite(r_est) || r_est <= 0.0)
        return std::numeric_limits<real>::infinity();
    return r_est;
}

int coeff_cap_h(const FutureSection& jet_minus, int H, real& out_radius_estimate) {
    out_radius_estimate = estimate_smooth_radius(jet_minus);
    if (!std::isfinite(out_radius_estimate)) return H;
    const real conservative = 0.9 * out_radius_estimate;
    if (!std::isfinite(conservative) || conservative < 0.0) return -1;
    return clamp_h(static_cast<int>(std::floor(conservative)), H);
}

int packet_cap_h(const FutureSection& jet_minus, int H) {
    real min_sigma = std::numeric_limits<real>::infinity();
    for (real s : jet_minus.sigma) {
        if (s > 0.0 && s < min_sigma) min_sigma = s;
    }
    if (!std::isfinite(min_sigma)) return H;
    return clamp_h(static_cast<int>(std::floor(min_sigma - 1e-9)), H);
}

real recurrence_condition_proxy(const RecurrencePipelineOut& rp) {
    if (rp.rec.r <= 0) return 1.0;

    real coeff_norm = 1.0;
    for (const cplx& c : rp.rec.c) coeff_norm += std::abs(c);

    real lambda_max = 0.0;
    real lambda_min = std::numeric_limits<real>::infinity();
    for (const auto& m : rp.spectrum.modes) {
        const real a = std::abs(m.lambda);
        lambda_max = std::max(lambda_max, a);
        if (a > 1e-12) lambda_min = std::min(lambda_min, a);
    }

    real lambda_ratio = 1.0;
    if (lambda_max > 0.0) {
        if (!std::isfinite(lambda_min) || lambda_min <= 1e-12)
            lambda_ratio = lambda_max / 1e-12;
        else
            lambda_ratio = lambda_max / lambda_min;
    }

    const real cond = coeff_norm * std::max<real>(1.0, lambda_ratio);
    return std::isfinite(cond) && cond > 0.0 ? cond : std::numeric_limits<real>::infinity();
}

int condition_cap_h(real cond, int H) {
    if (!std::isfinite(cond)) return 0;
    if (cond <= 1e2) return H;
    if (cond <= 1e3) return std::max(1, H / 2);
    if (cond <= 1e4) return std::max(1, H / 4);
    if (cond <= 1e6) return std::max(1, H / 8);
    return 0;
}

int residual_cap_h(const EvolutionEquality& ee,
                   int                      H,
                   real                     tol_evolution,
                   real                     tol_closure) {
    if (H < 0) return -1;

    usize max_h = static_cast<usize>(H);
    max_h = std::min(max_h, ee.U_continuous.empty() ? 0 : ee.U_continuous.size() - 1);
    max_h = std::min(max_h, ee.U_jet.empty()        ? 0 : ee.U_jet.size() - 1);
    max_h = std::min(max_h, ee.U_newton.empty()     ? 0 : ee.U_newton.size() - 1);
    max_h = std::min(max_h, ee.U_matrix.empty()     ? 0 : ee.U_matrix.size() - 1);

    if (ee.U_continuous.empty() || ee.U_jet.empty() || ee.U_newton.empty() || ee.U_matrix.empty())
        return -1;

    if (ee.closed_regime) {
        if (ee.U_rational.empty() || ee.U_spectral.empty()) return -1;
        max_h = std::min(max_h, ee.U_rational.size() - 1);
        max_h = std::min(max_h, ee.U_spectral.size() - 1);
    }

    int last_ok = -1;
    for (usize h = 0; h <= max_h; ++h) {
        const real r_cj = std::abs(ee.U_continuous[h] - ee.U_jet[h]);
        const real r_jn = std::abs(ee.U_jet[h] - ee.U_newton[h]);
        const real r_nm = std::abs(ee.U_newton[h] - ee.U_matrix[h]);
        if (!std::isfinite(r_cj) || !std::isfinite(r_jn) || !std::isfinite(r_nm)) break;
        if (!(r_cj < tol_evolution) || !(r_jn < tol_evolution) || !(r_nm < tol_evolution)) break;

        if (ee.closed_regime) {
            const real r_mr = std::abs(ee.U_matrix[h] - ee.U_rational[h]);
            const real r_rs = std::abs(ee.U_rational[h] - ee.U_spectral[h]);
            if (!std::isfinite(r_mr) || !std::isfinite(r_rs)) break;
            if (!(r_mr < tol_closure) || !(r_rs < tol_closure)) break;
        }
        last_ok = static_cast<int>(h);
    }

    return clamp_h(last_ok, H);
}

LocalResidualSummary summarize_local_residuals(const EvolutionEquality& ee,
                                               int                      h_max) {
    LocalResidualSummary out;
    if (h_max < 0) return out;

    const usize need = static_cast<usize>(h_max) + 1;
    if (ee.U_continuous.size() < need || ee.U_jet.size() < need
        || ee.U_newton.size() < need || ee.U_matrix.size() < need) {
        return out;
    }

    out.err_continuous_jet = 0.0;
    out.err_jet_newton = 0.0;
    out.err_newton_matrix = 0.0;
    out.err_matrix_rational = 0.0;
    out.err_rational_spectral = 0.0;

    for (int h = 0; h <= h_max; ++h) {
        const usize i = static_cast<usize>(h);
        out.err_continuous_jet = std::max(out.err_continuous_jet,
                                          std::abs(ee.U_continuous[i] - ee.U_jet[i]));
        out.err_jet_newton = std::max(out.err_jet_newton,
                                      std::abs(ee.U_jet[i] - ee.U_newton[i]));
        out.err_newton_matrix = std::max(out.err_newton_matrix,
                                         std::abs(ee.U_newton[i] - ee.U_matrix[i]));

        if (ee.closed_regime) {
            if (ee.U_rational.size() < need || ee.U_spectral.size() < need) {
                out.err_matrix_rational = std::numeric_limits<real>::infinity();
                out.err_rational_spectral = std::numeric_limits<real>::infinity();
                return out;
            }
            out.err_matrix_rational = std::max(out.err_matrix_rational,
                                               std::abs(ee.U_matrix[i] - ee.U_rational[i]));
            out.err_rational_spectral = std::max(out.err_rational_spectral,
                                                 std::abs(ee.U_rational[i] - ee.U_spectral[i]));
        }
    }

    out.finite = std::isfinite(out.err_continuous_jet)
              && std::isfinite(out.err_jet_newton)
              && std::isfinite(out.err_newton_matrix)
              && (!ee.closed_regime
                  || (std::isfinite(out.err_matrix_rational)
                      && std::isfinite(out.err_rational_spectral)));
    return out;
}

LocalTheoremSectorCertificate build_local_theorem_sector(const AnchorArtifacts& art,
                                                         const EngineConfig&    cfg) {
    LocalTheoremSectorCertificate cert;
    const int H = std::max(cfg.H, 0);

    cert.h_coeff_cap = coeff_cap_h(art.jet_minus, H, cert.coeff_radius_estimate);
    cert.h_packet_cap = packet_cap_h(art.jet_minus, H);
    cert.recurrence_condition = recurrence_condition_proxy(art.recurrence);
    cert.h_condition_cap = condition_cap_h(cert.recurrence_condition, H);
    cert.h_structural_cap = clamp_h(std::min({cert.h_coeff_cap,
                                              cert.h_packet_cap,
                                              cert.h_condition_cap}),
                                    H);

    cert.h_residual_cap = residual_cap_h(art.evolution,
                                         H,
                                         cfg.tol_evolution,
                                         cfg.tol_closure);

    cert.h_valid_max = clamp_h(std::min(cert.h_structural_cap, cert.h_residual_cap), H);
    cert.h_valid_count = cert.h_valid_max >= 0 ? (cert.h_valid_max + 1) : 0;
    cert.h_valid_fraction = (H >= 0)
        ? static_cast<real>(cert.h_valid_count) / static_cast<real>(H + 1)
        : 0.0;
    cert.verified = cert.h_valid_count >= kMinTheoremValidHCount;
    return cert;
}

Vec<real> build_interstice_pde_residual_grid(const IntersticeTrajectory& tr) {
    const usize S = static_cast<usize>(std::max(tr.s_count, 0));
    const usize H = static_cast<usize>(std::max(tr.H, 0));
    Vec<real> out(S * H, 0.0);
    if (S < 3 || H < 3) return out;
    if (tr.T_grid.size() < S * H || tr.s_grid.size() < S || tr.h_grid.size() < H || tr.kappa.size() < S)
        return out;

    auto at = [&](int s, int h) -> const cplx& {
        return tr.T_grid[static_cast<usize>(s) * H + static_cast<usize>(h)];
    };

    for (int si = 1; si + 1 < tr.s_count; ++si) {
        const real ds = tr.s_grid[static_cast<usize>(si + 1)] - tr.s_grid[static_cast<usize>(si - 1)];
        if (ds == 0.0) continue;
        for (int hi = 1; hi + 1 < tr.H; ++hi) {
            const real dh = tr.h_grid[static_cast<usize>(hi + 1)] - tr.h_grid[static_cast<usize>(hi - 1)];
            if (dh == 0.0) continue;
            const cplx dTds = (at(si + 1, hi) - at(si - 1, hi)) / ds;
            const cplx dTdh = (at(si, hi + 1) - at(si, hi - 1)) / dh;
            const cplx rhs  = (1.0 + tr.kappa[static_cast<usize>(si)] * tr.h_grid[static_cast<usize>(hi)]) * dTdh;
            out[static_cast<usize>(si) * H + static_cast<usize>(hi)] = std::abs(dTds - rhs);
        }
    }
    return out;
}

Vec<real> build_interstice_recurrence_residual_grid(const IntersticeTrajectory& tr,
                                                    const RecurrenceCoefficients& rc) {
    const usize S = static_cast<usize>(std::max(tr.s_count, 0));
    const usize H = static_cast<usize>(std::max(tr.H, 0));
    Vec<real> out(S * H, 0.0);
    if (rc.r <= 0 || S == 0 || H == 0) return out;
    if (tr.T_grid.size() < S * H || tr.H <= rc.r) return out;

    for (int si = 0; si < tr.s_count; ++si) {
        for (int n = 0; n + rc.r < tr.H; ++n) {
            cplx res = tr.T_grid[static_cast<usize>(si) * H + static_cast<usize>(n + rc.r)];
            for (int j = 0; j < rc.r; ++j) {
                res += rc.c[static_cast<usize>(j)] * tr.T_grid[static_cast<usize>(si) * H + static_cast<usize>(n + j)];
            }
            out[static_cast<usize>(si) * H + static_cast<usize>(n + rc.r)] = std::abs(res);
        }
    }
    return out;
}

ContractSelfTest run_contract_self_tests() {
    ContractSelfTest out;

    // Contract clause: J_alpha[F] = rho^{-alpha} A_alpha = (1/alpha!) ∂^alpha F.
    try {
        const cplx c0(0.7, -0.2);
        const cplx c1(1.3,  0.1);
        const cplx c2(-0.4, 0.3);
        const real rho = 0.05;
        Vec<real> z_re{0.2};
        Vec<real> z_im{-0.1};

        FieldCpu F = [=](const Vec<real>& re, const Vec<real>& im) {
            cplx z(re[0], im[0]);
            return c0 + c1 * z + c2 * z * z;
        };
        ExtractedCoefficients ec = extract_phase_torus_cpu(F, z_re, z_im, rho, 1, 2, 64);
        auto layout = make_multi_index_layout(1, 2);
        const usize i0 = flatten_multi_index(Vec<int>{0}, layout);
        const usize i1 = flatten_multi_index(Vec<int>{1}, layout);
        const usize i2 = flatten_multi_index(Vec<int>{2}, layout);

        const cplx z(z_re[0], z_im[0]);
        const cplx j0_expected = c0 + c1 * z + c2 * z * z;
        const cplx j1_expected = c1 + cplx(2.0, 0.0) * c2 * z;
        const cplx j2_expected = c2;

        const cplx j0 = ec.A[i0];
        const cplx j1 = ec.A[i1] / rho;
        const cplx j2 = ec.A[i2] / (rho * rho);

        out.jet_normalization_err = std::max({std::abs(j0 - j0_expected),
                                              std::abs(j1 - j1_expected),
                                              std::abs(j2 - j2_expected)});
        out.jet_normalization_verified = out.jet_normalization_err < 1e-6;
    } catch (...) {
        out.jet_normalization_verified = false;
    }

    // Contract clause: D_i I_i = I_i D_i = Pi_{eta_i,perp} on truncated jets.
    try {
        JetState J;
        J.Q = 2;
        J.order_max = 4;
        J.coeff.assign(25, cplx{});

        auto layout = make_multi_index_layout(J.Q, J.order_max);
        Vec<Vec<int>> idx;
        enumerate_multi_indices(J.Q, J.order_max, idx);
        for (const auto& alpha : idx) {
            if (multi_degree(alpha) > J.order_max - 2) continue;
            const usize k = flatten_multi_index(alpha, layout);
            J.coeff[k] = cplx(0.1 * static_cast<real>(k + 1), -0.03 * static_cast<real>((k % 5) + 1));
        }

        const real eps = 0.25;
        real m = 0.0;
        for (int axis = 0; axis < J.Q; ++axis) {
            const JetState perp = project_perp(J, axis);
            const JetState Jp   = project_perp(J, axis);
            const JetState di   = apply_D(apply_I(Jp, axis, eps), axis, eps);
            const JetState id   = apply_I(apply_D(Jp, axis, eps), axis, eps);
            m = std::max(m, diff_max(di.coeff, perp.coeff));
            m = std::max(m, diff_max(id.coeff, perp.coeff));
        }
        out.di_projection_err = m;
        out.di_projection_verified = (m < 1e-10);
    } catch (...) {
        out.di_projection_verified = false;
    }

    return out;
}

const ContractSelfTest& contract_self_tests() {
    static const ContractSelfTest cached = run_contract_self_tests();
    return cached;
}

}  // namespace

AuthoritativeStatus validate_anchor_authoritativeness(const AnchorArtifacts& art,
                                                      const EngineConfig& cfg) {
    AuthoritativeStatus st;
    auto fail = [&](const char* clause) { st.failed_clauses.push_back(clause); };

    st.gpu_mode = "cpu_authoritative";   // Path B policy until kernels are fully authoritative.
    st.closure_claimed = art.evolution.closed_regime;

    const LocalTheoremSectorCertificate theorem_sector = build_local_theorem_sector(art, cfg);
    st.theorem_local_sector_verified = theorem_sector.verified;
    st.theorem_valid_h_max = theorem_sector.h_valid_max;
    st.theorem_valid_h_count = theorem_sector.h_valid_count;
    st.theorem_valid_h_fraction = theorem_sector.h_valid_fraction;
    st.theorem_valid_h_coeff_cap = theorem_sector.h_coeff_cap;
    st.theorem_valid_h_packet_cap = theorem_sector.h_packet_cap;
    st.theorem_valid_h_condition_cap = theorem_sector.h_condition_cap;
    st.theorem_valid_h_residual_cap = theorem_sector.h_residual_cap;
    st.theorem_coeff_radius_estimate = theorem_sector.coeff_radius_estimate;
    st.theorem_recurrence_condition = theorem_sector.recurrence_condition;

    const LocalResidualSummary local_res = summarize_local_residuals(art.evolution,
                                                                     st.theorem_valid_h_max);

    st.max_theorem_residual = std::max({local_res.err_continuous_jet,
                                        local_res.err_jet_newton,
                                        local_res.err_newton_matrix});
    st.max_jet_normalization_residual = max_jet_normalization_residual(art.extracted_phase_torus,
                                                                        art.normalized_jet);
    st.max_closure_residual = std::max(local_res.err_matrix_rational,
                                       local_res.err_rational_spectral);
    st.max_interstice_pde_residual = max_interstice_pde_residual(art.trajectory);
    st.max_interstice_recurrence_residual = max_interstice_recurrence_residual(art.trajectory,
                                                                                art.recurrence.rec);
    st.err_phase_torus_to_future_section = local_res.err_continuous_jet;

    st.carrier_continuous_present = !art.evolution.U_continuous.empty();
    st.carrier_jet_present = !art.evolution.U_jet.empty();
    st.carrier_newton_present = !art.evolution.U_newton.empty();
    st.carrier_matrix_present = !art.evolution.U_matrix.empty();
    st.carrier_rational_present = !art.evolution.U_rational.empty();
    st.carrier_spectral_present = !art.evolution.U_spectral.empty();
    if (!art.evolution.closed_regime) {
        st.carrier_rational_reason_missing = "not_applicable_open_regime";
        st.carrier_spectral_reason_missing = "not_applicable_open_regime";
    } else {
        st.carrier_rational_reason_missing = st.carrier_rational_present ? "" : "fit_failed";
        st.carrier_spectral_reason_missing = st.carrier_spectral_present ? "" : "fit_failed";
    }

    st.nontrivial_future_field = max_abs_complex(art.evolution.U_continuous) > 1e-12;
    st.nontrivial_interstice_field = (max_abs_complex(art.trajectory.T) > 1e-12)
                                  && (max_abs_complex(art.trajectory.L) > 1e-12)
                                  && (max_abs_complex(art.trajectory.R) > 1e-12)
                                  && !price_path_is_identity(art.trajectory.P);
    st.event_state_nonempty = !art.section.event_boundaries.empty()
                           || art.section.single_sector_smooth_regime_only;
    // Production authority requires events from genuine OCHLV packet dynamics,
    // not from the deterministic max-transition synthetic fallback rescue.
    st.event_state_from_genuine_packet_dynamics =
        art.section.single_sector_smooth_regime_only
        || (art.section.event_boundaries_from_genuine_packets
            && !art.section.event_boundaries.empty()
            && !art.section.event_packets.empty());
    st.spectral_realization_nonempty = !art.evolution.closed_regime
                                    || (st.carrier_rational_present && st.carrier_spectral_present);
    st.carrier_not_constant_identity = !price_path_is_identity(art.trajectory.P);

    // Reject trivial r=1 single-mode closures whenever the event/spectral/
    // carrier geometry implies richer structure.  A single near-unit-root mode
    // can pass narrow numerical checks while being structurally useless for
    // path formation, so we treat it as non-authoritative when the surrounding
    // structure shows it ought to be richer.
    {
        const int r = art.recurrence.rec.r;
        const usize nmodes = art.recurrence.spectrum.modes.size();
        const bool trivial_r1 = (r == 1) && (nmodes <= 1);
        const bool implies_richer_structure =
               !art.section.event_boundaries.empty()
            || (art.section.event_packets.size() > 1)
            || (art.jet_minus.sigma.size() > 0);
        st.closure_nontrivial = !(trivial_r1 && implies_richer_structure);
    }

    const bool phase_torus_extraction_executed = !art.extracted_phase_torus.A.empty()
                                              && (art.extracted_phase_torus.Q > 0)
                                              && (art.extracted_phase_torus.order_max >= 0)
                                              && (art.extracted_phase_torus.rho_m > 0.0);
    const bool normalized_jet_built_from_extracted = art.jet_from_phase_torus
                                                  && !art.normalized_jet.coeff.empty()
                                                  && (art.normalized_jet.Q == art.extracted_phase_torus.Q)
                                                  && (art.normalized_jet.order_max == art.extracted_phase_torus.order_max);
    const bool jet_normalization_identity_verified = std::isfinite(st.max_jet_normalization_residual)
                                                  && (st.max_jet_normalization_residual < cfg.tol_evolution);
    const bool jet_semigroup_used_normalized_path = art.evolution.jet_semigroup_from_normalized_jet;
    const bool theorem_residual_on_phase_torus_path = art.evolution.theorem_residual_on_phase_torus_path;

    st.causality_anchor_truncation_verified = (art.t_anchor >= 0);
    st.causality_carrier_verified = max_carrier_residual(art) < cfg.tol_evolution;
    st.jet_normalization_identity_verified = jet_normalization_identity_verified;
    st.phase_torus_path_verified = phase_torus_extraction_executed
                                && normalized_jet_built_from_extracted
                                && jet_normalization_identity_verified
                                && jet_semigroup_used_normalized_path
                                && theorem_residual_on_phase_torus_path;
    if (!st.causality_anchor_truncation_verified) fail("causality.anchor_truncation");
    if (!st.causality_carrier_verified) fail("causality.carrier_from_past_jet");
    if (!phase_torus_extraction_executed) fail("theorem.phase_torus_extraction_executed");
    if (!normalized_jet_built_from_extracted) fail("theorem.phase_torus_normalized_jet_built");
    if (!jet_normalization_identity_verified) fail("theorem.phase_torus_jet_normalization_identity");
    if (!jet_semigroup_used_normalized_path) fail("theorem.phase_torus_jet_semigroup_path");
    if (!theorem_residual_on_phase_torus_path) fail("theorem.phase_torus_residual_evaluated");

    // Finite exact-sector analogue: theorem equality is certified only on a
    // local anchor-dependent sector H_valid, not assumed globally on 0..H.
    if (!st.theorem_local_sector_verified)
        fail("theorem.local_valid_sector");

    if (!(local_res.err_continuous_jet < cfg.tol_evolution)) fail("theorem.continuous_equals_jet");
    if (!(local_res.err_jet_newton < cfg.tol_evolution)) fail("theorem.jet_equals_newton");
    if (!(local_res.err_newton_matrix < cfg.tol_evolution)) fail("theorem.newton_equals_matrix");
    if (!(st.err_phase_torus_to_future_section < cfg.tol_evolution)) fail("phase_torus.future_section_residual");

    if (!st.nontrivial_future_field) fail("future_field.nontrivial");
    if (!st.nontrivial_interstice_field) fail("interstice.nontrivial_field");
    if (!st.event_state_nonempty) fail("event_state.nonempty");
    if (!st.event_state_from_genuine_packet_dynamics) fail("event_state.genuine_packet_dynamics");
    if (!st.closure_nontrivial) fail("recurrence.nontrivial_closure");
    if (!st.spectral_realization_nonempty) fail("carrier.spectral_or_rational_nonempty");
    if (!st.carrier_not_constant_identity) fail("carrier.not_constant_identity");

    const bool empty_event_structure = !art.jet_minus.A_phi.empty()
                                    && art.jet_minus.sigma.empty()
                                    && art.jet_minus.A_pi.empty();
    if (empty_event_structure && !art.section.single_sector_smooth_regime_only)
        fail("event_state.empty_structure");

    // Theorem-faithfulness clause 13.1: J_t^- must be built from the canonical
    // Q=5 normalized jet, not from a scalar close-only fallback.  When the
    // FutureSection is invalid (e.g. normalized_jet absent) the production
    // gate must fail with the precise machine-readable reason.
    if (!art.jet_minus.valid) {
        fail("theorem.past_jet_invalid");
        st.past_jet_invalid_reason = art.jet_minus.invalid_reason;
    }

    st.closure_verified = false;
    if (art.evolution.closed_regime) {
        st.closure_verified = (local_res.err_matrix_rational < cfg.tol_closure)
                           && (local_res.err_rational_spectral < cfg.tol_closure);
        if (!(local_res.err_matrix_rational < cfg.tol_closure))
            fail("closure.matrix_equals_rational");
        if (!(local_res.err_rational_spectral < cfg.tol_closure))
            fail("closure.rational_equals_spectral");
    }

    const bool interstice_pde_ok = st.max_interstice_pde_residual < cfg.tol_interstice;
    const bool interstice_recurrence_ok = (art.recurrence.rec.r <= 0)
                                       || (st.max_interstice_recurrence_residual < cfg.tol_interstice);

    st.interstice_verified = interstice_pde_ok && interstice_recurrence_ok;
    if (!interstice_pde_ok)
        fail("interstice.pde_transport");
    if (art.recurrence.rec.r > 0 && !interstice_recurrence_ok)
        fail("interstice.recurrence");

    const ContractSelfTest& self = contract_self_tests();
    st.jet_normalization_verified = self.jet_normalization_verified;
    st.di_projection_verified = self.di_projection_verified;
    if (!self.jet_normalization_verified) fail("operators.jet_normalization");
    if (!self.di_projection_verified) fail("operators.DI_projection");

    st.theorem_authoritative = st.failed_clauses.empty();
    st.implementation_authoritative = (st.gpu_mode == "authoritative");
    if (st.theorem_authoritative
        && art.recurrence.rec.r > 0
        && st.closure_claimed
        && st.closure_verified
        && !art.recurrence.spectrum.modes.empty()) {
        st.regime = AuthorityRegime::FiniteClosedRegime;
    } else if (st.theorem_authoritative
            && st.interstice_verified
            && st.phase_torus_path_verified
            && st.nontrivial_future_field) {
        st.regime = AuthorityRegime::OpenButAuthoritativeIntersticeRegime;
    } else {
        st.regime = AuthorityRegime::RejectedNonAuthoritativeRegime;
    }

    st.production_ready = false;
    st.production_ready = art.evolution_ok
                       && st.theorem_authoritative
                       && st.implementation_authoritative
                       && (!st.closure_claimed || st.closure_verified)
                       && st.interstice_verified
                       && st.causality_anchor_truncation_verified
                       && st.causality_carrier_verified
                       && st.phase_torus_path_verified
                       && st.jet_normalization_identity_verified
                       && st.di_projection_verified
                       && st.failed_clauses.empty()
                       && st.nontrivial_future_field
                       && st.nontrivial_interstice_field
                       && st.event_state_nonempty
                       && st.event_state_from_genuine_packet_dynamics
                       && st.closure_nontrivial
                       && st.spectral_realization_nonempty
                       && st.carrier_not_constant_identity;

    st.authoritative = st.production_ready;
    return st;
}

AnchorArtifacts run_anchor(const CandleColumns& candles,
                           int t_anchor,
                           const EngineConfig& cfg)
{
    AnchorArtifacts art;
    art.t_anchor = t_anchor;
    if (t_anchor >= 0 && static_cast<usize>(t_anchor) < candles.n)
        art.anchor_ts = candles.ts[static_cast<usize>(t_anchor)];

    // Causality: only feed bars indexed <= t_anchor into all anchor-state construction.
    MarketStateColumns s_full = build_market_state_from_candles(candles);
    MarketStateColumns s = s_full;
    s.n = static_cast<usize>(t_anchor) + 1;   // truncate strictly to past+present
    if (s.n > s_full.n) s.n = s_full.n;
    s.x0.resize(s.n);
    s.x1.resize(s.n);
    s.x2.resize(s.n);
    s.x3.resize(s.n);
    s.x4.resize(s.n);

    // Persist the exact canonical anchor state consumed by extraction and recurrence.
    art.market_state = s;

    art.cps     = phase_1_sectorizer_cpu(s, cfg.changepoint);
    art.section = build_piecewise_section_cpu(s, t_anchor, art.cps, cfg.section, cfg.changepoint);

    try {
        art.extracted_phase_torus = extract_anchor_phase_torus_coeffs(s, t_anchor, cfg);
        art.normalized_jet = make_jet_from_extracted(art.extracted_phase_torus,
                                                     art.extracted_phase_torus.rho_m);
        art.jet_from_phase_torus = !art.normalized_jet.coeff.empty();
    } catch (...) {
        art.extracted_phase_torus = ExtractedCoefficients{};
        art.normalized_jet = JetState{};
        art.jet_from_phase_torus = false;
    }

    art.jet_minus = build_past_jet_state_cpu(art.section,
                                             art.normalized_jet,
                                             s,
                                             t_anchor,
                                             cfg.N_phi,
                                             cfg.N_pi,
                                             cfg.L_max);

    // Recurrence carrier: u_n = F_t[J_t^-](n) on the FUTURE horizon.  ZERO bars
    // > t_anchor are read.  This sequence feeds the Hankel/companion fit and
    // the four-way theorem identity, so it MUST use the smooth-only future
    // section (strict ledger 13.6).  Packet contributions belong to the
    // trajectory / readout / risk layer, not to the carrier identity.
    const int carrier_n = std::max(2, std::min(cfg.recurrence_window, cfg.H + 1));
    Vec<cplx> u(static_cast<usize>(carrier_n), cplx{});
    for (int n = 0; n < carrier_n; ++n)
        u[static_cast<usize>(n)] = evaluate_smooth_future_section(art.jet_minus, static_cast<real>(n));
    art.u_carrier = u;

    art.recurrence = build_recurrence_pipeline(u, cfg.recurrence_window, cfg.tol_rank);

    HorizonGrid hg; hg.h.resize(static_cast<usize>(cfg.H + 1));
    for (int i = 0; i <= cfg.H; ++i) hg.h[static_cast<usize>(i)] = static_cast<real>(i);

    // Newton-Gregory carrier: PAST anchor samples u_past[j] = F_t[J_t^-](-j)
    // for j = 0..K-1, K = recurrence_window.  This is the sequence required by
    // the theorem-path Newton identity  U(h) = Σ β_k(h) ∇^k u_0  evaluated at
    // η := h.  Same SMOOTH anchor object as U_continuous and U_jet (strict
    // ledger 13.6), sampled at past offsets so backward differences ∇^k u_0
    // are well-defined without any shift convention.  Packet contributions
    // are excluded by construction here.
    const int K_newton = std::max(2, std::min(cfg.recurrence_window, t_anchor + 1));
    Vec<cplx> u_past(static_cast<usize>(K_newton), cplx{});
    for (int j = 0; j < K_newton; ++j)
        u_past[static_cast<usize>(j)] = evaluate_smooth_future_section(art.jet_minus,
                                                                       -static_cast<real>(j));

    art.evolution    = compute_evolution_equality(art.jet_minus,
                                                  art.normalized_jet,
                                                  art.recurrence,
                                                  u_past,
                                                  hg);
    art.evolution_ok = assert_evolution_equality(art.evolution, cfg.tol_evolution);
    art.future_section_from_phase_torus = art.evolution.U_jet;

    art.trajectory = run_interstice(art.recurrence.spectrum,
                                    cfg.s_count, cfg.ds, cfg.u_imag,
                                    /*s_index_anchor=*/0, hg);
    art.interstice_pde_residual_grid = build_interstice_pde_residual_grid(art.trajectory);
    art.interstice_recurrence_residual_grid = build_interstice_recurrence_residual_grid(art.trajectory,
                                                                                         art.recurrence.rec);
    art.risk = build_risk_field_from_trajectory(art.trajectory);
    art.section.single_sector_smooth_regime_only = cfg.changepoint.single_sector_smooth_regime_only;
    art.authority = validate_anchor_authoritativeness(art, cfg);
    return art;
}

bool is_anchor_production_authoritative(const AnchorArtifacts& art) {
    return art.authority.production_ready;
}

}  // namespace mt
