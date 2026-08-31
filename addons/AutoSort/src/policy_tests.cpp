#include "autosort_config.hpp"
#include "autosort_planner.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace ruffneckk::autosort;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    Require(input.is_open(), "test input file could not be opened");
    return {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
}

GridItem MakeItem(
        std::uint32_t guid,
        std::uint8_t width,
        std::uint8_t height,
        std::uint8_t x,
        std::uint8_t y,
        Anchor anchor,
        std::size_t groupOrder,
        std::uint64_t itemOrder = 0,
        std::size_t subgroupOrder = 0) {
    return GridItem{
        .guid = guid,
        .width = width,
        .height = height,
        .current = {x, y},
        .anchor = anchor,
        .groupOrder = groupOrder,
        .subgroupOrder = subgroupOrder,
        .itemOrder = itemOrder,
    };
}

void TestPackCodeAndClassification() {
    const auto code = PackCode("r01");
    Require(code.has_value(), "three-character item code was rejected");
    Require(UnpackCode(*code) == "r01", "item code round-trip failed");
    Require(
        *PackCode("box") == 0x00786F62u
            && *PackCode("rvs") == 0x00737672u
            && *PackCode("r03") == 0x00333072u,
        "short item codes are not canonically NUL padded");
    Require(
        NormalizePackedCode(0x20786F62u) == *PackCode("box")
            && NormalizePackedCode(0x20737672u) == *PackCode("rvs")
            && NormalizePackedCode(0x20333072u) == *PackCode("r03"),
        "space-padded table codes do not normalize to runtime item codes");
    Require(
        NaturalCodeSortKey(*PackCode("hp1"))
                < NaturalCodeSortKey(*PackCode("hp2"))
            && NaturalCodeSortKey(*PackCode("hp2"))
                < NaturalCodeSortKey(*PackCode("hp3"))
            && NaturalCodeSortKey(*PackCode("r09"))
                < NaturalCodeSortKey(*PackCode("r10")),
        "natural item-code ordering regressed");
    Require(
        SemanticItemSortKey(Category::Potions, *PackCode("hp1"))
                < SemanticItemSortKey(Category::Potions, *PackCode("hp2"))
            && SemanticItemSortKey(Category::Potions, *PackCode("hp5"))
                < SemanticItemSortKey(Category::Potions, *PackCode("mp1"))
            && SemanticItemSortKey(Category::Potions, *PackCode("rvs"))
                < SemanticItemSortKey(Category::Potions, *PackCode("rvl")),
        "semantic potion family and tier ordering regressed");
    Require(!PackCode("").has_value(), "empty item code was accepted");
    Require(!PackCode("abcde").has_value(), "oversized item code was accepted");

    Config config{};
    ItemTraits potion{
        .itemCode = *PackCode("gpl"),
        .itemTypeCodes = {*PackCode("tpot"), *PackCode("weap")},
        .quality = 2,
    };
    Require(
        Classify(config, potion).category == Category::Potions
            && Classify(config, potion).subgroup
                == Subgroup::ThrowingPotions,
        "missile potion precedence regressed");

    ItemTraits rejuvenation{
        .itemCode = *PackCode("rvl"),
        .itemTypeCodes = {
            *PackCode("misc"),
            *PackCode("hpot"),
            *PackCode("mpot"),
            *PackCode("rpot"),
        },
        .quality = 2,
    };
    Require(
        Classify(config, rejuvenation).category == Category::Potions
            && Classify(config, rejuvenation).subgroup
                == Subgroup::RejuvenationPotions,
        "rejuvenation potion parents overrode its specific family");

    ItemTraits boots{
        .itemCode = *PackCode("bt1"),
        .itemTypeCodes = {*PackCode("boot"), *PackCode("armo")},
        .quality = 2,
    };
    Require(
        Classify(config, boots).category == Category::Armor
            && Classify(config, boots).subgroup == Subgroup::Boots,
        "armor subgroup classification regressed");

    ItemTraits throwingAxe{
        .itemCode = *PackCode("tax"),
        .itemTypeCodes = {
            *PackCode("taxe"), *PackCode("axe"), *PackCode("weap")},
        .quality = 2,
    };
    Require(
        Classify(config, throwingAxe).subgroup
            == Subgroup::ThrowingWeapons,
        "specific throwing-weapon subgroup lost precedence");

    ItemTraits ring{
        .itemCode = *PackCode("rin"),
        .itemTypeCodes = {*PackCode("ring"), *PackCode("misc")},
        .quality = 6,
    };
    Require(
        Classify(config, ring).category == Category::Jewelry
            && Classify(config, ring).subgroup == Subgroup::Rings,
        "ring subgroup classification regressed");

    ItemTraits broadPotionAlias{
        .itemCode = *PackCode("tes"),
        .itemTypeCodes = {*PackCode("poti"), *PackCode("misc")},
        .quality = 2,
    };
    Require(
        Classify(config, broadPotionAlias).category == Category::Misc,
        "spot-derived currency was misclassified as a potion");

    ItemTraits charm{
        .itemCode = *PackCode("cm1"),
        .itemTypeCodes = {*PackCode("char"), *PackCode("misc")},
        .quality = 4,
    };
    Require(
        Classify(config, charm).category == Category::Charms,
        "charm precedence regressed");

    ItemTraits rune{
        .itemCode = *PackCode("r07"),
        .itemTypeCodes = {*PackCode("rune"), *PackCode("misc")},
        .quality = 2,
    };
    Require(
        Classify(config, rune).category == Category::Runes,
        "runes are not a separate configurable category");
}

void TestCustomFirstMatch() {
    Config config{};
    config.customRules = {
        CustomRule{
            .name = "first",
            .anchor = Anchor::TopRight,
            .itemCodes = {*PackCode("abcd")},
        },
        CustomRule{
            .name = "second",
            .anchor = Anchor::BottomLeft,
            .itemCodes = {*PackCode("abcd")},
        },
    };
    const ItemTraits item{
        .itemCode = *PackCode("abcd"),
        .quality = 7,
    };
    const auto classification = Classify(config, item);
    Require(
        classification.customRuleIndex == 0,
        "custom rules are not first-match-wins");
    Require(
        classification.anchor == Anchor::TopRight,
        "first custom rule anchor was not retained");
}

void TestPlacementPrecedence() {
    Config config{};
    config.anchors[Index(Category::Armor)] = Anchor::TopLeft;
    config.subgroupAnchors[Index(Subgroup::Boots)] = Anchor::BottomLeft;
    config.customRules = {
        CustomRule{
            .name = "specific mod boots",
            .anchor = Anchor::MiddleRight,
            .itemCodes = {*PackCode("btm")},
        },
    };

    const ItemTraits ordinaryBoots{
        .itemCode = *PackCode("bt1"),
        .itemTypeCodes = {*PackCode("boot"), *PackCode("armo")},
        .quality = 2,
    };
    const ItemTraits modBoots{
        .itemCode = *PackCode("btm"),
        .itemTypeCodes = {*PackCode("boot"), *PackCode("armo")},
        .quality = 7,
    };
    const ItemTraits bodyArmor{
        .itemCode = *PackCode("arm"),
        .itemTypeCodes = {*PackCode("tors"), *PackCode("armo")},
        .quality = 2,
    };

    Require(
        Classify(config, ordinaryBoots).anchor == Anchor::BottomLeft,
        "subgroup anchor did not override the category anchor");
    Require(
        Classify(config, modBoots).anchor == Anchor::MiddleRight,
        "specific item rule did not override the subgroup anchor");
    Require(
        Classify(config, bodyArmor).anchor == Anchor::TopLeft,
        "category anchor inheritance regressed");
}

void TestExclusionsPrecedeCustomGroups() {
    Config config{};
    config.exclusions = {
        ExclusionRule{
            .name = "cube",
            .itemCodes = {*PackCode("box")},
        },
    };
    config.customRules = {
        CustomRule{
            .name = "custom cube group",
            .anchor = Anchor::BottomRight,
            .itemCodes = {*PackCode("box")},
        },
    };
    const ItemTraits cube{
        .itemCode = *PackCode("box"),
        .itemTypeCodes = {*PackCode("misc")},
        .quality = 2,
    };
    const auto classification = Classify(config, cube);
    Require(classification.excluded, "matching exclusion was not retained");
    Require(
        classification.anchor == Anchor::Ignore,
        "excluded item did not become a fixed obstacle");
    Require(
        classification.exclusionRuleIndex == 0
            && !classification.customRuleIndex,
        "custom group overrode a prior exclusion");
}

void TestTomlContract() {
    Config config{};
    std::string error;
    Require(
        ParseToml(ReadFile(AUTOSORT_CONFIG_FILE), config, error),
        "packaged TOML did not parse");
    Require(config.enabled, "packaged TOML unexpectedly disables AutoSort");
    Require(
        config.fixedRegions.inventoryRightColumns == 1,
        "packaged BKVince fixed inventory column was not parsed");
    Require(
        config.anchors[Index(Category::Misc)] == Anchor::Middle,
        "middle anchor was not parsed");
    Require(
        !config.button.enabled,
        "unsupported Inventory button must stay disabled in 0.1");
    Require(
        config.diagnostics && !config.diagnosticsDryRun
            && !config.diagnosticsLogItems,
        "live-sort packaged diagnostics were not parsed");
    Require(
        config.subgroupOrder[Index(Category::Armor)].front()
            == Subgroup::BodyArmor
            && config.subgroupOrder[Index(Category::Armor)][5]
                == Subgroup::Boots,
        "configured armor subgroup order was not retained");
    Require(
        std::all_of(
            config.subgroupAnchors.begin(),
            config.subgroupAnchors.end(),
            [](const std::optional<Anchor>& anchor) {
                return !anchor.has_value();
            }),
        "packaged defaults unexpectedly split a category across anchors");
    Require(
        config.exclusions.empty()
            && config.customRules.size() == 1
            && config.customRules.front().name == "Horadric Cube"
            && config.customRules.front().anchor == Anchor::MiddleLeft
            && config.customRules.front().itemCodes.size() == 1
            && config.customRules.front().itemCodes.front()
                == 0x00786F62u,
        "packaged Horadric Cube special group is missing or not canonical");

    const ItemTraits healing{
        .itemCode = 0x00357068u,
        .itemTypeCodes = {*PackCode("hpot")},
        .quality = 2,
    };
    const ItemTraits mana{
        .itemCode = 0x0035706Du,
        .itemTypeCodes = {*PackCode("mpot")},
        .quality = 2,
    };
    const ItemTraits rejuvenation{
        .itemCode = 0x00737672u,
        .itemTypeCodes = {
            *PackCode("rpot"), *PackCode("hpot"), *PackCode("mpot")},
        .quality = 2,
    };
    const ItemTraits cube{
        .itemCode = 0x00786F62u,
        .itemTypeCodes = {*PackCode("ques")},
        .quality = 2,
    };
    Require(
        Classify(config, healing).anchor == Anchor::TopRight
            && Classify(config, mana).anchor == Anchor::TopRight
            && Classify(config, rejuvenation).anchor == Anchor::TopRight,
        "packaged potion defaults fragment healing, mana, and rejuvenation");
    Require(
        Classify(config, rejuvenation).subgroup
                == Subgroup::RejuvenationPotions
            && SemanticItemSortKey(
                    Category::Potions, *PackCode("rvs"))
                < SemanticItemSortKey(
                    Category::Potions, *PackCode("rvl")),
        "canonical rejuvenation codes lost family or tier order");
    Require(
        !Classify(config, cube).excluded
            && Classify(config, cube).customRuleIndex == 0
            && Classify(config, cube).anchor == Anchor::MiddleLeft,
        "canonical Horadric Cube did not use its dedicated destination");

    Require(
        !ParseToml("unknown = true\n", config, error),
        "unknown top-level setting was accepted");
    Require(
        !ParseToml(
            "[fixed_regions.inventory]\nright_columns=16\n",
            config,
            error),
        "out-of-range fixed inventory columns were accepted");
    Require(
        !ParseToml(
            "[sorting]\ncategory_order=[\"armor\"]\n",
            config,
            error),
        "incomplete category order was accepted");
    Require(
        !ParseToml(
            "[[custom_rules]]\nname=\"x\"\nanchor=\"middle\"\n",
            config,
            error),
        "selector-free custom rule was accepted");
    Require(
        !ParseToml(
            "[diagnostics]\nenabled=false\ndry_run=true\n",
            config,
            error),
        "dry-run diagnostics were accepted while diagnostics were disabled");
    Require(
        !ParseToml(
            "[subgroup_order]\njewelry=[\"rings\"]\n",
            config,
            error),
        "incomplete jewelry subgroup order was accepted");
    Require(
        ParseToml(
            "[[exclusions]]\nname=\"cube\"\nitem_codes=[\"box\"]\n",
            config,
            error)
            && config.exclusions.size() == 1,
        "dedicated exclusions did not parse");
    Require(
        ParseToml(
            "[[custom_groups]]\nname=\"tokens\"\nanchor=\"middle\"\nitem_codes=[\"tok1\"]\n",
            config,
            error)
            && config.customRules.size() == 1,
        "custom_groups did not parse");
    Require(
        !ParseToml(
            "[subgroup_anchors.armor]\nrings=\"bottom_right\"\n",
            config,
            error),
        "subgroup anchor under the wrong category was accepted");
    Require(
        !ParseToml(
            "[subgroup_anchors.jewelry]\nrings=\"somewhere\"\n",
            config,
            error),
        "invalid subgroup destination was accepted");
}

void TestLargestFreeRectangle() {
    constexpr std::uint8_t width = 5;
    constexpr std::uint8_t height = 4;
    std::vector<std::uint32_t> occupancy(width * height);
    occupancy[0] = 1;
    occupancy[width] = 1;
    const auto rectangle = LargestFreeRectangle(width, height, occupancy);
    Require(rectangle.area == 16, "largest free rectangle area is wrong");
    Require(
        rectangle.x == 1 && rectangle.y == 0
            && rectangle.width == 4 && rectangle.height == 4,
        "largest free rectangle coordinates are wrong");
}

void TestAnchorPriorityThenFreeSpaceOptimization() {
    CandidateLayout compact{
        .freeRectangle = {
            .x = 4,
            .y = 0,
            .width = 6,
            .height = 4,
            .area = 24,
        },
        .anchorPenalty = 20,
    };
    CandidateLayout anchored{
        .freeRectangle = {
            .x = 5,
            .y = 0,
            .width = 5,
            .height = 4,
            .area = 20,
        },
        .anchorPenalty = 5,
    };
    Require(
        BetterCandidate(anchored, compact, true),
        "free-space optimization overrode configured anchor fidelity");
    Require(
        BetterCandidate(anchored, compact, false),
        "anchor priority regressed when free-space optimization is disabled");
    compact.anchorPenalty = anchored.anchorPenalty;
    Require(
        BetterCandidate(compact, anchored, true),
        "equal-anchor candidates did not maximize the largest free rectangle");
}

void TestItemCodeCohesion() {
    const auto hp1 = NaturalCodeSortKey(*PackCode("hp1"));
    const auto hp2 = NaturalCodeSortKey(*PackCode("hp2"));
    const auto hp3 = NaturalCodeSortKey(*PackCode("hp3"));
    const std::vector<GridItem> items{
        MakeItem(10, 1, 1, 0, 0, Anchor::TopRight, 0, hp2),
        MakeItem(20, 1, 1, 1, 0, Anchor::TopRight, 0, hp1),
        MakeItem(30, 1, 1, 2, 0, Anchor::TopRight, 0, hp3),
        MakeItem(40, 1, 1, 3, 0, Anchor::TopRight, 0, hp1),
    };
    const auto plan = BuildPlan(4, 2, items, true);
    Require(plan.success, "item-code cohesion plan failed");
    const auto at = [&](std::uint32_t guid) {
        const auto placement = std::find_if(
            plan.placements.begin(),
            plan.placements.end(),
            [&](const Placement& candidate) {
                return candidate.guid == guid;
            });
        Require(placement != plan.placements.end(), "cohesion plan lost an item");
        return placement->to;
    };
    Require(
        static_cast<int>(std::abs(
            static_cast<int>(at(20).x) - static_cast<int>(at(40).x)))
                + static_cast<int>(std::abs(
                    static_cast<int>(at(20).y)
                        - static_cast<int>(at(40).y)))
            == 1,
        "same potion codes were not kept together");
    auto scanOrder = plan.placements;
    std::sort(
        scanOrder.begin(),
        scanOrder.end(),
        [](const Placement& left, const Placement& right) {
            return std::tuple{left.to.y, -static_cast<int>(left.to.x)}
                < std::tuple{right.to.y, -static_cast<int>(right.to.x)};
        });
    Require(
        scanOrder.size() == 4
            && (scanOrder[0].guid == 20 || scanOrder[0].guid == 40)
            && (scanOrder[1].guid == 20 || scanOrder[1].guid == 40)
            && scanOrder[2].guid == 10
            && scanOrder[3].guid == 30,
        "potion tiers were not ordered from the configured anchor");
}

void TestCohesionFirstPackingFallback() {
    const std::vector<GridItem> items{
        MakeItem(10, 1, 1, 1, 1, Anchor::TopLeft, 0, 1),
        MakeItem(20, 3, 1, 0, 0, Anchor::TopLeft, 0, 2),
        MakeItem(30, 1, 2, 0, 1, Anchor::TopLeft, 0, 3),
    };
    const auto plan = BuildPlan(3, 3, items, true);
    Require(plan.success, "dimension fallback did not recover a valid layout");
    Require(
        plan.placements.size() == items.size(),
        "dimension fallback lost an item");
}

void TestStrictHierarchyRejectsInterleaving() {
    const std::vector<Placement> clean{
        Placement{
            .guid = 1,
            .width = 1,
            .height = 2,
            .to = {0, 0},
            .anchor = Anchor::TopLeft,
            .groupOrder = 0,
            .subgroupOrder = 0,
            .itemOrder = 0,
        },
        Placement{
            .guid = 2,
            .width = 1,
            .height = 2,
            .to = {1, 0},
            .anchor = Anchor::TopLeft,
            .groupOrder = 1,
            .subgroupOrder = 0,
            .itemOrder = 0,
        },
    };
    Require(
        HasStrictHierarchicalCohesion(3, 2, clean),
        "adjacent hierarchy blocks were rejected");

    const std::vector<Placement> interleaved{
        Placement{
            .guid = 1,
            .width = 1,
            .height = 2,
            .to = {0, 0},
            .anchor = Anchor::TopLeft,
            .groupOrder = 0,
            .subgroupOrder = 0,
            .itemOrder = 0,
        },
        Placement{
            .guid = 2,
            .width = 1,
            .height = 1,
            .to = {1, 1},
            .anchor = Anchor::TopLeft,
            .groupOrder = 0,
            .subgroupOrder = 0,
            .itemOrder = 0,
        },
        Placement{
            .guid = 3,
            .width = 1,
            .height = 1,
            .to = {1, 0},
            .anchor = Anchor::TopLeft,
            .groupOrder = 1,
            .subgroupOrder = 0,
            .itemOrder = 0,
        },
    };
    Require(
        !HasStrictHierarchicalCohesion(3, 2, interleaved),
        "overlapping sibling block envelopes were accepted");

    auto fragmented = clean;
    fragmented[1].groupOrder = 0;
    fragmented[1].to = {2, 0};
    Require(
        !HasStrictHierarchicalCohesion(3, 2, fragmented),
        "a fragmented hierarchy block was accepted");
}

void TestNearFullLayoutRefusesGlobalMixing() {
    constexpr std::uint8_t width = 11;
    constexpr std::uint8_t height = 8;
    std::array<bool, width * height> occupied{};
    const auto reserve = [&](std::uint8_t x, std::uint8_t y,
                             std::uint8_t itemWidth,
                             std::uint8_t itemHeight) {
        for (std::uint8_t row = 0; row < itemHeight; ++row) {
            for (std::uint8_t column = 0; column < itemWidth; ++column) {
                occupied[static_cast<std::size_t>(y + row) * width
                    + x + column] = true;
            }
        }
    };

    std::vector<GridItem> items;
    items.reserve(78);
    items.push_back(MakeItem(
        1000, 2, 2, 0, 0, Anchor::TopLeft, 2, 2));
    reserve(0, 0, 2, 2);
    items.push_back(MakeItem(
        1001, 1, 3, 2, 0, Anchor::TopLeft, 1, 1));
    reserve(2, 0, 1, 3);

    std::uint32_t guid = 1;
    for (std::uint8_t y = 0; y < height && guid <= 76; ++y) {
        for (std::uint8_t x = 0; x < width && guid <= 76; ++x) {
            if (occupied[static_cast<std::size_t>(y) * width + x]) {
                continue;
            }
            items.push_back(MakeItem(
                guid, 1, 1, x, y, Anchor::TopLeft, 0, guid));
            occupied[static_cast<std::size_t>(y) * width + x] = true;
            ++guid;
        }
    }
    Require(items.size() == 78, "near-full fixture item count is wrong");

    const auto plan = BuildPlan(width, height, items, true);
    Require(
        !plan.success,
        "near-full grid sacrificed hierarchy to recover the layout");
    Require(
        plan.placements.empty(),
        "hierarchy refusal leaked partial placements");
    Require(
        plan.error.find("strict hierarchy") != std::string::npos,
        "hierarchy refusal did not report the strict contract");
}

void TestObservedElevenByEightHierarchyAndPerformance() {
    std::vector<GridItem> items;
    items.reserve(34);
    std::uint32_t guid = 1;
    const auto add = [&](std::uint8_t width,
                         std::uint8_t height,
                         std::uint8_t x,
                         std::uint8_t y,
                         Anchor anchor,
                         std::size_t groupOrder,
                         std::size_t subgroupOrder,
                         std::string_view code,
                         Category category) {
        items.push_back(MakeItem(
            guid++,
            width,
            height,
            x,
            y,
            anchor,
            groupOrder,
            SemanticItemSortKey(category, *PackCode(code)),
            subgroupOrder));
    };

    add(2, 3, 0, 0, Anchor::TopLeft, 0, 2, "uit", Category::Armor);
    add(1, 3, 2, 0, Anchor::TopLeft, 1, 0, "sbr", Category::Weapons);
    add(1, 3, 3, 0, Anchor::TopLeft, 1, 0, "ssd", Category::Weapons);
    add(2, 2, 4, 0, Anchor::MiddleLeft, 0, 0, "box", Category::Unknown);
    add(1, 2, 6, 0, Anchor::TopLeft, 1, 1, "tax", Category::Weapons);
    add(1, 2, 7, 0, Anchor::TopLeft, 1, 2, "dir", Category::Weapons);
    add(1, 2, 8, 0, Anchor::TopLeft, 1, 2, "9dg", Category::Weapons);

    const auto firstFrozenGuid = guid;
    for (std::uint8_t y = 0; y < 8; ++y) {
        add(1, 1, 10, y, Anchor::Ignore, 6, 0, "hp5", Category::Potions);
    }
    add(1, 1, 9, 6, Anchor::TopRight, 6, 0, "hp5", Category::Potions);
    add(1, 1, 8, 7, Anchor::TopRight, 6, 0, "hp5", Category::Potions);
    add(1, 1, 7, 4, Anchor::TopRight, 6, 0, "hp5", Category::Potions);
    add(1, 1, 7, 3, Anchor::TopRight, 6, 0, "hp5", Category::Potions);
    add(1, 1, 9, 0, Anchor::TopMiddle, 3, 0, "r03", Category::Runes);
    add(1, 1, 9, 1, Anchor::TopMiddle, 3, 0, "r07", Category::Runes);
    add(1, 1, 9, 7, Anchor::TopRight, 6, 1, "mp5", Category::Potions);
    add(1, 1, 9, 5, Anchor::TopRight, 6, 3, "wms", Category::Potions);
    add(1, 1, 9, 4, Anchor::TopRight, 6, 2, "rvl", Category::Potions);
    add(1, 1, 9, 3, Anchor::TopRight, 6, 2, "rvs", Category::Potions);
    add(1, 1, 9, 2, Anchor::TopRight, 6, 2, "rvs", Category::Potions);
    add(1, 1, 8, 4, Anchor::TopRight, 6, 2, "rvl", Category::Potions);
    add(1, 1, 8, 3, Anchor::TopRight, 6, 2, "rvl", Category::Potions);
    add(1, 1, 8, 2, Anchor::TopRight, 6, 2, "rvl", Category::Potions);
    add(1, 1, 7, 7, Anchor::TopRight, 6, 2, "rvs", Category::Potions);
    add(1, 1, 7, 6, Anchor::TopRight, 6, 2, "rvs", Category::Potions);
    add(1, 1, 8, 6, Anchor::BottomLeft, 2, 0, "rin", Category::Jewelry);
    add(1, 1, 8, 5, Anchor::Middle, 12, 0, "rds", Category::Misc);
    add(1, 1, 7, 5, Anchor::Middle, 12, 0, "key", Category::Misc);
    Require(items.size() == 34, "observed 11x8 fixture item count is wrong");

    const auto first = BuildPlan(11, 8, items, true, 1);
    Require(first.success, "observed 11x8 hierarchy did not fit");
    Require(
        first.placements.size() == items.size()
            && HasStrictHierarchicalCohesion(11, 8, first.placements),
        "observed 11x8 plan lost strict hierarchy");
    for (const auto& placement : first.placements) {
        if (placement.guid >= firstFrozenGuid
                && placement.guid < firstFrozenGuid + 8) {
            Require(
                placement.from == placement.to && placement.to.x == 10,
                "an item in the frozen right column moved");
        } else {
            Require(
                static_cast<unsigned>(placement.to.x) + placement.width <= 10,
                "a movable item entered the frozen right column");
        }
    }
    const auto cube = std::find_if(
        first.placements.begin(),
        first.placements.end(),
        [](const Placement& placement) { return placement.guid == 4; });
    Require(
        cube != first.placements.end()
            && cube->to.x == 0
            && cube->to.y == 3,
        "middle_left custom group lost its configured destination");

    constexpr int Iterations = 50;
    const auto started = std::chrono::steady_clock::now();
    for (int iteration = 0; iteration < Iterations; ++iteration) {
        const auto plan = BuildPlan(11, 8, items, true, 1);
        Require(plan.success, "performance iteration failed to build a plan");
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started);
    Require(
        elapsed < std::chrono::milliseconds(250),
        "hierarchical planner exceeded the 5 ms average performance budget");
}

void TestDeterminismAndIdempotence() {
    std::vector<GridItem> items{
        MakeItem(10, 2, 3, 0, 0, Anchor::TopLeft, 0),
        MakeItem(20, 1, 3, 2, 0, Anchor::TopLeft, 1),
        MakeItem(30, 1, 1, 9, 3, Anchor::BottomLeft, 2),
        MakeItem(40, 1, 2, 8, 2, Anchor::BottomRight, 3),
    };
    const auto first = BuildPlan(10, 4, items, true);
    Require(first.success, "deterministic plan failed");

    auto reversed = items;
    std::reverse(reversed.begin(), reversed.end());
    const auto second = BuildPlan(10, 4, reversed, true);
    Require(second.success, "reversed deterministic plan failed");
    Require(
        first.placements.size() == second.placements.size(),
        "deterministic plans have different sizes");
    for (std::size_t index = 0; index < first.placements.size(); ++index) {
        Require(
            first.placements[index].guid == second.placements[index].guid
                && first.placements[index].to
                    == second.placements[index].to,
            "input enumeration order changed the plan");
    }

    for (auto& item : items) {
        const auto placement = std::find_if(
            first.placements.begin(),
            first.placements.end(),
            [&](const Placement& candidate) {
                return candidate.guid == item.guid;
            });
        Require(placement != first.placements.end(), "plan lost an item");
        item.current = placement->to;
    }
    const auto repeated = BuildPlan(10, 4, items, true);
    Require(repeated.success, "idempotence rerun failed");
    Require(!repeated.changed, "idempotence rerun moved an item");
}

void TestIgnoredObstacleAndAtomicRefusal() {
    const std::vector<GridItem> items{
        MakeItem(1, 2, 2, 0, 0, Anchor::Ignore, 0),
        MakeItem(2, 2, 1, 2, 0, Anchor::TopLeft, 1),
    };
    const auto plan = BuildPlan(6, 4, items, true);
    Require(plan.success, "plan with ignored obstacle failed");
    const auto ignored = std::find_if(
        plan.placements.begin(),
        plan.placements.end(),
        [](const Placement& placement) { return placement.guid == 1; });
    Require(ignored != plan.placements.end(), "ignored item disappeared");
    Require(ignored->from == ignored->to, "ignored item was moved");

    auto invalid = items;
    invalid.push_back(MakeItem(3, 2, 2, 1, 1, Anchor::Middle, 2));
    const auto refused = BuildPlan(6, 4, invalid, true);
    Require(!refused.success, "overlapping source layout was accepted");
    Require(refused.placements.empty(), "refused plan leaked partial moves");
}

void TestConfigPrecedenceAndSourcePolicy() {
    const auto candidates = BuildConfigCandidates(
        "C:/mod/config",
        "C:/mod/config",
        "C:/global/config",
        "ruffneckk-autosort.toml");
    Require(candidates.size() == 2, "config candidate deduplication failed");
    Require(
        candidates.front().generic_string().find("mod/config")
            != std::string::npos,
        "mod-local config is not first");

    const auto source = ReadFile(AUTOSORT_SOURCE_FILE);
    const auto cmake = ReadFile(AUTOSORT_CMAKE_FILE);
    Require(
        source.find(".author = \"RuffnecKk\"") != std::string::npos,
        "author metadata regressed");
    Require(
        source.find(
            "D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks")
            != std::string::npos,
        "hybrid native flags regressed");
    Require(
        source.find("ModScopedOnly") == std::string::npos,
        "AutoSort became mod-scoped-only");
    Require(
        source.find("PlannerRva = 0x15E790") != std::string::npos
            && source.find("AutoSortWrapperRva = 0x160B00")
                != std::string::npos,
        "governed native AutoSort seam regressed");
    Require(
        source.find("BuildPlan(") != std::string::npos,
        "native hook no longer calls the pure planner");
    Require(
        source.find("D2RL::Input::Key::H") != std::string::npos
            && source.find("D2RL::Input::Modifier::Shift")
                != std::string::npos,
        "default Shift+H Controls binding regressed");
    Require(
        source.find("CheckExpectedBytes") != std::string::npos,
        "native fingerprint gate disappeared");
    Require(
        cmake.find("6eb8f8b6192868214706bd6d528c5294f2f551b7")
            != std::string::npos
            && cmake.find("4933e2c42cb2592958cd0df3b6dc5003102252d1")
                == std::string::npos,
        "PluginSDK v4 baseline pin regressed");
    Require(
        source.find(".version = \"0.1.1\"") != std::string::npos,
        "AutoSort component version regressed");
    Require(
        source.find("executeExistingItemTransaction") == std::string::npos
            && source.find("executeLocalPlayerMove") == std::string::npos,
        "baseline migration introduced a second transaction path");
}

} // namespace

int main() {
    try {
        TestPackCodeAndClassification();
        TestCustomFirstMatch();
        TestPlacementPrecedence();
        TestExclusionsPrecedeCustomGroups();
        TestTomlContract();
        TestLargestFreeRectangle();
        TestAnchorPriorityThenFreeSpaceOptimization();
        TestItemCodeCohesion();
        TestCohesionFirstPackingFallback();
        TestStrictHierarchyRejectsInterleaving();
        TestNearFullLayoutRefusesGlobalMixing();
        TestObservedElevenByEightHierarchyAndPerformance();
        TestDeterminismAndIdempotence();
        TestIgnoredObstacleAndAtomicRefusal();
        TestConfigPrecedenceAndSourcePolicy();
        std::cout << "AutoSort policy tests passed.\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "AutoSort policy test failed: "
                  << exception.what() << '\n';
        return 1;
    }
}
