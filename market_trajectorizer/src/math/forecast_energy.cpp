#include "mt/math/forecast_energy.hpp"
#include <algorithm>
#include <complex>

namespace mt {
Vec<real> forecast_energy_cpu(const Vec<cplx>& dV_E, const Vec<cplx>& dV_X) {
    const usize n = std::min(dV_E.size(), dV_X.size());
    Vec<real> out(n);
    for (usize i = 0; i < n; ++i) {
        cplx s = dV_E[i] + dV_X[i];
        out[i] = std::norm(s);
    }
    return out;
}
}
