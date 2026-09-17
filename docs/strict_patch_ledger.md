# Strict Patch Ledger

> **Live-artifact override.** This ledger describes the *intended* state of
> the code.  Runtime authority on the actual anchor artifacts is the final
> arbiter.  Any item below that is not corroborated by a fresh
> `mt_verify_authority` run on a real anchor (with
> `production_ready=true` and `failed_clauses=[]`) is a `PARTIAL` claim,
> not a `DONE` claim, regardless of what the per-item line below says.
> Stale archived diagnostics under `artifacts/_archive/debug_*` are *not*
> evidence of current correctness; only fresh outputs under
> `artifacts/<symbol>/<date>/...` count.

This ledger maps the audited drift items to concrete repository actions.
Status codes:
- `DONE`: implemented and verified in code
- `PARTIAL`: implemented as deterministic CPU theorem-scaffold baseline (not production-authoritative)
- `OPEN`: mathematically specified, not yet fully realized
- `STALE_CLAIM_REMOVED`: prior status text removed because code reality advanced

## 1) Causality and anchor recurrence carrier
- `PARTIAL`: `u_n` in anchor pipeline is formed from `F_t[J_t^-](n)` using only bars `<= t_anchor`, where `J_t^-` is built from extracted/normalized coefficients (no scalar-score surrogate coefficients).  The previous scalar `x_3`-only fallback in `build_past_jet_state_cpu` is removed; production now fails closed with `theorem.past_jet_invalid` when the normalized jet is absent.  Status remains `PARTIAL` until the actual-anchor harness passes with `production_ready=true`.
- Files:
  - `src/engine/extraction_engine.cpp`
  - `src/engine/pipeline.cpp`

Acceptance checks:
- No reads of `candles[t_anchor + n]` when forming anchor recurrence state.
- `run_anchor` truncates market state to `n = t_anchor + 1`.
- `FutureSection::valid` is true only when built from the canonical Q=5 normalized jet.

## 2) Mathematical contract replacement
- `DONE`: full framework-level contract document with theorem identities and transport law.
- Files:
  - `docs/math_contract.md`

## 3) Theorem object replacement
- `PARTIAL`: theorem object is `EvolutionEquality` with theorem carriers plus closure corollaries.  Newton path now uses past-anchor samples `u_past[j]=F_t[J_t^-](-j)` evaluated at `η=h` (no `η=h-(K-1)` shift).
- `PARTIAL`: theorem authority is currently an audited canonical-axis slice (`alpha=(n,0,0,0,0)`, channel `x0`) through Q=5, not yet a fully coupled multichannel theorem field.
- `PARTIAL`: theorem-equality certification is now local-sector based (`theorem.local_valid_sector`) rather than globally assumed on fixed `0..H`.
- `PARTIAL` remains until live anchors pass local-sector theorem clauses with `failed_clauses=[]` on the production dataset authority gate.
- Files:
  - `include/mt/engine/trajectorizer_engine.hpp`
  - `src/engine/trajectorizer_engine.cpp`
  - `include/mt/engine/pipeline.hpp`
  - `src/engine/pipeline.cpp`

## 4) Newton-Gregory basis convention lock
- `PARTIAL`: rising basis used in docs and code (`beta_k(h) = 1/k! * prod(h+j)`).  Theorem-authoritative entrypoint is `eval_newton_at_h(u_past, ...)` in `trajectorizer_engine.cpp`; the generic `eval_newton_gregory(...)` helper in `math/newton_gregory.cpp` MUST NOT be used as the theorem carrier on ad hoc vectors.
- Files:
  - `docs/math_contract.md`
  - `include/mt/math/newton_gregory.hpp`
  - `src/math/newton_gregory.cpp`
  - `src/engine/trajectorizer_engine.cpp`

## 5) Jet normalization correction
- `DONE`: `J_alpha = rho_m^{-|alpha|} A_alpha` with no extra factorial multiplier.
- Files:
  - `src/engine/jet_engine.cpp`

## 6) D/I/projection operators
- `DONE`: coefficientwise `D_i`, `I_i`, and `Pi_perp` implemented on dense multi-index lattice.
- Files:
  - `src/math/taylor.cpp`

## 7) GPU public API honesty
- `PARTIAL`: public CUDA interfaces now have concrete signatures and deterministic no-op fallback implementations.
- Files:
  - `include/mt/cuda/kernels_phase_torus.cuh`
  - `include/mt/cuda/kernels_newton.cuh`
  - `include/mt/cuda/kernels_hankel.cuh`
  - `include/mt/cuda/kernels_companion.cuh`
  - `include/mt/cuda/kernels_resolvent.cuh`
  - `include/mt/cuda/kernels_interstice.cuh`
  - `include/mt/cuda/kernels_risk.cuh`
  - `src/cuda/kernels_phase_torus.cu`
  - `src/cuda/kernels_newton.cu`
  - `src/cuda/kernels_hankel.cu`
  - `src/cuda/kernels_companion.cu`
  - `src/cuda/kernels_resolvent.cu`
  - `src/cuda/kernels_interstice.cu`
  - `src/cuda/kernels_risk.cu`

## 8) Resolvent numerator completeness
- `DONE`: `P_n = u_n + sum_{j=1..n} q_j u_{n-j}` implemented for `0 <= n <= r-1`.
- Files:
  - `src/math/resolvent.cpp`

## 9) Roots / spectral / residues path
- `PARTIAL`:
  - Deterministic polynomial roots: implemented.
  - Rational factorization and spectral mode solve: implemented.
  - Residue coefficient extraction: implemented with deterministic local fitting.
- Files:
  - `src/math/complex_roots.cpp`
  - `src/math/spectral.cpp`
  - `src/math/residues.cpp`

## 10) Jet semigroup action
- `DONE`: deterministic triangular translation map on truncated dense jet basis.
- Files:
  - `src/math/semigroup.cpp`

## 11) Interstice transport law
- `PARTIAL`: deterministic transport integrator implemented with fixed-step update and mode transport using `Lambda_j(s)=lambda_j^{delta(s)}`; recurrence validation now includes per-slice fitted transported checks on `T_grid`.
- Files:
  - `src/math/interstice.cpp`

## 12) Risk layer naming and structure
- `PARTIAL`: risk separates `energy_framework` and `energy_proxy`; trajectory path fills framework energy first from theorem-side carriers (`Risk` or interstice-return carrier) and keeps proxy as research-only fallback.  `RiskField::energy_framework_missing_reason` carries the precise reason when framework energy is absent.  `PARTIAL` until live anchors actually populate `tr.Risk` end-to-end.
- Files:
  - `include/mt/state/risk_state.hpp`
  - `src/engine/risk_engine.cpp`

## 13) Signal layer quarantine from toy one-step rule
- `PARTIAL`: production signal path is field-functional over the full s-grid with framework-energy-first penalties and hard authority gates (theorem authority + framework energy required; no proxy in production).  Legacy scalar heuristic remains only behind explicit `signal.research_heuristic_signal=true` AND `backtest.research_non_authoritative=true`.  `PARTIAL` until live anchors drive a non-zero production signal with `failed_clauses=[]`.
- Files:
  - `src/backtest/signal_rules.cpp`
  - `include/mt/backtest/signal_rules.hpp`

## 14) Backtest compartment realization
- `DONE`: walk-forward orchestration, simulator, position sizing/order checks, metrics and reports wired.
- Files:
  - `src/backtest/walk_forward.cpp`
  - `src/backtest/simulator.cpp`
  - `src/backtest/position_sizer.cpp`
  - `src/backtest/order_model.cpp`
  - `src/backtest/report.cpp`

## 15) Config/CLI productionization
- `DONE`: typed config loaders with schema/range/consistency checks and real app flows.
- Files:
  - `src/core/config.cpp`
  - `src/api/json_io.cpp`
  - `src/app/run_extract.cpp`
  - `src/app/run_trajectorize.cpp`
  - `src/app/run_backtest.cpp`
  - `src/app/run_benchmark.cpp`

## 16) Calendar and symbol semantics
- `DONE`: deterministic calendar kinds and index/timestamp mappings added; symbol metadata fixed for ES futures semantics.
- Files:
  - `include/mt/data/trading_calendar.hpp`
  - `src/data/trading_calendar.cpp`
  - `src/data/symbol_store.cpp`
  - `configs/symbols/ES_CONT.json`

## 17) Determinism enforcement in build flags
- `DONE`: deterministic CUDA precision flags enforced under deterministic mode.
- `DONE`: CUDA is optional; default build is deterministic CPU theorem-scaffold mode with explicit non-authoritative reporting.
- Files:
  - `CMakeLists.txt`
  - `cmake/DetectCUDA.cmake`

## 18) Hygiene grep for scaffold markers
- `DONE`: forbidden strings removed from authoritative code.

## 19) Remaining high-math closure work
- `PARTIAL`: split authority gates exist (`fixture_scaffold_gate`, `symbol_fixture_exactness_gate`, `production_dataset_authority_gate`) with explicit level labeling.
- `OPEN`: production-dataset theorem authority sweep beyond fixture datasets.
- `OPEN`: complete GPU kernels (currently deterministic CPU theorem-scaffold interface; `src/cuda/kernels_risk.cu` and spectral kernels still zero outputs via `cudaMemsetAsync`).  Implementation authority remains `false` and MUST stay false until full parity tests against the CPU theorem path pass at the tolerances in `math_contract.md` clauses 4–10.
- `PARTIAL`: OCHLV packet-event constructor drives event boundaries; the deterministic max-transition synthetic fallback is now strictly research-only (gated on `cp_cfg.allow_synthetic_event_boundary_research_only`, default false) and ALWAYS fails the production clause `event_state.genuine_packet_dynamics`.
- `STALE_CLAIM_REMOVED`: replaced old `OPEN: full theorem-level proof harness ...` blanket claim with split `PARTIAL` + `OPEN` status above.
- `STALE_CLAIM_REMOVED`: removed the prior `DONE` claim that the synthetic fallback boundary was unconditionally enabled in production.

## 20) Current acceptance summary
- `DONE`: no lookahead in anchor state formation.
- `DONE`: canonical market-state mapping is single-source and carried through anchor artifacts.
- `DONE`: horizon grid includes `h=0` anchor point (`0..H`).
- `DONE`: theorem object and pairwise identity checks wired in `EvolutionEquality`.
- `DONE`: deterministic release build of `mt_core` succeeds.
- `DONE`: fixture-backed authority tooling is split by gate level and emits metadata-rich per-anchor authority results (engine/dataset hashes, tolerances, `Q`, clauses).
- `OPEN`: production-dataset theorem authority CI sweep remains in progress.

## 22) Legacy alias + artifact hygiene
- `PARTIAL`: `evaluate_future_section(...)` is now deprecated and retained only
  as a back-compat alias to the full packet evaluator. Theorem-path translation
  units are guarded by tests that ban alias usage.
- `DONE`: advisory bundle exports exclude raw data CSVs and the full
  `artifacts/` tree so stale/non-authoritative runtime logs do not pollute code
  review bundles.

## 21) Authoritative diagnostics state
- `DONE`: `validate_anchor_authoritativeness(AnchorArtifacts, EngineConfig)` is wired into `run_anchor`.
- `DONE`: diagnostics/artifacts now emit full authority context, acceptance clauses, closure order/status, active spectral modes, and signal/risk field decomposition paths.
- `DONE`: Path B honesty is enforced (`gpu_mode = "cpu_authoritative"` until real kernels are complete).
