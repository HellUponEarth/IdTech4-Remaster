set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_VERSION
    24
    CACHE STRING "Android API level"
)
set(CMAKE_ANDROID_ARCH_ABI
    arm64-v8a
    CACHE STRING "Android ABI"
)
set(CMAKE_ANDROID_STL_TYPE
    c++_static
    CACHE STRING "Android STL"
)

if(NOT CMAKE_ANDROID_NDK)
    if(DEFINED ENV{ANDROID_NDK_ROOT})
        set(CMAKE_ANDROID_NDK
            "$ENV{ANDROID_NDK_ROOT}"
            CACHE PATH "Android NDK root"
        )
    elseif(DEFINED ENV{ANDROID_NDK_HOME})
        set(CMAKE_ANDROID_NDK
            "$ENV{ANDROID_NDK_HOME}"
            CACHE PATH "Android NDK root"
        )
    endif()
endif()

if(NOT CMAKE_ANDROID_NDK)
    message(FATAL_ERROR "Set CMAKE_ANDROID_NDK, ANDROID_NDK_ROOT, or ANDROID_NDK_HOME before using this toolchain.")
endif()
