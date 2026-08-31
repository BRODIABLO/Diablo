#pragma once

#include <toml++/toml.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ruffneckk::autosort {

enum class Anchor : std::uint8_t {
    TopLeft,
    TopMiddle,
    TopRight,
    MiddleLeft,
    Middle,
    MiddleRight,
    BottomLeft,
    BottomMiddle,
    BottomRight,
    Ignore,
};

enum class Category : std::uint8_t {
    Armor,
    Weapons,
    Jewelry,
    Charms,
    Potions,
    Keys,
    Scrolls,
    Books,
    Runes,
    Gems,
    Jewels,
    QuestItems,
    Misc,
    Unknown,
    Count,
};

enum class Subgroup : std::uint8_t {
    BodyArmor,
    Helms,
    Shields,
    Gloves,
    Belts,
    Boots,
    OtherArmor,
    Swords,
    Axes,
    Maces,
    Daggers,
    Scepters,
    Wands,
    Staves,
    Spears,
    Polearms,
    Bows,
    Crossbows,
    Javelins,
    ThrowingWeapons,
    AssassinClaws,
    Orbs,
    OtherWeapons,
    Rings,
    Amulets,
    OtherJewelry,
    SmallCharms,
    LargeCharms,
    GrandCharms,
    OtherCharms,
    HealingPotions,
    ManaPotions,
    RejuvenationPotions,
    UtilityPotions,
    ThrowingPotions,
    OtherPotions,
    Keys,
    Scrolls,
    Books,
    Runes,
    Gems,
    Jewels,
    QuestItems,
    Misc,
    Unknown,
    Count,
};

struct SubgroupDescriptor {
    Subgroup subgroup;
    Category category;
    std::string_view name;
};

inline constexpr std::size_t CategoryCount =
    static_cast<std::size_t>(Category::Count);

inline constexpr std::size_t SubgroupCount =
    static_cast<std::size_t>(Subgroup::Count);

inline constexpr std::array<Category, CategoryCount> DefaultCategoryOrder{
    Category::Armor,
    Category::Weapons,
    Category::Jewelry,
    Category::Runes,
    Category::Gems,
    Category::Jewels,
    Category::Potions,
    Category::Keys,
    Category::Scrolls,
    Category::Books,
    Category::QuestItems,
    Category::Misc,
    Category::Charms,
    Category::Unknown,
};

inline constexpr std::array<Anchor, CategoryCount> DefaultAnchors{
    Anchor::TopLeft,
    Anchor::TopLeft,
    Anchor::BottomLeft,
    Anchor::BottomRight,
    Anchor::TopRight,
    Anchor::Middle,
    Anchor::Middle,
    Anchor::Middle,
    Anchor::TopMiddle,
    Anchor::TopMiddle,
    Anchor::TopMiddle,
    Anchor::Middle,
    Anchor::Middle,
    Anchor::Middle,
};

inline constexpr std::array<std::string_view, CategoryCount> CategoryNames{
    "armor",
    "weapons",
    "jewelry",
    "charms",
    "potions",
    "keys",
    "scrolls",
    "books",
    "runes",
    "gems",
    "jewels",
    "quest_items",
    "misc",
    "unknown",
};

inline constexpr std::array<SubgroupDescriptor, 45> SubgroupDescriptors{{
    {Subgroup::BodyArmor, Category::Armor, "body_armor"},
    {Subgroup::Helms, Category::Armor, "helms"},
    {Subgroup::Shields, Category::Armor, "shields"},
    {Subgroup::Gloves, Category::Armor, "gloves"},
    {Subgroup::Belts, Category::Armor, "belts"},
    {Subgroup::Boots, Category::Armor, "boots"},
    {Subgroup::OtherArmor, Category::Armor, "other_armor"},
    {Subgroup::Swords, Category::Weapons, "swords"},
    {Subgroup::Axes, Category::Weapons, "axes"},
    {Subgroup::Maces, Category::Weapons, "maces"},
    {Subgroup::Daggers, Category::Weapons, "daggers"},
    {Subgroup::Scepters, Category::Weapons, "scepters"},
    {Subgroup::Wands, Category::Weapons, "wands"},
    {Subgroup::Staves, Category::Weapons, "staves"},
    {Subgroup::Spears, Category::Weapons, "spears"},
    {Subgroup::Polearms, Category::Weapons, "polearms"},
    {Subgroup::Bows, Category::Weapons, "bows"},
    {Subgroup::Crossbows, Category::Weapons, "crossbows"},
    {Subgroup::Javelins, Category::Weapons, "javelins"},
    {Subgroup::ThrowingWeapons, Category::Weapons, "throwing_weapons"},
    {Subgroup::AssassinClaws, Category::Weapons, "assassin_claws"},
    {Subgroup::Orbs, Category::Weapons, "orbs"},
    {Subgroup::OtherWeapons, Category::Weapons, "other_weapons"},
    {Subgroup::Rings, Category::Jewelry, "rings"},
    {Subgroup::Amulets, Category::Jewelry, "amulets"},
    {Subgroup::OtherJewelry, Category::Jewelry, "other_jewelry"},
    {Subgroup::SmallCharms, Category::Charms, "small_charms"},
    {Subgroup::LargeCharms, Category::Charms, "large_charms"},
    {Subgroup::GrandCharms, Category::Charms, "grand_charms"},
    {Subgroup::OtherCharms, Category::Charms, "other_charms"},
    {Subgroup::HealingPotions, Category::Potions, "healing"},
    {Subgroup::ManaPotions, Category::Potions, "mana"},
    {Subgroup::RejuvenationPotions, Category::Potions, "rejuvenation"},
    {Subgroup::UtilityPotions, Category::Potions, "utility"},
    {Subgroup::ThrowingPotions, Category::Potions, "throwing"},
    {Subgroup::OtherPotions, Category::Potions, "other_potions"},
    {Subgroup::Keys, Category::Keys, "keys"},
    {Subgroup::Scrolls, Category::Scrolls, "scrolls"},
    {Subgroup::Books, Category::Books, "books"},
    {Subgroup::Runes, Category::Runes, "runes"},
    {Subgroup::Gems, Category::Gems, "gems"},
    {Subgroup::Jewels, Category::Jewels, "jewels"},
    {Subgroup::QuestItems, Category::QuestItems, "quest_items"},
    {Subgroup::Misc, Category::Misc, "misc"},
    {Subgroup::Unknown, Category::Unknown, "unknown"},
}};

inline constexpr std::array<std::pair<std::string_view, Anchor>, 10>
        AnchorNames{{
    {"top_left", Anchor::TopLeft},
    {"top_middle", Anchor::TopMiddle},
    {"top_right", Anchor::TopRight},
    {"middle_left", Anchor::MiddleLeft},
    {"middle", Anchor::Middle},
    {"middle_right", Anchor::MiddleRight},
    {"bottom_left", Anchor::BottomLeft},
    {"bottom_middle", Anchor::BottomMiddle},
    {"bottom_right", Anchor::BottomRight},
    {"ignore", Anchor::Ignore},
}};

inline constexpr std::array<std::pair<std::string_view, std::uint8_t>, 9>
        QualityNames{{
    {"low_quality", 1},
    {"normal", 2},
    {"superior", 3},
    {"magic", 4},
    {"set", 5},
    {"rare", 6},
    {"unique", 7},
    {"crafted", 8},
    {"tempered", 9},
}};

constexpr std::size_t Index(Category category) noexcept {
    return static_cast<std::size_t>(category);
}

constexpr std::size_t Index(Subgroup subgroup) noexcept {
    return static_cast<std::size_t>(subgroup);
}

inline std::optional<Category> ParseCategory(std::string_view text) noexcept {
    for (std::size_t index = 0; index < CategoryNames.size(); ++index) {
        if (CategoryNames[index] == text) {
            return static_cast<Category>(index);
        }
    }
    return std::nullopt;
}

inline std::string_view CategoryName(Category category) noexcept {
    return CategoryNames[Index(category)];
}

inline std::optional<Subgroup> ParseSubgroup(
        Category category,
        std::string_view text) noexcept {
    for (const auto& descriptor : SubgroupDescriptors) {
        if (descriptor.category == category && descriptor.name == text) {
            return descriptor.subgroup;
        }
    }
    return std::nullopt;
}

inline std::string_view SubgroupName(Subgroup subgroup) noexcept {
    for (const auto& descriptor : SubgroupDescriptors) {
        if (descriptor.subgroup == subgroup) return descriptor.name;
    }
    return "unknown";
}

inline std::array<std::vector<Subgroup>, CategoryCount>
DefaultSubgroupOrder() {
    std::array<std::vector<Subgroup>, CategoryCount> result;
    for (const auto& descriptor : SubgroupDescriptors) {
        result[Index(descriptor.category)].push_back(descriptor.subgroup);
    }
    return result;
}

inline std::optional<Anchor> ParseAnchor(std::string_view text) noexcept {
    for (const auto& [name, anchor] : AnchorNames) {
        if (name == text) return anchor;
    }
    return std::nullopt;
}

inline std::string_view AnchorName(Anchor anchor) noexcept {
    for (const auto& [name, candidate] : AnchorNames) {
        if (candidate == anchor) return name;
    }
    return "middle";
}

inline std::optional<std::uint8_t> ParseQuality(
        std::string_view text) noexcept {
    for (const auto& [name, quality] : QualityNames) {
        if (name == text) return quality;
    }
    return std::nullopt;
}

constexpr std::uint32_t NormalizePackedCode(
        std::uint32_t packed) noexcept {
    std::uint32_t normalized{};
    for (unsigned index = 0; index < 4; ++index) {
        const auto character = static_cast<std::uint8_t>(
            (packed >> (index * 8)) & 0xFF);
        if (character == 0 || character == ' ') break;
        normalized |= static_cast<std::uint32_t>(character) << (index * 8);
    }
    return normalized;
}

inline std::optional<std::uint32_t> PackCode(
        std::string_view code) noexcept {
    if (code.empty() || code.size() > 4) return std::nullopt;
    // Runtime base-item codes and compiled ItemTypes records do not use the
    // same padding for short codes: ITEMS_GetItemCode returns NUL padding,
    // while ItemTypes records use ASCII spaces. Keep one NUL-padded canonical
    // representation and normalize every runtime/table value at the boundary.
    std::uint32_t packed{};
    for (std::size_t index = 0; index < code.size(); ++index) {
        const auto character = static_cast<unsigned char>(code[index]);
        if (character <= 0x20 || character > 0x7E) return std::nullopt;
        packed |= static_cast<std::uint32_t>(character) << (index * 8);
    }
    return packed;
}

inline std::string UnpackCode(std::uint32_t packed) {
    packed = NormalizePackedCode(packed);
    std::string result;
    for (unsigned index = 0; index < 4; ++index) {
        const auto character =
            static_cast<char>((packed >> (index * 8)) & 0xFF);
        if (character == '\0') break;
        result.push_back(character);
    }
    return result;
}

constexpr std::uint32_t NaturalCodeSortKey(
        std::uint32_t packed) noexcept {
    return ((packed & 0x000000FFu) << 24)
        | ((packed & 0x0000FF00u) << 8)
        | ((packed & 0x00FF0000u) >> 8)
        | ((packed & 0xFF000000u) >> 24);
}

constexpr std::uint64_t SemanticItemSortKey(
        Category category,
        std::uint32_t packed) noexcept {
    const auto natural = static_cast<std::uint64_t>(
        NaturalCodeSortKey(packed));
    if (category != Category::Potions) return natural;

    const auto first = static_cast<std::uint8_t>(packed & 0xFF);
    const auto second = static_cast<std::uint8_t>((packed >> 8) & 0xFF);
    const auto third = static_cast<std::uint8_t>((packed >> 16) & 0xFF);
    const auto ranked = [&](std::uint8_t family, std::uint8_t tier) {
        return (static_cast<std::uint64_t>(family) << 56)
            | (static_cast<std::uint64_t>(tier) << 48)
            | natural;
    };
    if (first == 'h' && second == 'p'
            && third >= '1' && third <= '9') {
        return ranked(0, static_cast<std::uint8_t>(third - '0'));
    }
    if (first == 'm' && second == 'p'
            && third >= '1' && third <= '9') {
        return ranked(1, static_cast<std::uint8_t>(third - '0'));
    }
    if (packed == PackCode("rvs").value()) return ranked(2, 1);
    if (packed == PackCode("rvl").value()) return ranked(2, 2);
    if (packed == PackCode("vps").value()) return ranked(3, 0);
    if (packed == PackCode("yps").value()) return ranked(4, 0);
    if (packed == PackCode("wms").value()) return ranked(5, 0);
    return ranked(0x7F, 0);
}

struct CustomRule {
    std::string name;
    Anchor anchor{Anchor::Middle};
    std::vector<std::uint32_t> itemCodes;
    std::vector<std::uint32_t> itemTypeCodes;
    std::vector<std::uint8_t> qualities;
};

struct ExclusionRule {
    std::string name;
    std::vector<std::uint32_t> itemCodes;
    std::vector<std::uint32_t> itemTypeCodes;
    std::vector<std::uint8_t> qualities;
};

struct ButtonConfig {
    bool enabled{};
    std::int32_t x{3};
    std::int32_t y{813};
};

struct FixedRegionConfig {
    std::uint8_t inventoryRightColumns{};
};

struct Config {
    bool enabled{true};
    bool optimizeFreeSpace{true};
    std::array<Anchor, CategoryCount> anchors{DefaultAnchors};
    std::array<Category, CategoryCount> categoryOrder{DefaultCategoryOrder};
    std::array<std::vector<Subgroup>, CategoryCount> subgroupOrder{
        DefaultSubgroupOrder()};
    std::array<std::optional<Anchor>, SubgroupCount> subgroupAnchors{};
    bool controllerActionEnabled{true};
    bool controlsActionEnabled{true};
    ButtonConfig button{};
    FixedRegionConfig fixedRegions{};
    bool diagnostics{};
    bool diagnosticsDryRun{};
    bool diagnosticsLogItems{};
    std::vector<ExclusionRule> exclusions;
    std::vector<CustomRule> customRules;
};

struct ItemTraits {
    std::uint32_t itemCode{};
    std::vector<std::uint32_t> itemTypeCodes;
    std::uint8_t quality{};
};

struct Classification {
    Anchor anchor{Anchor::Middle};
    Category category{Category::Unknown};
    Subgroup subgroup{Subgroup::Unknown};
    std::size_t groupOrder{};
    std::size_t subgroupOrder{};
    bool excluded{};
    std::optional<std::size_t> exclusionRuleIndex;
    std::optional<std::size_t> customRuleIndex;
};

inline bool Contains(
        const std::vector<std::uint32_t>& values,
        std::uint32_t value) noexcept {
    return std::find(values.begin(), values.end(), value) != values.end();
}

inline bool HasType(
        const ItemTraits& item,
        std::uint32_t typeCode) noexcept {
    return Contains(item.itemTypeCodes, typeCode);
}

template <typename Rule>
inline bool MatchesRule(
        const Rule& rule,
        const ItemTraits& item) noexcept {
    if (!rule.itemCodes.empty()
            && !Contains(rule.itemCodes, item.itemCode)) {
        return false;
    }
    if (!rule.itemTypeCodes.empty()) {
        const auto matchesType = std::any_of(
            rule.itemTypeCodes.begin(),
            rule.itemTypeCodes.end(),
            [&](std::uint32_t typeCode) {
                return HasType(item, typeCode);
            });
        if (!matchesType) return false;
    }
    if (!rule.qualities.empty()
            && std::find(
                rule.qualities.begin(),
                rule.qualities.end(),
                item.quality) == rule.qualities.end()) {
        return false;
    }
    return true;
}

inline Category ClassifyBuiltIn(const ItemTraits& item) noexcept {
    const auto ring = PackCode("ring").value();
    const auto amulet = PackCode("amul").value();
    const auto jewelry = PackCode("jwly").value();
    const auto charm = PackCode("char").value();
    const auto healingPotion = PackCode("hpot").value();
    const auto manaPotion = PackCode("mpot").value();
    const auto rejuvenationPotion = PackCode("rpot").value();
    const auto antidotePotion = PackCode("apot").value();
    const auto thawingPotion = PackCode("wpot").value();
    const auto missilePotion = PackCode("tpot").value();
    const auto staminaPotion = PackCode("vps").value();
    const auto key = PackCode("key").value();
    const auto customKeys = PackCode("ukey").value();
    const auto scroll = PackCode("scro").value();
    const auto book = PackCode("book").value();
    const auto gem = PackCode("gem").value();
    const auto rune = PackCode("rune").value();
    const auto jewel = PackCode("jewl").value();
    const auto quest = PackCode("ques").value();
    const auto armor = PackCode("armo").value();
    const auto weapon = PackCode("weap").value();
    const auto misc = PackCode("misc").value();

    if (HasType(item, ring) || HasType(item, amulet)
            || HasType(item, jewelry)) {
        return Category::Jewelry;
    }
    if (HasType(item, charm)) return Category::Charms;
    // Do not test the broad `poti` parent directly. BKVince reuses its `spot`
    // child for currencies and quest materials; only concrete potion families
    // plus the canonical stamina-potion item code are safe built-in matches.
    if (HasType(item, healingPotion)
            || HasType(item, manaPotion)
            || HasType(item, rejuvenationPotion)
            || HasType(item, antidotePotion)
            || HasType(item, thawingPotion)
            || HasType(item, missilePotion)
            || item.itemCode == staminaPotion) {
        return Category::Potions;
    }
    if (HasType(item, key) || HasType(item, customKeys)) {
        return Category::Keys;
    }
    if (HasType(item, scroll)) return Category::Scrolls;
    if (HasType(item, book)) return Category::Books;
    if (HasType(item, rune)) return Category::Runes;
    if (HasType(item, gem)) return Category::Gems;
    if (HasType(item, jewel)) return Category::Jewels;
    if (HasType(item, quest)) return Category::QuestItems;
    if (HasType(item, armor)) return Category::Armor;
    if (HasType(item, weapon)) return Category::Weapons;
    if (HasType(item, misc)) return Category::Misc;
    return Category::Unknown;
}

inline Subgroup ClassifyBuiltInSubgroup(
        Category category,
        const ItemTraits& item) noexcept {
    switch (category) {
    case Category::Armor:
        if (HasType(item, PackCode("tors").value())) {
            return Subgroup::BodyArmor;
        }
        if (HasType(item, PackCode("helm").value())) {
            return Subgroup::Helms;
        }
        if (HasType(item, PackCode("shld").value())) {
            return Subgroup::Shields;
        }
        if (HasType(item, PackCode("glov").value())) {
            return Subgroup::Gloves;
        }
        if (HasType(item, PackCode("belt").value())) {
            return Subgroup::Belts;
        }
        if (HasType(item, PackCode("boot").value())) {
            return Subgroup::Boots;
        }
        return Subgroup::OtherArmor;
    case Category::Weapons:
        // Check the most specific throwing families before their axe, knife,
        // and spear parents in the compiled item-type equivalence graph.
        if (HasType(item, PackCode("tkni").value())
                || HasType(item, PackCode("taxe").value())) {
            return Subgroup::ThrowingWeapons;
        }
        if (HasType(item, PackCode("jave").value())) {
            return Subgroup::Javelins;
        }
        if (HasType(item, PackCode("swor").value())) {
            return Subgroup::Swords;
        }
        if (HasType(item, PackCode("axe").value())) {
            return Subgroup::Axes;
        }
        if (HasType(item, PackCode("mace").value())
                || HasType(item, PackCode("hamm").value())
                || HasType(item, PackCode("club").value())) {
            return Subgroup::Maces;
        }
        if (HasType(item, PackCode("knif").value())) {
            return Subgroup::Daggers;
        }
        if (HasType(item, PackCode("scep").value())) {
            return Subgroup::Scepters;
        }
        if (HasType(item, PackCode("wand").value())) {
            return Subgroup::Wands;
        }
        if (HasType(item, PackCode("staf").value())) {
            return Subgroup::Staves;
        }
        if (HasType(item, PackCode("spea").value())) {
            return Subgroup::Spears;
        }
        if (HasType(item, PackCode("pole").value())) {
            return Subgroup::Polearms;
        }
        if (HasType(item, PackCode("bow").value())) {
            return Subgroup::Bows;
        }
        if (HasType(item, PackCode("xbow").value())) {
            return Subgroup::Crossbows;
        }
        if (HasType(item, PackCode("h2h").value())) {
            return Subgroup::AssassinClaws;
        }
        if (HasType(item, PackCode("orb").value())) {
            return Subgroup::Orbs;
        }
        return Subgroup::OtherWeapons;
    case Category::Jewelry:
        if (HasType(item, PackCode("ring").value())) {
            return Subgroup::Rings;
        }
        if (HasType(item, PackCode("amul").value())) {
            return Subgroup::Amulets;
        }
        return Subgroup::OtherJewelry;
    case Category::Charms:
        if (item.itemCode == PackCode("cm1").value()) {
            return Subgroup::SmallCharms;
        }
        if (item.itemCode == PackCode("cm2").value()) {
            return Subgroup::LargeCharms;
        }
        if (item.itemCode == PackCode("cm3").value()) {
            return Subgroup::GrandCharms;
        }
        return Subgroup::OtherCharms;
    case Category::Potions:
        // Rejuvenation potions inherit both hpot and mpot in the compiled type
        // graph, so their specific code/type must win before those parents.
        if (item.itemCode == PackCode("rvs").value()
                || item.itemCode == PackCode("rvl").value()
                || HasType(item, PackCode("rpot").value())) {
            return Subgroup::RejuvenationPotions;
        }
        if (item.itemCode == PackCode("hp1").value()
                || item.itemCode == PackCode("hp2").value()
                || item.itemCode == PackCode("hp3").value()
                || item.itemCode == PackCode("hp4").value()
                || item.itemCode == PackCode("hp5").value()
                || HasType(item, PackCode("hpot").value())) {
            return Subgroup::HealingPotions;
        }
        if (item.itemCode == PackCode("mp1").value()
                || item.itemCode == PackCode("mp2").value()
                || item.itemCode == PackCode("mp3").value()
                || item.itemCode == PackCode("mp4").value()
                || item.itemCode == PackCode("mp5").value()
                || HasType(item, PackCode("mpot").value())) {
            return Subgroup::ManaPotions;
        }
        if (HasType(item, PackCode("tpot").value())) {
            return Subgroup::ThrowingPotions;
        }
        if (HasType(item, PackCode("apot").value())
                || HasType(item, PackCode("wpot").value())
                || item.itemCode == PackCode("vps").value()
                || item.itemCode == PackCode("yps").value()
                || item.itemCode == PackCode("wms").value()) {
            return Subgroup::UtilityPotions;
        }
        return Subgroup::OtherPotions;
    case Category::Keys: return Subgroup::Keys;
    case Category::Scrolls: return Subgroup::Scrolls;
    case Category::Books: return Subgroup::Books;
    case Category::Runes: return Subgroup::Runes;
    case Category::Gems: return Subgroup::Gems;
    case Category::Jewels: return Subgroup::Jewels;
    case Category::QuestItems: return Subgroup::QuestItems;
    case Category::Misc: return Subgroup::Misc;
    case Category::Unknown:
    case Category::Count:
        return Subgroup::Unknown;
    }
    return Subgroup::Unknown;
}

inline Classification Classify(
        const Config& config,
        const ItemTraits& item) noexcept {
    const auto category = ClassifyBuiltIn(item);
    const auto subgroup = ClassifyBuiltInSubgroup(category, item);
    const auto categoryIterator = std::find(
        config.categoryOrder.begin(),
        config.categoryOrder.end(),
        category);
    const auto categoryRank = static_cast<std::size_t>(
        categoryIterator - config.categoryOrder.begin());
    const auto& subgroupOrder = config.subgroupOrder[Index(category)];
    const auto subgroupIterator = std::find(
        subgroupOrder.begin(), subgroupOrder.end(), subgroup);
    const auto subgroupRank = static_cast<std::size_t>(
        subgroupIterator - subgroupOrder.begin());

    for (std::size_t index = 0; index < config.exclusions.size(); ++index) {
        if (MatchesRule(config.exclusions[index], item)) {
            return Classification{
                .anchor = Anchor::Ignore,
                .category = category,
                .subgroup = subgroup,
                .groupOrder = config.customRules.size() + categoryRank,
                .subgroupOrder = subgroupRank,
                .excluded = true,
                .exclusionRuleIndex = index,
            };
        }
    }

    for (std::size_t index = 0; index < config.customRules.size(); ++index) {
        if (MatchesRule(config.customRules[index], item)) {
            return Classification{
                .anchor = config.customRules[index].anchor,
                .category = Category::Unknown,
                .subgroup = Subgroup::Unknown,
                .groupOrder = index,
                .subgroupOrder = 0,
                .customRuleIndex = index,
            };
        }
    }
    const auto subgroupAnchor = config.subgroupAnchors[Index(subgroup)];
    return Classification{
        .anchor = subgroupAnchor.value_or(config.anchors[Index(category)]),
        .category = category,
        .subgroup = subgroup,
        .groupOrder = config.customRules.size() + categoryRank,
        .subgroupOrder = subgroupRank,
        .customRuleIndex = std::nullopt,
    };
}

inline std::vector<std::filesystem::path> BuildConfigCandidates(
        const std::filesystem::path& activeModConfigDirectory,
        const std::filesystem::path& scopeConfigDirectory,
        const std::filesystem::path& globalConfigDirectory,
        const std::filesystem::path& fileName) {
    std::vector<std::filesystem::path> result;
    const auto append = [&](const std::filesystem::path& directory) {
        if (directory.empty()) return;
        const auto candidate = (directory / fileName).lexically_normal();
        if (std::find(result.begin(), result.end(), candidate)
                == result.end()) {
            result.emplace_back(candidate);
        }
    };
    append(activeModConfigDirectory);
    append(scopeConfigDirectory);
    append(globalConfigDirectory);
    return result;
}

inline bool ReadBoolean(
        const toml::table& table,
        const char* key,
        bool& destination,
        std::string& error,
        std::string_view path) {
    const auto* node = table.get(key);
    if (!node) return true;
    const auto value = node->value<bool>();
    if (!value) {
        error = std::string(path) + key + " must be true or false";
        return false;
    }
    destination = *value;
    return true;
}

inline bool ReadCoordinate(
        const toml::table& table,
        const char* key,
        std::int32_t& destination,
        std::string& error) {
    const auto* node = table.get(key);
    if (!node) return true;
    const auto value = node->value<std::int64_t>();
    if (!value || *value < -32768 || *value > 32767) {
        error = std::string("button.") + key
            + " must be an integer from -32768 to 32767";
        return false;
    }
    destination = static_cast<std::int32_t>(*value);
    return true;
}

inline bool ReadCodeArray(
        const toml::table& table,
        const char* key,
        std::vector<std::uint32_t>& destination,
        std::string& error,
        std::string_view ruleKind,
        std::string_view ruleName) {
    const auto* node = table.get(key);
    if (!node) return true;
    const auto* array = node->as_array();
    if (!array) {
        error = std::string(ruleKind) + " '" + std::string(ruleName) + "'."
            + key + " must be a string array";
        return false;
    }
    for (const auto& entry : *array) {
        const auto text = entry.value<std::string>();
        const auto packed = text ? PackCode(*text) : std::nullopt;
        if (!packed) {
            error = std::string(ruleKind) + " '" + std::string(ruleName) + "'."
                + key + " entries must be one to four printable non-space ASCII characters";
            return false;
        }
        if (Contains(destination, *packed)) {
            error = std::string(ruleKind) + " '" + std::string(ruleName) + "'."
                + key + " must not contain duplicates";
            return false;
        }
        destination.push_back(*packed);
    }
    return true;
}

inline bool ReadQualityArray(
        const toml::table& table,
        std::vector<std::uint8_t>& destination,
        std::string& error,
        std::string_view ruleKind,
        std::string_view ruleName) {
    const auto* node = table.get("qualities");
    if (!node) return true;
    const auto* qualities = node->as_array();
    if (!qualities) {
        error = std::string(ruleKind) + " '" + std::string(ruleName)
            + "'.qualities must be a string array";
        return false;
    }
    for (const auto& entry : *qualities) {
        const auto text = entry.value<std::string>();
        const auto quality = text ? ParseQuality(*text) : std::nullopt;
        if (!quality
                || std::find(
                    destination.begin(), destination.end(), *quality)
                    != destination.end()) {
            error = std::string(ruleKind) + " '" + std::string(ruleName)
                + "'.qualities contains an invalid or duplicate value";
            return false;
        }
        destination.push_back(*quality);
    }
    return true;
}

template <typename Rule>
inline bool HasSelectors(const Rule& rule) noexcept {
    return !rule.itemCodes.empty()
        || !rule.itemTypeCodes.empty()
        || !rule.qualities.empty();
}

inline bool ParseToml(
        std::string_view input,
        Config& result,
        std::string& error) {
    try {
        const auto root = toml::parse(input);
        constexpr std::string_view topLevelKeys[]{
            "enabled", "sorting", "anchors", "input", "button",
            "diagnostics", "subgroup_order", "subgroup_anchors", "exclusions",
            "custom_groups", "custom_rules", "fixed_regions",
        };
        for (const auto& [key, value] : root) {
            (void)value;
            if (std::find(
                    std::begin(topLevelKeys),
                    std::end(topLevelKeys),
                    key.str()) == std::end(topLevelKeys)) {
                error = "unknown top-level setting or section: "
                    + std::string(key.str());
                return false;
            }
        }

        Config parsed{};
        if (const auto* node = root.get("enabled")) {
            const auto enabled = node->value<bool>();
            if (!enabled) {
                error = "enabled must be true or false";
                return false;
            }
            parsed.enabled = *enabled;
        }

        if (const auto* node = root.get("sorting")) {
            const auto* sorting = node->as_table();
            if (!sorting) {
                error = "sorting must be a TOML table";
                return false;
            }
            for (const auto& [key, value] : *sorting) {
                (void)value;
                if (key != "optimize_free_space"
                        && key != "category_order") {
                    error = "unknown setting: sorting."
                        + std::string(key.str());
                    return false;
                }
            }
            if (!ReadBoolean(
                    *sorting,
                    "optimize_free_space",
                    parsed.optimizeFreeSpace,
                    error,
                    "sorting.")) {
                return false;
            }
            if (const auto* orderNode = sorting->get("category_order")) {
                const auto* order = orderNode->as_array();
                if (!order || order->size() != CategoryCount) {
                    error = "sorting.category_order must contain every built-in category exactly once";
                    return false;
                }
                std::array<bool, CategoryCount> seen{};
                for (std::size_t index = 0; index < order->size(); ++index) {
                    const auto name = (*order)[index].value<std::string>();
                    const auto category = name
                        ? ParseCategory(*name)
                        : std::nullopt;
                    if (!category || seen[Index(*category)]) {
                        error = "sorting.category_order must contain every built-in category exactly once";
                        return false;
                    }
                    seen[Index(*category)] = true;
                    parsed.categoryOrder[index] = *category;
                }
            }
        }

        if (const auto* node = root.get("fixed_regions")) {
            const auto* fixedRegions = node->as_table();
            if (!fixedRegions) {
                error = "fixed_regions must be a TOML table";
                return false;
            }
            for (const auto& [key, value] : *fixedRegions) {
                (void)value;
                if (key != "inventory") {
                    error = "unknown setting: fixed_regions."
                        + std::string(key.str());
                    return false;
                }
            }
            if (const auto* inventoryNode = fixedRegions->get("inventory")) {
                const auto* inventory = inventoryNode->as_table();
                if (!inventory) {
                    error = "fixed_regions.inventory must be a TOML table";
                    return false;
                }
                for (const auto& [key, value] : *inventory) {
                    (void)value;
                    if (key != "right_columns") {
                        error = "unknown setting: fixed_regions.inventory."
                            + std::string(key.str());
                        return false;
                    }
                }
                if (const auto* columnsNode = inventory->get("right_columns")) {
                    const auto columns = columnsNode->value<std::int64_t>();
                    if (!columns || *columns < 0 || *columns > 15) {
                        error = "fixed_regions.inventory.right_columns must be an integer from 0 to 15";
                        return false;
                    }
                    parsed.fixedRegions.inventoryRightColumns =
                        static_cast<std::uint8_t>(*columns);
                }
            }
        }

        if (const auto* node = root.get("anchors")) {
            const auto* anchors = node->as_table();
            if (!anchors) {
                error = "anchors must be a TOML table";
                return false;
            }
            for (const auto& [key, value] : *anchors) {
                const auto category = ParseCategory(key.str());
                const auto text = value.value<std::string>();
                const auto anchor = text ? ParseAnchor(*text) : std::nullopt;
                if (!category) {
                    error = "unknown setting: anchors."
                        + std::string(key.str());
                    return false;
                }
                if (!anchor) {
                    error = "anchors." + std::string(key.str())
                        + " has an invalid destination";
                    return false;
                }
                parsed.anchors[Index(*category)] = *anchor;
            }
        }

        if (const auto* node = root.get("subgroup_order")) {
            const auto* subgroupTable = node->as_table();
            if (!subgroupTable) {
                error = "subgroup_order must be a TOML table";
                return false;
            }
            for (const auto& [key, value] : *subgroupTable) {
                const auto category = ParseCategory(key.str());
                const auto* order = value.as_array();
                if (!category || !order) {
                    error = "unknown or invalid subgroup_order entry: "
                        + std::string(key.str());
                    return false;
                }
                const auto& defaults = parsed.subgroupOrder[Index(*category)];
                if (defaults.size() <= 1 || order->size() != defaults.size()) {
                    error = "subgroup_order." + std::string(key.str())
                        + " must contain every built-in subgroup exactly once";
                    return false;
                }
                std::vector<Subgroup> parsedOrder;
                parsedOrder.reserve(order->size());
                for (const auto& entry : *order) {
                    const auto name = entry.value<std::string>();
                    const auto subgroup = name
                        ? ParseSubgroup(*category, *name)
                        : std::nullopt;
                    if (!subgroup
                            || std::find(
                                parsedOrder.begin(),
                                parsedOrder.end(),
                                *subgroup) != parsedOrder.end()) {
                        error = "subgroup_order." + std::string(key.str())
                            + " must contain every built-in subgroup exactly once";
                        return false;
                    }
                    parsedOrder.push_back(*subgroup);
                }
                parsed.subgroupOrder[Index(*category)] =
                    std::move(parsedOrder);
            }
        }

        if (const auto* node = root.get("subgroup_anchors")) {
            const auto* categoryTables = node->as_table();
            if (!categoryTables) {
                error = "subgroup_anchors must be a TOML table";
                return false;
            }
            for (const auto& [categoryKey, categoryValue] : *categoryTables) {
                const auto category = ParseCategory(categoryKey.str());
                const auto* subgroupTable = categoryValue.as_table();
                if (!category || !subgroupTable) {
                    error = "unknown or invalid subgroup_anchors entry: "
                        + std::string(categoryKey.str());
                    return false;
                }
                for (const auto& [subgroupKey, anchorValue] : *subgroupTable) {
                    const auto subgroup = ParseSubgroup(
                        *category, subgroupKey.str());
                    const auto anchorText = anchorValue.value<std::string>();
                    const auto anchor = anchorText
                        ? ParseAnchor(*anchorText)
                        : std::nullopt;
                    if (!subgroup) {
                        error = "unknown setting: subgroup_anchors."
                            + std::string(categoryKey.str()) + "."
                            + std::string(subgroupKey.str());
                        return false;
                    }
                    if (!anchor) {
                        error = "subgroup_anchors."
                            + std::string(categoryKey.str()) + "."
                            + std::string(subgroupKey.str())
                            + " has an invalid destination";
                        return false;
                    }
                    parsed.subgroupAnchors[Index(*subgroup)] = *anchor;
                }
            }
        }

        if (const auto* node = root.get("input")) {
            const auto* inputTable = node->as_table();
            if (!inputTable) {
                error = "input must be a TOML table";
                return false;
            }
            for (const auto& [key, value] : *inputTable) {
                (void)value;
                if (key != "controller_action_enabled"
                        && key != "controls_action_enabled") {
                    error = "unknown setting: input."
                        + std::string(key.str());
                    return false;
                }
            }
            if (!ReadBoolean(
                    *inputTable,
                    "controller_action_enabled",
                    parsed.controllerActionEnabled,
                    error,
                    "input.")
                    || !ReadBoolean(
                        *inputTable,
                        "controls_action_enabled",
                        parsed.controlsActionEnabled,
                        error,
                        "input.")) {
                return false;
            }
        }

        if (const auto* node = root.get("button")) {
            const auto* button = node->as_table();
            if (!button) {
                error = "button must be a TOML table";
                return false;
            }
            for (const auto& [key, value] : *button) {
                (void)value;
                if (key != "enabled" && key != "x" && key != "y") {
                    error = "unknown setting: button."
                        + std::string(key.str());
                    return false;
                }
            }
            if (!ReadBoolean(
                    *button,
                    "enabled",
                    parsed.button.enabled,
                    error,
                    "button.")
                    || !ReadCoordinate(
                        *button, "x", parsed.button.x, error)
                    || !ReadCoordinate(
                        *button, "y", parsed.button.y, error)) {
                return false;
            }
        }

        if (const auto* node = root.get("diagnostics")) {
            const auto* diagnostics = node->as_table();
            if (!diagnostics) {
                error = "diagnostics must be a TOML table";
                return false;
            }
            for (const auto& [key, value] : *diagnostics) {
                (void)value;
                if (key != "enabled"
                        && key != "dry_run"
                        && key != "log_items") {
                    error = "unknown setting: diagnostics."
                        + std::string(key.str());
                    return false;
                }
            }
            if (!ReadBoolean(
                    *diagnostics,
                    "enabled",
                    parsed.diagnostics,
                    error,
                    "diagnostics.")
                    || !ReadBoolean(
                        *diagnostics,
                        "dry_run",
                        parsed.diagnosticsDryRun,
                        error,
                        "diagnostics.")
                    || !ReadBoolean(
                        *diagnostics,
                        "log_items",
                        parsed.diagnosticsLogItems,
                        error,
                        "diagnostics.")) {
                return false;
            }
            if ((parsed.diagnosticsDryRun || parsed.diagnosticsLogItems)
                    && !parsed.diagnostics) {
                error = "diagnostics.dry_run and diagnostics.log_items require diagnostics.enabled = true";
                return false;
            }
        }

        if (const auto* node = root.get("exclusions")) {
            const auto* rules = node->as_array();
            if (!rules) {
                error = "exclusions must be an array of TOML tables";
                return false;
            }
            std::unordered_set<std::string> names;
            for (const auto& entry : *rules) {
                const auto* table = entry.as_table();
                if (!table) {
                    error = "exclusions entries must be TOML tables";
                    return false;
                }
                constexpr std::string_view allowedKeys[]{
                    "name", "item_codes", "item_type_codes", "qualities",
                };
                for (const auto& [key, value] : *table) {
                    (void)value;
                    if (std::find(
                            std::begin(allowedKeys),
                            std::end(allowedKeys),
                            key.str()) == std::end(allowedKeys)) {
                        error = "unknown exclusion setting: "
                            + std::string(key.str());
                        return false;
                    }
                }
                ExclusionRule rule{};
                const auto name = (*table)["name"].value<std::string>();
                if (!name || name->empty()) {
                    error = "every exclusion requires a non-empty name";
                    return false;
                }
                if (!names.insert(*name).second) {
                    error = "duplicate exclusion name: " + *name;
                    return false;
                }
                rule.name = *name;
                if (!ReadCodeArray(
                        *table,
                        "item_codes",
                        rule.itemCodes,
                        error,
                        "exclusion",
                        rule.name)
                        || !ReadCodeArray(
                            *table,
                            "item_type_codes",
                            rule.itemTypeCodes,
                            error,
                            "exclusion",
                            rule.name)
                        || !ReadQualityArray(
                            *table,
                            rule.qualities,
                            error,
                            "exclusion",
                            rule.name)) {
                    return false;
                }
                if (!HasSelectors(rule)) {
                    error = "exclusion '" + rule.name
                        + "' requires at least one selector";
                    return false;
                }
                parsed.exclusions.emplace_back(std::move(rule));
            }
        }

        const auto* customGroups = root.get("custom_groups");
        const auto* legacyCustomRules = root.get("custom_rules");
        if (customGroups && legacyCustomRules) {
            error = "custom_groups and legacy custom_rules cannot both be present";
            return false;
        }
        const auto* customNode = customGroups
            ? customGroups
            : legacyCustomRules;
        if (customNode) {
            const auto* rules = customNode->as_array();
            if (!rules) {
                error = "custom_groups must be an array of TOML tables";
                return false;
            }
            std::unordered_set<std::string> names;
            for (const auto& entry : *rules) {
                const auto* table = entry.as_table();
                if (!table) {
                    error = "custom_groups entries must be TOML tables";
                    return false;
                }
                constexpr std::string_view allowedKeys[]{
                    "name", "anchor", "item_codes", "item_type_codes",
                    "qualities",
                };
                for (const auto& [key, value] : *table) {
                    (void)value;
                    if (std::find(
                            std::begin(allowedKeys),
                            std::end(allowedKeys),
                            key.str()) == std::end(allowedKeys)) {
                        error = "unknown custom group setting: "
                            + std::string(key.str());
                        return false;
                    }
                }
                CustomRule rule{};
                const auto name = (*table)["name"].value<std::string>();
                const auto anchorName =
                    (*table)["anchor"].value<std::string>();
                const auto anchor = anchorName
                    ? ParseAnchor(*anchorName)
                    : std::nullopt;
                if (!name || name->empty()) {
                    error = "every custom group requires a non-empty name";
                    return false;
                }
                if (!names.insert(*name).second) {
                    error = "duplicate custom group name: " + *name;
                    return false;
                }
                if (!anchor) {
                    error = "custom group '" + *name
                        + "' requires a valid anchor";
                    return false;
                }
                rule.name = *name;
                rule.anchor = *anchor;
                if (!ReadCodeArray(
                        *table,
                        "item_codes",
                        rule.itemCodes,
                        error,
                        "custom group",
                        rule.name)
                        || !ReadCodeArray(
                            *table,
                            "item_type_codes",
                            rule.itemTypeCodes,
                            error,
                            "custom group",
                            rule.name)
                        || !ReadQualityArray(
                            *table,
                            rule.qualities,
                            error,
                            "custom group",
                            rule.name)) {
                    return false;
                }
                if (!HasSelectors(rule)) {
                    error = "custom group '" + rule.name
                        + "' requires at least one selector";
                    return false;
                }
                parsed.customRules.emplace_back(std::move(rule));
            }
        }

        result = std::move(parsed);
        error.clear();
        return true;
    } catch (const toml::parse_error& exception) {
        error = exception.description();
        return false;
    }
}

} // namespace ruffneckk::autosort
