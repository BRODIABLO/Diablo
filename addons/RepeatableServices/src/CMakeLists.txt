cmake_minimum_required(VERSION 3.28)
project(RepeatableServices LANGUAGES CXX RC)

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

add_library(RepeatableServices SHARED plugin.cpp plugin.rc)
target_compile_features(RepeatableServices PRIVATE cxx_std_20)
target_link_libraries(RepeatableServices PRIVATE
    D2RLPlugin::D2RLPlugin
    nlohmann_json::nlohmann_json
)
set_target_properties(RepeatableServices PROPERTIES OUTPUT_NAME "RepeatableServices")

add_executable(RepeatableServicesTests repeatable-services-tests.cpp)
target_compile_features(RepeatableServicesTests PRIVATE cxx_std_20)
target_link_libraries(RepeatableServicesTests PRIVATE nlohmann_json::nlohmann_json)
enable_testing()
add_test(NAME repeatable-services-policy COMMAND RepeatableServicesTests)
