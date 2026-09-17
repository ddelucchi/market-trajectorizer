#include <catch2/catch_test_macros.hpp>
#include "mt/math/newton_gregory.hpp"

TEST_CASE("Newton-Gregory advances polynomial sequence from anchor", "[newton_gregory]") {
    mt::Vec<mt::cplx> u;
    for (int n = 0; n < 6; ++n) u.emplace_back(static_cast<double>(n*n), 0.0);
    mt::HorizonGrid hg; hg.h = {0.0, 1.0, 2.0, 3.0};
    auto v = mt::eval_newton_gregory(u, hg);
    // eval_newton_gregory is anchored at n = u.size()-1 and advances by h.
    const int n_anchor = static_cast<int>(u.size()) - 1;
    for (mt::usize i = 0; i < hg.h.size(); ++i) {
        const int n = n_anchor + static_cast<int>(hg.h[i]);
        REQUIRE(std::abs(v[i] - mt::cplx{static_cast<double>(n * n), 0.0}) < 1e-9);
    }
}
