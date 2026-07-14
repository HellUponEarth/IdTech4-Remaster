include_guard(GLOBAL)

function(idtech4_resolve_version)
    set(_fallback_version "0.1.0")
    set(_describe "unknown")

    find_package(Git QUIET)
    if(Git_FOUND)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" describe --tags --dirty --always
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE _describe_output
            ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(_describe_output)
            set(_describe "${_describe_output}")
        endif()
    endif()

    set(_version "${_fallback_version}")
    if(_describe MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)([-+._A-Za-z0-9]*)?$")
        set(_version "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
    endif()

    string(REGEX REPLACE "[^A-Za-z0-9.]+" "-" _label "${_describe}")
    string(REGEX REPLACE "^-|-$" "" _label "${_label}")
    if(NOT _label)
        set(_label "${_version}")
    endif()

    set(IDTECH4_GIT_DESCRIBE
        "${_describe}"
        CACHE STRING "Version label reported by git describe." FORCE
    )
    set(IDTECH4_VERSION
        "${_version}"
        CACHE STRING "Semantic package version derived from git describe, or the project fallback." FORCE
    )
    set(IDTECH4_VERSION_LABEL
        "${_label}"
        CACHE STRING "Filesystem-safe version label derived from git describe." FORCE
    )

    message(STATUS "IdTech4 version: ${IDTECH4_VERSION} (${IDTECH4_GIT_DESCRIBE})")
endfunction()
