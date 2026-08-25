if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.28)
    project(FourthSkillTree LANGUAGES CXX RC)
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

add_library(FourthSkillTree SHARED plugin.cpp plugin.rc)
target_compile_features(FourthSkillTree PRIVATE cxx_std_20)
target_link_libraries(FourthSkillTree PRIVATE
    D2RLPlugin::D2RLPlugin
    tomlplusplus::tomlplusplus
)
set_target_properties(FourthSkillTree PROPERTIES
    OUTPUT_NAME "FourthSkillTree"
    PREFIX ""
    RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE FALSE
)
d2rlplugin_embed_config(
    TARGET FourthSkillTree
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/../fourth-skill-tree.toml"
)

if(MSVC)
    target_compile_options(FourthSkillTree PRIVATE /utf-8 /W4 /permissive-)
    target_link_options(FourthSkillTree PRIVATE /INCREMENTAL:NO /Brepro)
endif()

if(BUILD_TESTING)
    add_executable(FourthSkillTreeTests policy_tests.cpp)
    target_compile_features(FourthSkillTreeTests PRIVATE cxx_std_20)
    target_link_libraries(
        FourthSkillTreeTests PRIVATE tomlplusplus::tomlplusplus)
    target_compile_definitions(FourthSkillTreeTests PRIVATE
        FOURTH_SKILL_TREE_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/../fourth-skill-tree.toml"
        FOURTH_SKILL_TREE_PLUGIN_FILE="${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp"
    )
    if(MSVC)
        target_compile_options(
            FourthSkillTreeTests PRIVATE /utf-8 /W4 /permissive-)
    endif()
    add_test(NAME fourth-skill-tree-policy COMMAND FourthSkillTreeTests)
endif()
