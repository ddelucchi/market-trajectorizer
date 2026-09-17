#include <catch2/catch_test_macros.hpp>
#include "mt/data/canonicalizer.hpp"

TEST_CASE("canonicalizer handles empty input", "[canonicalizer]") {
    mt::CandleColumns empty;
    auto out = mt::canonicalize(empty);
    REQUIRE(out.n == 0);
}
