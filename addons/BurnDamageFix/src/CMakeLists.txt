if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.28)
    project(BurnDamageFix VERSION 2.2.0 LANGUAGES CXX RC ASM_MASM)
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

add_library(BurnDamageFix SHARED
    burn_damage_fix_relay.asm
    plugin.cpp
    plugin.rc
)
target_compile_features(BurnDamageFix PRIVATE cxx_std_20)
target_compile_definitions(BurnDamageFix PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)
target_link_libraries(BurnDamageFix PRIVATE
    D2RLPlugin::D2RLPlugin
    tomlplusplus::tomlplusplus
)
set_target_properties(BurnDamageFix PROPERTIES
    OUTPUT_NAME "BurnDamageFix"
    PREFIX ""
    RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE TRUE
)
d2rlplugin_embed_config(
    TARGET BurnDamageFix
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/../burn-damage-fix.toml"
)

if(MSVC)
    target_compile_options(BurnDamageFix PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/WX>
        $<$<COMPILE_LANGUAGE:CXX>:/permissive->
        $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
        $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
    )
    target_link_options(BurnDamageFix PRIVATE /INCREMENTAL:NO /Brepro)
endif()

if(BUILD_TESTING)
    add_executable(BurnDamageFixTests burn_damage_fix_policy_tests.cpp)
    target_compile_features(BurnDamageFixTests PRIVATE cxx_std_20)
    target_compile_definitions(BurnDamageFixTests PRIVATE
        BURN_DAMAGE_FIX_PLUGIN_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp"
    )
    target_link_libraries(BurnDamageFixTests PRIVATE
        tomlplusplus::tomlplusplus
    )
    if(MSVC)
        target_compile_options(BurnDamageFixTests PRIVATE
            /W4 /WX /permissive- /EHsc /utf-8
        )
    endif()
    add_test(NAME burn-damage-fix-policy COMMAND BurnDamageFixTests)
endif()
