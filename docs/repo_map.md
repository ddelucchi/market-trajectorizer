# Repository Map

```
market_trajectorizer/
├── CMakeLists.txt              # mt_core library + 6 executables
├── cmake/                      # toolchain helpers
│   ├── DetectCUDA.cmake
│   ├── CompilerWarnings.cmake
│   ├── Sanitizers.cmake
│   └── ExportCompileCommands.cmake
├── include/mt/
│   ├── core/                   # types, config, errors, logging, buffers
│   ├── data/                   # OHLCV ingest, canonicalize, feature map
│   ├── state/                  # market/jet/interstice/spectral state
│   ├── math/                   # combinatorics, NG, hankel, companion,
│   │                           #   resolvent, residues, spectral, torus,
│   │                           #   interstice, energy, stability
│   ├── engine/                 # ingest/extract/jet/recurrence/interstice/
│   │                           #   trajectorizer/risk + pipeline
│   ├── cuda/                   # stream pool, launch helpers, kernel decls
│   ├── backtest/               # orders, fills, simulator, metrics
│   ├── api/                    # CLI, JSON IO, artifact writer
│   └── app/                    # run_xxx entry points
├── src/                        # mirror of include/ tree
├── apps/                       # mt_ingest, mt_extract, mt_trajectorize,
│                               #   mt_backtest, mt_benchmark,
│                               #   mt_verify_authority
├── tests/                      # Catch2 unit tests
├── benchmarks/                 # google-benchmark micro-benchmarks
├── configs/                    # JSON config schemas
│   ├── engine/                 # default, deterministic_fp64, research
│   ├── backtests/              # backtest configurations
│   ├── symbols/                # per-symbol metadata
│   └── experiments/            # experiment compositions
├── data/manifests/             # dataset manifests
├── docs/                       # contracts (math, determinism, data, backtest)
└── third_party/                # vendored and local dependency notes
```

## Targets
- `mt_core`         — library aggregating all .cpp / .cu units
- `mt_ingest`       — canonicalize raw CSV → mt binary
- `mt_extract`      — phase-torus extraction → ExtractedCoefficients
- `mt_trajectorize` — full pipeline → artifacts + authoritative diagnostics gate
- `mt_backtest`     — Signal stream + simulator → BacktestResult
- `mt_benchmark`    — micro-benchmarks
- `mt_verify_authority` — split-level authority scan (`fixture_scaffold_gate`, `symbol_fixture_exactness_gate`, `production_dataset_authority_gate`) with per-anchor identity metadata

## Gate Commands
- `ctest --test-dir build -C Release -R theorem_authority --output-on-failure`

Repository readiness state: Deterministic CPU baseline with theorem-chain scaffolding.
Current theorem authority path is a canonical-axis audit slice through Q=5 (not yet a fully coupled multichannel theorem field).
Not authoritative unless theorem, closure, interstice, and implementation clauses all pass on the actual anchor.
