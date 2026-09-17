// Taylor-jet operators on the dense (order_max+1)^Q multi-index lattice.
//
// Identities enforced:
//      D_i I_i  =  Π_{η_i,⊥}
//      I_i D_i  =  Π_{η_i,⊥}
//
// Coefficient-wise on J(η) = Σ_α c_α η^α :
//     (D_i J)(η)        = ε_m^{-1} Σ_{α: α_i ≥ 1} α_i c_α η^{α-e_i}
//     (I_i J)(η)        = ε_m     Σ_{α            } c_α / (α_i + 1) η^{α+e_i}
//     (Π_{η_i,⊥} J)(η) =          Σ_{α: α_i > 0} c_α η^α
//
// Storage layout: dense (order_max+1)^Q.  See math/combinatorics.cpp for the
// stride convention used by `make_multi_index_layout`.

#include "mt/math/taylor.hpp"
#include "mt/math/combinatorics.hpp"
#include "mt/core/errors.hpp"

#include <algorithm>

namespace mt {

namespace {
inline usize dense_size(int Q, int order_max) {
    usize n = 1;
    for (int q = 0; q < Q; ++q) n *= static_cast<usize>(order_max + 1);
    return n;
}
}  // namespace

JetState apply_D(const JetState& J, int i, real eps_m) {
    if (i < 0 || i >= J.Q) throw NumericError("apply_D: axis out of range");
    if (eps_m == 0.0)      throw NumericError("apply_D: eps_m must be nonzero");
    JetState R; R.Q = J.Q; R.order_max = J.order_max;
    const usize N = dense_size(J.Q, J.order_max);
    R.coeff.assign(N, cplx{});
    if (J.coeff.size() < N) return R;

    auto layout = make_multi_index_layout(J.Q, J.order_max);
    Vec<Vec<int>> idx; enumerate_multi_indices(J.Q, J.order_max, idx);
    const real inv = 1.0 / eps_m;

    for (const auto& alpha : idx) {
        const int ai = alpha[static_cast<usize>(i)];
        if (ai < 1) continue;
        Vec<int> beta = alpha;
        beta[static_cast<usize>(i)] = ai - 1;
        const usize src = flatten_multi_index(alpha, layout);
        const usize dst = flatten_multi_index(beta,  layout);
        R.coeff[dst] += inv * static_cast<real>(ai) * J.coeff[src];
    }
    return R;
}

JetState apply_I(const JetState& J, int i, real eps_m) {
    if (i < 0 || i >= J.Q) throw NumericError("apply_I: axis out of range");
    JetState R; R.Q = J.Q; R.order_max = J.order_max;
    const usize N = dense_size(J.Q, J.order_max);
    R.coeff.assign(N, cplx{});
    if (J.coeff.size() < N) return R;

    auto layout = make_multi_index_layout(J.Q, J.order_max);
    Vec<Vec<int>> idx; enumerate_multi_indices(J.Q, J.order_max, idx);

    for (const auto& alpha : idx) {
        const int ai = alpha[static_cast<usize>(i)];
        // raised index has α_i + 1; must remain within order_max
        if (multi_degree(alpha) + 1 > J.order_max) continue;
        Vec<int> beta = alpha;
        beta[static_cast<usize>(i)] = ai + 1;
        const usize src = flatten_multi_index(alpha, layout);
        const usize dst = flatten_multi_index(beta,  layout);
        R.coeff[dst] += (eps_m / static_cast<real>(ai + 1)) * J.coeff[src];
    }
    return R;
}

JetState project_perp(const JetState& J, int i) {
    if (i < 0 || i >= J.Q) throw NumericError("project_perp: axis out of range");
    JetState R = J;
    const usize N = dense_size(J.Q, J.order_max);
    if (R.coeff.size() < N) R.coeff.resize(N, cplx{});

    auto layout = make_multi_index_layout(J.Q, J.order_max);
    Vec<Vec<int>> idx; enumerate_multi_indices(J.Q, J.order_max, idx);
    for (const auto& alpha : idx) {
        if (alpha[static_cast<usize>(i)] == 0) {
            const usize k = flatten_multi_index(alpha, layout);
            R.coeff[k] = cplx{};
        }
    }
    return R;
}

}  // namespace mt
