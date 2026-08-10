cmake_minimum_required(VERSION 3.28)
project(BKVCombat LANGUAGES CXX RC)

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

add_library(BKVCombatPolicy STATIC
    bkv_combat_config.cpp
    bkv_combat_policy.cpp
)
target_compile_features(BKVCombatPolicy PUBLIC cxx_std_20)
target_include_directories(BKVCombatPolicy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(BKVCombatPolicy PUBLIC nlohmann_json::nlohmann_json)

add_library(BKVCombat SHARED
    plugin.cpp
    plugin.rc
)
target_compile_features(BKVCombat PRIVATE cxx_std_20)
target_link_libraries(BKVCombat PRIVATE
    BKVCombatPolicy
    D2RLPlugin::D2RLPlugin
)
set_target_properties(BKVCombat PROPERTIES OUTPUT_NAME "BKVCombat")

add_executable(BKVCombatPolicyTests bkv_combat_tests.cpp)
target_compile_features(BKVCombatPolicyTests PRIVATE cxx_std_20)
target_link_libraries(BKVCombatPolicyTests PRIVATE BKVCombatPolicy)

if(MSVC)
    target_compile_options(BKVCombat PRIVATE /W4 /WX /permissive- /utf-8)
    target_compile_options(BKVCombatPolicy PRIVATE /W4 /WX /permissive- /utf-8)
    target_compile_options(BKVCombatPolicyTests PRIVATE /W4 /WX /permissive- /utf-8)
endif()

add_custom_command(
    TARGET BKVCombat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:BKVCombat>
        ${CMAKE_CURRENT_SOURCE_DIR}/../package/BKVCombat.dll
    COMMENT "Staging BKVCombat.dll"
)

enable_testing()
add_test(
    NAME bkv-combat-config-and-policy
    COMMAND BKVCombatPolicyTests
        "${CMAKE_CURRENT_SOURCE_DIR}/../package/BKVCombat.json"
)
