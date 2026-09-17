#pragma once
#include "mt/core/types.hpp"

namespace mt {

cplx poly_eval(const Vec<cplx>& coeffs, cplx z);
Vec<cplx> poly_mul(const Vec<cplx>& a, const Vec<cplx>& b);
Vec<cplx> poly_add(const Vec<cplx>& a, const Vec<cplx>& b);

}  // namespace mt

