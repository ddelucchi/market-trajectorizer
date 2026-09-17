#pragma once
#include "mt/data/ohlcv.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace mt {

struct SymbolMeta {
    std::string symbol;
    std::string timeframe;   // e.g. "1d", "1h", "1m"
    std::string canonical_path;
    Timestamp   t_first = 0;
    Timestamp   t_last  = 0;
    usize       n_bars  = 0;
};

class SymbolStore {
public:
    void register_symbol(SymbolMeta meta);
    const SymbolMeta& get(std::string_view symbol) const;
    bool has(std::string_view symbol) const;
    std::vector<std::string> symbols() const;

    void save_manifest(std::string_view path) const;
    void load_manifest(std::string_view path);
private:
    std::unordered_map<std::string, SymbolMeta> map_;
};

}  // namespace mt
