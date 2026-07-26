cmake_minimum_required(VERSION 3.28)
project(ExtendedItemStats LANGUAGES CXX RC)

include(FetchContent)
FetchContent_Declare(
    D2RLPlugin
    GIT_REPOSITORY https://github.com/D2RLoader/PluginSDK.git
    GIT_TAG efcfaaa52eeec9e379b3fc2aad1013bb3dddc970
)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG v1.3.4
)
FetchContent_MakeAvailable(D2RLPlugin imgui minhook)

add_library(ExtendedItemStatsImGui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
)
target_include_directories(ExtendedItemStatsImGui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_compile_features(ExtendedItemStatsImGui PUBLIC cxx_std_20)
target_compile_definitions(ExtendedItemStatsImGui PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ExtendedItemStatsImGui PUBLIC dxgi)

add_library(ExtendedItemTransport STATIC extended_item_transport.cpp)
target_compile_features(ExtendedItemTransport PUBLIC cxx_std_20)
target_include_directories(ExtendedItemTransport PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(ScrollableTooltip STATIC scrollable_tooltip.cpp)
target_compile_features(ScrollableTooltip PUBLIC cxx_std_20)
target_include_directories(ScrollableTooltip PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(ExtendedItemStats SHARED plugin.cpp tooltip_overlay.cpp plugin.rc)
target_compile_features(ExtendedItemStats PRIVATE cxx_std_20)
target_link_libraries(ExtendedItemStats PRIVATE
    D2RLPlugin::D2RLPlugin
    ExtendedItemTransport
    ScrollableTooltip
    ExtendedItemStatsImGui
    minhook
    Xinput
    dxgi
)
set_target_properties(ExtendedItemStats PROPERTIES OUTPUT_NAME "ExtendedItemStats")

add_executable(ExtendedItemStatsTests extended_item_transport_tests.cpp)
target_compile_features(ExtendedItemStatsTests PRIVATE cxx_std_20)
target_link_libraries(ExtendedItemStatsTests PRIVATE ExtendedItemTransport)

add_executable(ScrollableTooltipTests scrollable_tooltip_tests.cpp)
target_compile_features(ScrollableTooltipTests PRIVATE cxx_std_20)
target_link_libraries(ScrollableTooltipTests PRIVATE ScrollableTooltip)

enable_testing()
add_test(NAME extended-item-transport COMMAND ExtendedItemStatsTests)
add_test(NAME scrollable-tooltip COMMAND ScrollableTooltipTests)
