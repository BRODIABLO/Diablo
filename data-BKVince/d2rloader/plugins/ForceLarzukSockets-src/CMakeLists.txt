cmake_minimum_required(VERSION 3.24)
project(ForceLarzukSockets LANGUAGES CXX RC)

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

add_library(ForceLarzukSockets SHARED plugin.cpp plugin.rc)
target_compile_features(ForceLarzukSockets PRIVATE cxx_std_20)
target_link_libraries(ForceLarzukSockets PRIVATE
    D2RLPlugin::D2RLPlugin
    nlohmann_json::nlohmann_json
)
set_target_properties(ForceLarzukSockets PROPERTIES OUTPUT_NAME "ForceLarzukSockets")

add_executable(ForceLarzukSocketsTests larzuk_policy_tests.cpp)
target_compile_features(ForceLarzukSocketsTests PRIVATE cxx_std_20)
enable_testing()
add_test(NAME force-larzuk-sockets-policy COMMAND ForceLarzukSocketsTests)
