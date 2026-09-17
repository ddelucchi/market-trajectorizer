#include <benchmark/benchmark.h>
#include "mt/math/quadrature.hpp"

static void BM_TorusGrid(benchmark::State& st) {
    for (auto _ : st) {
        auto tq = mt::build_torus_quadrature(3, static_cast<int>(st.range(0)));
        benchmark::DoNotOptimize(tq);
    }
}
BENCHMARK(BM_TorusGrid)->Arg(8)->Arg(16)->Arg(32);
