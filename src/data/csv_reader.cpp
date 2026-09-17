#include "mt/data/csv_reader.hpp"
#include "mt/core/errors.hpp"
#include <charconv>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>

namespace mt {

namespace {

Timestamp parse_ts(const std::string& tok) {
    // Try integer first.
    if (!tok.empty() && (std::isdigit(static_cast<unsigned char>(tok.front())) || tok.front() == '-')) {
        long long v = 0;
        auto [ptr, ec] = std::from_chars(tok.data(), tok.data() + tok.size(), v);
        if (ec == std::errc{} && ptr == tok.data() + tok.size()) {
            // Heuristic: < 10^11 = seconds; < 10^14 = ms; else ns.
            if      (v < 100000000000LL)    return v * 1'000'000'000LL;       // s   -> ns
            else if (v < 100000000000000LL) return v * 1'000'000LL;           // ms  -> ns
            else                             return v;                         // ns
        }
    }
    // ISO8601 UTC: YYYY-MM-DDTHH:MM:SSZ
    std::tm tm{};
    std::istringstream iss(tok);
    iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (iss.fail()) {
        iss.clear(); iss.str(tok);
        iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    }
    if (iss.fail()) throw DataError("Unparseable timestamp: " + tok);
#if defined(_WIN32)
    auto t = _mkgmtime(&tm);
#else
    auto t = timegm(&tm);
#endif
    return static_cast<Timestamp>(t) * 1'000'000'000LL;
}

} // namespace

CandleColumns read_csv(std::string_view path) {
    std::ifstream f{std::string(path)};
    if (!f) throw DataError("Cannot open CSV: " + std::string(path));
    CandleColumns out;
    std::string line;
    bool header_skipped = false;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        if (!header_skipped) { header_skipped = true; if (!std::isdigit(static_cast<unsigned char>(line.front())) && line.front() != '-') continue; }
        std::stringstream ss(line);
        std::string ts_s, o, h, l, c, v;
        if (!std::getline(ss, ts_s, ',') || !std::getline(ss, o, ',') ||
            !std::getline(ss, h,  ',') || !std::getline(ss, l, ',') ||
            !std::getline(ss, c,  ',') || !std::getline(ss, v, ',')) {
            throw DataError("Malformed CSV row: " + line);
        }
        out.ts.push_back(parse_ts(ts_s));
        out.open.push_back(std::stod(o));
        out.high.push_back(std::stod(h));
        out.low.push_back(std::stod(l));
        out.close.push_back(std::stod(c));
        out.volume.push_back(std::stod(v));
    }
    out.n = out.ts.size();
    return out;
}

}  // namespace mt
