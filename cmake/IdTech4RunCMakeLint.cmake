if(NOT CMAKE_LINT_EXECUTABLE)
    unset(CMAKE_LINT_EXECUTABLE)

    set(_format_tool_paths)
    if(CMAKE_HOST_WIN32 AND DEFINED ENV{APPDATA})
        file(GLOB _python_script_dirs "$ENV{APPDATA}/Python/Python*/Scripts")
        list(APPEND _format_tool_paths ${_python_script_dirs})
    endif()

    find_program(
        CMAKE_LINT_EXECUTABLE
        NAMES cmake-lint
        PATHS ${_format_tool_paths}
    )
endif()

if(NOT CMAKE_LINT_EXECUTABLE)
    message(FATAL_ERROR "cmake-lint was not found. Install cmakelang or run through pre-commit.")
endif()

if(NOT LINT_FILES)
    message(STATUS "No CMake files are in the cmake-lint enforcement set.")
    return()
endif()

execute_process(COMMAND "${CMAKE_LINT_EXECUTABLE}" ${LINT_FILES} RESULT_VARIABLE _result)

if(NOT _result EQUAL 0)
    message(FATAL_ERROR "cmake-lint failed.")
endif()
