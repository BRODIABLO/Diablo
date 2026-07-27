cmake_minimum_required(VERSION 3.28)
project(ReadableItems LANGUAGES CXX RC)

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
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
)
FetchContent_MakeAvailable(D2RLPlugin nlohmann_json imgui)

add_library(ReadableItemsImGui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(ReadableItemsImGui PUBLIC
    ${imgui_SOURCE_DIR}
)
target_compile_features(ReadableItemsImGui PUBLIC cxx_std_20)
target_compile_definitions(ReadableItemsImGui PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

add_library(ReadableItems SHARED plugin.cpp plugin.rc)
target_compile_features(ReadableItems PRIVATE cxx_std_20)
target_link_libraries(ReadableItems PRIVATE
    D2RLPlugin::D2RLPlugin
    ReadableItemsImGui
    nlohmann_json::nlohmann_json
    user32
)
set_target_properties(ReadableItems PROPERTIES OUTPUT_NAME "ReadableItems")

add_executable(ReadableItemsTests config_tests.cpp)
target_compile_features(ReadableItemsTests PRIVATE cxx_std_20)
target_link_libraries(ReadableItemsTests PRIVATE nlohmann_json::nlohmann_json)

enable_testing()
add_test(NAME readable-items-config COMMAND ReadableItemsTests)

get_filename_component(
    READABLE_ITEMS_BUNDLED_CONFIG
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../BKVince.mpq/ReadableItems.json"
    ABSOLUTE
)
add_test(
    NAME readable-items-bundled-config
    COMMAND ReadableItemsTests "${READABLE_ITEMS_BUNDLED_CONFIG}"
)
