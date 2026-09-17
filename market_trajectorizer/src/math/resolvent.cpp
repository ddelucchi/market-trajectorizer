// Resolvent / rational model.
//
//      U(z)      =  Σ_{n>=0} u_n z^n  =  P(z) / Q(z)
//      Q(z)      =  1 + Σ_{j=1}^{r} q_j z^j           (Q[0] = 1)
//      P(z)      =  Q(z) U(z)  mod  z^r
//      P_n       =  u_n + Σ_{j=1}^{n} q_j u_{n-j}     for 0 <= n <= r-1
//
// On input rc.c[0..r-1] holds (c_0, c_1, ..., c_{r-1}) such that
//      u_{n+r} + c_{r-1} u_{n+r-1} + ... + c_0 u_n = 0,
// i.e.  q_j = c_{r-j}  for j=1..r.

#include "mt/math/resolvent.hpp"
#include "mt/math/polynomial.hpp"
#include "mt/core/errors.hpp"

#include <algorithm>

namespace mt {

RationalModel build_rational_from_recurrence(const RecurrenceCoefficients& rc,
                                             const Vec<cplx>& u_init)
{
    RationalModel rm;
    const int r = rc.r;
    rm.Q.assign(static_cast<usize>(r + 1), cplx{});
    rm.Q[0] = cplx(1.0, 0.0);
    for (int j = 1; j <= r; ++j) rm.Q[static_cast<usize>(j)] = rc.c[static_cast<usize>(r - j)];

    // P_n = u_n + Σ_{j=1}^{n} q_j u_{n-j}, n = 0..r-1
    rm.P.assign(static_cast<usize>(r), cplx{});
    if (static_cast<int>(u_init.size()) < r)
        throw NumericError("build_rational_from_recurrence: need r initial samples");
    for (int n = 0; n < r; ++n) {
        cplx pn = u_init[static_cast<usize>(n)];
        for (int j = 1; j <= n; ++j)
            pn += rm.Q[static_cast<usize>(j)] * u_init[static_cast<usize>(n - j)];
        rm.P[static_cast<usize>(n)] = pn;
    }
    return rm;
}

Vec<cplx> eval_rational_coeffs(const RationalModel& rm, int H) {
    Vec<cplx> u(static_cast<usize>(H + 1), cplx{});
    if (rm.Q.empty() || rm.Q[0] == cplx{}) return u;
    for (int n = 0; n <= H; ++n) {
        cplx v = (n < static_cast<int>(rm.P.size())) ? rm.P[static_cast<usize>(n)] : cplx{};
        for (usize k = 1; k < rm.Q.size() && static_cast<usize>(n) >= k; ++k)
            v -= rm.Q[k] * u[static_cast<usize>(n) - k];
        u[static_cast<usize>(n)] = v / rm.Q[0];
    }
    return u;
}

Vec<cplx> eval_resolvent_grid(const RationalModel& rm, const Vec<cplx>& z_grid) {
    Vec<cplx> out(z_grid.size());
    for (usize i = 0; i < z_grid.size(); ++i)
        out[i] = poly_eval(rm.P, z_grid[i]) / poly_eval(rm.Q, z_grid[i]);
    return out;
}

Vec<cplx> eval_resolvent_grid(const RationalModel& rm, const HorizonGrid& hg) {
    int H = 0;
    for (real h : hg.h) H = std::max(H, static_cast<int>(h));
    auto u = eval_rational_coeffs(rm, H);
    Vec<cplx> out(hg.h.size());
    for (usize i = 0; i < hg.h.size(); ++i) {
        int idx = static_cast<int>(hg.h[i]);
        out[i] = (idx >= 0 && idx < static_cast<int>(u.size())) ? u[static_cast<usize>(idx)] : cplx{};
    }
    return out;
}

}  // namespace mt
