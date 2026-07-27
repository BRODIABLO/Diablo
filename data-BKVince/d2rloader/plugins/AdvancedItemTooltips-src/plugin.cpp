#define NOMINMAX
#include <D2RLPlugin/api.h>
#include "socket_tooltip.hpp"
#include "tooltip_ranges.hpp"

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr std::uint32_t SupportedBuild = 92777;
constexpr std::uintptr_t GetMaxSocketsRva = 0x36EAD0;
constexpr std::uintptr_t GetUnitStatRva = 0x2F5020;
constexpr std::uintptr_t GetItemDataContextRva = 0x34A0E0;
constexpr std::uintptr_t GetItemDataRva = 0x34A500;
constexpr std::uintptr_t GetItemsTxtRecordRva = 0x314110;
constexpr std::int32_t SocketsStat = 194;
constexpr std::int32_t ArmorClassStat = 31;
constexpr std::size_t UnitClassIdOffset = 0x04;
constexpr std::size_t ItemDataQualityOffset = 0x00;
constexpr std::size_t ItemDataFlagsOffset = 0x18;
constexpr std::size_t ItemDataFileIndexOffset = 0x34;
constexpr std::size_t ItemDataRarePrefixOffset = 0x42;
constexpr std::size_t ItemDataRareSuffixOffset = 0x44;
constexpr std::size_t ItemDataAutoPrefixOffset = 0x46;
constexpr std::size_t ItemDataMagicPrefixOffset = 0x48;
constexpr std::size_t ItemDataMagicSuffixOffset = 0x4E;
constexpr std::size_t ItemsTxtCodeOffset = 0x80;
constexpr std::uint32_t ItemFlagIdentified = 0x00000010;
constexpr std::uint32_t ItemFlagEthereal = 0x00400000;

using GetMaxSocketsFn = std::uint8_t(__fastcall*)(void*) noexcept;
using GetUnitStatFn = std::int32_t(__fastcall*)(void*, std::int32_t, std::uint16_t) noexcept;
using GetItemDataContextFn = std::uint8_t(__fastcall*)(void*) noexcept;
using GetItemDataFn = std::uint8_t*(__fastcall*)(void*) noexcept;
using GetItemsTxtRecordFn = std::uint8_t*(__fastcall*)(std::uint8_t, std::int32_t) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
GetMaxSocketsFn GetMaxSockets{};
GetUnitStatFn GetUnitStat{};
GetItemDataContextFn GetItemDataContext{};
GetItemDataFn GetItemData{};
GetItemsTxtRecordFn GetItemsTxtRecord{};
tcp::tooltips::RangeCatalog Catalog;
bool CatalogLoaded{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "advanced-item-tooltips",
    .name = "Advanced Item Tooltips",
    .version = "2.0.0",
    .author = "RuffnecKk",
    .description = "Shows maximum sockets and exact item roll ranges.",
    .flags = D2RL::PluginFlags::None,
};

template<class T>
T At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(Base + rva);
}

template<class T>
T Read(const std::uint8_t* address, std::size_t offset) noexcept {
    T value{};
    std::memcpy(&value, address + offset, sizeof(value));
    return value;
}

tcp::tooltips::ItemAffixIds ReadAffixIds(const std::uint8_t* data) noexcept {
    tcp::tooltips::ItemAffixIds ids{};
    ids.quality = Read<std::uint32_t>(data, ItemDataQualityOffset);
    ids.fileIndex = Read<std::uint32_t>(data, ItemDataFileIndexOffset);
    ids.rarePrefix = Read<std::uint16_t>(data, ItemDataRarePrefixOffset);
    ids.rareSuffix = Read<std::uint16_t>(data, ItemDataRareSuffixOffset);
    ids.autoPrefix = Read<std::uint16_t>(data, ItemDataAutoPrefixOffset);
    for (std::size_t index = 0; index < 3; ++index) {
        ids.magicPrefix[index] = Read<std::uint16_t>(data, ItemDataMagicPrefixOffset + index * 2);
        ids.magicSuffix[index] = Read<std::uint16_t>(data, ItemDataMagicSuffixOffset + index * 2);
    }
    return ids;
}

std::string ItemCode(void* item) noexcept {
    if (!GetItemsTxtRecord || !GetItemDataContext) return {};
    const auto classId = Read<std::int32_t>(static_cast<const std::uint8_t*>(item), UnitClassIdOffset);
    const auto* record = GetItemsTxtRecord(GetItemDataContext(item), classId);
    if (!record) return {};
    char code[5]{};
    std::memcpy(code, record + ItemsTxtCodeOffset, 4);
    std::string result(code);
    while (!result.empty() && (result.back() == ' ' || result.back() == '\0')) result.pop_back();
    return result;
}

bool ContainsKey(const std::vector<std::vector<tcp::tooltips::ModifierRange>>& candidates,
    std::string_view key) {
    for (const auto& candidate : candidates)
        for (const auto& range : candidate)
            if (range.key == key) return true;
    return false;
}

std::optional<int> FlatDefenseRoll(
    std::string_view tooltip,
    const std::vector<std::vector<tcp::tooltips::ModifierRange>>& candidates) {
    if (candidates.empty()) return std::nullopt;
    std::size_t start{};
    while (start <= tooltip.size()) {
        const auto end = tooltip.find('\n', start);
        const auto line = tooltip.substr(start, end - start);
        if (line.find("+" ) != std::string_view::npos && line.find("Defense") != std::string_view::npos) {
            if (const auto value = tcp::tooltips::FirstSignedInteger(line)) return *value;
        }
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return 0;
}

std::string InsertBaseDefense(std::string tooltip, void* item, const std::uint8_t* itemData,
    const std::vector<std::vector<tcp::tooltips::ModifierRange>>& candidates) {
    if (!GetUnitStat || ContainsKey(candidates, "item_armor_percent")) return tooltip;
    const auto armor = Catalog.FindArmor(ItemCode(item));
    if (!armor || tooltip.find("Base Defense:") != std::string::npos) return tooltip;
    auto minimum = armor->minimum;
    auto maximum = armor->maximum;
    if ((Read<std::uint32_t>(itemData, ItemDataFlagsOffset) & ItemFlagEthereal) != 0) {
        minimum = minimum * 3 / 2;
        maximum = maximum * 3 / 2;
    }
    const auto flat = FlatDefenseRoll(tooltip, candidates);
    if (!flat) return tooltip;
    const auto rolled = GetUnitStat(item, ArmorClassStat, 0) - *flat;
    if (rolled < minimum || rolled > maximum) return tooltip;
    std::size_t start{};
    while (start < tooltip.size()) {
        const auto end = tooltip.find('\n', start);
        const auto line = tooltip.substr(start, end - start);
        if (line.find("Defense:") != std::string_view::npos
            && line.find("Enhanced Defense") == std::string_view::npos) {
            const auto added = std::string("\xEE\x81\xBE" "0Base Defense: ")
                + std::to_string(rolled) + " "
                + tcp::tooltips::FormatPositiveRange(minimum, maximum, '0') + "\n";
            tooltip.insert(start, added);
            break;
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return tooltip;
}

auto Status(
    D2R::Game::Client*,
    const D2RL::ConsoleCommandContext* command,
    void*
) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    command->plugin->WriteConsoleMessage(
        "AdvancedItemTooltips 2.0.0: final-tooltip ranges and sockets are available."
    );
    return D2RL::ConsoleCommandResult::Handled;
}
} // namespace

extern "C" __declspec(dllexport) std::size_t __cdecl AdvancedItemTooltipsEnhanceTooltip(
    void* item,
    const char* tooltip,
    std::size_t tooltipLength,
    char* output,
    std::size_t outputCapacity
) noexcept {
    if (!item || !tooltip || !output || outputCapacity == 0 || !CatalogLoaded
        || tooltipLength == 0 || tooltipLength > 16 * 1024 || !GetItemData) return 0;
    try {
        const auto* itemData = GetItemData(item);
        if (!itemData || (Read<std::uint32_t>(itemData, ItemDataFlagsOffset) & ItemFlagIdentified) == 0) return 0;
        const auto ids = ReadAffixIds(itemData);
        const auto candidates = Catalog.ResolveCandidates(ids, ItemCode(item));
        const std::string original(tooltip, tooltipLength);
        auto enhanced = tcp::tooltips::AppendConsensusRanges(original, candidates);
        enhanced = InsertBaseDefense(std::move(enhanced), item, itemData, candidates);
        if (enhanced == original || enhanced.size() + 1 > outputCapacity) return 0;
        std::memcpy(output, enhanced.data(), enhanced.size());
        output[enhanced.size()] = '\0';
        return enhanced.size();
    } catch (...) {
        return 0;
    }
}

extern "C" __declspec(dllexport) std::size_t __cdecl AdvancedItemTooltipsBuildSocketLine(
    void* item,
    char* output,
    std::size_t outputCapacity
) noexcept {
    if (!item || !output || outputCapacity == 0 || !GetMaxSockets) return 0;
    try {
        const auto maximumSockets = static_cast<unsigned>(GetMaxSockets(item));
        const auto currentSockets = GetUnitStat
            ? GetUnitStat(item, SocketsStat, 0)
            : 0;
        const auto line = tcp::tooltips::FormatMaxSocketsLine(maximumSockets, currentSockets);
        if (line.empty() || line.size() + 1 > outputCapacity) return 0;
        std::memcpy(output, line.data(), line.size());
        output[line.size()] = '\0';
        return line.size();
    } catch (...) {
        return 0;
    }
}

extern "C" __declspec(dllexport) std::size_t __cdecl AdvancedItemTooltipsFindSocketLineInsertion(
    const char* tooltip,
    std::size_t tooltipLength
) noexcept {
    if (!tooltip || tooltipLength == 0 || tooltipLength > 16 * 1024) {
        return tcp::tooltips::NoSocketLineInsertion;
    }
    try {
        return tcp::tooltips::FindMaxSocketsInsertion(
            std::string_view(tooltip, tooltipLength)
        );
    } catch (...) {
        return tcp::tooltips::NoSocketLineInsertion;
    }
}

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    if (!context) return false;
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    if (!Base) return false;
    if (context->modDataVersionBuild != 0 && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("AdvancedItemTooltips: only D2R build 92777 is supported.");
        return false;
    }

    GetMaxSockets = At<GetMaxSocketsFn>(GetMaxSocketsRva);
    GetUnitStat = At<GetUnitStatFn>(GetUnitStatRva);
    GetItemDataContext = At<GetItemDataContextFn>(GetItemDataContextRva);
    GetItemData = At<GetItemDataFn>(GetItemDataRva);
    GetItemsTxtRecord = At<GetItemsTxtRecordFn>(GetItemsTxtRecordRva);
    std::string catalogError;
    if (context->modDirectory) {
        const auto modDirectory = std::filesystem::path(context->modDirectory);
        std::vector<std::filesystem::path> excelCandidates{
            modDirectory / L"data/global/excel"
        };
        if (context->activeMod && context->activeMod[0] != '\0') {
            excelCandidates.push_back(modDirectory
                / (std::string(context->activeMod) + ".mpq") / L"data/global/excel");
        }
        for (const auto& excel : excelCandidates) {
            if (!std::filesystem::exists(excel / L"properties.txt")) continue;
            if (Catalog.Load(excel, catalogError)) {
                CatalogLoaded = true;
                break;
            }
        }
    }
    if (!CatalogLoaded) {
        const auto message = "AdvancedItemTooltips: roll ranges unavailable; sockets remain active. " + catalogError;
        context->LogWarn(message.c_str());
    }
    if (!context->RegisterConsoleCommand(
            "advanced-item-tooltips",
            Status,
            "Show Advanced Item Tooltips integration status."
        )) {
        context->LogWarn("AdvancedItemTooltips: status command could not be registered.");
    }
    context->LogInfo("AdvancedItemTooltips 2.0.0 integration active for D2R 3.2.92777." );
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    GetMaxSockets = nullptr;
    GetUnitStat = nullptr;
    GetItemDataContext = nullptr;
    GetItemData = nullptr;
    GetItemsTxtRecord = nullptr;
    CatalogLoaded = false;
    Context = nullptr;
}
