cmake_minimum_required(VERSION 3.28)
project(RuffnecKkMapSense VERSION 0.13.11 LANGUAGES C CXX RC)

include(FetchContent)

set(D2RLPLUGIN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(D2RLPLUGIN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TOMLPLUSPLUS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TOMLPLUSPLUS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    D2RLPlugin
    GIT_REPOSITORY https://github.com/D2RLoader/PluginSDK.git
    GIT_TAG 4933e2c42cb2592958cd0df3b6dc5003102252d1
)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG c3fcafdc10146beb5919319d0683e44e3c30d537
)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG f401021d5a5d56fe2304056c391e78f81c8d4b8f
)
FetchContent_MakeAvailable(
    D2RLPlugin
    tomlplusplus
    nlohmann_json
    minhook
    imgui
)

add_library(RuffnecKkMapSenseImGui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp"
)
target_include_directories(RuffnecKkMapSenseImGui PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
)
target_compile_features(RuffnecKkMapSenseImGui PUBLIC cxx_std_20)
target_compile_definitions(RuffnecKkMapSenseImGui PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)
target_link_libraries(RuffnecKkMapSenseImGui PUBLIC dxgi)
if(MSVC)
    target_compile_options(RuffnecKkMapSenseImGui PRIVATE /W3 /WX-)
endif()

add_library(RuffnecKkMapSense SHARED
    d3d12_imgui_host.cpp
    imgui_settings_panel.cpp
    mapsense_data_catalog.cpp
    navigation_engine.cpp
    navigation_level_catalog.cpp
    navigation_policy.cpp
    navigation_resolver.cpp
    native_automap_marker.cpp
    native_automap_missile.cpp
    native_automap_poi.cpp
    native_ui_state.cpp
    overlay_scene.cpp
    plugin.cpp
    reveal_engine.cpp
    sfilllocation_diagnostic.cpp
    plugin.rc
)
target_compile_features(RuffnecKkMapSense PRIVATE cxx_std_20)
target_compile_definitions(RuffnecKkMapSense PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)
target_link_libraries(RuffnecKkMapSense PRIVATE
    D2RLPlugin::D2RLPlugin
    RuffnecKkMapSenseImGui
    minhook
    tomlplusplus::tomlplusplus
    dxgi
    comctl32
    shell32
    windowscodecs
    ole32
    crypt32
)
set_target_properties(RuffnecKkMapSense PROPERTIES
    OUTPUT_NAME "RuffnecKkMapSense"
)

d2rlplugin_embed_config(
    TARGET RuffnecKkMapSense
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/../package/ruffneckk-mapsense.toml"
)

add_executable(RuffnecKkMapSenseTests
    mapsense_data_catalog.cpp
    mapsense_tests.cpp
    navigation_engine.cpp
    navigation_level_catalog.cpp
    navigation_policy.cpp
    overlay_scene.cpp
)
target_compile_features(RuffnecKkMapSenseTests PRIVATE cxx_std_20)
target_compile_definitions(RuffnecKkMapSenseTests PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)
target_link_libraries(RuffnecKkMapSenseTests PRIVATE
    D2RLPlugin::D2RLPlugin
    tomlplusplus::tomlplusplus
    nlohmann_json::nlohmann_json
)

if(MSVC)
    target_compile_options(RuffnecKkMapSense PRIVATE
        /W4 /WX /permissive- /EHsc /utf-8
    )
    target_link_options(RuffnecKkMapSense PRIVATE /Brepro)
    target_compile_options(RuffnecKkMapSenseTests PRIVATE
        /W4 /WX /permissive- /EHsc /utf-8
    )
endif()

enable_testing()
add_test(
    NAME ruffneckk-mapsense-policy
    COMMAND RuffnecKkMapSenseTests
        "${CMAKE_CURRENT_SOURCE_DIR}/../package/ruffneckk-mapsense.toml"
)
