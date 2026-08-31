if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.28)
    project(AutoSort LANGUAGES CXX RC)
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
        GIT_TAG 6eb8f8b6192868214706bd6d528c5294f2f551b7
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

add_library(AutoSort SHARED plugin.cpp plugin.rc)
target_compile_features(AutoSort PRIVATE cxx_std_20)
target_link_libraries(AutoSort PRIVATE
    D2RLPlugin::D2RLPlugin
    tomlplusplus::tomlplusplus
)
set_target_properties(AutoSort PROPERTIES
    OUTPUT_NAME "d2rl-ruffneckk-autosort"
    PREFIX ""
    RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE TRUE
)
d2rlplugin_embed_config(
    TARGET AutoSort
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/../ruffneckk-autosort.toml"
)

if(MSVC)
    target_compile_options(
        AutoSort PRIVATE /utf-8 /W4 /WX /permissive- /EHsc)
    target_link_options(AutoSort PRIVATE /INCREMENTAL:NO /Brepro)
endif()

if(BUILD_TESTING)
    add_executable(AutoSortTests policy_tests.cpp)
    target_compile_features(AutoSortTests PRIVATE cxx_std_20)
    target_link_libraries(AutoSortTests PRIVATE tomlplusplus::tomlplusplus)
    target_compile_definitions(AutoSortTests PRIVATE
        AUTOSORT_SOURCE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp"
        AUTOSORT_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/../ruffneckk-autosort.toml"
        AUTOSORT_CMAKE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
    )
    if(MSVC)
        target_compile_options(
            AutoSortTests PRIVATE /utf-8 /W4 /WX /permissive- /EHsc)
    endif()
    add_test(NAME autosort-policy COMMAND AutoSortTests)
endif()
