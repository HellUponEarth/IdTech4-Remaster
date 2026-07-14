set(_idtech4_emscripten_toolchain "")

if(DEFINED ENV{EMSDK})
    set(_idtech4_emscripten_toolchain "$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
elseif(EXISTS "/usr/share/emscripten/cmake/Modules/Platform/Emscripten.cmake")
    set(_idtech4_emscripten_toolchain "/usr/share/emscripten/cmake/Modules/Platform/Emscripten.cmake")
endif()

if(EXISTS "${_idtech4_emscripten_toolchain}")
    include("${_idtech4_emscripten_toolchain}")
else()
    find_program(EMCC_EXECUTABLE emcc)
    find_program(EMXX_EXECUTABLE em++)
    if(NOT EMCC_EXECUTABLE OR NOT EMXX_EXECUTABLE)
        message(FATAL_ERROR "Set EMSDK to an activated Emscripten SDK, or put emcc and em++ on PATH.")
    endif()

    set(CMAKE_SYSTEM_NAME Emscripten)
    set(CMAKE_C_COMPILER
        "${EMCC_EXECUTABLE}"
        CACHE FILEPATH "Emscripten C compiler"
    )
    set(CMAKE_CXX_COMPILER
        "${EMXX_EXECUTABLE}"
        CACHE FILEPATH "Emscripten C++ compiler"
    )
endif()
