include_guard(GLOBAL)

option(IDTECH4_ENABLE_ASAN "Build targets with AddressSanitizer instrumentation." OFF)
option(IDTECH4_ENABLE_UBSAN "Build targets with UndefinedBehaviorSanitizer instrumentation." OFF)
option(IDTECH4_ENABLE_TSAN "Build targets with ThreadSanitizer instrumentation." OFF)

function(idtech4_any_sanitizer_enabled out_var)
    if(IDTECH4_ENABLE_ASAN
       OR IDTECH4_ENABLE_UBSAN
       OR IDTECH4_ENABLE_TSAN
    )
        set(${out_var}
            TRUE
            PARENT_SCOPE
        )
    else()
        set(${out_var}
            FALSE
            PARENT_SCOPE
        )
    endif()
endfunction()

function(idtech4_require_gnu_runtime sanitizer runtime_name)
    if(NOT WIN32 OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        return()
    endif()

    execute_process(
        COMMAND "${CMAKE_CXX_COMPILER}" "-print-file-name=${runtime_name}"
        RESULT_VARIABLE _runtime_result
        OUTPUT_VARIABLE _runtime_path
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT _runtime_result EQUAL 0 OR _runtime_path STREQUAL runtime_name)
        message(
            FATAL_ERROR
                "${sanitizer} sanitizer requires '${runtime_name}', but '${CMAKE_CXX_COMPILER}' did not report a usable runtime library."
        )
    endif()
endfunction()

function(idtech4_require_sanitizer_support sanitizer runtime_name)
    if(MSVC)
        if(NOT sanitizer STREQUAL "address")
            message(
                FATAL_ERROR
                    "${sanitizer} sanitizer is not supported by MSVC. Use a Clang or GCC preset on a platform with the matching sanitizer runtime."
            )
        endif()
        return()
    endif()

    if(WIN32 AND sanitizer STREQUAL "thread")
        message(
            FATAL_ERROR
                "ThreadSanitizer is not supported by this Windows preset. Use a non-Windows Clang/GCC toolchain with a ThreadSanitizer runtime."
        )
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        idtech4_require_gnu_runtime("${sanitizer}" "${runtime_name}")
        return()
    endif()

    message(FATAL_ERROR "${sanitizer} sanitizer is not configured for compiler '${CMAKE_CXX_COMPILER_ID}'.")
endfunction()

function(idtech4_collect_sanitizer_options out_compile out_link)
    set(_compile_options)
    set(_link_options)

    if(IDTECH4_ENABLE_ASAN AND IDTECH4_ENABLE_TSAN)
        message(FATAL_ERROR "AddressSanitizer and ThreadSanitizer cannot be enabled in the same build.")
    endif()

    if(IDTECH4_ENABLE_ASAN)
        idtech4_require_sanitizer_support("address" "libasan.a")
        if(MSVC)
            list(
                APPEND
                _compile_options
                /fsanitize=address
                /Zi
                /Oy-
            )
            list(APPEND _link_options /INCREMENTAL:NO)
        else()
            list(APPEND _compile_options -fsanitize=address -fno-omit-frame-pointer)
            list(APPEND _link_options -fsanitize=address)
        endif()
    endif()

    if(IDTECH4_ENABLE_UBSAN)
        idtech4_require_sanitizer_support("undefined" "libubsan.a")
        list(APPEND _compile_options -fsanitize=undefined -fno-omit-frame-pointer)
        list(APPEND _link_options -fsanitize=undefined)
    endif()

    if(IDTECH4_ENABLE_TSAN)
        idtech4_require_sanitizer_support("thread" "libtsan.a")
        list(APPEND _compile_options -fsanitize=thread -fno-omit-frame-pointer)
        list(APPEND _link_options -fsanitize=thread)
    endif()

    set(${out_compile}
        "${_compile_options}"
        PARENT_SCOPE
    )
    set(${out_link}
        "${_link_options}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_apply_sanitizers target)
    idtech4_any_sanitizer_enabled(_sanitizer_enabled)
    if(NOT _sanitizer_enabled)
        return()
    endif()

    idtech4_collect_sanitizer_options(_compile_options _link_options)
    target_compile_options(${target} PRIVATE ${_compile_options})

    get_target_property(_target_type ${target} TYPE)
    if(_link_options AND NOT _target_type STREQUAL "STATIC_LIBRARY")
        target_link_options(${target} PRIVATE ${_link_options})
    endif()
endfunction()
