#include <catch2/catch_test_macros.hpp>
#include "mt/math/resolvent.hpp"
#include <cmath>

TEST_CASE("rational model reconstructs a closed recurrence sequence", "[resolvent]") {
    // u_n = 2^n + 3^n satisfies u_{n+2} - 5 u_{n+1} + 6 u_n = 0.
    mt::RecurrenceCoefficients rc;
    rc.r = 2;
    rc.c = {mt::cplx{6.0, 0.0}, mt::cplx{-5.0, 0.0}};
    mt::Vec<mt::cplx> u_init{mt::cplx{2.0, 0.0}, mt::cplx{5.0, 0.0}};

    mt::RationalModel rm = mt::build_rational_from_recurrence(rc, u_init);
    auto u = mt::eval_rational_coeffs(rm, 8);
    REQUIRE(u.size() == 9);
    for (int n = 0; n <= 8; ++n) {
        const double expected = std::pow(2.0, static_cast<double>(n))
                              + std::pow(3.0, static_cast<double>(n));
        REQUIRE(std::abs(u[static_cast<mt::usize>(n)] - mt::cplx{expected, 0.0}) < 1e-9);
    }
}
