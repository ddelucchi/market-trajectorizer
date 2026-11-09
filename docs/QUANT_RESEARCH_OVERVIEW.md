# Quantitative Research Reviewer's Guide

**Status:** experimental C++20 research software. This repository is not evidence of a profitable trading strategy, a validated forecast, or a production execution system.

## Research question

Can past-only, deterministic representations of market time series be constructed and audited separately from their predictive utility? The implementation explores OHLCV-derived state representations, finite-difference and jet methods, Hankel/companion recurrence structures, phase-torus coefficient extraction, and spectral diagnostics.

A mathematical identity, numerical regression, causal computation, useful forecast, and profitable strategy are **different claims**. This repository intentionally keeps them separate.

## What a reviewer can inspect

| Area | Code | Evidence and limitations |
| --- | --- | --- |
| Data preparation | `src/data/`, `configs/symbols/` | OHLCV ingestion, canonicalization, and explicit symbol metadata; see `docs/data_contract.md` |
| Feature and state construction | `src/engine/`, `src/math/` | Deterministic numerical transforms and recurrence/spectral components; see `docs/math_contract.md` |
| Causality | `tests/unit/test_causality_and_determinism.cpp` | Anchor-state comparison under mutations made strictly after that anchor; a software regression, not predictive validation |
| Backtest and execution assumptions | `src/backtest/`, `configs/backtests/` | Walk-forward orchestration, lagged fills, commission/slippage configuration, reporting; see `docs/backtest_contract.md` |
| Verification and release gates | `src/app/run_verify_authority.cpp`, `tests/unit/` | A negative production-authority integration gate plus theorem/implementation checks; see `docs/strict_patch_ledger.md` |

## Reproduce the CPU research build

```bash
cmake -S . -B build \
  -DMT_ENABLE_CUDA=OFF \
  -DMT_BUILD_BENCHMARKS=OFF \
  -DMT_BUILD_TESTS=ON \
  -DMT_DETERMINISTIC=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

These commands are the repository's published reproduction path; a fresh local test run must be performed to establish the state of any particular checkout.

## Current limitations

- The public CPU path is a theorem-scaffold baseline, **not** implementation-authoritative for production forecasts.
- The public mathematical authority slice does not certify a fully coupled multi-channel theorem field.
- CUDA kernels remain a research/parity path. Do not treat the presence of CUDA interfaces as validated GPU parity.
- The included synthetic fixture deliberately fails the production-authority gate.
- No independently replicated, realistic out-of-sample market edge or live trading result is established in this repository.

## What a quant research report would still need

1. Fully specified data provenance, timestamp rules, and survivorship/corporate-action treatment where applicable.
2. Leakage tests across feature computation, horizon labeling, resampling, split construction, and execution timing.
3. Pre-registered horizons, markets, baselines, and untouched evaluation periods.
4. Walk-forward results with fees, slippage, uncertainty intervals, turnover, capacity, and multiple-testing controls.
5. Clear reporting of null and negative findings, not just selected best-performing configurations.

## Related public computational work

- [GW150914 Analysis](https://github.com/ddelucchi/gw150914-analysis): time-series conditioning, matched filtering, and reproducibility.
- [TopComp](https://github.com/ddelucchi/topcomp): experimental CUDA/C++ algebra and quantum-computation infrastructure.
- [Principia](https://github.com/ddelucchi/principia): deterministic C++ simulation and replay contracts.

**For recruiters:** this work is best evaluated as a mathematical/software engineering research project with unusually explicit verification boundaries, rather than as evidence of financial-market prediction accuracy.
