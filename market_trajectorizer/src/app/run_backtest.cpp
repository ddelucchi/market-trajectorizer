#include "mt/app/run_backtest.hpp"
#include "mt/backtest/report.hpp"
#include "mt/core/config.hpp"
#include "mt/data/dataset.hpp"
#include "mt/data/symbol_store.hpp"
#include <iostream>

namespace mt::app {
int run_backtest(const CliArgs& a) {
    const auto manifest = a.get("manifest");
    const auto symbol   = a.get("symbol");
    if (manifest.empty() || symbol.empty()) {
        std::cerr << "run_backtest: --manifest <symbols.json> --symbol <SYMBOL> required\n";
        return 2;
    }

    const auto engine_cfg = a.get("engine", "configs/engine/default.json");
    const auto bt_cfg     = a.get("backtest", "configs/backtests/default.json");
    const auto out_json   = a.get("out", "artifacts/backtest/report.json");

    SymbolStore store;
    store.load_manifest(manifest);
    Dataset ds = load_dataset(store, symbol);

    EngineConfig ec = load_engine_config(engine_cfg);
    SignalConfig sc = load_signal_config(bt_cfg);
    BacktestConfig bc = load_backtest_config(bt_cfg);
    WalkForwardConfig wf = load_walk_forward_config(bt_cfg);

    WalkForwardResult wfr = run_walk_forward(ds, wf, ec, sc, bc);
    if (!bc.research_non_authoritative
        && (wfr.non_authoritative_anchors > 0 || wfr.rejected_anchors > 0 || wfr.authoritative_anchors <= 0)) {
        std::cerr << "run_backtest: production authority gate failed"
                  << " authoritative_anchors=" << wfr.authoritative_anchors
                  << " non_authoritative_anchors=" << wfr.non_authoritative_anchors
                  << " rejected_anchors=" << wfr.rejected_anchors << "\n"
                  << "run_backtest: theorem-authoritative scaffold only; implementation not authoritative; trading disabled in production mode\n";
        return 7;
    }
    write_walk_forward_report(wfr, out_json);
    if (bc.research_non_authoritative) {
        std::cout << "backtest: research_non_authoritative mode enabled; production acceptance disabled\n";
    }
    std::cout << "backtest: windows=" << wfr.windows.size() << " sharpe=" << wfr.aggregate.sharpe
              << " authoritative_anchors=" << wfr.authoritative_anchors
              << " non_authoritative_anchors=" << wfr.non_authoritative_anchors
              << " rejected_anchors=" << wfr.rejected_anchors
              << " research_proxy_energy_signals=" << wfr.research_proxy_energy_signals
              << " out=" << out_json << "\n";
    return 0;
}
}
