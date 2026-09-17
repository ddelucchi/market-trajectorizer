#pragma once
#include "mt/backtest/simulator.hpp"

namespace mt {

struct Metrics {
    real cagr           = 0.0;
    real sharpe         = 0.0;
    real sortino        = 0.0;
    real max_drawdown   = 0.0;
    real calmar         = 0.0;
    real hit_rate       = 0.0;
    real profit_factor  = 0.0;
    real turnover       = 0.0;
    real avg_trade      = 0.0;
    real exposure       = 0.0;
    real pnl_per_bar    = 0.0;
    real path_deviation = 0.0;       // trajectorizer vs realized
};

Metrics compute_metrics(const BacktestResult& br);
real    compute_path_deviation(const Vec<real>& projected_log_returns,
                               const Vec<real>& realized_log_returns);

}  // namespace mt
