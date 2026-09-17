#pragma once
#include "mt/state/recurrence_state.hpp"

namespace mt {

RecurrenceCoefficients solve_recurrence_from_hankel(const Vec<cplx>& u, int r);

CompanionState build_companion_state(const RecurrenceCoefficients& rc,
                                     const Vec<cplx>& u_init /* size r */);

cplx eval_companion(const CompanionState& cs, int h);
Vec<cplx> eval_companion(const CompanionState& cs, const HorizonGrid& hg);

}  // namespace mt
