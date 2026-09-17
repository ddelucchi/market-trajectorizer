#pragma once
#include "mt/data/dataset.hpp"

namespace mt {

struct IngestReport {
    usize n_in        = 0;
    usize n_out       = 0;
    usize n_dedup     = 0;
    usize n_invalid   = 0;
    std::string canonical_path;
};

IngestReport run_ingest(std::string_view input_csv,
                        std::string_view symbol,
                        std::string_view timeframe,
                        std::string_view out_dir);

}  // namespace mt
