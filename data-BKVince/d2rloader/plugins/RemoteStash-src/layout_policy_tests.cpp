#include "remote_stash_layout_policy.hpp"

#include <cassert>

int main() {
    using namespace ruffneckk::remote_stash;

    static_assert(
        UiMessageHash("VendorPanelMessage:Repair") == 0x20BADA21BC8CB334ull
    );
    static_assert(
        UiMessageHash("VendorPanelMessage:RepairAll") == 0x5680CE604E1D402Full
    );
    static_assert(
        UiMessageHash("VendorPanelMessage:RefreshAll") == 0xB7AA1748D66EFCAFull
    );
    static_assert(
        UiMessageHash("VendorPanelMessage:Close") == 0x5E8250FB85D64C23ull
    );
    static_assert(
        UiMessageHash("PlayerInventoryPanelMessage:DropGold")
        == 0xB3B0A478381C4725ull
    );
    static_assert(
        RemoteStashMessageHash == 0x055A7CEA95897DC9ull
    );

    constexpr WidgetRect bkvincePanel{0, 0, 1280, 1760};
    constexpr WidgetRect bkvinceGrid{95, 857, 980, 760};
    constexpr WidgetRect bkvinceGoldButton{780, 1656, 58, 58};
    constexpr WidgetRect bkvinceGoldAmount{840, 1656, 249, 48};
    constexpr WidgetRect remoteButton{0, 0, 112, 96};
    constexpr auto bkvince = PlaceDesktopFooterLeft(
        bkvincePanel,
        bkvinceGrid,
        bkvinceGoldButton,
        bkvinceGoldAmount,
        remoteButton
    );
    static_assert(bkvince.valid);
    static_assert(bkvince.rect.x == 95);
    static_assert(bkvince.rect.y == 1637);
    static_assert(bkvince.failure == PlacementFailure::None);

    constexpr auto resized = PlaceDesktopFooterLeft(
        WidgetRect{0, 0, 1600, 2000},
        WidgetRect{140, 700, 1320, 1050},
        WidgetRect{1010, 1840, 64, 64},
        WidgetRect{1080, 1840, 330, 56},
        WidgetRect{0, 0, 128, 104}
    );
    static_assert(resized.valid);
    static_assert(resized.rect.x == 140);
    static_assert(resized.rect.y == 1820);

    constexpr auto goldButtonFallback = PlaceDesktopFooterLeft(
        WidgetRect{0, 0, 1280, 1760},
        WidgetRect{95, 857, 980, 760},
        WidgetRect{780, 1656, 309, 58},
        WidgetRect{},
        remoteButton
    );
    static_assert(goldButtonFallback.valid);

    constexpr auto gridConsumesFooter = PlaceDesktopFooterLeft(
        bkvincePanel,
        WidgetRect{95, 857, 980, 820},
        bkvinceGoldButton,
        bkvinceGoldAmount,
        remoteButton
    );
    static_assert(!gridConsumesFooter.valid);
    static_assert(gridConsumesFooter.failure == PlacementFailure::GridCollision);

    constexpr auto controllerCollision = PlaceDesktopFooterLeft(
        WidgetRect{0, 0, 1280, 1760},
        WidgetRect{210, 646, 980, 760},
        WidgetRect{201, 1424, 317, 44},
        WidgetRect{255, 1424, 257, 48},
        remoteButton
    );
    static_assert(!controllerCollision.valid);
    static_assert(
        controllerCollision.failure == PlacementFailure::GridCollision
        || controllerCollision.failure == PlacementFailure::FooterCollision
    );

    constexpr auto footerCollision = PlaceDesktopFooterLeft(
        WidgetRect{0, 0, 900, 700},
        WidgetRect{100, 100, 600, 400},
        WidgetRect{90, 600, 300, 48},
        WidgetRect{},
        WidgetRect{0, 0, 112, 80}
    );
    static_assert(!footerCollision.valid);
    static_assert(footerCollision.failure == PlacementFailure::FooterCollision);

    static_assert(
        PlaceDesktopFooterLeft(
            WidgetRect{}, bkvinceGrid, bkvinceGoldButton,
            bkvinceGoldAmount, remoteButton
        ).failure == PlacementFailure::InvalidPanel
    );
    static_assert(
        PlaceDesktopFooterLeft(
            bkvincePanel, WidgetRect{}, bkvinceGoldButton,
            bkvinceGoldAmount, remoteButton
        ).failure == PlacementFailure::InvalidGrid
    );
    static_assert(
        PlaceDesktopFooterLeft(
            bkvincePanel, bkvinceGrid, WidgetRect{},
            WidgetRect{}, remoteButton
        ).failure == PlacementFailure::InvalidFooter
    );
    static_assert(
        PlaceDesktopFooterLeft(
            bkvincePanel, bkvinceGrid, bkvinceGoldButton,
            bkvinceGoldAmount, WidgetRect{}
        ).failure == PlacementFailure::InvalidButton
    );

    assert(Contains(bkvincePanel, bkvince.rect));
    assert(!Intersects(bkvince.rect, bkvinceGrid));
}
