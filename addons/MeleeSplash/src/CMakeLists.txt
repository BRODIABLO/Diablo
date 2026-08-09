cmake_minimum_required(VERSION 3.28)
project(MeleeSplash LANGUAGES CXX RC)

include(FetchContent)
FetchContent_Declare(
    D2RLPlugin
    GIT_REPOSITORY https://github.com/D2RLoader/PluginSDK.git
    GIT_TAG efcfaaa52eeec9e379b3fc2aad1013bb3dddc970
)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(D2RLPlugin nlohmann_json)

add_library(MeleeSplash SHARED plugin.cpp plugin.rc)
target_compile_features(MeleeSplash PRIVATE cxx_std_20)
target_link_libraries(MeleeSplash PRIVATE
    D2RLPlugin::D2RLPlugin
    nlohmann_json::nlohmann_json
)

add_executable(MeleeSplashConfigTests config_tests.cpp)
target_compile_features(MeleeSplashConfigTests PRIVATE cxx_std_20)
target_link_libraries(MeleeSplashConfigTests PRIVATE
    nlohmann_json::nlohmann_json
)
if(MSVC)
    target_compile_options(MeleeSplash PRIVATE /W4 /WX /permissive- /utf-8)
    target_compile_options(
        MeleeSplashConfigTests PRIVATE /W4 /WX /permissive- /utf-8)
endif()
set_target_properties(MeleeSplash PROPERTIES OUTPUT_NAME "MeleeSplash")
add_custom_command(
    TARGET MeleeSplash POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:MeleeSplash>
        ${CMAKE_CURRENT_SOURCE_DIR}/../package/MeleeSplash.dll
    COMMENT "Staging the strict public MeleeSplash DLL"
)

enable_testing()
add_test(
    NAME melee-splash-config-policy
    COMMAND MeleeSplashConfigTests
        "${CMAKE_CURRENT_SOURCE_DIR}/../package/MeleeSplash.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/../examples/MeleeSplash.enabled.example.json"
)
