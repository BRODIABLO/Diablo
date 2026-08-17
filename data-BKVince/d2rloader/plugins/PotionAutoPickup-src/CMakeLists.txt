cmake_minimum_required(VERSION 3.24)
project(PotionAutoPickup LANGUAGES CXX RC)

include(FetchContent)
FetchContent_Declare(D2RLPlugin GIT_REPOSITORY https://github.com/D2RLoader/PluginSDK.git GIT_TAG efcfaaa52eeec9e379b3fc2aad1013bb3dddc970)
FetchContent_MakeAvailable(D2RLPlugin)

add_library(PotionAutoPickup SHARED plugin.cpp plugin.rc)
target_compile_features(PotionAutoPickup PRIVATE cxx_std_20)
target_link_libraries(PotionAutoPickup PRIVATE D2RLPlugin::D2RLPlugin)
target_link_options(PotionAutoPickup PRIVATE /Brepro)
set_target_properties(PotionAutoPickup PROPERTIES OUTPUT_NAME "PotionAutoPickup")
set(POTION_AUTO_PICKUP_CONFIG
    "${CMAKE_CURRENT_SOURCE_DIR}/../../config/PotionAutoPickup.toml"
)
set(POTION_AUTO_PICKUP_GENERATED_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/embedded-config"
)
file(MAKE_DIRECTORY "${POTION_AUTO_PICKUP_GENERATED_DIR}")
configure_file(
    "${POTION_AUTO_PICKUP_CONFIG}"
    "${POTION_AUTO_PICKUP_GENERATED_DIR}/PotionAutoPickup.toml"
    COPYONLY
)
file(TO_CMAKE_PATH
    "${POTION_AUTO_PICKUP_GENERATED_DIR}/PotionAutoPickup.toml"
    POTION_AUTO_PICKUP_CONFIG_RESOURCE_PATH
)
file(WRITE "${POTION_AUTO_PICKUP_GENERATED_DIR}/PotionAutoPickup-config.rc"
    "#include <D2RLPlugin/resource.h>\n"
    "D2RL_PLUGIN_CONFIG_RESOURCE_ID RCDATA \"${POTION_AUTO_PICKUP_CONFIG_RESOURCE_PATH}\"\n"
)
target_sources(PotionAutoPickup PRIVATE
    "${POTION_AUTO_PICKUP_GENERATED_DIR}/PotionAutoPickup-config.rc"
)

add_executable(PotionAutoPickupTests router_tests.cpp)
target_compile_features(PotionAutoPickupTests PRIVATE cxx_std_20)
target_compile_options(PotionAutoPickupTests PRIVATE /EHsc)
enable_testing()
add_test(
    NAME router-policy
    COMMAND PotionAutoPickupTests
        "${CMAKE_CURRENT_SOURCE_DIR}/../../config/PotionAutoPickup.toml"
)
