#include <benchmark/benchmark.h>
#include "mt/math/finite_differences.hpp"

static void BM_NablaK(benchmark::State& st) {
    mt::Vec<mt::cplx> u(static_cast<mt::usize>(st.range(0)), mt::cplx{1.0, 0.0});
    for (auto _ : st) {
        auto d = mt::nabla_k(u, 5);
        benchmark::DoNotOptimize(d);
    }
}
BENCHMARK(BM_NablaK)->Arg(64)->Arg(256)->Arg(1024);
