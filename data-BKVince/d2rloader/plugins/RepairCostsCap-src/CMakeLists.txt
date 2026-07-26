cmake_minimum_required(VERSION 3.28)
project(RepairCostsCap LANGUAGES CXX RC)

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

add_library(RepairCostsCap SHARED plugin.cpp plugin.rc)
target_compile_features(RepairCostsCap PRIVATE cxx_std_20)
target_link_libraries(RepairCostsCap PRIVATE
    D2RLPlugin::D2RLPlugin
    nlohmann_json::nlohmann_json
)
set_target_properties(RepairCostsCap PROPERTIES OUTPUT_NAME "RepairCostsCap")

add_executable(RepairCostsCapTests policy_tests.cpp)
target_compile_features(RepairCostsCapTests PRIVATE cxx_std_20)
enable_testing()
add_test(NAME repair-costs-cap-policy COMMAND RepairCostsCapTests)
