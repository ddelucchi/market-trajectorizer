#pragma once
// Determinism contract knobs (compile-time + runtime).
//
//   same input csv + same config + same binary
//   + same GPU arch + same CUDA toolkit + same driver branch
//   => bitwise-identical outputs.
//
// Rules enforced by this header:
//   * fixed reduction tree (pairwise) for floating-point sums on host
//   * fixed PRNG seed for any randomized auxiliary
//   * no atomic FP accumulation in user kernels (kernels guard via static_assert)

#include "mt/core/types.hpp"

namespace mt::deterministic {

inline constexpr u64 SEED = 0x9E3779B97F4A7C15ULL;  // golden-ratio constant

// Pairwise sum over a contiguous range; bitwise stable for fixed length.
template <class It>
auto pairwise_sum(It first, It last) -> typename std::iterator_traits<It>::value_type {
    using T = typename std::iterator_traits<It>::value_type;
    auto n = std::distance(first, last);
    if (n == 0) return T{};
    if (n == 1) return *first;
    auto mid = first + n / 2;
    return pairwise_sum(first, mid) + pairwise_sum(mid, last);
}

}  // namespace mt::deterministic
