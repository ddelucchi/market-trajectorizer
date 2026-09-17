#pragma once
#include "mt/backtest/fill_model.hpp"
#include "mt/backtest/signal_rules.hpp"
#include "mt/core/config.hpp"
#include "mt/data/ohlcv.hpp"

namespace mt {

struct Position {
    int  side    = 0;
    real qty     = 0.0;
    real avg_px  = 0.0;
};

struct EquityCurve {
    Vec<Timestamp> ts;
    Vec<real>      equity;
    Vec<real>      pnl;
    Vec<real>      dd;
};

struct BacktestResult {
    EquityCurve   curve;
    Vec<Fill>     fills;
    Vec<Order>    orders;
};

BacktestResult run_backtest(const CandleColumns& candles,
                            const Vec<Signal>&   signals,
                            const BacktestConfig& cfg);

}  // namespace mt
