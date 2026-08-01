cmake_minimum_required(VERSION 3.28)
project(CharmZone LANGUAGES CXX RC)

include(FetchContent)
FetchContent_Declare(
    D2RLPlugin
    GIT_REPOSITORY https://github.com/D2RLoader/PluginSDK.git
    GIT_TAG efcfaaa52eeec9e379b3fc2aad1013bb3dddc970
)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(D2RLPlugin tomlplusplus)

add_library(CharmZone SHARED plugin.cpp plugin.rc)
target_compile_features(CharmZone PRIVATE cxx_std_20)
target_compile_definitions(CharmZone PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(CharmZone PRIVATE
    D2RLPlugin::D2RLPlugin
    tomlplusplus::tomlplusplus
)
set_target_properties(CharmZone PROPERTIES OUTPUT_NAME "CharmZone")

if(MSVC)
    target_compile_options(CharmZone PRIVATE /W4 /permissive- /EHsc)
endif()

add_executable(CharmZoneTests policy_tests.cpp)
target_compile_features(CharmZoneTests PRIVATE cxx_std_20)
target_link_libraries(CharmZoneTests PRIVATE tomlplusplus::tomlplusplus)

enable_testing()
add_test(NAME charm-zone-policy COMMAND CharmZoneTests)
