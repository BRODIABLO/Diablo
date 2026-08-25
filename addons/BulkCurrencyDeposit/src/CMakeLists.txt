if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.28)
    project(BulkCurrencyDeposit LANGUAGES CXX RC)
    include(CTest)
else()
    enable_language(RC)
endif()

include(FetchContent)
if(NOT TARGET D2RLPlugin::D2RLPlugin)
    set(D2RLPLUGIN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(D2RLPLUGIN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        D2RLPlugin
        GIT_REPOSITORY https://github.com/D2RLoader/PluginSDK.git
        GIT_TAG 4933e2c42cb2592958cd0df3b6dc5003102252d1
    )
    FetchContent_MakeAvailable(D2RLPlugin)
endif()
if(NOT TARGET tomlplusplus::tomlplusplus)
    set(TOMLPLUSPLUS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(TOMLPLUSPLUS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(tomlplusplus)
endif()

set(BULK_CURRENCY_DEPOSIT_BUTTON_MOLD
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/button-mold.sprite")
set(BULK_CURRENCY_DEPOSIT_BUTTON_MOLD_LOWEND
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/button-mold.lowend.sprite")
set(BULK_CURRENCY_DEPOSIT_BUTTON
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/deposit-button.sprite")
set(BULK_CURRENCY_DEPOSIT_BUTTON_LOWEND
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/deposit-button.lowend.sprite")

add_library(BulkCurrencyDeposit SHARED plugin.cpp plugin.rc)
set_property(
    SOURCE plugin.rc
    APPEND PROPERTY OBJECT_DEPENDS
        "${BULK_CURRENCY_DEPOSIT_BUTTON_MOLD};${BULK_CURRENCY_DEPOSIT_BUTTON_MOLD_LOWEND};${BULK_CURRENCY_DEPOSIT_BUTTON};${BULK_CURRENCY_DEPOSIT_BUTTON_LOWEND}"
)
target_compile_features(BulkCurrencyDeposit PRIVATE cxx_std_20)
target_link_libraries(BulkCurrencyDeposit PRIVATE
    D2RLPlugin::D2RLPlugin
    tomlplusplus::tomlplusplus
)
set_target_properties(BulkCurrencyDeposit PROPERTIES
    OUTPUT_NAME "d2rl-ruffneckk-bulk-currency-deposit"
    PREFIX ""
    RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE TRUE
)
d2rlplugin_embed_config(
    TARGET BulkCurrencyDeposit
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/../ruffneckk-bulk-currency-deposit.toml"
)

if(MSVC)
    target_compile_options(BulkCurrencyDeposit PRIVATE /utf-8 /W4 /permissive-)
    target_link_options(BulkCurrencyDeposit PRIVATE /INCREMENTAL:NO /Brepro)
endif()

if(BUILD_TESTING)
    add_executable(BulkCurrencyDepositTests policy_tests.cpp)
    target_compile_features(BulkCurrencyDepositTests PRIVATE cxx_std_20)
    target_link_libraries(BulkCurrencyDepositTests PRIVATE
        tomlplusplus::tomlplusplus
    )
    target_compile_definitions(BulkCurrencyDepositTests PRIVATE
        BULK_CURRENCY_DEPOSIT_SOURCE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp"
        BULK_CURRENCY_DEPOSIT_BUTTON_MOLD_FILE="${BULK_CURRENCY_DEPOSIT_BUTTON_MOLD}"
        BULK_CURRENCY_DEPOSIT_BUTTON_MOLD_LOWEND_FILE="${BULK_CURRENCY_DEPOSIT_BUTTON_MOLD_LOWEND}"
        BULK_CURRENCY_DEPOSIT_BUTTON_FILE="${BULK_CURRENCY_DEPOSIT_BUTTON}"
        BULK_CURRENCY_DEPOSIT_BUTTON_LOWEND_FILE="${BULK_CURRENCY_DEPOSIT_BUTTON_LOWEND}"
    )
    if(MSVC)
        target_compile_options(BulkCurrencyDepositTests PRIVATE /utf-8 /W4 /permissive-)
    endif()
    add_test(NAME bulk-currency-deposit-policy COMMAND BulkCurrencyDepositTests)
endif()
