include_guard(GLOBAL)

include(FetchContent)

set(IDTECH4_CPM_VERSION
    "0.40.8"
    CACHE STRING "CPM.cmake version used for lightweight dependencies."
)
set(IDTECH4_CPM_SHA256
    "78BA32ABDF798BC616BAB7C73AAC32A17BBD7B06AD9E26A6ADD69DE8F3AE4791"
    CACHE STRING "Expected SHA256 for the pinned CPM.cmake release."
)
set(IDTECH4_DEPENDENCY_DOWNLOADS
    ON
    CACHE BOOL "Allow CMake to download pinned third-party dependencies during configure."
)
set(IDTECH4_USE_CPM
    ON
    CACHE BOOL "Use CPM.cmake for header-only and lightweight dependencies."
)
set(IDTECH4_LARGE_DEPENDENCY_PROVIDER
    "vcpkg"
    CACHE STRING "Provider for large third-party dependencies."
)
set_property(CACHE IDTECH4_LARGE_DEPENDENCY_PROVIDER PROPERTY STRINGS vcpkg system bundled)

set(IDTECH4_DEPENDENCY_CACHE_DIR
    "${CMAKE_BINARY_DIR}/_deps/cache"
    CACHE PATH "Shared cache for downloaded CMake dependencies."
)
set(CPM_SOURCE_CACHE
    "${IDTECH4_DEPENDENCY_CACHE_DIR}/cpm"
    CACHE PATH "CPM.cmake package cache."
)

function(idtech4_using_vcpkg out_var)
    set(_using_vcpkg FALSE)
    if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "[/\\\\]vcpkg\\.cmake$")
        set(_using_vcpkg TRUE)
    endif()

    if(DEFINED VCPKG_TARGET_TRIPLET)
        set(_using_vcpkg TRUE)
    endif()

    set(${out_var}
        "${_using_vcpkg}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_bootstrap_cpm)
    if(COMMAND CPMAddPackage)
        return()
    endif()

    if(NOT IDTECH4_USE_CPM)
        message(FATAL_ERROR "CPM.cmake was requested, but IDTECH4_USE_CPM is OFF.")
    endif()

    set(_cpm_dir "${IDTECH4_DEPENDENCY_CACHE_DIR}/cpm-bootstrap")
    set(_cpm_file "${_cpm_dir}/CPM-${IDTECH4_CPM_VERSION}.cmake")
    set(_cpm_url "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${IDTECH4_CPM_VERSION}/CPM.cmake")

    if(NOT EXISTS "${_cpm_file}")
        if(NOT IDTECH4_DEPENDENCY_DOWNLOADS)
            message(
                FATAL_ERROR "Pinned CPM.cmake is missing at '${_cpm_file}' and IDTECH4_DEPENDENCY_DOWNLOADS is OFF."
            )
        endif()

        file(MAKE_DIRECTORY "${_cpm_dir}")
        message(STATUS "Downloading CPM.cmake ${IDTECH4_CPM_VERSION}")
        file(
            DOWNLOAD "${_cpm_url}" "${_cpm_file}"
            EXPECTED_HASH "SHA256=${IDTECH4_CPM_SHA256}"
            TLS_VERIFY ON
        )
    endif()

    include("${_cpm_file}")
endfunction()

function(idtech4_report_dependency_management)
    idtech4_using_vcpkg(_using_vcpkg)

    message(STATUS "IdTech4 dependency providers:")
    if(IDTECH4_USE_CPM)
        message(STATUS "  CPM.cmake: enabled for header-only/lightweight dependencies")
    else()
        message(STATUS "  CPM.cmake: disabled")
    endif()
    message(STATUS "  FetchContent: enabled for small source dependencies")
    message(STATUS "  Large dependencies: ${IDTECH4_LARGE_DEPENDENCY_PROVIDER}")
    if(_using_vcpkg)
        message(STATUS "  vcpkg toolchain: active")
    elseif(IDTECH4_LARGE_DEPENDENCY_PROVIDER STREQUAL "vcpkg")
        message(STATUS "  vcpkg toolchain: not active; set VCPKG_ROOT or use a vcpkg preset for large dependencies")
    endif()
endfunction()

function(idtech4_provide_googletest)
    if(TARGET GTest::gtest_main)
        return()
    endif()

    set(gtest_force_shared_crt
        ON
        CACHE BOOL "Use shared CRT for GoogleTest on MSVC." FORCE
    )
    set(INSTALL_GTEST
        OFF
        CACHE BOOL "Do not install GoogleTest from this project." FORCE
    )

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        URL_HASH SHA256=1F357C27CA988C3F7C6B4BF68A9395005AC6761F034046E9DDE0896E3ABA00E4
    )
    FetchContent_MakeAvailable(googletest)
endfunction()

function(idtech4_provide_catch2)
    if(TARGET Catch2::Catch2WithMain)
        return()
    endif()

    set(CATCH_INSTALL_DOCS
        OFF
        CACHE BOOL "Do not install Catch2 documentation." FORCE
    )
    set(CATCH_INSTALL_EXTRAS
        OFF
        CACHE BOOL "Do not install Catch2 extras." FORCE
    )

    if(IDTECH4_USE_CPM)
        idtech4_bootstrap_cpm()
        CPMAddPackage(
            NAME Catch2
            VERSION 3.5.4
            URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.4.zip
            URL_HASH SHA256=190A236FE0772AC4F5EEBFDEBFC18F92EEECFD270C55A1E5095AE4F10BE2343F
        )
    else()
        FetchContent_Declare(
            Catch2
            URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.4.zip
            URL_HASH SHA256=190A236FE0772AC4F5EEBFDEBFC18F92EEECFD270C55A1E5095AE4F10BE2343F
        )
        FetchContent_MakeAvailable(Catch2)
    endif()

    if(NOT TARGET Catch2::Catch2WithMain)
        message(FATAL_ERROR "Catch2::Catch2WithMain was not created by the configured dependency provider.")
    endif()
endfunction()

function(idtech4_catch2_extras_dir out_var)
    set(_source_dir "")
    foreach(_candidate Catch2_SOURCE_DIR catch2_SOURCE_DIR)
        if(DEFINED ${_candidate} AND EXISTS "${${_candidate}}/extras/Catch.cmake")
            set(_source_dir "${${_candidate}}")
        endif()
    endforeach()

    if(NOT _source_dir)
        message(FATAL_ERROR "Catch2 extras were not found after dependency provisioning.")
    endif()

    set(${out_var}
        "${_source_dir}/extras"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_provide_test_dependencies)
    idtech4_provide_googletest()
    idtech4_provide_catch2()
endfunction()
