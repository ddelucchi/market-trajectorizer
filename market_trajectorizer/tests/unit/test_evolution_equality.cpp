#include <catch2/catch_test_macros.hpp>
#include "mt/engine/trajectorizer_engine.hpp"

TEST_CASE("evolution equality enforces theorem carriers and closure corollaries", "[evolution]") {
    mt::EvolutionEquality ee;

    ee.U_continuous = {mt::cplx{1.0, 0.0}, mt::cplx{2.0, 0.0}};
    ee.U_jet        = ee.U_continuous;
    ee.U_newton     = ee.U_continuous;
    ee.U_matrix     = ee.U_continuous;

    SECTION("theorem carriers pass below tolerance") {
        ee.err_continuous_jet = 1e-10;
        ee.err_jet_newton = 1e-10;
        ee.err_newton_matrix = 1e-10;
        ee.closed_regime = false;
        REQUIRE(mt::assert_evolution_equality(ee, 1e-8));
    }

    SECTION("theorem carrier mismatch fails") {
        ee.err_continuous_jet = 1e-3;
        ee.err_jet_newton = 0.0;
        ee.err_newton_matrix = 0.0;
        ee.closed_regime = false;
        REQUIRE_FALSE(mt::assert_evolution_equality(ee, 1e-8));
    }

    SECTION("closure corollaries are mandatory when closed_regime is true") {
        ee.closed_regime = true;
        ee.err_continuous_jet = 1e-10;
        ee.err_jet_newton = 1e-10;
        ee.err_newton_matrix = 1e-10;
        ee.err_matrix_rational = 1e-6;
        ee.err_rational_spectral = 1e-10;
        REQUIRE_FALSE(mt::assert_evolution_equality(ee, 1e-8));

        ee.err_matrix_rational = 1e-10;
        REQUIRE(mt::assert_evolution_equality(ee, 1e-8));
    }
}
