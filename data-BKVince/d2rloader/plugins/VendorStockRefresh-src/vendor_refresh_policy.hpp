#pragma once

#include <cstdint>

namespace ruffneckk::vendor_stock_refresh {

constexpr std::uint8_t NormalVendorMode = 2;
constexpr std::uint8_t GambleVendorMode = 3;

constexpr bool ShouldArmNormalRefresh(
    bool enabled,
    std::uint8_t requestedMode,
    std::uint8_t currentMode,
    std::int32_t currentVendorClass,
    std::int32_t targetVendorClass,
    bool hasVendorEntry,
    bool vendorInventoryFilled
) noexcept {
    return enabled
        && requestedMode == NormalVendorMode
        && currentMode == NormalVendorMode
        && currentVendorClass == targetVendorClass
        && hasVendorEntry
        && vendorInventoryFilled;
}

constexpr std::uint32_t RefreshActionForPanel(bool isGambling) noexcept {
    return isGambling ? 2u : 1u;
}

} // namespace ruffneckk::vendor_stock_refresh
