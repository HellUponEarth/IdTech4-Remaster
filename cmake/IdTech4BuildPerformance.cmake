include_guard(GLOBAL)

include(CheckIPOSupported)

option(IDTECH4_ENABLE_FAST_ITERATION "Enable the fast-iteration bundle: unity builds, PCH, and compiler cache." OFF)
option(IDTECH4_ENABLE_UNITY_BUILDS "Enable CMake unity builds on IdTech4 targets." OFF)
option(IDTECH4_ENABLE_PRECOMPILED_HEADERS "Enable target precompiled headers for C++ IdTech4 targets." OFF)
option(IDTECH4_ENABLE_BUILD_CACHE "Enable an automatically detected compiler cache on IdTech4 targets." OFF)
option(IDTECH4_ENABLE_CCACHE "Prefer ccache as the target compiler launcher." OFF)
option(IDTECH4_ENABLE_SCCACHE "Prefer sccache as the target compiler launcher." OFF)
option(IDTECH4_REQUIRE_BUILD_CACHE "Fail configure when compiler cache support is requested but unavailable." OFF)
option(IDTECH4_ENABLE_LTO "Enable link-time optimization through target IPO properties." OFF)
option(IDTECH4_REQUIRE_LTO "Fail configure when LTO is requested but unsupported by the active toolchain." OFF)
option(IDTECH4_ALLOW_LEGACY_PCH_ON_NON_MSVC
       "Allow the legacy engine precompiled header to be force-included by non-MSVC toolchains." OFF
)

set(IDTECH4_UNITY_BATCH_SIZE
    "16"
    CACHE STRING "Number of sources CMake batches into each unity translation unit."
)
set(IDTECH4_PCH_HEADER
    "${CMAKE_SOURCE_DIR}/neo/idlib/precompiled.h"
    CACHE FILEPATH "Header used by target precompiled-header support."
)
set(IDTECH4_BUILD_CACHE_LAUNCHER
    ""
    CACHE FILEPATH "Explicit compiler cache launcher. Leave empty to auto-detect ccache or sccache."
)

function(idtech4_bool_or_fast_iteration out_var option_value)
    if(${option_value} OR IDTECH4_ENABLE_FAST_ITERATION)
        set(_enabled TRUE)
    else()
        set(_enabled FALSE)
    endif()

    set(${out_var}
        "${_enabled}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_find_build_cache_launcher out_var)
    set(_launcher "")
    idtech4_bool_or_fast_iteration(_cache_requested IDTECH4_ENABLE_BUILD_CACHE)

    if(NOT _cache_requested
       AND NOT IDTECH4_ENABLE_CCACHE
       AND NOT IDTECH4_ENABLE_SCCACHE
    )
        set(${out_var}
            "${_launcher}"
            PARENT_SCOPE
        )
        return()
    endif()

    if(IDTECH4_BUILD_CACHE_LAUNCHER)
        set(_launcher "${IDTECH4_BUILD_CACHE_LAUNCHER}")
    elseif(IDTECH4_ENABLE_CCACHE)
        find_program(_ccache_launcher ccache)
        set(_launcher "${_ccache_launcher}")
    elseif(IDTECH4_ENABLE_SCCACHE OR MSVC)
        find_program(_sccache_launcher sccache)
        set(_launcher "${_sccache_launcher}")
        if(NOT _launcher AND NOT MSVC)
            find_program(_ccache_launcher ccache)
            set(_launcher "${_ccache_launcher}")
        endif()
    else()
        find_program(_sccache_launcher sccache)
        if(_sccache_launcher)
            set(_launcher "${_sccache_launcher}")
        else()
            find_program(_ccache_launcher ccache)
            set(_launcher "${_ccache_launcher}")
        endif()
    endif()

    if(NOT _launcher)
        if(IDTECH4_REQUIRE_BUILD_CACHE)
            message(FATAL_ERROR "Compiler cache support was requested, but ccache/sccache was not found.")
        endif()
        set(_launcher "")
    endif()

    set(${out_var}
        "${_launcher}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_target_has_cxx_sources out_var target)
    get_target_property(_sources ${target} SOURCES)
    set(_has_cxx FALSE)

    foreach(_source IN LISTS _sources)
        if(_source MATCHES "\\.(cc|cpp|cxx|C)$")
            set(_has_cxx TRUE)
        endif()
    endforeach()

    set(${out_var}
        "${_has_cxx}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_apply_build_cache target)
    idtech4_find_build_cache_launcher(_launcher)
    if(NOT _launcher)
        return()
    endif()

    set_target_properties(${target} PROPERTIES C_COMPILER_LAUNCHER "${_launcher}" CXX_COMPILER_LAUNCHER "${_launcher}")
endfunction()

function(idtech4_apply_unity_build target)
    idtech4_bool_or_fast_iteration(_unity_enabled IDTECH4_ENABLE_UNITY_BUILDS)
    if(NOT _unity_enabled)
        return()
    endif()

    set_target_properties(${target} PROPERTIES UNITY_BUILD ON UNITY_BUILD_BATCH_SIZE "${IDTECH4_UNITY_BATCH_SIZE}")
endfunction()

function(idtech4_apply_precompiled_headers target)
    idtech4_bool_or_fast_iteration(_pch_enabled IDTECH4_ENABLE_PRECOMPILED_HEADERS)
    if(NOT _pch_enabled)
        return()
    endif()

    get_target_property(_skip_pch ${target} IDTECH4_SKIP_PRECOMPILED_HEADERS)
    if(_skip_pch)
        return()
    endif()

    if(NOT EXISTS "${IDTECH4_PCH_HEADER}")
        message(FATAL_ERROR "IDTECH4_PCH_HEADER points to a missing file: ${IDTECH4_PCH_HEADER}")
    endif()

    if(NOT MSVC
       AND NOT IDTECH4_ALLOW_LEGACY_PCH_ON_NON_MSVC
       AND IDTECH4_PCH_HEADER STREQUAL "${CMAKE_SOURCE_DIR}/neo/idlib/precompiled.h"
    )
        return()
    endif()

    idtech4_target_has_cxx_sources(_has_cxx ${target})
    if(NOT _has_cxx)
        return()
    endif()

    target_precompile_headers(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${IDTECH4_PCH_HEADER}>")
endfunction()

function(idtech4_apply_lto target)
    if(NOT IDTECH4_ENABLE_LTO)
        return()
    endif()

    if(IDTECH4_ENABLE_COVERAGE
       OR IDTECH4_ENABLE_ASAN
       OR IDTECH4_ENABLE_UBSAN
       OR IDTECH4_ENABLE_TSAN
    )
        message(FATAL_ERROR "IDTECH4_ENABLE_LTO cannot be combined with coverage or sanitizer presets.")
    endif()

    check_ipo_supported(
        RESULT _ipo_supported
        OUTPUT _ipo_output
        LANGUAGES C CXX
    )
    if(NOT _ipo_supported)
        if(IDTECH4_REQUIRE_LTO)
            message(FATAL_ERROR "LTO was requested, but the active toolchain does not support IPO: ${_ipo_output}")
        endif()
        message(WARNING "LTO was requested, but IPO is unsupported by this toolchain: ${_ipo_output}")
        return()
    endif()

    set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
endfunction()

function(idtech4_apply_build_performance target)
    idtech4_apply_build_cache(${target})
    idtech4_apply_unity_build(${target})
    idtech4_apply_precompiled_headers(${target})
    idtech4_apply_lto(${target})
endfunction()

function(idtech4_report_build_performance)
    idtech4_bool_or_fast_iteration(_unity_enabled IDTECH4_ENABLE_UNITY_BUILDS)
    idtech4_bool_or_fast_iteration(_pch_enabled IDTECH4_ENABLE_PRECOMPILED_HEADERS)
    idtech4_bool_or_fast_iteration(_cache_enabled IDTECH4_ENABLE_BUILD_CACHE)
    idtech4_find_build_cache_launcher(_launcher)

    message(STATUS "IdTech4 build performance:")
    message(STATUS "  Fast iteration bundle: ${IDTECH4_ENABLE_FAST_ITERATION}")
    message(STATUS "  Unity builds: ${_unity_enabled}")
    message(STATUS "  Precompiled headers: ${_pch_enabled}")
    if(_pch_enabled
       AND NOT MSVC
       AND NOT IDTECH4_ALLOW_LEGACY_PCH_ON_NON_MSVC
       AND IDTECH4_PCH_HEADER STREQUAL "${CMAKE_SOURCE_DIR}/neo/idlib/precompiled.h"
    )
        message(STATUS "  Precompiled headers detail: legacy engine PCH is MSVC-only by default")
    endif()
    message(STATUS "  Build cache requested: ${_cache_enabled}")
    if(_launcher)
        message(STATUS "  Build cache launcher: ${_launcher}")
    elseif(
        _cache_enabled
        OR IDTECH4_ENABLE_CCACHE
        OR IDTECH4_ENABLE_SCCACHE
    )
        message(STATUS "  Build cache launcher: unavailable")
    endif()
    message(STATUS "  Link-time optimization: ${IDTECH4_ENABLE_LTO}")
endfunction()
