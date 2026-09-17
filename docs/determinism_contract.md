# Determinism Contract

`mt_core` is bit-reproducible across runs on the same machine and CUDA driver
when built with `MT_DETERMINISTIC=ON` (default).

## Invariants
1. **FP64 on the critical path.** All math/extraction/recurrence/spectral kernels run in `double`.
2. **No fast-math, no TF32.** `--fmad=false`, `--use_fast_math=OFF`, `cublasMath_t` set to `CUBLAS_DEFAULT_MATH`.
3. **Fixed stream topology.** `enum class StreamId { H2D, Feature, Torus, Recurrence, Interstice, D2H, Count }` — index assignments are immutable.
4. **Pairwise summation.** All host reductions use `mt::deterministic::pairwise_sum` (CPU) or block-pairwise reductions (GPU).
5. **Fixed RNG seed.** `mt::deterministic::SEED = 0x9E3779B97F4A7C15ULL`.
6. **Fixed launch grid policy.** `DEFAULT_BLOCK = 256`, `grid_for(n) = (n + 255) / 256`.
7. **Fixed multi-index ordering.** `enumerate_multi_indices` produces lexicographic order.
8. **Stable sort everywhere.** `std::stable_sort` for canonicalization, `thrust::stable_sort_by_key` on device.

## What breaks determinism (forbidden)
- atomicAdd-based reductions on float values
- non-stable sorts
- async H2D/D2H reordering
- driver-level CUDA graphs without explicit topology pinning
