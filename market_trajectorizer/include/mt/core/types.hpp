#pragma once
// Canonical numeric/type aliases for the entire engine.
// Core path is FP64 + std::complex<double>; FP32 only for non-authoritative
// profiling builds (gated by MT_ENABLE_FP32_PROFILE).

#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mt {

using i32  = std::int32_t;
using i64  = std::int64_t;
using u32  = std::uint32_t;
using u64  = std::uint64_t;
using usize = std::size_t;

using real = double;
using cplx = std::complex<double>;

using Timestamp = std::int64_t;  // canonical: UNIX nanoseconds

template <class T> using Vec = std::vector<T>;

struct HorizonGrid {
    Vec<real> h;       // forecast horizons (unitless steps from anchor)
};

struct MultiIndexLayout {
    int Q;             // number of channels
    int order_max;     // |alpha| <= order_max
    Vec<usize> strides;
};

}  // namespace mt
