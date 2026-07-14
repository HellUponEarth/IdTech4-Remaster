include_guard(GLOBAL)

set(IDTECH4_TESTING_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

include("${CMAKE_CURRENT_LIST_DIR}/IdTech4BuildPerformance.cmake")
include(CTest)

option(IDTECH4_BUILD_TESTS "Build GoogleTest/Catch2 unit tests and CTest integration checks." OFF)

function(idtech4_configure_test_target target)
    target_compile_features(${target} PRIVATE cxx_std_14)
    set_target_properties(${target} PROPERTIES IDTECH4_SKIP_PRECOMPILED_HEADERS ON)
    idtech4_apply_compiler_profile(${target})
    idtech4_apply_build_performance(${target})
    idtech4_apply_coverage(${target})
    set_target_properties(
        ${target} PROPERTIES FOLDER tests RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/$<CONFIG>"
    )
endfunction()

function(idtech4_add_tests)
    if(NOT BUILD_TESTING OR NOT IDTECH4_BUILD_TESTS)
        return()
    endif()

    idtech4_provide_test_dependencies()

    add_executable(idtech4_gtest_unit_tests tests/unit/gtest_build_contract_tests.cpp)
    target_link_libraries(idtech4_gtest_unit_tests PRIVATE GTest::gtest_main)
    idtech4_configure_test_target(idtech4_gtest_unit_tests)

    include(GoogleTest)
    gtest_discover_tests(
        idtech4_gtest_unit_tests
        TEST_PREFIX "unit.gtest."
        PROPERTIES LABELS unit
    )

    add_executable(idtech4_catch2_unit_tests tests/unit/catch2_build_contract_tests.cpp)
    target_link_libraries(idtech4_catch2_unit_tests PRIVATE Catch2::Catch2WithMain)
    idtech4_configure_test_target(idtech4_catch2_unit_tests)

    idtech4_catch2_extras_dir(_catch2_extras_dir)
    list(APPEND CMAKE_MODULE_PATH "${_catch2_extras_dir}")
    include(Catch)
    catch_discover_tests(
        idtech4_catch2_unit_tests
        TEST_PREFIX
        "unit.catch2."
        PROPERTIES
        LABELS
        unit
    )

    add_custom_target(idtech4_unit_tests DEPENDS idtech4_gtest_unit_tests idtech4_catch2_unit_tests)

    add_custom_target(idtech4_integration_artifacts DEPENDS CurlLib)

    add_test(NAME integration.CurlLibArtifact
             COMMAND "${CMAKE_COMMAND}" "-DARTIFACT=$<TARGET_FILE:CurlLib>" "-DARTIFACT_NAME=CurlLib" -P
                     "${IDTECH4_TESTING_CMAKE_DIR}/IdTech4VerifyArtifact.cmake"
    )
    set_tests_properties(
        integration.CurlLibArtifact PROPERTIES LABELS "integration" FIXTURES_REQUIRED idtech4_artifacts
    )

    add_test(NAME integration.CMakeCacheArtifact
             COMMAND "${CMAKE_COMMAND}" "-DARTIFACT=${CMAKE_BINARY_DIR}/CMakeCache.txt" "-DARTIFACT_NAME=CMakeCache" -P
                     "${IDTECH4_TESTING_CMAKE_DIR}/IdTech4VerifyArtifact.cmake"
    )
    set_tests_properties(
        integration.CMakeCacheArtifact PROPERTIES LABELS "integration" FIXTURES_REQUIRED idtech4_artifacts
    )

    add_test(NAME integration.ArtifactsBuilt COMMAND "${CMAKE_COMMAND}" -E true)
    set_tests_properties(integration.ArtifactsBuilt PROPERTIES LABELS "integration" FIXTURES_SETUP idtech4_artifacts)

    add_custom_target(idtech4_tests DEPENDS idtech4_unit_tests idtech4_integration_artifacts)
endfunction()
