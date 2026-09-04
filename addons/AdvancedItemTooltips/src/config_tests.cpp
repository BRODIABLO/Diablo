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
    CHECK(defaults.propertyRangeColor == PropertyRangeColor::ChronicleColor);
    CHECK(defaults.rangeDisplayMode == RangeDisplayMode::Always);
    CHECK(defaults.holdToDisplayHotkey.virtualKey == 0x10);
    CHECK(defaults.holdToDisplayHotkey.name == "Shift");
    CHECK(ShouldDisplayRanges(defaults.rangeDisplayMode, false));
    CHECK(ShouldDisplayRanges(defaults.rangeDisplayMode, true));
    CHECK(PropertyRangeColorCode(defaults.propertyRangeColor) == 'U');
    CHECK(PropertyRangeColorName(defaults.propertyRangeColor) == "ChronicleColor");

    const auto combined = ParseConfig({
        {"enabled", true},
        {"showMaxSockets", false},
        {"showMaxSocketsOnSocketedItems", true},
        {"showBaseDefenseRange", false},
        {"showPropertyRanges", true},
        {"includeSocketedContributionsInRanges", true},
        {"_propertyRangeColorHelp", "ChronicleColor uses Chronicle teal; BHDarkGreen uses BH dark green."},
        {"propertyRangeColor", "BHDarkGreen"},
        {"_rangeDisplayModeHelp", "Always shows ranges; HoldHotkey shows them while the configured key is held."},
        {"rangeDisplayMode", "HoldHotkey"},
        {"_holdToDisplayHotkeyHelp", "Key held to show ranges in HoldHotkey mode."},
        {"holdToDisplayHotkey", "Ctrl"},
    });
    CHECK(!combined.showMaxSockets);
    CHECK(combined.showMaxSocketsOnSocketedItems);
    CHECK(!combined.showBaseDefenseRange);
    CHECK(combined.includeSocketedContributionsInRanges);
    CHECK(combined.propertyRangeColor == PropertyRangeColor::BHDarkGreen);
    CHECK(combined.rangeDisplayMode == RangeDisplayMode::HoldHotkey);
    CHECK(combined.holdToDisplayHotkey.virtualKey == 0x11);
    CHECK(combined.holdToDisplayHotkey.name == "Ctrl");
    CHECK(!ShouldDisplayRanges(combined.rangeDisplayMode, false));
    CHECK(ShouldDisplayRanges(combined.rangeDisplayMode, true));
    CHECK(PropertyRangeColorCode(combined.propertyRangeColor) == ':');
    CHECK(PropertyRangeColorName(combined.propertyRangeColor) == "BHDarkGreen");

    const auto legacy = ParseConfig({{"rangeDisplayMode", "HoldShift"}});
    CHECK(legacy.rangeDisplayMode == RangeDisplayMode::HoldHotkey);
    CHECK(legacy.holdToDisplayHotkey.virtualKey == 0x10);
    CHECK(legacy.holdToDisplayHotkey.name == "Shift");

    const auto lowerCase = ParseConfig({{"holdToDisplayHotkey", "f12"}});
    CHECK(lowerCase.holdToDisplayHotkey.virtualKey == 0x7B);
    CHECK(lowerCase.holdToDisplayHotkey.name == "F12");

    const auto defaultSpelling = ParseConfig({{"holdToDisplayHotkey", "shift"}});
    CHECK(defaultSpelling.holdToDisplayHotkey.virtualKey == 0x10);
    CHECK(defaultSpelling.holdToDisplayHotkey.name == "Shift");

    const auto letter = ParseConfig({{"holdToDisplayHotkey", "r"}});
    CHECK(letter.holdToDisplayHotkey.virtualKey == 'R');
    CHECK(letter.holdToDisplayHotkey.name == "R");

    CHECK(Throws([] { ParseConfig(nlohmann::json::array()); }));
    CHECK(Throws([] { ParseConfig({{"enabled", 1}}); }));
    CHECK(Throws([] { ParseConfig({{"_propertyRangeColorHelp", true}}); }));
    CHECK(Throws([] { ParseConfig({{"_rangeDisplayModeHelp", true}}); }));
    CHECK(Throws([] { ParseConfig({{"_holdToDisplayHotkeyHelp", true}}); }));
    CHECK(Throws([] { ParseConfig({{"propertyRangeColor", true}}); }));
    CHECK(Throws([] { ParseConfig({{"rangeDisplayMode", true}}); }));
    CHECK(Throws([] { ParseConfig({{"holdToDisplayHotkey", true}}); }));
    CHECK(Throws([] { ParseConfig({{"propertyRangeColor", "Blue"}}); }));
    CHECK(Throws([] { ParseConfig({{"rangeDisplayMode", "Toggle"}}); }));
    CHECK(Throws([] { ParseConfig({{"holdToDisplayHotkey", "Space"}}); }));
    CHECK(Throws([] { ParseConfig({{"holdToDisplayHotkey", "F13"}}); }));
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
