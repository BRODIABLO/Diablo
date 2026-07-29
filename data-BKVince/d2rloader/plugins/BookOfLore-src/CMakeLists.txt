cmake_minimum_required(VERSION 3.28)
project(BookOfLore LANGUAGES CXX RC)

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

add_library(BookOfLore SHARED plugin.cpp plugin.rc)
target_compile_features(BookOfLore PRIVATE cxx_std_20)
target_link_libraries(BookOfLore PRIVATE
    D2RLPlugin::D2RLPlugin
    tomlplusplus::tomlplusplus
)
set_target_properties(BookOfLore PROPERTIES OUTPUT_NAME "BookOfLore")

add_executable(BookOfLoreConfigTests config_tests.cpp)
target_compile_features(BookOfLoreConfigTests PRIVATE cxx_std_20)
target_link_libraries(BookOfLoreConfigTests PRIVATE tomlplusplus::tomlplusplus)

add_executable(BookOfLoreStateTests state_tests.cpp)
target_compile_features(BookOfLoreStateTests PRIVATE cxx_std_20)
target_link_libraries(BookOfLoreStateTests PRIVATE tomlplusplus::tomlplusplus)

enable_testing()
add_test(NAME book-of-lore-config COMMAND BookOfLoreConfigTests)
add_test(NAME book-of-lore-state COMMAND BookOfLoreStateTests)

get_filename_component(
    BOOK_OF_LORE_BUNDLED_CONFIG
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../BKVince.mpq/BookOfLore.toml"
    ABSOLUTE
)
add_test(
    NAME book-of-lore-bundled-config
    COMMAND BookOfLoreConfigTests "${BOOK_OF_LORE_BUNDLED_CONFIG}"
)
