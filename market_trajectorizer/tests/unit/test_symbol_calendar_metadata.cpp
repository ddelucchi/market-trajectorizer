#include <catch2/catch_test_macros.hpp>

#include "mt/api/json_io.hpp"

#include <filesystem>

TEST_CASE("ES_CONT metadata uses futures calendar semantics", "[metadata][calendar]") {
    const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    const std::filesystem::path symbol_path = root / "configs" / "symbols" / "ES_CONT.json";
    const std::string text = mt::json_io::read_text(symbol_path.string());
    REQUIRE(text.find("\"calendar\": \"us_futures_rth\"") != std::string::npos);
    REQUIRE(text.find("\"calendar\": \"us_equities_rth\"") == std::string::npos);
}
