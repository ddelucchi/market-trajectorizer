# Market Trajectorizer

Market Trajectorizer is a C++20 research engine for deterministic, causality-aware transformation of historical market time series into mathematical state representations, recurrence models, diagnostic artifacts, and backtest inputs.

It is deliberately framed as research software, not as a claim of profitable trading performance.

## Current authority boundary

The default public build is the deterministic CPU theorem-scaffold path.

The code distinguishes several ideas that are intentionally not conflated:

- mathematical identities implemented by the framework
- numerical agreement within explicit tolerances
- causality of an anchor with respect to post-anchor data
- finite-dimensional closure claims
- implementation authority
- experimental signal generation
- backtest behavior

CUDA kernels exist as a research/parity path, but CPU runs are explicitly marked non-implementation-authoritative until the CUDA parity and authority conditions are satisfied.

## What is implemented

- canonical OHLCV ingestion and resampling
- deterministic market-state construction
- past-jet and piecewise-section representations
- phase-torus coefficient extraction
- jet normalization
- Newton-Gregory evolution
- Hankel/companion recurrence construction
- rational and spectral reconstruction paths
- interstice transport and residual diagnostics
- explicit authority-state metadata
- backtest execution with lag, fees, slippage, and reporting
- artifact emission for intermediate mathematical carriers
- Catch2 regression tests across causality, determinism, recurrence, closure, numerical identities, and authority gates

## Causality is tested, not merely asserted

The regression suite includes an anchor-causality test that duplicates one historical series, mutates bars strictly after the chosen anchor, and verifies that anchor-derived carriers remain unchanged within the declared tolerance.

The deterministic test path also repeats the same anchor calculation and compares its carrier, recurrence, authority state, and residual metadata.

## Mathematical contract

The implementation contract is documented in:

- [math_contract.md](docs/math_contract.md)
- [determinism_contract.md](docs/determinism_contract.md)
- [data_contract.md](docs/data_contract.md)
- [backtest_contract.md](docs/backtest_contract.md)
- [repo_map.md](docs/repo_map.md)

These files are part of the software boundary. They state the identities and acceptance conditions the code is intended to realize.

## Build

From the repository root:

```bash
cmake -S . -B build \
  -DMT_ENABLE_CUDA=OFF \
  -DMT_BUILD_BENCHMARKS=OFF \
  -DMT_BUILD_TESTS=ON \
  -DMT_DETERMINISTIC=ON \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --parallel
```

Run the reproducible unit/core suite:

```bash
ctest --test-dir build --output-on-failure -LE theorem_authority
```

## Authority-gate limitation in this snapshot

The full `theorem_authority` CTest fixture path references a local fixture dataset that is not included in this repository snapshot. CI therefore excludes tests carrying that label rather than pretending the full authority gate is reproducible here.

That missing fixture is a release limitation, not a hidden success condition.

## Command-line programs

The CMake project builds tools for ingestion, extraction, trajectorization, backtesting, benchmarking, and authority verification. Executable targets and their source entrypoints are defined directly in the root `CMakeLists.txt`; use each program's built-in help where available and the contracts under `docs/` for interpretation.

## No performance claim

Nothing in this repository should be interpreted as investment advice, a promise of predictive edge, or evidence of live profitability. A mathematically interesting representation can still have zero economic forecasting value. Performance claims would require separate out-of-sample evidence, realistic execution assumptions, and independent replication.

## License

Source is publicly viewable for portfolio and technical evaluation. See [LICENSE](LICENSE).
