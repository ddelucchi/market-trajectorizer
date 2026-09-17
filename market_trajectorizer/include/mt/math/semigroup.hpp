#pragma once
#include "mt/state/jet_state.hpp"

namespace mt {

// e^{h L_jet,κ} J  (truncated jet semigroup).
JetState shift_jet_semigroup_cpu(const JetState& J, real h, const JetOperator& L);

}  // namespace mt
