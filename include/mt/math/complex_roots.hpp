#pragma once
#include "mt/core/types.hpp"

namespace mt {

// Aberth-Ehrlich complex root finder; deterministic with fixed initial seed lattice.
Vec<cplx> find_polynomial_roots(const Vec<cplx>& coeffs, real tol, int max_iter);

}  // namespace mt
