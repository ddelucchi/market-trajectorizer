cmake_minimum_required(VERSION 3.27)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

set(MT_TEST_SOURCES
    unit/test_combinatorics.cpp
    unit/test_finite_differences.cpp
    unit/test_newton_gregory.cpp
    unit/test_companion.cpp
    unit/test_resolvent.cpp
    unit/test_evolution_equality.cpp
    unit/test_phase_torus_contract.cpp
    unit/test_taylor_contract.cpp
    unit/test_closure_reconstruction.cpp
    unit/test_interstice_contract.cpp
    unit/test_causality_and_determinism.cpp
    unit/test_signal_policy_gates.cpp
    unit/test_symbol_calendar_metadata.cpp
    unit/test_theorem_authority_harness.cpp
    unit/test_canonicalizer.cpp
    unit/test_metrics.cpp
    unit/test_position_sizer.cpp
    unit/test_framework_anchor_q5.cpp
    unit/test_future_section_identity_on_actual_anchor.cpp
    unit/test_authority_rejects_scalar_fallback.cpp
    unit/test_authority_rejects_synthetic_event_boundary.cpp
    unit/test_nontrivial_closure_gate.cpp
    unit/test_framework_energy_grid_aggregation.cpp
    unit/test_theorem_path_bans_legacy_alias.cpp
    unit/test_theorem_scope_limit.cpp
)

add_executable(mt_unit_tests ${MT_TEST_SOURCES})
target_link_libraries(mt_unit_tests PRIVATE mt_core Catch2::Catch2WithMain)
target_compile_features(mt_unit_tests PRIVATE cxx_std_20)

add_executable(mt_prepare_authority_fixture
    tools/prepare_authority_fixture.cpp
)
target_link_libraries(mt_prepare_authority_fixture PRIVATE mt_core)
target_compile_features(mt_prepare_authority_fixture PRIVATE cxx_std_20)

include(CTest)
include(Catch)
catch_discover_tests(mt_unit_tests)

set(MT_AUTHORITY_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/authority_fixture")
set(MT_AUTHORITY_MANIFEST "${MT_AUTHORITY_FIXTURE_DIR}/symbol_store.json")
set(MT_AUTHORITY_SYMBOL "AUTH_FIX")

add_test(
    NAME theorem_authority_prepare_fixture
        COMMAND $<TARGET_FILE:mt_prepare_authority_fixture>
            --in ${CMAKE_SOURCE_DIR}/tests/fixtures/authority_fixture.csv
            --out_dir ${MT_AUTHORITY_FIXTURE_DIR}
            --manifest ${MT_AUTHORITY_MANIFEST}
            --symbol ${MT_AUTHORITY_SYMBOL}
            --timeframe 1d
)
set_tests_properties(theorem_authority_prepare_fixture PROPERTIES
    FIXTURES_SETUP theorem_authority_fixture
    LABELS "theorem_authority"
)

add_test(
        NAME fixture_scaffold_gate
    COMMAND $<TARGET_FILE:mt_verify_authority>
      --manifest ${MT_AUTHORITY_MANIFEST}
            --engine ${CMAKE_SOURCE_DIR}/tests/fixtures/authority_engine_gate.json
            --gate_level fixture_scaffold_gate
      --symbol ${MT_AUTHORITY_SYMBOL}
      --last_only
      --max_failures 1
)
set_tests_properties(fixture_scaffold_gate PROPERTIES
    FIXTURES_REQUIRED theorem_authority_fixture
    LABELS "theorem_authority"
    WILL_FAIL TRUE
)

add_test(
        NAME symbol_fixture_exactness_gate
        COMMAND $<TARGET_FILE:mt_verify_authority>
            --manifest ${MT_AUTHORITY_MANIFEST}
            --engine ${CMAKE_SOURCE_DIR}/tests/fixtures/authority_engine_symbol_exactness.json
            --gate_level symbol_fixture_exactness_gate
            --symbol ${MT_AUTHORITY_SYMBOL}
            --last_only
            --max_failures 1
)
set_tests_properties(symbol_fixture_exactness_gate PROPERTIES
        FIXTURES_REQUIRED theorem_authority_fixture
        LABELS "theorem_authority"
        WILL_FAIL TRUE
)

add_test(
        NAME production_dataset_authority_gate
        COMMAND $<TARGET_FILE:mt_verify_authority>
            --manifest ${MT_AUTHORITY_MANIFEST}
            --engine ${CMAKE_SOURCE_DIR}/configs/engine/default.json
            --gate_level production_dataset_authority_gate
            --production
            --symbol ${MT_AUTHORITY_SYMBOL}
            --last_only
            --max_failures 1
)
set_tests_properties(production_dataset_authority_gate PROPERTIES
        FIXTURES_REQUIRED theorem_authority_fixture
        LABELS "theorem_authority"
        WILL_FAIL TRUE
)
