cmake_minimum_required(VERSION 3.28)
project(RuffnecKkScriptedAI VERSION 0.2.0 LANGUAGES C CXX RC)

include(CTest)
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
    lua54
    URL https://www.lua.org/ftp/lua-5.4.9.tar.gz
    URL_HASH SHA256=2335b6c582a52654f94612bf10d2f4672805d05329aa6568b1d8cd9e5c6fb8e6
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(D2RLPlugin tomlplusplus lua54)

add_library(RuffnecKkLua54 STATIC
    "${lua54_SOURCE_DIR}/src/lapi.c"
    "${lua54_SOURCE_DIR}/src/lauxlib.c"
    "${lua54_SOURCE_DIR}/src/lbaselib.c"
    "${lua54_SOURCE_DIR}/src/lcode.c"
    "${lua54_SOURCE_DIR}/src/lctype.c"
    "${lua54_SOURCE_DIR}/src/ldebug.c"
    "${lua54_SOURCE_DIR}/src/ldo.c"
    "${lua54_SOURCE_DIR}/src/ldump.c"
    "${lua54_SOURCE_DIR}/src/lfunc.c"
    "${lua54_SOURCE_DIR}/src/lgc.c"
    "${lua54_SOURCE_DIR}/src/llex.c"
    "${lua54_SOURCE_DIR}/src/lmathlib.c"
    "${lua54_SOURCE_DIR}/src/lmem.c"
    "${lua54_SOURCE_DIR}/src/lobject.c"
    "${lua54_SOURCE_DIR}/src/lopcodes.c"
    "${lua54_SOURCE_DIR}/src/lparser.c"
    "${lua54_SOURCE_DIR}/src/lstate.c"
    "${lua54_SOURCE_DIR}/src/lstring.c"
    "${lua54_SOURCE_DIR}/src/ltable.c"
    "${lua54_SOURCE_DIR}/src/ltablib.c"
    "${lua54_SOURCE_DIR}/src/ltm.c"
    "${lua54_SOURCE_DIR}/src/lundump.c"
    "${lua54_SOURCE_DIR}/src/lvm.c"
    "${lua54_SOURCE_DIR}/src/lzio.c"
)
target_include_directories(
    RuffnecKkLua54 PUBLIC "${lua54_SOURCE_DIR}/src")
if(MSVC)
    target_compile_options(RuffnecKkLua54 PRIVATE /W3 /WX-)
endif()

add_library(RuffnecKkScriptedAICore STATIC
    scripted_ai_bridge.cpp
    scripted_ai_config.cpp
    scripted_ai_fingerprint.cpp
    scripted_ai_sandbox.cpp
)
target_compile_features(RuffnecKkScriptedAICore PUBLIC cxx_std_20)
target_include_directories(
    RuffnecKkScriptedAICore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(RuffnecKkScriptedAICore PUBLIC
    RuffnecKkLua54
    tomlplusplus::tomlplusplus
)

add_library(RuffnecKkScriptedAI SHARED plugin.cpp plugin.rc)
target_compile_features(RuffnecKkScriptedAI PRIVATE cxx_std_20)
target_link_libraries(RuffnecKkScriptedAI PRIVATE
    D2RLPlugin::D2RLPlugin
    RuffnecKkScriptedAICore
)
set_target_properties(RuffnecKkScriptedAI PROPERTIES
    OUTPUT_NAME "d2rl-ruffneckk-scripted-ai"
    PREFIX ""
    RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE FALSE
)
d2rlplugin_embed_config(
    TARGET RuffnecKkScriptedAI
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/../ruffneckk-scripted-ai.toml"
)

if(MSVC)
    target_compile_options(RuffnecKkScriptedAICore PRIVATE
        /utf-8 /W4 /WX /permissive- /EHsc)
    target_compile_options(RuffnecKkScriptedAI PRIVATE
        /utf-8 /W4 /WX /permissive- /EHsc)
    target_link_options(RuffnecKkScriptedAI PRIVATE /INCREMENTAL:NO /Brepro)
endif()

if(BUILD_TESTING)
    add_executable(RuffnecKkScriptedAITests scripted_ai_tests.cpp)
    target_compile_features(RuffnecKkScriptedAITests PRIVATE cxx_std_20)
    target_link_libraries(
        RuffnecKkScriptedAITests PRIVATE RuffnecKkScriptedAICore)
    target_compile_definitions(RuffnecKkScriptedAITests PRIVATE
        SCRIPTED_AI_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/../ruffneckk-scripted-ai.toml"
        SCRIPTED_AI_PLUGIN_SOURCE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp"
        SCRIPTED_AI_BRIDGE_SOURCE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/scripted_ai_bridge.cpp"
    )
    if(MSVC)
        target_compile_options(RuffnecKkScriptedAITests PRIVATE
            /utf-8 /W4 /WX /permissive- /EHsc)
        target_link_options(
            RuffnecKkScriptedAITests PRIVATE /INCREMENTAL:NO /Brepro)
    endif()

    set(
        SCRIPTED_AI_CANONICAL_IMAGE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../reverse-engineering/d2r-3.2.92777/analysis-cache/images/D2R-3.2.92777-decrypted.exe"
        CACHE FILEPATH
        "Governed canonical image used by the optional 22-window uniqueness test"
    )
    if(EXISTS "${SCRIPTED_AI_CANONICAL_IMAGE}")
        add_test(
            NAME ruffneckk-scripted-ai-bridge-ai-1
            COMMAND RuffnecKkScriptedAITests "${SCRIPTED_AI_CANONICAL_IMAGE}"
        )
    else()
        add_test(
            NAME ruffneckk-scripted-ai-bridge-ai-1
            COMMAND RuffnecKkScriptedAITests
        )
    endif()
endif()
