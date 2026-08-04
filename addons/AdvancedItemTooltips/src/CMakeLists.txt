cmake_minimum_required(VERSION 3.24)
project(AdvancedItemTooltipsPublic LANGUAGES CXX RC)

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

get_filename_component(
    VANILLA_EXCEL_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../data-vanilla3.2/data/data/global/excel"
    ABSOLUTE
)
set(ADVANCED_ITEM_TOOLTIPS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
configure_file(vanilla_tables.rc.in vanilla_tables.rc @ONLY)

add_library(AdvancedItemTooltipsPublic SHARED
    plugin.cpp
    socket_tooltip.cpp
    tooltip_ranges.cpp
    plugin.rc
    "${CMAKE_CURRENT_BINARY_DIR}/vanilla_tables.rc"
)
target_compile_features(AdvancedItemTooltipsPublic PRIVATE cxx_std_20)
target_link_libraries(AdvancedItemTooltipsPublic PRIVATE
    D2RLPlugin::D2RLPlugin
    nlohmann_json::nlohmann_json
)
set_target_properties(AdvancedItemTooltipsPublic PROPERTIES OUTPUT_NAME "AdvancedItemTooltipsPublic")

add_executable(AdvancedItemTooltipsTests socket_tooltip_tests.cpp socket_tooltip.cpp)
target_compile_features(AdvancedItemTooltipsTests PRIVATE cxx_std_20)
add_executable(AdvancedItemTooltipsRangeTests
    tooltip_ranges_tests.cpp
    tooltip_ranges.cpp
    socket_tooltip.cpp
)
target_compile_features(AdvancedItemTooltipsRangeTests PRIVATE cxx_std_20)
add_executable(AdvancedItemTooltipsConfigTests config_tests.cpp)
target_compile_features(AdvancedItemTooltipsConfigTests PRIVATE cxx_std_20)
target_link_libraries(AdvancedItemTooltipsConfigTests PRIVATE nlohmann_json::nlohmann_json)
if(MSVC)
    target_compile_options(AdvancedItemTooltipsPublic PRIVATE /utf-8)
    target_compile_options(AdvancedItemTooltipsTests PRIVATE /utf-8)
    target_compile_options(AdvancedItemTooltipsRangeTests PRIVATE /utf-8)
    target_compile_options(AdvancedItemTooltipsConfigTests PRIVATE /utf-8)
endif()
enable_testing()
add_test(NAME advanced-item-tooltips-policy COMMAND AdvancedItemTooltipsTests)
add_test(NAME advanced-item-tooltips-ranges COMMAND AdvancedItemTooltipsRangeTests)
add_test(
    NAME advanced-item-tooltips-embedded-provider
    COMMAND AdvancedItemTooltipsRangeTests --provider-smoke "${VANILLA_EXCEL_DIR}"
)
add_test(NAME advanced-item-tooltips-config COMMAND AdvancedItemTooltipsConfigTests)
