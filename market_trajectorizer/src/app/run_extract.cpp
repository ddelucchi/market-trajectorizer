#include "mt/app/run_extract.hpp"
#include "mt/api/json_io.hpp"
#include "mt/core/config.hpp"
#include "mt/data/canonicalizer.hpp"
#include "mt/engine/pipeline.hpp"
#include <iostream>
#include <sstream>

namespace mt::app {
int run_extract(const CliArgs& a) {
    const auto in = a.get("in");
    if (in.empty()) {
        std::cerr << "run_extract: --in <canonical.bin> required\n";
        return 2;
    }

    const auto cfg_path = a.get("config", "configs/engine/default.json");
    const auto out      = a.get("out", "artifacts/extract_summary.json");
    const int anchor    = a.has("anchor") ? std::stoi(a.get("anchor")) : -1;

    EngineConfig cfg = load_engine_config(cfg_path);
    CandleColumns c = load_canonical(in);
    if (c.n == 0) {
        std::cerr << "run_extract: empty canonical dataset\n";
        return 3;
    }
    const int t = (anchor >= 0) ? anchor : static_cast<int>(c.n) - 1;
    if (t < 0 || static_cast<usize>(t) >= c.n) {
        std::cerr << "run_extract: anchor out of range\n";
        return 4;
    }

    AnchorArtifacts art = run_anchor(c, t, cfg);
    std::ostringstream s;
    s << "{\n"
      << "  \"schema\": \"extract_v1\",\n"
      << "  \"anchor\": " << t << ",\n"
      << "  \"anchor_ts\": " << art.anchor_ts << ",\n"
      << "  \"changepoints\": " << art.cps.sigma_idx.size() << ",\n"
      << "  \"phi_terms\": " << art.jet_minus.A_phi.size() << ",\n"
    << "  \"pi_terms\": " << art.jet_minus.sigma.size() << ",\n"
      << "  \"theorem_authoritative\": " << (art.authority.theorem_authoritative ? "true" : "false") << ",\n"
      << "  \"implementation_authoritative\": " << (art.authority.implementation_authoritative ? "true" : "false") << ",\n"
      << "  \"production_ready\": " << (art.authority.production_ready ? "true" : "false") << "\n"
      << "}\n";
    json_io::write_text(out, s.str());
    std::cout << "extract: anchor=" << t << " changepoints=" << art.cps.sigma_idx.size() << " out=" << out << "\n";
    return 0;
}
}
