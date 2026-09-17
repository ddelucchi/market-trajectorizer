#include "mt/api/cli_commands.hpp"
#include <cstring>

namespace mt {

bool CliArgs::has(std::string_view key) const {
    for (const auto& [k, v] : kv) if (k == key) { (void)v; return true; }
    return false;
}

std::string CliArgs::get(std::string_view key, std::string_view def) const {
    for (const auto& [k, v] : kv) if (k == key) return v;
    return std::string(def);
}

CliArgs parse_cli(int argc, char** argv) {
    CliArgs a;
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (s.size() > 2 && s[0] == '-' && s[1] == '-') {
            auto eq = s.find('=');
            if (eq != std::string::npos)
                a.kv.emplace_back(s.substr(2, eq - 2), s.substr(eq + 1));
            else if (i + 1 < argc && argv[i+1][0] != '-')
                a.kv.emplace_back(s.substr(2), argv[++i]);
            else
                a.kv.emplace_back(s.substr(2), "true");
        } else {
            a.positional.push_back(s);
        }
    }
    return a;
}

}  // namespace mt
