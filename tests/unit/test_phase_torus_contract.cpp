#include <catch2/catch_test_macros.hpp>

#include "mt/math/combinatorics.hpp"
#include "mt/math/phase_torus.hpp"

#include <cmath>

TEST_CASE("phase torus extraction satisfies jet normalization on analytic toy field", "[phase_torus][jet_normalization]") {
    const mt::cplx c0(0.7, -0.2);
    const mt::cplx c1(1.3, 0.1);
    const mt::cplx c2(-0.4, 0.3);
    const mt::real rho = 0.05;

    mt::FieldCpu F = [=](const mt::Vec<mt::real>& re, const mt::Vec<mt::real>& im) {
        mt::cplx z(re[0], im[0]);
        return c0 + c1 * z + c2 * z * z;
    };

    mt::Vec<mt::real> z_re{0.2};
    mt::Vec<mt::real> z_im{-0.1};

    mt::ExtractedCoefficients ec = mt::extract_phase_torus_cpu(F, z_re, z_im, rho,
                                                                /*Q=*/1,
                                                                /*order_max=*/2,
                                                                /*M_per_dim=*/64);

    auto layout = mt::make_multi_index_layout(1, 2);
    const mt::usize i0 = mt::flatten_multi_index(mt::Vec<int>{0}, layout);
    const mt::usize i1 = mt::flatten_multi_index(mt::Vec<int>{1}, layout);
    const mt::usize i2 = mt::flatten_multi_index(mt::Vec<int>{2}, layout);

    const mt::cplx z(z_re[0], z_im[0]);
    const mt::cplx expected_j0 = c0 + c1 * z + c2 * z * z;
    const mt::cplx expected_j1 = c1 + mt::cplx(2.0, 0.0) * c2 * z;
    const mt::cplx expected_j2 = c2;

    const mt::cplx j0 = ec.A[i0];
    const mt::cplx j1 = ec.A[i1] / rho;
    const mt::cplx j2 = ec.A[i2] / (rho * rho);

    REQUIRE(std::abs(j0 - expected_j0) < 1e-6);
    REQUIRE(std::abs(j1 - expected_j1) < 1e-6);
    REQUIRE(std::abs(j2 - expected_j2) < 1e-6);
}
