#pragma once
#include <string>
#include <vector>

namespace mt {

struct CliArgs {
    std::vector<std::string> positional;
    // Deterministic CLI flag map parser for --key=value and --key value forms.
    std::vector<std::pair<std::string, std::string>> kv;
    bool has(std::string_view key) const;
    std::string get(std::string_view key, std::string_view def = "") const;
};

CliArgs parse_cli(int argc, char** argv);

}  // namespace mt
