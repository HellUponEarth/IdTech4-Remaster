if(NOT FORMAT_MODE)
    message(FATAL_ERROR "FORMAT_MODE is required.")
endif()

set(_format_tool_paths)
if(CMAKE_HOST_WIN32 AND DEFINED ENV{APPDATA})
    file(GLOB _python_script_dirs "$ENV{APPDATA}/Python/Python*/Scripts")
    list(APPEND _format_tool_paths ${_python_script_dirs})
endif()

if(NOT CMAKE_FORMAT_EXECUTABLE)
    unset(CMAKE_FORMAT_EXECUTABLE)
    find_program(
        CMAKE_FORMAT_EXECUTABLE
        NAMES cmake-format
        PATHS ${_format_tool_paths}
    )
endif()

if(NOT CMAKE_FORMAT_EXECUTABLE)
    message(FATAL_ERROR "cmake-format was not found. Install cmakelang or run through pre-commit.")
endif()

if(NOT FORMAT_FILES)
    message(STATUS "No CMake files are in the cmake-format enforcement set.")
    return()
endif()

if(FORMAT_MODE STREQUAL "check")
    execute_process(COMMAND "${CMAKE_FORMAT_EXECUTABLE}" --check ${FORMAT_FILES} RESULT_VARIABLE _result)
elseif(FORMAT_MODE STREQUAL "fix")
    execute_process(COMMAND "${CMAKE_FORMAT_EXECUTABLE}" -i ${FORMAT_FILES} RESULT_VARIABLE _result)
else()
    message(FATAL_ERROR "Unknown FORMAT_MODE '${FORMAT_MODE}'.")
endif()

if(NOT _result EQUAL 0)
    message(FATAL_ERROR "cmake-format ${FORMAT_MODE} failed.")
endif()
