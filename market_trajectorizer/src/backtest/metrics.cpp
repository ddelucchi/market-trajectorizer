#include "mt/backtest/metrics.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace mt {

Metrics compute_metrics(const BacktestResult& br) {
    Metrics m;
    const auto& eq = br.curve.equity;
    const auto& pnl = br.curve.pnl;
    const auto& dd  = br.curve.dd;
    if (eq.size() < 2) return m;

    real e0 = eq.front(), eN = eq.back();
    if (e0 > 0) m.cagr = std::pow(eN / e0, 1.0 / static_cast<real>(eq.size())) - 1.0;

    Vec<real> ret(eq.size() - 1);
    for (usize i = 1; i < eq.size(); ++i)
        ret[i-1] = (eq[i-1] != 0.0) ? (eq[i] - eq[i-1]) / eq[i-1] : 0.0;

    real mean = std::accumulate(ret.begin(), ret.end(), 0.0) / static_cast<real>(ret.size());
    real var = 0.0, dvar = 0.0;
    for (real r : ret) { var += (r - mean) * (r - mean); if (r < 0) dvar += r * r; }
    var  /= static_cast<real>(ret.size());
    dvar /= static_cast<real>(ret.size());
    real sd  = std::sqrt(var), dsd = std::sqrt(dvar);
    m.sharpe  = (sd  > 0) ? mean / sd  : 0.0;
    m.sortino = (dsd > 0) ? mean / dsd : 0.0;

    m.max_drawdown = 0.0;
    for (real d : dd) m.max_drawdown = std::min(m.max_drawdown, d);
    m.calmar = (m.max_drawdown < 0) ? m.cagr / -m.max_drawdown : 0.0;

    int wins = 0, n_trades = 0;
    real gp = 0.0, gl = 0.0;
    for (real p : pnl) {
        if (p == 0.0) continue;
        ++n_trades;
        if (p > 0) { ++wins; gp += p; } else gl -= p;
    }
    m.hit_rate      = n_trades ? static_cast<real>(wins) / n_trades : 0.0;
    m.profit_factor = (gl > 0) ? gp / gl : 0.0;
    m.avg_trade     = n_trades ? (gp - gl) / n_trades : 0.0;
    m.pnl_per_bar   = pnl.empty() ? 0.0 : std::accumulate(pnl.begin(), pnl.end(), 0.0) / pnl.size();

    real notional = 0.0;
    for (const auto& f : br.fills) notional += std::abs(f.qty * f.px);
    m.turnover = (e0 > 0) ? notional / e0 : 0.0;

    int exposed = 0;
    for (real p : pnl) if (p != 0.0) ++exposed;
    m.exposure = pnl.empty() ? 0.0 : static_cast<real>(exposed) / pnl.size();
    return m;
}

real compute_path_deviation(const Vec<real>& proj, const Vec<real>& real_) {
    const usize n = std::min(proj.size(), real_.size());
    real s = 0.0;
    for (usize i = 0; i < n; ++i) { real d = proj[i] - real_[i]; s += d * d; }
    return n ? std::sqrt(s / n) : 0.0;
}

}  // namespace mt
