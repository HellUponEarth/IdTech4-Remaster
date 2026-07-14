include_guard(GLOBAL)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(IDTECH4_INSTALL_CMAKE_DIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/IdTech4"
    CACHE STRING "Install destination for IdTech4 CMake package files."
)
option(IDTECH4_INSTALL_RUNTIME_TARGETS "Install built engine runtime/library targets when packaging a full build." ON)
option(IDTECH4_INSTALL_SDK_HEADERS "Install SDK header trees for downstream projects." ON)

function(idtech4_add_sdk_target)
    if(TARGET IdTech4SDK)
        return()
    endif()

    add_library(IdTech4SDK INTERFACE)
    add_library(IdTech4::SDK ALIAS IdTech4SDK)
    target_compile_features(IdTech4SDK INTERFACE cxx_std_11)
    target_include_directories(
        IdTech4SDK
        INTERFACE "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/neo>"
                  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/neo/curl/include>"
                  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/idtech4>"
                  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/idtech4/curl>"
    )
    idtech4_apply_compiler_profile(IdTech4SDK)
    set_target_properties(IdTech4SDK PROPERTIES EXPORT_NAME SDK FOLDER sdk)
endfunction()

function(idtech4_add_install_targets)
    idtech4_add_sdk_target()

    install(
        TARGETS IdTech4SDK
        EXPORT IdTech4Targets
        COMPONENT sdk
    )

    if(IDTECH4_INSTALL_SDK_HEADERS)
        install(
            DIRECTORY "${CMAKE_SOURCE_DIR}/neo/idlib/"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/idtech4/idlib"
            COMPONENT sdk
            FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
            PATTERN "*.inl"
        )
        install(
            DIRECTORY "${CMAKE_SOURCE_DIR}/neo/curl/include/"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/idtech4/curl"
            COMPONENT sdk
            FILES_MATCHING
            PATTERN "*.h"
        )
    endif()

    install(
        EXPORT IdTech4Targets
        NAMESPACE IdTech4::
        DESTINATION "${IDTECH4_INSTALL_CMAKE_DIR}"
        COMPONENT sdk
    )

    set(_config_file "${CMAKE_BINARY_DIR}/cmake/IdTech4Config.cmake")
    set(_version_file "${CMAKE_BINARY_DIR}/cmake/IdTech4ConfigVersion.cmake")

    configure_package_config_file(
        "${CMAKE_SOURCE_DIR}/cmake/IdTech4Config.cmake.in" "${_config_file}"
        INSTALL_DESTINATION "${IDTECH4_INSTALL_CMAKE_DIR}"
    )

    write_basic_package_version_file(
        "${_version_file}"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES "${_config_file}" "${_version_file}"
        DESTINATION "${IDTECH4_INSTALL_CMAKE_DIR}"
        COMPONENT sdk
    )

    install(
        FILES "${CMAKE_SOURCE_DIR}/CMakePresets.json" "${CMAKE_SOURCE_DIR}/vcpkg.json"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}/build-system"
        COMPONENT sdk
    )

    install(
        DIRECTORY "${CMAKE_SOURCE_DIR}/cmake/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/idtech4/cmake"
        COMPONENT sdk
        FILES_MATCHING
        PATTERN "*.cmake"
    )

    install(
        DIRECTORY "${CMAKE_SOURCE_DIR}/Documentation/"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
        COMPONENT docs
        OPTIONAL
    )

    if(TARGET idtech4_generate_docs)
        install(
            DIRECTORY "${CMAKE_BINARY_DIR}/docs/"
            DESTINATION "${CMAKE_INSTALL_DOCDIR}/generated"
            COMPONENT docs
            OPTIONAL
        )
    endif()

    if(IDTECH4_INSTALL_RUNTIME_TARGETS AND TARGET CurlLib)
        set(_runtime_targets
            CurlLib
            idLib
            TypeInfo
            DoomDLL
            Game
            Game-d3xp
        )
        if(IDTECH4_BUILD_MAYAIMPORT)
            list(APPEND _runtime_targets MayaImport)
        endif()

        install(
            TARGETS ${_runtime_targets}
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT runtime
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT runtime
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT runtime
        )
    endif()
endfunction()
