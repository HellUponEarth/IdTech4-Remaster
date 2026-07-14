include_guard(GLOBAL)

set(IDTECH4_DOCUMENTATION_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

option(IDTECH4_ENABLE_DOXYGEN "Generate Doxygen API reference documentation." OFF)
option(IDTECH4_REQUIRE_DOXYGEN "Fail configure when Doxygen documentation is enabled but Doxygen is unavailable." OFF)
set(IDTECH4_DOCS_SITE_DIR
    "${CMAKE_BINARY_DIR}/docs/site"
    CACHE PATH "Output directory for the deployable documentation site."
)
set(IDTECH4_DOXYGEN_INPUT_DIRS
    "${CMAKE_SOURCE_DIR}/neo;${CMAKE_SOURCE_DIR}/tests"
    CACHE STRING "Semicolon-separated directories scanned by Doxygen."
)
set(IDTECH4_DOXYGEN_EXCLUDE_DIRS
    "${CMAKE_SOURCE_DIR}/neo/curl;${CMAKE_SOURCE_DIR}/neo/openal;${CMAKE_SOURCE_DIR}/neo/mssdk;${CMAKE_SOURCE_DIR}/neo/MayaImport/Maya5.0;${CMAKE_SOURCE_DIR}/neo/tools;${CMAKE_SOURCE_DIR}/neo/sys/win32/rc"
    CACHE STRING "Semicolon-separated directories excluded from Doxygen."
)

function(idtech4_quote_doxygen_paths out_var)
    set(_quoted_paths "")
    foreach(_path IN LISTS ARGN)
        string(APPEND _quoted_paths " \"${_path}\"")
    endforeach()

    string(STRIP "${_quoted_paths}" _quoted_paths)
    set(${out_var}
        "${_quoted_paths}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_add_build_system_docs out_doc)
    set(_docs_dir "${CMAKE_BINARY_DIR}/docs")
    set(_build_system_doc "${_docs_dir}/BUILD_SYSTEM.md")

    add_custom_command(
        OUTPUT "${_build_system_doc}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_docs_dir}"
        COMMAND
            "${CMAKE_COMMAND}" "-DSOURCE_DIR=${CMAKE_SOURCE_DIR}" "-DBINARY_DIR=${CMAKE_BINARY_DIR}"
            "-DOUTPUT_FILE=${_build_system_doc}" -P "${IDTECH4_DOCUMENTATION_CMAKE_DIR}/IdTech4GenerateBuildDocs.cmake"
        DEPENDS "${CMAKE_SOURCE_DIR}/CMakeLists.txt" "${CMAKE_SOURCE_DIR}/CMakePresets.json"
                "${IDTECH4_DOCUMENTATION_CMAKE_DIR}/IdTech4GenerateBuildDocs.cmake"
        COMMENT "Generating build-system documentation"
        VERBATIM
    )

    add_custom_target(idtech4_build_system_docs DEPENDS "${_build_system_doc}")
    set_target_properties(idtech4_build_system_docs PROPERTIES FOLDER docs)

    set(${out_doc}
        "${_build_system_doc}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_add_doxygen_api_reference out_target out_html_dir)
    set(_api_target "")
    set(_api_html_dir "${IDTECH4_DOCS_SITE_DIR}/api")

    if(NOT IDTECH4_ENABLE_DOXYGEN)
        set(${out_target}
            "${_api_target}"
            PARENT_SCOPE
        )
        set(${out_html_dir}
            "${_api_html_dir}"
            PARENT_SCOPE
        )
        return()
    endif()

    find_package(Doxygen QUIET)
    if(NOT DOXYGEN_FOUND)
        if(IDTECH4_REQUIRE_DOXYGEN)
            message(FATAL_ERROR "IDTECH4_ENABLE_DOXYGEN is ON, but Doxygen was not found.")
        endif()

        message(WARNING "IDTECH4_ENABLE_DOXYGEN is ON, but Doxygen was not found. API reference generation skipped.")
        set(${out_target}
            "${_api_target}"
            PARENT_SCOPE
        )
        set(${out_html_dir}
            "${_api_html_dir}"
            PARENT_SCOPE
        )
        return()
    endif()

    idtech4_quote_doxygen_paths(IDTECH4_DOXYGEN_INPUT ${IDTECH4_DOXYGEN_INPUT_DIRS})
    idtech4_quote_doxygen_paths(IDTECH4_DOXYGEN_EXCLUDE ${IDTECH4_DOXYGEN_EXCLUDE_DIRS})
    set(IDTECH4_DOXYGEN_OUTPUT_DIR "${IDTECH4_DOCS_SITE_DIR}")
    set(IDTECH4_DOXYGEN_HTML_OUTPUT "api")
    set(IDTECH4_DOXYGEN_PROJECT_NUMBER "${PROJECT_VERSION} (${IDTECH4_VERSION_LABEL})")
    set(_doxyfile "${CMAKE_BINARY_DIR}/docs/Doxyfile")
    configure_file("${CMAKE_SOURCE_DIR}/cmake/Doxyfile.in" "${_doxyfile}" @ONLY)

    add_custom_target(
        idtech4_api_reference
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${_api_html_dir}"
        COMMAND Doxygen::doxygen "${_doxyfile}"
        BYPRODUCTS "${_api_html_dir}/index.html"
        DEPENDS "${_doxyfile}" "${CMAKE_SOURCE_DIR}/cmake/Doxyfile.in"
        COMMENT "Generating Doxygen API reference"
        VERBATIM
    )
    set_target_properties(idtech4_api_reference PROPERTIES FOLDER docs)

    set(${out_target}
        idtech4_api_reference
        PARENT_SCOPE
    )
    set(${out_html_dir}
        "${_api_html_dir}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_add_docs_site_target build_system_doc api_target api_html_dir)
    set(_site_stamp "${IDTECH4_DOCS_SITE_DIR}/.idtech4-docs-site.stamp")

    add_custom_command(
        OUTPUT "${_site_stamp}"
        COMMAND
            "${CMAKE_COMMAND}" "-DSOURCE_DIR=${CMAKE_SOURCE_DIR}" "-DBINARY_DIR=${CMAKE_BINARY_DIR}"
            "-DBUILD_SYSTEM_DOC=${build_system_doc}" "-DSITE_DIR=${IDTECH4_DOCS_SITE_DIR}"
            "-DAPI_HTML_DIR=${api_html_dir}" "-P" "${IDTECH4_DOCUMENTATION_CMAKE_DIR}/IdTech4GenerateDocsSite.cmake"
        DEPENDS "${build_system_doc}" "${IDTECH4_DOCUMENTATION_CMAKE_DIR}/IdTech4GenerateDocsSite.cmake"
        COMMENT "Assembling deployable documentation site"
        VERBATIM
    )

    add_custom_target(idtech4_docs_site DEPENDS "${_site_stamp}")
    add_dependencies(idtech4_docs_site idtech4_build_system_docs)
    if(api_target)
        add_dependencies(idtech4_docs_site ${api_target})
    endif()
    set_target_properties(idtech4_docs_site PROPERTIES FOLDER docs)
endfunction()

function(idtech4_add_documentation_targets)
    idtech4_add_build_system_docs(_build_system_doc)
    idtech4_add_doxygen_api_reference(_api_target _api_html_dir)
    idtech4_add_docs_site_target("${_build_system_doc}" "${_api_target}" "${_api_html_dir}")

    add_custom_target(idtech4_generate_docs DEPENDS idtech4_build_system_docs idtech4_docs_site)
    if(_api_target)
        add_dependencies(idtech4_generate_docs ${_api_target})
    endif()
    set_target_properties(idtech4_generate_docs PROPERTIES FOLDER docs)
endfunction()
