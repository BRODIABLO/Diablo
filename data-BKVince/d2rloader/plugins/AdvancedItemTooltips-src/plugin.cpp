#define NOMINMAX
#include <D2RLPlugin/api.h>
#include "socket_tooltip.hpp"
#include "tooltip_ranges.hpp"

#include <Windows.h>

#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern "C" __declspec(dllexport) std::size_t __cdecl AdvancedItemTooltipsEnhanceTooltip(
    void* item,
    const char* tooltip,
    std::size_t tooltipLength,
    char* output,
    std::size_t outputCapacity
) noexcept;
extern "C" __declspec(dllexport) std::size_t __cdecl AdvancedItemTooltipsBuildSocketLine(
    void* item,
    char* output,
    std::size_t outputCapacity
) noexcept;
extern "C" __declspec(dllexport) std::size_t __cdecl AdvancedItemTooltipsFindSocketLineInsertion(
    const char* tooltip,
    std::size_t tooltipLength
) noexcept;

namespace {
constexpr std::uint32_t SupportedBuild = 92777;
constexpr std::uintptr_t BuildItemTooltipRva = 0x2BD480;
constexpr std::uintptr_t EnsureStringCapacityRva = 0x076210;
constexpr std::uintptr_t GetMaxSocketsRva = 0x36EAD0;
constexpr std::uintptr_t GetUnitStatRva = 0x2F5020;
constexpr std::uintptr_t GetItemDataContextRva = 0x34A0E0;
constexpr std::uintptr_t GetItemDataRva = 0x34A500;
constexpr std::uintptr_t GetInventoryRva = 0x34A360;
constexpr std::uintptr_t GetFirstInventoryItemRva = 0x388C10;
constexpr std::uintptr_t GetNextInventoryItemRva = 0x38ABA0;
constexpr std::uintptr_t GetItemsTxtRecordRva = 0x314110;
constexpr std::uintptr_t GetRunesTxtRecordFromItemRva = 0x372260;
constexpr std::uintptr_t GetLocalizedStringRva = 0x5F4A50;
constexpr std::uintptr_t GetLocalizedStringByKeyRva = 0x5F4B90;
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
constexpr std::size_t RunesTxtStringIdOffset = 0x46;
constexpr std::uint32_t ItemFlagIdentified = 0x00000010;
constexpr std::uint32_t ItemFlagEthereal = 0x00400000;
constexpr std::uint32_t ItemFlagRuneword = 0x04000000;

struct GameStringView {
    const char* data{};
    std::size_t size{};
};

using GetMaxSocketsFn = std::uint8_t(__fastcall*)(void*) noexcept;
using GetUnitStatFn = std::int32_t(__fastcall*)(void*, std::int32_t, std::uint16_t) noexcept;
using GetItemDataContextFn = std::uint8_t(__fastcall*)(void*) noexcept;
using GetItemDataFn = std::uint8_t*(__fastcall*)(void*) noexcept;
using GetInventoryFn = void*(__fastcall*)(void*) noexcept;
using GetInventoryItemFn = void*(__fastcall*)(void*) noexcept;
using GetItemsTxtRecordFn = std::uint8_t*(__fastcall*)(std::uint8_t, std::int32_t) noexcept;
using GetRunesTxtRecordFromItemFn = std::uint8_t*(__fastcall*)(void*) noexcept;
using GetLocalizedStringFn = const char*(__fastcall*)(std::uint16_t) noexcept;
using GetLocalizedStringByKeyFn = const char*(__fastcall*)(const GameStringView*) noexcept;
using BuildItemTooltipFn = void*(__fastcall*)(
    void*, void*, void*, void*, std::uint64_t, std::uint64_t, std::uint64_t,
    std::uint64_t, std::uint64_t) noexcept;
using EnsureStringCapacityFn = void(__fastcall*)(void*, std::size_t) noexcept;

struct TooltipCallSite {
    std::uintptr_t rva{};
    std::array<std::uint8_t, 5> expected{};
};

constexpr std::array TooltipCallSites{
    TooltipCallSite{0x2291DC, {0xE8,0x9F,0x42,0x09,0x00}},
    TooltipCallSite{0x2BCEE9, {0xE8,0x92,0x05,0x00,0x00}},
    TooltipCallSite{0x2C8C02, {0xE8,0x79,0x48,0xFF,0xFF}},
    TooltipCallSite{0x2CB32E, {0xE8,0x4D,0x21,0xFF,0xFF}},
    TooltipCallSite{0x2CE716, {0xE8,0x65,0xED,0xFE,0xFF}},
    TooltipCallSite{0x87E882, {0xE8,0xF9,0xEB,0xA3,0xFF}},
    TooltipCallSite{0x150C377, {0xE8,0x04,0x11,0xDB,0xFE}},
};

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
GetMaxSocketsFn GetMaxSockets{};
GetUnitStatFn GetUnitStat{};
GetItemDataContextFn GetItemDataContext{};
GetItemDataFn GetItemData{};
GetInventoryFn GetInventory{};
GetInventoryItemFn GetFirstInventoryItem{};
GetInventoryItemFn GetNextInventoryItem{};
GetItemsTxtRecordFn GetItemsTxtRecord{};
GetRunesTxtRecordFromItemFn GetRunesTxtRecordFromItem{};
GetLocalizedStringFn GetLocalizedString{};
GetLocalizedStringByKeyFn GetLocalizedStringByKey{};
BuildItemTooltipFn BuildItemTooltip{};
EnsureStringCapacityFn EnsureStringCapacity{};
void* TooltipRelay{};
tcp::tooltips::RangeCatalog Catalog;
bool CatalogLoaded{};
std::mutex RunewordNamesMutex;
bool RunewordNamesBuilt{};
std::unordered_map<std::string, std::string> RunewordKeyByLocalizedName;

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "advanced-item-tooltips",
    .name = "Advanced Item Tooltips",
    .version = "2.2.0",
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

bool IsReadable(const void* address, std::size_t size) noexcept {
    if (!address || size == 0) return false;
    MEMORY_BASIC_INFORMATION memory{};
    if (VirtualQuery(address, &memory, sizeof(memory)) != sizeof(memory)
        || memory.State != MEM_COMMIT
        || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0) return false;
    const auto begin = reinterpret_cast<std::uintptr_t>(address);
    const auto regionEnd = reinterpret_cast<std::uintptr_t>(memory.BaseAddress) + memory.RegionSize;
    return begin <= regionEnd && size <= regionEnd - begin;
}

tcp::tooltips::ItemAffixIds ReadAffixIds(const std::uint8_t* data) noexcept {
    tcp::tooltips::ItemAffixIds ids{};
    ids.quality = Read<std::uint32_t>(data, ItemDataQualityOffset);
    ids.fileIndex = Read<std::uint32_t>(data, ItemDataFileIndexOffset);
    ids.runeword = (Read<std::uint32_t>(data, ItemDataFlagsOffset) & ItemFlagRuneword) != 0;
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

bool HasRunewordTitle(std::string_view tooltip, std::string_view name) {
    std::size_t start{};
    while (start <= tooltip.size()) {
        const auto end = tooltip.find('\n', start);
        const auto line = tooltip.substr(start, end - start);
        std::string visible;
        visible.reserve(line.size());
        for (std::size_t index = 0; index < line.size();) {
            if (index + 4 <= line.size()
                && static_cast<unsigned char>(line[index]) == 0xEE
                && static_cast<unsigned char>(line[index + 1]) == 0x81
                && static_cast<unsigned char>(line[index + 2]) == 0xBE) {
                index += 4;
                continue;
            }
            visible.push_back(line[index++]);
        }
        while (!visible.empty() && std::isspace(static_cast<unsigned char>(visible.front())))
            visible.erase(visible.begin());
        while (!visible.empty() && std::isspace(static_cast<unsigned char>(visible.back())))
            visible.pop_back();
        if (visible == name
            || (visible.size() > name.size() + 2
                && visible.compare(0, name.size(), name) == 0
                && visible[name.size()] == ' '
                && visible[name.size() + 1] == '(')) return true;
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return false;
}

std::string ResolveRunewordKey(
    void* item, const std::uint8_t* itemData, std::string_view tooltip) {
    if (!item || !itemData || !GetLocalizedStringByKey
        || (Read<std::uint32_t>(itemData, ItemDataFlagsOffset) & ItemFlagRuneword) == 0) return {};

    std::scoped_lock lock(RunewordNamesMutex);
    if (!RunewordNamesBuilt) {
        std::unordered_map<std::string, std::string> resolved;
        std::unordered_set<std::string> ambiguous;
        for (const auto& key : Catalog.RunewordKeys()) {
            const GameStringView view{key.data(), key.size()};
            const auto* text = GetLocalizedStringByKey(&view);
            if (!text || text[0] == '\0') continue;
            const std::string name(text);
            if (const auto existing = resolved.find(name); existing != resolved.end()) {
                resolved.erase(existing);
                ambiguous.insert(name);
            } else if (!ambiguous.contains(name)) {
                resolved.emplace(name, key);
            }
        }
        // Localization is initialized by the time the first tooltip is built.
        // Still retry on a later tooltip if the resolver unexpectedly yielded
        // no names instead of caching an unusable empty map.
        if (!resolved.empty()) {
            RunewordKeyByLocalizedName = std::move(resolved);
            RunewordNamesBuilt = true;
        }
    }
    // Prefer the native runes.txt record. Some torso runewords can however
    // arrive without a usable localized record during final-tooltip assembly;
    // fail over to the already-rendered title instead of dropping every range.
    if (GetRunesTxtRecordFromItem && GetLocalizedString) {
        if (const auto* record = GetRunesTxtRecordFromItem(item)) {
            const auto stringId = Read<std::uint16_t>(record, RunesTxtStringIdOffset);
            if (const auto* localizedName = GetLocalizedString(stringId);
                localizedName && localizedName[0] != '\0') {
                if (const auto found = RunewordKeyByLocalizedName.find(localizedName);
                    found != RunewordKeyByLocalizedName.end()) return found->second;
            }
        }
    }
    for (const auto& [name, key] : RunewordKeyByLocalizedName)
        if (HasRunewordTitle(tooltip, name)) return key;
    return {};
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

std::string InsertMaxSocketsBelowPrimaryStat(std::string tooltip, void* item) {
    std::array<char, 64> line{};
    const auto length = AdvancedItemTooltipsBuildSocketLine(item, line.data(), line.size());
    if (length == 0 || length >= line.size()) return tooltip;
    const std::string_view text(line.data(), length);
    if (tooltip.find(text) != std::string::npos) return tooltip;
    const auto insertion = AdvancedItemTooltipsFindSocketLineInsertion(
        tooltip.data(), tooltip.size());
    if (insertion > tooltip.size()) return tooltip;
    tooltip.insert(insertion, std::string(text) + "\n");
    return tooltip;
}

void* TransformOwnedTooltip(void* result, void* item) noexcept {
    if (!result || !item || !EnsureStringCapacity || !IsReadable(result, 24)) return result;
    try {
        const auto* object = static_cast<const std::uint8_t*>(result);
        const auto* data = *reinterpret_cast<char* const*>(object);
        const auto length = *reinterpret_cast<const std::size_t*>(object + 8);
        if (length == 0 || length > 16 * 1024 || !IsReadable(data, length + 1)) return result;

        const std::string original(data, length);
        auto enhanced = original;
        std::array<char, 64 * 1024> buffer{};
        const auto enhancedLength = AdvancedItemTooltipsEnhanceTooltip(
            item, enhanced.data(), enhanced.size(), buffer.data(), buffer.size());
        if (enhancedLength > 0 && enhancedLength < buffer.size()) {
            enhanced.assign(buffer.data(), enhancedLength);
        }
        enhanced = InsertMaxSocketsBelowPrimaryStat(std::move(enhanced), item);
        if (enhanced == original) return result;

        EnsureStringCapacity(result, enhanced.size());
        auto* destination = *reinterpret_cast<char**>(result);
        if (!IsReadable(destination, enhanced.size() + 1)) return result;
        std::memcpy(destination, enhanced.c_str(), enhanced.size() + 1);
        const auto size = enhanced.size();
        std::memcpy(static_cast<std::uint8_t*>(result) + 8, &size, sizeof(size));
    } catch (...) {
        if (Context) Context->LogError(
            "AdvancedItemTooltips: autonomous tooltip transform failed safely.");
    }
    return result;
}

void* __fastcall HookBuildItemTooltip(
    void* output,
    void* a2,
    void* a3,
    void* item,
    std::uint64_t a5,
    std::uint64_t a6,
    std::uint64_t a7,
    std::uint64_t a8,
    std::uint64_t a9) noexcept {
    auto* result = BuildItemTooltip(output, a2, a3, item, a5, a6, a7, a8, a9);
    return TransformOwnedTooltip(result, item);
}

void* AllocateNear(void* hint, std::size_t size) noexcept {
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const auto granularity = static_cast<std::uintptr_t>(systemInfo.dwAllocationGranularity);
    const auto base = reinterpret_cast<std::uintptr_t>(hint) & ~(granularity - 1);
    for (std::uintptr_t delta = granularity; delta < 0x70000000ULL; delta += granularity) {
        if (base > std::numeric_limits<std::uintptr_t>::max() - delta) break;
        if (auto* memory = VirtualAlloc(
                reinterpret_cast<void*>(base + delta), size,
                MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)) return memory;
    }
    return nullptr;
}

bool InstallTooltipCallSites() noexcept {
    constexpr std::size_t RelaySize = 14;
    TooltipRelay = AllocateNear(Base + TooltipCallSites.front().rva, RelaySize);
    if (!TooltipRelay) return false;
    auto* relay = static_cast<std::uint8_t*>(TooltipRelay);
    relay[0] = 0xFF;
    relay[1] = 0x25;
    relay[2] = relay[3] = relay[4] = relay[5] = 0x00;
    const auto target = reinterpret_cast<std::uint64_t>(&HookBuildItemTooltip);
    std::memcpy(relay + 6, &target, sizeof(target));
    FlushInstructionCache(GetCurrentProcess(), relay, RelaySize);
    DWORD previousProtection{};
    if (!VirtualProtect(relay, RelaySize, PAGE_EXECUTE_READ, &previousProtection)) return false;

    const auto relayAddress = reinterpret_cast<std::uintptr_t>(TooltipRelay);
    const auto baseAddress = reinterpret_cast<std::uintptr_t>(Base);
    if (relayAddress < baseAddress) return false;
    const auto relayRva = static_cast<std::uint64_t>(relayAddress - baseAddress);
    for (const auto& site : TooltipCallSites) {
        const auto nextInstruction = reinterpret_cast<std::uintptr_t>(Base + site.rva + 5);
        const auto displacement = static_cast<std::int64_t>(relayAddress)
            - static_cast<std::int64_t>(nextInstruction);
        if (displacement < std::numeric_limits<std::int32_t>::min()
            || displacement > std::numeric_limits<std::int32_t>::max()
            || !Context->PatchCallRel32(
                site.rva, site.expected.data(), static_cast<std::uint32_t>(site.expected.size()),
                relayRva, static_cast<std::uint32_t>(site.expected.size()))) return false;
    }
    return true;
}

auto Status(
    D2R::Game::Client*,
    const D2RL::ConsoleCommandContext* command,
    void*
) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    command->plugin->WriteConsoleMessage(
        "AdvancedItemTooltips 2.2.0: autonomous affix, defense, and socket ranges are available."
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
        const auto code = ItemCode(item);
        const auto runewordKey = ResolveRunewordKey(
            item, itemData, std::string_view(tooltip, tooltipLength));
        auto candidates = Catalog.ResolveCandidates(ids, code, runewordKey);
        // Runeword rune bonuses are already reconstructed from runes.txt plus
        // gems.txt in ResolveCandidates. Enumerating their socket inventory as
        // ordinary fillers would count every rune twice.
        if (!ids.runeword && GetInventory && GetFirstInventoryItem && GetNextInventoryItem) {
            if (auto* inventory = GetInventory(item)) {
                auto* socketFiller = GetFirstInventoryItem(inventory);
                for (std::size_t index = 0; socketFiller && index < 6; ++index) {
                    if (const auto* fillerData = GetItemData(socketFiller)) {
                        const auto fillerIds = ReadAffixIds(fillerData);
                        const auto fillerCode = ItemCode(socketFiller);
                        const auto fillerCandidates = Catalog.ResolveSocketFillerCandidates(
                            fillerIds, fillerCode, code);
                        candidates = tcp::tooltips::MergeCandidateSources(
                            candidates, fillerCandidates);
                    }
                    socketFiller = GetNextInventoryItem(socketFiller);
                }
            }
        }
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
        const auto line = tcp::tooltips::FormatMaxSocketsLine(maximumSockets, 0);
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
    GetInventory = At<GetInventoryFn>(GetInventoryRva);
    GetFirstInventoryItem = At<GetInventoryItemFn>(GetFirstInventoryItemRva);
    GetNextInventoryItem = At<GetInventoryItemFn>(GetNextInventoryItemRva);
    GetItemsTxtRecord = At<GetItemsTxtRecordFn>(GetItemsTxtRecordRva);
    constexpr std::array<std::uint8_t, 29> runewordResolverExpected{
        0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x74,
        0x24,0x18,0x48,0x89,0x7C,0x24,0x20,0x55,
        0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,
        0x48,0x8B,0xEC,0x48,0x83};
    constexpr std::array<std::uint8_t, 14> localizedStringExpected{
        0x80,0x3D,0x05,0xAE,0xCF,0x02,0x00,
        0x48,0x8D,0x05,0xDA,0x96,0xDB,0x01};
    constexpr std::array<std::uint8_t, 29> localizedStringByKeyExpected{
        0x4C,0x8B,0xDC,0x55,0x53,0x57,0x49,0x8D,
        0x6B,0xA1,0x48,0x81,0xEC,0xB0,0x00,0x00,
        0x00,0x48,0x8B,0x05,0x20,0x67,0x3D,0x02,
        0x48,0x33,0xC4,0x48,0x89};
    constexpr std::array<std::uint8_t, 16> getInventoryExpected{
        0x48,0x89,0x5C,0x24,0x18,0x56,0x48,0x83,
        0xEC,0x20,0x48,0x8B,0xF1,0x48,0x85,0xC9};
    constexpr std::array<std::uint8_t, 14> getFirstInventoryItemExpected{
        0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,
        0xD9,0x48,0x85,0xC9,0x74,0x2E};
    constexpr std::array<std::uint8_t, 14> getNextInventoryItemExpected{
        0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,
        0xD9,0x48,0x85,0xC9,0x75,0x10};
    constexpr std::array<std::uint8_t, 24> ensureStringCapacityExpected{
        0x4C,0x8B,0xDC,0x49,0x89,0x5B,0x08,0x49,
        0x89,0x6B,0x18,0x49,0x89,0x73,0x20,0x49,
        0x89,0x53,0x10,0x57,0x48,0x83,0xEC,0x30};
    if (!context->CheckExpectedBytes(GetRunesTxtRecordFromItemRva,
            runewordResolverExpected.data(), runewordResolverExpected.size())
        || !context->CheckExpectedBytes(GetLocalizedStringRva,
            localizedStringExpected.data(), localizedStringExpected.size())
        || !context->CheckExpectedBytes(GetLocalizedStringByKeyRva,
            localizedStringByKeyExpected.data(), localizedStringByKeyExpected.size())) {
        context->LogError("AdvancedItemTooltips: runeword ABI signature mismatch for build 92777.");
        return false;
    }
    if (!context->CheckExpectedBytes(GetInventoryRva,
            getInventoryExpected.data(), getInventoryExpected.size())
        || !context->CheckExpectedBytes(GetFirstInventoryItemRva,
            getFirstInventoryItemExpected.data(), getFirstInventoryItemExpected.size())
        || !context->CheckExpectedBytes(GetNextInventoryItemRva,
            getNextInventoryItemExpected.data(), getNextInventoryItemExpected.size())) {
        context->LogError("AdvancedItemTooltips: socket inventory ABI signature mismatch for build 92777.");
        return false;
    }
    if (!context->CheckExpectedBytes(EnsureStringCapacityRva,
            ensureStringCapacityExpected.data(), ensureStringCapacityExpected.size())) {
        context->LogError("AdvancedItemTooltips: native string ABI signature mismatch for build 92777.");
        return false;
    }
    BuildItemTooltip = At<BuildItemTooltipFn>(BuildItemTooltipRva);
    EnsureStringCapacity = At<EnsureStringCapacityFn>(EnsureStringCapacityRva);
    GetRunesTxtRecordFromItem = At<GetRunesTxtRecordFromItemFn>(GetRunesTxtRecordFromItemRva);
    GetLocalizedString = At<GetLocalizedStringFn>(GetLocalizedStringRva);
    GetLocalizedStringByKey = At<GetLocalizedStringByKeyFn>(GetLocalizedStringByKeyRva);
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
    if (!InstallTooltipCallSites()) {
        context->LogError(
            "AdvancedItemTooltips: autonomous tooltip call-sites are unavailable; plugin refused.");
        return false;
    }
    context->LogInfo(
        "AdvancedItemTooltips 2.2.0 autonomous pipeline active for D2R 3.2.92777 (7/7 call-sites)." );
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    GetMaxSockets = nullptr;
    GetUnitStat = nullptr;
    GetItemDataContext = nullptr;
    GetItemData = nullptr;
    GetItemsTxtRecord = nullptr;
    GetRunesTxtRecordFromItem = nullptr;
    GetLocalizedString = nullptr;
    GetLocalizedStringByKey = nullptr;
    BuildItemTooltip = nullptr;
    EnsureStringCapacity = nullptr;
    {
        std::scoped_lock lock(RunewordNamesMutex);
        RunewordKeyByLocalizedName.clear();
        RunewordNamesBuilt = false;
    }
    CatalogLoaded = false;
    Context = nullptr;
}
