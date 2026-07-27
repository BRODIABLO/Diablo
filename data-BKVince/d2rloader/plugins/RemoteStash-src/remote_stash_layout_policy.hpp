#pragma once

#include <cstdint>
#include <limits>

namespace ruffneckk::remote_stash {

struct WidgetRect {
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t width{};
    std::int32_t height{};
};

enum class PlacementFailure : std::uint8_t {
    None,
    InvalidPanel,
    InvalidGrid,
    InvalidFooter,
    InvalidButton,
    CoordinateOverflow,
    OutsidePanel,
    GridCollision,
    FooterCollision,
};

struct PlacementResult {
    bool valid{};
    WidgetRect rect{};
    PlacementFailure failure{PlacementFailure::InvalidPanel};
};

constexpr bool HasUsableSize(const WidgetRect& rect) noexcept {
    return rect.width > 0 && rect.height > 0;
}

constexpr std::int64_t Right(const WidgetRect& rect) noexcept {
    return static_cast<std::int64_t>(rect.x) + rect.width;
}

constexpr std::int64_t Bottom(const WidgetRect& rect) noexcept {
    return static_cast<std::int64_t>(rect.y) + rect.height;
}

constexpr bool Contains(
    const WidgetRect& bounds,
    const WidgetRect& candidate
) noexcept {
    return HasUsableSize(bounds)
        && HasUsableSize(candidate)
        && candidate.x >= bounds.x
        && candidate.y >= bounds.y
        && Right(candidate) <= Right(bounds)
        && Bottom(candidate) <= Bottom(bounds);
}

constexpr bool Intersects(
    const WidgetRect& first,
    const WidgetRect& second
) noexcept {
    return HasUsableSize(first)
        && HasUsableSize(second)
        && first.x < Right(second)
        && second.x < Right(first)
        && first.y < Bottom(second)
        && second.y < Bottom(first);
}

constexpr WidgetRect UnionRect(
    const WidgetRect& first,
    const WidgetRect& second
) noexcept {
    if (!HasUsableSize(first)) return second;
    if (!HasUsableSize(second)) return first;

    const auto left = first.x < second.x ? first.x : second.x;
    const auto top = first.y < second.y ? first.y : second.y;
    const auto right = Right(first) > Right(second) ? Right(first) : Right(second);
    const auto bottom = Bottom(first) > Bottom(second) ? Bottom(first) : Bottom(second);
    const auto width = right - left;
    const auto height = bottom - top;
    if (width > std::numeric_limits<std::int32_t>::max()
        || height > std::numeric_limits<std::int32_t>::max()) {
        return {};
    }
    return {
        .x = left,
        .y = top,
        .width = static_cast<std::int32_t>(width),
        .height = static_cast<std::int32_t>(height),
    };
}

// Desktop policy: align the button with the runtime inventory-grid origin and
// keep it as close as possible to the runtime gold footer's vertical center.
// When a taller custom sprite would overlap the grid, move it down into the
// available footer strip. Controller layouts require a separate policy because
// their gold button occupies that same horizontal area.
constexpr PlacementResult PlaceDesktopFooterLeft(
    const WidgetRect& panel,
    const WidgetRect& grid,
    const WidgetRect& goldButton,
    const WidgetRect& goldAmount,
    const WidgetRect& button
) noexcept {
    if (!HasUsableSize(panel)) {
        return {.failure = PlacementFailure::InvalidPanel};
    }
    if (!HasUsableSize(grid)) {
        return {.failure = PlacementFailure::InvalidGrid};
    }
    const auto footer = UnionRect(goldButton, goldAmount);
    if (!HasUsableSize(footer)) {
        return {.failure = PlacementFailure::InvalidFooter};
    }
    if (!HasUsableSize(button)) {
        return {.failure = PlacementFailure::InvalidButton};
    }

    const auto preferredY = static_cast<std::int64_t>(footer.y)
        + (static_cast<std::int64_t>(footer.height) - button.height) / 2;
    const auto minimumY = Bottom(grid);
    const auto maximumY = Bottom(panel) - button.height;
    if (minimumY > maximumY) {
        return {.failure = PlacementFailure::GridCollision};
    }
    auto y = preferredY < minimumY ? minimumY : preferredY;
    if (y > maximumY) y = maximumY;
    if (y < std::numeric_limits<std::int32_t>::min()
        || y > std::numeric_limits<std::int32_t>::max()) {
        return {.failure = PlacementFailure::CoordinateOverflow};
    }

    const WidgetRect candidate{
        .x = grid.x,
        .y = static_cast<std::int32_t>(y),
        .width = button.width,
        .height = button.height,
    };
    if (!Contains(panel, candidate)) {
        return {.failure = PlacementFailure::OutsidePanel};
    }
    if (Intersects(candidate, grid)) {
        return {.failure = PlacementFailure::GridCollision};
    }
    if (Intersects(candidate, footer)) {
        return {.failure = PlacementFailure::FooterCollision};
    }
    return {
        .valid = true,
        .rect = candidate,
        .failure = PlacementFailure::None,
    };
}

} // namespace ruffneckk::remote_stash
