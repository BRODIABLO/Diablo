if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.28)
    project(AutomapSerializationFix VERSION 0.1.0 LANGUAGES CXX RC)
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

add_library(AutomapSerializationFix SHARED plugin.cpp plugin.rc)
target_compile_features(AutomapSerializationFix PRIVATE cxx_std_20)
target_link_libraries(AutomapSerializationFix PRIVATE D2RLPlugin::D2RLPlugin)
set_target_properties(AutomapSerializationFix PROPERTIES
    OUTPUT_NAME "d2rl-ruffneckk-automap-serialization-fix"
    PREFIX ""
    RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE TRUE
)

if(MSVC)
    target_compile_options(AutomapSerializationFix PRIVATE
        /utf-8 /W4 /WX /permissive- /EHsc)
    target_link_options(AutomapSerializationFix PRIVATE
        /INCREMENTAL:NO /Brepro)
endif()

if(BUILD_TESTING)
    add_executable(AutomapSerializationFixTests policy_tests.cpp)
    target_compile_features(AutomapSerializationFixTests PRIVATE cxx_std_20)
    target_compile_definitions(AutomapSerializationFixTests PRIVATE
        AUTOMAP_SERIALIZATION_PLUGIN_FILE="${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp"
        AUTOMAP_SERIALIZATION_CMAKE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
        AUTOMAP_SERIALIZATION_RESOURCE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/plugin.rc"
    )
    if(MSVC)
        target_compile_options(AutomapSerializationFixTests PRIVATE
            /utf-8 /W4 /WX /permissive- /EHsc)
    endif()
    add_test(
        NAME automap-serialization-fix-policy
        COMMAND AutomapSerializationFixTests)
endif()
