#pragma once
#include "mt/state/spectral_state.hpp"
#include "mt/state/recurrence_state.hpp"

namespace mt {

// R(z) = Σ_{h>=0} u_h z^h = P_{r-1}(z) / Q_r(z).
RationalModel build_rational_from_recurrence(const RecurrenceCoefficients& rc,
                                             const Vec<cplx>& u_init /* size r */);

Vec<cplx> eval_rational_coeffs(const RationalModel& rm, int H);
Vec<cplx> eval_resolvent_grid(const RationalModel& rm, const Vec<cplx>& z_grid);
Vec<cplx> eval_resolvent_grid(const RationalModel& rm, const HorizonGrid& hg);

}  // namespace mt
