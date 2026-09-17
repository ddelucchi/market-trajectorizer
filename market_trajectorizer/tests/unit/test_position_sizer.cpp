#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "mt/backtest/position_sizer.hpp"

TEST_CASE("position sizing respects risk and leverage caps", "[backtest][position_sizer]") {
    mt::PositionSizerConfig cfg;
    cfg.risk_per_trade = 0.01;
    cfg.max_leverage = 1.0;
    cfg.min_qty = 1e-6;

    REQUIRE(mt::compute_qty(100000.0, 100.0, 98.0, 1, cfg)
            == Catch::Approx(500.0));

    cfg.risk_per_trade = 0.50;
    REQUIRE(mt::compute_qty(100000.0, 100.0, 98.0, -1, cfg)
            == Catch::Approx(1000.0));
}

TEST_CASE("position sizing rejects invalid or degenerate inputs", "[backtest][position_sizer]") {
    mt::PositionSizerConfig cfg;
    REQUIRE(mt::compute_qty(0.0, 100.0, 98.0, 1, cfg) == 0.0);
    REQUIRE(mt::compute_qty(100000.0, 0.0, 98.0, 1, cfg) == 0.0);
    REQUIRE(mt::compute_qty(100000.0, 100.0, 100.0, 1, cfg) == 0.0);
    REQUIRE(mt::compute_qty(100000.0, 100.0, 98.0, 0, cfg) == 0.0);
}
