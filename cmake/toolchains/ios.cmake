set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_ARCHITECTURES
    arm64
    CACHE STRING "iOS architectures"
)
set(CMAKE_OSX_DEPLOYMENT_TARGET
    14.0
    CACHE STRING "Minimum iOS deployment target"
)
set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH
    NO
    CACHE BOOL "Build all requested iOS architectures"
)
