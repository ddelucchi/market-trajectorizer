// Symbol store: in-memory registry plus a deterministic JSON manifest format.
//
// Manifest format (single object, line-per-symbol arrays - written by hand,
// no external dependency):
//   {
//     "schema": "symbol_store_v1",
//     "symbols": [
//       {"symbol":"...","timeframe":"...","canonical_path":"...",
//        "t_first":..., "t_last":..., "n_bars":...},
//       ...
//     ]
//   }

#include "mt/api/json_io.hpp"
#include "mt/core/errors.hpp"
#include "mt/data/symbol_store.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace mt {

void SymbolStore::register_symbol(SymbolMeta meta) {
    map_[meta.symbol] = std::move(meta);
}

bool SymbolStore::has(std::string_view symbol) const {
    return map_.find(std::string(symbol)) != map_.end();
}

std::vector<std::string> SymbolStore::symbols() const {
    std::vector<std::string> out;
    out.reserve(map_.size());
    for (const auto& kv : map_) out.push_back(kv.first);
    std::sort(out.begin(), out.end());
    return out;
}

const SymbolMeta& SymbolStore::get(std::string_view symbol) const {
    auto it = map_.find(std::string(symbol));
    if (it == map_.end()) throw DataError("SymbolStore::get: unknown symbol " + std::string(symbol));
    return it->second;
}

void SymbolStore::save_manifest(std::string_view path) const {
    std::ostringstream os;
    os << "{\n";
    os << "  \"schema\": \"symbol_store_v1\",\n";
    os << "  \"symbols\": [";
    bool first = true;
    for (const auto& [k, m] : map_) {
        if (!first) os << ",";
        first = false;
        os << "\n    {"
           << "\"symbol\":\"" << m.symbol << "\","
           << "\"timeframe\":\"" << m.timeframe << "\","
           << "\"canonical_path\":\"" << m.canonical_path << "\","
           << "\"t_first\":" << m.t_first << ","
           << "\"t_last\":"  << m.t_last  << ","
           << "\"n_bars\":"  << m.n_bars
           << "}";
    }
    os << "\n  ]\n}\n";
    json_io::write_text(path, os.str());
}

void SymbolStore::load_manifest(std::string_view path) {
    map_.clear();
    std::string text = json_io::read_text(path);
    // Find "symbols" array, then parse each {...} block.
    auto p = text.find("\"symbols\"");
    if (p == std::string::npos) throw DataError("symbol_store: missing 'symbols' field");
    p = text.find('[', p);
    if (p == std::string::npos) throw DataError("symbol_store: malformed 'symbols' array");
    ++p;
    auto end = text.rfind(']');
    if (end == std::string::npos || end < p) throw DataError("symbol_store: missing array close");

    auto extract_str = [&](const std::string& obj, const char* key, std::string& out) {
        auto pos = obj.find(key);
        if (pos == std::string::npos) return false;
        pos = obj.find(':', pos); if (pos == std::string::npos) return false;
        pos = obj.find('"', pos); if (pos == std::string::npos) return false;
        auto q = obj.find('"', pos + 1); if (q == std::string::npos) return false;
        out = obj.substr(pos + 1, q - pos - 1);
        return true;
    };
    auto extract_num = [&](const std::string& obj, const char* key) -> long long {
        auto pos = obj.find(key);
        if (pos == std::string::npos) return 0;
        pos = obj.find(':', pos); if (pos == std::string::npos) return 0;
        ++pos;
        while (pos < obj.size() && (obj[pos] == ' ' || obj[pos] == '\t')) ++pos;
        return std::strtoll(obj.c_str() + pos, nullptr, 10);
    };

    usize i = p;
    while (i < end) {
        while (i < end && text[i] != '{') ++i;
        if (i >= end) break;
        usize j = i + 1; int depth = 1;
        while (j < end && depth > 0) {
            if (text[j] == '{') ++depth;
            else if (text[j] == '}') --depth;
            ++j;
        }
        std::string obj = text.substr(i, j - i);
        SymbolMeta m;
        extract_str(obj, "\"symbol\"",         m.symbol);
        extract_str(obj, "\"timeframe\"",      m.timeframe);
        extract_str(obj, "\"canonical_path\"", m.canonical_path);
        m.t_first = extract_num(obj, "\"t_first\"");
        m.t_last  = extract_num(obj, "\"t_last\"");
        m.n_bars  = static_cast<usize>(extract_num(obj, "\"n_bars\""));
        if (!m.symbol.empty()) map_[m.symbol] = m;
        i = j;
    }
}

}  // namespace mt
