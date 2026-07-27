cmake_minimum_required(VERSION 3.28)
project(RemoteStash LANGUAGES CXX RC)

include(FetchContent)
FetchContent_Declare(
    D2RLPlugin
    GIT_REPOSITORY https://github.com/D2RLoader/PluginSDK.git
    GIT_TAG efcfaaa52eeec9e379b3fc2aad1013bb3dddc970
)
FetchContent_MakeAvailable(D2RLPlugin)

add_library(RemoteStash SHARED plugin.cpp plugin.rc)
target_compile_features(RemoteStash PRIVATE cxx_std_20)
target_link_libraries(RemoteStash PRIVATE D2RLPlugin::D2RLPlugin)
set_target_properties(RemoteStash PROPERTIES OUTPUT_NAME "RemoteStash")

add_executable(RemoteStashLayoutPolicyTests layout_policy_tests.cpp)
target_compile_features(RemoteStashLayoutPolicyTests PRIVATE cxx_std_20)

enable_testing()
add_test(NAME remote-stash-layout-policy COMMAND RemoteStashLayoutPolicyTests)
