cmake_minimum_required(VERSION 3.28)
project(ProgressiveAffixesPlugin VERSION 0.2.0 LANGUAGES CXX RC)

include(FetchContent)
FetchContent_Declare(
    D2RLPlugin
    GIT_REPOSITORY https://github.com/D2RLoader/PluginSDK.git
    GIT_TAG efcfaaa52eeec9e379b3fc2aad1013bb3dddc970
)
FetchContent_MakeAvailable(D2RLPlugin)

add_library(ProgressiveAffixesPlugin SHARED plugin.cpp plugin.rc)
target_compile_features(ProgressiveAffixesPlugin PRIVATE cxx_std_20)
target_link_libraries(ProgressiveAffixesPlugin PRIVATE D2RLPlugin::D2RLPlugin)
set_target_properties(
    ProgressiveAffixesPlugin
    PROPERTIES OUTPUT_NAME "ProgressiveAffixesPlugin"
)

add_executable(
    ProgressiveAffixesPluginTests
    progressive_affixes_tests.cpp
)
target_compile_features(ProgressiveAffixesPluginTests PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(
        ProgressiveAffixesPlugin
        PRIVATE /W4 /WX /permissive- /utf-8
    )
    target_compile_options(
        ProgressiveAffixesPluginTests
        PRIVATE /W4 /WX /permissive- /utf-8
    )
    target_link_options(ProgressiveAffixesPlugin PRIVATE /Brepro)
endif()

add_custom_command(
    TARGET ProgressiveAffixesPlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ProgressiveAffixesPlugin>
        ${CMAKE_CURRENT_SOURCE_DIR}/../package/ProgressiveAffixesPlugin.dll
    COMMENT "Staging the strict public ProgressiveAffixesPlugin DLL"
)

enable_testing()
add_test(
    NAME progressive-affixes-config-policy
    COMMAND ProgressiveAffixesPluginTests
        "${CMAKE_CURRENT_SOURCE_DIR}/../package/ProgressiveAffixesPlugin.toml"
)
