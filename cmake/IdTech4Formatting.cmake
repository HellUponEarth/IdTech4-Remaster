include_guard(GLOBAL)

option(IDTECH4_ENABLE_FORMAT_TARGETS "Add clang-format and cmake-format targets for modern build-system files." ON)

function(idtech4_collect_format_files out_cxx out_cmake)
    file(
        GLOB_RECURSE
        _cxx_files
        CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/tests/*.c"
        "${CMAKE_SOURCE_DIR}/tests/*.cc"
        "${CMAKE_SOURCE_DIR}/tests/*.cpp"
        "${CMAKE_SOURCE_DIR}/tests/*.cxx"
        "${CMAKE_SOURCE_DIR}/tests/*.h"
        "${CMAKE_SOURCE_DIR}/tests/*.hh"
        "${CMAKE_SOURCE_DIR}/tests/*.hpp"
        "${CMAKE_SOURCE_DIR}/tests/*.hxx"
    )

    file(
        GLOB_RECURSE
        _cmake_files
        CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/cmake/*.cmake"
        "${CMAKE_SOURCE_DIR}/tests/package/*/CMakeLists.txt"
    )
    list(APPEND _cmake_files "${CMAKE_SOURCE_DIR}/CMakeLists.txt")

    list(SORT _cxx_files)
    list(SORT _cmake_files)

    set(${out_cxx}
        "${_cxx_files}"
        PARENT_SCOPE
    )
    set(${out_cmake}
        "${_cmake_files}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_add_format_targets)
    if(NOT IDTECH4_ENABLE_FORMAT_TARGETS)
        return()
    endif()

    idtech4_collect_format_files(_cxx_files _cmake_files)

    set(_format_tool_paths)
    if(CMAKE_HOST_WIN32 AND DEFINED ENV{APPDATA})
        file(GLOB _python_script_dirs "$ENV{APPDATA}/Python/Python*/Scripts")
        list(APPEND _format_tool_paths ${_python_script_dirs})
    endif()

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
    find_program(
        CMAKE_FORMAT_EXECUTABLE
        NAMES cmake-format
        PATHS ${_format_tool_paths}
    )
    find_program(
        CMAKE_LINT_EXECUTABLE
        NAMES cmake-lint
        PATHS ${_format_tool_paths}
    )

    add_custom_target(
        idtech4_clang_format_check
        COMMAND "${CMAKE_COMMAND}" "-DCLANG_FORMAT_EXECUTABLE=${CLANG_FORMAT_EXECUTABLE}" "-DFORMAT_MODE=check"
                "-DFORMAT_FILES=${_cxx_files}" -P "${CMAKE_SOURCE_DIR}/cmake/IdTech4RunClangFormat.cmake"
        COMMENT "Checking clang-format style"
        VERBATIM
    )

    add_custom_target(
        idtech4_clang_format
        COMMAND "${CMAKE_COMMAND}" "-DCLANG_FORMAT_EXECUTABLE=${CLANG_FORMAT_EXECUTABLE}" "-DFORMAT_MODE=fix"
                "-DFORMAT_FILES=${_cxx_files}" -P "${CMAKE_SOURCE_DIR}/cmake/IdTech4RunClangFormat.cmake"
        COMMENT "Applying clang-format style"
        VERBATIM
    )

    add_custom_target(
        idtech4_cmake_format_check
        COMMAND "${CMAKE_COMMAND}" "-DCMAKE_FORMAT_EXECUTABLE=${CMAKE_FORMAT_EXECUTABLE}" "-DFORMAT_MODE=check"
                "-DFORMAT_FILES=${_cmake_files}" -P "${CMAKE_SOURCE_DIR}/cmake/IdTech4RunCMakeFormat.cmake"
        COMMENT "Checking cmake-format style"
        VERBATIM
    )

    add_custom_target(
        idtech4_cmake_format
        COMMAND "${CMAKE_COMMAND}" "-DCMAKE_FORMAT_EXECUTABLE=${CMAKE_FORMAT_EXECUTABLE}" "-DFORMAT_MODE=fix"
                "-DFORMAT_FILES=${_cmake_files}" -P "${CMAKE_SOURCE_DIR}/cmake/IdTech4RunCMakeFormat.cmake"
        COMMENT "Applying cmake-format style"
        VERBATIM
    )

    add_custom_target(
        idtech4_cmake_lint
        COMMAND "${CMAKE_COMMAND}" "-DCMAKE_LINT_EXECUTABLE=${CMAKE_LINT_EXECUTABLE}" "-DLINT_FILES=${_cmake_files}" -P
                "${CMAKE_SOURCE_DIR}/cmake/IdTech4RunCMakeLint.cmake"
        COMMENT "Running cmake-lint"
        VERBATIM
    )

    add_custom_target(idtech4_format_check DEPENDS idtech4_clang_format_check idtech4_cmake_format_check)
    set_target_properties(idtech4_format_check PROPERTIES FOLDER checks)

    add_custom_target(idtech4_format DEPENDS idtech4_clang_format idtech4_cmake_format)
    set_target_properties(idtech4_format PROPERTIES FOLDER checks)

    add_custom_target(idtech4_lint DEPENDS idtech4_cmake_lint)
    set_target_properties(idtech4_lint PROPERTIES FOLDER checks)
endfunction()
