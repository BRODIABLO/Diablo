if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.28)
    project(ResistanceFloor VERSION 0.2.0 LANGUAGES C CXX RC ASM_MASM)
    include(CTest)
else()
    enable_language(RC)
    enable_language(ASM_MASM)
endif()

include(FetchContent)

if(NOT TARGET D2RLPlugin::D2RLPlugin)
    set(D2RLPLUGIN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(D2RLPLUGIN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        D2RLPlugin
        GIT_REPOSITORY https://github.com/D2RLoader/PluginSDK.git
        GIT_TAG 4933e2c42cb2592958cd0df3b6dc5003102252d1
    )
    FetchContent_MakeAvailable(D2RLPlugin)
endif()

if(NOT TARGET tomlplusplus::tomlplusplus)
    set(TOMLPLUSPLUS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(TOMLPLUSPLUS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(tomlplusplus)
endif()

if(NOT TARGET ResistanceFloorImGui)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG f401021d5a5d56fe2304056c391e78f81c8d4b8f
    )
    FetchContent_MakeAvailable(imgui)
    add_library(ResistanceFloorImGui STATIC
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    )
    target_include_directories(ResistanceFloorImGui PUBLIC
        "${imgui_SOURCE_DIR}"
    )
    target_compile_features(ResistanceFloorImGui PUBLIC cxx_std_20)
    target_compile_definitions(ResistanceFloorImGui PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )
    if(MSVC)
        target_compile_options(ResistanceFloorImGui PRIVATE /W3 /WX- /utf-8)
    endif()
endif()

add_library(ResistanceFloor SHARED
    floor_relays.asm
    plugin.cpp
    plugin.rc
)
target_compile_features(ResistanceFloor PRIVATE cxx_std_20)
target_compile_definitions(ResistanceFloor PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)
target_link_libraries(ResistanceFloor PRIVATE
    D2RLPlugin::D2RLPlugin
    ResistanceFloorImGui
    tomlplusplus::tomlplusplus
)
set_target_properties(ResistanceFloor PROPERTIES
    OUTPUT_NAME "ResistanceFloor"
    PREFIX ""
    RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE TRUE
)
d2rlplugin_embed_config(
    TARGET ResistanceFloor
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/../ruffneckk-resistance-floor.toml"
)

if(MSVC)
    target_compile_options(ResistanceFloor PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/WX>
        $<$<COMPILE_LANGUAGE:CXX>:/permissive->
        $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
        $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
    )
    target_link_options(ResistanceFloor PRIVATE /INCREMENTAL:NO /Brepro)
endif()

if(BUILD_TESTING)
    add_executable(ResistanceFloorTests policy_tests.cpp)
    target_compile_features(ResistanceFloorTests PRIVATE cxx_std_20)
    target_link_libraries(ResistanceFloorTests PRIVATE
        tomlplusplus::tomlplusplus
    )
    target_compile_definitions(ResistanceFloorTests PRIVATE
        RESISTANCE_FLOOR_PLUGIN_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp"
        RESISTANCE_FLOOR_RELAY_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/floor_relays.asm"
        RESISTANCE_FLOOR_TOML_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/../ruffneckk-resistance-floor.toml"
    )
    if(MSVC)
        target_compile_options(ResistanceFloorTests PRIVATE
            /W4 /WX /permissive- /EHsc /utf-8
        )
    endif()
    add_test(NAME resistance-floor-policy COMMAND ResistanceFloorTests)
endif()
