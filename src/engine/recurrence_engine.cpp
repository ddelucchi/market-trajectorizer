#include "mt/engine/recurrence_engine.hpp"
#include "mt/math/companion.hpp"
#include "mt/math/hankel.hpp"
#include "mt/math/resolvent.hpp"
#include "mt/math/spectral.hpp"

namespace mt {

RecurrencePipelineOut build_recurrence_pipeline(const Vec<cplx>& u, int N_max, real tol) {
    RecurrencePipelineOut out;
    try {
        int r = detect_min_rank(u, N_max, tol);
        out.rec      = solve_recurrence_from_hankel(u, r);
        Vec<cplx> u_init(u.begin(), u.begin() + std::min<usize>(static_cast<usize>(r), u.size()));
        out.comp     = build_companion_state(out.rec, u_init);
        out.rational = build_rational_from_recurrence(out.rec, u_init);
        out.spectrum = factor_rational_model(out.rational, tol);
    } catch (...) {
        out = RecurrencePipelineOut{};
    }
    return out;
}

}  // namespace mt
