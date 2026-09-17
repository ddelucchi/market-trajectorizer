#pragma once
#include "mt/backtest/order_model.hpp"
#include "mt/data/ohlcv.hpp"

namespace mt {

struct Fill {
    Timestamp ts  = 0;
    real      px  = 0.0;
    real      qty = 0.0;
    real      fee = 0.0;
};

// Fill model: order arrives at bar i, fills at next-bar open by default.
Fill simulate_fill(const Order& o, const Candle& next_bar, real fee_bps, real slippage_bps);

}  // namespace mt
