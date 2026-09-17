#pragma once
#include "mt/state/spectral_state.hpp"

namespace mt {

// Returns max |λ_j| spectral radius; > 1 + tol => extrapolation flagged unstable.
real spectral_radius(const SpectralDecomposition& sd);
bool is_stable(const SpectralDecomposition& sd, real tol);

}  // namespace mt
