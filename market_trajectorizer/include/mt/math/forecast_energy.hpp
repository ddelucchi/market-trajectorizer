#pragma once
#include "mt/core/types.hpp"

namespace mt {

// P_{t,V}^{(N)}(τ) = | ∂_V E + ∂_V X^{(N)} |^2.
Vec<real> forecast_energy_cpu(const Vec<cplx>& dV_E, const Vec<cplx>& dV_X);

}  // namespace mt
