#include "mt/data/feature_map.hpp"
#include "mt/core/constants.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace mt {

MarketStateColumns build_market_state_from_candles(const CandleColumns& in, real volume_norm) {
    MarketStateColumns s;
    s.resize(in.n);
    const real v_norm = (volume_norm > 0.0) ? volume_norm : 1.0;
    for (usize i = 0; i < in.n; ++i) {
        s.x0[i] = std::log(in.open[i]);
        s.x1[i] = std::log(in.high[i]);
        s.x2[i] = std::log(in.low[i]);
        s.x3[i] = std::log(in.close[i]);
        const real v = std::max(in.volume[i], 0.0);
        s.x4[i] = std::log(1.0 + (v / v_norm));
    }
    return s;
}

MarketStateColumns build_market_state_cpu(const CandleColumns& in) {
    return build_market_state_from_candles(in, constants::LOG_VOL_EPS);
}

DeviceMarketStateColumns build_market_state_gpu(const DeviceCandleColumns&, cudaStream_t) {
    // Device buffer wiring lives in src/cuda/kernels_feature.cu.
    return DeviceMarketStateColumns{};
}

DerivedChannels build_derived_channels(const MarketStateColumns& s, int w) {
    DerivedChannels d;
    d.ret1.assign(s.n, 0.0); d.range.assign(s.n, 0.0);
    d.body.assign(s.n, 0.0); d.gap.assign(s.n, 0.0);
    d.upper_wick.assign(s.n, 0.0);
    d.lower_wick.assign(s.n, 0.0);
    d.wick_asymmetry.assign(s.n, 0.0);
    d.volume_shock.assign(s.n, 0.0);
    d.vol_proxy.assign(s.n, 0.0);
    for (usize i = 0; i < s.n; ++i) {
        if (i > 0) d.ret1[i] = s.x3[i] - s.x3[i-1];
        d.range[i] = s.x1[i] - s.x2[i];
        d.body [i] = s.x3[i] - s.x0[i];
        if (i > 0) d.gap[i] = s.x0[i] - s.x3[i-1];

        const real body_hi = std::max(s.x0[i], s.x3[i]);
        const real body_lo = std::min(s.x0[i], s.x3[i]);
        d.upper_wick[i] = s.x1[i] - body_hi;
        d.lower_wick[i] = body_lo - s.x2[i];
        d.wick_asymmetry[i] = d.upper_wick[i] - d.lower_wick[i];
    }
    if (w > 1) {
        for (usize i = static_cast<usize>(w); i < s.n; ++i) {
            real mean = 0.0; for (int k = 0; k < w; ++k) mean += d.ret1[i-k]; mean /= w;
            real var = 0.0;  for (int k = 0; k < w; ++k) { real e = d.ret1[i-k] - mean; var += e*e; }
            d.vol_proxy[i] = std::sqrt(var / w);

            real mean_x4 = 0.0;
            real var_x4 = 0.0;
            for (int k = 1; k <= w; ++k) mean_x4 += s.x4[i-k];
            mean_x4 /= static_cast<real>(w);
            for (int k = 1; k <= w; ++k) {
                const real e = s.x4[i-k] - mean_x4;
                var_x4 += e * e;
            }
            const real std_x4 = std::sqrt(var_x4 / static_cast<real>(w));
            if (std_x4 > 0.0) d.volume_shock[i] = (s.x4[i] - mean_x4) / std_x4;
        }
    }
    return d;
}

}  // namespace mt
