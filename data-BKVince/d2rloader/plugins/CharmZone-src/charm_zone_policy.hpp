#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace ruffneckk::charm_zone {

inline constexpr std::uint32_t InventoryPage = 0;
inline constexpr std::int32_t CharmItemTypeId = 13;
inline constexpr std::size_t MaximumExceptionItemCodes = 16;

constexpr std::uint32_t PackItemCodeBytes(
    char first,
    char second = ' ',
    char third = ' ',
    char fourth = ' ') noexcept {
    return static_cast<std::uint32_t>(static_cast<unsigned char>(first))
        | (static_cast<std::uint32_t>(static_cast<unsigned char>(second)) << 8)
        | (static_cast<std::uint32_t>(static_cast<unsigned char>(third)) << 16)
        | (static_cast<std::uint32_t>(static_cast<unsigned char>(fourth)) << 24);
}

struct ExceptionConfig {
    std::array<std::uint32_t, MaximumExceptionItemCodes> itemCodes{
        PackItemCodeBytes('m', 'f', 'd'),
        PackItemCodeBytes('m', 'f', 'c'),
        PackItemCodeBytes('m', 'f', 'f')
    };
    std::size_t itemCodeCount{3};
};

struct Zone {
    std::uint16_t gridWidth{11};
    std::uint16_t gridHeight{8};
    std::uint16_t left{0};
    std::uint16_t top{4};
    std::uint16_t width{11};
    std::uint16_t height{4};
};

struct ItemPlacement {
    std::uint32_t page{};
    std::uint16_t x{};
    std::uint16_t y{};
    std::uint8_t width{};
    std::uint8_t height{};
};

struct ScreenRect {
    float left{};
    float top{};
    float right{};
    float bottom{};
};

constexpr bool IsExceptionItemCode(
    const ExceptionConfig& exceptions,
    std::uint32_t itemCode) noexcept {
    for (std::size_t index = 0; index < exceptions.itemCodeCount; ++index) {
        if (exceptions.itemCodes[index] == itemCode) return true;
    }
    return false;
}

constexpr bool IsZoneValid(const Zone& zone) noexcept {
    return zone.gridWidth != 0
        && zone.gridHeight != 0
        && zone.width != 0
        && zone.height != 0
        && static_cast<std::uint32_t>(zone.left) + zone.width <= zone.gridWidth
        && static_cast<std::uint32_t>(zone.top) + zone.height <= zone.gridHeight;
}

constexpr bool IsFullyContained(
    const Zone& zone,
    const ItemPlacement& item) noexcept {
    if (!IsZoneValid(zone)
        || item.page != InventoryPage
        || item.width == 0
        || item.height == 0) {
        return false;
    }
    const auto itemRight = static_cast<std::uint32_t>(item.x) + item.width;
    const auto itemBottom = static_cast<std::uint32_t>(item.y) + item.height;
    const auto zoneRight = static_cast<std::uint32_t>(zone.left) + zone.width;
    const auto zoneBottom = static_cast<std::uint32_t>(zone.top) + zone.height;
    return item.x >= zone.left
        && item.y >= zone.top
        && itemRight <= zoneRight
        && itemBottom <= zoneBottom;
}

inline bool TryMakeScreenRect(
    std::int32_t screenX,
    std::int32_t screenY,
    float scale,
    std::uint8_t itemWidth,
    std::uint8_t itemHeight,
    float cellSize,
    float displayHeight,
    ScreenRect& output) noexcept {
    if (screenX < 0
        || screenY < 0
        || !std::isfinite(scale)
        || !std::isfinite(cellSize)
        || !std::isfinite(displayHeight)
        || scale <= 0.0f
        || cellSize <= 0.0f
        || displayHeight <= 0.0f
        || itemWidth == 0
        || itemHeight == 0) {
        return false;
    }
    const float width = cellSize * scale * itemWidth;
    const float height = cellSize * scale * itemHeight;
    output.left = static_cast<float>(screenX);
    output.right = output.left + width;
    output.top = static_cast<float>(screenY);
    output.bottom = output.top + height;
    return std::isfinite(output.right)
        && std::isfinite(output.top)
        && output.right > output.left
        && output.bottom > output.top;
}

constexpr bool ContainsPoint(
    const ScreenRect& rect,
    float x,
    float y) noexcept {
    return x >= rect.left && x <= rect.right
        && y >= rect.top && y <= rect.bottom;
}

} // namespace ruffneckk::charm_zone
