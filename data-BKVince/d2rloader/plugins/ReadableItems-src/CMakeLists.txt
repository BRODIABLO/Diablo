cmake_minimum_required(VERSION 3.28)
project(ReadableItemsCore LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

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
