#pragma once
#include "mt/state/spectral_state.hpp"

namespace mt {

SpectralDecomposition factor_rational_model(const RationalModel& rm, real tol);

// u_h = Σ_j Σ_{s=0}^{μ_j-1} γ_{j,s} C(h,s) λ_j^{h-s}.
Vec<cplx> eval_spectral_modes(const SpectralDecomposition& sd, int H);
Vec<cplx> eval_spectral_modes(const SpectralDecomposition& sd, const HorizonGrid& hg);

}  // namespace mt
