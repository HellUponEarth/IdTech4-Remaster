if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR is required.")
endif()

if(NOT DEFINED BINARY_DIR)
    message(FATAL_ERROR "BINARY_DIR is required.")
endif()

if(NOT DEFINED BUILD_SYSTEM_DOC)
    message(FATAL_ERROR "BUILD_SYSTEM_DOC is required.")
endif()

if(NOT DEFINED SITE_DIR)
    message(FATAL_ERROR "SITE_DIR is required.")
endif()

if(NOT DEFINED API_HTML_DIR)
    set(API_HTML_DIR "")
endif()

file(
    REMOVE
    "${SITE_DIR}/index.html"
    "${SITE_DIR}/BUILD_SYSTEM.md"
    "${SITE_DIR}/.nojekyll"
    "${SITE_DIR}/.idtech4-docs-site.stamp"
)
file(REMOVE_RECURSE "${SITE_DIR}/reference-docs")
file(MAKE_DIRECTORY "${SITE_DIR}" "${SITE_DIR}/reference-docs")
file(WRITE "${SITE_DIR}/.nojekyll" "")

if(EXISTS "${BUILD_SYSTEM_DOC}")
    file(COPY "${BUILD_SYSTEM_DOC}" DESTINATION "${SITE_DIR}")
else()
    message(FATAL_ERROR "Generated build-system documentation is missing: '${BUILD_SYSTEM_DOC}'.")
endif()

file(
    GLOB _reference_docs
    RELATIVE "${SOURCE_DIR}/Documentation"
    "${SOURCE_DIR}/Documentation/*.md" "${SOURCE_DIR}/Documentation/*.txt"
)
list(SORT _reference_docs)
foreach(_doc IN LISTS _reference_docs)
    file(COPY "${SOURCE_DIR}/Documentation/${_doc}" DESTINATION "${SITE_DIR}/reference-docs")
endforeach()

set(_api_link "")
if(API_HTML_DIR AND EXISTS "${API_HTML_DIR}/index.html")
    set(_api_link "<li><a href=\"api/index.html\">API Reference</a></li>")
endif()

set(_reference_links "")
foreach(_doc IN LISTS _reference_docs)
    string(APPEND _reference_links "        <li><a href=\"reference-docs/${_doc}\">${_doc}</a></li>\n")
endforeach()

string(
    CONCAT _index
           "<!doctype html>\n"
           "<html lang=\"en\">\n"
           "<head>\n"
           "  <meta charset=\"utf-8\">\n"
           "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
           "  <title>IdTech4 Remaster Documentation</title>\n"
           "  <style>\n"
           "    body { font-family: system-ui, -apple-system, Segoe UI, sans-serif; margin: 2rem; line-height: 1.5; }\n"
           "    main { max-width: 960px; }\n"
           "    code { background: #f3f4f6; padding: 0.1rem 0.25rem; border-radius: 3px; }\n"
           "  </style>\n"
           "</head>\n"
           "<body>\n"
           "  <main>\n"
           "    <h1>IdTech4 Remaster Documentation</h1>\n"
           "    <p>Generated documentation for the modern CMake build and source API reference.</p>\n"
           "    <h2>Generated Documentation</h2>\n"
           "    <ul>\n"
           "      <li><a href=\"BUILD_SYSTEM.md\">Build System Reference</a></li>\n"
           "      ${_api_link}\n"
           "    </ul>\n"
           "    <h2>Repository Documentation</h2>\n"
           "    <ul>\n"
           "${_reference_links}"
           "    </ul>\n"
           "  </main>\n"
           "</body>\n"
           "</html>\n"
)

file(WRITE "${SITE_DIR}/index.html" "${_index}")
file(WRITE "${SITE_DIR}/.idtech4-docs-site.stamp" "IdTech4 documentation site generated from ${SOURCE_DIR}\n")

message(STATUS "Documentation site written to '${SITE_DIR}'.")
