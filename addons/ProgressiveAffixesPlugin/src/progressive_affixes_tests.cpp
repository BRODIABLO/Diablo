#include "progressive_affixes_config.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace RuffnecKk::ProgressiveAffixes;

namespace {

void Check(bool condition) {
    if (!condition) throw std::runtime_error("progressive affix policy check failed");
}

Config Parse(std::string_view text) {
    std::istringstream input{std::string(text)};
    return ParseConfig(input);
}

bool Throws(std::string_view text) {
    try {
        static_cast<void>(Parse(text));
        return false;
    } catch (const std::invalid_argument&) {
        return true;
    }
}

void CheckDistribution(
        const WeightedCategory& category,
        std::int32_t itemLevel,
        const std::vector<std::uint32_t>& expected) {
    const auto* step = FindStep(category.steps, itemLevel);
    Check(step != nullptr);
    std::vector<std::uint32_t> actual(category.counts.size());
    for (std::uint32_t roll = 0; roll < TotalWeight(*step); ++roll) {
        const auto count = PickWeightedCount(category, *step, roll);
        const auto position = std::find(category.counts.begin(), category.counts.end(), count);
        Check(position != category.counts.end());
        ++actual[static_cast<std::size_t>(position - category.counts.begin())];
    }
    Check(actual == expected);
}

} // namespace

int main(int argc, char** argv) {
    Check(argc == 2);
    std::ifstream shipped(argv[1]);
    Check(shipped.is_open());
    const auto config = ParseConfig(shipped);
    Check(config.enabled);
    Check(!config.diagnostics);
    Check(config.magic.size() == 4);
    Check(config.rare.size() == 2);
    Check(config.crafted.size() == 1);

    Check(config.magic[0].name == "weapons_and_armor");
    Check(config.magic[0].steps[0].minimumItemLevel == 65);
    Check(config.magic[1].steps[0].minimumItemLevel == 85);
    Check(config.magic[2].steps[0].minimumItemLevel == 90);
    Check(config.magic[3].itemTypes[0].wildcard);
    Check(FindStep(config.magic[0].steps, 64) == nullptr);
    Check(FindStep(config.magic[0].steps, 65)->minimumAffixes == 2);

    CheckDistribution(config.rare[0], 1, {0, 1});
    CheckDistribution(config.rare[1], 1, {1, 3, 3, 1});
    CheckDistribution(config.rare[1], 44, {1, 3, 3, 1});
    CheckDistribution(config.rare[1], 45, {0, 1, 2, 1});
    CheckDistribution(config.rare[1], 65, {0, 0, 1, 1});
    CheckDistribution(config.rare[1], 85, {0, 0, 0, 1});
    CheckDistribution(config.crafted[0], 1, {2, 1, 1, 1});
    CheckDistribution(config.crafted[0], 31, {0, 3, 1, 1});
    CheckDistribution(config.crafted[0], 51, {0, 0, 4, 1});
    CheckDistribution(config.crafted[0], 71, {0, 0, 0, 1});

    const auto disabled = Parse("[plugin]\nenabled = false\n");
    Check(!disabled.enabled);
    Check(disabled.magic.empty());

    Check(Throws("[plugin]\nenabled = true\nunknown = false\n"));
    Check(Throws("[plugin]\nenabled = 1\n"));
    Check(Throws("[plugin]\nenabled = true\n"));
    Check(Throws(R"toml(
[plugin]
enabled = true
[[rare.categories]]
name = "fallback"
item_types = ["*"]
counts = [3, 4]
[[rare.categories.steps]]
minimum_item_level = 1
weights = [1]
)toml"));
    Check(Throws(R"toml(
[plugin]
enabled = true
[[crafted.categories]]
name = "fallback"
item_types = ["*"]
counts = [1, 2, 3, 4]
[[crafted.categories.steps]]
minimum_item_level = 1
weights = [0, 0, 0, 0]
)toml"));
    Check(Throws(R"toml(
[plugin]
enabled = true
[[magic.categories]]
name = "fallback"
item_types = ["*"]
[[magic.categories.steps]]
minimum_item_level = 1
minimum_affixes = 3
)toml"));
    Check(Throws(R"toml(
[plugin]
enabled = true
[[magic.categories]]
name = "fallback"
item_types = ["*"]
[[magic.categories.steps]]
minimum_item_level = 1
minimum_affixes = 1
[[magic.categories]]
name = "late"
item_types = ["weap"]
[[magic.categories.steps]]
minimum_item_level = 65
minimum_affixes = 2
)toml"));
    Check(Throws(R"toml(
[plugin]
enabled = true
[[magic.categories]]
name = "invalid_code"
item_types = ["too-long"]
[[magic.categories.steps]]
minimum_item_level = 1
minimum_affixes = 2
[[magic.categories]]
name = "fallback"
item_types = ["*"]
[[magic.categories.steps]]
minimum_item_level = 1
minimum_affixes = 1
)toml"));

    const auto candidates = BuildConfigCandidates(
        L"C:/D2R/mods/BKVince/d2rloader/config",
        L"C:/D2R/mods/BKVince/d2rloader/config",
        L"C:/D2R/d2rloader/config",
        L"ProgressiveAffixesPlugin.toml");
    Check(candidates.size() == 2);
    Check(candidates[0].filename() == L"ProgressiveAffixesPlugin.toml");
    Check(candidates[1].filename() == L"ProgressiveAffixesPlugin.toml");
    return 0;
}
