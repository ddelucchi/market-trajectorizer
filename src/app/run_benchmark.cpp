#include "mt/app/run_benchmark.hpp"
#include "mt/core/config.hpp"
#include "mt/core/timer.hpp"
#include "mt/data/canonicalizer.hpp"
#include "mt/engine/pipeline.hpp"
#include <algorithm>
#include <iostream>

namespace mt::app {
int run_benchmark(const CliArgs& a) {
    const auto in = a.get("in");
    if (in.empty()) {
        std::cerr << "run_benchmark: --in <canonical.bin> required\n";
        return 2;
    }
    const auto cfg_path = a.get("config", "configs/engine/default.json");
    const int runs = a.has("runs") ? std::max(1, std::stoi(a.get("runs"))) : 5;

    EngineConfig ec = load_engine_config(cfg_path);
    CandleColumns c = load_canonical(in);
    if (c.n == 0) {
        std::cerr << "run_benchmark: empty dataset\n";
        return 3;
    }
    const int t = static_cast<int>(c.n) - 1;

    Vec<double> ms;
    ms.reserve(static_cast<usize>(runs));
    for (int i = 0; i < runs; ++i) {
        Timer tm;
        auto art = run_anchor(c, t, ec);
        (void)art;
        ms.push_back(tm.seconds() * 1000.0);
    }
    double sum = 0.0;
    for (double v : ms) sum += v;
    const double mean = sum / static_cast<double>(ms.size());
    std::cout << "benchmark: runs=" << runs << " mean_ms=" << mean << "\n";
    return 0;
}
}
