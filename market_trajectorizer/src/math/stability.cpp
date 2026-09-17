#include "mt/math/stability.hpp"
#include <algorithm>
#include <complex>

namespace mt {
real spectral_radius(const SpectralDecomposition& sd) {
    real r = 0.0;
    for (const auto& m : sd.modes) r = std::max(r, std::abs(m.lambda));
    return r;
}
bool is_stable(const SpectralDecomposition& sd, real tol) {
    return spectral_radius(sd) <= 1.0 + tol;
}
}
