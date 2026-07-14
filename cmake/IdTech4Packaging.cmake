include_guard(GLOBAL)

set(IDTECH4_PACKAGING_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(idtech4_add_package_targets)
    if(NOT TARGET idtech4_tests)
        return()
    endif()

    set(_package_root "${CMAKE_BINARY_DIR}/package")
    set(_stage_dir "${_package_root}/stage")
    set(_archive_path "${_package_root}/idtech4-remaster-${CMAKE_SYSTEM_NAME}-$<CONFIG>.zip")
    set(_manifest_path "${_stage_dir}/manifest.txt")

    file(MAKE_DIRECTORY "${_package_root}")

    add_custom_target(
        idtech4_ci_package
        DEPENDS idtech4_tests idtech4_generate_docs
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${_stage_dir}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_stage_dir}/bin" "${_stage_dir}/lib"
                "${_stage_dir}/docs/generated" "${_stage_dir}/docs/source" "${_package_root}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:idtech4_gtest_unit_tests>" "${_stage_dir}/bin/"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:idtech4_catch2_unit_tests>" "${_stage_dir}/bin/"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:CurlLib>" "${_stage_dir}/lib/"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_BINARY_DIR}/docs" "${_stage_dir}/docs/generated"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/Documentation" "${_stage_dir}/docs/source"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_SOURCE_DIR}/CMakePresets.json"
                "${_stage_dir}/docs/generated/CMakePresets.json"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_SOURCE_DIR}/vcpkg.json"
                "${_stage_dir}/docs/generated/vcpkg.json"
        COMMAND
            "${CMAKE_COMMAND}" "-DOUTPUT_FILE=${_manifest_path}" "-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}"
            "-DCONFIG=$<CONFIG>" "-DSOURCE_DIR=${CMAKE_SOURCE_DIR}" "-DBINARY_DIR=${CMAKE_BINARY_DIR}" -P
            "${IDTECH4_PACKAGING_CMAKE_DIR}/IdTech4WritePackageManifest.cmake"
        COMMAND "${CMAKE_COMMAND}" -E tar cf "${_archive_path}" --format=zip -- "stage"
        WORKING_DIRECTORY "${_package_root}"
        COMMENT "Packaging CI build artifacts"
        VERBATIM
    )
    set_target_properties(idtech4_ci_package PROPERTIES FOLDER package)
endfunction()
