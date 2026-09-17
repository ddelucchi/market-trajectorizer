#include "mt/app/run_trajectorize.hpp"
#include "mt/api/artifact_writer.hpp"
#include "mt/backtest/signal_rules.hpp"
#include "mt/core/config.hpp"
#include "mt/data/canonicalizer.hpp"
#include "mt/engine/pipeline.hpp"
#include <algorithm>
#include <iostream>
#include <string>

namespace mt::app {
int run_trajectorize(const CliArgs& a) {
    const auto in = a.get("in");
    if (in.empty()) {
        std::cerr << "run_trajectorize: --in <canonical.bin> required\n";
        return 2;
    }
    const auto engine_cfg = a.get("engine", "configs/engine/default.json");
    const auto bt_cfg     = a.get("backtest", "configs/backtests/default.json");
    const auto out_root   = a.get("out", "artifacts/trajectorize");
    const auto symbol     = a.get("symbol", "UNKNOWN");
    BacktestConfig bc = load_backtest_config(bt_cfg);
    const bool research_non_authoritative = a.has("research_non_authoritative") || bc.research_non_authoritative;

    EngineConfig ec   = load_engine_config(engine_cfg);
    SignalConfig sc   = load_signal_config(bt_cfg);

    CandleColumns c = load_canonical(in);
    if (c.n == 0) {
        std::cerr << "run_trajectorize: empty dataset\n";
        return 3;
    }
    const int t = static_cast<int>(c.n) - 1;
    AnchorArtifacts art = run_anchor(c, t, ec);

    const bool production_ready = is_anchor_production_authoritative(art);
    if (!production_ready && !research_non_authoritative) {
        std::cerr << "run_trajectorize: production authority gate failed at anchor=" << t;
        if (!art.authority.failed_clauses.empty()) {
            std::cerr << " clauses=";
            const usize k = std::min<usize>(art.authority.failed_clauses.size(), 5);
            for (usize i = 0; i < k; ++i) {
                if (i) std::cerr << ",";
                std::cerr << art.authority.failed_clauses[i];
            }
        }
        std::cerr << " regime=" << authority_regime_name(art.authority.regime)
                  << " theorem_authoritative=" << (art.authority.theorem_authoritative ? "true" : "false")
                  << " implementation_authoritative=" << (art.authority.implementation_authoritative ? "true" : "false")
                  << " interstice_verified=" << (art.authority.interstice_verified ? "true" : "false")
                  << "\n"
                  << "run_trajectorize: theorem-authoritative scaffold only; implementation not authoritative; trading disabled in production mode\n";
        return 6;
    }

    TrajectoryFieldContext sctx{
        art.trajectory,
        art.risk,
        art.market_state,
        &art.section,
        t,
        art.anchor_ts,
        art.authority.theorem_authoritative,
        art.authority.theorem_local_sector_verified,
        art.authority.implementation_authoritative,
        art.authority.theorem_valid_h_max,
        art.authority.theorem_valid_h_count,
        art.authority.theorem_valid_h_fraction,
        art.authority.closure_claimed,
        art.authority.closure_verified,
        art.recurrence.rec.r,
        static_cast<int>(art.recurrence.spectrum.modes.size()),
        art.authority.regime,
        art.authority.failed_clauses,
        !production_ready && research_non_authoritative,
        sc.research_heuristic_signal
    };
    Signal sig = build_signal_from_trajectory(sctx, sc);

    std::string effective_out = out_root;
    if (!production_ready)
        effective_out += "/research_non_authoritative";
    if (sc.research_heuristic_signal)
        effective_out += "/research_heuristic";
    if (sig.used_proxy_energy)
        effective_out += "/research_proxy_energy";

    ArtifactPaths paths = build_artifact_paths(effective_out, symbol, std::to_string(art.anchor_ts));
    write_anchor_artifacts(art, sig, paths);

    if (!art.authority.implementation_authoritative) {
        std::cout << "trajectorize: implementation mode=" << art.authority.gpu_mode << "\n"
                  << "trajectorize: theorem-authoritative scaffold only; implementation not authoritative; trading disabled in production mode\n";
    }
    if (!production_ready && research_non_authoritative) {
        std::cout << "trajectorize: research_non_authoritative mode enabled for non-authoritative anchor\n";
    }
    std::cout << "trajectorize: anchor=" << t << " regime=" << authority_regime_name(art.authority.regime)
              << " signal_side=" << sig.side
              << " max_err=" << art.evolution.max_pairwise_abs_err << " out=" << effective_out << "\n";
    return 0;
}
}
