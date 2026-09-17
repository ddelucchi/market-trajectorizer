// Risk field assembly.
//
// Framework distinction (math_contract §F):
//   Energy carrier  P_{t,V}(s,h) := |⟨dV[E], dV[X]⟩| in the dual pairing.  This
//   is the abstract object; we expose the practical surrogate below.
//
// Practical proxies on the realised interstice trajectory:
//   energy_proxy[h]      = |T(s,h)|^2
//   curvature[h]         = (Re T[h+1] - 2 Re T[h] + Re T[h-1])     (centred Δ²)
//   drawdown_proxy[h]    = max(0, max_{0<=k<=h} Re T[k] - Re T[h])
//   uncertainty_proxy[h] = (Im T[h])^2 + Var(Re T[0..h])
//
// The dual-pairing entry point compute_risk_field is kept for callers that
// already have (dV_E, dV_X) tensor handles.

#include "mt/engine/risk_engine.hpp"

#include <algorithm>
#include <cmath>

namespace mt {

namespace {
real running_var(const Vec<real>& v, usize j) {
    if (j == 0) return 0.0;
    real mean = 0.0;
    for (usize i = 0; i <= j; ++i) mean += v[i];
    mean /= static_cast<real>(j + 1);
    real acc = 0.0;
    for (usize i = 0; i <= j; ++i) acc += (v[i] - mean) * (v[i] - mean);
    return acc / static_cast<real>(j + 1);
}
}  // namespace

RiskField compute_risk_field(const Vec<cplx>& dV_E, const Vec<cplx>& dV_X) {
    RiskField R;
    const usize H = std::min(dV_E.size(), dV_X.size());
    R.energy_framework.assign(H, 0.0);
    R.energy_proxy.assign(H, 0.0);
    R.curvature.assign(H, 0.0);
    R.drawdown_proxy.assign(H, 0.0);
    R.uncertainty_proxy.assign(H, 0.0);
    for (usize i = 0; i < H; ++i) {
        cplx ip = std::conj(dV_E[i]) * dV_X[i];
        R.energy_framework[i] = std::abs(ip);
        R.uncertainty_proxy[i] = std::norm(dV_E[i] - dV_X[i]);
    }
    R.energy_framework_present = (H > 0);
    for (usize i = 1; i + 1 < H; ++i)
        R.curvature[i] = R.energy_framework[i + 1] - 2.0 * R.energy_framework[i] + R.energy_framework[i - 1];
    real peak = (H > 0) ? R.energy_framework[0] : 0.0;
    for (usize i = 0; i < H; ++i) {
        peak = std::max(peak, R.energy_framework[i]);
        R.drawdown_proxy[i] = std::max(0.0, peak - R.energy_framework[i]);
    }
    return R;
}

RiskField build_risk_field_from_trajectory(const IntersticeTrajectory& tr) {
    RiskField R;
    const usize H = tr.T.size();
    R.energy_framework.assign(H, 0.0);
    R.energy_proxy.assign(H, 0.0);
    R.curvature.assign(H, 0.0);
    R.drawdown_proxy.assign(H, 0.0);
    R.uncertainty_proxy.assign(H, 0.0);

    // Theorem-side energy carrier MUST come from tr.Risk (the framework
    // dual-pairing energy carried by the trajectorizer).  Production code
    // MUST NOT silently fall back to std::norm(tr.T[i]) — that is proxy
    // energy and is research-only.  We mark the framework path absent if the
    // theorem-side carrier is not provided, and surface that to gates so the
    // signal/risk layer can refuse the anchor.
    R.energy_framework_present = (tr.Risk.size() >= H) && (H > 0);
    if (!R.energy_framework_present) {
        if (H == 0) {
            R.energy_framework_missing_reason = "trajectory_horizon_empty";
        } else if (tr.Risk.empty()) {
            R.energy_framework_missing_reason = "trajectory_risk_carrier_absent";
        } else {
            R.energy_framework_missing_reason = "trajectory_risk_carrier_too_short";
        }
    } else {
        R.energy_framework_missing_reason.clear();
    }

    Vec<real> reT(H);
    for (usize i = 0; i < H; ++i) {
        reT[i] = tr.T[i].real();
        if (R.energy_framework_present) {
            R.energy_framework[i] = std::max(0.0, tr.Risk[i]);
        } else {
            R.energy_framework[i] = 0.0;   // explicit absence; do NOT fall back.
        }
        R.energy_proxy[i] = std::norm(tr.T[i]);   // research-only practical proxy.
    }
    for (usize i = 1; i + 1 < H; ++i)
        R.curvature[i] = reT[i + 1] - 2.0 * reT[i] + reT[i - 1];
    real peak = H > 0 ? reT[0] : 0.0;
    for (usize i = 0; i < H; ++i) {
        peak = std::max(peak, reT[i]);
        R.drawdown_proxy[i] = std::max(0.0, peak - reT[i]);
        const real im = tr.T[i].imag();
        R.uncertainty_proxy[i] = im * im + running_var(reT, i);
    }
    return R;
}

}  // namespace mt
