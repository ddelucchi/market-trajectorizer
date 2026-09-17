// Truncated jet semigroup e^{h L_jet,κ} J  on the dense (order_max+1)^Q layout.
//
// On the canonical 1-D axial jet J(η) = Σ_{n=0}^{N-1} c_n η^n, the semigroup
// acts by Taylor shift:   (e^{h L} J)(η) = J(η + h),   so the new constant
// term is J(h) = Σ c_n h^n.  More generally on the dense lattice we apply
// translation along each axis using the binomial law
//      (T_h J)_β = Σ_{α >= β} C(α, β) h^{α-β} c_α,
// implemented as a deterministic triangular sweep.  This is exact in
// arithmetic and identity-preserving when L is the canonical generator.
//
// The `JetOperator L` argument is reserved for future use (custom generator);
// when L.dim == 0 the canonical translation is used.

#include "mt/math/semigroup.hpp"
#include "mt/math/combinatorics.hpp"

namespace mt {

namespace {
inline usize dense_size(int Q, int order_max) {
    usize n = 1;
    for (int q = 0; q < Q; ++q) n *= static_cast<usize>(order_max + 1);
    return n;
}
}  // namespace

JetState shift_jet_semigroup_cpu(const JetState& J, real h, const JetOperator& /*L*/) {
    JetState R; R.Q = J.Q; R.order_max = J.order_max;
    const usize N = dense_size(J.Q, J.order_max);
    R.coeff.assign(N, cplx{});
    if (J.coeff.size() < N) {
        // permissive path: fallback to a 1-D Taylor sum if storage is packed.
        if (J.Q == 1 && J.coeff.size() == static_cast<usize>(J.order_max + 1)) {
            cplx acc{}; cplx hp(1.0, 0.0);
            for (usize n = 0; n < J.coeff.size(); ++n) {
                acc += J.coeff[n] * hp;
                hp *= cplx(h, 0.0);
            }
            R.coeff.assign(J.coeff.size(), cplx{});
            R.coeff[0] = acc;
            return R;
        }
        return R;
    }

    auto layout = make_multi_index_layout(J.Q, J.order_max);
    Vec<Vec<int>> idx; enumerate_multi_indices(J.Q, J.order_max, idx);

    // (T_h J)_β = Σ_{α >= β} C(α-β + β, β) ... using product over axes:
    //   for each α, distribute c_α into all β <= α with weight Π_q C(α_q, β_q) h^{α_q-β_q}.
    for (const auto& alpha : idx) {
        const usize sa = flatten_multi_index(alpha, layout);
        const cplx ca = J.coeff[sa];
        if (ca == cplx{}) continue;
        // enumerate β in [0, α] elementwise
        Vec<int> beta(static_cast<usize>(J.Q), 0);
        while (true) {
            real w = 1.0;
            int  delta_total = 0;
            for (int q = 0; q < J.Q; ++q) {
                int aq = alpha[static_cast<usize>(q)];
                int bq = beta [static_cast<usize>(q)];
                w *= binom(aq, bq);
                delta_total += (aq - bq);
            }
            cplx hp = std::pow(cplx(h, 0.0), static_cast<real>(delta_total));
            R.coeff[flatten_multi_index(beta, layout)] += w * hp * ca;

            // increment β in [0, α] lex order
            int q = J.Q - 1;
            while (q >= 0) {
                if (beta[static_cast<usize>(q)] < alpha[static_cast<usize>(q)]) {
                    ++beta[static_cast<usize>(q)];
                    for (int qq = q + 1; qq < J.Q; ++qq) beta[static_cast<usize>(qq)] = 0;
                    break;
                }
                --q;
            }
            if (q < 0) break;
        }
    }
    return R;
}

}  // namespace mt
