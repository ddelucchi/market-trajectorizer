cmake_minimum_required(VERSION 3.27)
project(market_trajectorizer LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(CompilerWarnings)
include(Sanitizers)
include(ExportCompileCommands)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(MT_BUILD_TESTS      "Build unit/integration tests"      ON)
option(MT_BUILD_BENCHMARKS "Build benchmarks"                  ON)
option(MT_DETERMINISTIC    "Enforce deterministic numerics path" ON)
option(MT_ENABLE_CUDA      "Enable CUDA research/parity path" OFF)

set(MT_CUDA_ENABLED OFF)
if(MT_ENABLE_CUDA)
  include(DetectCUDA)
  if(MT_CUDA_AVAILABLE)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_ARCHITECTURES 75)
    set(MT_CUDA_ENABLED ON)
    message(STATUS "CUDA research/parity path enabled")
  else()
    message(WARNING "MT_ENABLE_CUDA=ON but CUDA toolchain is unavailable; building deterministic CPU theorem-scaffold mode (not production-authoritative)")
  endif()
else()
  message(STATUS "MT_ENABLE_CUDA=OFF; building deterministic CPU theorem-scaffold mode (not production-authoritative)")
endif()

set(MT_CORE_SOURCES
  # core
  src/core/config.cpp
  # data
  src/data/csv_reader.cpp
  src/data/canonicalizer.cpp
  src/data/symbol_store.cpp
  src/data/trading_calendar.cpp
  src/data/resampler.cpp
  src/data/feature_map.cpp
  src/data/dataset.cpp
  # math
  src/math/combinatorics.cpp
  src/math/finite_differences.cpp
  src/math/taylor.cpp
  src/math/newton_gregory.cpp
  src/math/hankel.cpp
  src/math/companion.cpp
  src/math/resolvent.cpp
  src/math/residues.cpp
  src/math/spectral.cpp
  src/math/polynomial.cpp
  src/math/complex_roots.cpp
  src/math/semigroup.cpp
  src/math/quadrature.cpp
  src/math/phase_torus.cpp
  src/math/interstice.cpp
  src/math/forecast_energy.cpp
  src/math/stability.cpp
  # state
  src/state/market_state.cpp
  src/state/interstice_state.cpp
  src/state/jet_state.cpp
  src/state/history_filtration.cpp
  src/state/piecewise_section.cpp
  src/state/extracted_coeffs.cpp
  src/state/recurrence_state.cpp
  src/state/spectral_state.cpp
  src/state/trajectorizer_state.cpp
  src/state/risk_state.cpp
  # engine
  src/engine/ingest_engine.cpp
  src/engine/extraction_engine.cpp
  src/engine/jet_engine.cpp
  src/engine/recurrence_engine.cpp
  src/engine/interstice_engine.cpp
  src/engine/trajectorizer_engine.cpp
  src/engine/risk_engine.cpp
  src/engine/execution_graph.cpp
  src/engine/pipeline.cpp
  # backtest
  src/backtest/order_model.cpp
  src/backtest/fill_model.cpp
  src/backtest/slippage_model.cpp
  src/backtest/commission_model.cpp
  src/backtest/position_sizer.cpp
  src/backtest/signal_rules.cpp
  src/backtest/simulator.cpp
  src/backtest/metrics.cpp
  src/backtest/walk_forward.cpp
  src/backtest/report.cpp
  # api
  src/api/cli_commands.cpp
  src/api/json_io.cpp
  src/api/result_schema.cpp
  src/api/artifact_writer.cpp
  # app
  src/app/run_ingest.cpp
  src/app/run_extract.cpp
  src/app/run_trajectorize.cpp
  src/app/run_backtest.cpp
  src/app/run_benchmark.cpp
  src/app/run_verify_authority.cpp
)

if(MT_CUDA_ENABLED)
  list(APPEND MT_CORE_SOURCES
    src/cuda/stream_pool.cpp
    src/cuda/kernels_feature.cu
    src/cuda/kernels_phase_torus.cu
    src/cuda/kernels_newton.cu
    src/cuda/kernels_hankel.cu
    src/cuda/kernels_companion.cu
    src/cuda/kernels_resolvent.cu
    src/cuda/kernels_interstice.cu
    src/cuda/kernels_risk.cu
  )
endif()

# -----------------------------------------------------------------------------
# Core library
# -----------------------------------------------------------------------------
add_library(mt_core ${MT_CORE_SOURCES})

target_include_directories(mt_core PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)
if(MT_CUDA_ENABLED)
  target_compile_features(mt_core PUBLIC cxx_std_20 cuda_std_20)
  target_compile_definitions(mt_core PUBLIC MT_ENABLE_CUDA=1)
else()
  target_compile_features(mt_core PUBLIC cxx_std_20)
  target_compile_definitions(mt_core PUBLIC MT_ENABLE_CUDA=0)
endif()

target_compile_options(mt_core PRIVATE
  $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<CXX_COMPILER_ID:MSVC>>,$<CONFIG:Release>>:-O3>
  $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2>
)

if(MT_CUDA_ENABLED)
  target_compile_options(mt_core PRIVATE
    $<$<AND:$<COMPILE_LANGUAGE:CUDA>,$<CONFIG:Release>>:-O3>
    $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr --generate-line-info>
  )
endif()

if(MT_DETERMINISTIC AND MT_CUDA_ENABLED)
  target_compile_options(mt_core PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--fmad=false>
    $<$<COMPILE_LANGUAGE:CUDA>:--prec-div=true>
    $<$<COMPILE_LANGUAGE:CUDA>:--prec-sqrt=true>
  )
endif()

if(MT_DETERMINISTIC)
  target_compile_definitions(mt_core PUBLIC MT_DETERMINISTIC=1)
endif()

if(MT_CUDA_ENABLED)
  set_target_properties(mt_core PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
  )
endif()

# -----------------------------------------------------------------------------
# Executables
# -----------------------------------------------------------------------------
foreach(_app mt_ingest mt_extract mt_trajectorize mt_backtest mt_benchmark mt_verify_authority)
  add_executable(${_app} apps/${_app}.cpp)
  target_link_libraries(${_app} PRIVATE mt_core)
endforeach()

# -----------------------------------------------------------------------------
# Tests / benchmarks
# -----------------------------------------------------------------------------
if(MT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(MT_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
