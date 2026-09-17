#include "mt/backtest/position_sizer.hpp"

#include <algorithm>
#include <cmath>

namespace mt {

real compute_qty(real equity, real entry_px, real stop_px, int side,
                 const PositionSizerConfig& cfg) {
    if (!std::isfinite(equity) || !std::isfinite(entry_px) || !std::isfinite(stop_px) ||
        !std::isfinite(cfg.risk_per_trade) || !std::isfinite(cfg.max_leverage) ||
        !std::isfinite(cfg.min_qty) || equity <= 0.0 || entry_px <= 0.0 ||
        side == 0 || cfg.risk_per_trade <= 0.0 || cfg.max_leverage <= 0.0) {
        return 0.0;
    }

    const real stop_distance = std::abs(entry_px - stop_px);
    if (stop_distance <= 0.0) return 0.0;

    const real risk_budget = equity * cfg.risk_per_trade;
    const real risk_limited_qty = risk_budget / stop_distance;
    const real leverage_limited_qty = (equity * cfg.max_leverage) / entry_px;
    const real qty = std::min(risk_limited_qty, leverage_limited_qty);

    return qty >= std::max<real>(0.0, cfg.min_qty) ? qty : 0.0;
}

}  // namespace mt
