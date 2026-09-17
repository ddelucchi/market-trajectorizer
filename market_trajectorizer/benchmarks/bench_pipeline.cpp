#include <benchmark/benchmark.h>
#include "mt/engine/pipeline.hpp"

static void BM_Pipeline(benchmark::State& st) {
    mt::CandleColumns c; c.resize(static_cast<mt::usize>(st.range(0)));
    for (mt::usize i = 0; i < c.n; ++i) {
        c.ts[i] = static_cast<mt::Timestamp>(i);
        c.open[i] = c.high[i] = c.low[i] = c.close[i] = 100.0;
        c.volume[i] = 1.0;
    }
    mt::EngineConfig cfg; cfg.recurrence_window = 16; cfg.H = 8;
    for (auto _ : st) {
        auto art = mt::run_anchor(c, 0, cfg);
        benchmark::DoNotOptimize(art);
    }
}
BENCHMARK(BM_Pipeline)->Arg(64)->Arg(256);
