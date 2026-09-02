if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.28)
    project(ISC12 VERSION 0.2.0 LANGUAGES CXX RC ASM_MASM)
    include(CTest)
else()
    enable_language(RC)
    enable_language(ASM_MASM)
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
add_library(ISC12 SHARED
    plugin.cpp
    isc12_codec_patch.cpp
    isc12_envelope.cpp
    isc12_item_packet_budget.cpp
    isc12_native_persistence_adapter.cpp
    isc12_native_schema_adapter.cpp
    isc12_persistence_policy.cpp
    isc12_player_stat_preflight.cpp
    isc12_publication_adapters.cpp
    isc12_publication_coordinator.cpp
    isc12_schema.cpp
    isc12_loader.cpp
    isc12_loader_tail.asm
    isc12_persistence_relay.asm
    plugin.rc
)
target_compile_features(ISC12 PRIVATE cxx_std_20)
target_compile_definitions(ISC12 PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)
target_link_libraries(ISC12 PRIVATE
    D2RLPlugin::D2RLPlugin
    bcrypt
)
set_target_properties(ISC12 PROPERTIES
    OUTPUT_NAME "d2rl-ruffneckk-isc12"
    PREFIX ""
    RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE FALSE
)
if(MSVC)
    target_compile_options(ISC12 PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/WX>
        $<$<COMPILE_LANGUAGE:CXX>:/permissive->
        $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
        $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
    )
    target_link_options(ISC12 PRIVATE /INCREMENTAL:NO /Brepro)
endif()

if(BUILD_TESTING)
    add_executable(ISC12Tests
        policy_tests.cpp
        isc12_atomic_file.cpp
        isc12_codec_patch.cpp
        isc12_envelope.cpp
        isc12_item_packet_budget.cpp
        isc12_persistence_policy.cpp
        isc12_player_stat_preflight.cpp
        isc12_schema.cpp
    )
    target_compile_features(ISC12Tests PRIVATE cxx_std_20)
    target_compile_definitions(ISC12Tests PRIVATE
        ISC12_CODEC_PATCH_TESTING=1
        ISC12_PLUGIN_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp"
        ISC12_RESOURCE_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/plugin.rc"
        ISC12_LOADER_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/isc12_loader.cpp"
        ISC12_LOADER_ASM_PATH="${CMAKE_CURRENT_SOURCE_DIR}/isc12_loader_tail.asm"
        ISC12_PERSISTENCE_ASM_PATH="${CMAKE_CURRENT_SOURCE_DIR}/isc12_persistence_relay.asm"
        ISC12_CMAKE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
    )
    target_link_libraries(ISC12Tests PRIVATE
        bcrypt
    )
    if(MSVC)
        target_compile_options(ISC12Tests PRIVATE
            /W4 /WX /permissive- /EHsc /utf-8
        )
        # The exhaustive G9 staging cases deliberately keep many independent
        # fixed-size transaction/probe fixtures alive in one test function.
        # This raises only the policy-test executable's reserve; the plugin
        # keeps its single thread_local, allocation-free production context.
        target_link_options(ISC12Tests PRIVATE /STACK:4194304)
    endif()
    add_test(NAME isc12-policy COMMAND ISC12Tests)

    add_executable(ISC12NativeSchemaAdapterTests
        native_schema_adapter_tests.cpp
        isc12_envelope.cpp
        isc12_native_schema_adapter.cpp
        isc12_schema.cpp
    )
    target_compile_features(ISC12NativeSchemaAdapterTests PRIVATE cxx_std_20)
    target_link_libraries(ISC12NativeSchemaAdapterTests PRIVATE bcrypt)
    if(MSVC)
        target_compile_options(ISC12NativeSchemaAdapterTests PRIVATE
            /W4 /WX /permissive- /EHsc /utf-8
        )
    endif()
    add_test(
        NAME isc12-native-schema-adapter
        COMMAND ISC12NativeSchemaAdapterTests
    )

    add_executable(ISC12NativePersistenceAdapterTests
        native_persistence_adapter_tests.cpp
        isc12_envelope.cpp
        isc12_native_persistence_adapter.cpp
        isc12_persistence_policy.cpp
    )
    target_compile_features(ISC12NativePersistenceAdapterTests PRIVATE
        cxx_std_20
    )
    target_link_libraries(ISC12NativePersistenceAdapterTests PRIVATE bcrypt)
    if(MSVC)
        target_compile_options(ISC12NativePersistenceAdapterTests PRIVATE
            /W4 /WX /permissive- /EHsc /utf-8
        )
    endif()
    add_test(
        NAME isc12-native-persistence-adapter
        COMMAND ISC12NativePersistenceAdapterTests
    )

    add_executable(ISC12PublicationCoordinatorTests
        publication_coordinator_tests.cpp
        isc12_publication_coordinator.cpp
    )
    target_compile_features(ISC12PublicationCoordinatorTests PRIVATE
        cxx_std_20
    )
    target_compile_definitions(ISC12PublicationCoordinatorTests PRIVATE
        ISC12_CODEC_PATCH_TESTING=1
    )
    if(MSVC)
        target_compile_options(ISC12PublicationCoordinatorTests PRIVATE
            /W4 /WX /permissive- /EHsc /utf-8
        )
    endif()
    add_test(
        NAME isc12-publication-coordinator
        COMMAND ISC12PublicationCoordinatorTests
    )

    add_executable(ISC12PublicationAdapterTests
        publication_adapter_tests.cpp
        isc12_codec_patch.cpp
        isc12_publication_adapters.cpp
        isc12_publication_coordinator.cpp
    )
    target_compile_features(ISC12PublicationAdapterTests PRIVATE
        cxx_std_20
    )
    target_compile_definitions(ISC12PublicationAdapterTests PRIVATE
        ISC12_CODEC_PATCH_TESTING=1
    )
    if(MSVC)
        target_compile_options(ISC12PublicationAdapterTests PRIVATE
            /W4 /WX /permissive- /EHsc /utf-8
        )
    endif()
    add_test(
        NAME isc12-publication-adapters
        COMMAND ISC12PublicationAdapterTests
    )
endif()
