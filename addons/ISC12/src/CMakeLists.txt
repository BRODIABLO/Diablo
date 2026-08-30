if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.28)
    project(ISC12 VERSION 0.2.0 LANGUAGES CXX RC ASM_MASM)
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

add_library(ISC12 SHARED
    plugin.cpp
    isc12_loader.cpp
    isc12_loader_tail.asm
    plugin.rc
)
target_compile_features(ISC12 PRIVATE cxx_std_20)
target_compile_definitions(ISC12 PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)
target_link_libraries(ISC12 PRIVATE
    D2RLPlugin::D2RLPlugin
    tomlplusplus::tomlplusplus
)
set_target_properties(ISC12 PROPERTIES
    OUTPUT_NAME "d2rl-ruffneckk-isc12"
    PREFIX ""
    RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE FALSE
)
d2rlplugin_embed_config(
    TARGET ISC12
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/../ruffneckk-isc12.toml"
)

if(MSVC)
    target_compile_options(ISC12 PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/WX>
        $<$<COMPILE_LANGUAGE:CXX>:/permissive->
        $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
        $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
    )
    target_link_options(ISC12 PRIVATE /INCREMENTAL:NO /Brepro)
endif()

if(BUILD_TESTING)
    add_executable(ISC12Tests policy_tests.cpp)
    target_compile_features(ISC12Tests PRIVATE cxx_std_20)
    target_compile_definitions(ISC12Tests PRIVATE
        ISC12_CONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../ruffneckk-isc12.toml"
        ISC12_PLUGIN_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp"
        ISC12_RESOURCE_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/plugin.rc"
        ISC12_LOADER_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/isc12_loader.cpp"
        ISC12_LOADER_ASM_PATH="${CMAKE_CURRENT_SOURCE_DIR}/isc12_loader_tail.asm"
        ISC12_CMAKE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
    )
    target_link_libraries(ISC12Tests PRIVATE tomlplusplus::tomlplusplus)
    if(MSVC)
        target_compile_options(ISC12Tests PRIVATE
            /W4 /WX /permissive- /EHsc /utf-8
        )
    endif()
    add_test(NAME isc12-policy COMMAND ISC12Tests)
endif()
