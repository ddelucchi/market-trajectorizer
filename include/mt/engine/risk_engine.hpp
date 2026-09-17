#pragma once
#include "mt/math/interstice.hpp"
#include "mt/state/risk_state.hpp"

namespace mt {

RiskField compute_risk_field(const Vec<cplx>& dV_E, const Vec<cplx>& dV_X);
RiskField build_risk_field_from_trajectory(const IntersticeTrajectory& tr);

}  // namespace mt
