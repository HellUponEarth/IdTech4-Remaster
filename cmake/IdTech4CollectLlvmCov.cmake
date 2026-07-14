if(NOT DEFINED LLVM_COV_EXECUTABLE)
    message(FATAL_ERROR "LLVM_COV_EXECUTABLE is required.")
endif()
if(NOT DEFINED LLVM_PROFDATA_EXECUTABLE)
    message(FATAL_ERROR "LLVM_PROFDATA_EXECUTABLE is required.")
endif()
if(NOT DEFINED PROFILE_DIR)
    message(FATAL_ERROR "PROFILE_DIR is required.")
endif()
if(NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "OUTPUT_DIR is required.")
endif()
if(NOT DEFINED BINARIES)
    message(FATAL_ERROR "BINARIES is required.")
endif()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
file(GLOB _profiles "${PROFILE_DIR}/*.profraw")
if(NOT _profiles)
    message(FATAL_ERROR "No .profraw files were found under '${PROFILE_DIR}'.")
endif()

set(_profdata "${OUTPUT_DIR}/idtech4.profdata")
execute_process(
    COMMAND "${LLVM_PROFDATA_EXECUTABLE}" merge -sparse ${_profiles} -o "${_profdata}" RESULT_VARIABLE _profdata_result
)
if(NOT _profdata_result EQUAL 0)
    message(FATAL_ERROR "llvm-profdata failed.")
endif()
if(NOT EXISTS "${_profdata}")
    message(FATAL_ERROR "llvm-profdata did not create '${_profdata}'.")
endif()

set(_binary_args)
foreach(_binary IN LISTS BINARIES)
    if(EXISTS "${_binary}")
        if(NOT _binary_args)
            list(APPEND _binary_args "${_binary}")
        else()
            list(APPEND _binary_args "-object=${_binary}")
        endif()
    endif()
endforeach()

if(NOT _binary_args)
    message(FATAL_ERROR "No coverage binaries exist from BINARIES='${BINARIES}'.")
endif()

execute_process(
    COMMAND "${LLVM_COV_EXECUTABLE}" show ${_binary_args} "-instr-profile=${_profdata}" -format=html
            "-output-dir=${OUTPUT_DIR}/html" RESULT_VARIABLE _llvm_cov_result
)
if(NOT _llvm_cov_result EQUAL 0)
    message(FATAL_ERROR "llvm-cov show failed.")
endif()

message(STATUS "llvm-cov HTML coverage written to '${OUTPUT_DIR}/html'.")
