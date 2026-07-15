include_guard(GLOBAL)

option(IDTECH4_ENABLE_COMPILER_DIAGNOSTICS "Apply compiler-family diagnostics to IdTech4 targets." ON)
option(IDTECH4_ENABLE_CLANG_BEST_DIAGNOSTICS "Enable extra Clang diagnostics for Clang-focused builds." ON)
option(IDTECH4_BUILD_ENGINE_TARGETS "Build the native engine, game, and tool targets." ON)

function(idtech4_detect_compiler_family out_var)
    if(EMSCRIPTEN OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        set(_family "Emscripten")
    elseif(MSVC)
        set(_family "MSVC")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(_family "Clang")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(_family "GCC")
    else()
        set(_family "${CMAKE_CXX_COMPILER_ID}")
    endif()

    set(${out_var}
        "${_family}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_report_compiler_profile)
    idtech4_detect_compiler_family(_family)
    message(STATUS "IdTech4 compiler profile: ${_family}")
    message(STATUS "  C compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} (${CMAKE_C_COMPILER})")
    message(STATUS "  CXX compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER})")
    if(EMSCRIPTEN OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        message(STATUS "  Emscripten/WebAssembly mode: enabled")
    endif()
endfunction()

function(idtech4_apply_compiler_profile target)
    if(NOT IDTECH4_ENABLE_COMPILER_DIAGNOSTICS)
        return()
    endif()

    idtech4_detect_compiler_family(_family)
    get_target_property(_target_type ${target} TYPE)
    if(_target_type STREQUAL "INTERFACE_LIBRARY")
        set(_usage_scope INTERFACE)
    else()
        set(_usage_scope PRIVATE)
    endif()

    if(_family STREQUAL "MSVC")
        target_compile_options(
            ${target}
            ${_usage_scope}
            /diagnostics:caret
            /permissive-
            /Zc:__cplusplus
            /Zc:strictStrings-
        )
    elseif(_family STREQUAL "Clang")
        target_compile_options(
            ${target}
            ${_usage_scope}
            -Wall
            -Wextra
            -Wpedantic
            -Wno-unused-parameter
            -Wno-missing-field-initializers
            -fcolor-diagnostics
        )
        if(IDTECH4_ENABLE_CLANG_BEST_DIAGNOSTICS)
            target_compile_options(${target} ${_usage_scope} -fdiagnostics-show-template-tree)
        endif()
    elseif(_family STREQUAL "GCC")
        target_compile_options(
            ${target}
            ${_usage_scope}
            -Wall
            -Wextra
            -Wpedantic
            -Wno-unused-parameter
            -Wno-missing-field-initializers
            -fdiagnostics-color=always
        )
    elseif(_family STREQUAL "Emscripten")
        target_compile_definitions(${target} ${_usage_scope} IDTECH4_EMSCRIPTEN=1)
        target_compile_options(
            ${target}
            ${_usage_scope}
            -Wall
            -Wextra
            -Wno-unused-parameter
        )
        if(NOT _target_type STREQUAL "INTERFACE_LIBRARY")
            target_link_options(${target} PRIVATE "SHELL:-sALLOW_MEMORY_GROWTH=1")
        endif()
    endif()
endfunction()

function(idtech4_add_emscripten_smoke_target)
    if(NOT (EMSCRIPTEN OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten"))
        return()
    endif()

    add_executable(idtech4_emscripten_smoke tests/wasm/emscripten_smoke.cpp)
    target_compile_features(idtech4_emscripten_smoke PRIVATE cxx_std_11)
    idtech4_apply_compiler_profile(idtech4_emscripten_smoke)
    set_target_properties(idtech4_emscripten_smoke PROPERTIES FOLDER tests SUFFIX ".html")
endfunction()
