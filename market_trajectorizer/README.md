# Top-level README

The market trajectorizer engine + separate backtester compartment.
See [docs/repo_map.md](docs/repo_map.md), [docs/math_contract.md](docs/math_contract.md),
[docs/determinism_contract.md](docs/determinism_contract.md),
[docs/data_contract.md](docs/data_contract.md),
[docs/backtest_contract.md](docs/backtest_contract.md).

Current status: deterministic CPU theorem-scaffold (Path B, "honest
CPU-first"). The implementation realizes the framework identities listed
in [docs/math_contract.md](docs/math_contract.md) §1–§13 on the CPU; the
CUDA kernels are deferred. Runs are therefore **not implementation-
authoritative** until CUDA parity is established
(`gpu_mode = "cpu_authoritative"`,
`implementation_authoritative = false`). A run is theorem-authoritative
on the actual anchor only when the theorem, closure, interstice and
framework-faithfulness clauses (math_contract §12, §13) all pass and
none of the failed-clause names are reported by `mt_verify_authority`.

Stale debug output from earlier (pre-faithful) iterations of the
authority verifier lives under
[`artifacts/_archive/`](artifacts/_archive/README.md) and must not be
cited as evidence of any current claim.

## Build (Linux/Windows, CUDA 12.x)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## CLI
- `mt_ingest --in raw.csv --symbol ES_CONT --out data/canonical/`
- `mt_extract --config configs/engine/default.json --in data/canonical/ES_CONT.bin`
- `mt_trajectorize --engine configs/engine/default.json --backtest configs/backtests/default.json --in data/canonical/ES_CONT.bin`
- `mt_backtest --engine configs/engine/default.json --backtest configs/backtests/default.json`
- `mt_benchmark`
- `mt_verify_authority --manifest data/manifests/symbol_store.json --engine configs/engine/default.json`
- `mt_verify_authority --manifest data/manifests/symbol_store.json --engine configs/engine/default.json --production`

## Theorem Authority Gate
- `ctest --test-dir build -C Release -R theorem_authority --output-on-failure`
- Gates are partitioned into three levels:
	- `fixture_scaffold_gate`
	- `symbol_fixture_exactness_gate`
	- `production_dataset_authority_gate`
- Only production-dataset authority results are trading-authoritative candidates.
- `mt_verify_authority` emits per-anchor authority_result metadata including gate_level, engine_config_hash, dataset_hash, symbol, timeframe, `Q`, tolerances, and exact failed clauses.

This repository is under active implementation with deterministic contracts in `docs/`.
