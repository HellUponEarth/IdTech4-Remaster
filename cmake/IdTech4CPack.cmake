include_guard(GLOBAL)

function(idtech4_configure_cpack)
    set(CPACK_PACKAGE_NAME "idtech4-remaster")
    set(CPACK_PACKAGE_VENDOR "IdTech4 Remaster")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "IdTech4 Remaster engine build artifacts and SDK")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME
        "idtech4-remaster-${IDTECH4_VERSION_LABEL}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}"
    )
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "IdTech4 Remaster")
    set(CPACK_VERBATIM_VARIABLES ON)

    set(CPACK_COMPONENTS_ALL runtime sdk docs)
    set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Runtime")
    set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Built engine binaries and libraries.")
    set(CPACK_COMPONENT_SDK_DISPLAY_NAME "Engine SDK")
    set(CPACK_COMPONENT_SDK_DESCRIPTION
        "Headers, exported CMake targets, presets, and dependency manifests for downstream projects."
    )
    set(CPACK_COMPONENT_DOCS_DISPLAY_NAME "Documentation")
    set(CPACK_COMPONENT_DOCS_DESCRIPTION "Source and generated build-system documentation.")
    set(CPACK_COMPONENT_SDK_DEPENDS docs)

    set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
    set(CPACK_DEB_COMPONENT_INSTALL ON)
    set(CPACK_RPM_COMPONENT_INSTALL ON)
    set(CPACK_NSIS_COMPONENT_INSTALL ON)
    set(CPACK_WIX_COMPONENT_INSTALL ON)

    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "IdTech4 Remaster Maintainers")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
    set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
    set(CPACK_RPM_PACKAGE_RELEASE "1")
    set(CPACK_NSIS_DISPLAY_NAME "IdTech4 Remaster")
    set(CPACK_NSIS_PACKAGE_NAME "IdTech4 Remaster")
    set(CPACK_WIX_UPGRADE_GUID "5F02E1D4-872B-45A7-B685-576C5918E1B9")

    if(WIN32)
        set(CPACK_GENERATOR "ZIP;STGZ;NSIS;WIX")
    elseif(UNIX AND NOT APPLE)
        set(CPACK_GENERATOR "ZIP;TGZ;STGZ;DEB;RPM")
    else()
        set(CPACK_GENERATOR "ZIP;TGZ;STGZ;DragNDrop")
    endif()

    include(CPack)
endfunction()
