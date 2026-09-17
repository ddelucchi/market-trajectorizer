#include "mt/core/config.hpp"
#include "mt/api/json_io.hpp"
#include "mt/core/errors.hpp"

#include <string>

namespace mt {
namespace {

[[noreturn]] void fail(const std::string& msg) {
    throw ConfigError("config: " + msg);
}

void require_schema_v1(const std::string& text) {
    const int schema = json_io::read_int(text, "schema_version", -1);
    if (schema != 1) fail("schema_version must be 1");
}

void validate_engine(const EngineConfig& c) {
    if (c.H <= 0) fail("H must be > 0");
    if (c.N_phi <= 0) fail("N_phi must be > 0");
    if (c.N_pi <= 0) fail("N_pi must be > 0");
    if (c.L_max < 0) fail("L_max must be >= 0");
    if (c.recurrence_window <= 1) fail("recurrence_window must be > 1");
    if (c.s_count <= 0) fail("s_count must be > 0");
    if (c.ds <= 0.0) fail("ds must be > 0");
    if (c.torus.rho_m <= 0.0) fail("torus.rho_m must be > 0");
    if (c.torus.M_per_dim < 2) fail("torus.M_per_dim must be >= 2");
    if (c.torus.Q <= 0) fail("torus.Q must be > 0");
    if (c.torus.order_max <= 0) fail("torus.order_max must be > 0");
    if (c.tol_rank <= 0.0) fail("tol_rank must be > 0");
    if (c.tol_evolution <= 0.0) fail("tol_evolution must be > 0");
    if (c.tol_closure <= 0.0) fail("tol_closure must be > 0");
    if (c.tol_interstice <= 0.0) fail("tol_interstice must be > 0");
    if (c.tol_evolution > c.tol_rank) fail("tol_evolution must be <= tol_rank");
    if (c.changepoint.packet_transition_eps <= 0.0) fail("changepoint.packet_transition_eps must be > 0");
    if (c.changepoint.packet_persistence < 1) fail("changepoint.packet_persistence must be >= 1");
    if (c.changepoint.packet_mad_eps <= 0.0) fail("changepoint.packet_mad_eps must be > 0");
    if (c.changepoint.weight_gap < 0.0) fail("changepoint.weight_gap must be >= 0");
    if (c.changepoint.weight_range < 0.0) fail("changepoint.weight_range must be >= 0");
    if (c.changepoint.weight_body < 0.0) fail("changepoint.weight_body must be >= 0");
    if (c.changepoint.weight_volume < 0.0) fail("changepoint.weight_volume must be >= 0");
    if (c.changepoint.weight_wick < 0.0) fail("changepoint.weight_wick must be >= 0");
    if (c.changepoint.weight_compression < 0.0) fail("changepoint.weight_compression must be >= 0");
    if (c.changepoint.weight_drift < 0.0) fail("changepoint.weight_drift must be >= 0");
    if (c.changepoint.weight_exhaustion < 0.0) fail("changepoint.weight_exhaustion must be >= 0");
}

void validate_signal(const SignalConfig& c) {
    if (c.h_eval < 0) fail("signal.h_eval must be >= 0");
    if (c.signal_threshold < 0.0) fail("signal_threshold must be >= 0");
    if (c.stop_atr_mult < 0.0) fail("stop_atr_mult must be >= 0");
    if (c.dd_threshold < 0.0) fail("dd_threshold must be >= 0");
    if (c.lambda_energy < 0.0) fail("signal.lambda_energy must be >= 0");
    if (c.lambda_drawdown < 0.0) fail("signal.lambda_drawdown must be >= 0");
    if (c.lambda_curvature < 0.0) fail("signal.lambda_curvature must be >= 0");
}

void validate_backtest(const BacktestConfig& c) {
    if (c.starting_equity <= 0.0) fail("starting_equity must be > 0");
    if (c.fee_bps < 0.0) fail("fee_bps must be >= 0");
    if (c.slippage_bps < 0.0) fail("slippage_bps must be >= 0");
    if (c.fill_lag_bars < 1) fail("fill_lag_bars must be >= 1");
}

void validate_walk_forward(const WalkForwardConfig& c) {
    if (c.train_min <= 0) fail("walk_forward.train_min must be > 0");
    if (c.test_window <= 0) fail("walk_forward.test_window must be > 0");
    if (c.step <= 0) fail("walk_forward.step must be > 0");
    if (c.horizon_max <= 0) fail("walk_forward.horizon_max must be > 0");
    if (c.anchor_stride <= 0) fail("walk_forward.anchor_stride must be > 0");
    if (c.max_windows < 0) fail("walk_forward.max_windows must be >= 0");
}

}  // namespace

EngineConfig load_engine_config(std::string_view path) {
    EngineConfig c;
    const std::string text = json_io::read_text(path);
    require_schema_v1(text);

    c.deterministic      = json_io::read_bool(text, "deterministic", c.deterministic);
    c.fp32_profile       = json_io::read_bool(text, "fp32_profile", c.fp32_profile);
    c.H                  = json_io::read_int (text, "H", c.H);
    c.N_phi              = json_io::read_int (text, "N_phi", c.N_phi);
    c.N_pi               = json_io::read_int (text, "N_pi", c.N_pi);
    c.L_max              = json_io::read_int (text, "L_max", c.L_max);
    c.recurrence_window  = json_io::read_int (text, "recurrence_window", c.recurrence_window);
    c.tol_rank           = json_io::read_real(text, "tol_rank", c.tol_rank);
    const real legacy_tol_key = json_io::read_real(text, "tol_four_way", c.tol_evolution);
    c.tol_evolution      = json_io::read_real(text, "tol_evolution", legacy_tol_key);
    c.tol_closure        = json_io::read_real(text, "tol_closure", c.tol_evolution);
    c.tol_interstice     = json_io::read_real(text, "tol_interstice", c.tol_interstice);
    c.s_count            = json_io::read_int (text, "s_count", c.s_count);
    c.ds                 = json_io::read_real(text, "ds", c.ds);
    c.u_imag             = json_io::read_real(text, "u_imag", c.u_imag);

    c.changepoint.k_return       = json_io::read_real(text, "changepoint.k_return", c.changepoint.k_return);
    c.changepoint.k_range        = json_io::read_real(text, "changepoint.k_range", c.changepoint.k_range);
    c.changepoint.k_volume       = json_io::read_real(text, "changepoint.k_volume", c.changepoint.k_volume);
    c.changepoint.rolling_window = json_io::read_int (text, "changepoint.rolling_window", c.changepoint.rolling_window);
    c.changepoint.min_separation = json_io::read_int (text, "changepoint.min_separation", c.changepoint.min_separation);
    c.changepoint.gap_sigma      = json_io::read_real(text, "changepoint.gap_sigma", c.changepoint.gap_sigma);
    c.changepoint.range_sigma    = json_io::read_real(text, "changepoint.range_sigma", c.changepoint.range_sigma);
    c.changepoint.body_sigma     = json_io::read_real(text, "changepoint.body_sigma", c.changepoint.body_sigma);
    c.changepoint.volume_sigma   = json_io::read_real(text, "changepoint.volume_sigma", c.changepoint.volume_sigma);
    c.changepoint.wick_sigma     = json_io::read_real(text, "changepoint.wick_sigma", c.changepoint.wick_sigma);
    c.changepoint.compression_sigma = json_io::read_real(text, "changepoint.compression_sigma", c.changepoint.compression_sigma);
    c.changepoint.drift_sigma       = json_io::read_real(text, "changepoint.drift_sigma", c.changepoint.drift_sigma);
    c.changepoint.exhaustion_sigma  = json_io::read_real(text, "changepoint.exhaustion_sigma", c.changepoint.exhaustion_sigma);
    c.changepoint.packet_transition_eps = json_io::read_real(text, "changepoint.packet_transition_eps", c.changepoint.packet_transition_eps);
    c.changepoint.packet_persistence = json_io::read_int(text, "changepoint.packet_persistence", c.changepoint.packet_persistence);
    c.changepoint.packet_mad_eps = json_io::read_real(text, "changepoint.packet_mad_eps", c.changepoint.packet_mad_eps);
    c.changepoint.weight_gap = json_io::read_real(text, "changepoint.weight_gap", c.changepoint.weight_gap);
    c.changepoint.weight_range = json_io::read_real(text, "changepoint.weight_range", c.changepoint.weight_range);
    c.changepoint.weight_body = json_io::read_real(text, "changepoint.weight_body", c.changepoint.weight_body);
    c.changepoint.weight_volume = json_io::read_real(text, "changepoint.weight_volume", c.changepoint.weight_volume);
    c.changepoint.weight_wick = json_io::read_real(text, "changepoint.weight_wick", c.changepoint.weight_wick);
    c.changepoint.weight_compression = json_io::read_real(text, "changepoint.weight_compression", c.changepoint.weight_compression);
    c.changepoint.weight_drift = json_io::read_real(text, "changepoint.weight_drift", c.changepoint.weight_drift);
    c.changepoint.weight_exhaustion = json_io::read_real(text, "changepoint.weight_exhaustion", c.changepoint.weight_exhaustion);
    c.changepoint.single_sector_smooth_regime_only =
        json_io::read_bool(text, "changepoint.single_sector_smooth_regime_only", c.changepoint.single_sector_smooth_regime_only);

    c.section.N_phi = json_io::read_int(text, "section.N_phi", c.section.N_phi);
    c.section.N_pi  = json_io::read_int(text, "section.N_pi", c.section.N_pi);
    c.section.L_max = json_io::read_int(text, "section.L_max", c.section.L_max);

    c.torus.Q         = json_io::read_int (text, "torus.Q", c.torus.Q);
    c.torus.order_max = json_io::read_int (text, "torus.order_max", c.torus.order_max);
    c.torus.M_per_dim = json_io::read_int (text, "torus.M_per_dim", c.torus.M_per_dim);
    c.torus.rho_m     = json_io::read_real(text, "torus.rho_m", c.torus.rho_m);

    validate_engine(c);
    return c;
}

SignalConfig load_signal_config(std::string_view path) {
    SignalConfig c;
    const std::string text = json_io::read_text(path);
    require_schema_v1(text);

    c.h_eval           = json_io::read_int (text, "signal.h_eval", c.h_eval);
    c.signal_threshold = json_io::read_real(text, "signal.signal_threshold", c.signal_threshold);
    c.stop_atr_mult    = json_io::read_real(text, "signal.stop_atr_mult", c.stop_atr_mult);
    c.long_threshold   = json_io::read_real(text, "signal.long_threshold", c.long_threshold);
    c.short_threshold  = json_io::read_real(text, "signal.short_threshold", c.short_threshold);
    c.dd_threshold     = json_io::read_real(text, "signal.dd_threshold", c.dd_threshold);
    c.lambda_energy    = json_io::read_real(text, "signal.lambda_energy", c.lambda_energy);
    c.lambda_drawdown  = json_io::read_real(text, "signal.lambda_drawdown", c.lambda_drawdown);
    c.lambda_curvature = json_io::read_real(text, "signal.lambda_curvature", c.lambda_curvature);
    c.require_authoritative_anchor = json_io::read_bool(text, "signal.require_authoritative_anchor", c.require_authoritative_anchor);
    c.require_framework_energy = json_io::read_bool(text, "signal.require_framework_energy", c.require_framework_energy);
    c.allow_proxy_energy_in_research = json_io::read_bool(text, "signal.allow_proxy_energy_in_research", c.allow_proxy_energy_in_research);
    c.research_heuristic_signal = json_io::read_bool(text, "signal.research_heuristic_signal", c.research_heuristic_signal);

    validate_signal(c);
    return c;
}

BacktestConfig load_backtest_config(std::string_view path) {
    BacktestConfig c;
    const std::string text = json_io::read_text(path);
    require_schema_v1(text);

    c.starting_equity = json_io::read_real(text, "starting_equity", c.starting_equity);
    c.fee_bps         = json_io::read_real(text, "fee_bps", c.fee_bps);
    c.slippage_bps    = json_io::read_real(text, "slippage_bps", c.slippage_bps);
    c.fill_lag_bars   = json_io::read_int (text, "fill_lag_bars", c.fill_lag_bars);
    c.intrabar        = json_io::read_bool(text, "intrabar", c.intrabar);
    c.research_non_authoritative = json_io::read_bool(text, "research_non_authoritative", c.research_non_authoritative);

    validate_backtest(c);
    return c;
}

WalkForwardConfig load_walk_forward_config(std::string_view path) {
    WalkForwardConfig c;
    const std::string text = json_io::read_text(path);
    require_schema_v1(text);

    c.train_min   = json_io::read_int(text, "walk_forward.train_min", c.train_min);
    c.test_window = json_io::read_int(text, "walk_forward.test_window", c.test_window);
    c.step        = json_io::read_int(text, "walk_forward.step", c.step);
    c.horizon_max = json_io::read_int(text, "walk_forward.horizon_max", c.horizon_max);
    c.anchor_stride = json_io::read_int(text, "walk_forward.anchor_stride", c.anchor_stride);
    c.max_windows = json_io::read_int(text, "walk_forward.max_windows", c.max_windows);

    validate_walk_forward(c);
    return c;
}

}  // namespace mt
