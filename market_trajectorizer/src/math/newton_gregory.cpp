#include "mt/math/newton_gregory.hpp"
#include "mt/math/combinatorics.hpp"

namespace mt {

real beta_k(int k, real h) {
    real prod = 1.0;
    for (int j = 0; j < k; ++j) prod *= (h + static_cast<real>(j));
    return prod / factorial_int(k);
}

cplx eval_newton_gregory_point(const Vec<cplx>& nabla, real h) {
    cplx acc{};
    for (usize k = 0; k < nabla.size(); ++k) acc += beta_k(static_cast<int>(k), h) * nabla[k];
    return acc;
}

Vec<cplx> eval_newton_gregory_grid(const Vec<cplx>& x, int n, int K, const Vec<real>& h_grid) {
    Vec<cplx> nabla;
    finite_backward_differences(x, n, K - 1, nabla);
    Vec<cplx> out(h_grid.size());
    for (usize i = 0; i < h_grid.size(); ++i) out[i] = eval_newton_gregory_point(nabla, h_grid[i]);
    return out;
}

Vec<cplx> eval_newton_gregory(const Vec<cplx>& u, const HorizonGrid& hg) {
    if (u.empty()) return Vec<cplx>(hg.h.size(), cplx{});
    return eval_newton_gregory_grid(u, static_cast<int>(u.size()) - 1,
                                    static_cast<int>(u.size()), hg.h);
}

}  // namespace mt
