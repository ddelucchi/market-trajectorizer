#include "mt/app/run_verify_authority.hpp"

#include "mt/api/json_io.hpp"
#include "mt/core/config.hpp"
#include "mt/data/dataset.hpp"
#include "mt/data/symbol_store.hpp"
#include "mt/engine/pipeline.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace mt::app {

namespace {

int parse_nonnegative(std::string_view text, int fallback) {
    if (text.empty()) return fallback;
    try {
        int v = std::stoi(std::string(text));
        return v < 0 ? fallback : v;
    } catch (...) {
        return fallback;
    }
}

std::uint64_t fnv1a_bytes(const void* data, usize size, std::uint64_t seed = 1469598103934665603ull) {
    auto h = seed;
    const auto* p = static_cast<const unsigned char*>(data);
    for (usize i = 0; i < size; ++i) {
        h ^= static_cast<std::uint64_t>(p[i]);
        h *= 1099511628211ull;
    }
    return h;
}

template <class T>
std::uint64_t fnv1a_pod(const T& v, std::uint64_t seed) {
    return fnv1a_bytes(&v, sizeof(T), seed);
}

std::string to_hex64(std::uint64_t v) {
    std::ostringstream s;
    s << std::hex;
    s.width(16);
    s.fill('0');
    s << v;
    return s.str();
}

std::string hash_engine_config(std::string_view path) {
    const std::string text = json_io::read_text(path);
    return to_hex64(fnv1a_bytes(text.data(), text.size()));
}

std::string hash_dataset(const Dataset& ds) {
    std::uint64_t h = 1469598103934665603ull;
    h = fnv1a_bytes(ds.symbol.data(), ds.symbol.size(), h);
    h = fnv1a_bytes(ds.timeframe.data(), ds.timeframe.size(), h);
    h = fnv1a_pod(ds.candles.n, h);
    for (usize i = 0; i < ds.candles.n; ++i) {
        h = fnv1a_pod(ds.candles.ts[i], h);
        h = fnv1a_pod(ds.candles.open[i], h);
        h = fnv1a_pod(ds.candles.high[i], h);
        h = fnv1a_pod(ds.candles.low[i], h);
        h = fnv1a_pod(ds.candles.close[i], h);
        h = fnv1a_pod(ds.candles.volume[i], h);
    }
    return to_hex64(h);
}

void write_failed_clauses(std::ostream& os, const Vec<std::string>& failed) {
    os << "[";
    for (usize i = 0; i < failed.size(); ++i) {
        if (i) os << ",";
        os << "\"" << failed[i] << "\"";
    }
    os << "]";
}

std::vector<SymbolMeta> parse_dataset_manifest_fallback(std::string_view path) {
    std::vector<SymbolMeta> out;

    const std::string text = json_io::read_text(path);
    auto p = text.find("\"datasets\"");
    if (p == std::string::npos) return out;
    p = text.find('[', p);
    if (p == std::string::npos) return out;
    ++p;

    const auto end = text.rfind(']');
    if (end == std::string::npos || end < p) return out;

    auto extract_str = [](const std::string& obj, const char* key, std::string& value) {
        auto pos = obj.find(key);
        if (pos == std::string::npos) return false;
        pos = obj.find(':', pos); if (pos == std::string::npos) return false;
        pos = obj.find('"', pos); if (pos == std::string::npos) return false;
        const auto q = obj.find('"', pos + 1); if (q == std::string::npos) return false;
        value = obj.substr(pos + 1, q - pos - 1);
        return true;
    };

    auto extract_num = [](const std::string& obj, const char* key) -> long long {
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

        usize j = i + 1;
        int depth = 1;
        while (j < end && depth > 0) {
            if (text[j] == '{') ++depth;
            else if (text[j] == '}') --depth;
            ++j;
        }

        const std::string obj = text.substr(i, j - i);
        SymbolMeta m;
        extract_str(obj, "\"symbol\"", m.symbol);
        extract_str(obj, "\"timeframe\"", m.timeframe);
        if (!extract_str(obj, "\"canonical_path\"", m.canonical_path)) {
            extract_str(obj, "\"path\"", m.canonical_path);
        }
        m.t_first = static_cast<Timestamp>(extract_num(obj, "\"t_first\""));
        m.t_last  = static_cast<Timestamp>(extract_num(obj, "\"t_last\""));
        m.n_bars  = static_cast<usize>(extract_num(obj, "\"n_bars\""));

        if (!m.symbol.empty() && !m.canonical_path.empty()) {
            if (m.timeframe.empty()) m.timeframe = "unknown";
            out.push_back(std::move(m));
        }
        i = j;
    }

    return out;
}

}  // namespace

int run_verify_authority(const CliArgs& a) {
    const auto manifest_path = a.get("manifest");
    if (manifest_path.empty()) {
        std::cerr << "run_verify_authority: --manifest <symbols.json> required\n";
        return 2;
    }

    const auto engine_cfg_path = a.get("engine", "configs/engine/default.json");
    const auto symbol_filter   = a.get("symbol");
    const bool last_only       = a.has("last_only");
    const bool production_mode = a.has("production");
    const std::string gate_level = a.get("gate_level",
        production_mode ? "production_dataset_authority_gate" : "fixture_scaffold_gate");
    const int start_anchor_arg = parse_nonnegative(a.get("start_anchor"), -1);
    const int end_anchor_arg   = parse_nonnegative(a.get("end_anchor"), -1);
    const int max_failures     = std::max(1, parse_nonnegative(a.get("max_failures"), 32));

    std::string engine_hash;
    try {
        engine_hash = hash_engine_config(engine_cfg_path);
    } catch (...) {
        engine_hash = "unavailable";
    }

    SymbolStore store;
    try {
        store.load_manifest(manifest_path);
    } catch (const std::exception& e) {
        std::vector<SymbolMeta> fallback;
        try {
            fallback = parse_dataset_manifest_fallback(manifest_path);
        } catch (const std::exception& fallback_error) {
            std::cerr << "run_verify_authority: failed to load manifest: " << e.what()
                      << "; fallback parse failed: " << fallback_error.what() << "\n";
            return 3;
        }

        if (fallback.empty()) {
            std::cerr << "run_verify_authority: manifest is not a symbol_store manifest and contains no dataset entries\n";
            return 4;
        }

        for (const auto& meta : fallback) store.register_symbol(meta);
    }

    std::vector<std::string> symbols;
    if (!symbol_filter.empty()) {
        if (!store.has(symbol_filter)) {
            std::cerr << "run_verify_authority: symbol not present in manifest: " << symbol_filter << "\n";
            return 4;
        }
        symbols.push_back(symbol_filter);
    } else {
        symbols = store.symbols();
    }

    if (symbols.empty()) {
        std::cerr << "run_verify_authority: manifest contains no symbols\n";
        return 4;
    }

    EngineConfig cfg;
    try {
        cfg = load_engine_config(engine_cfg_path);
    } catch (const std::exception& e) {
        std::cerr << "run_verify_authority: failed to load engine config: " << e.what() << "\n";
        return 5;
    }

    int symbols_checked = 0;
    int anchors_checked = 0;
    int theorem_failures = 0;
    int implementation_non_authoritative = 0;
    int production_non_authoritative = 0;
    int active_failures = 0;

    for (const std::string& symbol : symbols) {
        Dataset ds;
        try {
            ds = load_dataset(store, symbol);
        } catch (const std::exception& e) {
            std::cerr << "run_verify_authority: failed to load dataset for " << symbol << ": " << e.what() << "\n";
            ++theorem_failures;
            if (theorem_failures >= max_failures) break;
            continue;
        }

        if (ds.candles.n == 0) {
            std::cerr << "run_verify_authority: empty dataset for " << symbol << "\n";
            ++theorem_failures;
            if (theorem_failures >= max_failures) break;
            continue;
        }

        ++symbols_checked;

        const int n = static_cast<int>(ds.candles.n);
        int start_anchor = std::max(cfg.recurrence_window, 0);
        if (start_anchor_arg >= 0) start_anchor = std::max(start_anchor, start_anchor_arg);

        int end_anchor = n - 1;
        if (end_anchor_arg >= 0) end_anchor = std::min(end_anchor, end_anchor_arg);

        if (last_only) start_anchor = end_anchor;
        if (start_anchor > end_anchor) continue;

        const std::string dataset_hash = hash_dataset(ds);

        for (int t = start_anchor; t <= end_anchor; ++t) {
            AnchorArtifacts art = run_anchor(ds.candles, t, cfg);
            ++anchors_checked;

            bool anchor_failed = false;
            const bool theorem_failed = !art.authority.theorem_authoritative;
            const bool impl_failed = !art.authority.implementation_authoritative;
            const bool production_failed = !art.authority.production_ready;

            if (impl_failed) {
                ++implementation_non_authoritative;
                if (production_mode) {
                    std::cerr << "implementation_authority_fail symbol=" << symbol
                              << " anchor=" << t
                              << " ts=" << art.anchor_ts
                              << " regime=" << authority_regime_name(art.authority.regime)
                              << " gpu_mode=" << art.authority.gpu_mode
                              << "\n";
                    anchor_failed = true;
                }
            }

            if (production_failed) {
                ++production_non_authoritative;
                if (production_mode) {
                    std::cerr << "production_authority_fail symbol=" << symbol
                              << " anchor=" << t
                              << " ts=" << art.anchor_ts
                              << " theorem_authoritative=" << (art.authority.theorem_authoritative ? "true" : "false")
                              << " implementation_authoritative=" << (art.authority.implementation_authoritative ? "true" : "false");
                    if (!art.authority.failed_clauses.empty()) {
                        std::cerr << " clauses=";
                        const usize k = std::min<usize>(art.authority.failed_clauses.size(), 6);
                        for (usize i = 0; i < k; ++i) {
                            if (i) std::cerr << ",";
                            std::cerr << art.authority.failed_clauses[i];
                        }
                    }
                    std::cerr << "\n";
                    anchor_failed = true;
                }
            }

            if (theorem_failed) {
                ++theorem_failures;
                std::cerr << "theorem_authority_fail symbol=" << symbol
                          << " anchor=" << t
                          << " ts=" << art.anchor_ts;
                if (!art.authority.failed_clauses.empty()) {
                    std::cerr << " clauses=";
                    const usize k = std::min<usize>(art.authority.failed_clauses.size(), 6);
                    for (usize i = 0; i < k; ++i) {
                        if (i) std::cerr << ",";
                        std::cerr << art.authority.failed_clauses[i];
                    }
                }
                std::cerr << "\n";
                anchor_failed = true;
            }

            std::cout << "authority_result {"
                      << "\"gate_level\":\"" << gate_level << "\""
                      << ",\"engine_config_hash\":\"" << engine_hash << "\""
                      << ",\"dataset_hash\":\"" << dataset_hash << "\""
                      << ",\"symbol\":\"" << symbol << "\""
                      << ",\"timeframe\":\"" << ds.timeframe << "\""
                      << ",\"anchor\":" << t
                      << ",\"anchor_ts\":" << art.anchor_ts
                      << ",\"Q\":" << cfg.torus.Q
                      << ",\"tol_evolution\":" << cfg.tol_evolution
                      << ",\"tol_closure\":" << cfg.tol_closure
                      << ",\"tol_interstice\":" << cfg.tol_interstice
                      << ",\"theorem_authoritative\":" << (art.authority.theorem_authoritative ? "true" : "false")
                      << ",\"implementation_authoritative\":" << (art.authority.implementation_authoritative ? "true" : "false")
                      << ",\"production_ready\":" << (art.authority.production_ready ? "true" : "false")
                      << ",\"max_theorem_residual\":" << art.authority.max_theorem_residual
                      << ",\"max_jet_normalization_residual\":" << art.authority.max_jet_normalization_residual
                      << ",\"max_closure_residual\":" << art.authority.max_closure_residual
                      << ",\"max_interstice_pde_residual\":" << art.authority.max_interstice_pde_residual
                      << ",\"max_interstice_recurrence_residual\":" << art.authority.max_interstice_recurrence_residual
                      << ",\"theorem_slice_single_axis\":" << (art.authority.theorem_slice_single_axis ? "true" : "false")
                      << ",\"theorem_local_sector_verified\":" << (art.authority.theorem_local_sector_verified ? "true" : "false")
                      << ",\"theorem_valid_h_max\":" << art.authority.theorem_valid_h_max
                      << ",\"theorem_valid_h_count\":" << art.authority.theorem_valid_h_count
                      << ",\"theorem_valid_h_fraction\":" << art.authority.theorem_valid_h_fraction
                      << ",\"theorem_valid_h_coeff_cap\":" << art.authority.theorem_valid_h_coeff_cap
                      << ",\"theorem_valid_h_packet_cap\":" << art.authority.theorem_valid_h_packet_cap
                      << ",\"theorem_valid_h_condition_cap\":" << art.authority.theorem_valid_h_condition_cap
                      << ",\"theorem_valid_h_residual_cap\":" << art.authority.theorem_valid_h_residual_cap
                      << ",\"theorem_coeff_radius_estimate\":" << art.authority.theorem_coeff_radius_estimate
                      << ",\"theorem_recurrence_condition\":" << art.authority.theorem_recurrence_condition
                      << ",\"err_continuous_jet\":" << art.evolution.err_continuous_jet
                      << ",\"err_jet_newton\":" << art.evolution.err_jet_newton
                      << ",\"err_newton_matrix\":" << art.evolution.err_newton_matrix
                      << ",\"err_matrix_rational\":" << art.evolution.err_matrix_rational
                      << ",\"err_rational_spectral\":" << art.evolution.err_rational_spectral
                      << ",\"closed_regime\":" << (art.evolution.closed_regime ? "true" : "false")
                      << ",\"recurrence_r\":" << art.recurrence.rec.r
                      << ",\"failed_clauses\":";
            write_failed_clauses(std::cout, art.authority.failed_clauses);
            std::cout << "}\n";

            if (production_mode) {
                if (impl_failed || production_failed || theorem_failed) anchor_failed = true;
            } else {
                if (theorem_failed) anchor_failed = true;
            }

            if (anchor_failed) {
                ++active_failures;
            }

            if (active_failures >= max_failures) {
                break;
            }
        }

        if (active_failures >= max_failures) {
            std::cerr << "run_verify_authority: stopping early at max_failures=" << max_failures << "\n";
            break;
        }
    }

    std::cout << "verify_authority: symbols_checked=" << symbols_checked
              << " anchors_checked=" << anchors_checked
              << " theorem_failures=" << theorem_failures
              << " implementation_non_authoritative=" << implementation_non_authoritative
              << " production_non_authoritative=" << production_non_authoritative
              << " gate_level=" << gate_level
              << " engine_config_hash=" << engine_hash
              << " Q=" << cfg.torus.Q
              << " tol_evolution=" << cfg.tol_evolution
              << " tol_closure=" << cfg.tol_closure
              << " tol_interstice=" << cfg.tol_interstice
              << " mode=" << (production_mode ? "production" : "theorem")
              << " gpu_mode=cpu_authoritative\n";

    if (anchors_checked == 0) {
        std::cerr << "run_verify_authority: no anchors were evaluated\n";
        return 6;
    }
    if (production_mode) {
        return (theorem_failures == 0
             && implementation_non_authoritative == 0
             && production_non_authoritative == 0) ? 0 : 1;
    }
    return theorem_failures == 0 ? 0 : 1;
}

}  // namespace mt::app
