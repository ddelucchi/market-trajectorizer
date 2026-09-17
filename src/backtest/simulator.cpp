#include "mt/backtest/simulator.hpp"
#include "mt/backtest/slippage_model.hpp"
#include "mt/backtest/commission_model.hpp"
#include <algorithm>

namespace mt {

Fill simulate_fill(const Order& o, const Candle& next_bar, real fee_bps, real slippage_bps) {
    Fill f;
    f.ts  = next_bar.ts;
    f.qty = o.side * o.qty;
    f.px  = apply_slippage(next_bar.open, o.side, slippage_bps);
    f.fee = commission(std::abs(f.qty) * f.px, fee_bps);
    return f;
}

BacktestResult run_backtest(const CandleColumns& candles,
                            const Vec<Signal>&   signals,
                            const BacktestConfig& cfg)
{
    BacktestResult R;
    R.curve.ts.reserve(candles.n);
    R.curve.equity.reserve(candles.n);
    R.curve.pnl.reserve(candles.n);
    R.curve.dd.reserve(candles.n);

    real equity = cfg.starting_equity, peak = equity;
    Position pos{};

    usize sig_i = 0;
    for (usize i = 0; i < candles.n; ++i) {
        Candle bar{
            candles.ts[i], candles.open[i], candles.high[i], candles.low[i],
            candles.close[i], candles.volume[i]
        };
        // Mark-to-market on close.
        real mtm_pnl = pos.qty * (bar.close - pos.avg_px);
        real bar_equity = equity + mtm_pnl;

        // Process signals timestamped at this bar; fill at next bar open (lag).
        while (sig_i < signals.size() && signals[sig_i].ts <= bar.ts) {
            const Signal& s = signals[sig_i++];
            if (s.side == 0 || (i + cfg.fill_lag_bars) >= candles.n) continue;
            usize fi = i + cfg.fill_lag_bars;
            Candle nb{
                candles.ts[fi], candles.open[fi], candles.high[fi], candles.low[fi],
                candles.close[fi], candles.volume[fi]
            };
            Order o; o.ts = nb.ts; o.side = s.side; o.qty = s.strength * cfg.starting_equity / nb.open;
            Fill f = simulate_fill(o, nb, cfg.fee_bps, cfg.slippage_bps);
            R.orders.push_back(o);
            R.fills.push_back(f);

            real new_qty = pos.qty + f.qty;
            if (std::abs(new_qty) > 1e-12) {
                pos.avg_px = (pos.qty * pos.avg_px + f.qty * f.px) / new_qty;
            } else {
                pos.avg_px = 0.0;
            }
            pos.qty  = new_qty;
            pos.side = (new_qty > 0) - (new_qty < 0);
            equity  -= f.fee;
        }

        peak = std::max(peak, bar_equity);
        R.curve.ts.push_back(bar.ts);
        R.curve.equity.push_back(bar_equity);
        R.curve.pnl.push_back(mtm_pnl);
        R.curve.dd.push_back(peak > 0 ? (bar_equity - peak) / peak : 0.0);
    }
    return R;
}

}  // namespace mt
