#define NOMINMAX

#include <D2RLPlugin/api.h>

#include "cast_triggers_policy.hpp"

#include <Windows.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace RuffnecKk::CastTriggers {
namespace {

using namespace ruffneckk::cast_triggers;

constexpr wchar_t ConfigFileName[] = L"ruffneckk-cast-triggers.toml";
constexpr std::int32_t DoActiveEvent = 4;
constexpr std::size_t GameDataContextOffset = 0x106;
constexpr std::size_t SkillFlagsOffset = 0x24;
constexpr std::size_t SkillAnimationOffset = 0x30;
constexpr std::size_t SkillSequenceTransitionOffset = 0x32;

constexpr std::uintptr_t SkillHandlerRva = 0x43ACB0;
constexpr std::uintptr_t DispatchUnitStatEventRva = 0x44D570;
constexpr std::uintptr_t GetTargetUnitRva = 0x48FE20;
constexpr std::uintptr_t GetUnitTypeRva = 0x34B9D0;
constexpr std::uintptr_t GetSkillsRecordRva = 0x097790;
constexpr std::uintptr_t CastItemSkillOnTargetRva = 0x5896E0;
constexpr std::uintptr_t CastItemSkillAtPositionRva = 0x589820;

constexpr auto SkillHandlerExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x44,0x89,0x4C,
    0x24,0x20,0x44,0x89,0x44,0x24,0x18,0x55,
    0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,
    0x41,0x57,0x48,0x81,0xEC,0x80,0x00,0x00,
    0x00,
});
constexpr auto DispatchUnitStatEventExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x55,0x56,0x57,0x41,0x56,0x48,
    0x83,0xEC,0x60,0x48,0x8B,0x05,0x46,0xDD,
    0x57,0x02,0x48,0x33,0xC4,0x48,0x89,0x44,
    0x24,0x58,
});
constexpr auto GetTargetUnitExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,
    0xDA,0xE8,0x22,0x1D,0x00,0x00,0x48,0x8B,
    0xCB,0xE8,0x4A,0xB0,0xEB,0xFF,0x48,0x8B,
    0xC8,0xE8,0x02,0x1C,0xEB,0xFF,
});
constexpr auto GetUnitTypeExpected = std::to_array<std::uint8_t>({
    0x48,0x83,0xEC,0x28,0x48,0x85,0xC9,0x75,
    0x1D,0x88,0x4C,0x24,0x30,0x48,0x8D,0x4C,
    0x24,0x30,0xE8,0x39,0x9E,0xFF,0xFF,0x84,
    0xC0,0x74,0x01,0xCC,0xB8,0x06,0x00,0x00,
});
constexpr auto GetSkillsRecordExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x74,
    0x24,0x20,0x57,0x48,0x83,0xEC,0x30,0x48,
    0x63,0xF2,0xE8,0xE9,0x92,0x26,0x00,0x48,
    0x8B,0xF8,0x48,0x8B,0xDE,0x85,0xF6,0x78,
    0x09,0x48,0x3B,0x98,0xB8,0x11,0x00,0x00,
});
constexpr auto CastItemSkillOnTargetExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x6C,
    0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57,
    0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x70,
    0x49,0x8B,0xF1,0x41,0x8B,0xE8,0x44,0x8B,
    0xF2,0x48,0x8B,0xD9,0x48,0x85,0xC9,0x0F,
    0x84,0xF0,0x00,0x00,0x00,
});
constexpr auto CastItemSkillAtPositionExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x6C,
    0x24,0x18,0x56,0x57,0x41,0x54,0x41,0x56,
    0x41,0x57,0x48,0x83,0xEC,0x70,
    0x41,0x8B,0xE9,0x45,0x8B,0xF0,0x44,0x8B,
    0xFA,0x48,0x8B,0xD9,0x48,0x85,0xC9,0x0F,
    0x84,0xCA,0x00,0x00,0x00,
});

using SkillHandlerFn = std::int32_t(__fastcall*)(
    void*, void*, std::int32_t, std::int32_t,
    std::int32_t, std::int32_t, std::int32_t) noexcept;
using DispatchUnitStatEventFn = std::int32_t(__fastcall*)(
    void*, std::int32_t, void*, void*, void*) noexcept;
using GetTargetUnitFn = void*(__fastcall*)(void*, void*) noexcept;
using GetUnitTypeFn = std::int32_t(__fastcall*)(void*) noexcept;
using GetSkillsRecordFn = const std::uint8_t*(__fastcall*)(
    std::uint8_t, std::int32_t) noexcept;
using CastItemSkillOnTargetFn = std::int32_t(__fastcall*)(
    void*, std::int32_t, std::int32_t, void*, std::int32_t) noexcept;
using CastItemSkillAtPositionFn = std::int32_t(__fastcall*)(
    void*, std::int32_t, std::int32_t,
    std::int32_t, std::int32_t, std::int32_t) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
std::string LoadedConfigPath{"built-in defaults"};
std::atomic_bool Operational{};
SkillHandlerFn OriginalSkillHandler{};
CastItemSkillOnTargetFn OriginalCastItemSkillOnTarget{};
CastItemSkillAtPositionFn OriginalCastItemSkillAtPosition{};
DispatchUnitStatEventFn DispatchUnitStatEvent{};
GetTargetUnitFn GetTargetUnit{};
GetUnitTypeFn GetUnitType{};
GetSkillsRecordFn GetSkillsRecord{};

thread_local std::uint32_t CastDispatchDepth{};
thread_local std::int32_t CastDispatchSourceLevel{};

std::atomic_uint64_t ManualCastsObserved{};
std::atomic_uint64_t EligibleCasts{};
std::atomic_uint64_t EventDispatches{};
std::atomic_uint64_t FixedLevelProcs{};
std::atomic_uint64_t SameLevelProcs{};

void LogDiagnostic(const char* message) noexcept {
    if (Settings.diagnostics && Context && message) {
        Context->LogInfo(message);
    }
}

template <typename Fn>
Fn At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<Fn>(Base + rva);
}

template <typename Value>
Value ReadRecordValue(
        const std::uint8_t* record,
        std::size_t offset) noexcept {
    Value result{};
    if (record) std::memcpy(&result, record + offset, sizeof(result));
    return result;
}

class DispatchScope final {
public:
    explicit DispatchScope(std::int32_t sourceLevel) noexcept
        : previousDepth_(CastDispatchDepth),
          previousLevel_(CastDispatchSourceLevel) {
        ++CastDispatchDepth;
        CastDispatchSourceLevel = sourceLevel;
    }

    ~DispatchScope() {
        CastDispatchDepth = previousDepth_;
        CastDispatchSourceLevel = previousLevel_;
    }

    DispatchScope(const DispatchScope&) = delete;
    auto operator=(const DispatchScope&) -> DispatchScope& = delete;

private:
    std::uint32_t previousDepth_{};
    std::int32_t previousLevel_{};
};

class SuspendDispatchScope final {
public:
    SuspendDispatchScope() noexcept
        : previousDepth_(CastDispatchDepth),
          previousLevel_(CastDispatchSourceLevel) {
        CastDispatchDepth = 0;
        CastDispatchSourceLevel = 0;
    }

    ~SuspendDispatchScope() {
        CastDispatchDepth = previousDepth_;
        CastDispatchSourceLevel = previousLevel_;
    }

    SuspendDispatchScope(const SuspendDispatchScope&) = delete;
    auto operator=(const SuspendDispatchScope&)
        -> SuspendDispatchScope& = delete;

private:
    std::uint32_t previousDepth_{};
    std::int32_t previousLevel_{};
};

std::vector<std::filesystem::path> ConfigCandidates() {
    std::vector<std::filesystem::path> directories;
    if (Context && Context->activeMod && Context->activeMod[0] != '\0'
            && Context->modSupportDirectory
            && Context->modSupportDirectory[0] != L'\0') {
        directories.emplace_back(
            std::filesystem::path(Context->modSupportDirectory) / L"config");
    }
    if (Context && Context->pluginConfigPath
            && Context->pluginConfigPath[0] != L'\0') {
        directories.emplace_back(
            std::filesystem::path(Context->pluginConfigPath).parent_path());
    }
    std::error_code error;
    const auto current = std::filesystem::current_path(error);
    if (!error) directories.emplace_back(current / L"d2rloader" / L"config");
    return BuildConfigCandidates(directories, ConfigFileName);
}

bool LoadConfig() noexcept {
    Settings = {};
    LoadedConfigPath = "built-in defaults";
    for (const auto& path : ConfigCandidates()) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) continue;
        try {
            std::ifstream input(path, std::ios::binary);
            if (!input.is_open()) {
                throw std::runtime_error("configuration file cannot be opened");
            }
            const std::string text{
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>()};
            Config parsed{};
            std::string parseError;
            if (!ParseToml(text, parsed, parseError)) {
                throw std::runtime_error(parseError);
            }
            Settings = std::move(parsed);
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            const auto message = std::string("CastTriggers: invalid ")
                + path.string() + " (" + exception.what() + ").";
            Context->LogError(message.c_str());
            return false;
        }
    }
    return true;
}

template <std::size_t Size>
bool Check(
        std::uintptr_t rva,
        const std::array<std::uint8_t, Size>& expected,
        const char* label) noexcept {
    if (std::memcmp(Base + rva, expected.data(), expected.size()) == 0) {
        return true;
    }
    const auto message = std::string("CastTriggers: ") + label
        + " signature mismatch; plugin refused.";
    Context->LogError(message.c_str());
    return false;
}

bool ValidateRuntime() noexcept {
    return Check(SkillHandlerRva, SkillHandlerExpected, "skill handler")
        && Check(
            DispatchUnitStatEventRva,
            DispatchUnitStatEventExpected,
            "unit-stat event dispatcher")
        && Check(GetTargetUnitRva, GetTargetUnitExpected, "target resolver")
        && Check(GetUnitTypeRva, GetUnitTypeExpected, "unit type helper")
        && Check(
            GetSkillsRecordRva,
            GetSkillsRecordExpected,
            "SkillsTxt lookup")
        && Check(
            CastItemSkillOnTargetRva,
            CastItemSkillOnTargetExpected,
            "target item-skill caster")
        && Check(
            CastItemSkillAtPositionRva,
            CastItemSkillAtPositionExpected,
            "position item-skill caster");
}

void ResolveNativeFunctions() noexcept {
    DispatchUnitStatEvent = At<DispatchUnitStatEventFn>(
        DispatchUnitStatEventRva);
    GetTargetUnit = At<GetTargetUnitFn>(GetTargetUnitRva);
    GetUnitType = At<GetUnitTypeFn>(GetUnitTypeRva);
    GetSkillsRecord = At<GetSkillsRecordFn>(GetSkillsRecordRva);
}

bool IsEligibleSourceRecord(
        void* game,
        std::int32_t skillId) noexcept {
    if (!game || skillId < MinimumSkillId
            || skillId > MaximumSkillId
            || !IsConfiguredSourceSkill(Settings, skillId)) {
        return false;
    }
    const auto dataContext = ReadRecordValue<std::uint8_t>(
        static_cast<const std::uint8_t*>(game),
        GameDataContextOffset);
    const auto* record = GetSkillsRecord(dataContext, skillId);
    if (!record) return false;
    const auto flags = ReadRecordValue<std::uint64_t>(
        record,
        SkillFlagsOffset);
    const auto animation = ReadRecordValue<std::uint8_t>(
        record,
        SkillAnimationOffset);
    const auto sequenceTransition = ReadRecordValue<std::uint8_t>(
        record,
        SkillSequenceTransitionOffset);
    return IsEligibleSkillRecord(animation, sequenceTransition, flags);
}

std::int32_t __fastcall HookCastItemSkillOnTarget(
        void* caster,
        std::int32_t skillId,
        std::int32_t skillLevel,
        void* target,
        std::int32_t flag) noexcept {
    const bool inCastDispatch = Operational.load(std::memory_order_acquire)
        && CastDispatchDepth != 0;
    const bool sameLevel = inCastDispatch
        && skillLevel == SameLevelMarker
        && CastDispatchSourceLevel > 0;
    const auto requestedLevel = skillLevel;
    if (sameLevel) skillLevel = CastDispatchSourceLevel;

    const SuspendDispatchScope suspend;
    const auto result = OriginalCastItemSkillOnTarget(
        caster, skillId, skillLevel, target, flag);
    if (inCastDispatch && result != 0) {
        if (sameLevel) {
            SameLevelProcs.fetch_add(1, std::memory_order_relaxed);
        } else {
            FixedLevelProcs.fetch_add(1, std::memory_order_relaxed);
        }
    }
    if (inCastDispatch && Settings.diagnostics) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "CastTriggers diagnostic: target proc skill=%d requested-level=%d effective-level=%d result=%d.",
            skillId,
            requestedLevel,
            skillLevel,
            result);
        LogDiagnostic(message);
    }
    return result;
}

std::int32_t __fastcall HookCastItemSkillAtPosition(
        void* caster,
        std::int32_t skillId,
        std::int32_t skillLevel,
        std::int32_t x,
        std::int32_t y,
        std::int32_t flag) noexcept {
    const bool inCastDispatch = Operational.load(std::memory_order_acquire)
        && CastDispatchDepth != 0;
    const bool sameLevel = inCastDispatch
        && skillLevel == SameLevelMarker
        && CastDispatchSourceLevel > 0;
    const auto requestedLevel = skillLevel;
    if (sameLevel) skillLevel = CastDispatchSourceLevel;

    const SuspendDispatchScope suspend;
    const auto result = OriginalCastItemSkillAtPosition(
        caster, skillId, skillLevel, x, y, flag);
    if (inCastDispatch && result != 0) {
        if (sameLevel) {
            SameLevelProcs.fetch_add(1, std::memory_order_relaxed);
        } else {
            FixedLevelProcs.fetch_add(1, std::memory_order_relaxed);
        }
    }
    if (inCastDispatch && Settings.diagnostics) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "CastTriggers diagnostic: position proc skill=%d requested-level=%d effective-level=%d result=%d.",
            skillId,
            requestedLevel,
            skillLevel,
            result);
        LogDiagnostic(message);
    }
    return result;
}

std::int32_t __fastcall HookSkillHandler(
        void* game,
        void* unit,
        std::int32_t skillId,
        std::int32_t skillLevel,
        std::int32_t consumeResources,
        std::int32_t itemCast,
        std::int32_t itemEffect) noexcept {
    const auto nativeResult = OriginalSkillHandler(
        game,
        unit,
        skillId,
        skillLevel,
        consumeResources,
        itemCast,
        itemEffect);
    if (!Operational.load(std::memory_order_acquire)
            || CastDispatchDepth != 0) {
        return nativeResult;
    }

    ManualCastsObserved.fetch_add(1, std::memory_order_relaxed);
    const auto unitType = unit ? GetUnitType(unit) : 6;
    if (!IsManualPlayerCast(
            nativeResult,
            consumeResources,
            itemCast,
            itemEffect,
            unitType)
            || skillLevel <= 0
            || !IsEligibleSourceRecord(game, skillId)) {
        return nativeResult;
    }

    EligibleCasts.fetch_add(1, std::memory_order_relaxed);
    if (Settings.diagnostics) {
        char message[192]{};
        std::snprintf(
            message,
            sizeof(message),
            "CastTriggers diagnostic: eligible source skill=%d effective-level=%d.",
            skillId,
            skillLevel);
        LogDiagnostic(message);
    }
    void* const target = GetTargetUnit(game, unit);
    const DispatchScope dispatch(skillLevel);
    DispatchUnitStatEvent(game, DoActiveEvent, unit, target, nullptr);
    EventDispatches.fetch_add(1, std::memory_order_relaxed);
    return nativeResult;
}

bool InstallHooks() noexcept {
    if (!Context->InstallInlineHook(
            CastItemSkillOnTargetRva,
            CastItemSkillOnTargetExpected.data(),
            static_cast<std::uint32_t>(
                CastItemSkillOnTargetExpected.size()),
            HookCastItemSkillOnTarget,
            &OriginalCastItemSkillOnTarget)) {
        Context->LogError(
            "CastTriggers: target item-skill caster hook is already owned or unavailable.");
        return false;
    }
    if (!Context->InstallInlineHook(
            CastItemSkillAtPositionRva,
            CastItemSkillAtPositionExpected.data(),
            static_cast<std::uint32_t>(
                CastItemSkillAtPositionExpected.size()),
            HookCastItemSkillAtPosition,
            &OriginalCastItemSkillAtPosition)) {
        Context->LogError(
            "CastTriggers: position item-skill caster hook is already owned or unavailable.");
        return false;
    }

    if (!Context->InstallInlineHook(
            SkillHandlerRva,
            SkillHandlerExpected.data(),
            static_cast<std::uint32_t>(SkillHandlerExpected.size()),
            HookSkillHandler,
            &OriginalSkillHandler)) {
        Context->LogError(
            "CastTriggers: central skill handler hook is already owned or unavailable.");
        return false;
    }
    return true;
}

auto Status(
        D2R::Game::Client*,
        const D2RL::ConsoleCommandContext* command,
        void*) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) {
        return D2RL::ConsoleCommandResult::Failed;
    }
    char message[640]{};
    std::snprintf(
        message,
        sizeof(message),
        "Cast Triggers 0.1.0: %s; observed=%llu; eligible=%llu; dispatches=%llu; fixed procs=%llu; same-level procs=%llu; config=%s.",
        Operational.load(std::memory_order_acquire) ? "active" : "disabled",
        static_cast<unsigned long long>(
            ManualCastsObserved.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            EligibleCasts.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            EventDispatches.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            FixedLevelProcs.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            SameLevelProcs.load(std::memory_order_relaxed)),
        LoadedConfigPath.c_str());
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}

void ResetState() noexcept {
    Operational.store(false, std::memory_order_release);
    CastDispatchDepth = 0;
    CastDispatchSourceLevel = 0;
    ManualCastsObserved.store(0, std::memory_order_relaxed);
    EligibleCasts.store(0, std::memory_order_relaxed);
    EventDispatches.store(0, std::memory_order_relaxed);
    FixedLevelProcs.store(0, std::memory_order_relaxed);
    SameLevelProcs.store(0, std::memory_order_relaxed);
}

} // namespace
} // namespace RuffnecKk::CastTriggers

namespace {

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "ruffneckk-cast-triggers",
    .name = "Cast Triggers",
    .version = "0.1.0",
    .author = "RuffnecKk",
    .description = "Triggers item skills when players cast spells.",
    .flags = D2RL::PluginFlags::Server | D2RL::PluginFlags::NativeHooks,
};

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept
        -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    using namespace RuffnecKk::CastTriggers;
    Context = context;
    Base = context ? reinterpret_cast<std::uint8_t*>(context->exeBase) : nullptr;
    ResetState();
    if (!Context || !Base) return false;

    const auto* runtimeBuild = D2RL::GetBuildName(context);
    if (!runtimeBuild || std::strcmp(runtimeBuild, "93847") != 0) {
        context->LogError(
            "CastTriggers: only D2R 3.3 build 93847 is supported.");
        return false;
    }
    if (!LoadConfig()) return false;
    if (!Settings.enabled) {
        const auto message = std::string(
            "Cast Triggers 0.1.0 by RuffnecKk loaded disabled; config=")
            + LoadedConfigPath + ".";
        context->LogInfo(message.c_str());
        return true;
    }
    if (!ValidateRuntime()) return false;
    ResolveNativeFunctions();
    if (!InstallHooks()) return false;

    Operational.store(true, std::memory_order_release);
    if (!context->RegisterConsoleCommand(
            "cast-triggers",
            Status,
            "Show Cast Triggers status and counters.")) {
        context->LogWarn(
            "CastTriggers: status command could not be registered.");
    }
    const auto message = std::string(
        "Cast Triggers 0.1.0 by RuffnecKk active; config=")
        + LoadedConfigPath + ".";
    context->LogInfo(message.c_str());
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    RuffnecKk::CastTriggers::Operational.store(
        false,
        std::memory_order_release);
}
