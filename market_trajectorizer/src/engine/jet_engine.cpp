// J_α[F] = ρ_m^{-|α|} · A^{(m)}_{F,α}  =  (1/α!) ∂^α F.
//
// Storage convention: dense (order_max+1)^Q lattice, flattened with
// `make_multi_index_layout` (see math/combinatorics.cpp).  Both
// ExtractedCoefficients::A and JetState::coeff use this same layout.

#include "mt/engine/jet_engine.hpp"
#include "mt/math/combinatorics.hpp"
#include "mt/core/errors.hpp"

#include <cmath>

namespace mt {

namespace {
inline usize dense_size(int Q, int order_max) {
    usize n = 1;
    for (int q = 0; q < Q; ++q) n *= static_cast<usize>(order_max + 1);
    return n;
}
}  // namespace

JetState make_jet_from_extracted(const ExtractedCoefficients& A, real /*rho_m0*/) {
    JetState jet; jet.Q = A.Q; jet.order_max = A.order_max;
    const usize N = dense_size(A.Q, A.order_max);
    jet.coeff.assign(N, cplx{});
    if (A.A.size() < N) throw NumericError("make_jet_from_extracted: A.A size != dense layout");
    if (A.rho_m <= 0.0) throw NumericError("make_jet_from_extracted: rho_m must be > 0");

    auto layout = make_multi_index_layout(A.Q, A.order_max);
    Vec<Vec<int>> idx; enumerate_multi_indices(A.Q, A.order_max, idx);

    // J_α = ρ_m^{-|α|} · A^{(m)}_{F,α}.  No additional factorial.
    for (const auto& alpha : idx) {
        const usize k = flatten_multi_index(alpha, layout);
        const int   d = multi_degree(alpha);
        jet.coeff[k] = A.A[k] * std::pow(A.rho_m, -static_cast<real>(d));
    }
    return jet;
}

cplx evaluate_jet(const JetState& J, const Vec<real>& eta) {
    auto layout = make_multi_index_layout(J.Q, J.order_max);
    Vec<Vec<int>> idx; enumerate_multi_indices(J.Q, J.order_max, idx);
    cplx acc{};
    for (const auto& alpha : idx) {
        const usize k = flatten_multi_index(alpha, layout);
        if (k >= J.coeff.size()) continue;
        cplx term = J.coeff[k];   // already contains the 1/α! from normalization
        for (int q = 0; q < J.Q; ++q) {
            int a = alpha[static_cast<usize>(q)];
            for (int j = 0; j < a; ++j) term *= eta[static_cast<usize>(q)];
        }
        acc += term;
    }
    return acc;
}

}  // namespace mt
