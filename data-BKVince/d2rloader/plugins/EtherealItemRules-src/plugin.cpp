#define NOMINMAX
#include <D2RLPlugin/api.h>
#include <nlohmann/json.hpp>

#include "ethereal_policy.hpp"

#include <Windows.h>
#include <intrin.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using ruffneckk::ethereal::Config;
using ruffneckk::ethereal::FindItemTypeId;
using ruffneckk::ethereal::HasDirectRulePatches;
using ruffneckk::ethereal::ItemTypeRecordStride;
using ruffneckk::ethereal::MaxExcludedItemTypes;
using ruffneckk::ethereal::ParseConfig;
using ruffneckk::ethereal::PatchChance;
using ruffneckk::ethereal::PatchIndestructibleItems;
using ruffneckk::ethereal::PatchSetItems;

constexpr std::uint32_t SupportedBuild = 92777;
constexpr wchar_t ConfigFileName[] = L"EtherealItemRules.json";

constexpr std::uintptr_t CheckItemTypeRva = 0x373890;
constexpr std::uintptr_t GetItemContextRva = 0x34A0E0;
constexpr std::uintptr_t GetDataTablesRva = 0x300A90;
constexpr std::uintptr_t EtherealWeaponCheckReturnRva = 0x4432DA;
constexpr std::uintptr_t EtherealArmorCheckReturnRva = 0x4432E9;
constexpr std::uintptr_t ItemTypesRecordsOffset = 0x1348;
constexpr std::uintptr_t ItemTypesCountOffset = 0x1350;

constexpr std::uintptr_t EtherealChanceRva = 0x4434DF;
constexpr std::uintptr_t SetQualityBranchRva = 0x443315;
constexpr std::uintptr_t DurabilityEligibilityCallRva = 0x4432F4;
constexpr std::uintptr_t IndestructibleHelperCaveRva = 0x46D840;

constexpr std::array<std::uint8_t, 15> ExpectedCheckItemType{
    0x48, 0x89, 0x5C, 0x24, 0x10,
    0x48, 0x89, 0x6C, 0x24, 0x18,
    0x48, 0x89, 0x74, 0x24, 0x20
};
constexpr std::array<std::uint8_t, 16> ExpectedGetItemContext{
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75,
    0x1A, 0x88, 0x4C, 0x24, 0x30, 0x48, 0x8D, 0x4C
};
constexpr std::array<std::uint8_t, 16> ExpectedGetDataTables{
    0x48, 0x83, 0xEC, 0x28, 0x0F, 0xB6, 0xC1, 0x48,
    0x89, 0x44, 0x24, 0x38, 0x48, 0x83, 0xF8, 0x04
};
constexpr std::array<std::uint8_t, 1> ExpectedEtherealChance{0x05};
constexpr std::array<std::uint8_t, 6> ExpectedSetQualityBranch{
    0x0F, 0x84, 0x3D, 0x02, 0x00, 0x00
};
constexpr std::array<std::uint8_t, 5> ExpectedDurabilityEligibilityCall{
    0xE8, 0x47, 0x02, 0xF3, 0xFF
};
constexpr auto ExpectedIndestructibleHelperCave = [] {
    std::array<std::uint8_t, 67> bytes{};
    bytes.fill(0xCC);
    return bytes;
}();
constexpr std::array<std::uint8_t, 67> IndestructibleHelper{
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x89, 0x4C, 0x24, 0x20,
    0xE8, 0xF2, 0x5C, 0xF0, 0xFF, 0x85, 0xC0, 0x75, 0x2C,
    0x48, 0x8B, 0x4C, 0x24, 0x20, 0x45, 0x33, 0xC0, 0xBA,
    0x98, 0x00, 0x00, 0x00, 0xE8, 0xBC, 0x77, 0xE8, 0xFF,
    0x85, 0xC0, 0x7E, 0x14, 0x48, 0x8B, 0x4C, 0x24, 0x20,
    0xE8, 0xEE, 0x72, 0xE8, 0xFF, 0x85, 0xC0, 0x0F, 0x9F,
    0xC0, 0x0F, 0xB6, 0xC0, 0xEB, 0x02, 0x33, 0xC0, 0x48,
    0x83, 0xC4, 0x28, 0xC3
};

struct ResolvedTypeCache {
    const void* dataTables{};
    const void* records{};
    std::uint64_t recordCount{};
    std::array<std::int32_t, MaxExcludedItemTypes> ids{};
    std::size_t idCount{};
    std::size_t unresolvedCount{};
};

using CheckItemTypeFn = std::int32_t(__fastcall*)(const void*, std::int32_t) noexcept;
using GetItemContextFn = std::uint8_t(__fastcall*)(const void*) noexcept;
using GetDataTablesFn = std::uint8_t*(__fastcall*)(std::uint8_t) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
std::string LoadedConfigPath{"built-in vanilla defaults"};
CheckItemTypeFn OriginalCheckItemType{};
GetItemContextFn GetItemContext{};
GetDataTablesFn GetDataTables{};
std::atomic<std::uint64_t> ExcludedEligibleItems{};
std::atomic<std::uint32_t> ResolvedTypeCount{};
std::atomic<std::uint32_t> UnresolvedTypeCount{};
std::atomic_flag UnresolvedWarningLogged = ATOMIC_FLAG_INIT;
thread_local ResolvedTypeCache TypeCache{};
thread_local const void* PendingGateItem{};
thread_local bool PendingGateExcluded{};
thread_local bool PendingGateWasWeapon{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "ethereal-item-rules",
    .name = "Ethereal Item Rules",
    .version = "0.1.0",
    .author = "RuffnecKk",
    .description = "Controls ethereal item eligibility, chance, and exclusions.",
    .flags = D2RL::PluginFlags::NativeHooks,
};

template<class T>
T At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(Base + rva);
}

std::vector<std::filesystem::path> ConfigCandidates() {
    std::vector<std::filesystem::path> candidates;
    if (Context && Context->modDirectory && Context->modDirectory[0] != L'\0') {
        candidates.emplace_back(std::filesystem::path(Context->modDirectory) / ConfigFileName);
    }
    candidates.emplace_back(ConfigFileName);
    return candidates;
}

bool LoadConfig() noexcept {
    Settings = {};
    LoadedConfigPath = "built-in vanilla defaults";
    for (const auto& path : ConfigCandidates()) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) continue;

        try {
            std::ifstream input(path);
            if (!input.is_open()) throw std::runtime_error("file could not be opened");
            Settings = ParseConfig(nlohmann::json::parse(input, nullptr, true, true));
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            if (Context) {
                const auto message = std::string("EtherealItemRules: invalid ")
                    + path.string() + " (" + exception.what() + ").";
                Context->LogError(message.c_str());
            }
            return false;
        }
    }
    return true;
}

bool RefreshTypeCache(const void* item) noexcept {
    if (!item || !GetItemContext || !GetDataTables) return false;
    const auto context = GetItemContext(item);
    auto* dataTables = GetDataTables(context);
    if (!dataTables) return false;

    const auto* records =
        *reinterpret_cast<const std::uint8_t* const*>(dataTables + ItemTypesRecordsOffset);
    const auto recordCount =
        *reinterpret_cast<const std::uint64_t*>(dataTables + ItemTypesCountOffset);
    if (!records || recordCount == 0 || recordCount > 4096) return false;
    if (TypeCache.dataTables == dataTables
        && TypeCache.records == records
        && TypeCache.recordCount == recordCount) {
        return true;
    }

    TypeCache = {};
    TypeCache.dataTables = dataTables;
    TypeCache.records = records;
    TypeCache.recordCount = recordCount;
    for (std::size_t index = 0; index < Settings.exclusions.itemTypeCount; ++index) {
        const auto id = FindItemTypeId(
            records,
            recordCount,
            ItemTypeRecordStride,
            Settings.exclusions.itemTypes[index]
        );
        if (id < 0) {
            ++TypeCache.unresolvedCount;
            continue;
        }
        TypeCache.ids[TypeCache.idCount++] = id;
    }

    ResolvedTypeCount.store(
        static_cast<std::uint32_t>(TypeCache.idCount),
        std::memory_order_relaxed
    );
    UnresolvedTypeCount.store(
        static_cast<std::uint32_t>(TypeCache.unresolvedCount),
        std::memory_order_relaxed
    );
    if (TypeCache.unresolvedCount != 0
        && !UnresolvedWarningLogged.test_and_set(std::memory_order_relaxed)) {
        Context->LogWarn(
            "EtherealItemRules: one or more configured item type codes do not exist "
            "in the active itemtypes table."
        );
    }
    return true;
}

bool IsExcluded(const void* item) noexcept {
    if (!Settings.exclusions.enabled
        || Settings.exclusions.itemTypeCount == 0
        || !RefreshTypeCache(item)) {
        return false;
    }
    for (std::size_t index = 0; index < TypeCache.idCount; ++index) {
        if (OriginalCheckItemType(item, TypeCache.ids[index]) != 0) return true;
    }
    return false;
}

std::int32_t __fastcall HookCheckItemType(const void* item, std::int32_t itemType) noexcept {
    const auto result = OriginalCheckItemType(item, itemType);
    const auto returnRva = reinterpret_cast<std::uintptr_t>(_ReturnAddress())
        - reinterpret_cast<std::uintptr_t>(Base);

    if (returnRva == EtherealWeaponCheckReturnRva) {
        PendingGateItem = item;
        PendingGateExcluded = IsExcluded(item);
        PendingGateWasWeapon = result != 0;
        return PendingGateExcluded ? 0 : result;
    }
    if (returnRva == EtherealArmorCheckReturnRva) {
        const bool excluded = PendingGateItem == item ? PendingGateExcluded : IsExcluded(item);
        const bool wasEligible = (PendingGateItem == item && PendingGateWasWeapon) || result != 0;
        PendingGateItem = nullptr;
        PendingGateExcluded = false;
        PendingGateWasWeapon = false;
        if (excluded && wasEligible) {
            ExcludedEligibleItems.fetch_add(1, std::memory_order_relaxed);
            return 0;
        }
    }
    return result;
}

template<std::size_t Size>
bool Preflight(
    std::uintptr_t rva,
    const std::array<std::uint8_t, Size>& expected,
    const char* label
) noexcept {
    if (Context->CheckExpectedBytes(
            rva,
            expected.data(),
            static_cast<std::uint32_t>(expected.size())
        )) {
        return true;
    }
    const auto message = std::string("EtherealItemRules: ") + label
        + " signature mismatch; plugin refused before mutation.";
    Context->LogError(message.c_str());
    return false;
}

bool PreflightEnabledSites() noexcept {
    bool valid = true;
    if (Settings.exclusions.enabled) {
        valid = Preflight(CheckItemTypeRva, ExpectedCheckItemType, "item-type hook") && valid;
        valid = Preflight(GetItemContextRva, ExpectedGetItemContext, "item context helper")
            && valid;
        valid = Preflight(GetDataTablesRva, ExpectedGetDataTables, "data tables helper")
            && valid;
    }
    if (PatchChance(Settings)) {
        valid = Preflight(EtherealChanceRva, ExpectedEtherealChance, "chance byte") && valid;
    }
    if (PatchSetItems(Settings)) {
        valid = Preflight(SetQualityBranchRva, ExpectedSetQualityBranch, "set-quality branch")
            && valid;
    }
    if (PatchIndestructibleItems(Settings)) {
        valid = Preflight(
            DurabilityEligibilityCallRva,
            ExpectedDurabilityEligibilityCall,
            "durability eligibility call"
        ) && valid;
        valid = Preflight(
            IndestructibleHelperCaveRva,
            ExpectedIndestructibleHelperCave,
            "indestructible helper cave"
        ) && valid;
    }
    return valid;
}

bool InstallExclusionHook() noexcept {
    if (!Settings.exclusions.enabled) return true;
    GetItemContext = At<GetItemContextFn>(GetItemContextRva);
    GetDataTables = At<GetDataTablesFn>(GetDataTablesRva);
    if (!Context->InstallInlineHook(
            CheckItemTypeRva,
            ExpectedCheckItemType.data(),
            static_cast<std::uint32_t>(ExpectedCheckItemType.size()),
            HookCheckItemType,
            &OriginalCheckItemType
        )) {
        Context->LogError("EtherealItemRules: item-type hook could not be installed.");
        return false;
    }
    return true;
}

bool InstallRulePatches() noexcept {
    if (PatchIndestructibleItems(Settings)) {
        if (!Context->PatchBytes(
                IndestructibleHelperCaveRva,
                ExpectedIndestructibleHelperCave.data(),
                static_cast<std::uint32_t>(ExpectedIndestructibleHelperCave.size()),
                IndestructibleHelper.data(),
                static_cast<std::uint32_t>(IndestructibleHelper.size())
            )) {
            Context->LogError("EtherealItemRules: indestructible helper patch failed.");
            return false;
        }
        if (!Context->PatchCallRel32(
                DurabilityEligibilityCallRva,
                ExpectedDurabilityEligibilityCall.data(),
                static_cast<std::uint32_t>(ExpectedDurabilityEligibilityCall.size()),
                IndestructibleHelperCaveRva
            )) {
            Context->LogError("EtherealItemRules: durability eligibility call patch failed.");
            return false;
        }
    }
    if (PatchSetItems(Settings)
        && !Context->PatchNop(
            SetQualityBranchRva,
            ExpectedSetQualityBranch.data(),
            static_cast<std::uint32_t>(ExpectedSetQualityBranch.size()),
            static_cast<std::uint32_t>(ExpectedSetQualityBranch.size())
        )) {
        Context->LogError("EtherealItemRules: set-quality branch patch failed.");
        return false;
    }
    if (PatchChance(Settings)
        && !Context->PatchWriteU8(
            EtherealChanceRva,
            ExpectedEtherealChance.data(),
            static_cast<std::uint32_t>(ExpectedEtherealChance.size()),
            Settings.rules.chancePercent
        )) {
        Context->LogError("EtherealItemRules: chance byte patch failed.");
        return false;
    }
    return true;
}

void FormatConfiguredTypes(char* output, std::size_t size) noexcept {
    if (!output || size == 0) return;
    std::size_t used{};
    for (std::size_t index = 0; index < Settings.exclusions.itemTypeCount; ++index) {
        const auto& code = Settings.exclusions.itemTypes[index];
        const auto written = std::snprintf(
            output + used,
            size - used,
            "%s%.*s",
            index == 0 ? "" : ",",
            static_cast<int>(code.length),
            code.text.data()
        );
        if (written < 0 || static_cast<std::size_t>(written) >= size - used) break;
        used += static_cast<std::size_t>(written);
    }
    if (Settings.exclusions.itemTypeCount == 0) std::snprintf(output, size, "none");
}

auto Status(
    D2R::Game::Client*,
    const D2RL::ConsoleCommandContext* command,
    void*
) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    char types[384]{};
    char message[1024]{};
    FormatConfiguredTypes(types, sizeof(types));
    std::snprintf(
        message,
        sizeof(message),
        "EtherealItemRules 0.1.0: JSON=%s; exclusions=%s configured=[%s] "
        "resolved=%u unresolved=%u excluded=%llu; rules=%s chance=%u%% "
        "sets=%s indestructible=%s.",
        LoadedConfigPath.c_str(),
        Settings.exclusions.enabled ? "enabled" : "disabled",
        types,
        ResolvedTypeCount.load(std::memory_order_relaxed),
        UnresolvedTypeCount.load(std::memory_order_relaxed),
        static_cast<unsigned long long>(ExcludedEligibleItems.load(std::memory_order_relaxed)),
        Settings.rules.enabled ? "enabled" : "disabled",
        static_cast<unsigned>(Settings.rules.chancePercent),
        Settings.rules.allowSetItems ? "allowed" : "vanilla-blocked",
        Settings.rules.allowIndestructibleItems ? "allowed" : "vanilla-filtered"
    );
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}
} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
    const D2RL::PluginContext* context
) noexcept -> bool {
    if (!context) return false;
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(context->exeBase);
    if (!Base) Base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    if (!Base || !LoadConfig()) {
        context->LogError("EtherealItemRules: configuration could not be loaded or is invalid.");
        return false;
    }
    if (context->modDataVersionBuild != 0
        && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("EtherealItemRules: only D2R build 92777 is supported.");
        return false;
    }
    if (!PreflightEnabledSites()) return false;
    if (!InstallExclusionHook() || !InstallRulePatches()) return false;

    if (!context->RegisterConsoleCommand(
            "ethereal-item-rules",
            Status,
            "Show ethereal item rules, exclusions, and runtime counters."
        )) {
        context->LogWarn("EtherealItemRules: status command could not be registered.");
    }

    const bool changed = Settings.exclusions.enabled || HasDirectRulePatches(Settings);
    const auto message = std::string("EtherealItemRules 0.1.0 loaded from ")
        + LoadedConfigPath
        + (changed
            ? " (configured native ownership active)."
            : " (vanilla defaults; no hook or patch installed).");
    context->LogInfo(message.c_str());
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    Context = nullptr;
}
