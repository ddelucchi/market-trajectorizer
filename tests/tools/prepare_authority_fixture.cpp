#include "mt/api/json_io.hpp"
#include "mt/data/canonicalizer.hpp"
#include "mt/data/csv_reader.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string get_arg(int argc, char** argv, std::string_view key, std::string_view def = "") {
    const std::string prefix = "--" + std::string(key) + "=";
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a.rfind(prefix, 0) == 0) return a.substr(prefix.size());
        if (a == "--" + std::string(key) && i + 1 < argc) return argv[++i];
    }
    return std::string(def);
}

std::string escape_json(std::string_view in) {
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string in_csv = get_arg(argc, argv, "in");
    const std::string out_dir = get_arg(argc, argv, "out_dir");
    const std::string manifest = get_arg(argc, argv, "manifest");
    const std::string symbol = get_arg(argc, argv, "symbol", "AUTH_FIX");
    const std::string timeframe = get_arg(argc, argv, "timeframe", "1d");

    if (in_csv.empty() || out_dir.empty() || manifest.empty()) {
        std::cerr << "prepare_authority_fixture: --in, --out_dir, and --manifest are required\n";
        return 2;
    }

    try {
        namespace fs = std::filesystem;
        fs::create_directories(out_dir);

        mt::CandleColumns raw = mt::read_csv(in_csv);
        mt::CandleColumns canonical = mt::canonicalize(raw);

        fs::path canonical_path = fs::path(out_dir) / (symbol + ".bin");
        mt::save_canonical(canonical, canonical_path.string());

        std::ostringstream s;
        s << "{\n"
          << "  \"schema\": \"symbol_store_v1\",\n"
          << "  \"symbols\": [\n"
          << "    {\"symbol\":\"" << escape_json(symbol) << "\"," 
          << "\"timeframe\":\"" << escape_json(timeframe) << "\"," 
          << "\"canonical_path\":\"" << escape_json(canonical_path.generic_string()) << "\"," 
          << "\"t_first\":" << canonical.ts.front() << ","
          << "\"t_last\":" << canonical.ts.back() << ","
          << "\"n_bars\":" << canonical.n << "}\n"
          << "  ]\n"
          << "}\n";

        mt::json_io::write_text(manifest, s.str());
        std::cout << "prepare_authority_fixture: manifest=" << manifest
                  << " bars=" << canonical.n << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "prepare_authority_fixture: " << e.what() << "\n";
        return 1;
    }
}
