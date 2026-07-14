if(NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE is required.")
endif()

if(NOT DEFINED SYSTEM_NAME)
    set(SYSTEM_NAME "unknown")
endif()

if(NOT DEFINED CONFIG)
    set(CONFIG "unknown")
endif()

if(NOT DEFINED SOURCE_DIR)
    set(SOURCE_DIR "unknown")
endif()

if(NOT DEFINED BINARY_DIR)
    set(BINARY_DIR "unknown")
endif()

string(TIMESTAMP _generated_at "%Y-%m-%dT%H:%M:%SZ" UTC)

set(_manifest "IdTech4 Remaster CI Package\n")
string(APPEND _manifest "Generated: ${_generated_at}\n")
string(APPEND _manifest "System: ${SYSTEM_NAME}\n")
string(APPEND _manifest "Configuration: ${CONFIG}\n")
string(APPEND _manifest "Source: ${SOURCE_DIR}\n")
string(APPEND _manifest "Build: ${BINARY_DIR}\n")
string(APPEND _manifest "\nPayload:\n")
string(APPEND _manifest "- bin/idtech4_gtest_unit_tests\n")
string(APPEND _manifest "- bin/idtech4_catch2_unit_tests\n")
string(APPEND _manifest "- lib/CurlLib\n")
string(APPEND _manifest "- docs/generated/BUILD_SYSTEM.md\n")
string(APPEND _manifest "- docs/generated/CMakePresets.json\n")
string(APPEND _manifest "- docs/generated/vcpkg.json\n")
string(APPEND _manifest "- docs/source/\n")

file(WRITE "${OUTPUT_FILE}" "${_manifest}")
