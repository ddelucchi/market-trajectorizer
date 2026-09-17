#pragma once
#include "mt/data/ohlcv.hpp"
#include <string_view>

namespace mt {

// Read CSV with header: timestamp,open,high,low,close,volume.
// Accepted ts: UNIX seconds, UNIX milliseconds, ISO8601 UTC.
CandleColumns read_csv(std::string_view path);

}  // namespace mt
