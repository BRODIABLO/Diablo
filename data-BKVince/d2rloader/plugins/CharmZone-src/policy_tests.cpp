#include "charm_zone_config.hpp"

#include <cassert>
#include <cmath>
#include <stdexcept>

using namespace ruffneckk::charm_zone;

int main() {
    constexpr Zone zone{};
    static_assert(IsZoneValid(zone));
    static_assert(IsFullyContained(zone, {0, 0, 4, 1, 1}));
    static_assert(IsFullyContained(zone, {0, 10, 7, 1, 1}));
    static_assert(IsFullyContained(zone, {0, 9, 6, 2, 2}));
    static_assert(!IsFullyContained(zone, {0, 0, 3, 1, 2}));
    static_assert(!IsFullyContained(zone, {0, 10, 6, 2, 1}));
    static_assert(!IsFullyContained(zone, {3, 0, 4, 1, 1}));
    static_assert(!IsFullyContained(zone, {0, 0, 4, 0, 1}));

    ScreenRect rect{};
    assert(TryMakeScreenRect(100, 200, 1.0f, 2, 1, 98.0f, 1080.0f, rect));
    assert(std::fabs(rect.left - 100.0f) < 0.01f);
    assert(std::fabs(rect.right - 296.0f) < 0.01f);
    assert(std::fabs(rect.top - 200.0f) < 0.01f);
    assert(std::fabs(rect.bottom - 298.0f) < 0.01f);
    assert(ContainsPoint(rect, 150.0f, 250.0f));
    assert(!ContainsPoint(rect, 99.0f, 250.0f));

    const auto parsed = ParseConfig(R"toml(
[general]
enabled = true
[zone]
grid_width = 11
grid_height = 8
left = 0
top = 4
width = 11
height = 4
[visual]
enabled = true
overlay_alpha = 0.5
cell_size = 98.0
tooltip = "Inactive outside Charm Zone"
)toml");
    assert(parsed.enabled);
    assert(parsed.zone.top == 4);
    assert(parsed.zone.height == 4);
    assert(std::fabs(parsed.visual.overlayAlpha - 0.5f) < 0.001f);

    bool rejected{};
    try {
        (void)ParseConfig("[zone]\nheight = 5\n");
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    assert(rejected);

    rejected = false;
    try {
        (void)ParseConfig("[visual]\noverlay_alpha = 2.0\n");
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    assert(rejected);

    rejected = false;
    try {
        (void)ParseConfig("[general]\nunknown = true\n");
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    assert(rejected);
}
