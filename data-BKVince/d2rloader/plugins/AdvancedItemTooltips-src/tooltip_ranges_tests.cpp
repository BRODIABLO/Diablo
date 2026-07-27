#include "tooltip_ranges.hpp"

#include <filesystem>
#include <string>
#include <vector>

using tcp::tooltips::AppendConsensusRanges;
using tcp::tooltips::ModifierRange;

int main(int argc, char** argv) {
#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)
    constexpr auto blue = "\xEE\x81\xBE" "3";
    constexpr auto green = "\xEE\x81\xBE" "2";
    const std::string tooltip = blue + std::string("+18% Faster Cast Rate");
    const std::vector<std::vector<ModifierRange>> stacked{{
        {"item_fastercastrate", "+#% Faster Cast Rate", 15, 20, 142}
    }};
    CHECK(tcp::tooltips::FirstSignedInteger(tooltip) == 18);
    const auto enhanced = AppendConsensusRanges(tooltip, stacked);
    CHECK(enhanced.find(green + std::string("[15 - 20]") + blue) != std::string::npos);

    const std::vector<std::vector<ModifierRange>> disagreement{
        {{"item_fastercastrate", "+#% Faster Cast Rate", 5, 10, 142}},
        {{"item_fastercastrate", "+#% Faster Cast Rate", 5, 15, 142}}
    };
    CHECK(AppendConsensusRanges(tooltip, disagreement) == tooltip);

    const std::vector<std::vector<ModifierRange>> doubled{{
        {"enhanced_damage", "+#% Enhanced Damage", 150, 300, 130}
    }};
    CHECK(AppendConsensusRanges(blue + std::string("+250% Enhanced Damage"), doubled)
        .find("[150 - 300]") != std::string::npos);

    if (argc == 2) {
        tcp::tooltips::RangeCatalog catalog;
        std::string error;
        CHECK(catalog.Load(std::filesystem::path(argv[1]), error));
        CHECK(catalog.PropertyCount() > 0);
        CHECK(catalog.FindArmor("ci3")->minimum == 50);
        CHECK(catalog.FindArmor("ci3")->maximum == 60);
        tcp::tooltips::ItemAffixIds casterAmulet{};
        casterAmulet.quality = 8;
        casterAmulet.magicSuffix[0] = 22; // of the Apprentice: fixed +10 FCR
        const auto candidates = catalog.ResolveCandidates(casterAmulet, "amu");
        bool foundStackedFcr{};
        for (const auto& candidate : candidates) {
            for (const auto& range : candidate) {
                if (range.key == "item_fastercastrate"
                    && range.minimum == 15 && range.maximum == 20) {
                    foundStackedFcr = true;
                }
            }
        }
        CHECK(foundStackedFcr);
    }
#undef CHECK
}
