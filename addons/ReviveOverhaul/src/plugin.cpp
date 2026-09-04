#define NOMINMAX
#include <D2RLPlugin/api.h>

#include "revive_overhaul_policy.hpp"
#include "scripted_ai_revive_abi.hpp"

#include <Windows.h>
#include <intrin.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace ruffneckk::revive_overhaul;

constexpr wchar_t ConfigFileName[] = L"ruffneckk-revive-overhaul.toml";
constexpr wchar_t LegacyConfigFileName[] = L"ReviveOverhaul.toml";
constexpr char Version[] = "2.3.0";

constexpr char DefaultConfig[] = R"toml(# Revive Overhaul
# Improves Revive following and safely expands eligible high-rank corpses.

# Master switch. false loads the plugin without installing native hooks.
enabled = true

[ai]
# Enables the Revive-only owner-following improvements.
enabled = true

# Prevents Revives from scattering when they are already beside their owner.
disable_owner_scatter = true

# Forces native catch-up behavior beyond this owner distance.
# Accepted range: 2 to 20.
catch_up_distance = 8

# Native follow distance used after catch-up. It must remain lower than
# catch_up_distance. Accepted range: 1 to 19.
follow_distance = 4

# Native catch-up velocity bonus. Accepted range: 0 to 255.
velocity_bonus = 80

[revive]
# Allows eligible Champions, Uniques and SuperUniques. Native boss, scripted,
# SwitchAI, corpse-selection, Revive and corpse-consumption checks remain active.
allow_high_rank_monsters = true

# Reactivates the exact aura skill selected by Aura Enchanted after a successful
# Revive through the complete native assignment path. No aura is rerolled.
preserve_native_auras = true

[diagnostics]
# Adds detailed counters to the revive-overhaul console command.
enabled = false
)toml";

constexpr std::uintptr_t AiFunction03Rva = 0x4A3A20;
constexpr std::uintptr_t SetVelocityRva = 0x4A7270;
constexpr std::uintptr_t DistanceRva = 0x596720;
constexpr std::uintptr_t WalkToOwnerRva = 0x4A8090;
constexpr std::uintptr_t DistanceReturnRva = 0x4A3C0A;
constexpr std::uintptr_t SetVelocityReturnRva = 0x4A3C4F;
constexpr std::uintptr_t WalkToOwnerReturnRva = 0x4A3C66;
constexpr std::uintptr_t StatesCheckStateRva = 0x3351B0;
constexpr std::uintptr_t ReviveStateMarkerWitnessRva = 0x55EB48;
constexpr std::uintptr_t AiSpecialStateDispatchReadRva = 0x4A2BC8;
constexpr std::uintptr_t GetMinionOwnerRva = 0x4A53C0;

constexpr std::uintptr_t ValidateReviveTargetRva = 0x55A510;
constexpr std::uintptr_t SrvDo58ReviveRva = 0x55E7E0;
constexpr std::uintptr_t ClientReviveSelectorRva = 0x096600;
constexpr std::uintptr_t ClientReviveAiGateRva = 0x096635;
constexpr std::uintptr_t ClientReviveAiGateCallRva = 0x096648;
constexpr std::uintptr_t GetUnitTypeRva = 0x34B9D0;
constexpr std::uintptr_t GetMonStats2Rva = 0x0975B0;
constexpr std::uintptr_t IsUnitDeadRva = 0x34C2C0;
constexpr std::uintptr_t IsCorpseConsumableRva = 0x33A9A0;
constexpr std::uintptr_t CanUnitSwitchAiRva = 0x34C730;
constexpr std::uintptr_t CheckMonsterTypeFlagRva = 0x38E870;
constexpr std::uintptr_t GetMonsterUModsRva = 0x38E310;
constexpr std::uintptr_t GetRightSkillRva = 0x34B400;
constexpr std::uintptr_t AssignSkillRva = 0x438A70;

constexpr std::size_t GameDataContextOffset = 0x106;
constexpr std::size_t UnitClassIdOffset = 0x04;
constexpr std::size_t MonStats2FlagsOffset = 0x04;
constexpr std::uint8_t MonStats2CorpseSelMask = 0x80;
constexpr std::uint8_t MonStats2ReviveMask = 0x01;
constexpr std::size_t SkillRecordOffset = 0x00;
constexpr std::size_t SkillOwnerGuidOffset = 0x4C;
constexpr std::uint32_t ReviveSkillId = 95;
constexpr std::uint32_t SkillsRecordSize = 0x2EC;
constexpr std::size_t SkillsSelectProcOffset = 0x35;
constexpr std::size_t SkillsSrvStFuncOffset = 0x4C;
constexpr std::size_t SkillsSrvDoFuncOffset = 0x4E;
constexpr std::size_t SkillsCltStFuncOffset = 0x148;
constexpr std::size_t SkillsCltDoFuncOffset = 0x14A;
constexpr std::uint8_t NativeReviveSelectProc = 3;
constexpr std::int16_t NativeReviveSrvStFunc = 21;
constexpr std::int16_t NativeReviveSrvDoFunc = 58;
constexpr std::int16_t NativeReviveCltStFunc = 24;
constexpr std::int16_t NativeReviveCltDoFunc = 0;
constexpr std::uint8_t BridgedReviveSelectProc = 2;
constexpr std::int16_t BridgedReviveSrvStFunc = 0;
constexpr std::int16_t BridgedReviveCltStFunc = 39;
constexpr std::int16_t BridgedReviveCltDoFunc = 36;

constexpr auto AiFunction03Expected = std::to_array<std::uint8_t>({
    0x40,0x53,0x57,0x41,0x56,0x48,0x81,0xEC,0xD0,0x00,0x00,0x00,
});
constexpr auto DistanceExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x6C,0x24,0x18,
    0x48,0x89,0x74,0x24,0x20,0x57,0x41,0x54,0x41,0x55,
    0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x20,
});
constexpr auto SetVelocityExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,0x24,0x18,
    0x48,0x89,0x74,0x24,0x20,0x57,0x48,0x83,0xEC,0x20,
});
constexpr auto WalkToOwnerExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x6C,0x24,0x18,
    0x48,0x89,0x74,0x24,0x20,0x41,0x56,0x48,0x83,0xEC,
});
constexpr auto DistanceCallExpected = std::to_array<std::uint8_t>({
    0xE8,0x16,0x2B,0x0F,0x00,0x83,0xF8,0x01,
});
constexpr auto SetVelocityCallExpected = std::to_array<std::uint8_t>({
    0x41,0xB1,0x28,0x48,0x8B,0xCB,0x41,0x8D,0x50,0x07,
    0xE8,0x21,0x36,0x00,0x00,
});
constexpr auto WalkToOwnerCallExpected = std::to_array<std::uint8_t>({
    0xE8,0x2A,0x44,0x00,0x00,0xB8,0x01,0x00,0x00,0x00,
});
constexpr auto StatesCheckStateExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x74,0x24,0x10,
    0x57,0x48,0x83,0xEC,0x20,0x8B,0xDA,0x48,0x8B,0xF1,
    0xE8,0x07,0x68,0x01,0x00,0x85,0xC0,0x74,0x0E,0x83,
    0xE8,0x01,
});
constexpr auto ReviveStateMarkerWitnessExpected =
    std::to_array<std::uint8_t>({
        0x48,0x8B,0xCD,0xE8,0x50,0x5E,0xFE,0xFF,0xBB,0x01,
        0x00,0x00,0x00,0xBA,0x00,0x00,0x00,0x80,0x44,0x8B,
        0xC3,0x48,0x8B,0xCD,0xE8,0x2B,0xF6,0xDE,0xFF,0x44,
        0x8B,0xC3,0x8D,0x53,0x5F,0x48,0x8B,0xCD,0xE8,0x4D,
        0x69,0xDD,0xFF,
    });
constexpr auto AiSpecialStateDispatchReadExpected =
    std::to_array<std::uint8_t>({
        0x48,0x8B,0x45,0xAF,0x49,0x8B,0xCE,0x8B,0x10,0xE8,
        0xEA,0x0A,0x00,0x00,0x48,0x63,0x10,0x83,0xFA,0x06,
    });
constexpr auto GetMinionOwnerExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0xE8,
    0x02,0x66,0xEA,0xFF,0x83,0xF8,0x01,0x0F,0x85,0x9E,
    0x00,0x00,0x00,0x48,0x85,0xDB,0x74,0x0D,0x48,0x8B,
    0xCB,0xE8,0xEC,0x65,0xEA,0xFF,
});

constexpr auto ValidateReviveTargetExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,0xEC,0x30,
    0x48,0x8B,0xF9,0x49,0x8B,0xD8,0x49,0x8B,0xC8,0xE8,
    0xA8,0x14,0xDF,0xFF,0x83,0xF8,0x01,0x75,0x3B,0x41,
    0xB8,0x1E,0x01,0x00,0x00,
});
constexpr auto SrvDo58ReviveExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,
    0x56,0x41,0x57,0x48,0x81,0xEC,0x98,0x00,0x00,0x00,
    0x48,0x8B,0x05,0xCD,0xCA,0x46,0x02,0x48,0x33,0xC4,
    0x48,0x89,0x84,0x24,0x88,0x00,0x00,0x00,
});
constexpr auto ClientReviveSelectorExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,0x24,0x18,
    0x48,0x89,0x74,0x24,0x20,0x57,0x48,0x83,0xEC,0x30,
    0x48,0x8B,0xDA,0x48,0x8B,0xF1,0x48,0x85,0xD2,0x0F,
    0x84,0x6B,
});
constexpr auto ClientReviveAiGateExpected = std::to_array<std::uint8_t>({
    0x41,0xB1,0x01,0xC6,0x44,0x24,0x20,0x01,0x45,0x0F,
    0xB6,0xC1,0x41,0x0F,0xB6,0xD1,0x48,0x8B,0xCB,0xE8,
    0xE3,0x60,0x2B,0x00,
});
constexpr auto ClientReviveAiGateCallExpected =
    std::to_array<std::uint8_t>({0xE8,0xE3,0x60,0x2B,0x00});
constexpr auto GetUnitTypeExpected = std::to_array<std::uint8_t>({
    0x48,0x83,0xEC,0x28,0x48,0x85,0xC9,0x75,0x1D,0x88,
    0x4C,0x24,0x30,0x48,0x8D,0x4C,0x24,0x30,0xE8,0x39,
    0x9E,0xFF,0xFF,0x84,0xC0,0x74,0x01,0xCC,0xB8,0x06,
    0x00,0x00,
});
constexpr auto GetMonStats2Expected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x55,0x56,0x57,0x48,0x83,
    0xEC,0x30,0x48,0x63,0xF2,0x0F,0xB6,0xE9,0xE8,0xC9,
    0x94,0x26,0x00,0x48,0x8B,0xF8,
});
constexpr auto IsUnitDeadExpected = std::to_array<std::uint8_t>({
    0x48,0x83,0xEC,0x28,0x48,0x85,0xC9,0x75,0x1D,0x88,
    0x4C,0x24,0x30,0x48,0x8D,0x4C,0x24,0x30,0xE8,0x59,
    0x94,0xFF,0xFF,0x84,0xC0,0x74,0x4D,0xCC,0xB8,0x01,
    0x00,0x00,0x00,
});
constexpr auto IsCorpseConsumableExpected = std::to_array<std::uint8_t>({
    0x40,0x57,0x48,0x83,0xEC,0x20,0x48,0x8B,0xF9,0xE8,
    0x32,0x52,0x00,0x00,0x85,0xC0,0x74,0x46,0x41,0xB8,
    0xDB,0x0E,0x00,0x00,
});
constexpr auto CanUnitSwitchAiExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x6C,0x24,0x18,
    0x48,0x89,0x74,0x24,0x20,0x57,0x41,0x56,0x41,0x57,
    0x48,0x83,0xEC,0x20,0x45,0x0F,0xB6,0xF1,0x45,0x0F,
    0xB6,0xF8,
});
constexpr auto CheckMonsterTypeFlagExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x57,0x48,0x83,0xEC,0x20,
    0x0F,0xB7,0xFA,0x48,0x8B,0xD9,0x48,0x85,0xC9,0x74,
    0x0A,0xE8,0x46,0xD1,0xFB,0xFF,0x83,0xF8,0x01,0x74,
    0x19,
});
constexpr auto GetMonsterUModsExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0x48,
    0x85,0xC9,0x74,0x0A,0xE8,0xAD,0xD6,0xFB,0xFF,0x83,
    0xF8,0x01,0x74,0x19,
});
constexpr auto GetRightSkillExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0x48,
    0x85,0xC9,0x75,0x13,0x88,0x4C,0x24,0x30,0x48,0x8D,
    0x4C,0x24,0x30,0xE8,0xF4,0x9A,0xFF,0xFF,0x84,0xC0,
    0x74,0x01,
});
constexpr auto AssignSkillExpected = std::to_array<std::uint8_t>({
    0x41,0x83,0xF8,0x05,0x0F,0x84,0x28,0x02,0x00,0x00,
    0x44,0x89,0x4C,0x24,0x20,0x55,0x41,0x56,0x41,0x57,
    0x48,0x83,0xEC,0x40,0x48,0x89,0x5C,0x24,0x60,0x45,
    0x8B,0xF0,
});

using AiFunction03Fn = std::int32_t(__fastcall*)(
    void*, void*, void*) noexcept;
using SetVelocityFn = void(__fastcall*)(
    void*, std::int32_t, std::int32_t, std::uint8_t) noexcept;
using DistanceFn = std::int32_t(__fastcall*)(void*, void*) noexcept;
using WalkToOwnerFn = std::int32_t(__fastcall*)(
    void*, void*, void*, std::uint8_t, std::uint16_t) noexcept;
using StatesCheckStateFn = std::int32_t(__fastcall*)(
    void*, std::int32_t) noexcept;
using GetMinionOwnerFn = void*(__fastcall*)(void*) noexcept;
using ValidateReviveTargetFn = std::int32_t(__fastcall*)(
    void*, void*, void*) noexcept;
using SrvDo58ReviveFn = std::int32_t(__fastcall*)(
    void*, void*, std::int32_t, std::int32_t) noexcept;
using ClientValidateReviveTargetFn = bool(__fastcall*)(
    void*, void*) noexcept;
using GetUnitTypeFn = std::int32_t(__fastcall*)(void*) noexcept;
using GetMonStats2Fn = const std::uint8_t*(__fastcall*)(
    std::uint8_t, std::int32_t) noexcept;
using IsUnitDeadFn = std::int32_t(__fastcall*)(void*) noexcept;
using IsCorpseConsumableFn = std::int32_t(__fastcall*)(
    void*, std::int32_t) noexcept;
using CanUnitSwitchAiFn = bool(__fastcall*)(
    void*, bool, bool, bool, bool) noexcept;
using CheckMonsterTypeFlagFn = bool(__fastcall*)(
    void*, std::uint16_t) noexcept;
using GetMonsterUModsFn = const std::uint8_t*(__fastcall*)(void*) noexcept;
using GetRightSkillFn = void*(__fastcall*)(void*) noexcept;
using AssignSkillFn = void(__fastcall*)(
    void*, std::int32_t, std::int32_t, std::int32_t) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
std::string LoadedConfigPath{"embedded defaults"};
std::atomic_bool Operational{};
std::atomic_bool AnyHookInstalled{};

AiFunction03Fn OriginalAiFunction03{};
SetVelocityFn OriginalSetVelocity{};
DistanceFn OriginalDistance{};
WalkToOwnerFn OriginalWalkToOwner{};
ValidateReviveTargetFn OriginalValidateReviveTarget{};
SrvDo58ReviveFn OriginalSrvDo58Revive{};
ClientValidateReviveTargetFn OriginalClientValidateReviveTarget{};
GetUnitTypeFn GetUnitType{};
GetMonStats2Fn GetMonStats2{};
IsUnitDeadFn IsUnitDead{};
IsCorpseConsumableFn IsCorpseConsumable{};
CanUnitSwitchAiFn CanUnitSwitchAi{};
CheckMonsterTypeFlagFn CheckMonsterTypeFlag{};
GetMonsterUModsFn GetMonsterUMods{};
GetRightSkillFn GetRightSkill{};
StatesCheckStateFn StatesCheckState{};
GetMinionOwnerFn GetMinionOwner{};
AssignSkillFn AssignSkill{};
void* ClientReviveAiGateRelay{};

thread_local std::uint32_t ReviveAiDepth{};
std::atomic_uint64_t ReviveTicks{};
std::atomic_uint64_t ScatterSuppressions{};
std::atomic_uint64_t CatchUpsForced{};
std::atomic_uint64_t FollowAdjustments{};
std::atomic_uint64_t VelocityAdjustments{};
std::atomic_uint64_t HighRankAdmissions{};
std::atomic_uint64_t ClientGateCalls{};
std::atomic_uint64_t ClientHighRankCandidates{};
std::atomic_uint64_t ClientHighRankSelections{};
std::atomic_uint64_t ClientSelectorHighRankCalls{};
std::atomic_uint64_t ClientSelectorHighRankAccepts{};
std::atomic_uint64_t ClientSelectorHighRankRejects{};
std::atomic_uint64_t NativeAuraCaptures{};
std::atomic_uint64_t NativeAuraReactivations{};
std::atomic_uint64_t CallbackRouteApplications{};
std::atomic_uint64_t CallbackRouteFailures{};
std::atomic_uint64_t CallbackRouteRevision{};
std::atomic_bool CallbackRouteReady{};
const D2RL::DataTableServiceV1* DataTables{};
const D2RL::LifecycleServiceV1* Lifecycle{};
D2RL::Lifecycle::ListenerHandle DataTablesListener{
    D2RL::Lifecycle::InvalidHandle};
std::atomic_uint64_t NativeAuraConflicts{};
thread_local std::uint32_t ScriptedAiProbeCooldown{};
std::atomic_bool ScriptedAiAbiWarningLogged{};
std::atomic_bool ScriptedAiProviderLogged{};
std::atomic_uint64_t ScriptedAiProviderProbes{};
std::atomic_uint64_t ScriptedAiTacticalCalls{};
std::atomic_uint64_t ScriptedAiTacticalHandled{};
std::atomic_uint64_t ScriptedAiTacticalDelegates{};
std::atomic_uint64_t ScriptedAiTacticalUnavailable{};
std::atomic_uint64_t ScriptedAiTacticalErrors{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "revive-overhaul",
    .name = "Revive Overhaul",
    .version = Version,
    .author = "RuffnecKk",
    .description = "Improves Revive behavior and preserves eligible unique monster auras.",
    .flags = D2RL::PluginFlags::Client | D2RL::PluginFlags::Server
        | D2RL::PluginFlags::NativeHooks,
};

template<class T>
T At(const std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(Base + rva);
}

template<std::size_t Size>
bool Check(
        const std::uintptr_t rva,
        const std::array<std::uint8_t, Size>& expected,
        const char* name) noexcept {
    if (Context->CheckExpectedBytes(
            rva,
            expected.data(),
            static_cast<std::uint32_t>(expected.size()))) {
        return true;
    }
    const auto message = std::string("ReviveOverhaul: signature mismatch at ")
        + name + ".";
    Context->LogError(message.c_str());
    return false;
}

std::vector<std::filesystem::path> ConfigCandidates() {
    std::filesystem::path activeModConfigDirectory;
    std::filesystem::path scopeConfigDirectory;
    if (Context && Context->activeMod && Context->activeMod[0] != '\0'
            && Context->modSupportDirectory
            && Context->modSupportDirectory[0] != L'\0') {
        activeModConfigDirectory =
            std::filesystem::path(Context->modSupportDirectory) / L"config";
    }
    if (Context && Context->pluginConfigPath
            && Context->pluginConfigPath[0] != L'\0') {
        scopeConfigDirectory =
            std::filesystem::path(Context->pluginConfigPath).parent_path();
    }
    std::error_code currentPathError;
    const auto currentPath = std::filesystem::current_path(currentPathError);
    const auto globalConfigDirectory = currentPathError
        ? std::filesystem::path{}
        : currentPath / L"d2rloader" / L"config";
    return BuildConfigCandidates(
        {activeModConfigDirectory, scopeConfigDirectory, globalConfigDirectory},
        ConfigFileName,
        LegacyConfigFileName);
}

bool MaterializeDefaultConfig(
        const std::filesystem::path& path) noexcept {
    try {
        std::error_code directoryError;
        std::filesystem::create_directories(path.parent_path(), directoryError);
        if (directoryError) return false;
        const auto handle = CreateFileW(
            path.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (handle == INVALID_HANDLE_VALUE) return false;
        DWORD written{};
        const auto success = WriteFile(
            handle,
            DefaultConfig,
            static_cast<DWORD>(sizeof(DefaultConfig) - 1),
            &written,
            nullptr) != FALSE
            && written == sizeof(DefaultConfig) - 1;
        CloseHandle(handle);
        if (!success) (void)DeleteFileW(path.c_str());
        return success;
    } catch (...) {
        return false;
    }
}

bool LoadConfig() noexcept {
    Settings = {};
    LoadedConfigPath = "embedded defaults";
    const auto candidates = ConfigCandidates();
    for (const auto& path : candidates) {
        std::error_code statusError;
        if (!std::filesystem::is_regular_file(path, statusError)) continue;
        try {
            std::ifstream input(path, std::ios::binary);
            if (!input.is_open()) {
                throw std::runtime_error("configuration file cannot be opened");
            }
            const std::string text{
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>()};
            Config parsed{};
            std::string error;
            if (!ParseToml(text, parsed, error)) {
                throw std::invalid_argument(error);
            }
            Settings = parsed;
            LoadedConfigPath = path.string();
            if (path.filename() == LegacyConfigFileName && Context) {
                Context->LogWarn(
                    "ReviveOverhaul: legacy ReviveOverhaul.toml loaded; rename it to ruffneckk-revive-overhaul.toml when convenient.");
            }
            return true;
        } catch (const std::exception& exception) {
            if (Context) {
                const auto message = std::string("ReviveOverhaul: invalid ")
                    + path.string() + " (" + exception.what() + ").";
                Context->LogError(message.c_str());
            }
            return false;
        }
    }

    std::filesystem::path materializedPath;
    if (Context && Context->pluginConfigPath
            && Context->pluginConfigPath[0] != L'\0') {
        materializedPath = std::filesystem::path(
            Context->pluginConfigPath).parent_path() / ConfigFileName;
    } else if (!candidates.empty()) {
        materializedPath = candidates.front();
    }
    if (!materializedPath.empty() && MaterializeDefaultConfig(materializedPath)) {
        LoadedConfigPath = materializedPath.string();
        if (Context) {
            const auto message = std::string(
                "ReviveOverhaul: created default configuration at ")
                + LoadedConfigPath + ".";
            Context->LogInfo(message.c_str());
        }
        return true;
    }
    if (Context) {
        Context->LogWarn(
            "ReviveOverhaul: no TOML was found or created; embedded defaults are active.");
    }
    return true;
}

bool IsExpectedReturnAddress(
        void* returnAddress,
        const std::uintptr_t rva) noexcept {
    return Base && reinterpret_cast<std::uint8_t*>(returnAddress) == Base + rva;
}

bool IsNativeRevive(void* monster) noexcept {
    return monster
        && StatesCheckState(monster, NativeReviveState) != 0;
}

struct AiTickView {
    void* aiControl{};
    std::uint64_t reserved08{};
    void* target{};
    std::uint64_t reserved18{};
    std::int32_t targetDistance{};
    std::int32_t inCombat{};
    const void* monStats{};
    const void* monStats2{};
};

static_assert(offsetof(AiTickView, aiControl) == 0U);
static_assert(offsetof(AiTickView, target) == 16U);
static_assert(offsetof(AiTickView, targetDistance) == 32U);
static_assert(offsetof(AiTickView, inCombat) == 36U);
static_assert(sizeof(void*) != 8U || sizeof(AiTickView) == 56U);

[[nodiscard]] std::int32_t ReadAiSpecialState(void* tickParam) noexcept {
    if (!tickParam) return -1;
    const auto* const tick = static_cast<const AiTickView*>(tickParam);
    if (!tick->aiControl) return -1;
    return *static_cast<const std::int32_t*>(tick->aiControl);
}

struct ScriptedAiModuleLease {
    HMODULE module{};

    ScriptedAiModuleLease() = default;
    ScriptedAiModuleLease(const ScriptedAiModuleLease&) = delete;
    auto operator=(const ScriptedAiModuleLease&)
        -> ScriptedAiModuleLease& = delete;
    ~ScriptedAiModuleLease() noexcept {
        if (module != nullptr) ::FreeLibrary(module);
    }
};

[[nodiscard]] auto ResolveScriptedAiReviveProvider(
        ScriptedAiModuleLease& lease) noexcept
        -> const ruffneckk::scripted_ai::revive_v3::Interface* {
    using namespace ruffneckk::scripted_ai::revive_v3;
    if (ScriptedAiProbeCooldown != 0U) {
        --ScriptedAiProbeCooldown;
        return nullptr;
    }
    ScriptedAiProbeCooldown = 255U;
    ScriptedAiProviderProbes.fetch_add(1U, std::memory_order_relaxed);
    if (::GetModuleHandleW(ProviderModuleName) == nullptr
            || !::GetModuleHandleExW(0U, ProviderModuleName, &lease.module)) {
        return nullptr;
    }
    const auto query = reinterpret_cast<QueryFunction>(
        ::GetProcAddress(lease.module, QueryExportName));
    if (query == nullptr) {
        if (!ScriptedAiAbiWarningLogged.exchange(
                true,
                std::memory_order_acq_rel)
                && Context != nullptr) {
            Context->LogWarn(
                "ReviveOverhaul: Scripted AI was found without a compatible Revive tactics ABI v3; native AI remains active.");
        }
        ::FreeLibrary(lease.module);
        lease.module = nullptr;
        return nullptr;
    }
    const auto* const candidate = query(AbiVersion, sizeof(Interface));
    if (candidate == nullptr) {
        ::FreeLibrary(lease.module);
        lease.module = nullptr;
        return nullptr;
    }
    if (!IsCompatible(candidate)) {
        if (!ScriptedAiAbiWarningLogged.exchange(
                true,
                std::memory_order_acq_rel)
                && Context != nullptr) {
            Context->LogWarn(
                "ReviveOverhaul: Scripted AI returned an incompatible Revive tactics ABI v3; native AI remains active.");
        }
        ::FreeLibrary(lease.module);
        lease.module = nullptr;
        return nullptr;
    }
    ScriptedAiProbeCooldown = 0U;
    if (Settings.diagnostics && Context != nullptr
            && !ScriptedAiProviderLogged.exchange(
                true,
                std::memory_order_acq_rel)) {
        Context->LogInfo(
            "ReviveOverhaul: compatible Scripted AI Revive tactics ABI v3 discovered; Revive Overhaul remains the sole AiFunction03 hook owner.");
    }
    return candidate;
}

[[nodiscard]] bool ExecuteScriptedAiReviveTactics(
        void* game,
        void* monster,
        void* tickParam) noexcept {
    using namespace ruffneckk::scripted_ai::revive_v3;
    const auto specialState = ReadAiSpecialState(tickParam);
    if (specialState != 0 && specialState != 7) return false;
    if (!GetMinionOwner || !OriginalDistance) return false;
    ScriptedAiModuleLease lease;
    const auto* const provider = ResolveScriptedAiReviveProvider(lease);
    if (!provider) return false;
    auto* const owner = GetMinionOwner(monster);
    if (!owner) return false;
    const auto ownerDistance = OriginalDistance(monster, owner);
    if (ownerDistance < 0) return false;
    const auto* const tick = static_cast<const AiTickView*>(tickParam);
    if (!tick) return false;
    const auto targetOwnerDistance = tick->target != nullptr
        ? OriginalDistance(tick->target, owner)
        : -1;
    if (tick->target != nullptr && (tick->targetDistance < 0
            || targetOwnerDistance < 0)) {
        return false;
    }

    const ruffneckk::scripted_ai::revive_v3::Context request{
        .structSize = sizeof(
            ruffneckk::scripted_ai::revive_v3::Context),
        .abiVersion = AbiVersion,
        .game = game,
        .monster = monster,
        .target = tick->target,
        .owner = owner,
        .monStats = tick->monStats,
        .ownerDistance = ownerDistance,
        .targetDistance = tick->target != nullptr
            ? tick->targetDistance
            : -1,
        .targetOwnerDistance = targetOwnerDistance,
        .specialState = specialState,
        .inCombat = tick->inCombat,
    };
    ScriptedAiTacticalCalls.fetch_add(1U, std::memory_order_relaxed);
    const auto result = provider->evaluate(&request);
    switch (result) {
    case Result::Handled:
        ScriptedAiTacticalHandled.fetch_add(
            1U,
            std::memory_order_relaxed);
        return true;
    case Result::DelegateNative:
        ScriptedAiTacticalDelegates.fetch_add(1U, std::memory_order_relaxed);
        return false;
    case Result::Unavailable:
        ScriptedAiTacticalUnavailable.fetch_add(1U, std::memory_order_relaxed);
        return false;
    case Result::Error:
        ScriptedAiTacticalErrors.fetch_add(1U, std::memory_order_relaxed);
        return false;
    }
    ScriptedAiTacticalErrors.fetch_add(1U, std::memory_order_relaxed);
    return false;
}

class ReviveAiScope {
public:
    explicit ReviveAiScope(const bool active) noexcept : active_(active) {
        if (active_) ++ReviveAiDepth;
    }
    ~ReviveAiScope() noexcept {
        if (active_) --ReviveAiDepth;
    }
    ReviveAiScope(const ReviveAiScope&) = delete;
    ReviveAiScope& operator=(const ReviveAiScope&) = delete;

private:
    bool active_{};
};

bool IsEligibleHighRankCorpse(void* game, void* target) noexcept {
    if (!game || !target
            || GetUnitType(target) != 1
            || !CheckMonsterTypeFlag(
                target, ChampionUniqueSuperUniqueMask)) {
        return false;
    }
    const auto classId = *reinterpret_cast<const std::int32_t*>(
        static_cast<const std::uint8_t*>(target) + UnitClassIdOffset);
    if (classId < 0) return false;
    const auto dataContext = *(static_cast<const std::uint8_t*>(game)
        + GameDataContextOffset);
    const auto* monStats2 = GetMonStats2(dataContext, classId);
    if (!monStats2) return false;
    const auto corpseSelectionFlags =
        monStats2[MonStats2FlagsOffset];
    const auto reviveFlags = monStats2[MonStats2FlagsOffset + 1];
    if ((corpseSelectionFlags & MonStats2CorpseSelMask) == 0
            || (reviveFlags & MonStats2ReviveMask) == 0
            || IsUnitDead(target) == 0
            || IsCorpseConsumable(target, 0) == 0) {
        return false;
    }

    // Preserve native state, mode, boss/prime-evil and SwitchAI checks. Only
    // the native Unique|SuperUnique rejection is disabled (third argument).
    return CanUnitSwitchAi
        && CanUnitSwitchAi(target, true, false, true, true);
}

struct AuraSnapshot {
    bool valid{};
    std::int32_t skillId{};
    std::int32_t ownerGuid{-1};
};

struct AuraCaptureFrame {
    AuraCaptureFrame* previous{};
    void* target{};
    AuraSnapshot aura{};
    bool captured{};
};

thread_local AuraCaptureFrame* ActiveAuraCaptureFrame{};

AuraSnapshot CaptureNativeAura(void* target) noexcept {
    if (!target
            || GetUnitType(target) != 1
            || !CheckMonsterTypeFlag(
                target, ChampionUniqueSuperUniqueMask)
            || !HasAuraEnchantedUMod(GetMonsterUMods(target))) {
        return {};
    }
    auto* activeSkill = GetRightSkill(target);
    if (!activeSkill) return {};
    const auto* skillRecord = *reinterpret_cast<const std::uint8_t* const*>(
        static_cast<std::uint8_t*>(activeSkill) + SkillRecordOffset);
    if (!skillRecord) return {};
    const auto skillId = static_cast<std::int32_t>(
        *reinterpret_cast<const std::int16_t*>(skillRecord));
    if (skillId <= 0) return {};
    const auto ownerGuid = *reinterpret_cast<const std::int32_t*>(
        static_cast<std::uint8_t*>(activeSkill) + SkillOwnerGuidOffset);
    ++NativeAuraCaptures;
    return {true, skillId, ownerGuid};
}

void RestoreNativeAura(
        void* target,
        const AuraSnapshot& snapshot) noexcept {
    if (!target || !snapshot.valid) return;
    if (auto* current = GetRightSkill(target)) {
        const auto* record = *reinterpret_cast<const std::uint8_t* const*>(
            static_cast<std::uint8_t*>(current) + SkillRecordOffset);
        if (!record) {
            ++NativeAuraConflicts;
            return;
        }
        const auto currentSkillId = static_cast<std::int32_t>(
            *reinterpret_cast<const std::int16_t*>(record));
        const auto currentOwnerGuid =
            *reinterpret_cast<const std::int32_t*>(
                static_cast<std::uint8_t*>(current)
                + SkillOwnerGuidOffset);
        if (currentSkillId != snapshot.skillId
                || currentOwnerGuid != snapshot.ownerGuid) {
            ++NativeAuraConflicts;
            return;
        }
    }
    // Aura Enchanted itself assigns the generated skill through this complete
    // path. It updates the active slot and performs the additional native
    // activation/synchronization work that SetRightActiveSkill alone omits.
    AssignSkill(target, 0, snapshot.skillId, snapshot.ownerGuid);
    ++NativeAuraReactivations;
}

__declspec(noinline) std::int32_t __fastcall HookAiFunction03(
        void* game,
        void* monster,
        void* tickParam) noexcept {
    const bool revived = Operational.load(std::memory_order_acquire)
        && Settings.ai.enabled
        && IsNativeRevive(monster);
    if (revived) ++ReviveTicks;
    const auto* const tick = static_cast<const AiTickView*>(tickParam);
    if (revived && tick != nullptr
            && ExecuteScriptedAiReviveTactics(game, monster, tickParam)) {
        return 1;
    }
    const bool peaceful = revived && tick != nullptr
        && IsPeacefulReviveTick(tick->target != nullptr, tick->inCombat != 0);
    const ReviveAiScope scope(peaceful);
    return OriginalAiFunction03(game, monster, tickParam);
}

__declspec(noinline) bool __fastcall HookClientCanUnitSwitchAi(
        void* monster,
        bool checkState143,
        bool checkUnique,
        bool checkBoss,
        bool checkSwitchAi) noexcept {
    ++ClientGateCalls;
    const bool clientReviveHighRank =
        Operational.load(std::memory_order_acquire)
        && Settings.revive.allowHighRankMonsters
        && CallbackRouteReady.load(std::memory_order_acquire)
        && checkUnique
        && CheckMonsterTypeFlag(
            monster, ChampionUniqueSuperUniqueMask);
    if (clientReviveHighRank) ++ClientHighRankCandidates;
    const auto result = CanUnitSwitchAi(
        monster,
        checkState143,
        clientReviveHighRank ? false : checkUnique,
        checkBoss,
        checkSwitchAi);
    if (clientReviveHighRank && result) ++ClientHighRankSelections;
    return result;
}

void* AllocateNear(void* hint, const std::size_t size) noexcept {
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const auto granularity = static_cast<std::uintptr_t>(
        systemInfo.dwAllocationGranularity);
    const auto base = reinterpret_cast<std::uintptr_t>(hint)
        & ~(granularity - 1);
    for (std::uintptr_t delta = granularity;
            delta < 0x70000000ULL;
            delta += granularity) {
        if (base > std::numeric_limits<std::uintptr_t>::max() - delta) break;
        if (auto* memory = VirtualAlloc(
                reinterpret_cast<void*>(base + delta),
                size,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_READWRITE)) {
            return memory;
        }
    }
    return nullptr;
}

bool InstallClientReviveAiGateRedirect() noexcept {
    constexpr std::size_t RelaySize = 14;
    ClientReviveAiGateRelay = AllocateNear(
        Base + ClientReviveAiGateCallRva, RelaySize);
    if (!ClientReviveAiGateRelay) return false;

    auto* relay = static_cast<std::uint8_t*>(ClientReviveAiGateRelay);
    const std::array<std::uint8_t, 6> absoluteJump{
        0xFF,0x25,0x00,0x00,0x00,0x00,
    };
    std::memcpy(relay, absoluteJump.data(), absoluteJump.size());
    const auto target = reinterpret_cast<std::uint64_t>(
        &HookClientCanUnitSwitchAi);
    std::memcpy(relay + absoluteJump.size(), &target, sizeof(target));

    DWORD previousProtection{};
    if (!VirtualProtect(
            relay, RelaySize, PAGE_EXECUTE_READ, &previousProtection)) {
        return false;
    }
    FlushInstructionCache(GetCurrentProcess(), relay, RelaySize);

    const auto relayAddress = reinterpret_cast<std::uintptr_t>(relay);
    const auto baseAddress = reinterpret_cast<std::uintptr_t>(Base);
    if (relayAddress < baseAddress) return false;
    const auto relayRva = relayAddress - baseAddress;
    const auto nextInstruction = baseAddress
        + ClientReviveAiGateCallRva
        + ClientReviveAiGateCallExpected.size();
    const auto displacement = static_cast<std::int64_t>(relayAddress)
        - static_cast<std::int64_t>(nextInstruction);
    if (displacement < std::numeric_limits<std::int32_t>::min()
            || displacement > std::numeric_limits<std::int32_t>::max()) {
        return false;
    }
    return Context->PatchCallRel32(
        ClientReviveAiGateCallRva,
        ClientReviveAiGateCallExpected.data(),
        static_cast<std::uint32_t>(
            ClientReviveAiGateCallExpected.size()),
        relayRva,
        static_cast<std::uint32_t>(
            ClientReviveAiGateCallExpected.size()));
}

template <typename T>
T ReadCompiledField(const void* row, const std::size_t offset) noexcept {
    T value{};
    std::memcpy(
        &value,
        static_cast<const std::uint8_t*>(row) + offset,
        sizeof(value));
    return value;
}

template <typename T>
bool WriteCompiledField(
        void* row,
        const std::size_t offset,
        const T value) noexcept {
    auto* destination = static_cast<std::uint8_t*>(row) + offset;
    DWORD previousProtection{};
    if (!VirtualProtect(
            destination,
            sizeof(value),
            PAGE_READWRITE,
            &previousProtection)) {
        return false;
    }
    std::memcpy(destination, &value, sizeof(value));
    DWORD ignored{};
    return VirtualProtect(
        destination,
        sizeof(value),
        previousProtection,
        &ignored) != FALSE;
}

struct ReviveCallbackRouteSnapshot {
    void* row{};
    std::uint8_t selectProc{};
    std::int16_t srvStFunc{};
    std::int16_t srvDoFunc{};
    std::int16_t cltStFunc{};
    std::int16_t cltDoFunc{};
    bool needsPatch{};
};

bool IsNativeReviveCallbackRoute(
        const ReviveCallbackRouteSnapshot& row) noexcept {
    return row.selectProc == NativeReviveSelectProc
        && row.srvStFunc == NativeReviveSrvStFunc
        && row.srvDoFunc == NativeReviveSrvDoFunc
        && row.cltStFunc == NativeReviveCltStFunc
        && row.cltDoFunc == NativeReviveCltDoFunc;
}

bool IsBridgedReviveCallbackRoute(
        const ReviveCallbackRouteSnapshot& row) noexcept {
    return row.selectProc == BridgedReviveSelectProc
        && row.srvStFunc == BridgedReviveSrvStFunc
        && row.srvDoFunc == NativeReviveSrvDoFunc
        && row.cltStFunc == BridgedReviveCltStFunc
        && row.cltDoFunc == BridgedReviveCltDoFunc;
}

bool WriteReviveCallbackRoute(
        const ReviveCallbackRouteSnapshot& row,
        const bool bridge) noexcept {
    return WriteCompiledField(
            row.row,
            SkillsSelectProcOffset,
            bridge ? BridgedReviveSelectProc : row.selectProc)
        && WriteCompiledField(
            row.row,
            SkillsSrvStFuncOffset,
            bridge ? BridgedReviveSrvStFunc : row.srvStFunc)
        && WriteCompiledField(
            row.row,
            SkillsCltStFuncOffset,
            bridge ? BridgedReviveCltStFunc : row.cltStFunc)
        && WriteCompiledField(
            row.row,
            SkillsCltDoFuncOffset,
            bridge ? BridgedReviveCltDoFunc : row.cltDoFunc);
}

void __cdecl OnDataTablesLoaded(
        const D2RL::PluginContext* context,
        const D2RL::Lifecycle::DataTablesLoadedEvent* event,
        void*) noexcept {
    CallbackRouteReady.store(false, std::memory_order_release);
    ScriptedAiProbeCooldown = 0U;
    ScriptedAiAbiWarningLogged.store(false, std::memory_order_release);
    ScriptedAiProviderLogged.store(false, std::memory_order_release);
    if (!context || context != Context || !event || !DataTables
            || !Operational.load(std::memory_order_acquire)
            || !Settings.revive.allowHighRankMonsters
            || !D2RL::Lifecycle::HasDataTablesLoadedEventField(
                event,
                D2RL::Lifecycle::DataTablesLoadedEventRequiredSize)) {
        return;
    }

    constexpr std::array banks{
        D2RL::DataTables::Bank::Classic,
        D2RL::DataTables::Bank::Lod,
        D2RL::DataTables::Bank::Rotw,
    };
    std::array<ReviveCallbackRouteSnapshot, banks.size()> rows{};
    bool valid = true;
    for (std::size_t index = 0; index < banks.size(); ++index) {
        D2RL::DataTables::RowView view{
            .structSize = D2RL::DataTables::RowViewSize,
        };
        if (DataTables->findRowById(
                context,
                banks[index],
                D2RL::DataTables::TableId::Skills,
                ReviveSkillId,
                &view) != D2RL::DataTables::Result::Success
                || view.revision != event->revision
                || view.row == nullptr
                || view.rowSize != SkillsRecordSize
                || ReadCompiledField<std::uint16_t>(view.row, 0)
                    != ReviveSkillId) {
            valid = false;
            break;
        }
        auto& row = rows[index];
        row.row = const_cast<void*>(view.row);
        row.selectProc = ReadCompiledField<std::uint8_t>(
            view.row, SkillsSelectProcOffset);
        row.srvStFunc = ReadCompiledField<std::int16_t>(
            view.row, SkillsSrvStFuncOffset);
        row.srvDoFunc = ReadCompiledField<std::int16_t>(
            view.row, SkillsSrvDoFuncOffset);
        row.cltStFunc = ReadCompiledField<std::int16_t>(
            view.row, SkillsCltStFuncOffset);
        row.cltDoFunc = ReadCompiledField<std::int16_t>(
            view.row, SkillsCltDoFuncOffset);
        row.needsPatch = IsNativeReviveCallbackRoute(row);
        if (!row.needsPatch && !IsBridgedReviveCallbackRoute(row)) {
            valid = false;
            break;
        }
    }

    std::size_t patched{};
    if (valid) {
        for (const auto& row : rows) {
            if (!row.needsPatch) continue;
            if (!WriteReviveCallbackRoute(row, true)) {
                valid = false;
                break;
            }
            ++patched;
        }
    }
    if (!valid) {
        for (const auto& row : rows) {
            if (row.row && row.needsPatch) {
                (void)WriteReviveCallbackRoute(row, false);
            }
        }
        ++CallbackRouteFailures;
        context->LogError(
            "ReviveOverhaul: automatic Revive callback route rejected; compiled Skills contract drifted or could not be updated.");
        return;
    }

    CallbackRouteApplications.fetch_add(
        patched, std::memory_order_relaxed);
    CallbackRouteRevision.store(
        event->revision, std::memory_order_release);
    CallbackRouteReady.store(true, std::memory_order_release);
    context->LogInfo(
        "ReviveOverhaul: automatic high-rank Revive callback route active; no Skills.txt edit is required.");
}

bool RegisterDataTableRoute() noexcept {
    if (!Settings.revive.allowHighRankMonsters) return true;
    if (Context->QueryService(
            D2RL::ServiceId::DataTable,
            D2RL::DataTableServiceV1Version,
            &DataTables) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasDataTableServiceV1Field(
                DataTables,
                D2RL::DataTableServiceV1RequiredSize)
            || Context->QueryService(
                D2RL::ServiceId::Lifecycle,
                D2RL::LifecycleServiceV1Version,
                &Lifecycle) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasLifecycleServiceV1Field(
                Lifecycle,
                D2RL::LifecycleServiceV1RequiredSize)) {
        Context->LogError(
            "ReviveOverhaul: DataTableServiceV1 and LifecycleServiceV1 are required for automatic Revive callback routing.");
        return false;
    }
    const D2RL::Lifecycle::DataTablesLoadedListener listener{
        .structSize = D2RL::Lifecycle::DataTablesLoadedListenerSize,
        .flags = 0,
        .callback = OnDataTablesLoaded,
        .userData = nullptr,
    };
    return Lifecycle->registerDataTablesLoadedListener(
            Context,
            &listener,
            &DataTablesListener) == D2RL::Lifecycle::Result::Success
        && DataTablesListener != D2RL::Lifecycle::InvalidHandle;
}

__declspec(noinline) bool __fastcall HookClientValidateReviveTarget(
        void* caster,
        void* target) noexcept {
    const bool highRank = Operational.load(std::memory_order_acquire)
        && Settings.revive.allowHighRankMonsters
        && CheckMonsterTypeFlag(target, ChampionUniqueSuperUniqueMask);
    const auto result = OriginalClientValidateReviveTarget(caster, target);
    if (highRank) {
        ++ClientSelectorHighRankCalls;
        if (result) ++ClientSelectorHighRankAccepts;
        else ++ClientSelectorHighRankRejects;
    }
    return result;
}

__declspec(noinline) std::int32_t __fastcall HookDistance(
        void* monster,
        void* owner) noexcept {
    void* const returnAddress = _ReturnAddress();
    const auto distance = OriginalDistance(monster, owner);
    if (!Operational.load(std::memory_order_acquire)
            || ReviveAiDepth == 0
            || !IsExpectedReturnAddress(returnAddress, DistanceReturnRva)) {
        return distance;
    }
    const auto transformed = TransformLeashDistance(distance, Settings.ai);
    if (transformed != distance) {
        if (distance <= NativeScatterMaximumDistance) ++ScatterSuppressions;
        else ++CatchUpsForced;
    }
    return transformed;
}

__declspec(noinline) void __fastcall HookSetVelocity(
        void* monster,
        std::int32_t mode,
        std::int32_t velocity,
        std::uint8_t bonus) noexcept {
    void* const returnAddress = _ReturnAddress();
    if (Operational.load(std::memory_order_acquire)
            && ReviveAiDepth != 0
            && IsExpectedReturnAddress(returnAddress, SetVelocityReturnRva)) {
        const auto transformed = TransformVelocityBonus(bonus, Settings.ai);
        if (transformed != bonus) ++VelocityAdjustments;
        bonus = transformed;
    }
    OriginalSetVelocity(monster, mode, velocity, bonus);
}

__declspec(noinline) std::int32_t __fastcall HookWalkToOwner(
        void* game,
        void* monster,
        void* owner,
        std::uint8_t distance,
        std::uint16_t flags) noexcept {
    void* const returnAddress = _ReturnAddress();
    if (Operational.load(std::memory_order_acquire)
            && ReviveAiDepth != 0
            && IsExpectedReturnAddress(returnAddress, WalkToOwnerReturnRva)) {
        const auto transformed = TransformFollowDistance(distance, Settings.ai);
        if (transformed != distance) ++FollowAdjustments;
        distance = transformed;
    }
    return OriginalWalkToOwner(game, monster, owner, distance, flags);
}

__declspec(noinline) std::int32_t __fastcall HookValidateReviveTarget(
        void* game,
        void* caster,
        void* target) noexcept {
    auto result = OriginalValidateReviveTarget(
        game, caster, target);
    if (result == 0
            && Operational.load(std::memory_order_acquire)
            && Settings.revive.allowHighRankMonsters
            && CallbackRouteReady.load(std::memory_order_acquire)
            && IsEligibleHighRankCorpse(game, target)) {
        ++HighRankAdmissions;
        result = 1;
    }
    if (result != 0
            && ActiveAuraCaptureFrame
            && !ActiveAuraCaptureFrame->captured) {
        ActiveAuraCaptureFrame->target = target;
        ActiveAuraCaptureFrame->aura = CaptureNativeAura(target);
        ActiveAuraCaptureFrame->captured = true;
    }
    return result;
}

__declspec(noinline) std::int32_t __fastcall HookSrvDo58Revive(
        void* game,
        void* caster,
        std::int32_t skillId,
        std::int32_t skillLevel) noexcept {
    AuraCaptureFrame capture{};
    const bool captureEnabled = Operational.load(std::memory_order_acquire)
        && Settings.revive.preserveNativeAuras;
    if (captureEnabled) {
        capture.previous = ActiveAuraCaptureFrame;
        ActiveAuraCaptureFrame = &capture;
    }
    const auto result = OriginalSrvDo58Revive(
        game, caster, skillId, skillLevel);
    if (captureEnabled) ActiveAuraCaptureFrame = capture.previous;
    if (result != 0 && capture.aura.valid) {
        RestoreNativeAura(capture.target, capture.aura);
    }
    return result;
}

bool ValidateAiRuntime() noexcept {
    return Check(AiFunction03Rva, AiFunction03Expected, "AiFunction03")
        && Check(
            StatesCheckStateRva,
            StatesCheckStateExpected,
            "STATES_CheckState")
        && Check(
            ReviveStateMarkerWitnessRva,
            ReviveStateMarkerWitnessExpected,
            "native Revive state marker")
        && Check(
            AiSpecialStateDispatchReadRva,
            AiSpecialStateDispatchReadExpected,
            "AI special-state dispatch read")
        && Check(
            GetMinionOwnerRva,
            GetMinionOwnerExpected,
            "minion owner resolver")
        && Check(DistanceRva, DistanceExpected, "unit distance helper")
        && Check(SetVelocityRva, SetVelocityExpected, "SetVelocity")
        && Check(WalkToOwnerRva, WalkToOwnerExpected, "WalkToOwner")
        && Check(
            DistanceReturnRva - 5,
            DistanceCallExpected,
            "Revive distance call site")
        && Check(
            SetVelocityReturnRva - SetVelocityCallExpected.size(),
            SetVelocityCallExpected,
            "Revive velocity call site")
        && Check(
            WalkToOwnerReturnRva - 5,
            WalkToOwnerCallExpected,
            "Revive owner-follow call site");
}

bool ValidateReviveRuntime() noexcept {
    if (!Check(
            ValidateReviveTargetRva,
            ValidateReviveTargetExpected,
            "Revive target validator")) {
        return false;
    }
    if (Settings.revive.allowHighRankMonsters) {
        if (!Check(
                    ClientReviveSelectorRva,
                    ClientReviveSelectorExpected,
                    "client Revive selector")
                || !Check(
                    ClientReviveAiGateRva,
                    ClientReviveAiGateExpected,
                    "client Revive AI gate")
                || !Check(
                    GetMonStats2Rva,
                    GetMonStats2Expected,
                    "GetMonStats2")
                || !Check(IsUnitDeadRva, IsUnitDeadExpected, "IsUnitDead")
                || !Check(
                    IsCorpseConsumableRva,
                    IsCorpseConsumableExpected,
                    "IsCorpseConsumable")
                || !Check(
                    CanUnitSwitchAiRva,
                    CanUnitSwitchAiExpected,
                    "CanUnitSwitchAi")) {
            return false;
        }
    }
    if (!Settings.revive.allowHighRankMonsters
            && !Settings.revive.preserveNativeAuras) {
        return true;
    }
    if (!Check(GetUnitTypeRva, GetUnitTypeExpected, "GetUnitType")
            || !Check(
                CheckMonsterTypeFlagRva,
                CheckMonsterTypeFlagExpected,
                "CheckMonsterTypeFlag")) {
        return false;
    }
    if (!Settings.revive.preserveNativeAuras) return true;
    return Check(SrvDo58ReviveRva, SrvDo58ReviveExpected, "SrvDoFunc 58")
        && Check(
            GetMonsterUModsRva,
            GetMonsterUModsExpected,
            "GetMonsterUMods")
        && Check(GetRightSkillRva, GetRightSkillExpected, "GetRightSkill")
        && Check(AssignSkillRva, AssignSkillExpected, "AssignSkill");
}

bool ValidateRuntime() noexcept {
    if (Settings.ai.enabled && !ValidateAiRuntime()) return false;
    if ((Settings.revive.allowHighRankMonsters
            || Settings.revive.preserveNativeAuras)
            && !ValidateReviveRuntime()) {
        return false;
    }
    return true;
}

void ResolveNativeFunctions() noexcept {
    GetUnitType = At<GetUnitTypeFn>(GetUnitTypeRva);
    GetMonStats2 = At<GetMonStats2Fn>(GetMonStats2Rva);
    IsUnitDead = At<IsUnitDeadFn>(IsUnitDeadRva);
    IsCorpseConsumable = At<IsCorpseConsumableFn>(IsCorpseConsumableRva);
    CanUnitSwitchAi = At<CanUnitSwitchAiFn>(CanUnitSwitchAiRva);
    CheckMonsterTypeFlag = At<CheckMonsterTypeFlagFn>(
        CheckMonsterTypeFlagRva);
    GetMonsterUMods = At<GetMonsterUModsFn>(GetMonsterUModsRva);
    GetRightSkill = At<GetRightSkillFn>(GetRightSkillRva);
    StatesCheckState = At<StatesCheckStateFn>(StatesCheckStateRva);
    GetMinionOwner = At<GetMinionOwnerFn>(GetMinionOwnerRva);
    AssignSkill = At<AssignSkillFn>(AssignSkillRva);
}

bool InstallAiHooks() noexcept {
    if (!Settings.ai.enabled) return true;
    if (!Context->InstallInlineHook(
            AiFunction03Rva,
            AiFunction03Expected.data(),
            static_cast<std::uint32_t>(AiFunction03Expected.size()),
            HookAiFunction03,
            &OriginalAiFunction03)) {
        return false;
    }
    AnyHookInstalled.store(true, std::memory_order_release);
    if (!Context->InstallInlineHook(
            DistanceRva,
            DistanceExpected.data(),
            static_cast<std::uint32_t>(DistanceExpected.size()),
            HookDistance,
            &OriginalDistance)) {
        return false;
    }
    if (!Context->InstallInlineHook(
            SetVelocityRva,
            SetVelocityExpected.data(),
            static_cast<std::uint32_t>(SetVelocityExpected.size()),
            HookSetVelocity,
            &OriginalSetVelocity)) {
        return false;
    }
    return Context->InstallInlineHook(
        WalkToOwnerRva,
        WalkToOwnerExpected.data(),
        static_cast<std::uint32_t>(WalkToOwnerExpected.size()),
        HookWalkToOwner,
        &OriginalWalkToOwner);
}

bool InstallReviveHooks() noexcept {
    if (NeedsReviveTargetValidator(Settings.revive)) {
        if (!Context->InstallInlineHook(
                ValidateReviveTargetRva,
                ValidateReviveTargetExpected.data(),
                static_cast<std::uint32_t>(
                    ValidateReviveTargetExpected.size()),
                HookValidateReviveTarget,
                &OriginalValidateReviveTarget)) {
            return false;
        }
        AnyHookInstalled.store(true, std::memory_order_release);
    }
    if (Settings.revive.allowHighRankMonsters) {
        if (!Context->InstallInlineHook(
                ClientReviveSelectorRva,
                ClientReviveSelectorExpected.data(),
                static_cast<std::uint32_t>(
                    ClientReviveSelectorExpected.size()),
                HookClientValidateReviveTarget,
                &OriginalClientValidateReviveTarget)) {
            return false;
        }
        AnyHookInstalled.store(true, std::memory_order_release);
        if (!InstallClientReviveAiGateRedirect()) {
            return false;
        }
        AnyHookInstalled.store(true, std::memory_order_release);
    }
    if (!Settings.revive.preserveNativeAuras) return true;
    if (!Context->InstallInlineHook(
            SrvDo58ReviveRva,
            SrvDo58ReviveExpected.data(),
            static_cast<std::uint32_t>(SrvDo58ReviveExpected.size()),
            HookSrvDo58Revive,
            &OriginalSrvDo58Revive)) {
        return false;
    }
    AnyHookInstalled.store(true, std::memory_order_release);
    return true;
}

bool InstallHooks() noexcept {
    if (!InstallAiHooks()) return false;
    if (Settings.ai.enabled) {
        AnyHookInstalled.store(true, std::memory_order_release);
    }
    if (!InstallReviveHooks()) return false;
    if (Settings.revive.allowHighRankMonsters
            || Settings.revive.preserveNativeAuras) {
        AnyHookInstalled.store(true, std::memory_order_release);
    }
    return true;
}

D2RL::ConsoleCommandResult Status(
        D2R::Game::Client*,
        const D2RL::ConsoleCommandContext* command,
        void*) noexcept {
    if (!command || !command->plugin) {
        return D2RL::ConsoleCommandResult::Failed;
    }
    char message[1600]{};
    std::snprintf(
        message,
        sizeof(message),
        "Revive Overhaul %s: enabled=%d, AI=%d, high-rank=%d, "
        "native auras=%d, ticks=%llu, scatter fixes=%llu, catch-ups=%llu, "
        "follow adjustments=%llu, velocity adjustments=%llu, "
        "scripted provider probes=%llu, scripted tactical calls=%llu, "
        "scripted handled=%llu, scripted delegates=%llu, "
        "scripted unavailable=%llu, scripted errors=%llu, "
        "server admissions=%llu, client gate calls=%llu, "
        "client high-rank candidates=%llu, client selections=%llu, "
        "selector high-rank calls=%llu, selector accepts=%llu, "
        "selector rejects=%llu, callback route ready=%d, "
        "callback route applications=%llu, callback route failures=%llu, "
        "callback route revision=%llu, aura captures=%llu, "
        "aura reactivations=%llu, aura conflicts=%llu, config=%s",
        Version,
        Settings.enabled ? 1 : 0,
        Settings.ai.enabled ? 1 : 0,
        Settings.revive.allowHighRankMonsters ? 1 : 0,
        Settings.revive.preserveNativeAuras ? 1 : 0,
        static_cast<unsigned long long>(ReviveTicks.load()),
        static_cast<unsigned long long>(ScatterSuppressions.load()),
        static_cast<unsigned long long>(CatchUpsForced.load()),
        static_cast<unsigned long long>(FollowAdjustments.load()),
        static_cast<unsigned long long>(VelocityAdjustments.load()),
        static_cast<unsigned long long>(ScriptedAiProviderProbes.load()),
        static_cast<unsigned long long>(ScriptedAiTacticalCalls.load()),
        static_cast<unsigned long long>(
            ScriptedAiTacticalHandled.load()),
        static_cast<unsigned long long>(ScriptedAiTacticalDelegates.load()),
        static_cast<unsigned long long>(ScriptedAiTacticalUnavailable.load()),
        static_cast<unsigned long long>(ScriptedAiTacticalErrors.load()),
        static_cast<unsigned long long>(HighRankAdmissions.load()),
        static_cast<unsigned long long>(ClientGateCalls.load()),
        static_cast<unsigned long long>(ClientHighRankCandidates.load()),
        static_cast<unsigned long long>(ClientHighRankSelections.load()),
        static_cast<unsigned long long>(ClientSelectorHighRankCalls.load()),
        static_cast<unsigned long long>(ClientSelectorHighRankAccepts.load()),
        static_cast<unsigned long long>(ClientSelectorHighRankRejects.load()),
        CallbackRouteReady.load(std::memory_order_acquire) ? 1 : 0,
        static_cast<unsigned long long>(CallbackRouteApplications.load()),
        static_cast<unsigned long long>(CallbackRouteFailures.load()),
        static_cast<unsigned long long>(CallbackRouteRevision.load()),
        static_cast<unsigned long long>(NativeAuraCaptures.load()),
        static_cast<unsigned long long>(NativeAuraReactivations.load()),
        static_cast<unsigned long long>(NativeAuraConflicts.load()),
        LoadedConfigPath.c_str());
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}

void RegisterStatusCommand() noexcept {
    if (!Context->RegisterConsoleCommand(
            "revive-overhaul",
            Status,
            "Show Revive Overhaul configuration and runtime counters.")) {
        Context->LogWarn(
            "ReviveOverhaul: optional status command was not registered.");
    }
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
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(context->exeBase);
    Operational.store(false, std::memory_order_release);
    AnyHookInstalled.store(false, std::memory_order_release);
    CallbackRouteReady.store(false, std::memory_order_release);
    ScriptedAiProbeCooldown = 0U;
    ScriptedAiAbiWarningLogged.store(false, std::memory_order_release);
    ScriptedAiProviderLogged.store(false, std::memory_order_release);
    DataTables = nullptr;
    Lifecycle = nullptr;
    DataTablesListener = D2RL::Lifecycle::InvalidHandle;
    if (!Base || !LoadConfig()) return false;
    RegisterStatusCommand();

    if (!Settings.enabled) {
        char message[160]{};
        std::snprintf(
            message,
            sizeof(message),
            "Revive Overhaul %s by RuffnecKk loaded disabled; no hooks installed.",
            Version);
        context->LogInfo(message);
        return true;
    }
    const auto* runtimeBuild = D2RL::GetBuildName(context);
    const auto* runtimeLabel = runtimeBuild ? runtimeBuild : "<unknown>";
    if (!Settings.ai.enabled
            && !Settings.revive.allowHighRankMonsters) {
        char message[128]{};
        std::snprintf(
            message,
            sizeof(message),
            "Revive Overhaul %s has no enabled feature; no hooks installed.",
            Version);
        context->LogInfo(message);
        return true;
    }
    if (!ValidateRuntime()) {
        char message[176]{};
        std::snprintf(
            message,
            sizeof(message),
            "ReviveOverhaul: native fingerprint validation failed for reported D2R build %s; plugin refused.",
            runtimeLabel);
        context->LogError(message);
        return false;
    }
    if (!RegisterDataTableRoute()) {
        context->LogError(
            "ReviveOverhaul: automatic Revive callback route registration failed.");
        return false;
    }
    ResolveNativeFunctions();
    if (!InstallHooks()) {
        Operational.store(false, std::memory_order_release);
        if (AnyHookInstalled.load(std::memory_order_acquire)) {
            context->LogError(
                "ReviveOverhaul: partial hook commit is pass-through and inert; keep the DLL loaded and cold-restart after resolving the conflict.");
            return true;
        }
        context->LogError(
            "ReviveOverhaul: native hook installation failed before mutation.");
        return false;
    }
    Operational.store(true, std::memory_order_release);
    char message[192]{};
    std::snprintf(
        message,
        sizeof(message),
        Settings.diagnostics
            ? "Revive Overhaul %s by RuffnecKk active for D2R build %s with diagnostics enabled."
            : "Revive Overhaul %s by RuffnecKk active for D2R build %s.",
        Version,
        runtimeLabel);
    context->LogInfo(message);
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    Operational.store(false, std::memory_order_release);
    CallbackRouteReady.store(false, std::memory_order_release);
    ClientReviveAiGateRelay = nullptr;
    ScriptedAiProbeCooldown = 0U;
    ScriptedAiProviderLogged.store(false, std::memory_order_release);
    DataTables = nullptr;
    Lifecycle = nullptr;
    DataTablesListener = D2RL::Lifecycle::InvalidHandle;
    Context = nullptr;
}
