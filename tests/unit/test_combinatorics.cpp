#include <catch2/catch_test_macros.hpp>
#include "mt/math/combinatorics.hpp"

TEST_CASE("factorial_int basic values", "[combinatorics]") {
    REQUIRE(mt::factorial_int(0) == 1.0);
    REQUIRE(mt::factorial_int(1) == 1.0);
    REQUIRE(mt::factorial_int(5) == 120.0);
}

TEST_CASE("multi_index enumeration count matches binomial", "[combinatorics]") {
    REQUIRE(mt::multi_index_count(3, 2) == 10u);   // C(2+3,3) = 10
    REQUIRE(mt::multi_index_count(2, 3) == 10u);   // C(3+2,2) = 10
}
