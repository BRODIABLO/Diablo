cmake_minimum_required(VERSION 3.28)
project(EquippedItemToCube LANGUAGES CXX RC)

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
FetchContent_MakeAvailable(D2RLPlugin)
FetchContent_MakeAvailable(nlohmann_json)

add_library(EquippedItemToCube SHARED plugin.cpp plugin.rc)
target_compile_features(EquippedItemToCube PRIVATE cxx_std_20)
target_link_libraries(EquippedItemToCube PRIVATE
    D2RLPlugin::D2RLPlugin
    nlohmann_json::nlohmann_json
)
set_target_properties(EquippedItemToCube PROPERTIES OUTPUT_NAME "EquippedItemToCube")

add_executable(EquippedItemToCubeTests policy_tests.cpp)
target_compile_features(EquippedItemToCubeTests PRIVATE cxx_std_20)
enable_testing()
add_test(NAME equipped-item-to-cube-policy COMMAND EquippedItemToCubeTests)
