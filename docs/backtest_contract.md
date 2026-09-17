# Backtest Contract

## Compartmentalization
The backtest pipeline is **separate** from the trajectorizer engine.
The interface between them is `Vec<Signal>` produced by
`build_signal_from_trajectory(SignalContext, SignalConfig)`.
No backtest code reaches into engine internals; no engine code references
positions, fills, or PnL.

## Bar contract
Backtester consumes the same `CandleColumns` the engine consumes.
Default fill model: order timestamped at bar `i` fills at `open[i + fill_lag_bars]`,
slippage applied as bps of arrival price, commission as bps of notional.

## Metrics
Implemented in `src/backtest/metrics.cpp`:
CAGR, Sharpe, Sortino, MaxDrawdown, Calmar, HitRate, ProfitFactor,
Turnover, AvgTrade, Exposure, PnL/bar.
Plus `path_deviation` measuring projected vs realized log-returns.

## Walk-forward
`WalkForwardConfig { train_min, test_window, step, horizon_max }`.
Anchors are placed every `step` bars within the test window;
training stays to the left of the anchor window strictly.
