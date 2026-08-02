cmake_minimum_required(VERSION 3.28)
project(RogueScoutMovement LANGUAGES CXX RC)

include(FetchContent)
FetchContent_Declare(
    D2RLPlugin
    GIT_REPOSITORY https://github.com/D2RLoader/PluginSDK.git
    GIT_TAG efcfaaa52eeec9e379b3fc2aad1013bb3dddc970
)
FetchContent_MakeAvailable(D2RLPlugin)

add_library(RogueScoutMovement SHARED plugin.cpp plugin.rc)
target_compile_features(RogueScoutMovement PRIVATE cxx_std_20)
target_link_libraries(RogueScoutMovement PRIVATE D2RLPlugin::D2RLPlugin)
set_target_properties(RogueScoutMovement PROPERTIES OUTPUT_NAME "RogueScoutMovement")

add_executable(RogueScoutMovementTests policy_tests.cpp)
target_compile_features(RogueScoutMovementTests PRIVATE cxx_std_20)
enable_testing()
add_test(NAME rogue-scout-movement-policy COMMAND RogueScoutMovementTests)
