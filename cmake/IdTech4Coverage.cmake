include_guard(GLOBAL)

set(IDTECH4_COVERAGE_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

option(IDTECH4_ENABLE_COVERAGE "Build targets with coverage instrumentation." OFF)
set(IDTECH4_COVERAGE_FORMAT
    "gcov"
    CACHE STRING "Coverage runtime/report format: gcov or llvm-cov."
)
set_property(CACHE IDTECH4_COVERAGE_FORMAT PROPERTY STRINGS "gcov" "llvm-cov")

function(idtech4_require_coverage_support)
    if(NOT IDTECH4_ENABLE_COVERAGE)
        return()
    endif()

    if(MSVC)
        message(
            FATAL_ERROR
                "Coverage builds require a GCC or Clang-style compiler. Use a Ninja gcov/llvm-cov preset with a matching toolchain."
        )
    endif()

    if(IDTECH4_COVERAGE_FORMAT STREQUAL "gcov")
        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            message(FATAL_ERROR "gcov coverage is not configured for compiler '${CMAKE_CXX_COMPILER_ID}'.")
        endif()
    elseif(IDTECH4_COVERAGE_FORMAT STREQUAL "llvm-cov")
        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            message(
                FATAL_ERROR
                    "llvm-cov coverage requires a Clang compiler. Set CMAKE_C_COMPILER/CMAKE_CXX_COMPILER to clang/clang++."
            )
        endif()
    else()
        message(FATAL_ERROR "Unknown IDTECH4_COVERAGE_FORMAT '${IDTECH4_COVERAGE_FORMAT}'. Expected gcov or llvm-cov.")
    endif()
endfunction()

function(idtech4_apply_coverage target)
    if(NOT IDTECH4_ENABLE_COVERAGE)
        return()
    endif()

    idtech4_require_coverage_support()

    if(IDTECH4_COVERAGE_FORMAT STREQUAL "llvm-cov")
        target_compile_options(${target} PRIVATE -O0 -g -fprofile-instr-generate -fcoverage-mapping)
        set(_coverage_link_options -fprofile-instr-generate -fcoverage-mapping)
    else()
        target_compile_options(${target} PRIVATE -O0 -g --coverage)
        set(_coverage_link_options --coverage)
    endif()

    get_target_property(_target_type ${target} TYPE)
    if(NOT _target_type STREQUAL "STATIC_LIBRARY")
        target_link_options(${target} PRIVATE ${_coverage_link_options})
    endif()
endfunction()

function(idtech4_add_coverage_target)
    if(NOT IDTECH4_ENABLE_COVERAGE)
        return()
    endif()

    idtech4_require_coverage_support()
    if(NOT TARGET idtech4_tests)
        message(FATAL_ERROR "Coverage collection requires IDTECH4_BUILD_TESTS=ON so the idtech4_tests target exists.")
    endif()

    set(_coverage_dir "${CMAKE_BINARY_DIR}/coverage")

    if(IDTECH4_COVERAGE_FORMAT STREQUAL "llvm-cov")
        find_program(IDTECH4_LLVM_COV_EXECUTABLE NAMES llvm-cov REQUIRED)
        find_program(IDTECH4_LLVM_PROFDATA_EXECUTABLE NAMES llvm-profdata REQUIRED)
        add_custom_target(
            idtech4_coverage
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${_coverage_dir}/profiles" "${_coverage_dir}/llvm-cov"
            COMMAND "${CMAKE_COMMAND}" -E env "LLVM_PROFILE_FILE=${_coverage_dir}/profiles/idtech4-%p.profraw"
                    "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_BINARY_DIR}" --output-on-failure
            COMMAND
                "${CMAKE_COMMAND}" "-DLLVM_COV_EXECUTABLE=${IDTECH4_LLVM_COV_EXECUTABLE}"
                "-DLLVM_PROFDATA_EXECUTABLE=${IDTECH4_LLVM_PROFDATA_EXECUTABLE}"
                "-DPROFILE_DIR=${_coverage_dir}/profiles" "-DOUTPUT_DIR=${_coverage_dir}/llvm-cov"
                "-DBINARIES=$<TARGET_FILE:idtech4_gtest_unit_tests>;$<TARGET_FILE:idtech4_catch2_unit_tests>" -P
                "${IDTECH4_COVERAGE_CMAKE_DIR}/IdTech4CollectLlvmCov.cmake"
            DEPENDS idtech4_tests
            COMMENT "Running CTest and collecting llvm-cov coverage"
            VERBATIM
        )
    else()
        find_program(IDTECH4_GCOV_EXECUTABLE NAMES gcov REQUIRED)
        add_custom_target(
            idtech4_coverage
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${_coverage_dir}/gcov"
            COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_BINARY_DIR}" --output-on-failure
            COMMAND
                "${CMAKE_COMMAND}" "-DGCOV_EXECUTABLE=${IDTECH4_GCOV_EXECUTABLE}" "-DOBJECT_ROOT=${CMAKE_BINARY_DIR}"
                "-DOUTPUT_DIR=${_coverage_dir}/gcov" -P "${IDTECH4_COVERAGE_CMAKE_DIR}/IdTech4CollectGcov.cmake"
            DEPENDS idtech4_tests
            COMMENT "Running CTest and collecting gcov coverage"
            VERBATIM
        )
    endif()
endfunction()
