cmake_minimum_required(VERSION 3.28)
project(ExtendedActLevelIds LANGUAGES CXX RC)

include(CTest)
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

if(NOT TARGET nlohmann_json::nlohmann_json)
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_library(ExtendedActLevelIds SHARED plugin.cpp plugin.rc)
target_compile_features(ExtendedActLevelIds PRIVATE cxx_std_20)
target_link_libraries(ExtendedActLevelIds PRIVATE
    D2RLPlugin::D2RLPlugin
    nlohmann_json::nlohmann_json
)
set_target_properties(ExtendedActLevelIds PROPERTIES
    OUTPUT_NAME "d2rl-ruffneckk-extended-act-level-ids"
    PREFIX ""
    RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE TRUE
)
d2rlplugin_embed_config(
    TARGET ExtendedActLevelIds
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/../ruffneckk-extended-act-level-ids.json"
)

if(MSVC)
    target_compile_options(ExtendedActLevelIds PRIVATE /utf-8 /W4 /permissive-)
    target_link_options(ExtendedActLevelIds PRIVATE /INCREMENTAL:NO /Brepro)
endif()

if(BUILD_TESTING)
    add_executable(ExtendedActLevelIdsTests policy_tests.cpp)
    target_compile_features(ExtendedActLevelIdsTests PRIVATE cxx_std_20)
    target_link_libraries(ExtendedActLevelIdsTests PRIVATE
        nlohmann_json::nlohmann_json
    )
    target_compile_definitions(ExtendedActLevelIdsTests PRIVATE
        EXTENDED_ACT_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/../ruffneckk-extended-act-level-ids.json"
        EXTENDED_ACT_PLUGIN_FILE="${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp"
    )
    if(MSVC)
        target_compile_options(ExtendedActLevelIdsTests PRIVATE /utf-8 /W4 /permissive-)
    endif()
    add_test(NAME extended-act-level-ids-policy COMMAND ExtendedActLevelIdsTests)
endif()
