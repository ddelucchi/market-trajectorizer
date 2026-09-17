#include <catch2/catch_test_macros.hpp>

#include "mt/engine/recurrence_engine.hpp"
#include "mt/math/companion.hpp"
#include "mt/math/resolvent.hpp"
#include "mt/math/spectral.hpp"

#include <algorithm>
#include <cmath>

namespace {

mt::real max_abs_diff(const mt::Vec<mt::cplx>& a, const mt::Vec<mt::cplx>& b) {
    const mt::usize n = std::min(a.size(), b.size());
    mt::real m = 0.0;
    for (mt::usize i = 0; i < n; ++i) m = std::max(m, std::abs(a[i] - b[i]));
    return m;
}

}  // namespace

TEST_CASE("closed recurrence reconstructs matrix, rational, and spectral carriers", "[closure][spectral][resolvent]") {
    // u_n = 0.8^n + 0.6^n is exactly closed of order 2.
    mt::Vec<mt::cplx> u;
    for (int n = 0; n < 20; ++n) {
        const double v = std::pow(0.8, static_cast<double>(n))
                       + std::pow(0.6, static_cast<double>(n));
        u.emplace_back(v, 0.0);
    }

    mt::RecurrencePipelineOut rp = mt::build_recurrence_pipeline(u, /*N_max=*/10, /*tol=*/1e-12);
    REQUIRE(rp.rec.r >= 2);
    REQUIRE_FALSE(rp.rational.Q.empty());
    REQUIRE_FALSE(rp.spectrum.modes.empty());

    mt::HorizonGrid hg;
    hg.h = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};

    const mt::Vec<mt::cplx> u_matrix = mt::eval_companion(rp.comp, hg);
    const mt::Vec<mt::cplx> u_rational = mt::eval_resolvent_grid(rp.rational, hg);
    const mt::Vec<mt::cplx> u_spectral = mt::eval_spectral_modes(rp.spectrum, hg);

    REQUIRE(max_abs_diff(u_matrix, u_rational) < 1e-6);
    REQUIRE(max_abs_diff(u_rational, u_spectral) < 1e-6);
}
