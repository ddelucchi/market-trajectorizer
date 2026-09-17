#include <catch2/catch_test_macros.hpp>
#include "mt/math/finite_differences.hpp"

TEST_CASE("backward differences of constant series vanish above order zero", "[finite_differences]") {
    mt::Vec<mt::cplx> u(8, mt::cplx{1.0, 0.0});
    auto d = mt::nabla_k(u, 3);
    REQUIRE(std::abs(d[0] - mt::cplx{1.0, 0.0}) < 1e-12);
    for (mt::usize i = 1; i < d.size(); ++i) REQUIRE(std::abs(d[i]) < 1e-12);
}
