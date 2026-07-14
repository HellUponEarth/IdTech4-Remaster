include_guard(GLOBAL)

option(IDTECH4_ENABLE_CLANG_TIDY "Run clang-tidy automatically during supported C/C++ builds." OFF)
option(IDTECH4_ENABLE_CPPCHECK "Run cppcheck automatically during supported C/C++ builds." OFF)
option(IDTECH4_ENABLE_IWYU "Run include-what-you-use automatically during supported C/C++ builds." OFF)

find_program(IDTECH4_CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
if(WIN32 AND NOT DEFINED IDTECH4_CPPCHECK_EXECUTABLE)
    find_program(
        IDTECH4_CPPCHECK_EXECUTABLE
        NAMES cppcheck
        PATHS "$ENV{ProgramFiles}/Cppcheck" "$ENV{ProgramFiles}/Cppcheck/bin" "$ENV{ProgramFiles\(x86\)}/Cppcheck"
              "$ENV{ProgramFiles\(x86\)}/Cppcheck/bin"
        NO_DEFAULT_PATH
    )
endif()
find_program(IDTECH4_CPPCHECK_EXECUTABLE NAMES cppcheck)
find_program(IDTECH4_IWYU_EXECUTABLE NAMES include-what-you-use iwyu)

set(IDTECH4_CLANG_TIDY_ARGS
    "--checks=-*,clang-analyzer-core.*;--quiet"
    CACHE STRING "Semicolon-separated extra arguments passed to clang-tidy."
)
set(IDTECH4_CPPCHECK_ARGS
    "--quiet;--inline-suppr;--std=c++11;--enable=warning,performance,portability;--suppress=missingIncludeSystem"
    CACHE STRING "Semicolon-separated extra arguments passed to cppcheck."
)
set(IDTECH4_IWYU_ARGS
    ""
    CACHE STRING "Semicolon-separated extra arguments passed to include-what-you-use."
)

function(idtech4_require_static_analysis_tool enabled tool_name executable)
    if(${enabled} AND NOT executable)
        message(
            FATAL_ERROR
                "${tool_name} analysis is enabled, but ${tool_name} was not found. Set the matching IDTECH4_*_EXECUTABLE cache entry or disable the analyzer."
        )
    endif()
endfunction()

idtech4_require_static_analysis_tool(IDTECH4_ENABLE_CLANG_TIDY "clang-tidy" "${IDTECH4_CLANG_TIDY_EXECUTABLE}")
idtech4_require_static_analysis_tool(IDTECH4_ENABLE_CPPCHECK "cppcheck" "${IDTECH4_CPPCHECK_EXECUTABLE}")
idtech4_require_static_analysis_tool(IDTECH4_ENABLE_IWYU "include-what-you-use" "${IDTECH4_IWYU_EXECUTABLE}")

if(IDTECH4_ENABLE_CPPCHECK)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/CMakeFiles")
    set(_idtech4_cppcheck_probe "${CMAKE_BINARY_DIR}/CMakeFiles/idtech4_cppcheck_probe.c")
    file(WRITE "${_idtech4_cppcheck_probe}" "int idtech4_cppcheck_probe(void) { return 0; }\n")
    execute_process(
        COMMAND "${IDTECH4_CPPCHECK_EXECUTABLE}" --check-config "${_idtech4_cppcheck_probe}"
        RESULT_VARIABLE _idtech4_cppcheck_probe_result
        OUTPUT_QUIET
        ERROR_VARIABLE _idtech4_cppcheck_probe_error
    )
    if(NOT _idtech4_cppcheck_probe_result EQUAL 0)
        message(
            FATAL_ERROR
                "cppcheck was found at '${IDTECH4_CPPCHECK_EXECUTABLE}', but its configuration probe failed:\n${_idtech4_cppcheck_probe_error}"
        )
    endif()
endif()

if((IDTECH4_ENABLE_CLANG_TIDY
    OR IDTECH4_ENABLE_CPPCHECK
    OR IDTECH4_ENABLE_IWYU)
   AND NOT CMAKE_GENERATOR MATCHES "Ninja|Makefiles"
)
    message(
        WARNING
            "CMake's compile-time analyzer properties are run by Ninja and Makefile generators. The current generator may not invoke them during compilation."
    )
endif()

function(idtech4_apply_static_analysis target)
    if(IDTECH4_ENABLE_CLANG_TIDY)
        set(_clang_tidy "${IDTECH4_CLANG_TIDY_EXECUTABLE}")
        list(APPEND _clang_tidy ${IDTECH4_CLANG_TIDY_ARGS})
        if(WIN32 AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                list(APPEND _clang_tidy "--extra-arg-before=--target=x86_64-w64-windows-gnu")
            else()
                list(APPEND _clang_tidy "--extra-arg-before=--target=i686-w64-windows-gnu")
            endif()
            list(APPEND _clang_tidy "--extra-arg=-Wno-incompatible-pointer-types")
        endif()
        set_target_properties(${target} PROPERTIES C_CLANG_TIDY "${_clang_tidy}" CXX_CLANG_TIDY "${_clang_tidy}")
    endif()

    if(IDTECH4_ENABLE_CPPCHECK)
        set(_cppcheck "${IDTECH4_CPPCHECK_EXECUTABLE}")
        list(APPEND _cppcheck ${IDTECH4_CPPCHECK_ARGS})
        set_target_properties(${target} PROPERTIES C_CPPCHECK "${_cppcheck}" CXX_CPPCHECK "${_cppcheck}")
    endif()

    if(IDTECH4_ENABLE_IWYU)
        set(_iwyu "${IDTECH4_IWYU_EXECUTABLE}")
        list(APPEND _iwyu ${IDTECH4_IWYU_ARGS})
        set_target_properties(
            ${target} PROPERTIES C_INCLUDE_WHAT_YOU_USE "${_iwyu}" CXX_INCLUDE_WHAT_YOU_USE "${_iwyu}"
        )
    endif()
endfunction()
