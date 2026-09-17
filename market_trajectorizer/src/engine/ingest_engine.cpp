#include "mt/engine/ingest_engine.hpp"
#include "mt/data/canonicalizer.hpp"
#include "mt/data/csv_reader.hpp"
#include <filesystem>

namespace mt {

IngestReport run_ingest(std::string_view input_csv,
                        std::string_view symbol,
                        std::string_view /*timeframe*/,
                        std::string_view out_dir)
{
    IngestReport rep;
    auto raw = read_csv(input_csv);
    rep.n_in = raw.n;
    auto canon = canonicalize(raw);
    rep.n_out  = canon.n;
    rep.n_dedup = rep.n_in - rep.n_out;
    std::filesystem::create_directories(std::string(out_dir));
    rep.canonical_path = std::string(out_dir) + "/" + std::string(symbol) + ".bin";
    save_canonical(canon, rep.canonical_path);
    return rep;
}

}  // namespace mt
