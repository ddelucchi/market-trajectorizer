#include <catch2/catch_test_macros.hpp>

#include "mt/math/combinatorics.hpp"
#include "mt/math/taylor.hpp"

#include <algorithm>

namespace {

mt::real coeff_diff_max(const mt::Vec<mt::cplx>& a, const mt::Vec<mt::cplx>& b) {
    const mt::usize n = std::min(a.size(), b.size());
    mt::real m = 0.0;
    for (mt::usize i = 0; i < n; ++i) m = std::max(m, std::abs(a[i] - b[i]));
    return m;
}

}  // namespace

TEST_CASE("D_i I_i and I_i D_i equal Pi_{eta_i,perp} on dense jets", "[taylor][operators]") {
    mt::JetState J;
    J.Q = 2;
    J.order_max = 4;
    J.coeff.assign(25, mt::cplx{});

    auto layout = mt::make_multi_index_layout(J.Q, J.order_max);
    mt::Vec<mt::Vec<int>> idx;
    mt::enumerate_multi_indices(J.Q, J.order_max, idx);
    for (const auto& alpha : idx) {
        // Keep only interior terms to avoid degree-truncation boundary effects.
        if (mt::multi_degree(alpha) > J.order_max - 2) continue;
        const mt::usize k = mt::flatten_multi_index(alpha, layout);
        J.coeff[k] = mt::cplx(0.1 * static_cast<mt::real>(k + 1), -0.03 * static_cast<mt::real>((k % 5) + 1));
    }

    const mt::real eps = 0.25;
    for (int axis = 0; axis < J.Q; ++axis) {
        const mt::JetState perp = mt::project_perp(J, axis);
        const mt::JetState Jp = mt::project_perp(J, axis);
        const mt::JetState di = mt::apply_D(mt::apply_I(Jp, axis, eps), axis, eps);
        const mt::JetState id = mt::apply_I(mt::apply_D(Jp, axis, eps), axis, eps);

        REQUIRE(coeff_diff_max(di.coeff, perp.coeff) < 1e-10);
        REQUIRE(coeff_diff_max(id.coeff, perp.coeff) < 1e-10);
    }
}
