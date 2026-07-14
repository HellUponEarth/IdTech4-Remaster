if(NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE is required.")
endif()

set(_artifacts "")
if(DEFINED ARTIFACTS AND NOT ARTIFACTS STREQUAL "")
    string(REPLACE "|" ";" _artifacts "${ARTIFACTS}")
endif()

string(TIMESTAMP _generated_at "%Y-%m-%dT%H:%M:%SZ" UTC)
set(_report "# IdTech4 Binary Size Report\n\n")
string(APPEND _report "Generated: `${_generated_at}`\n\n")
string(APPEND _report "| Target | Type | Bytes | MiB | Path |\n")
string(APPEND _report "| --- | --- | ---: | ---: | --- |\n")

set(_total_bytes 0)
set(_artifact_count 0)

foreach(_entry IN LISTS _artifacts)
    if(_entry STREQUAL "")
        continue()
    endif()

    string(REPLACE "::" ";" _parts "${_entry}")
    list(LENGTH _parts _part_count)
    if(NOT _part_count EQUAL 3)
        message(FATAL_ERROR "Malformed binary-size artifact entry: ${_entry}")
    endif()

    list(GET _parts 0 _target_name)
    list(GET _parts 1 _target_type)
    list(GET _parts 2 _artifact_path)

    if(NOT EXISTS "${_artifact_path}")
        message(FATAL_ERROR "Binary-size artifact for ${_target_name} does not exist: ${_artifact_path}")
    endif()

    file(SIZE "${_artifact_path}" _artifact_bytes)
    math(EXPR _total_bytes "${_total_bytes} + ${_artifact_bytes}")
    math(EXPR _artifact_count "${_artifact_count} + 1")
    math(EXPR _artifact_kib_tenths "(${_artifact_bytes} * 10) / 1048576")
    math(EXPR _artifact_mib_major "${_artifact_kib_tenths} / 10")
    math(EXPR _artifact_mib_minor "${_artifact_kib_tenths} % 10")

    string(
        APPEND
        _report
        "| `${_target_name}` | `${_target_type}` | ${_artifact_bytes} | ${_artifact_mib_major}.${_artifact_mib_minor} | `${_artifact_path}` |\n"
    )
endforeach()

math(EXPR _total_mib_tenths "(${_total_bytes} * 10) / 1048576")
math(EXPR _total_mib_major "${_total_mib_tenths} / 10")
math(EXPR _total_mib_minor "${_total_mib_tenths} % 10")

string(APPEND _report "\nArtifacts measured: `${_artifact_count}`\n\n")
string(APPEND _report "Total bytes: `${_total_bytes}`\n\n")
string(APPEND _report "Total MiB: `${_total_mib_major}.${_total_mib_minor}`\n")

file(WRITE "${OUTPUT_FILE}" "${_report}")
message(STATUS "Binary-size report written to '${OUTPUT_FILE}'.")
