#pragma once
#include "mt/data/ohlcv.hpp"
#include "mt/state/market_state.hpp"
// CUDA-aware build: forward-declare cudaStream_t to avoid including cuda_runtime.h
// from public headers.
using cudaStream_t = struct CUstream_st*;

namespace mt {

// Canonical 5-channel market state map used across extraction/trajectory/risk/signal:
//   x0 = log(open), x1 = log(high), x2 = log(low),
//   x3 = log(close), x4 = log(1 + volume / volume_norm).
MarketStateColumns      build_market_state_from_candles(const CandleColumns& in,
                                                        real volume_norm = 1.0);

// Backward-compatible alias for existing call sites.
MarketStateColumns      build_market_state_cpu(const CandleColumns& in);
DeviceMarketStateColumns build_market_state_gpu(const DeviceCandleColumns& in,
                                                cudaStream_t stream);

// Optional derived channels.
struct DerivedChannels {
    Vec<real> ret1;   // x3[i] - x3[i-1]
    Vec<real> range;  // x1 - x2
    Vec<real> body;   // x3 - x0
    Vec<real> gap;    // x0[i] - x3[i-1]
    Vec<real> upper_wick;
    Vec<real> lower_wick;
    Vec<real> wick_asymmetry;
    Vec<real> volume_shock;
    Vec<real> vol_proxy;
};
DerivedChannels build_derived_channels(const MarketStateColumns& s, int rolling_window);

}  // namespace mt
