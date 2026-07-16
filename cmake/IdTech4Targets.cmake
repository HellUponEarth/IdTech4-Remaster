include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/IdTech4Coverage.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/IdTech4Sanitizers.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/IdTech4StaticAnalysis.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/IdTech4BuildPerformance.cmake")

option(IDTECH4_STRICT_MISSING_VCXPROJ_FILES "Fail configure when a legacy .vcxproj references missing files." OFF)
option(IDTECH4_BUILD_MAYAIMPORT "Build the MayaImport plug-in by default. Requires the legacy Maya SDK." OFF)
option(IDTECH4_ENABLE_EDITOR_CVARS "Build with editor/tool CVar support explicitly enabled." OFF)
option(IDTECH4_BUILD_DEDICATED_SERVER "Build single-config generators with dedicated server definitions." OFF)
option(IDTECH4_USE_LEGACY_RUNTIME_LAYOUT "Place launchable runtime artifacts in the original Doom 3 folder layout." ON)

if(IDTECH4_ENABLE_EDITOR_CVARS AND IDTECH4_BUILD_DEDICATED_SERVER)
    message(FATAL_ERROR "Editor CVar support and dedicated server mode cannot be enabled together.")
endif()

function(idtech4_neo_dir out_var)
    set(${out_var}
        "${CMAKE_CURRENT_SOURCE_DIR}/neo"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_platform_dir out_var)
    if(CMAKE_GENERATOR_PLATFORM)
        set(_platform "${CMAKE_GENERATOR_PLATFORM}")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_platform "x64")
    else()
        set(_platform "Win32")
    endif()

    set(${out_var}
        "${_platform}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_output_root out_var)
    idtech4_platform_dir(_platform)
    set(${out_var}
        "${CMAKE_CURRENT_SOURCE_DIR}/build/${_platform}/$<CONFIG>"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_set_common_output target)
    idtech4_output_root(_output_root)
    set_target_properties(
        ${target}
        PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${_output_root}"
                   LIBRARY_OUTPUT_DIRECTORY "${_output_root}"
                   RUNTIME_OUTPUT_DIRECTORY "${_output_root}"
    )
endfunction()

function(idtech4_set_output_subdir target subdir)
    idtech4_output_root(_output_root)
    set_target_properties(
        ${target}
        PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${_output_root}/${subdir}"
                   LIBRARY_OUTPUT_DIRECTORY "${_output_root}/${subdir}"
                   RUNTIME_OUTPUT_DIRECTORY "${_output_root}/${subdir}"
    )
endfunction()

function(idtech4_legacy_runtime_dir out_var subdir)
    if(subdir)
        set(_runtime_dir "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}")
    else()
        set(_runtime_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    set(${out_var}
        "${_runtime_dir}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_set_legacy_runtime_output target subdir)
    if(NOT IDTECH4_USE_LEGACY_RUNTIME_LAYOUT)
        return()
    endif()

    idtech4_legacy_runtime_dir(_runtime_dir "${subdir}")
    set(_runtime_output_dir "${_runtime_dir}/$<0:>")
    set_target_properties(
        ${target}
        PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${_runtime_output_dir}"
                   LIBRARY_OUTPUT_DIRECTORY "${_runtime_output_dir}"
                   RUNTIME_OUTPUT_DIRECTORY "${_runtime_output_dir}"
                   PDB_OUTPUT_DIRECTORY "${_runtime_output_dir}"
    )
endfunction()

function(idtech4_normalize_vcxproj_path out_var path)
    string(REPLACE "\\" "/" _path "${path}")
    set(${out_var}
        "${_path}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_import_vcxproj_sources target vcxproj)
    get_filename_component(
        _vcxproj_abs
        "${vcxproj}"
        ABSOLUTE
        BASE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    get_filename_component(_vcxproj_dir "${_vcxproj_abs}" DIRECTORY)
    file(READ "${_vcxproj_abs}" _vcxproj_xml)
    idtech4_platform_dir(_platform)

    set(_files)
    set(_missing)
    set(_always_excluded
        "idlib/math/Simd_AltiVec.cpp"
        "tools/debugger/debugger.cpp"
        "tools/debugger/DebuggerApp.cpp"
        "tools/debugger/DebuggerBreakpoint.cpp"
        "tools/debugger/DebuggerClient.cpp"
        "tools/debugger/DebuggerFindDlg.cpp"
        "tools/debugger/DebuggerQuickWatchDlg.cpp"
        "tools/debugger/DebuggerScript.cpp"
        "tools/debugger/DebuggerServer.cpp"
        "tools/debugger/DebuggerWindow.cpp"
    )
    set(_x64_excluded
        "idlib/math/Simd_3DNow.cpp"
        "idlib/math/Simd_MMX.cpp"
        "idlib/math/Simd_SSE.cpp"
        "idlib/math/Simd_SSE2.cpp"
        "idlib/math/Simd_SSE3.cpp"
        "sys/win32/win_taskkeyhook.cpp"
    )

    set(_source_tags ClCompile ClInclude None)
    if(WIN32)
        list(APPEND _source_tags ResourceCompile)
    endif()

    foreach(_tag IN LISTS _source_tags)
        string(REGEX MATCHALL "<${_tag}[^>]*Include=\"[^\"]+\"" _matches "${_vcxproj_xml}")

        foreach(_match IN LISTS _matches)
            string(REGEX REPLACE "^.*Include=\"([^\"]+)\".*$" "\\1" _legacy_rel "${_match}")
            idtech4_normalize_vcxproj_path(_legacy_rel "${_legacy_rel}")

            if(_legacy_rel IN_LIST _always_excluded)
                continue()
            endif()

            if(NOT IDTECH4_ENABLE_EDITOR_CVARS)
                if(_legacy_rel MATCHES "^tools/(af|comafx|common|decl|guied|materialeditor|particle|pda|radiant|script|sound)/")
                    continue()
                endif()
                if(_legacy_rel MATCHES "^sys/win32/rc/(AFEditor|Common|Debugger|DeclEditor|GuiEd|MaterialEditor|ParticleEditor|PDAEditor|PropTree|Radiant|ScriptEditor|SoundEditor)\\.")
                    continue()
                endif()
            endif()

            if(_platform STREQUAL "x64" AND _legacy_rel IN_LIST _x64_excluded)
                continue()
            endif()

            set(_candidate "${_vcxproj_dir}/${_legacy_rel}")
            if(EXISTS "${_candidate}")
                list(APPEND _files "${_candidate}")
            else()
                list(APPEND _missing "${_legacy_rel}")
            endif()
        endforeach()
    endforeach()

    if(_files)
        target_sources(${target} PRIVATE ${_files})
        source_group(TREE "${_vcxproj_dir}" FILES ${_files})
    endif()

    if(_missing)
        list(LENGTH _missing _missing_count)
        if(IDTECH4_STRICT_MISSING_VCXPROJ_FILES)
            message(FATAL_ERROR "${target}: ${_missing_count} files referenced by ${vcxproj} are missing: ${_missing}")
        else()
            message(
                WARNING
                    "${target}: skipped ${_missing_count} files referenced by ${vcxproj} because they are not present in this checkout."
            )
        endif()
    endif()
endfunction()

function(idtech4_apply_common_settings target)
    idtech4_neo_dir(_neo_dir)

    target_compile_features(${target} PRIVATE cxx_std_11)
    set_target_properties(
        ${target}
        PROPERTIES CXX_STANDARD 11
                   CXX_STANDARD_REQUIRED ON
                   CXX_EXTENSIONS OFF
                   VS_DEBUGGER_WORKING_DIRECTORY "${_neo_dir}"
    )

    target_include_directories(${target} PRIVATE "${_neo_dir}")
    target_compile_definitions(
        ${target}
        PRIVATE
            $<$<PLATFORM_ID:Windows>:WIN32;_WINDOWS;_CRT_SECURE_NO_DEPRECATE;_CRT_NONSTDC_NO_DEPRECATE>
            $<$<CONFIG:Debug>:_DEBUG>
            "$<$<STREQUAL:$<CONFIG>,Debug with inlines>:_DEBUG;_INLINEDEBUG>"
            "$<$<STREQUAL:$<CONFIG>,Debug with inlines and memory log>:_DEBUG;_INLINEDEBUG;ID_REDIRECT_NEWDELETE;ID_DEBUG_MEMORY;ID_DEBUG_UNINITIALIZED_MEMORY>"
            "$<$<STREQUAL:$<CONFIG>,Dedicated Debug>:_DEBUG;ID_DEDICATED>"
            "$<$<STREQUAL:$<CONFIG>,Dedicated Debug with inlines>:_DEBUG;ID_DEDICATED;_INLINEDEBUG>"
            "$<$<STREQUAL:$<CONFIG>,Dedicated Release>:NDEBUG;ID_DEDICATED>"
            $<$<CONFIG:Release>:NDEBUG>
            $<$<BOOL:${IDTECH4_ENABLE_EDITOR_CVARS}>:ID_ALLOW_TOOLS>
            $<$<BOOL:${IDTECH4_BUILD_DEDICATED_SERVER}>:ID_DEDICATED>
    )

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_compile_definitions(${target} PRIVATE $<$<PLATFORM_ID:Windows>:WIN64;_WIN64>)
    else()
        target_compile_definitions(${target} PRIVATE $<$<PLATFORM_ID:Windows>:_USE_32BIT_TIME_T>)
    endif()

    if(MSVC)
        target_compile_options(
            ${target}
            PRIVATE /W4
                    /FS
                    $<$<CONFIG:Debug>:/Od;/RTC1;/Zi>
                    "$<$<STREQUAL:$<CONFIG>,Debug with inlines>:/Od;/RTC1;/Zi;/Ob1>"
                    "$<$<STREQUAL:$<CONFIG>,Debug with inlines and memory log>:/Od;/RTC1;/Zi;/Ob1>"
                    "$<$<STREQUAL:$<CONFIG>,Dedicated Debug>:/Od;/RTC1;/Zi>"
                    "$<$<STREQUAL:$<CONFIG>,Dedicated Debug with inlines>:/Od;/RTC1;/Zi;/Ob1>"
                    $<$<CONFIG:Release>:/O2;/Ob2;/Oi;/Oy;/GF;/Gy;/Zi>
                    "$<$<STREQUAL:$<CONFIG>,Dedicated Release>:/O2;/Ob2;/Oi;/Oy;/GF;/Gy;/Zi>"
        )
        set_property(
            TARGET ${target}
            PROPERTY
                MSVC_RUNTIME_LIBRARY
                "MultiThreaded$<$<OR:$<CONFIG:Debug>,$<STREQUAL:$<CONFIG>,Debug with inlines>,$<STREQUAL:$<CONFIG>,Debug with inlines and memory log>,$<STREQUAL:$<CONFIG>,Dedicated Debug>,$<STREQUAL:$<CONFIG>,Dedicated Debug with inlines>>:Debug>"
        )
    endif()

    idtech4_set_common_output(${target})
    idtech4_apply_coverage(${target})
    idtech4_apply_sanitizers(${target})
    idtech4_apply_static_analysis(${target})
    idtech4_apply_compiler_profile(${target})
    idtech4_apply_build_performance(${target})
endfunction()

function(idtech4_add_static_library target vcxproj)
    add_library(${target} STATIC)
    idtech4_import_vcxproj_sources(${target} "${vcxproj}")
    idtech4_apply_common_settings(${target})
endfunction()

function(idtech4_add_executable target vcxproj)
    set(_exe_args ${ARGN})
    if("WIN32" IN_LIST _exe_args)
        add_executable(${target} WIN32)
    else()
        add_executable(${target})
    endif()

    idtech4_import_vcxproj_sources(${target} "${vcxproj}")
    idtech4_apply_common_settings(${target})
endfunction()

function(idtech4_link_idlib target scope idlib_target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        idtech4_neo_dir(_neo_dir)
        target_sources(${target} PRIVATE "${_neo_dir}/idlib/Dict.cpp")
        target_link_libraries(${target} ${scope} "$<LINK_GROUP:RESCAN,${idlib_target}>")
    else()
        target_link_libraries(${target} ${scope} ${idlib_target})
    endif()
endfunction()

function(
    idtech4_add_game_library
    target
    vcxproj
    module_definition
    idlib_target
    typeinfo_target
)
    idtech4_platform_dir(_platform)
    if(_platform STREQUAL "x64")
        set(_game_dll_name "gamex64")
    else()
        set(_game_dll_name "gamex86")
    endif()

    idtech4_neo_dir(_neo_dir)
    add_library(${target} SHARED)
    idtech4_import_vcxproj_sources(${target} "${vcxproj}")
    idtech4_apply_common_settings(${target})

    target_compile_definitions(${target} PRIVATE __DOOM__ GAME_DLL)
    idtech4_link_idlib(${target} PRIVATE ${idlib_target})
    add_dependencies(${target} ${typeinfo_target})

    set_target_properties(${target} PROPERTIES OUTPUT_NAME "${_game_dll_name}" PREFIX "")
    idtech4_set_legacy_runtime_output(${target} base)

    if(MSVC)
        target_link_options(${target} PRIVATE "/DEF:${_neo_dir}/${module_definition}")
    endif()

    add_custom_command(
        TARGET ${target}
        PRE_BUILD
        COMMAND $<TARGET_FILE:${typeinfo_target}>
        WORKING_DIRECTORY "${_neo_dir}"
        COMMENT "Running TypeInfo before ${target}"
        VERBATIM
    )
endfunction()

function(idtech4_configure_curllib target)
    idtech4_neo_dir(_neo_dir)
    target_include_directories(${target} PRIVATE "${_neo_dir}/curl/include")
    target_compile_definitions(${target} PRIVATE USRDLL _USRDLL CURLLIB_EXPORTS)
    target_sources(${target} PRIVATE "${_neo_dir}/curl/lib/strtoofft.c")
    if(UNIX AND NOT APPLE)
        set_target_properties(
            ${target}
            PROPERTIES C_STANDARD 90
                       C_STANDARD_REQUIRED ON
                       C_EXTENSIONS ON
        )
        target_compile_definitions(
            ${target}
            PRIVATE STDC_HEADERS=1
                    HAVE_ARPA_INET_H=1
                    HAVE_FCNTL_H=1
                    HAVE_GETHOSTBYADDR=1
                    HAVE_GETHOSTBYNAME=1
                    HAVE_GETTIMEOFDAY=1
                    HAVE_INET_ADDR=1
                    HAVE_INET_NTOA=1
                    HAVE_LONGLONG=1
                    HAVE_MEMORY_H=1
                    HAVE_NETDB_H=1
                    HAVE_NET_IF_H=1
                    HAVE_NETINET_IN_H=1
                    HAVE_O_NONBLOCK=1
                    HAVE_SELECT=1
                    HAVE_SOCKET=1
                    HAVE_STDLIB_H=1
                    HAVE_STRING_H=1
                    HAVE_STRINGS_H=1
                    HAVE_SYS_IOCTL_H=1
                    HAVE_SYS_SOCKET_H=1
                    HAVE_SYS_TIME_H=1
                    HAVE_SYS_TYPES_H=1
                    HAVE_TIME_H=1
                    HAVE_UNISTD_H=1
                    NEED_REENTRANT=1
                    OS=\"Linux\"
                    SIZEOF_CURL_OFF_T=8
        )
    endif()
    if(MSVC)
        target_compile_options(${target} PRIVATE /W3)
    endif()
endfunction()

function(idtech4_configure_idlib target)

endfunction()

function(idtech4_configure_typeinfo target idlib_target)
    target_compile_definitions(${target} PRIVATE ID_ENABLE_CURL=0 ID_TYPEINFO __DOOM_DLL__)
    idtech4_link_idlib(${target} PRIVATE ${idlib_target})
endfunction()

function(
    idtech4_configure_doomdll
    target
    curllib_target
    idlib_target
    typeinfo_target
)
    idtech4_neo_dir(_neo_dir)
    idtech4_platform_dir(_platform)

    target_compile_definitions(${target} PRIVATE __DOOM__ __DOOM_DLL__)
    target_sources(${target} PRIVATE "${_neo_dir}/renderer/cg_explicit.cpp")
    if(NOT IDTECH4_ENABLE_EDITOR_CVARS)
        target_sources(${target} PRIVATE "${_neo_dir}/tools/guied/GEWindowWrapper_stub.cpp")
    endif()
    target_include_directories(
        ${target}
        PRIVATE
            "${_neo_dir}/sound/vorbis/include"
            "${_neo_dir}/sound/OggVorbis"
            "${_neo_dir}/sound/OggVorbis/vorbissrc"
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-narrowing>)
    endif()
    target_link_libraries(${target} PRIVATE ${curllib_target})
    idtech4_link_idlib(${target} PRIVATE ${idlib_target})
    add_dependencies(${target} ${typeinfo_target})
    set(_dedicated_config
        "$<OR:$<BOOL:${IDTECH4_BUILD_DEDICATED_SERVER}>,$<STREQUAL:$<CONFIG>,Dedicated Debug>,$<STREQUAL:$<CONFIG>,Dedicated Debug with inlines>,$<STREQUAL:$<CONFIG>,Dedicated Release>>"
    )
    set_target_properties(${target} PROPERTIES OUTPUT_NAME "$<IF:${_dedicated_config},DedServer,Doom3>")
    idtech4_set_legacy_runtime_output(${target} "")

    if(WIN32)
        if(_platform STREQUAL "x64")
            set(_directx_platform "x64")
        else()
            set(_directx_platform "x86")
        endif()

        target_link_directories(
            ${target}
            PRIVATE
            "${_neo_dir}/openal/lib"
            "${_neo_dir}/sys/win32/dongle"
        )
        if(MSVC)
            target_include_directories(${target} PRIVATE "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include")
            target_link_directories(${target} PRIVATE "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/${_directx_platform}")
        endif()
        target_link_libraries(
            ${target}
            PRIVATE dbghelp
                    dinput8
                    dsound
                    dxguid
                    glu32
                    iphlpapi
                    ksuser
                    odbc32
                    odbccp32
                    opengl32
                    wbemuuid
                    winmm
                    wsock32
        )
        if(MSVC)
            target_link_libraries(${target} PRIVATE DxErr eaxguid)
        endif()

        if(MSVC)
            target_compile_options(${target} PRIVATE /W3)
            target_link_options(${target} PRIVATE /STACK:16777216,16777216 /LARGEADDRESSAWARE)
        endif()
    endif()
endfunction()

function(idtech4_configure_d3xp_game target)
    target_compile_definitions(${target} PRIVATE _D3XP CTF)
    if(IDTECH4_USE_LEGACY_RUNTIME_LAYOUT)
        idtech4_set_legacy_runtime_output(${target} d3xp)
    else()
        idtech4_set_output_subdir(${target} d3xp)
    endif()
endfunction()

function(idtech4_add_maya_import target vcxproj idlib_target)
    idtech4_neo_dir(_neo_dir)

    set(_maya_header "${_neo_dir}/MayaImport/Maya5.0/maya.h")
    set(_maya_lib_dir "${_neo_dir}/MayaImport/maya5.0/libs")
    set(_maya_libs Foundation OpenMaya OpenMayaAnim)
    set(_missing_maya_sdk_files)

    if(NOT EXISTS "${_maya_header}")
        list(APPEND _missing_maya_sdk_files "${_maya_header}")
    endif()

    foreach(_maya_lib IN LISTS _maya_libs)
        if(NOT EXISTS "${_maya_lib_dir}/${_maya_lib}.lib")
            list(APPEND _missing_maya_sdk_files "${_maya_lib_dir}/${_maya_lib}.lib")
        endif()
    endforeach()

    if(_missing_maya_sdk_files)
        if(IDTECH4_BUILD_MAYAIMPORT)
            message(FATAL_ERROR "MayaImport requires the legacy Maya 5.0 SDK files: ${_missing_maya_sdk_files}")
        endif()

        add_custom_target(
            ${target}
            COMMAND "${CMAKE_COMMAND}" -E echo
                    "MayaImport skipped: legacy Maya 5.0 SDK files are not present in neo/MayaImport/Maya5.0 and neo/MayaImport/maya5.0/libs."
            VERBATIM
        )
        return()
    endif()

    if(IDTECH4_BUILD_MAYAIMPORT)
        add_library(${target} SHARED)
    else()
        add_library(${target} SHARED EXCLUDE_FROM_ALL )
    endif()

    idtech4_import_vcxproj_sources(${target} "${vcxproj}")
    idtech4_apply_common_settings(${target})

    target_include_directories(${target} PRIVATE "${_neo_dir}/mssdk/include" "${_neo_dir}/MayaImport/Maya5.0")
    target_compile_definitions(${target} PRIVATE _USRDLL MAYAIMPORT_EXPORTS MAYA_IMPORT REQUIRE_IOSTREAM)
    target_link_libraries(${target} PRIVATE ${idlib_target})

    if(WIN32)
        target_link_directories(${target} PRIVATE "${_maya_lib_dir}")
        target_link_libraries(${target} PRIVATE ${_maya_libs})
    endif()

    if(MSVC)
        target_compile_options(${target} PRIVATE /W3)
        target_link_options(${target} PRIVATE "/DEF:${_neo_dir}/MayaImport/mayaimport.def")
    endif()
endfunction()

function(
    idtech4_append_sln_project_configs
    out_var
    project_guid
    platform
    include_build
)
    set(_text "")
    foreach(
        _config IN
        ITEMS "Debug"
              "Debug with inlines"
              "Debug with inlines and memory log"
              "Dedicated Debug"
              "Dedicated Debug with inlines"
              "Dedicated Release"
              "RelWithDebInfo"
              "Release"
    )
        string(APPEND _text "\t\t{${project_guid}}.${_config}|${platform}.ActiveCfg = ${_config}|${platform}\n")
        if(include_build)
            string(APPEND _text "\t\t{${project_guid}}.${_config}|${platform}.Build.0 = ${_config}|${platform}\n")
        endif()
    endforeach()

    set(${out_var}
        "${${out_var}}${_text}"
        PARENT_SCOPE
    )
endfunction()

function(idtech4_write_legacy_solution)
    if(NOT CMAKE_GENERATOR MATCHES "Visual Studio")
        return()
    endif()

    idtech4_platform_dir(_platform)
    set(_configs "")
    foreach(
        _config IN
        ITEMS "Debug"
              "Debug with inlines"
              "Debug with inlines and memory log"
              "Dedicated Debug"
              "Dedicated Debug with inlines"
              "Dedicated Release"
              "RelWithDebInfo"
              "Release"
    )
        string(APPEND _configs "\t\t${_config}|${_platform} = ${_config}|${_platform}\n")
    endforeach()

    set(_project_configs "")
    idtech4_append_sln_project_configs(_project_configs "EAB27FC7-F6FF-3AA5-BACF-12682BB9F187" "${_platform}" TRUE)
    idtech4_append_sln_project_configs(_project_configs "0626C65F-7724-327D-AD35-E519A5A356F6" "${_platform}" TRUE)
    idtech4_append_sln_project_configs(_project_configs "3D773AFD-79C9-318F-A756-806B0DFA0559" "${_platform}" TRUE)
    idtech4_append_sln_project_configs(_project_configs "A41ADB15-7D86-3F36-8050-BE215649BE3D" "${_platform}" TRUE)
    idtech4_append_sln_project_configs(_project_configs "2B47F442-45E4-3F7C-8E47-DE3CEEC2AACE" "${_platform}" TRUE)
    idtech4_append_sln_project_configs(
        _project_configs "F099A9A9-15AA-3330-9F7D-AFD1473DE324" "${_platform}" "${IDTECH4_BUILD_MAYAIMPORT}"
    )
    idtech4_append_sln_project_configs(_project_configs "53DA5741-2BA5-3258-8A3C-C58400B509DD" "${_platform}" TRUE)

    set(_solution
        "Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 18
VisualStudioVersion = 18.0.36248.0
MinimumVisualStudioVersion = 10.0.40219.1
Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"libs\", \"libs\", \"{347D107C-D787-4408-A60D-86FA45997F9B}\"
EndProject
Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"exes\", \"exes\", \"{003B01AB-152D-45C8-BF45-E5A035042D7F}\"
EndProject
Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"dlls\", \"dlls\", \"{1E2B3940-65F8-4D8F-9EEE-85E94EBBC6DF}\"
EndProject
Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"Game\", \"Game.vcxproj\", \"{EAB27FC7-F6FF-3AA5-BACF-12682BB9F187}\"
EndProject
Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"idLib\", \"idLib.vcxproj\", \"{0626C65F-7724-327D-AD35-E519A5A356F6}\"
EndProject
Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"CurlLib\", \"CurlLib.vcxproj\", \"{3D773AFD-79C9-318F-A756-806B0DFA0559}\"
EndProject
Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"TypeInfo\", \"TypeInfo.vcxproj\", \"{A41ADB15-7D86-3F36-8050-BE215649BE3D}\"
EndProject
Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"DoomDLL\", \"DoomDLL.vcxproj\", \"{2B47F442-45E4-3F7C-8E47-DE3CEEC2AACE}\"
EndProject
Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"MayaImport\", \"MayaImport.vcxproj\", \"{F099A9A9-15AA-3330-9F7D-AFD1473DE324}\"
EndProject
Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"Game-d3xp\", \"Game-d3xp.vcxproj\", \"{53DA5741-2BA5-3258-8A3C-C58400B509DD}\"
EndProject
Global
\tGlobalSection(SolutionConfigurationPlatforms) = preSolution
${_configs}\tEndGlobalSection
\tGlobalSection(ProjectConfigurationPlatforms) = postSolution
${_project_configs}\tEndGlobalSection
\tGlobalSection(SolutionProperties) = preSolution
\t\tHideSolutionNode = FALSE
\tEndGlobalSection
\tGlobalSection(NestedProjects) = preSolution
\t\t{0626C65F-7724-327D-AD35-E519A5A356F6} = {347D107C-D787-4408-A60D-86FA45997F9B}
\t\t{3D773AFD-79C9-318F-A756-806B0DFA0559} = {347D107C-D787-4408-A60D-86FA45997F9B}
\t\t{A41ADB15-7D86-3F36-8050-BE215649BE3D} = {003B01AB-152D-45C8-BF45-E5A035042D7F}
\t\t{2B47F442-45E4-3F7C-8E47-DE3CEEC2AACE} = {003B01AB-152D-45C8-BF45-E5A035042D7F}
\t\t{EAB27FC7-F6FF-3AA5-BACF-12682BB9F187} = {1E2B3940-65F8-4D8F-9EEE-85E94EBBC6DF}
\t\t{F099A9A9-15AA-3330-9F7D-AFD1473DE324} = {1E2B3940-65F8-4D8F-9EEE-85E94EBBC6DF}
\t\t{53DA5741-2BA5-3258-8A3C-C58400B509DD} = {1E2B3940-65F8-4D8F-9EEE-85E94EBBC6DF}
\tEndGlobalSection
EndGlobal
"
    )

    file(
        GENERATE
        OUTPUT "${CMAKE_BINARY_DIR}/doom.sln"
        CONTENT "${_solution}"
    )
endfunction()
