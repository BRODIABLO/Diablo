cmake_minimum_required(VERSION 3.28)
project(GroundItemLabelLimit LANGUAGES CXX RC)

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

add_library(GroundItemLabelLimit SHARED plugin.cpp plugin.rc)
target_compile_features(GroundItemLabelLimit PRIVATE cxx_std_20)
target_link_libraries(GroundItemLabelLimit PRIVATE
    D2RLPlugin::D2RLPlugin
    nlohmann_json::nlohmann_json
)
set_target_properties(GroundItemLabelLimit PROPERTIES OUTPUT_NAME "GroundItemLabelLimit")

add_executable(GroundItemLabelLimitTests policy_tests.cpp)
target_compile_features(GroundItemLabelLimitTests PRIVATE cxx_std_20)
target_link_libraries(GroundItemLabelLimitTests PRIVATE nlohmann_json::nlohmann_json)
enable_testing()
add_test(NAME ground-item-label-limit-policy COMMAND GroundItemLabelLimitTests)
