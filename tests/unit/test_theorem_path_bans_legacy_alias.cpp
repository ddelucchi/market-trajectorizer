#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string read_text(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    REQUIRE(f.good());
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

std::filesystem::path repo_root() {
    std::filesystem::path here = std::filesystem::path(__FILE__).lexically_normal();
    // tests/unit/<this_file> -> tests -> repo root
    return here.parent_path().parent_path().parent_path();
}

}  // namespace

TEST_CASE("theorem-path translation units ban legacy evaluate_future_section alias",
          "[framework][theorem_path][alias_ban]") {
    const std::filesystem::path root = repo_root();
    const std::vector<std::filesystem::path> theorem_path_files = {
        root / "src/engine/pipeline.cpp",
        root / "src/engine/trajectorizer_engine.cpp",
        root / "src/app/run_verify_authority.cpp",
        root / "tests/unit/test_theorem_authority_harness.cpp",
        root / "tests/unit/test_future_section_identity_on_actual_anchor.cpp",
    };

    for (const auto& p : theorem_path_files) {
        INFO("file=" << p.string());
        REQUIRE(std::filesystem::exists(p));
        const std::string text = read_text(p);
        REQUIRE(text.find("evaluate_future_section(") == std::string::npos);
    }
}
