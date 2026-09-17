#pragma once
#include "mt/data/ohlcv.hpp"
#include "mt/data/symbol_store.hpp"
#include <string>

namespace mt {

struct Dataset {
    std::string    symbol;
    std::string    timeframe;
    CandleColumns  candles;
};

Dataset load_dataset(const SymbolStore& store, std::string_view symbol);

}  // namespace mt
