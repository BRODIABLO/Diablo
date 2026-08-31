#define NOMINMAX
#include <D2RLPlugin/api.h>

#include "revive_overhaul_policy.hpp"

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
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace ruffneckk::revive_overhaul;

constexpr wchar_t ConfigFileName[] = L"ruffneckk-revive-overhaul.toml";
constexpr wchar_t LegacyConfigFileName[] = L"ReviveOverhaul.toml";
constexpr char Version[] = "2.1.1";

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
catch_up_distance = 12

# Native follow distance used after catch-up. It must remain lower than
# catch_up_distance. Accepted range: 1 to 19.
follow_distance = 8

# Native catch-up velocity bonus. Accepted range: 0 to 255.
velocity_bonus = 40

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

constexpr std::uintptr_t ValidateReviveTargetRva = 0x55A510;
constexpr std::uintptr_t SrvDo58ReviveRva = 0x55E7E0;
constexpr std::uintptr_t ClientReviveSelectorRva = 0x096600;
constexpr std::uintptr_t ClientReviveAiGateRva = 0x096635;
constexpr std::uintptr_t ClientReviveAiGateReturnRva = 0x09664D;
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
using ValidateReviveTargetFn = std::int32_t(__fastcall*)(
    void*, void*, void*) noexcept;
using SrvDo58ReviveFn = std::int32_t(__fastcall*)(
    void*, void*, std::int32_t, std::int32_t) noexcept;
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
CanUnitSwitchAiFn OriginalCanUnitSwitchAi{};
ValidateReviveTargetFn OriginalValidateReviveTarget{};
SrvDo58ReviveFn OriginalSrvDo58Revive{};
GetUnitTypeFn GetUnitType{};
GetMonStats2Fn GetMonStats2{};
IsUnitDeadFn IsUnitDead{};
IsCorpseConsumableFn IsCorpseConsumable{};
CanUnitSwitchAiFn CanUnitSwitchAi{};
CheckMonsterTypeFlagFn CheckMonsterTypeFlag{};
GetMonsterUModsFn GetMonsterUMods{};
GetRightSkillFn GetRightSkill{};
StatesCheckStateFn StatesCheckState{};
AssignSkillFn AssignSkill{};

thread_local std::uint32_t ReviveAiDepth{};
std::atomic_uint64_t ReviveTicks{};
std::atomic_uint64_t ScatterSuppressions{};
std::atomic_uint64_t CatchUpsForced{};
std::atomic_uint64_t FollowAdjustments{};
std::atomic_uint64_t VelocityAdjustments{};
std::atomic_uint64_t HighRankAdmissions{};
std::atomic_uint64_t ClientHighRankSelections{};
std::atomic_uint64_t NativeAuraCaptures{};
std::atomic_uint64_t NativeAuraReactivations{};
std::atomic_uint64_t NativeAuraConflicts{};

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
    return OriginalCanUnitSwitchAi
        && OriginalCanUnitSwitchAi(target, true, false, true, true);
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
    const ReviveAiScope scope(revived);
    return OriginalAiFunction03(game, monster, tickParam);
}

__declspec(noinline) bool __fastcall HookCanUnitSwitchAi(
        void* monster,
        bool checkState143,
        bool checkUnique,
        bool checkBoss,
        bool checkSwitchAi) noexcept {
    const bool clientReviveHighRank =
        Operational.load(std::memory_order_acquire)
        && Settings.revive.allowHighRankMonsters
        && checkUnique
        && IsExpectedReturnAddress(
            _ReturnAddress(), ClientReviveAiGateReturnRva)
        && CheckMonsterTypeFlag(
            monster, ChampionUniqueSuperUniqueMask);
    const auto result = OriginalCanUnitSwitchAi(
        monster,
        checkState143,
        clientReviveHighRank ? false : checkUnique,
        checkBoss,
        checkSwitchAi);
    if (clientReviveHighRank && result) ++ClientHighRankSelections;
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
                CanUnitSwitchAiRva,
                CanUnitSwitchAiExpected.data(),
                static_cast<std::uint32_t>(CanUnitSwitchAiExpected.size()),
                HookCanUnitSwitchAi,
                &OriginalCanUnitSwitchAi)) {
            return false;
        }
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
    char message[1024]{};
    std::snprintf(
        message,
        sizeof(message),
        "Revive Overhaul %s: enabled=%d, AI=%d, high-rank=%d, "
        "native auras=%d, ticks=%llu, scatter fixes=%llu, catch-ups=%llu, "
        "follow adjustments=%llu, velocity adjustments=%llu, "
        "server admissions=%llu, client selections=%llu, aura captures=%llu, "
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
        static_cast<unsigned long long>(HighRankAdmissions.load()),
        static_cast<unsigned long long>(ClientHighRankSelections.load()),
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
    Context = nullptr;
}
