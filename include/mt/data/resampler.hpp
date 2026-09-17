#pragma once
#include "mt/data/ohlcv.hpp"

namespace mt {

// Resample bars to a coarser uniform grid. OHLC are aggregated as
// open=first, high=max, low=min, close=last; volume = sum.
CandleColumns resample(const CandleColumns& in, int factor);

}  // namespace mt
