cmake_minimum_required(VERSION 3.26)

include(cmake/IdTech4Version.cmake)
idtech4_resolve_version()

project(
    doom
    VERSION ${IDTECH4_VERSION}
    LANGUAGES C CXX
)

if(WIN32)
    enable_language(RC)
endif()

include(cmake/IdTech4Targets.cmake)
include(cmake/IdTech4Compiler.cmake)
include(cmake/IdTech4Dependencies.cmake)
include(cmake/IdTech4Testing.cmake)
include(cmake/IdTech4BinarySize.cmake)
include(cmake/IdTech4Documentation.cmake)
include(cmake/IdTech4Packaging.cmake)
include(cmake/IdTech4Install.cmake)
include(cmake/IdTech4CPack.cmake)
include(cmake/IdTech4Formatting.cmake)

idtech4_report_dependency_management()
idtech4_report_compiler_profile()
idtech4_report_build_performance()

if(IDTECH4_BUILD_ENGINE_TARGETS)
    idtech4_add_static_library(CurlLib neo/curllib.vcxproj)
    idtech4_configure_curllib(CurlLib)

    idtech4_add_static_library(idLib neo/idlib.vcxproj)
    idtech4_configure_idlib(idLib)

    idtech4_add_executable(TypeInfo neo/typeinfo.vcxproj)
    idtech4_configure_typeinfo(TypeInfo idLib)

    idtech4_add_executable(DoomDLL neo/doomdll.vcxproj WIN32)
    idtech4_configure_doomdll(DoomDLL CurlLib idLib TypeInfo)

    idtech4_add_game_library(
        Game
        neo/game.vcxproj
        game/game.def
        idLib
        TypeInfo
    )
    idtech4_add_game_library(
        Game-d3xp
        neo/game-d3xp.vcxproj
        d3xp/game.def
        idLib
        TypeInfo
    )
    idtech4_configure_d3xp_game(Game-d3xp)

    idtech4_add_maya_import(MayaImport neo/MayaImport.vcxproj idLib)

    set_property(TARGET idLib CurlLib PROPERTY FOLDER libs)
    set_property(TARGET TypeInfo DoomDLL PROPERTY FOLDER exes)
    set_property(TARGET Game Game-d3xp MayaImport PROPERTY FOLDER dlls)
endif()

idtech4_write_legacy_solution()
idtech4_add_tests()
idtech4_add_binary_size_target()
idtech4_add_emscripten_smoke_target()
idtech4_add_documentation_targets()
idtech4_add_install_targets()
idtech4_add_coverage_target()
idtech4_add_package_targets()
idtech4_configure_cpack()
idtech4_add_format_targets()
