#pragma once
#include "mt/state/extracted_coeffs.hpp"
#include "mt/state/jet_state.hpp"

namespace mt {

JetState make_jet_from_extracted(const ExtractedCoefficients& A, real rho_m0);
cplx     evaluate_jet(const JetState& J, const Vec<real>& eta_real);

}  // namespace mt
