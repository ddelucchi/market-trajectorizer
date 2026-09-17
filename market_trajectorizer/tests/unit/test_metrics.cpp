#include <catch2/catch_test_macros.hpp>
#include "mt/backtest/metrics.hpp"

TEST_CASE("metrics on empty backtest are zero", "[metrics]") {
    mt::BacktestResult R;
    auto m = mt::compute_metrics(R);
    REQUIRE(m.cagr == 0.0);
    REQUIRE(m.sharpe == 0.0);
}
