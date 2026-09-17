#include "mt/data/resampler.hpp"
#include "mt/core/errors.hpp"

namespace mt {

CandleColumns resample(const CandleColumns& in, int factor) {
    if (factor <= 1) return in;
    CandleColumns out;
    const usize n_out = (in.n + factor - 1) / static_cast<usize>(factor);
    out.resize(n_out);
    usize w = 0;
    for (usize i = 0; i < in.n; i += static_cast<usize>(factor), ++w) {
        const usize j_end = std::min(in.n, i + static_cast<usize>(factor));
        out.ts[w]    = in.ts[i];
        out.open[w]  = in.open[i];
        out.close[w] = in.close[j_end - 1];
        real hi = in.high[i], lo = in.low[i], v = 0.0;
        for (usize k = i; k < j_end; ++k) {
            hi = std::max(hi, in.high[k]);
            lo = std::min(lo, in.low[k]);
            v += in.volume[k];
        }
        out.high[w] = hi; out.low[w] = lo; out.volume[w] = v;
    }
    out.resize(w);
    return out;
}

}  // namespace mt
