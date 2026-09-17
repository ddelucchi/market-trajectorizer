#include "mt/app/run_ingest.hpp"
#include "mt/api/cli_commands.hpp"
#include "mt/engine/ingest_engine.hpp"
#include <iostream>

namespace mt::app {
int run_ingest(const CliArgs& a) {
    auto in   = a.get("in");
    auto sym  = a.get("symbol", "UNKNOWN");
    auto tf   = a.get("timeframe", "1m");
    auto out  = a.get("out", "data/canonical");
    if (in.empty()) { std::cerr << "run_ingest: --in required\n"; return 2; }
    auto rep = mt::run_ingest(in, sym, tf, out);
    std::cout << "ingest: in=" << rep.n_in << " out=" << rep.n_out
              << " dedup=" << rep.n_dedup << " path=" << rep.canonical_path << "\n";
    return 0;
}
}
