#include <Windows.h>
#include <D2RLPlugin/api.h>

#include "burn_damage_fix_policy.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

extern "C" void BurnDamageFixProductionMidHook();
extern "C" void* gBurnDamageFixProductionContinuation{};

namespace {
using namespace ruffneckk::burn_damage_fix;

constexpr wchar_t ConfigFileName[] = L"burn-damage-fix.toml";
constexpr std::uintptr_t GenericBurnProductionRva = 0x44CB32;
constexpr std::uintptr_t GenericBurnProductionContinuationRva = 0x44CB38;
constexpr std::uintptr_t ApplyBurnDamageRva = 0x451380;
constexpr std::uintptr_t ApplyResistancesAndAbsorbRva = 0x4523E0;
constexpr std::uintptr_t GetDifficultyRecordRva = 0x300830;
constexpr std::uintptr_t GetUnitStatRva = 0x2F5020;
constexpr std::uintptr_t CheckStateRva = 0x3351B0;
constexpr std::uintptr_t GetUnitTypeRva = 0x34B9D0;
constexpr std::uintptr_t IsHirelingRva = 0x3AF240;
constexpr std::uintptr_t GetGameFromUnitRva = 0x48FF00;
constexpr std::size_t GameDifficultyOffset = 0x104;
constexpr std::size_t GameDataSetOffset = 0x106;
constexpr std::int32_t MonsterUnitType = 1;
constexpr std::int32_t BurningState = 115;
constexpr std::int32_t FireLengthStat = 315;
constexpr std::int32_t BurningMinStat = 316;
constexpr std::int32_t BurningMaxStat = 317;
constexpr std::int32_t PassiveFireMasteryStat = 329;
constexpr std::uint32_t GenericBurnPatchSize = 6;
constexpr std::size_t RelayBytes = 16;
constexpr char FireTypeName[] = "fire";

constexpr std::array<std::uint8_t, 10> GenericBurnProductionExpected{
    0x81, 0xC3, 0x3C, 0x01, 0x00, 0x00, 0x41, 0x0F, 0x48, 0xDE,
};
constexpr std::array<std::uint8_t, 23> ApplyBurnDamageExpected{
    0x45, 0x85, 0xC9, 0x0F, 0x8E, 0xE6, 0x01, 0x00, 0x00,
    0x48, 0x89, 0x6C, 0x24, 0x20, 0x56, 0x41, 0x56, 0x41,
    0x57, 0x48, 0x83, 0xEC, 0x40,
};
constexpr std::array<std::uint8_t, 32> GetDifficultyRecordExpected{
    0x40,0x53,0x56,0x57,0x48,0x83,0xEC,0x30,0x0F,0xB6,0xC1,0x8B,0xF2,0x48,
    0x89,0x44,0x24,0x60,0x48,0x83,0xF8,0x04,0x72,0x19,0x48,0x8D,0x44,0x24,
    0x60,0x48,0x8D,0x4C,
};
constexpr std::array<std::uint8_t, 32> GetUnitStatExpected{
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x6C,0x24,0x18,0x48,0x89,0x74,0x24,
    0x20,0x57,0x48,0x83,0xEC,0x20,0x41,0x0F,0xB7,0xE8,0x8B,0xFA,0x48,0x8B,
    0xD9,0x48,0x85,0xC9,
};
constexpr std::array<std::uint8_t, 32> CheckStateExpected{
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xEC,
    0x20,0x8B,0xDA,0x48,0x8B,0xF1,0xE8,0x07,0x68,0x01,0x00,0x85,0xC0,0x74,
    0x0E,0x83,0xE8,0x01,
};
constexpr std::array<std::uint8_t, 28> GetUnitTypeExpected{
    0x48,0x83,0xEC,0x28,0x48,0x85,0xC9,0x75,0x1D,0x88,0x4C,0x24,0x30,0x48,
    0x8D,0x4C,0x24,0x30,0xE8,0x39,0x9E,0xFF,0xFF,0x84,0xC0,0x74,0x01,0xCC,
};
constexpr std::array<std::uint8_t, 24> IsHirelingExpected{
    0x40,0x56,0x48,0x83,0xEC,0x20,0x48,0x8B,0xF1,0xE8,0x82,0xC7,0xF9,0xFF,
    0x83,0xF8,0x01,0x75,0x5D,0x48,0x89,0x5C,0x24,0x30,
};
constexpr std::array<std::uint8_t, 32> GetGameFromUnitExpected{
    0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0x48,0x85,0xC9,0x75,0x20,
    0x88,0x4C,0x24,0x30,0x48,0x8D,0x4C,0x24,0x30,0xE8,0x44,0xD9,0xFF,0xFF,
    0x84,0xC0,0x74,0x01,
};

struct DamagePayloadView {
    std::array<std::byte, 0x20> reserved00{};
    std::int32_t fireDamage{};
    std::int32_t burnDamage{};
    std::array<std::byte, 0x118> reserved28{};
};

struct DamageInfoView {
    void* game{};
    void* difficultyRecord{};
    void* attacker{};
    void* defender{};
    std::int32_t attackerIsMonster{};
    std::int32_t defenderIsMonster{};
    DamagePayloadView* damage{};
    std::array<std::int32_t, 32> reductions{};
};

struct ResistanceRecordView {
    std::int32_t* value{};
    std::int32_t resistanceStat{};
    std::int32_t maximumResistanceStat{};
    std::int32_t field10{};
    std::int32_t field14{};
    std::int32_t field18{};
    std::int32_t field1C{};
    std::int32_t damageReductionIndex{};
    std::int32_t attackerGate{};
    std::int32_t flags28{};
    std::int32_t flags2C{};
    const char* typeName{};
    std::uint8_t logFlag{};
    std::array<std::byte, 7> reserved39{};
};

static_assert(offsetof(DamagePayloadView, fireDamage) == 0x20);
static_assert(offsetof(DamagePayloadView, burnDamage) == 0x24);
static_assert(sizeof(DamagePayloadView) == 0x140);
static_assert(offsetof(DamageInfoView, game) == 0x00);
static_assert(offsetof(DamageInfoView, difficultyRecord) == 0x08);
static_assert(offsetof(DamageInfoView, attacker) == 0x10);
static_assert(offsetof(DamageInfoView, defender) == 0x18);
static_assert(offsetof(DamageInfoView, attackerIsMonster) == 0x20);
static_assert(offsetof(DamageInfoView, defenderIsMonster) == 0x24);
static_assert(offsetof(DamageInfoView, damage) == 0x28);
static_assert(offsetof(DamageInfoView, reductions) == 0x30);
static_assert(offsetof(ResistanceRecordView, resistanceStat) == 0x08);
static_assert(offsetof(ResistanceRecordView, maximumResistanceStat) == 0x0C);
static_assert(offsetof(ResistanceRecordView, field10) == 0x10);
static_assert(offsetof(ResistanceRecordView, damageReductionIndex) == 0x20);
static_assert(offsetof(ResistanceRecordView, typeName) == 0x30);
static_assert(offsetof(ResistanceRecordView, logFlag) == 0x38);
static_assert(sizeof(ResistanceRecordView) == 0x40);

using ApplyBurnDamageFn = void(__fastcall*)(
    void*, void*, std::int32_t, std::int32_t) noexcept;
using ApplyResistancesAndAbsorbFn = std::int32_t(__fastcall*)(
    DamageInfoView*, ResistanceRecordView*, std::int32_t) noexcept;
using GetDifficultyRecordFn = void*(__fastcall*)(
    std::uint32_t, std::int32_t) noexcept;
using GetUnitStatFn = std::int32_t(__fastcall*)(
    void*, std::int32_t, std::int32_t) noexcept;
using CheckStateFn = std::int32_t(__fastcall*)(
    void*, std::int32_t) noexcept;
using GetUnitTypeFn = std::int32_t(__fastcall*)(void*) noexcept;
using IsHirelingFn = std::int32_t(__fastcall*)(void*) noexcept;
using GetGameFromUnitFn = void*(__fastcall*)(void*) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
std::string LoadedConfigPath{"embedded defaults"};
std::string RuntimeBuild{"unknown"};
void* RelayPage{};
ApplyBurnDamageFn OriginalApplyBurnDamage{};
ApplyResistancesAndAbsorbFn ApplyResistancesAndAbsorb{};
GetDifficultyRecordFn GetDifficultyRecord{};
GetUnitStatFn GetUnitStat{};
CheckStateFn CheckState{};
GetUnitTypeFn GetUnitType{};
IsHirelingFn IsHireling{};
GetGameFromUnitFn GetGameFromUnit{};
std::atomic_bool Operational{};
std::atomic<std::uint64_t> GenericProductionCalls{};
std::atomic<std::uint64_t> GenericProductionRolls{};
std::atomic<std::uint64_t> ResolvedBurnApplications{};
std::atomic<std::uint64_t> BurningStateActiveWitnesses{};
std::atomic<std::uint64_t> BurningStateMissingWitnesses{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "burn-damage-fix",
    .name = "Burn Damage Fix",
    .version = "2.0.0",
    .author = "RuffnecKk",
    .description = "Restores Burn damage and makes it respect Fire Resistance.",
    .flags = D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks,
};

template <typename T>
auto At(std::uintptr_t rva) noexcept -> T {
    return reinterpret_cast<T>(Base + rva);
}

template <std::size_t Size>
auto Matches(
        std::uintptr_t rva,
        const std::array<std::uint8_t, Size>& expected) noexcept -> bool {
    return Base != nullptr
        && std::memcmp(Base + rva, expected.data(), expected.size()) == 0;
}

auto IsExecutableAddress(const void* address) noexcept -> bool {
    MEMORY_BASIC_INFORMATION info{};
    if (!address || VirtualQuery(address, &info, sizeof(info)) != sizeof(info)) {
        return false;
    }
    if (info.State != MEM_COMMIT || (info.Protect & PAGE_GUARD) != 0
            || (info.Protect & PAGE_NOACCESS) != 0) {
        return false;
    }
    const auto protection = info.Protect & 0xFFU;
    return protection == PAGE_EXECUTE
        || protection == PAGE_EXECUTE_READ
        || protection == PAGE_EXECUTE_READWRITE
        || protection == PAGE_EXECUTE_WRITECOPY;
}

auto ConfigCandidates() -> std::vector<std::filesystem::path> {
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
        activeModConfigDirectory,
        scopeConfigDirectory,
        globalConfigDirectory,
        ConfigFileName);
}

auto LoadConfig() noexcept -> bool {
    Settings = {};
    LoadedConfigPath = "embedded defaults";
    for (const auto& path : ConfigCandidates()) {
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
            return true;
        } catch (const std::exception& exception) {
            const auto message = std::string("BurnDamageFix: invalid ")
                + path.string() + " (" + exception.what() + ").";
            Context->LogError(message.c_str());
            return false;
        }
    }
    Context->LogWarn(
        "BurnDamageFix: no TOML was found; embedded defaults are active.");
    return true;
}

auto ValidateRuntime() noexcept -> bool {
    struct Check {
        bool matched;
        const char* label;
    };
    const Check checks[]{
        {Matches(GenericBurnProductionRva, GenericBurnProductionExpected),
         "generic Burn production seam"},
        {Matches(ApplyBurnDamageRva, ApplyBurnDamageExpected),
         "Burn application entry"},
        {Matches(GetDifficultyRecordRva, GetDifficultyRecordExpected),
         "difficulty record resolver"},
        {Matches(GetUnitStatRva, GetUnitStatExpected), "unit stat resolver"},
        {Matches(GetUnitTypeRva, GetUnitTypeExpected), "unit type resolver"},
        {Matches(IsHirelingRva, IsHirelingExpected), "hireling classifier"},
        {Matches(GetGameFromUnitRva, GetGameFromUnitExpected),
         "unit game resolver"},
    };
    for (const auto& check : checks) {
        if (check.matched) continue;
        char message[256]{};
        std::snprintf(
            message, sizeof(message),
            "BurnDamageFix: %s signature mismatch or hook collision; plugin refused.",
            check.label);
        Context->LogError(message);
        return false;
    }

    if (Settings.diagnostics && !Matches(CheckStateRva, CheckStateExpected)) {
        Context->LogError(
            "BurnDamageFix: burning-state witness signature mismatch; plugin refused.");
        return false;
    }

    // This entry is intentionally composable. MonsterDisplay may already own it,
    // or may hook it later. Calling the live address preserves either load order.
    if (!IsExecutableAddress(Base + ApplyResistancesAndAbsorbRva)) {
        Context->LogError(
            "BurnDamageFix: live Fire Resistance resolver is not executable; plugin refused.");
        return false;
    }
    return true;
}

auto AllocateNear(void* hint, std::size_t size) noexcept -> void* {
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const auto granularity = static_cast<std::uintptr_t>(
        systemInfo.dwAllocationGranularity);
    const auto aligned = reinterpret_cast<std::uintptr_t>(hint)
        & ~(granularity - 1U);
    for (std::uintptr_t delta = granularity;
            delta < 0x70000000ULL; delta += granularity) {
        if (aligned > std::numeric_limits<std::uintptr_t>::max() - delta) break;
        const auto candidate = aligned + delta;
        if (!CanEncodeRel32(
                reinterpret_cast<std::uintptr_t>(hint), candidate)) {
            break;
        }
        if (auto* memory = VirtualAlloc(
                reinterpret_cast<void*>(candidate), size,
                MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)) {
            return memory;
        }
    }
    return nullptr;
}

auto WriteAbsoluteJump(std::uint8_t* destination, const void* target) noexcept
        -> bool {
    if (!destination || !target) return false;
    destination[0] = 0xFF;
    destination[1] = 0x25;
    destination[2] = destination[3] = destination[4] = destination[5] = 0;
    const auto address = reinterpret_cast<std::uint64_t>(target);
    std::memcpy(destination + 6, &address, sizeof(address));
    return true;
}

auto InstallProductionRelay() noexcept -> bool {
    if (!Settings.normalizeGenericBurn) return true;
    RelayPage = AllocateNear(Base + GenericBurnProductionRva, RelayBytes);
    if (!RelayPage) {
        Context->LogError(
            "BurnDamageFix: no relay page was available within rel32 reach.");
        return false;
    }
    auto* relay = static_cast<std::uint8_t*>(RelayPage);
    if (!WriteAbsoluteJump(
            relay,
            reinterpret_cast<const void*>(&BurnDamageFixProductionMidHook))) {
        return false;
    }
    DWORD previousProtection{};
    if (!VirtualProtect(
            relay, RelayBytes, PAGE_EXECUTE_READ, &previousProtection)) {
        Context->LogError(
            "BurnDamageFix: relay page protection could not be finalized.");
        return false;
    }
    FlushInstructionCache(GetCurrentProcess(), relay, RelayBytes);
    const auto relayAddress = reinterpret_cast<std::uintptr_t>(relay);
    const auto baseAddress = reinterpret_cast<std::uintptr_t>(Base);
    if (relayAddress < baseAddress
            || !CanEncodeRel32(
                baseAddress + GenericBurnProductionRva, relayAddress)) {
        Context->LogError(
            "BurnDamageFix: relay displacement validation failed.");
        return false;
    }
    gBurnDamageFixProductionContinuation =
        Base + GenericBurnProductionContinuationRva;
    if (!Context->PatchJmpRel32(
            GenericBurnProductionRva,
            GenericBurnProductionExpected.data(),
            GenericBurnPatchSize,
            relayAddress - baseAddress,
            GenericBurnPatchSize)) {
        Context->LogError(
            "BurnDamageFix: generic Burn production seam is already owned; plugin refused.");
        return false;
    }
    return true;
}

auto IsNonHirelingMonster(void* unit) noexcept -> std::int32_t {
    if (!unit || GetUnitType(unit) != MonsterUnitType) return 0;
    return IsHireling(unit) == 0 ? 1 : 0;
}

auto TryResolveBurn(
        void* attacker,
        void* defender,
        std::int32_t burnDamage,
        std::int32_t& resolvedBurn) noexcept -> bool {
    resolvedBurn = burnDamage;
    if (!attacker || !defender || burnDamage <= 0) return false;

    bool resolved{};
    __try {
        auto* game = GetGameFromUnit(attacker);
        if (!game) return false;
        const auto* gameBytes = static_cast<const std::uint8_t*>(game);
        auto* difficultyRecord = GetDifficultyRecord(
            gameBytes[GameDataSetOffset], gameBytes[GameDifficultyOffset]);
        if (!difficultyRecord) return false;

        DamagePayloadView damage{};
        damage.burnDamage = burnDamage;

        DamageInfoView damageInfo{};
        damageInfo.game = game;
        damageInfo.difficultyRecord = difficultyRecord;
        damageInfo.attacker = attacker;
        damageInfo.defender = defender;
        damageInfo.attackerIsMonster = IsNonHirelingMonster(attacker);
        damageInfo.defenderIsMonster = IsNonHirelingMonster(defender);
        damageInfo.damage = &damage;

        ResistanceRecordView fireResistance{};
        fireResistance.value = &damage.burnDamage;
        fireResistance.resistanceStat = 39;
        fireResistance.maximumResistanceStat = 40;
        fireResistance.field18 = 4;
        fireResistance.damageReductionIndex = 2;
        fireResistance.flags28 = 1;
        fireResistance.typeName = FireTypeName;
        fireResistance.logFlag = 8;

        // dontAbsorb=1 preserves resistance, caps, immunity and Fire pierce while
        // excluding Magic Damage Reduction and Fire Absorb.
        ApplyResistancesAndAbsorb(&damageInfo, &fireResistance, 1);
        resolvedBurn = damage.burnDamage;
        resolved = true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        resolvedBurn = burnDamage;
        resolved = false;
    }
    return resolved;
}

void HookApplyBurnDamage(
        void* attacker,
        void* defender,
        std::int32_t burnDamage,
        std::int32_t burnLength) noexcept {
    auto resolvedBurn = burnDamage;
    if (ShouldResolveBurn(Settings, burnDamage, burnLength)
            && TryResolveBurn(attacker, defender, burnDamage, resolvedBurn)
            && Settings.diagnostics) {
        ResolvedBurnApplications.fetch_add(1, std::memory_order_relaxed);
    }
    OriginalApplyBurnDamage(attacker, defender, resolvedBurn, burnLength);

    if (!ShouldWitnessBurningState(Settings, resolvedBurn, burnLength)
            || !defender || !CheckState) {
        return;
    }
    bool stateActive{};
    __try {
        stateActive = CheckState(defender, BurningState) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        stateActive = false;
    }
    (stateActive
        ? BurningStateActiveWitnesses
        : BurningStateMissingWitnesses).fetch_add(1, std::memory_order_relaxed);
}

auto Status(
        D2R::Game::Client*,
        const D2RL::ConsoleCommandContext* command,
        void*) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) {
        return D2RL::ConsoleCommandResult::Failed;
    }
    char message[1024]{};
    std::snprintf(
        message,
        sizeof(message),
        "Burn Damage Fix 2.0.0: active=%s; build=%s; generic=%s; resistance=%s; diagnostics=%s; production=%llu/%llu; resolved=%llu; burning-state=%llu/%llu active/missing; config=%s.",
        Operational.load(std::memory_order_acquire) ? "true" : "false",
        RuntimeBuild.c_str(),
        Settings.normalizeGenericBurn ? "on" : "off",
        Settings.applyFireResistance ? "on" : "off",
        Settings.diagnostics ? "on" : "off",
        static_cast<unsigned long long>(
            GenericProductionCalls.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            GenericProductionRolls.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            ResolvedBurnApplications.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            BurningStateActiveWitnesses.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            BurningStateMissingWitnesses.load(std::memory_order_relaxed)),
        LoadedConfigPath.c_str());
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}

void ResetState() noexcept {
    Operational.store(false, std::memory_order_release);
    GenericProductionCalls.store(0, std::memory_order_relaxed);
    GenericProductionRolls.store(0, std::memory_order_relaxed);
    ResolvedBurnApplications.store(0, std::memory_order_relaxed);
    BurningStateActiveWitnesses.store(0, std::memory_order_relaxed);
    BurningStateMissingWitnesses.store(0, std::memory_order_relaxed);
    RuntimeBuild = "unknown";
    LoadedConfigPath = "embedded defaults";
    RelayPage = nullptr;
}

} // namespace

extern "C" std::int32_t __fastcall BurnDamageFixNormalizeGeneric(
        void* attacker,
        std::int32_t scaledExistingBurn,
        std::uint32_t advancedRandom) noexcept {
    if (!Operational.load(std::memory_order_acquire)
            || !Settings.normalizeGenericBurn || !GetUnitStat || !attacker) {
        return NormalizeGenericNumerator(
            scaledExistingBurn, 0, 0, 0, advancedRandom);
    }

    std::int32_t burningMin{};
    std::int32_t burningMax{};
    std::int32_t fireMastery{};
    __try {
        burningMax = GetUnitStat(attacker, BurningMaxStat, 0);
        if (burningMax > 0) {
            burningMin = GetUnitStat(attacker, BurningMinStat, 0);
            if (burningMin > 0) {
                fireMastery = GetUnitStat(attacker, PassiveFireMasteryStat, 0);
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        burningMin = burningMax = fireMastery = 0;
    }

    if (Settings.diagnostics) {
        GenericProductionCalls.fetch_add(1, std::memory_order_relaxed);
        if (burningMin > 0 && burningMax > 0) {
            GenericProductionRolls.fetch_add(1, std::memory_order_relaxed);
        }
    }
    return NormalizeGenericNumerator(
        scaledExistingBurn,
        burningMin,
        burningMax,
        fireMastery,
        advancedRandom);
}

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
    ResetState();
    if (!Base || !LoadConfig()) return false;

    if (!context->RegisterConsoleCommand(
            "burn-damage-fix",
            Status,
            "Show Burn Damage Fix settings and diagnostic counters.")) {
        context->LogWarn(
            "BurnDamageFix: optional status command was not registered.");
    }
    if (!Settings.enabled) {
        context->LogInfo(
            "Burn Damage Fix 2.0.0 by RuffnecKk loaded disabled; no hook was installed.");
        return true;
    }

    const auto* runtimeBuild = D2RL::GetBuildName(context);
    if (!runtimeBuild || !IsSupportedBuild(runtimeBuild)) {
        context->LogError(
            "BurnDamageFix: only governed D2R builds 92777 and 93847 are supported.");
        return false;
    }
    RuntimeBuild = runtimeBuild;
    if (!ValidateRuntime()) return false;

    ApplyResistancesAndAbsorb = At<ApplyResistancesAndAbsorbFn>(
        ApplyResistancesAndAbsorbRva);
    GetDifficultyRecord = At<GetDifficultyRecordFn>(GetDifficultyRecordRva);
    GetUnitStat = At<GetUnitStatFn>(GetUnitStatRva);
    CheckState = At<CheckStateFn>(CheckStateRva);
    GetUnitType = At<GetUnitTypeFn>(GetUnitTypeRva);
    IsHireling = At<IsHirelingFn>(IsHirelingRva);
    GetGameFromUnit = At<GetGameFromUnitFn>(GetGameFromUnitRva);

    if (Settings.applyFireResistance
            && !context->InstallInlineHook(
                ApplyBurnDamageRva,
                ApplyBurnDamageExpected.data(),
                static_cast<std::uint32_t>(ApplyBurnDamageExpected.size()),
                HookApplyBurnDamage,
                &OriginalApplyBurnDamage)) {
        context->LogError(
            "BurnDamageFix: Burn application entry is already owned; plugin refused.");
        return false;
    }
    if (!InstallProductionRelay()) return false;
    Operational.store(true, std::memory_order_release);

    char message[768]{};
    std::snprintf(
        message,
        sizeof(message),
        "Burn Damage Fix 2.0.0 by RuffnecKk active for D2R %s; generic=%s; resistance=%s; installation=%s; TOML=%s.",
        runtimeBuild,
        Settings.normalizeGenericBurn ? "enabled" : "disabled",
        Settings.applyFireResistance ? "enabled" : "disabled",
        context->loadScope == D2RL::LoadScope::Mod ? "mod-local" : "global",
        LoadedConfigPath.c_str());
    context->LogInfo(message);
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    Operational.store(false, std::memory_order_release);
}
