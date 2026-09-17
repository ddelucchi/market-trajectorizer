#pragma once
#include "mt/core/types.hpp"

namespace mt {

struct PositionSizerConfig {
    real risk_per_trade   = 0.005;     // fraction of equity at risk per trade
    real max_leverage     = 1.0;
    real min_qty          = 1e-6;
};

real compute_qty(real equity, real entry_px, real stop_px, int side,
                 const PositionSizerConfig& cfg);

}  // namespace mt
