#pragma once
#include "mt/backtest/walk_forward.hpp"
#include <string_view>

namespace mt {

void write_backtest_report(const BacktestResult&    br,
                           const Metrics&           m,
                           std::string_view         out_json);

void write_walk_forward_report(const WalkForwardResult& wfr,
                               std::string_view         out_json);

}  // namespace mt
