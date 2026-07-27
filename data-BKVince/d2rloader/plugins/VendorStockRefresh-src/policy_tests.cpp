#include "vendor_refresh_policy.hpp"

#include <cassert>

int main() {
    using namespace ruffneckk::vendor_stock_refresh;

    assert(RefreshActionForPanel(false) == NormalRefreshAction);
    assert(RefreshActionForPanel(true) == VanillaGambleRefreshAction);
    assert(ShouldShowNormalRefresh(true, false));
    assert(!ShouldShowNormalRefresh(true, true));
    assert(!ShouldShowNormalRefresh(false, false));

    constexpr WidgetRect vanillaGold{421, 1305, 313, 58};
    constexpr WidgetRect vanillaRefresh{877, 1277, 112, 112};
    constexpr auto vanillaPlacement = CenterBelow(vanillaGold, vanillaRefresh);
    static_assert(vanillaPlacement.valid);
    static_assert(vanillaPlacement.x == 521);
    static_assert(vanillaPlacement.y == 1382);

    constexpr WidgetRect moddedGold{600, 1500, 500, 80};
    constexpr WidgetRect moddedRefresh{1100, 1400, 160, 160};
    constexpr auto moddedPlacement = CenterBelow(moddedGold, moddedRefresh);
    static_assert(moddedPlacement.valid);
    static_assert(moddedPlacement.x == 770);
    static_assert(moddedPlacement.y == 1607);

    constexpr auto fallbackGold = UnionRect(
        WidgetRect{427, 1304, 57, 57},
        WidgetRect{487, 1309, 249, 48}
    );
    static_assert(fallbackGold.x == 427);
    static_assert(fallbackGold.y == 1304);
    static_assert(fallbackGold.width == 309);
    static_assert(fallbackGold.height == 57);
    static_assert(!CenterBelow(WidgetRect{}, vanillaRefresh).valid);
    static_assert(!CenterBelow(vanillaGold, WidgetRect{}).valid);

    assert(ShouldArmNormalRefresh(true, true, NormalVendorMode, true, true));
    assert(!ShouldArmNormalRefresh(false, true, NormalVendorMode, true, true));
    assert(!ShouldArmNormalRefresh(true, false, NormalVendorMode, true, true));
    assert(!ShouldArmNormalRefresh(true, true, GambleVendorMode, true, true));
    assert(!ShouldArmNormalRefresh(true, true, NormalVendorMode, false, true));
    assert(!ShouldArmNormalRefresh(true, true, NormalVendorMode, true, false));
}
