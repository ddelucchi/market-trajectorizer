#include <catch2/catch_test_macros.hpp>

#include "mt/math/interstice.hpp"

#include <algorithm>
#include <limits>

namespace {

mt::real max_interstice_pde_residual(const mt::IntersticeTrajectory& tr) {
    if (tr.s_count < 3 || tr.H < 3) return std::numeric_limits<mt::real>::infinity();

    const mt::usize S = static_cast<mt::usize>(tr.s_count);
    const mt::usize H = static_cast<mt::usize>(tr.H);
    if (tr.T_grid.size() < S * H || tr.s_grid.size() < S || tr.h_grid.size() < H || tr.kappa.size() < S)
        return std::numeric_limits<mt::real>::infinity();

    auto at = [&](int s, int h) -> const mt::cplx& {
        return tr.T_grid[static_cast<mt::usize>(s) * H + static_cast<mt::usize>(h)];
    };

    mt::real m = 0.0;
    for (int si = 1; si + 1 < tr.s_count; ++si) {
        const mt::real ds = tr.s_grid[static_cast<mt::usize>(si + 1)] - tr.s_grid[static_cast<mt::usize>(si - 1)];
        for (int hi = 1; hi + 1 < tr.H; ++hi) {
            const mt::real dh = tr.h_grid[static_cast<mt::usize>(hi + 1)] - tr.h_grid[static_cast<mt::usize>(hi - 1)];
            const mt::cplx dTds = (at(si + 1, hi) - at(si - 1, hi)) / ds;
            const mt::cplx dTdh = (at(si, hi + 1) - at(si, hi - 1)) / dh;
            const mt::cplx rhs = (1.0 + tr.kappa[static_cast<mt::usize>(si)] * tr.h_grid[static_cast<mt::usize>(hi)]) * dTdh;
            m = std::max(m, std::abs(dTds - rhs));
        }
    }
    return m;
}

mt::real max_interstice_recurrence_residual(const mt::IntersticeTrajectory& tr) {
    // Constant mode λ=1 implies u_{n+1} - u_n = 0 at each s.
    if (tr.H < 2 || tr.s_count < 1) return std::numeric_limits<mt::real>::infinity();

    const mt::usize H = static_cast<mt::usize>(tr.H);
    const mt::usize S = static_cast<mt::usize>(tr.s_count);
    mt::real m = 0.0;
    for (mt::usize s = 0; s < S; ++s) {
        for (int n = 0; n + 1 < tr.H; ++n) {
            const mt::cplx curr = tr.T_grid[s * H + static_cast<mt::usize>(n)];
            const mt::cplx next = tr.T_grid[s * H + static_cast<mt::usize>(n + 1)];
            m = std::max(m, std::abs(next - curr));
        }
    }
    return m;
}

}  // namespace

TEST_CASE("interstice transport satisfies PDE and recurrence on constant transported mode", "[interstice][transport]") {
    mt::SpectralDecomposition sd;
    sd.r = 1;
    mt::SpectralMode mode;
    mode.lambda = mt::cplx{1.0, 0.0};
    mode.multiplicity = 1;
    mode.gamma = {mt::cplx{1.0, 0.0}};
    sd.modes.push_back(mode);

    mt::IntersticeDynamics dyn = mt::build_interstice_dynamics(/*s_count=*/8, /*ds=*/0.25, /*u_imag=*/0.0);

    mt::HorizonGrid hg;
    hg.h = {0.0, 1.0, 2.0, 3.0, 4.0};

    mt::IntersticeTrajectory tr = mt::build_interstice_trajectory(sd, dyn, /*s_index=*/0, hg);

    REQUIRE(max_interstice_pde_residual(tr) < 1e-10);
    REQUIRE(max_interstice_recurrence_residual(tr) < 1e-10);
}
