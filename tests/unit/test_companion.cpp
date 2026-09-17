#include <catch2/catch_test_macros.hpp>
#include "mt/math/companion.hpp"
#include <cmath>

TEST_CASE("companion propagates exact recurrence", "[companion]") {
    // u_{n+1} - 2 u_n = 0  ->  u_{n+1} + c0 u_n = 0 with c0 = -2.
    mt::RecurrenceCoefficients rc;
    rc.r = 1;
    rc.c = {mt::cplx{-2.0, 0.0}};
    mt::Vec<mt::cplx> u_init{mt::cplx{3.0, 0.0}};

    auto cs = mt::build_companion_state(rc, u_init);
    mt::HorizonGrid hg;
    hg.h = {0.0, 1.0, 2.0, 3.0, 4.0};

    auto v = mt::eval_companion(cs, hg);
    REQUIRE(v.size() == hg.h.size());
    for (mt::usize i = 0; i < hg.h.size(); ++i) {
        const double expected = 3.0 * std::pow(2.0, static_cast<double>(i));
        REQUIRE(std::abs(v[i] - mt::cplx{expected, 0.0}) < 1e-12);
    }
}
