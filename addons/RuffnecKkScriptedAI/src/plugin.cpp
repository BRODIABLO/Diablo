#define NOMINMAX
#include <Windows.h>
#include <D2RLPlugin/api.h>

#include "scripted_ai_bridge.hpp"
#include "scripted_ai_config.hpp"
#include "scripted_ai_d2r.hpp"
#include "scripted_ai_fingerprint.hpp"
#include "scripted_ai_ownership.hpp"
#include "scripted_ai_revive_abi.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

using namespace ruffneckk::scripted_ai;

constexpr char Version[] = "0.7.0";
constexpr char PluginId[] = "ruffneckk-scripted-ai";
constexpr std::size_t MaximumConfigBytes = 64U * 1024U;
constexpr std::size_t MaximumAiScriptTableBytes = 1024U * 1024U;
constexpr char AiScriptTableName[] = "aiscript";
constexpr wchar_t AiScriptSupportDirectory[] = L"ruffneckk-scripted-ai";
constexpr wchar_t BaseAiScriptFileName[] = L"aiscript-base.txt";
constexpr wchar_t RotwAiScriptFileName[] = L"aiscript-rotw.txt";
constexpr char BaseAiScriptResource[] =
    "data/global/excel/base/d2rloader/ruffneckk-scripted-ai/aiscript.txt";
constexpr char RotwAiScriptResource[] =
    "data/global/excel/d2rloader/ruffneckk-scripted-ai/aiscript.txt";
constexpr char DefaultAiScriptTable[] =
    "MonStatsId\tScript\tFallbackAi\tTargetProfile\tEnabled\n"
    "0\t\t0\t0\t0\n";

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
const D2RL::DataTableServiceV1* DataTables{};
Config Settings{};
std::string LoadedConfigPath{"compiled disabled defaults"};
std::filesystem::path ScriptRoot;
std::unique_ptr<BridgeCoordinator> Bridge;
using ResolverFunction = const D2AiTableRecord*(__fastcall*)(
    void* unit,
    std::int32_t specialState) noexcept;
ResolverFunction OriginalResolver{};
D2NativeFunctions NativeFunctions{};
std::array<NativeTableBankView, 3U> NativeBanks{};
bool NativeBanksReady{};
std::atomic<std::uint64_t> AttestedSession{};
std::atomic<std::uint64_t> FirstRoutedThinkSession{};
std::atomic<std::uint64_t> ThinkToken{};
std::atomic<std::uint32_t> AuthoritativeThread{};
std::atomic<bool> Operational{};
std::atomic<std::uint64_t> RevivePolicyCalls{};
std::atomic<std::uint64_t> RevivePolicyNativeFollowRequests{};
std::atomic<std::uint64_t> RevivePolicyDelegates{};
std::atomic<std::uint64_t> RevivePolicyFailures{};
std::atomic<std::uint64_t> FirstRevivePolicySession{};
std::atomic<std::uint64_t> ReviveTacticalCalls{};
std::atomic<std::uint64_t> ReviveTacticalHandled{};
std::atomic<std::uint64_t> ReviveTacticalDelegates{};
std::atomic<std::uint64_t> ReviveTacticalFailures{};
std::atomic<std::uint64_t> ReviveTargetLocks{};
std::atomic<std::uint64_t> ReviveTargetLockReuses{};
std::atomic<std::uint64_t> ReviveSpellRotations{};
std::atomic<std::uint64_t> FirstReviveTacticalSession{};

constexpr std::size_t MaximumReviveTacticalMemories = 2'048U;
constexpr std::int32_t ReviveTacticalHardRadius = 32;

struct ReviveTacticalMemory {
    std::uint64_t session{};
    std::uint64_t lastSeenToken{};
    std::uint32_t monsterClassId{};
    std::uint32_t monsterGuid{InvalidUnitId};
    std::int32_t targetType{-1};
    std::uint32_t targetGuid{InvalidUnitId};
    std::uintptr_t targetAddress{};
    ActionKind lastAction{ActionKind::Idle};
    std::uint8_t nextSkillSlot{};
    bool hasTargetLock{};
    bool hasLastAction{};
};

std::mutex ReviveTacticalMemoryMutex;
std::unordered_map<std::uint32_t, ReviveTacticalMemory>
    ReviveTacticalMemories;

[[nodiscard]] auto CurrentThreadId() noexcept -> std::uint32_t {
    return static_cast<std::uint32_t>(::GetCurrentThreadId());
}

[[nodiscard]] constexpr auto ActionName(ActionKind action) noexcept
        -> const char* {
    switch (action) {
    case ActionKind::Idle: return "idle";
    case ActionKind::Wander: return "wander";
    case ActionKind::AttackTarget: return "attack";
    case ActionKind::ChaseTarget: return "chase";
    case ActionKind::RetreatFromTarget: return "retreat";
    case ActionKind::CastOnTarget: return "cast";
    case ActionKind::FollowOwner: return "follow-owner";
    }
    return "unknown";
}

[[nodiscard]] constexpr auto ContinuationName(
        NativeContinuation continuation) noexcept -> const char* {
    switch (continuation) {
    case NativeContinuation::ActionPipeline: return "action";
    case NativeContinuation::CapabilityFallbackScheduled:
        return "capability-fallback";
    case NativeContinuation::FallbackIdleScheduled: return "fallback-idle";
    case NativeContinuation::FallbackIdleFailed: return "fallback-failed";
    case NativeContinuation::InvalidContext: return "invalid-context";
    }
    return "unknown";
}

[[nodiscard]] constexpr auto TacticalProfileName(
        TacticalProfile profile) noexcept -> const char* {
    switch (profile) {
    case TacticalProfile::None: return "none";
    case TacticalProfile::MeleeVanguard: return "melee";
    case TacticalProfile::RangedSkirmisher: return "ranged";
    case TacticalProfile::CasterArtillery: return "caster";
    }
    return "unknown";
}

void ClearReviveTacticalMemories() noexcept {
    std::lock_guard lock(ReviveTacticalMemoryMutex);
    ReviveTacticalMemories.clear();
}

void EraseReviveTacticalMemory(std::uint32_t monsterGuid) noexcept {
    if (monsterGuid == InvalidUnitId) return;
    std::lock_guard lock(ReviveTacticalMemoryMutex);
    ReviveTacticalMemories.erase(monsterGuid);
}

[[nodiscard]] auto LoadReviveTacticalMemory(
        std::uint64_t session,
        std::uint32_t monsterGuid,
        std::uint32_t monsterClassId) noexcept -> ReviveTacticalMemory {
    std::lock_guard lock(ReviveTacticalMemoryMutex);
    const auto found = ReviveTacticalMemories.find(monsterGuid);
    if (found == ReviveTacticalMemories.end()
            || found->second.session != session
            || found->second.monsterClassId != monsterClassId) {
        return {
            .session = session,
            .monsterClassId = monsterClassId,
            .monsterGuid = monsterGuid,
        };
    }
    return found->second;
}

void StoreReviveTacticalMemory(
        const ReviveTacticalMemory& memory) noexcept {
    if (memory.session == 0U || memory.monsterGuid == InvalidUnitId) return;
    std::lock_guard lock(ReviveTacticalMemoryMutex);
    if (!ReviveTacticalMemories.contains(memory.monsterGuid)
            && ReviveTacticalMemories.size()
                >= MaximumReviveTacticalMemories) {
        const auto oldest = std::min_element(
            ReviveTacticalMemories.begin(),
            ReviveTacticalMemories.end(),
            [](const auto& left, const auto& right) {
                return left.second.lastSeenToken
                    < right.second.lastSeenToken;
            });
        if (oldest != ReviveTacticalMemories.end()) {
            ReviveTacticalMemories.erase(oldest);
        }
    }
    ReviveTacticalMemories.insert_or_assign(memory.monsterGuid, memory);
}

void __fastcall OnScriptedAiThink(
    void* game,
    void* unit,
    D2AiTickParam* tick) noexcept;

const D2AiTableRecord ScriptedAiRecord{
    .targetProfile = ResolverTargetProfile,
    .reserved04 = 0U,
    .initialize = nullptr,
    .think = OnScriptedAiThink,
    .transition = nullptr,
};

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
    if (Context->QueryService(
            D2RL::ServiceId::DataTable,
            D2RL::DataTableServiceV1Version,
            &DataTables) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasDataTableServiceV1Field(
                DataTables,
                D2RL::DataTableServiceV1RequiredSize)
            || DataTables->getTable == nullptr) {
        Context->LogError(
            "ScriptedAI: D2RLoader DataTableService v1 is required."
        );
        return false;
    }
    return true;
}

[[nodiscard]] auto ValidateResolverOwnership(bool afterInstall) -> bool {
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
    const auto accepted = afterInstall
        ? IsExclusivelyOwnedAfterInstall(observation, PluginId)
        : IsSafeBeforeInstall(observation);
    if (!accepted) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "ScriptedAI: resolver ownership %s failed (owners=%u, owner=%.*s).",
            afterInstall ? "post-install attestation" : "pre-install check",
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

[[nodiscard]] auto AttestResolverForSession(
        std::uint64_t session) -> bool {
    if (session == 0U) return false;
    if (AttestedSession.load(std::memory_order_acquire) == session) {
        return true;
    }
    if (!ValidateResolverOwnership(true)) return false;
    AttestedSession.store(session, std::memory_order_release);
    if (Settings.diagnostics) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "ScriptedAI: session %llu confirmed exclusive resolver ownership before its first routed lookup.",
            static_cast<unsigned long long>(session));
        Context->LogInfo(message);
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

[[nodiscard]] auto CaptureNativeTableViews(
        std::uint64_t revision,
        std::string& error) noexcept -> bool {
    struct BankRequest {
        D2RL::DataTables::Bank nativeBank;
        ScriptBank scriptBank;
    };
    constexpr std::array requests{
        BankRequest{
            D2RL::DataTables::Bank::Classic,
            ScriptBank::Base,
        },
        BankRequest{
            D2RL::DataTables::Bank::Lod,
            ScriptBank::Base,
        },
        BankRequest{
            D2RL::DataTables::Bank::Rotw,
            ScriptBank::Rotw,
        },
    };
    std::array<NativeTableBankView, requests.size()> candidate{};
    for (std::size_t index{}; index < requests.size(); ++index) {
        D2RL::DataTables::TableView monStats{
            .structSize = D2RL::DataTables::TableViewSize,
        };
        D2RL::DataTables::TableView skills{
            .structSize = D2RL::DataTables::TableViewSize,
        };
        if (DataTables->getTable(
                Context,
                requests[index].nativeBank,
                D2RL::DataTables::TableId::MonStats,
                &monStats) != D2RL::DataTables::Result::Success
                || DataTables->getTable(
                    Context,
                    requests[index].nativeBank,
                    D2RL::DataTables::TableId::Skills,
                    &skills) != D2RL::DataTables::Result::Success
                || !D2RL::DataTables::HasTableViewField(
                    &monStats,
                    D2RL::DataTables::TableViewRequiredSize)
                || !D2RL::DataTables::HasTableViewField(
                    &skills,
                    D2RL::DataTables::TableViewRequiredSize)
                || monStats.revision != revision
                || skills.revision != revision
                || monStats.rows == nullptr || monStats.rowCount == 0U
                || monStats.rowSize < MonStatsRequiredRecordSize
                || skills.rowCount == 0U) {
            error = "authoritative MonStats/Skills table views are unavailable";
            return false;
        }
        candidate[index] = {
            .rows = static_cast<const std::byte*>(monStats.rows),
            .rowCount = monStats.rowCount,
            .rowSize = monStats.rowSize,
            .skillRowCount = skills.rowCount,
            .scriptBank = requests[index].scriptBank,
        };
    }
    NativeBanks = candidate;
    NativeBanksReady = true;
    error.clear();
    return true;
}

void ScheduleEmergencyIdle(void* game, void* unit) noexcept {
    if (game != nullptr && unit != nullptr && NativeFunctions.idle != nullptr) {
        NativeFunctions.idle(game, unit, NativeFallbackIdleFrames);
    }
}

[[nodiscard]] auto NextThinkToken() noexcept -> std::uint64_t {
    auto token = ThinkToken.fetch_add(1U, std::memory_order_relaxed) + 1U;
    if (token == 0U) {
        token = ThinkToken.fetch_add(1U, std::memory_order_relaxed) + 1U;
    }
    return token;
}

[[nodiscard]] auto ResolveNativeBank(
        const void* monStats,
        std::uint32_t classId) noexcept -> ResolvedNativeBank {
    return NativeBanksReady
        ? ClassifyMonStatsRecord(monStats, classId, NativeBanks)
        : ResolvedNativeBank{};
}

[[nodiscard]] auto __fastcall HookAiTableResolver(
        void* unit,
        std::int32_t specialState) noexcept -> const D2AiTableRecord* {
    const auto* const original = OriginalResolver != nullptr
        ? OriginalResolver(unit, specialState)
        : nullptr;
    try {
        if (!Operational.load(std::memory_order_acquire)
                || specialState != 0 || unit == nullptr || Bridge == nullptr
                || Context == nullptr || NativeFunctions.getClassId == nullptr
                || NativeFunctions.getUnitType == nullptr
                || NativeFunctions.getUnitType(unit)
                    != static_cast<std::int32_t>(MonsterUnitType)) {
            return original;
        }
        const auto session = Bridge->DesiredSession();
        const auto generation = Bridge->ActiveFor(session);
        if (!generation || CurrentThreadId()
                != AuthoritativeThread.load(std::memory_order_acquire)
                || !AttestResolverForSession(session)) {
            return original;
        }
        const auto classId = NativeFunctions.getClassId(unit);
        if (classId < 0) return original;
        const auto nativeBank = ResolveNativeBank(
            ReadUnitMonStatsRecord(unit),
            static_cast<std::uint32_t>(classId));
        if (!nativeBank.found) return original;
        const auto binding = generation->InspectBinding(
            nativeBank.scriptBank,
            static_cast<std::uint32_t>(classId));
        const auto decision = SelectPreCallbackRoute(
            specialState,
            generation->SessionId() == session,
            binding);
        if (decision.route == PreCallbackRoute::ScriptedBridge) {
            return &ScriptedAiRecord;
        }
        if (decision.route == PreCallbackRoute::StockFallback) {
            const auto* const fallback = StockAiRecord(
                Context->exeBase,
                decision.fallbackAi);
            return fallback != nullptr ? fallback : original;
        }
        return original;
    } catch (...) {
        return original;
    }
}

void __fastcall OnScriptedAiThink(
        void* game,
        void* unit,
        D2AiTickParam* tick) noexcept {
    try {
        if (!Operational.load(std::memory_order_acquire)
                || game == nullptr || unit == nullptr || tick == nullptr
                || Bridge == nullptr || NativeFunctions.getClassId == nullptr) {
            ScheduleEmergencyIdle(game, unit);
            return;
        }
        const auto session = Bridge->DesiredSession();
        const auto generation = Bridge->ActiveFor(session);
        const auto classId = NativeFunctions.getClassId(unit);
        if (!generation || classId < 0
                || AttestedSession.load(std::memory_order_acquire) != session) {
            ScheduleEmergencyIdle(game, unit);
            return;
        }
        const auto nativeBank = ResolveNativeBank(
            tick->monStats,
            static_cast<std::uint32_t>(classId));
        if (!nativeBank.found) {
            ScheduleEmergencyIdle(game, unit);
            return;
        }
        D2NativeActionAdapter adapter{
            NativeFunctions,
            session,
            AuthoritativeThread.load(std::memory_order_acquire),
            nativeBank.skillRowCount,
            nativeBank.monStats,
            CurrentThreadId,
        };
        const NativeThinkContext nativeContext{
            .game = game,
            .unit = unit,
            .target = tick->target,
            .sessionGeneration = session,
            .thinkToken = NextThinkToken(),
            .monStatsId = static_cast<std::uint32_t>(classId),
            .targetDistance = tick->targetDistance,
            .inCombat = tick->inCombat != 0,
        };
        std::string error;
        const auto execution = ExecuteNativeThink(
            *generation,
            nativeBank.scriptBank,
            nativeContext,
            adapter,
            {},
            error);
        if (Settings.diagnostics
                && FirstRoutedThinkSession.exchange(
                    session,
                    std::memory_order_acq_rel) != session) {
            char message[512]{};
            std::snprintf(
                message,
                sizeof(message),
                "ScriptedAI: session %llu routed its first scripted think (MonStatsId=%u, enteredLua=%s, disposition=%s, action=%s, continuation=%s, adapterCalls=%u, lua_us=%llu).",
                static_cast<unsigned long long>(session),
                nativeContext.monStatsId,
                execution.decision.enteredLua ? "yes" : "no",
                execution.decision.disposition == ThinkDisposition::Action
                    ? "action" : "fallback",
                ActionName(execution.decision.action.kind),
                ContinuationName(execution.continuation),
                execution.adapterCalls,
                static_cast<unsigned long long>(
                    execution.decision.luaMicroseconds));
            Context->LogInfo(message);
        }
        if (execution.continuation == NativeContinuation::InvalidContext) {
            (void)adapter.TryIdle(nativeContext, NativeFallbackIdleFrames);
        }
    } catch (...) {
        ScheduleEmergencyIdle(game, unit);
    }
}

[[nodiscard]] auto __cdecl EvaluateRevivePolicy(
        const revive_v2::Context* request) noexcept -> revive_v2::Result {
    RevivePolicyCalls.fetch_add(1U, std::memory_order_relaxed);
    try {
        if (request == nullptr || request->structSize < sizeof(*request)
                || request->abiVersion != revive_v2::AbiVersion
                || request->game == nullptr || request->monster == nullptr
                || request->aiTickParam == nullptr) {
            RevivePolicyFailures.fetch_add(1U, std::memory_order_relaxed);
            return revive_v2::Result::Error;
        }
        if (!Operational.load(std::memory_order_acquire)
                || !Settings.revive.enabled || Bridge == nullptr
                || request->owner == nullptr || request->ownerDistance < 0
                || (request->specialState != 0
                    && request->specialState != 7)) {
            RevivePolicyDelegates.fetch_add(1U, std::memory_order_relaxed);
            return revive_v2::Result::DelegateNative;
        }

        const auto session = Bridge->DesiredSession();
        const auto generation = Bridge->ActiveFor(session);
        if (!generation || !generation->HasReviveScript()
                || CurrentThreadId()
                    != AuthoritativeThread.load(std::memory_order_acquire)
                || !AttestResolverForSession(session)
                || NativeFunctions.getClassId == nullptr) {
            RevivePolicyDelegates.fetch_add(1U, std::memory_order_relaxed);
            return revive_v2::Result::Unavailable;
        }

        auto* const tick = static_cast<D2AiTickParam*>(request->aiTickParam);
        const auto classId = NativeFunctions.getClassId(request->monster);
        if (classId < 0) {
            RevivePolicyFailures.fetch_add(1U, std::memory_order_relaxed);
            return revive_v2::Result::Error;
        }

        const NativeThinkContext context{
            .game = request->game,
            .unit = request->monster,
            .target = tick->target,
            .owner = request->owner,
            .sessionGeneration = session,
            .thinkToken = NextThinkToken(),
            .monStatsId = static_cast<std::uint32_t>(classId),
            .targetDistance = tick->targetDistance,
            .ownerDistance = request->ownerDistance,
            .inCombat = tick->inCombat != 0,
        };
        std::string error;
        const auto execution = EvaluateRevivePolicyThink(
            *generation,
            context,
            {},
            error);
        const bool requested = execution.continuation
            == RevivePolicyContinuation::RequestNativeFollow;
        if (requested) {
            RevivePolicyNativeFollowRequests.fetch_add(
                1U,
                std::memory_order_relaxed);
            if (Settings.diagnostics
                    && FirstRevivePolicySession.exchange(
                        session,
                        std::memory_order_acq_rel) != session) {
                char message[512]{};
                std::snprintf(
                    message,
                    sizeof(message),
                    "ScriptedAI: session %llu issued its first native-follow request for a Revive (MonStatsId=%u, special=%d, ownerDistance=%d, policyRequests=%u, lua_us=%llu).",
                    static_cast<unsigned long long>(session),
                    context.monStatsId,
                    request->specialState,
                    request->ownerDistance,
                    execution.policyRequests,
                    static_cast<unsigned long long>(
                        execution.decision.luaMicroseconds));
                Context->LogInfo(message);
            }
            return revive_v2::Result::RequestNativeFollow;
        }
        RevivePolicyDelegates.fetch_add(1U, std::memory_order_relaxed);
        return revive_v2::Result::DelegateNative;
    } catch (...) {
        RevivePolicyFailures.fetch_add(1U, std::memory_order_relaxed);
        return revive_v2::Result::Error;
    }
}

const revive_v2::Interface RevivePolicyInterface{
    .structSize = sizeof(revive_v2::Interface),
    .abiVersion = revive_v2::AbiVersion,
    .magic = revive_v2::InterfaceMagic,
    .capabilities = revive_v2::CapabilityRequestNativeFollow,
    .reserved = 0U,
    .evaluate = EvaluateRevivePolicy,
};

[[nodiscard]] auto __cdecl EvaluateReviveTactics(
        const revive_v3::Context* request) noexcept -> revive_v3::Result {
    ReviveTacticalCalls.fetch_add(1U, std::memory_order_relaxed);
    try {
        if (request == nullptr || request->structSize < sizeof(*request)
                || request->abiVersion != revive_v3::AbiVersion
                || request->game == nullptr || request->monster == nullptr
                || request->owner == nullptr || request->monStats == nullptr) {
            ReviveTacticalFailures.fetch_add(1U, std::memory_order_relaxed);
            return revive_v3::Result::Error;
        }
        if (!Operational.load(std::memory_order_acquire)
                || !Settings.revive.enabled || Bridge == nullptr
                || (request->specialState != 0
                    && request->specialState != 7)) {
            ReviveTacticalDelegates.fetch_add(1U, std::memory_order_relaxed);
            return revive_v3::Result::DelegateNative;
        }

        const auto session = Bridge->DesiredSession();
        const auto generation = Bridge->ActiveFor(session);
        const auto authoritativeThread =
            AuthoritativeThread.load(std::memory_order_acquire);
        if (!generation || !generation->HasReviveScript()
                || CurrentThreadId() != authoritativeThread
                || !AttestResolverForSession(session)
                || NativeFunctions.getClassId == nullptr
                || NativeFunctions.getUnitId == nullptr
                || NativeFunctions.getUnitType == nullptr
                || NativeFunctions.isUnitDead == nullptr
                || NativeFunctions.getServerUnit == nullptr
                || NativeFunctions.distance == nullptr) {
            ReviveTacticalDelegates.fetch_add(1U, std::memory_order_relaxed);
            return revive_v3::Result::Unavailable;
        }

        const auto classId = NativeFunctions.getClassId(request->monster);
        const auto signedMonsterGuid =
            NativeFunctions.getUnitId(request->monster);
        if (classId < 0 || signedMonsterGuid == -1
                || NativeFunctions.getUnitType(request->monster)
                    != static_cast<std::int32_t>(MonsterUnitType)
                || NativeFunctions.isUnitDead(request->monster) != 0
                || NativeFunctions.isUnitDead(request->owner) != 0) {
            ReviveTacticalFailures.fetch_add(1U, std::memory_order_relaxed);
            return revive_v3::Result::Error;
        }
        const auto monsterGuid = static_cast<std::uint32_t>(
            signedMonsterGuid);
        if (request->target == nullptr) {
            EraseReviveTacticalMemory(monsterGuid);
            ReviveTacticalDelegates.fetch_add(1U, std::memory_order_relaxed);
            return revive_v3::Result::DelegateNative;
        }

        const auto ownerDistance = NativeFunctions.distance(
            request->monster,
            request->owner);
        if (ownerDistance < 0
                || ownerDistance > ReviveTacticalHardRadius) {
            EraseReviveTacticalMemory(monsterGuid);
            ReviveTacticalDelegates.fetch_add(1U, std::memory_order_relaxed);
            return revive_v3::Result::DelegateNative;
        }

        const auto nativeBank = ResolveNativeBank(
            request->monStats,
            static_cast<std::uint32_t>(classId));
        if (!nativeBank.found
                || ReadUnitMonStatsRecord(request->monster)
                    != nativeBank.monStats) {
            ReviveTacticalFailures.fetch_add(1U, std::memory_order_relaxed);
            return revive_v3::Result::Error;
        }

        const auto token = NextThinkToken();
        auto memory = LoadReviveTacticalMemory(
            session,
            monsterGuid,
            static_cast<std::uint32_t>(classId));
        memory.lastSeenToken = token;

        void* tacticalTarget{};
        std::int32_t targetType{-1};
        std::uint32_t targetGuid{InvalidUnitId};
        bool reusedLock{};
        if (memory.hasTargetLock && memory.targetType >= 0
                && memory.targetType
                    <= static_cast<std::int32_t>(MaximumUnitType)
                && memory.targetGuid != InvalidUnitId) {
            auto* const resolved = NativeFunctions.getServerUnit(
                request->game,
                memory.targetType,
                static_cast<std::int32_t>(memory.targetGuid));
            if (resolved != nullptr
                    && reinterpret_cast<std::uintptr_t>(resolved)
                        == memory.targetAddress
                    && resolved != request->monster
                    && resolved != request->owner
                    && NativeFunctions.isUnitDead(resolved) == 0) {
                const auto lockedOwnerDistance = NativeFunctions.distance(
                    resolved,
                    request->owner);
                if (lockedOwnerDistance >= 0
                        && lockedOwnerDistance
                            <= ReviveTacticalHardRadius) {
                    tacticalTarget = resolved;
                    targetType = memory.targetType;
                    targetGuid = memory.targetGuid;
                    reusedLock = true;
                }
            }
        }

        if (tacticalTarget == nullptr) {
            const auto currentType =
                NativeFunctions.getUnitType(request->target);
            const auto currentGuid =
                NativeFunctions.getUnitId(request->target);
            const auto currentOwnerDistance = NativeFunctions.distance(
                request->target,
                request->owner);
            if (request->target == request->monster
                    || request->target == request->owner
                    || currentType < 0
                    || currentType
                        > static_cast<std::int32_t>(MaximumUnitType)
                    || currentGuid == -1
                    || NativeFunctions.isUnitDead(request->target) != 0
                    || currentOwnerDistance < 0
                    || currentOwnerDistance
                        > ReviveTacticalHardRadius) {
                memory.hasTargetLock = false;
                StoreReviveTacticalMemory(memory);
                ReviveTacticalDelegates.fetch_add(
                    1U,
                    std::memory_order_relaxed);
                return revive_v3::Result::DelegateNative;
            }
            tacticalTarget = request->target;
            targetType = currentType;
            targetGuid = static_cast<std::uint32_t>(currentGuid);
            memory.targetType = targetType;
            memory.targetGuid = targetGuid;
            memory.targetAddress = reinterpret_cast<std::uintptr_t>(
                tacticalTarget);
            memory.hasTargetLock = true;
            ReviveTargetLocks.fetch_add(1U, std::memory_order_relaxed);
        } else {
            ReviveTargetLockReuses.fetch_add(1U, std::memory_order_relaxed);
        }

        const auto targetDistance = NativeFunctions.distance(
            request->monster,
            tacticalTarget);
        const auto targetOwnerDistance = NativeFunctions.distance(
            tacticalTarget,
            request->owner);
        if (targetDistance < 0 || targetOwnerDistance < 0
                || targetOwnerDistance > ReviveTacticalHardRadius) {
            memory.hasTargetLock = false;
            StoreReviveTacticalMemory(memory);
            ReviveTacticalDelegates.fetch_add(1U, std::memory_order_relaxed);
            return revive_v3::Result::DelegateNative;
        }

        const auto loadout = SelectReviveTacticalLoadout(
            nativeBank.monStats,
            nativeBank.skillRowCount,
            memory.nextSkillSlot);
        if (loadout.profile == TacticalProfile::None) {
            ReviveTacticalDelegates.fetch_add(1U, std::memory_order_relaxed);
            return revive_v3::Result::DelegateNative;
        }

        StoreReviveTacticalMemory(memory);
        D2NativeActionAdapter adapter{
            NativeFunctions,
            session,
            authoritativeThread,
            nativeBank.skillRowCount,
            nativeBank.monStats,
            CurrentThreadId,
        };
        const NativeThinkContext context{
            .game = request->game,
            .unit = request->monster,
            .target = tacticalTarget,
            .owner = request->owner,
            .sessionGeneration = session,
            .thinkToken = token,
            .monStatsId = static_cast<std::uint32_t>(classId),
            .targetDistance = targetDistance,
            .ownerDistance = ownerDistance,
            .targetOwnerDistance = targetOwnerDistance,
            .tacticalProfile = loadout.profile,
            .hasLastAction = memory.hasLastAction,
            .lastAction = memory.lastAction,
            .hasPreferredSkill = loadout.hasPreferredSkill,
            .preferredSkill = loadout.preferredSkill,
            .inCombat = request->inCombat != 0,
        };
        std::string error;
        const auto execution = ExecuteReviveTacticalThink(
            *generation,
            context,
            adapter,
            {},
            error);
        if (execution.continuation
                != ReviveTacticalContinuation::Handled) {
            ReviveTacticalDelegates.fetch_add(1U, std::memory_order_relaxed);
            return revive_v3::Result::DelegateNative;
        }

        const auto action = execution.decision.disposition
                == ThinkDisposition::Action
            ? execution.decision.action
            : execution.attemptedAction;
        memory.lastAction = action.kind;
        memory.hasLastAction = true;
        if (action.kind == ActionKind::CastOnTarget
                && loadout.hasPreferredSkill) {
            memory.nextSkillSlot = static_cast<std::uint8_t>(
                (loadout.preferredSlot + 1U) % MonStatsSkillSlotCount);
            ReviveSpellRotations.fetch_add(1U, std::memory_order_relaxed);
        }
        StoreReviveTacticalMemory(memory);
        ReviveTacticalHandled.fetch_add(1U, std::memory_order_relaxed);
        if (Settings.diagnostics
                && FirstReviveTacticalSession.exchange(
                    session,
                    std::memory_order_acq_rel) != session) {
            char message[640]{};
            std::snprintf(
                message,
                sizeof(message),
                "ScriptedAI: session %llu handled its first Revive tactical action (MonStatsId=%u, profile=%s, action=%s, target=%d:%u, lock=%s, ownerDistance=%d, targetOwnerDistance=%d, lua_us=%llu).",
                static_cast<unsigned long long>(session),
                context.monStatsId,
                TacticalProfileName(context.tacticalProfile),
                ActionName(action.kind),
                targetType,
                targetGuid,
                reusedLock ? "reused" : "new",
                ownerDistance,
                targetOwnerDistance,
                static_cast<unsigned long long>(
                    execution.decision.luaMicroseconds));
            Context->LogInfo(message);
        }
        return revive_v3::Result::Handled;
    } catch (...) {
        ReviveTacticalFailures.fetch_add(1U, std::memory_order_relaxed);
        return revive_v3::Result::Error;
    }
}

const revive_v3::Interface ReviveTacticalInterface{
    .structSize = sizeof(revive_v3::Interface),
    .abiVersion = revive_v3::AbiVersion,
    .magic = revive_v3::InterfaceMagic,
    .capabilities = revive_v3::CapabilityTacticalActions,
    .reserved = 0U,
    .evaluate = EvaluateReviveTactics,
};

[[nodiscard]] auto InstallResolverHook() -> bool {
    std::vector<std::uint8_t> expected;
    std::string error;
    if (!DecodeResolverExpected(expected, error)
            || !Context->InstallInlineHook(
                ResolverHookRva,
                expected.data(),
                static_cast<std::uint32_t>(expected.size()),
                HookAiTableResolver,
                &OriginalResolver)
            || OriginalResolver == nullptr) {
        Context->LogError(
            "ScriptedAI: managed resolver hook installation failed."
        );
        return false;
    }
    if (!ValidateResolverOwnership(true)) return false;
    AttestedSession.store(0U, std::memory_order_release);
    Operational.store(true, std::memory_order_release);
    return true;
}

void CleanupBridgeRegistration() noexcept {
    Operational.store(false, std::memory_order_release);
    AttestedSession.store(0U, std::memory_order_release);
    FirstRoutedThinkSession.store(0U, std::memory_order_release);
    FirstRevivePolicySession.store(0U, std::memory_order_release);
    AuthoritativeThread.store(0U, std::memory_order_release);
    ClearReviveTacticalMemories();
    NativeBanksReady = false;
    NativeBanks = {};
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
    DataTables = nullptr;
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
        std::string_view bytes,
        D2RL::Resources::RegistrationHandle& handle) noexcept -> bool {
    const D2RL::Resources::ResourceRegistration registration{
        .structSize = D2RL::Resources::ResourceRegistrationSize,
        .flags = 0U,
        .path = path,
        .bytes = bytes.data(),
        .byteCount = bytes.size(),
    };
    return Resources->registerResource(Context, &registration, &handle)
        == D2RL::Resources::Result::Success;
}

[[nodiscard]] auto PrepareAiScriptResource(
        const std::filesystem::path& supportRoot,
        const std::filesystem::path& fileName,
        std::string& text,
        bool& external,
        std::string& error) -> bool {
    external = false;
    const auto status = ReadOptionalBoundedTextFile(
        supportRoot,
        fileName,
        MaximumAiScriptTableBytes,
        text,
        error);
    if (status == OptionalTextFileStatus::Error) return false;
    if (status == OptionalTextFileStatus::Absent) {
        text.assign(
            DefaultAiScriptTable,
            sizeof(DefaultAiScriptTable) - 1U);
        error.clear();
        return true;
    }
    external = true;
    if (text.empty() || text.back() != '\n'
            || !std::all_of(
                text.begin(),
                text.end(),
                [](unsigned char value) {
                    return value == '\t' || value == '\r' || value == '\n'
                        || (value >= 0x20U && value <= 0x7eU);
                })) {
        error = "external AIScript text must be printable ASCII TSV with a final line feed";
        return false;
    }
    constexpr std::string_view headerLf{
        "MonStatsId\tScript\tFallbackAi\tTargetProfile\tEnabled\n"};
    constexpr std::string_view headerCrlf{
        "MonStatsId\tScript\tFallbackAi\tTargetProfile\tEnabled\r\n"};
    if (text == headerLf || text == headerCrlf) {
        text.assign(
            DefaultAiScriptTable,
            sizeof(DefaultAiScriptTable) - 1U);
    }
    error.clear();
    return true;
}

[[nodiscard]] auto RegisterBridgeResourcesAndTable() noexcept -> bool {
    const auto supportRoot = std::filesystem::path(LoadedConfigPath)
        .parent_path() / AiScriptSupportDirectory;
    std::string baseText;
    std::string rotwText;
    std::string error;
    bool baseExternal{};
    bool rotwExternal{};
    if (!PrepareAiScriptResource(
            supportRoot,
            BaseAiScriptFileName,
            baseText,
            baseExternal,
            error)
            || !PrepareAiScriptResource(
                supportRoot,
                RotwAiScriptFileName,
                rotwText,
                rotwExternal,
                error)) {
        const auto message = std::string(
            "ScriptedAI: external AIScript resource refused (")
            + error + ").";
        Context->LogError(message.c_str());
        return false;
    }
    if (!RegisterResource(
            BaseAiScriptResource,
            baseText,
            BaseResourceHandle)
            || !RegisterResource(
                RotwAiScriptResource,
                rotwText,
                RotwResourceHandle)) {
        Context->LogError(
            "ScriptedAI: selected AIScript resources could not be registered."
        );
        return false;
    }
    if (Settings.diagnostics) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "ScriptedAI: AIScript resources selected (Base=%s, RotW=%s, support=%s).",
            baseExternal ? "external" : "embedded-empty",
            rotwExternal ? "external" : "embedded-empty",
            supportRoot.string().c_str());
        Context->LogInfo(message);
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
        if (!CaptureNativeTableViews(event->revision, error)
                || !CopyTableBank(
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
            {.revive = Settings.revive.enabled
                ? Settings.revive.script
                : std::string{}},
            Settings.limits,
            ReadScriptSource,
            error);
        if (!candidate || !Bridge->PublishPrepared(candidate, error)) {
            const auto message = std::string(
                "ScriptedAI: rejected Scripted AI source transaction (")
                + error + "); the prior generation remains published.";
            Context->LogError(message.c_str());
            return;
        }
        if (Settings.diagnostics) {
            char message[256]{};
            std::snprintf(
                message,
                sizeof(message),
                "ScriptedAI: staged revision %llu (%zu unique scripts); publication awaits authoritative compilation.",
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
        AttestedSession.store(0U, std::memory_order_release);
        FirstRoutedThinkSession.store(0U, std::memory_order_release);
        FirstRevivePolicySession.store(0U, std::memory_order_release);
        FirstReviveTacticalSession.store(0U, std::memory_order_release);
        ClearReviveTacticalMemories();
        Bridge->AnnounceGameJoined(event->sessionGeneration);
        (void)QueueAuthoritativeReconcile("GameJoined");
    } else if (event->kind == D2RL::Lifecycle::GameplayEventKind::GameLeft) {
        AttestedSession.store(0U, std::memory_order_release);
        FirstRoutedThinkSession.store(0U, std::memory_order_release);
        FirstRevivePolicySession.store(0U, std::memory_order_release);
        FirstReviveTacticalSession.store(0U, std::memory_order_release);
        ClearReviveTacticalMemories();
        AuthoritativeThread.store(0U, std::memory_order_release);
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
        const auto session = Bridge->DesiredSession();
        const auto active = Bridge->ActiveFor(session);
        AuthoritativeThread.store(
            active ? CurrentThreadId() : 0U,
            std::memory_order_release);
        if (Settings.diagnostics) {
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
            || !ValidateResolverOwnership(false)) {
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

    NativeFunctions = ResolveD2NativeFunctions(context->exeBase);
    if (NativeFunctions.getClassId == nullptr
            || NativeFunctions.getUnitId == nullptr
            || NativeFunctions.getUnitType == nullptr
            || NativeFunctions.isUnitDead == nullptr
            || NativeFunctions.getServerUnit == nullptr
            || NativeFunctions.distance == nullptr
            || NativeFunctions.idle == nullptr
            || NativeFunctions.wander == nullptr
            || NativeFunctions.attack == nullptr
            || NativeFunctions.chase == nullptr
            || NativeFunctions.retreat == nullptr
            || NativeFunctions.cast == nullptr) {
        context->LogError(
            "ScriptedAI: native action adapter resolution failed closed."
        );
        ResetState();
        return false;
    }

    Bridge = std::make_unique<BridgeCoordinator>(Settings.limits);
    if (!RegisterBridgeResourcesAndTable()
            || !RegisterLifecycleListeners()
            || !InstallResolverHook()) {
        ResetState();
        return false;
    }

    char message[512]{};
    std::snprintf(
        message,
        sizeof(message),
        "RuffnecKk Scripted AI %s installed its exclusively owned resolver bridge from %s for reported build %s; Classic/LoD/RotW MonStats views route configured monsters and the optional Revive tactical domain to bounded Lua and governed D2R actions.",
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

extern "C" __declspec(dllexport) auto __cdecl
RuffnecKkScriptedAIQueryRevivePolicyV2(
        std::uint32_t requestedVersion,
        std::uint32_t callerInterfaceSize) noexcept
        -> const ruffneckk::scripted_ai::revive_v2::Interface* {
    if (requestedVersion
            != ruffneckk::scripted_ai::revive_v2::AbiVersion
            || callerInterfaceSize
                < sizeof(ruffneckk::scripted_ai::revive_v2::Interface)
            || !Operational.load(std::memory_order_acquire)
            || !Settings.revive.enabled) {
        return nullptr;
    }
    return &RevivePolicyInterface;
}

extern "C" __declspec(dllexport) auto __cdecl
RuffnecKkScriptedAIQueryReviveTacticsV3(
        std::uint32_t requestedVersion,
        std::uint32_t callerInterfaceSize) noexcept
        -> const ruffneckk::scripted_ai::revive_v3::Interface* {
    if (requestedVersion
            != ruffneckk::scripted_ai::revive_v3::AbiVersion
            || callerInterfaceSize
                < sizeof(ruffneckk::scripted_ai::revive_v3::Interface)
            || !Operational.load(std::memory_order_acquire)
            || !Settings.revive.enabled) {
        return nullptr;
    }
    return &ReviveTacticalInterface;
}
