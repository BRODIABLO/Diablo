#define NOMINMAX
#include <D2RLPlugin/api.h>

#include "scripted_ai_bridge.hpp"
#include "scripted_ai_config.hpp"
#include "scripted_ai_fingerprint.hpp"
#include "scripted_ai_ownership.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace ruffneckk::scripted_ai;

constexpr char Version[] = "0.2.0";
constexpr char PluginId[] = "ruffneckk-scripted-ai";
constexpr std::size_t MaximumConfigBytes = 64U * 1024U;
constexpr char AiScriptTableName[] = "aiscript";
constexpr char BaseAiScriptResource[] =
    "data/global/excel/base/d2rloader/ruffneckk-scripted-ai/aiscript.txt";
constexpr char RotwAiScriptResource[] =
    "data/global/excel/d2rloader/ruffneckk-scripted-ai/aiscript.txt";
constexpr char EmptyAiScriptTable[] =
    "MonStatsId\tScript\tFallbackAi\tTargetProfile\tEnabled\n";

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

constexpr std::array<D2RL::CustomTables::ColumnDefinition, 5U>
    AiScriptColumns{{
        {
            .structSize = D2RL::CustomTables::ColumnDefinitionSize,
            .flags = 0U,
            .name = "MonStatsId",
            .type = D2RL::CustomTables::ColumnType::Dword,
            .offset = static_cast<std::uint32_t>(
                offsetof(AiScriptTableRow, monStatsId)),
            .length = 0U,
            .reserved = 0U,
        },
        {
            .structSize = D2RL::CustomTables::ColumnDefinitionSize,
            .flags = 0U,
            .name = "Script",
            .type = D2RL::CustomTables::ColumnType::Ascii,
            .offset = static_cast<std::uint32_t>(
                offsetof(AiScriptTableRow, script)),
            .length = static_cast<std::uint32_t>(AiScriptNameCapacity),
            .reserved = 0U,
        },
        {
            .structSize = D2RL::CustomTables::ColumnDefinitionSize,
            .flags = 0U,
            .name = "FallbackAi",
            .type = D2RL::CustomTables::ColumnType::Word,
            .offset = static_cast<std::uint32_t>(
                offsetof(AiScriptTableRow, fallbackAi)),
            .length = 0U,
            .reserved = 0U,
        },
        {
            .structSize = D2RL::CustomTables::ColumnDefinitionSize,
            .flags = 0U,
            .name = "TargetProfile",
            .type = D2RL::CustomTables::ColumnType::Byte,
            .offset = static_cast<std::uint32_t>(
                offsetof(AiScriptTableRow, targetProfile)),
            .length = 0U,
            .reserved = 0U,
        },
        {
            .structSize = D2RL::CustomTables::ColumnDefinitionSize,
            .flags = 0U,
            .name = "Enabled",
            .type = D2RL::CustomTables::ColumnType::Byte,
            .offset = static_cast<std::uint32_t>(
                offsetof(AiScriptTableRow, enabled)),
            .length = 0U,
            .reserved = 0U,
        },
    }};

const D2RL::PluginContext* Context{};
const D2RL::DiagnosticsServiceV1* Diagnostics{};
const D2RL::LifecycleServiceV1* Lifecycle{};
const D2RL::ThreadServiceV1* Threads{};
const D2RL::ResourceServiceV1* Resources{};
const D2RL::CustomTableServiceV1* CustomTables{};
Config Settings{};
std::string LoadedConfigPath{"compiled disabled defaults"};
std::filesystem::path ScriptRoot;
std::unique_ptr<BridgeCoordinator> Bridge;

D2RL::Resources::RegistrationHandle BaseResourceHandle{
    D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle RotwResourceHandle{
    D2RL::Resources::InvalidHandle};
D2RL::CustomTables::TableHandle AiScriptHandle{
    D2RL::CustomTables::InvalidHandle};
D2RL::Lifecycle::ListenerHandle DataTablesListener{
    D2RL::Lifecycle::InvalidHandle};
D2RL::Lifecycle::ListenerHandle GameJoinedListener{
    D2RL::Lifecycle::InvalidHandle};
D2RL::Lifecycle::ListenerHandle GameLeftListener{
    D2RL::Lifecycle::InvalidHandle};

void __cdecl OnGameThreadLifecycle(
    const D2RL::PluginContext* context,
    void* userData) noexcept;

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

[[nodiscard]] auto ResolveScriptRoot(std::string& error) -> bool {
    std::filesystem::path scopeRoot;
    if (Context->activeMod != nullptr && Context->activeMod[0] != '\0'
            && Context->modSupportDirectory != nullptr
            && Context->modSupportDirectory[0] != L'\0') {
        scopeRoot = Context->modSupportDirectory;
    } else if (Context->scopeRootDirectory != nullptr
            && Context->scopeRootDirectory[0] != L'\0') {
        scopeRoot = Context->scopeRootDirectory;
    }
    if (scopeRoot.empty()) {
        error = "D2RLoader did not provide a script scope root";
        return false;
    }
    ScriptRoot = scopeRoot / std::filesystem::path(Settings.scriptDirectory);
    error.clear();
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
            || Lifecycle->registerDataTablesLoadedListener == nullptr
            || Lifecycle->unregisterDataTablesLoadedListener == nullptr
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
    if (Context->QueryService(
            D2RL::ServiceId::Resource,
            D2RL::ResourceServiceV1Version,
            &Resources) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasResourceServiceV1Field(
                Resources,
                D2RL::ResourceServiceV1RequiredSize)
            || Resources->registerResource == nullptr
            || Resources->unregisterResource == nullptr) {
        Context->LogError(
            "ScriptedAI: D2RLoader ResourceService v1 is required."
        );
        return false;
    }
    if (Context->QueryService(
            D2RL::ServiceId::CustomTable,
            D2RL::CustomTableServiceV1Version,
            &CustomTables) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasCustomTableServiceV1Field(
                CustomTables,
                D2RL::CustomTableServiceV1RequiredSize)
            || CustomTables->registerTable == nullptr
            || CustomTables->unregisterTable == nullptr
            || CustomTables->getTableInfo == nullptr
            || CustomTables->copyRows == nullptr) {
        Context->LogError(
            "ScriptedAI: D2RLoader CustomTableService v1 is required."
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

void CleanupBridgeRegistration() noexcept {
    if (Context != nullptr && Lifecycle != nullptr) {
        if (GameLeftListener != D2RL::Lifecycle::InvalidHandle) {
            (void)Lifecycle->unregisterGameplayEventListener(
                Context, GameLeftListener);
        }
        if (GameJoinedListener != D2RL::Lifecycle::InvalidHandle) {
            (void)Lifecycle->unregisterGameplayEventListener(
                Context, GameJoinedListener);
        }
        if (DataTablesListener != D2RL::Lifecycle::InvalidHandle) {
            (void)Lifecycle->unregisterDataTablesLoadedListener(
                Context, DataTablesListener);
        }
    }
    GameLeftListener = D2RL::Lifecycle::InvalidHandle;
    GameJoinedListener = D2RL::Lifecycle::InvalidHandle;
    DataTablesListener = D2RL::Lifecycle::InvalidHandle;

    if (Context != nullptr && CustomTables != nullptr
            && AiScriptHandle != D2RL::CustomTables::InvalidHandle) {
        (void)CustomTables->unregisterTable(Context, AiScriptHandle);
    }
    AiScriptHandle = D2RL::CustomTables::InvalidHandle;

    if (Context != nullptr && Resources != nullptr) {
        if (RotwResourceHandle != D2RL::Resources::InvalidHandle) {
            (void)Resources->unregisterResource(Context, RotwResourceHandle);
        }
        if (BaseResourceHandle != D2RL::Resources::InvalidHandle) {
            (void)Resources->unregisterResource(Context, BaseResourceHandle);
        }
    }
    RotwResourceHandle = D2RL::Resources::InvalidHandle;
    BaseResourceHandle = D2RL::Resources::InvalidHandle;

    if (Bridge) Bridge->ResetGameThread();
    Bridge.reset();
    ScriptRoot.clear();
}

void ResetState() noexcept {
    CleanupBridgeRegistration();
    CustomTables = nullptr;
    Resources = nullptr;
    Threads = nullptr;
    Lifecycle = nullptr;
    Diagnostics = nullptr;
    Context = nullptr;
    Settings.enabled = false;
    LoadedConfigPath.clear();
}

[[nodiscard]] auto RegisterResource(
        const char* path,
        D2RL::Resources::RegistrationHandle& handle) noexcept -> bool {
    const D2RL::Resources::ResourceRegistration registration{
        .structSize = D2RL::Resources::ResourceRegistrationSize,
        .flags = 0U,
        .path = path,
        .bytes = EmptyAiScriptTable,
        .byteCount = sizeof(EmptyAiScriptTable) - 1U,
    };
    return Resources->registerResource(Context, &registration, &handle)
        == D2RL::Resources::Result::Success;
}

[[nodiscard]] auto RegisterBridgeResourcesAndTable() noexcept -> bool {
    if (!RegisterResource(BaseAiScriptResource, BaseResourceHandle)
            || !RegisterResource(RotwAiScriptResource, RotwResourceHandle)) {
        Context->LogError(
            "ScriptedAI: default AIScript resources could not be registered."
        );
        return false;
    }
    const D2RL::CustomTables::TableRegistration registration{
        .structSize = D2RL::CustomTables::TableRegistrationSize,
        .flags = 0U,
        .name = AiScriptTableName,
        .banks = D2RL::CustomTables::TableBank::All,
        .rowSize = static_cast<std::uint32_t>(sizeof(AiScriptTableRow)),
        .columns = AiScriptColumns.data(),
        .columnCount = static_cast<std::uint32_t>(AiScriptColumns.size()),
        .columnStride = D2RL::CustomTables::ColumnDefinitionSize,
    };
    if (CustomTables->registerTable(
            Context,
            &registration,
            &AiScriptHandle) != D2RL::CustomTables::Result::Success) {
        Context->LogError(
            "ScriptedAI: the private AIScript custom table could not be registered."
        );
        return false;
    }
    return true;
}

[[nodiscard]] auto CopyTableBank(
        D2RL::CustomTables::TableBank bank,
        std::vector<AiScriptTableRow>& rows,
        std::uint64_t& revision,
        std::string& error) -> bool {
    D2RL::CustomTables::TableInfo info{
        .structSize = D2RL::CustomTables::TableInfoSize,
    };
    const auto infoResult = CustomTables->getTableInfo(
        Context,
        AiScriptHandle,
        bank,
        &info);
    if (infoResult != D2RL::CustomTables::Result::Success
            || info.structSize < D2RL::CustomTables::TableInfoRequiredSize
            || info.state != D2RL::CustomTables::TableState::Ready) {
        error = "AIScript table bank is not Ready";
        return false;
    }
    const auto expectedBytes = static_cast<std::uint64_t>(info.rowCount)
        * sizeof(AiScriptTableRow);
    if (info.rowSize != sizeof(AiScriptTableRow)
            || info.rowCount > MaximumAiScriptRows
            || info.byteCount != expectedBytes) {
        error = "AIScript table metadata violates the frozen row contract";
        return false;
    }
    rows.resize(info.rowCount);
    const auto copyResult = CustomTables->copyRows(
        Context,
        AiScriptHandle,
        bank,
        info.revision,
        rows.empty() ? nullptr : rows.data(),
        info.byteCount);
    if (copyResult != D2RL::CustomTables::Result::Success) {
        error = copyResult == D2RL::CustomTables::Result::StaleRevision
            ? "AIScript table changed during its atomic copy"
            : "AIScript rows could not be copied";
        rows.clear();
        return false;
    }
    revision = info.revision;
    return true;
}

[[nodiscard]] auto QueueAuthoritativeReconcile(const char* source) noexcept
        -> D2RL::Threads::Result {
    const auto result = Threads->runOnGameThread(
        Context,
        OnGameThreadLifecycle,
        nullptr);
    if (result == D2RL::Threads::Result::Unavailable) {
        if (Settings.diagnostics) {
            char message[256]{};
            std::snprintf(
                message,
                sizeof(message),
                "ScriptedAI: %s observed no authoritative game thread; no Lua VM was created.",
                source);
            Context->LogInfo(message);
        }
    } else if (result != D2RL::Threads::Result::Success) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "ScriptedAI: %s could not queue authoritative reconciliation (result=%u).",
            source,
            static_cast<unsigned>(result));
        Context->LogWarn(message);
    }
    return result;
}

void __cdecl OnDataTablesLoaded(
        const D2RL::PluginContext*,
        const D2RL::Lifecycle::DataTablesLoadedEvent* event,
        void*) noexcept {
    try {
        if (Bridge == nullptr
                || !D2RL::Lifecycle::HasDataTablesLoadedEventField(
                    event,
                    D2RL::Lifecycle::DataTablesLoadedEventRequiredSize)) {
            return;
        }
        std::vector<AiScriptTableRow> baseRows;
        std::vector<AiScriptTableRow> rotwRows;
        std::uint64_t baseRevision{};
        std::uint64_t rotwRevision{};
        std::string error;
        if (!CopyTableBank(
                D2RL::CustomTables::TableBank::Base,
                baseRows,
                baseRevision,
                error)
                || !CopyTableBank(
                    D2RL::CustomTables::TableBank::Rotw,
                    rotwRows,
                    rotwRevision,
                    error)) {
            const auto message = std::string(
                "ScriptedAI: rejected AIScript table transaction (")
                + error + "); the prior generation remains published.";
            Context->LogError(message.c_str());
            return;
        }
        auto candidate = StagePreparedBundle(
            event->revision,
            {.revision = baseRevision, .rows = baseRows},
            {.revision = rotwRevision, .rows = rotwRows},
            ScriptRoot,
            Settings.limits,
            ReadScriptSource,
            error);
        if (!candidate || !Bridge->PublishPrepared(candidate, error)) {
            const auto message = std::string(
                "ScriptedAI: rejected AIScript source transaction (")
                + error + "); the prior generation remains published.";
            Context->LogError(message.c_str());
            return;
        }
        if (Settings.diagnostics) {
            char message[256]{};
            std::snprintf(
                message,
                sizeof(message),
                "ScriptedAI: staged AIScript revision %llu (%zu unique scripts); publication awaits authoritative compilation.",
                static_cast<unsigned long long>(event->revision),
                candidate->scripts.size());
            Context->LogInfo(message);
        }
        (void)QueueAuthoritativeReconcile("DataTablesLoaded");
    } catch (const std::exception& exception) {
        const auto message = std::string(
            "ScriptedAI: AIScript data callback failed closed (")
            + exception.what() + ").";
        Context->LogError(message.c_str());
    } catch (...) {
        Context->LogError(
            "ScriptedAI: AIScript data callback failed closed (unknown failure)."
        );
    }
}

void __cdecl OnGameplayEvent(
        const D2RL::PluginContext*,
        const D2RL::Lifecycle::GameplayEvent* event,
        void*) noexcept {
    if (Bridge == nullptr
            || !D2RL::Lifecycle::HasGameplayEventField(
                event,
                D2RL::Lifecycle::GameplayEventRequiredSize)) {
        return;
    }
    if (event->kind == D2RL::Lifecycle::GameplayEventKind::GameJoined) {
        Bridge->AnnounceGameJoined(event->sessionGeneration);
        (void)QueueAuthoritativeReconcile("GameJoined");
    } else if (event->kind == D2RL::Lifecycle::GameplayEventKind::GameLeft) {
        Bridge->AnnounceGameLeft(event->sessionGeneration);
        (void)QueueAuthoritativeReconcile("GameLeft");
    }
}

void __cdecl OnGameThreadLifecycle(
        const D2RL::PluginContext*,
        void*) noexcept {
    try {
        if (Bridge == nullptr) return;
        std::string error;
        if (!Bridge->ReconcileAuthoritativeSession(error)) {
            const auto message = std::string(
                "ScriptedAI: authoritative generation reconciliation failed (")
                + error + "); the prior same-session generation remains published.";
            Context->LogError(message.c_str());
            return;
        }
        if (Settings.diagnostics) {
            const auto session = Bridge->DesiredSession();
            const auto active = Bridge->ActiveFor(session);
            char message[256]{};
            std::snprintf(
                message,
                sizeof(message),
                "ScriptedAI: reconciled authoritative session %llu (%zu scripts, Lua VM=%s).",
                static_cast<unsigned long long>(session),
                active ? active->ScriptCount() : 0U,
                active && active->HasLuaVm() ? "yes" : "no");
            Context->LogInfo(message);
        }
    } catch (const std::exception& exception) {
        const auto message = std::string(
            "ScriptedAI: authoritative reconciliation failed closed (")
            + exception.what() + ").";
        Context->LogError(message.c_str());
    } catch (...) {
        Context->LogError(
            "ScriptedAI: authoritative reconciliation failed closed (unknown failure)."
        );
    }
}

[[nodiscard]] auto RegisterLifecycleListeners() noexcept -> bool {
    const D2RL::Lifecycle::DataTablesLoadedListener dataListener{
        .structSize = D2RL::Lifecycle::DataTablesLoadedListenerSize,
        .flags = 0U,
        .callback = OnDataTablesLoaded,
        .userData = nullptr,
    };
    if (Lifecycle->registerDataTablesLoadedListener(
            Context,
            &dataListener,
            &DataTablesListener) != D2RL::Lifecycle::Result::Success) {
        Context->LogError(
            "ScriptedAI: DataTablesLoaded listener registration failed."
        );
        return false;
    }

    const auto registerGameplay = [](
            D2RL::Lifecycle::GameplayEventKind kind,
            D2RL::Lifecycle::ListenerHandle& handle) noexcept {
        const D2RL::Lifecycle::GameplayEventListener listener{
            .structSize = D2RL::Lifecycle::GameplayEventListenerSize,
            .flags = 0U,
            .kind = kind,
            .reserved = 0U,
            .callback = OnGameplayEvent,
            .userData = nullptr,
        };
        return Lifecycle->registerGameplayEventListener(
            Context,
            &listener,
            &handle) == D2RL::Lifecycle::Result::Success;
    };
    if (!registerGameplay(
            D2RL::Lifecycle::GameplayEventKind::GameJoined,
            GameJoinedListener)
            || !registerGameplay(
                D2RL::Lifecycle::GameplayEventKind::GameLeft,
                GameLeftListener)) {
        Context->LogError(
            "ScriptedAI: gameplay lifecycle listener registration failed."
        );
        return false;
    }
    return true;
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
            "RuffnecKk Scripted AI %s loaded disabled from %s; no service, native surface, hook, or Lua VM was touched.",
            Version,
            LoadedConfigPath.c_str());
        context->LogInfo(message);
        return true;
    }

    const auto* build = D2RL::GetBuildName(context);
    const auto* buildLabel = build != nullptr ? build : "<unknown>";
    std::string rootError;
    if (!ValidateRequiredServices()
            || !ResolveScriptRoot(rootError)
            || !ValidateResolverOwnership()) {
        if (!rootError.empty()) {
            const auto message = std::string(
                "ScriptedAI: script root initialization failed (")
                + rootError + ").";
            context->LogError(message.c_str());
        }
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

    Bridge = std::make_unique<BridgeCoordinator>(Settings.limits);
    if (!RegisterBridgeResourcesAndTable()
            || !RegisterLifecycleListeners()) {
        ResetState();
        return false;
    }

    char message[512]{};
    std::snprintf(
        message,
        sizeof(message),
        "RuffnecKk Scripted AI %s registered its private Base+RotW AIScript transaction bridge from %s for reported build %s; Lua generation creation is authoritative-session-only, and no resolver hook or gameplay action is installed.",
        Version,
        LoadedConfigPath.c_str(),
        buildLabel);
    context->LogInfo(message);
    return true;
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
