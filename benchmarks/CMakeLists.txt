cmake_minimum_required(VERSION 3.27)

include(FetchContent)
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

add_executable(mt_benchmarks
    bench_phase_torus.cpp
    bench_recurrence.cpp
    bench_pipeline.cpp
)
target_link_libraries(mt_benchmarks PRIVATE mt_core benchmark::benchmark)
target_compile_features(mt_benchmarks PRIVATE cxx_std_20)
