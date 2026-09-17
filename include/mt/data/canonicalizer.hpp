#pragma once
#include "mt/data/ohlcv.hpp"
#include <string_view>

namespace mt {

// Sort by ts asc, dedupe (last-write-wins on identical ts), validate OHLC + volume,
// and return the canonical CandleColumns.
CandleColumns canonicalize(const CandleColumns& in);

void validate_candles(const CandleColumns& c);

void save_canonical(const CandleColumns& c, std::string_view path);
CandleColumns load_canonical(std::string_view path);

}  // namespace mt
