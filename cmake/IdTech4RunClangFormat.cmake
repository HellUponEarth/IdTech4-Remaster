if(NOT FORMAT_MODE)
    message(FATAL_ERROR "FORMAT_MODE is required.")
endif()

set(_format_tool_paths)
if(CMAKE_HOST_WIN32 AND DEFINED ENV{APPDATA})
    file(GLOB _python_script_dirs "$ENV{APPDATA}/Python/Python*/Scripts")
    list(APPEND _format_tool_paths ${_python_script_dirs})
endif()

if(NOT CLANG_FORMAT_EXECUTABLE)
    unset(CLANG_FORMAT_EXECUTABLE)
    find_program(
        CLANG_FORMAT_EXECUTABLE
        NAMES clang-format
              clang-format-22
              clang-format-19
              clang-format-18
              clang-format-17
              clang-format-16
        PATHS ${_format_tool_paths}
    )
endif()

if(NOT CLANG_FORMAT_EXECUTABLE)
    message(FATAL_ERROR "clang-format was not found. Install clang-format or run through pre-commit.")
endif()

if(NOT FORMAT_FILES)
    message(STATUS "No C/C++ files are in the clang-format enforcement set.")
    return()
endif()

if(FORMAT_MODE STREQUAL "check")
    execute_process(COMMAND "${CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror ${FORMAT_FILES} RESULT_VARIABLE _result)
elseif(FORMAT_MODE STREQUAL "fix")
    execute_process(COMMAND "${CLANG_FORMAT_EXECUTABLE}" -i ${FORMAT_FILES} RESULT_VARIABLE _result)
else()
    message(FATAL_ERROR "Unknown FORMAT_MODE '${FORMAT_MODE}'.")
endif()

if(NOT _result EQUAL 0)
    message(FATAL_ERROR "clang-format ${FORMAT_MODE} failed.")
endif()
