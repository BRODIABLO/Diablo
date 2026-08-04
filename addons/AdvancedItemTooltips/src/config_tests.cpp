#include "advanced_item_tooltips_config.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
int Failures{};

void Check(bool condition, const char* expression, int line) {
    if (condition) return;
    std::cerr << "FAIL line " << line << ": " << expression << '\n';
    ++Failures;
}

#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

template <class Callable>
bool Throws(Callable&& callable) {
    try {
        callable();
        return false;
    } catch (const std::exception&) {
        return true;
    }
}
} // namespace

int main() {
    using namespace ruffneckk::advanced_item_tooltips;

    const auto defaults = ParseConfig(nlohmann::json::object());
    CHECK(defaults.enabled);
    CHECK(defaults.showMaxSockets);
    CHECK(!defaults.showMaxSocketsOnSocketedItems);
    CHECK(defaults.showBaseDefenseRange);
    CHECK(defaults.showPropertyRanges);
    CHECK(!defaults.includeSocketedContributionsInRanges);

    const auto combined = ParseConfig({
        {"enabled", true},
        {"showMaxSockets", false},
        {"showMaxSocketsOnSocketedItems", true},
        {"showBaseDefenseRange", false},
        {"showPropertyRanges", true},
        {"includeSocketedContributionsInRanges", true},
    });
    CHECK(!combined.showMaxSockets);
    CHECK(combined.showMaxSocketsOnSocketedItems);
    CHECK(!combined.showBaseDefenseRange);
    CHECK(combined.includeSocketedContributionsInRanges);

    CHECK(Throws([] { ParseConfig(nlohmann::json::array()); }));
    CHECK(Throws([] { ParseConfig({{"enabled", 1}}); }));
    CHECK(Throws([] { ParseConfig({{"unknown", true}}); }));

    const auto paths = ConfigCandidates(
        "C:/D2R/mods/Test/Test.mpq",
        "C:/D2R/d2rloader/config/advanced-item-tooltips.toml",
        "C:/D2R",
        "AdvancedItemTooltips.json");
    CHECK(paths.size() == 2);
    CHECK(paths[0] == std::filesystem::path(
        "C:/D2R/mods/Test/Test.mpq/d2rloader/config/AdvancedItemTooltips.json"));
    CHECK(paths[1] == std::filesystem::path(
        "C:/D2R/d2rloader/config/AdvancedItemTooltips.json"));

    return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
