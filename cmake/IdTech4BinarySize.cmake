include_guard(GLOBAL)

set(IDTECH4_BINARY_SIZE_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(IDTECH4_BINARY_SIZE_REPORT
    "${CMAKE_BINARY_DIR}/reports/binary-size.md"
    CACHE FILEPATH "Markdown report written by the idtech4_binary_size target."
)
option(IDTECH4_BINARY_SIZE_INCLUDE_ENGINE_TARGETS "Include full native engine artifacts in binary-size analysis." OFF)

function(idtech4_append_size_artifact out_entries out_deps target)
    if(NOT TARGET ${target})
        return()
    endif()

    get_target_property(_target_type ${target} TYPE)
    if(_target_type STREQUAL "INTERFACE_LIBRARY" OR _target_type STREQUAL "UTILITY")
        return()
    endif()

    set(_entry "${target}::${_target_type}::$<TARGET_FILE:${target}>")
    set(${out_entries}
        ${${out_entries}} "${_entry}"
        PARENT_SCOPE
    )
    set(${out_deps}
        ${${out_deps}} ${target}
        PARENT_SCOPE
    )
endfunction()

function(idtech4_add_binary_size_target)
    set(_entries)
    set(_deps)

    idtech4_append_size_artifact(_entries _deps CurlLib)
    idtech4_append_size_artifact(_entries _deps idtech4_gtest_unit_tests)
    idtech4_append_size_artifact(_entries _deps idtech4_catch2_unit_tests)

    if(IDTECH4_BINARY_SIZE_INCLUDE_ENGINE_TARGETS)
        idtech4_append_size_artifact(_entries _deps idLib)
        idtech4_append_size_artifact(_entries _deps TypeInfo)
        idtech4_append_size_artifact(_entries _deps DoomDLL)
        idtech4_append_size_artifact(_entries _deps Game)
        idtech4_append_size_artifact(_entries _deps Game-d3xp)
        if(IDTECH4_BUILD_MAYAIMPORT)
            idtech4_append_size_artifact(_entries _deps MayaImport)
        endif()
    endif()

    string(JOIN "|" _artifact_entries ${_entries})
    get_filename_component(_report_dir "${IDTECH4_BINARY_SIZE_REPORT}" DIRECTORY)

    add_custom_target(
        idtech4_binary_size
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_report_dir}"
        COMMAND "${CMAKE_COMMAND}" "-DOUTPUT_FILE=${IDTECH4_BINARY_SIZE_REPORT}" "-DARTIFACTS=${_artifact_entries}" -P
                "${IDTECH4_BINARY_SIZE_CMAKE_DIR}/IdTech4AnalyzeBinarySize.cmake"
        DEPENDS ${_deps}
        COMMENT "Analyzing IdTech4 binary artifact sizes"
        VERBATIM
    )
    set_target_properties(idtech4_binary_size PROPERTIES FOLDER analysis)
endfunction()
