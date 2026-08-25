if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.28)
    project(CastTriggers LANGUAGES CXX RC)
    include(CTest)
else()
    enable_language(RC)
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

add_library(CastTriggers SHARED plugin.cpp plugin.rc)
target_compile_features(CastTriggers PRIVATE cxx_std_20)
target_link_libraries(CastTriggers PRIVATE
    D2RLPlugin::D2RLPlugin
    tomlplusplus::tomlplusplus
)
set_target_properties(CastTriggers PROPERTIES
    OUTPUT_NAME "d2rl-ruffneckk-cast-triggers"
    PREFIX ""
    RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE TRUE
)
d2rlplugin_embed_config(
    TARGET CastTriggers
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/../ruffneckk-cast-triggers.toml"
)

if(MSVC)
    target_compile_options(CastTriggers PRIVATE /utf-8 /W4 /permissive-)
    target_link_options(CastTriggers PRIVATE /INCREMENTAL:NO /Brepro)
endif()

if(BUILD_TESTING)
    add_executable(CastTriggersTests policy_tests.cpp)
    target_compile_features(CastTriggersTests PRIVATE cxx_std_20)
    target_link_libraries(
        CastTriggersTests PRIVATE tomlplusplus::tomlplusplus)
    target_compile_definitions(CastTriggersTests PRIVATE
        CAST_TRIGGERS_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/../ruffneckk-cast-triggers.toml"
        CAST_TRIGGERS_PLUGIN_FILE="${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp"
    )
    if(MSVC)
        target_compile_options(
            CastTriggersTests PRIVATE /utf-8 /W4 /permissive-)
    endif()
    add_test(NAME cast-triggers-policy COMMAND CastTriggersTests)
endif()
