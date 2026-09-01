#define NOMINMAX
#include <D2RLPlugin/api.h>

#include "scripted_ai_config.hpp"
#include "scripted_ai_fingerprint.hpp"
#include "scripted_ai_ownership.hpp"
#include "scripted_ai_sandbox.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace ruffneckk::scripted_ai;

constexpr char Version[] = "0.1.0";
constexpr char PluginId[] = "ruffneckk-scripted-ai";
constexpr std::size_t MaximumConfigBytes = 64U * 1024U;

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = PluginId,
    .name = "RuffnecKk Scripted AI",
    .version = Version,
    .author = "RuffnecKk",
    .description = "Lets configured monsters run bounded Lua behavior trees.",
    .flags = D2RL::PluginFlags::Server | D2RL::PluginFlags::NativeHooks,
};

const D2RL::PluginContext* Context{};
const D2RL::DiagnosticsServiceV1* Diagnostics{};
const D2RL::LifecycleServiceV1* Lifecycle{};
const D2RL::ThreadServiceV1* Threads{};
Config Settings{};
std::string LoadedConfigPath{"compiled disabled defaults"};

[[nodiscard]] auto ConfigCandidates()
        -> std::vector<std::filesystem::path> {
    std::filesystem::path activeModConfigDirectory;
    std::filesystem::path scopeConfigDirectory;
    if (Context != nullptr && Context->activeMod != nullptr
            && Context->activeMod[0] != '\0'
            && Context->modSupportDirectory != nullptr
            && Context->modSupportDirectory[0] != L'\0') {
        activeModConfigDirectory =
            std::filesystem::path(Context->modSupportDirectory) / L"config";
    }
    if (Context != nullptr && Context->pluginConfigPath != nullptr
            && Context->pluginConfigPath[0] != L'\0') {
        scopeConfigDirectory =
            std::filesystem::path(Context->pluginConfigPath).parent_path();
    }
    std::error_code error;
    const auto currentPath = std::filesystem::current_path(error);
    const auto globalConfigDirectory = error
        ? std::filesystem::path{}
        : currentPath / L"d2rloader" / L"config";
    return BuildConfigCandidates(
        activeModConfigDirectory,
        scopeConfigDirectory,
        globalConfigDirectory);
}

[[nodiscard]] auto LoadConfig() -> bool {
    Settings = {};
    LoadedConfigPath = "compiled disabled defaults";
    for (const auto& path : ConfigCandidates()) {
        std::error_code statusError;
        if (!std::filesystem::is_regular_file(path, statusError)) continue;
        try {
            const auto size = std::filesystem::file_size(path);
            if (size > MaximumConfigBytes) {
                throw std::runtime_error(
                    "configuration exceeds the 64 KiB file limit");
            }
            std::ifstream input(path, std::ios::binary);
            if (!input.is_open()) {
                throw std::runtime_error("configuration cannot be opened");
            }
            const std::string text{
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>()};
            Settings = ParseConfig(text);
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            const auto message = std::string(
                "ScriptedAI: invalid configuration ")
                + path.string() + " (" + exception.what() + ").";
            Context->LogError(message.c_str());
            return false;
        } catch (...) {
            Context->LogError(
                "ScriptedAI: invalid configuration (unknown parser failure)."
            );
            return false;
        }
    }
    Context->LogWarn(
        "ScriptedAI: no configuration file was found; compiled disabled defaults are active."
    );
    return true;
}

[[nodiscard]] auto DecodeResolverExpected(
        std::vector<std::uint8_t>& expected,
        std::string& error) -> bool {
    for (const auto& window : NativeFingerprint()) {
        if (window.hookTarget) {
            return DecodeNativeWindow(window, expected, error);
        }
    }
    error = "resolver hook fingerprint is missing";
    return false;
}

[[nodiscard]] auto MapState(
        D2RL::Diagnostics::ModificationState value) noexcept
        -> OwnershipState {
    switch (value) {
        case D2RL::Diagnostics::ModificationState::Unchanged:
            return OwnershipState::Unchanged;
        case D2RL::Diagnostics::ModificationState::Tracked:
            return OwnershipState::Tracked;
        case D2RL::Diagnostics::ModificationState::Untracked:
            return OwnershipState::Untracked;
    }
    return OwnershipState::Untracked;
}

[[nodiscard]] auto MapKind(
        D2RL::Diagnostics::ModificationKind value) noexcept
        -> OwnershipKind {
    switch (value) {
        case D2RL::Diagnostics::ModificationKind::Unknown:
            return OwnershipKind::Unknown;
        case D2RL::Diagnostics::ModificationKind::BytePatch:
            return OwnershipKind::BytePatch;
        case D2RL::Diagnostics::ModificationKind::InlineHook:
            return OwnershipKind::InlineHook;
        case D2RL::Diagnostics::ModificationKind::Multiple:
            return OwnershipKind::Multiple;
    }
    return OwnershipKind::Unknown;
}

[[nodiscard]] auto ValidateRequiredServices() noexcept -> bool {
    if (Context->QueryService(
            D2RL::ServiceId::Diagnostics,
            D2RL::DiagnosticsServiceV1Version,
            &Diagnostics) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasDiagnosticsServiceV1Field(
                Diagnostics,
                D2RL::DiagnosticsServiceV1RequiredSize)
            || Diagnostics->queryHookStatus == nullptr) {
        Context->LogError(
            "ScriptedAI: D2RLoader DiagnosticsService v1 is required."
        );
        return false;
    }
    if (Context->QueryService(
            D2RL::ServiceId::Lifecycle,
            D2RL::LifecycleServiceV1Version,
            &Lifecycle) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasLifecycleServiceV1Field(
                Lifecycle,
                D2RL::LifecycleServiceV1RequiredSize)
            || Lifecycle->registerGameplayEventListener == nullptr
            || Lifecycle->unregisterGameplayEventListener == nullptr) {
        Context->LogError(
            "ScriptedAI: D2RLoader LifecycleService v1 is required."
        );
        return false;
    }
    if (Context->QueryService(
            D2RL::ServiceId::Thread,
            D2RL::ThreadServiceV1Version,
            &Threads) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasThreadServiceV1Field(
                Threads,
                D2RL::ThreadServiceV1RequiredSize)
            || Threads->runOnGameThread == nullptr) {
        Context->LogError(
            "ScriptedAI: D2RLoader ThreadService v1 with runOnGameThread is required."
        );
        return false;
    }
    return true;
}

[[nodiscard]] auto ValidateResolverOwnership() -> bool {
    std::vector<std::uint8_t> expected;
    std::string decodeError;
    if (!DecodeResolverExpected(expected, decodeError)) {
        const auto message = std::string(
            "ScriptedAI: invalid resolver fingerprint (")
            + decodeError + ").";
        Context->LogError(message.c_str());
        return false;
    }
    const D2RL::Diagnostics::HookQuery query{
        .structSize = D2RL::Diagnostics::HookQuerySize,
        .flags = 0U,
        .rva = ResolverHookRva,
        .expected = expected.data(),
        .expectedSize = static_cast<std::uint32_t>(expected.size()),
        .reserved = 0U,
    };
    D2RL::Diagnostics::HookStatus status{
        .structSize = D2RL::Diagnostics::HookStatusSize,
    };
    if (Diagnostics->queryHookStatus(Context, &query, &status)
            != D2RL::Diagnostics::Result::Success
            || status.structSize
                < D2RL::Diagnostics::HookStatusRequiredSize) {
        Context->LogError(
            "ScriptedAI: resolver ownership could not be attested."
        );
        return false;
    }
    std::size_t ownerLength{};
    while (ownerLength < std::size(status.ownerPluginId)
            && status.ownerPluginId[ownerLength] != '\0') {
        ++ownerLength;
    }
    const OwnershipObservation observation{
        .state = MapState(status.state),
        .kind = MapKind(status.kind),
        .ownerCount = status.ownerCount,
        .ownerPluginId = status.ownerCount == 1U
            ? std::string_view(status.ownerPluginId, ownerLength)
            : std::string_view{},
    };
    if (!IsSafeBeforeInstall(observation)) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "ScriptedAI: resolver ownership conflict (owners=%u, owner=%.*s).",
            status.ownerCount,
            status.ownerCount == 1U
                ? static_cast<int>(ownerLength)
                : static_cast<int>(sizeof("<multiple-or-unknown>") - 1U),
            status.ownerCount == 1U
                ? status.ownerPluginId
                : "<multiple-or-unknown>");
        Context->LogError(message);
        return false;
    }
    return true;
}

[[nodiscard]] auto CheckNativeWindow(
        void*,
        std::uintptr_t rva,
        std::span<const std::uint8_t> expected) noexcept -> bool {
    return Context != nullptr && Context->CheckExpectedBytes(
        rva,
        expected.data(),
        static_cast<std::uint32_t>(expected.size()));
}

void ResetState() noexcept {
    Threads = nullptr;
    Lifecycle = nullptr;
    Diagnostics = nullptr;
    Context = nullptr;
    Settings.enabled = false;
    LoadedConfigPath.clear();
}

[[nodiscard]] auto LoadPluginImpl(
        const D2RL::PluginContext* context) -> bool {
    Context = context;
    if (!LoadConfig()) {
        ResetState();
        return false;
    }
    if (!Settings.enabled) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "RuffnecKk Scripted AI %s loaded disabled from %s; no native surface was read and no hook or Lua VM was created.",
            Version,
            LoadedConfigPath.c_str());
        context->LogInfo(message);
        return true;
    }

    const auto* build = D2RL::GetBuildName(context);
    const auto* buildLabel = build != nullptr ? build : "<unknown>";
    if (!ValidateRequiredServices() || !ValidateResolverOwnership()) {
        ResetState();
        return false;
    }
    const auto fingerprint = ValidateNativeFingerprint(
        CheckNativeWindow,
        nullptr);
    if (!fingerprint.accepted) {
        const auto message = std::string(
            "ScriptedAI: native fingerprint refused reported D2R build ")
            + buildLabel + " (" + fingerprint.error + ").";
        context->LogError(message.c_str());
        ResetState();
        return false;
    }

    std::string sandboxError;
    const auto sandbox = Sandbox::Create(Settings.limits, sandboxError);
    if (!sandbox) {
        const auto message = std::string(
            "ScriptedAI: bounded Lua 5.4.9 VM preflight failed (")
            + sandboxError + ").";
        context->LogError(message.c_str());
        ResetState();
        return false;
    }

    context->LogError(
        "ScriptedAI: enabled=true passed native ownership, fingerprint and temporary Lua sandbox preflights, but the AI bridge is intentionally unavailable in 0.1.0; no hook or persistent gameplay VM was created."
    );
    ResetState();
    return false;
}

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept
        -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    if (!D2RL::HasContext(context)
            || context->apiVersion != D2RL_PLUGIN_API_VERSION) {
        return false;
    }
    try {
        return LoadPluginImpl(context);
    } catch (const std::exception& exception) {
        char message[512]{};
        std::snprintf(
            message,
            sizeof(message),
            "ScriptedAI: initialization failed closed (%s).",
            exception.what());
        context->LogError(message);
        ResetState();
        return false;
    } catch (...) {
        context->LogError(
            "ScriptedAI: initialization failed closed (unknown exception)."
        );
        ResetState();
        return false;
    }
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    ResetState();
}
