#include "mt/data/dataset.hpp"
#include "mt/data/canonicalizer.hpp"

namespace mt {
Dataset load_dataset(const SymbolStore& store, std::string_view symbol) {
    const SymbolMeta& m = store.get(symbol);
    Dataset d;
    d.symbol    = m.symbol;
    d.timeframe = m.timeframe;
    d.candles   = load_canonical(m.canonical_path);
    return d;
}
}
