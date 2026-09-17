// Deterministic complex root finder via Durand-Kerner (Weierstrass) iteration.
//
// Polynomial passed in monomial form, low-degree first:
//      p(z) = c[0] + c[1] z + c[2] z^2 + ... + c[d] z^d.
// Initial seeds: regularly spaced on a circle at the geometric-mean root
// magnitude bound R = (|c[0]/c[d]|)^{1/d}, with a fixed phase offset
// φ_0 = 0.4 to avoid eigenvalue clusters near the real axis.

#include "mt/math/complex_roots.hpp"
#include "mt/core/constants.hpp"

#include <algorithm>
#include <cmath>

namespace mt {

namespace {

cplx horner(const Vec<cplx>& c, cplx z) {
    cplx v{};
    for (int i = static_cast<int>(c.size()) - 1; i >= 0; --i) v = v * z + c[static_cast<usize>(i)];
    return v;
}

}  // namespace

Vec<cplx> find_polynomial_roots(const Vec<cplx>& coeffs, real tol, int max_iter) {
    Vec<cplx> c = coeffs;
    while (c.size() > 1 && c.back() == cplx{}) c.pop_back();
    const int d = static_cast<int>(c.size()) - 1;
    if (d <= 0) return Vec<cplx>{};

    // Normalize to monic.
    cplx lead = c.back();
    for (auto& v : c) v /= lead;

    // Magnitude bound.
    real R = std::pow(std::max(std::abs(c.front()), 1e-300), 1.0 / static_cast<real>(d));
    if (!(R > 0.0)) R = 1.0;
    R = std::max(R, 1.0);

    Vec<cplx> roots(static_cast<usize>(d));
    const real phi0 = 0.4;
    for (int j = 0; j < d; ++j) {
        real theta = phi0 + static_cast<real>(j) * (constants::TWO_PI / static_cast<real>(d));
        roots[static_cast<usize>(j)] = R * cplx(std::cos(theta), std::sin(theta));
    }

    for (int it = 0; it < max_iter; ++it) {
        real max_corr = 0.0;
        for (int j = 0; j < d; ++j) {
            cplx z = roots[static_cast<usize>(j)];
            cplx denom(1.0, 0.0);
            for (int k = 0; k < d; ++k) if (k != j) denom *= (z - roots[static_cast<usize>(k)]);
            cplx num = horner(c, z);
            if (denom == cplx{}) continue;
            cplx delta = num / denom;
            roots[static_cast<usize>(j)] = z - delta;
            real m = std::abs(delta);
            if (m > max_corr) max_corr = m;
        }
        if (max_corr < tol) break;
    }
    return roots;
}

}  // namespace mt
