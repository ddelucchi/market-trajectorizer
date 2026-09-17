#pragma once
#include "mt/state/spectral_state.hpp"

namespace mt {

// γ_{j,μ_j-1} via leading-order residue at z = Λ_j^{-1}; lower γ_{j,s} via successive subtraction.
Vec<cplx> compute_residue_coefficients(const RationalModel& rm,
                                       cplx lambda_j, int mu_j);

}  // namespace mt
