if(NOT DEFINED GCOV_EXECUTABLE)
    message(FATAL_ERROR "GCOV_EXECUTABLE is required.")
endif()
if(NOT DEFINED OBJECT_ROOT)
    message(FATAL_ERROR "OBJECT_ROOT is required.")
endif()
if(NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "OUTPUT_DIR is required.")
endif()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
file(GLOB_RECURSE _gcno_files "${OBJECT_ROOT}/*.gcno")
if(NOT _gcno_files)
    message(FATAL_ERROR "No .gcno files were found under '${OBJECT_ROOT}'. Build with coverage instrumentation first.")
endif()

foreach(_gcno IN LISTS _gcno_files)
    get_filename_component(_object_dir "${_gcno}" DIRECTORY)
    execute_process(
        COMMAND "${GCOV_EXECUTABLE}" -b -c -o "${_object_dir}" "${_gcno}"
        WORKING_DIRECTORY "${OUTPUT_DIR}"
        RESULT_VARIABLE _gcov_result
        OUTPUT_QUIET
        ERROR_VARIABLE _gcov_error
    )
    if(NOT _gcov_result EQUAL 0)
        message(WARNING "gcov failed for '${_gcno}': ${_gcov_error}")
    endif()
endforeach()

message(STATUS "gcov coverage files written to '${OUTPUT_DIR}'.")
