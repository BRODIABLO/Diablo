#define NOMINMAX

#include <D2RLPlugin/api.h>

#include "automap_serialization_policy.hpp"

#include <Windows.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string>

namespace RuffnecKk::AutomapSerializationFix {
namespace {

constexpr wchar_t SingletonName[] =
    L"Local\\RuffnecKk.AutomapSerializationFix.Singleton";

const D2RL::PluginContext* Context{};
HANDLE SingletonHandle{};
std::string RuntimeBuildName{"unknown"};
std::atomic_bool Operational{};

template <std::size_t Size>
[[nodiscard]] auto CheckFingerprint(
        std::uintptr_t rva,
        const std::array<std::uint8_t, Size>& expected,
        const char* label) noexcept -> bool {
    if (Context->CheckExpectedBytes(
            rva,
            expected.data(),
            static_cast<std::uint32_t>(expected.size()))) {
        return true;
    }
    const auto message = std::string("AutomapSerializationFix: ") + label
        + " fingerprint mismatch; plugin refused.";
    Context->LogError(message.c_str());
    return false;
}

[[nodiscard]] auto ValidateRuntime() noexcept -> bool {
    if (!CheckFingerprint(
            SerializerEntryRva,
            SerializerEntryExpected,
            "serializer entry")) {
        return false;
    }
    if (!CheckFingerprint(
            SerializerEmissionWitnessRva,
            SerializerEmissionExpected,
            "tag filter and record layout")) {
        return false;
    }
    for (std::size_t index = 0; index < SerializerCallsiteRvas.size(); ++index) {
        if (!CheckFingerprint(
                SerializerCallsiteRvas[index],
                SerializerCallsiteExpected[index],
                "serializer callsite")) {
            return false;
        }
    }
    return CheckFingerprint(
            SerializerByteCountRva,
            SerializerByteCountOriginal,
            "signed byte-count epilogue")
        && CheckFingerprint(
            SerializerContinuationRva,
            SerializerContinuationExpected,
            "serializer return path")
        && CheckFingerprint(
            OwnerCommitWitnessRva,
            OwnerCommitExpected,
            "four-tree sidecar commit");
}

[[nodiscard]] auto AcquireSingleton() noexcept -> bool {
    SingletonHandle = CreateMutexW(nullptr, FALSE, SingletonName);
    if (!SingletonHandle) {
        Context->LogError(
            "AutomapSerializationFix: process singleton could not be created.");
        return false;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(SingletonHandle);
        SingletonHandle = nullptr;
        Context->LogError(
            "AutomapSerializationFix: duplicate global/mod-local installation refused.");
        return false;
    }
    return true;
}

void ReleaseSingleton() noexcept {
    if (!SingletonHandle) return;
    CloseHandle(SingletonHandle);
    SingletonHandle = nullptr;
}

[[nodiscard]] auto InstallPatch() noexcept -> bool {
    if (Context->PatchBytes(
            SerializerByteCountRva,
            SerializerByteCountOriginal.data(),
            static_cast<std::uint32_t>(SerializerByteCountOriginal.size()),
            SerializerByteCountPatched.data(),
            static_cast<std::uint32_t>(SerializerByteCountPatched.size()))) {
        return true;
    }
    Context->LogError(
        "AutomapSerializationFix: serializer byte-count range is already owned or changed; plugin refused.");
    return false;
}

auto Status(
        D2R::Game::Client*,
        const D2RL::ConsoleCommandContext* command,
        void*) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) {
        return D2RL::ConsoleCommandResult::Failed;
    }
    char message[256]{};
    std::snprintf(
        message,
        sizeof(message),
        "Automap Serialization Fix 0.1.0: %s; build=%s; byte-count=checked-uint32; config=none.",
        Operational.load(std::memory_order_acquire) ? "active" : "inactive",
        RuntimeBuildName.c_str());
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}

} // namespace
} // namespace RuffnecKk::AutomapSerializationFix

namespace {

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "ruffneckk-automap-serialization-fix",
    .name = "Automap Serialization Fix",
    .version = "0.1.0",
    .author = "RuffnecKk",
    .description = "Prevents large explored maps from crashing during area transitions.",
    .flags = D2RL::PluginFlags::Client | D2RL::PluginFlags::NativeHooks,
};

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept
        -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    using namespace RuffnecKk::AutomapSerializationFix;
    Context = context;
    Operational.store(false, std::memory_order_release);
    if (!D2RL::HasContext(context) || context->exeBase == 0U) return false;
    if (!AcquireSingleton()) return false;

    const auto* const runtimeBuild = D2RL::GetBuildName(context);
    RuntimeBuildName = runtimeBuild ? runtimeBuild : "unknown";
    const auto diagnostic = std::string(
        "AutomapSerializationFix: build=") + RuntimeBuildName
        + "; build identity is diagnostic only; validating native fingerprint.";
    context->LogInfo(diagnostic.c_str());

    if (!ValidateRuntime() || !InstallPatch()) {
        ReleaseSingleton();
        return false;
    }

    Operational.store(true, std::memory_order_release);
    if (!context->RegisterConsoleCommand(
            "automap-serialization-fix",
            Status,
            "Show Automap Serialization Fix status.")) {
        context->LogWarn(
            "AutomapSerializationFix: status command could not be registered.");
    }
    const auto message = std::string(
        "Automap Serialization Fix 0.1.0 by RuffnecKk active; build=")
        + RuntimeBuildName + "; config=none.";
    context->LogInfo(message.c_str());
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    using namespace RuffnecKk::AutomapSerializationFix;
    Operational.store(false, std::memory_order_release);
    ReleaseSingleton();
}
