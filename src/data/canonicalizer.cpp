#include "mt/data/canonicalizer.hpp"
#include "mt/core/errors.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <numeric>
#include <vector>

namespace mt {

void validate_candles(const CandleColumns& c) {
    for (usize i = 0; i < c.n; ++i) {
        const real lo = c.low[i], hi = c.high[i], op = c.open[i], cl = c.close[i], v = c.volume[i];
        if (!(lo <= op && lo <= cl && lo <= hi)) throw DataError("low constraint violated at " + std::to_string(i));
        if (!(hi >= op && hi >= cl && hi >= lo)) throw DataError("high constraint violated at " + std::to_string(i));
        if (!(v >= 0))                            throw DataError("negative volume at " + std::to_string(i));
        if (i > 0 && !(c.ts[i] > c.ts[i-1]))     throw DataError("non-monotonic ts at " + std::to_string(i));
    }
}

CandleColumns canonicalize(const CandleColumns& in) {
    std::vector<usize> idx(in.n);
    std::iota(idx.begin(), idx.end(), usize{0});
    std::sort(idx.begin(), idx.end(),
              [&](usize a, usize b){ return in.ts[a] < in.ts[b]; });
    CandleColumns out;
    out.resize(in.n);
    Timestamp prev = std::numeric_limits<Timestamp>::min();
    usize w = 0;
    for (usize r : idx) {
        if (in.ts[r] == prev) {  // dedupe: last write wins
            --w;
        }
        out.ts[w]     = in.ts[r];
        out.open[w]   = in.open[r];
        out.high[w]   = in.high[r];
        out.low[w]    = in.low[r];
        out.close[w]  = in.close[r];
        out.volume[w] = in.volume[r];
        prev = in.ts[r];
        ++w;
    }
    out.resize(w);
    validate_candles(out);
    return out;
}

namespace {
constexpr char MAGIC[8] = {'M','T','C','A','N','D','0','1'};
}

void save_canonical(const CandleColumns& c, std::string_view path) {
    std::ofstream f{std::string(path), std::ios::binary};
    if (!f) throw DataError("Cannot open for write: " + std::string(path));
    f.write(MAGIC, 8);
    u64 n = c.n;
    f.write(reinterpret_cast<const char*>(&n), sizeof(n));
    f.write(reinterpret_cast<const char*>(c.ts.data()),     n * sizeof(Timestamp));
    f.write(reinterpret_cast<const char*>(c.open.data()),   n * sizeof(real));
    f.write(reinterpret_cast<const char*>(c.high.data()),   n * sizeof(real));
    f.write(reinterpret_cast<const char*>(c.low.data()),    n * sizeof(real));
    f.write(reinterpret_cast<const char*>(c.close.data()),  n * sizeof(real));
    f.write(reinterpret_cast<const char*>(c.volume.data()), n * sizeof(real));
}

CandleColumns load_canonical(std::string_view path) {
    std::ifstream f{std::string(path), std::ios::binary};
    if (!f) throw DataError("Cannot open: " + std::string(path));
    char magic[8];
    f.read(magic, 8);
    if (std::memcmp(magic, MAGIC, 8) != 0) throw DataError("Bad magic in canonical file: " + std::string(path));
    u64 n = 0;
    f.read(reinterpret_cast<char*>(&n), sizeof(n));
    CandleColumns c; c.resize(static_cast<usize>(n));
    f.read(reinterpret_cast<char*>(c.ts.data()),     n * sizeof(Timestamp));
    f.read(reinterpret_cast<char*>(c.open.data()),   n * sizeof(real));
    f.read(reinterpret_cast<char*>(c.high.data()),   n * sizeof(real));
    f.read(reinterpret_cast<char*>(c.low.data()),    n * sizeof(real));
    f.read(reinterpret_cast<char*>(c.close.data()),  n * sizeof(real));
    f.read(reinterpret_cast<char*>(c.volume.data()), n * sizeof(real));
    return c;
}

}  // namespace mt
