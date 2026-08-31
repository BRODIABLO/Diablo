#pragma once

#include "autosort_config.hpp"

#include <algorithm>
#include <array>
#include <compare>
#include <cstdlib>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ruffneckk::autosort {

struct Position {
    std::uint8_t x{};
    std::uint8_t y{};

    auto operator<=>(const Position&) const = default;
};

struct GridItem {
    std::uint32_t guid{};
    std::uint8_t width{};
    std::uint8_t height{};
    Position current{};
    Anchor anchor{Anchor::Middle};
    std::size_t groupOrder{};
    std::size_t subgroupOrder{};
    std::uint64_t itemOrder{};
};

struct Placement {
    std::uint32_t guid{};
    std::uint8_t width{};
    std::uint8_t height{};
    Position from{};
    Position to{};
    Anchor anchor{Anchor::Middle};
    std::size_t groupOrder{};
    std::size_t subgroupOrder{};
    std::uint64_t itemOrder{};
};

struct FreeRectangle {
    std::uint8_t x{};
    std::uint8_t y{};
    std::uint8_t width{};
    std::uint8_t height{};
    std::uint16_t area{};
};

struct Plan {
    bool success{};
    bool changed{};
    bool usedPackingFallback{};
    std::size_t movedItemCount{};
    FreeRectangle largestFreeRectangle{};
    std::uint64_t anchorPenalty{};
    std::vector<Placement> placements;
    std::string error;
};

inline bool BetterFreeRectangle(
        const FreeRectangle& left,
        const FreeRectangle& right) noexcept {
    const auto leftMinimum = std::min(left.width, left.height);
    const auto rightMinimum = std::min(right.width, right.height);
    const auto leftMaximum = std::max(left.width, left.height);
    const auto rightMaximum = std::max(right.width, right.height);
    if (left.area != right.area) return left.area > right.area;
    if (leftMinimum != rightMinimum) return leftMinimum > rightMinimum;
    if (leftMaximum != rightMaximum) return leftMaximum > rightMaximum;
    if (left.y != right.y) return left.y < right.y;
    return left.x < right.x;
}

inline FreeRectangle LargestFreeRectangle(
        std::uint8_t gridWidth,
        std::uint8_t gridHeight,
        const std::vector<std::uint32_t>& occupancy) {
    FreeRectangle best{};
    for (std::uint8_t top = 0; top < gridHeight; ++top) {
        std::vector<bool> columns(gridWidth, true);
        for (std::uint8_t bottom = top; bottom < gridHeight; ++bottom) {
            for (std::uint8_t x = 0; x < gridWidth; ++x) {
                columns[x] = columns[x]
                    && occupancy[static_cast<std::size_t>(bottom) * gridWidth
                        + x] == 0;
            }
            std::uint8_t runStart{};
            std::uint8_t runLength{};
            for (std::uint8_t x = 0; x <= gridWidth; ++x) {
                const auto free = x < gridWidth && columns[x];
                if (free) {
                    if (runLength == 0) runStart = x;
                    ++runLength;
                    continue;
                }
                if (runLength != 0) {
                    const auto height = static_cast<std::uint8_t>(
                        bottom - top + 1);
                    const FreeRectangle candidate{
                        .x = runStart,
                        .y = top,
                        .width = runLength,
                        .height = height,
                        .area = static_cast<std::uint16_t>(
                            runLength * height),
                    };
                    if (BetterFreeRectangle(candidate, best)) {
                        best = candidate;
                    }
                }
                runLength = 0;
            }
        }
    }
    return best;
}

inline std::pair<std::int32_t, std::int32_t> AnchorTargetCenter2(
        Anchor anchor,
        std::uint8_t gridWidth,
        std::uint8_t gridHeight) noexcept {
    const auto left = 0;
    const auto horizontalMiddle = static_cast<std::int32_t>(gridWidth);
    const auto right = static_cast<std::int32_t>(gridWidth) * 2;
    const auto top = 0;
    const auto verticalMiddle = static_cast<std::int32_t>(gridHeight);
    const auto bottom = static_cast<std::int32_t>(gridHeight) * 2;
    switch (anchor) {
    case Anchor::TopLeft: return {left, top};
    case Anchor::TopMiddle: return {horizontalMiddle, top};
    case Anchor::TopRight: return {right, top};
    case Anchor::MiddleLeft: return {left, verticalMiddle};
    case Anchor::Middle: return {horizontalMiddle, verticalMiddle};
    case Anchor::MiddleRight: return {right, verticalMiddle};
    case Anchor::BottomLeft: return {left, bottom};
    case Anchor::BottomMiddle: return {horizontalMiddle, bottom};
    case Anchor::BottomRight: return {right, bottom};
    case Anchor::Ignore: return {horizontalMiddle, verticalMiddle};
    }
    return {horizontalMiddle, verticalMiddle};
}

inline std::uint32_t AnchorPenalty(
        const GridItem& item,
        Position position,
        std::uint8_t gridWidth,
        std::uint8_t gridHeight) noexcept {
    const auto [targetX, targetY] =
        AnchorTargetCenter2(item.anchor, gridWidth, gridHeight);
    const auto centerX = static_cast<std::int32_t>(position.x) * 2
        + item.width;
    const auto centerY = static_cast<std::int32_t>(position.y) * 2
        + item.height;
    return static_cast<std::uint32_t>(
        std::abs(centerX - targetX) + std::abs(centerY - targetY));
}

inline std::vector<Position> CandidatePositions(
        const GridItem& item,
        std::uint8_t gridWidth,
        std::uint8_t gridHeight) {
    std::vector<Position> positions;
    if (item.anchor == Anchor::Ignore
            || item.width > gridWidth
            || item.height > gridHeight) {
        return positions;
    }
    for (std::uint8_t y = 0; y + item.height <= gridHeight; ++y) {
        for (std::uint8_t x = 0; x + item.width <= gridWidth; ++x) {
            positions.push_back(Position{x, y});
        }
    }

    const auto horizontalMiddle = item.anchor == Anchor::TopMiddle
        || item.anchor == Anchor::Middle
        || item.anchor == Anchor::BottomMiddle;
    const auto verticalMiddle = item.anchor == Anchor::MiddleLeft
        || item.anchor == Anchor::Middle
        || item.anchor == Anchor::MiddleRight;
    const auto rightAligned = item.anchor == Anchor::TopRight
        || item.anchor == Anchor::MiddleRight
        || item.anchor == Anchor::BottomRight;
    const auto bottomAligned = item.anchor == Anchor::BottomLeft
        || item.anchor == Anchor::BottomMiddle
        || item.anchor == Anchor::BottomRight;
    std::sort(
        positions.begin(),
        positions.end(),
        [&](Position left, Position right) {
            const auto verticalKey = [&](Position position) {
                const auto coordinate = static_cast<std::int32_t>(position.y);
                if (verticalMiddle) {
                    const auto center = coordinate * 2 + item.height;
                    return std::pair{
                        std::abs(center - static_cast<std::int32_t>(gridHeight)),
                        coordinate};
                }
                return std::pair{
                    0,
                    bottomAligned ? -coordinate : coordinate};
            };
            const auto horizontalKey = [&](Position position) {
                const auto coordinate = static_cast<std::int32_t>(position.x);
                if (horizontalMiddle) {
                    const auto center = coordinate * 2 + item.width;
                    return std::pair{
                        std::abs(center - static_cast<std::int32_t>(gridWidth)),
                        coordinate};
                }
                return std::pair{
                    0,
                    rightAligned ? -coordinate : coordinate};
            };
            return std::tuple{verticalKey(left), horizontalKey(left)}
                < std::tuple{verticalKey(right), horizontalKey(right)};
        });
    return positions;
}

inline bool Fits(
        const GridItem& item,
        Position position,
        std::uint8_t gridWidth,
        std::uint8_t gridHeight,
        const std::vector<std::uint32_t>& occupancy) noexcept {
    if (position.x + item.width > gridWidth
            || position.y + item.height > gridHeight) {
        return false;
    }
    for (std::uint8_t y = 0; y < item.height; ++y) {
        for (std::uint8_t x = 0; x < item.width; ++x) {
            if (occupancy[
                    static_cast<std::size_t>(position.y + y) * gridWidth
                        + position.x + x] != 0) {
                return false;
            }
        }
    }
    return true;
}

inline void Occupy(
        const GridItem& item,
        Position position,
        std::uint8_t gridWidth,
        std::vector<std::uint32_t>& occupancy) noexcept {
    for (std::uint8_t y = 0; y < item.height; ++y) {
        for (std::uint8_t x = 0; x < item.width; ++x) {
            occupancy[
                static_cast<std::size_t>(position.y + y) * gridWidth
                    + position.x + x] = item.guid;
        }
    }
}

inline auto DimensionOrderKey(
        const GridItem& item,
        unsigned variant) noexcept {
    const auto area = static_cast<std::int32_t>(item.width) * item.height;
    const auto maximum = std::max(item.width, item.height);
    const auto minimum = std::min(item.width, item.height);
    switch (variant) {
    case 0:
        return std::tuple{
            -area, -static_cast<int>(item.height),
            -static_cast<int>(item.width), item.guid};
    case 1:
        return std::tuple{
            -area, -static_cast<int>(item.width),
            -static_cast<int>(item.height), item.guid};
    case 2:
        return std::tuple{
            -static_cast<int>(item.height),
            -static_cast<int>(item.width), -area, item.guid};
    default:
        return std::tuple{
            -static_cast<int>(item.width),
            -static_cast<int>(item.height),
            -static_cast<int>(maximum + minimum), item.guid};
    }
}

struct CandidateLayout {
    FreeRectangle freeRectangle{};
    std::uint64_t anchorPenalty{};
    bool usedPackingFallback{};
    std::vector<Placement> placements;
};

struct ClusterBounds {
    std::uint8_t left{};
    std::uint8_t top{};
    std::uint8_t right{};
    std::uint8_t bottom{};
};

inline bool BoundsOverlap(
        const ClusterBounds& left,
        const ClusterBounds& right) noexcept {
    return left.left <= right.right && right.left <= left.right
        && left.top <= right.bottom && right.top <= left.bottom;
}

template <typename Key, typename MakeKey, typename SameParent>
inline bool CohesionLevelIsValid(
        std::uint8_t gridWidth,
        std::uint8_t gridHeight,
        const std::vector<Placement>& placements,
        MakeKey makeKey,
        SameParent sameParent) {
    std::map<Key, std::vector<const Placement*>> clusters;
    for (const auto& placement : placements) {
        if (placement.anchor == Anchor::Ignore) continue;
        clusters[makeKey(placement)].push_back(&placement);
    }

    std::vector<std::pair<Key, ClusterBounds>> footprints;
    footprints.reserve(clusters.size());
    for (const auto& [key, members] : clusters) {
        std::vector<bool> cells(
            static_cast<std::size_t>(gridWidth) * gridHeight);
        ClusterBounds bounds{
            .left = gridWidth,
            .top = gridHeight,
        };
        std::size_t occupiedCellCount{};
        for (const auto* member : members) {
            for (std::uint8_t y = 0; y < member->height; ++y) {
                for (std::uint8_t x = 0; x < member->width; ++x) {
                    const auto cellX = static_cast<std::uint8_t>(
                        member->to.x + x);
                    const auto cellY = static_cast<std::uint8_t>(
                        member->to.y + y);
                    const auto index = static_cast<std::size_t>(cellY)
                        * gridWidth + cellX;
                    if (!cells[index]) {
                        cells[index] = true;
                        ++occupiedCellCount;
                    }
                    bounds.left = std::min(bounds.left, cellX);
                    bounds.top = std::min(bounds.top, cellY);
                    bounds.right = std::max(bounds.right, cellX);
                    bounds.bottom = std::max(bounds.bottom, cellY);
                }
            }
        }
        if (occupiedCellCount == 0) return false;

        const auto first = std::find(cells.begin(), cells.end(), true);
        if (first == cells.end()) return false;
        std::vector<bool> visited(cells.size());
        std::vector<std::size_t> pending{
            static_cast<std::size_t>(std::distance(cells.begin(), first))};
        visited[pending.front()] = true;
        std::size_t connectedCellCount{};
        while (!pending.empty()) {
            const auto index = pending.back();
            pending.pop_back();
            ++connectedCellCount;
            const auto x = index % gridWidth;
            const auto y = index / gridWidth;
            const auto visit = [&](std::size_t neighbor) {
                if (cells[neighbor] && !visited[neighbor]) {
                    visited[neighbor] = true;
                    pending.push_back(neighbor);
                }
            };
            if (x > 0) visit(index - 1);
            if (x + 1 < gridWidth) visit(index + 1);
            if (y > 0) visit(index - gridWidth);
            if (y + 1 < gridHeight) visit(index + gridWidth);
        }
        if (connectedCellCount != occupiedCellCount) return false;
        footprints.emplace_back(key, bounds);
    }

    for (std::size_t left = 0; left < footprints.size(); ++left) {
        for (std::size_t right = left + 1;
             right < footprints.size(); ++right) {
            if (sameParent(footprints[left].first, footprints[right].first)
                    && BoundsOverlap(
                        footprints[left].second,
                        footprints[right].second)) {
                return false;
            }
        }
    }
    return true;
}

inline bool HasStrictHierarchicalCohesion(
        std::uint8_t gridWidth,
        std::uint8_t gridHeight,
        const std::vector<Placement>& placements) {
    using GroupKey = std::tuple<Anchor, std::size_t>;
    using SubgroupKey = std::tuple<Anchor, std::size_t, std::size_t>;
    using ItemKey = std::tuple<
        Anchor, std::size_t, std::size_t, std::uint64_t>;

    const auto groupsAreValid = CohesionLevelIsValid<GroupKey>(
        gridWidth,
        gridHeight,
        placements,
        [](const Placement& placement) {
            return GroupKey{placement.anchor, placement.groupOrder};
        },
        [](const GroupKey& left, const GroupKey& right) {
            return std::get<0>(left) == std::get<0>(right);
        });
    if (!groupsAreValid) return false;

    const auto subgroupsAreValid = CohesionLevelIsValid<SubgroupKey>(
        gridWidth,
        gridHeight,
        placements,
        [](const Placement& placement) {
            return SubgroupKey{
                placement.anchor,
                placement.groupOrder,
                placement.subgroupOrder};
        },
        [](const SubgroupKey& left, const SubgroupKey& right) {
            return std::get<0>(left) == std::get<0>(right)
                && std::get<1>(left) == std::get<1>(right);
        });
    if (!subgroupsAreValid) return false;

    return CohesionLevelIsValid<ItemKey>(
        gridWidth,
        gridHeight,
        placements,
        [](const Placement& placement) {
            return ItemKey{
                placement.anchor,
                placement.groupOrder,
                placement.subgroupOrder,
                placement.itemOrder};
        },
        [](const ItemKey& left, const ItemKey& right) {
            return std::get<0>(left) == std::get<0>(right)
                && std::get<1>(left) == std::get<1>(right)
                && std::get<2>(left) == std::get<2>(right);
        });
}

struct LocalBlockLayout {
    std::uint8_t width{};
    std::uint8_t height{};
    bool usedPackingFallback{};
    std::vector<Placement> placements;
};

inline std::optional<LocalBlockLayout> PackItemsAsShelf(
        std::vector<GridItem> items,
        std::uint8_t widthLimit,
        unsigned dimensionVariant) {
    if (items.empty() || widthLimit == 0) return std::nullopt;
    std::sort(
        items.begin(),
        items.end(),
        [dimensionVariant](const GridItem& left, const GridItem& right) {
            return DimensionOrderKey(left, dimensionVariant)
                < DimensionOrderKey(right, dimensionVariant);
        });

    LocalBlockLayout result{};
    std::uint16_t x{};
    std::uint16_t y{};
    std::uint16_t rowHeight{};
    std::uint16_t usedWidth{};
    for (const auto& item : items) {
        if (item.width > widthLimit) return std::nullopt;
        if (x != 0 && x + item.width > widthLimit) {
            y += rowHeight;
            x = 0;
            rowHeight = 0;
        }
        if (y + item.height > std::numeric_limits<std::uint8_t>::max()) {
            return std::nullopt;
        }
        result.placements.push_back(Placement{
            .guid = item.guid,
            .width = item.width,
            .height = item.height,
            .from = item.current,
            .to = {
                static_cast<std::uint8_t>(x),
                static_cast<std::uint8_t>(y),
            },
            .anchor = item.anchor,
            .groupOrder = item.groupOrder,
            .subgroupOrder = item.subgroupOrder,
            .itemOrder = item.itemOrder,
        });
        x += item.width;
        rowHeight = std::max<std::uint16_t>(rowHeight, item.height);
        usedWidth = std::max(usedWidth, x);
    }
    const auto usedHeight = y + rowHeight;
    if (usedWidth == 0 || usedHeight == 0
            || usedWidth > std::numeric_limits<std::uint8_t>::max()
            || usedHeight > std::numeric_limits<std::uint8_t>::max()) {
        return std::nullopt;
    }
    result.width = static_cast<std::uint8_t>(usedWidth);
    result.height = static_cast<std::uint8_t>(usedHeight);
    return result;
}

inline std::optional<LocalBlockLayout> PackBlocksAsShelf(
        const std::vector<LocalBlockLayout>& blocks,
        std::uint8_t widthLimit) {
    if (blocks.empty() || widthLimit == 0) return std::nullopt;
    LocalBlockLayout result{};
    std::uint16_t x{};
    std::uint16_t y{};
    std::uint16_t rowHeight{};
    std::uint16_t usedWidth{};
    for (const auto& block : blocks) {
        if (block.width == 0 || block.height == 0
                || block.width > widthLimit) {
            return std::nullopt;
        }
        result.usedPackingFallback = result.usedPackingFallback
            || block.usedPackingFallback;
        if (x != 0 && x + block.width > widthLimit) {
            y += rowHeight;
            x = 0;
            rowHeight = 0;
        }
        if (y + block.height > std::numeric_limits<std::uint8_t>::max()) {
            return std::nullopt;
        }
        for (const auto& placement : block.placements) {
            auto translated = placement;
            translated.to.x = static_cast<std::uint8_t>(
                translated.to.x + x);
            translated.to.y = static_cast<std::uint8_t>(
                translated.to.y + y);
            result.placements.push_back(translated);
        }
        x += block.width;
        rowHeight = std::max<std::uint16_t>(rowHeight, block.height);
        usedWidth = std::max(usedWidth, x);
    }
    const auto usedHeight = y + rowHeight;
    if (usedWidth == 0 || usedHeight == 0
            || usedWidth > std::numeric_limits<std::uint8_t>::max()
            || usedHeight > std::numeric_limits<std::uint8_t>::max()) {
        return std::nullopt;
    }
    result.width = static_cast<std::uint8_t>(usedWidth);
    result.height = static_cast<std::uint8_t>(usedHeight);
    return result;
}

inline std::optional<LocalBlockLayout> BuildHierarchyGroupLayout(
        const std::vector<GridItem>& items,
        std::uint8_t widthLimit,
        std::uint8_t gridHeight,
        unsigned dimensionVariant) {
    if (items.empty()) return std::nullopt;
    std::map<
        std::size_t,
        std::map<std::uint64_t, std::vector<GridItem>>> hierarchy;
    for (const auto& item : items) {
        hierarchy[item.subgroupOrder][item.itemOrder].push_back(item);
    }

    std::vector<LocalBlockLayout> subgroupBlocks;
    subgroupBlocks.reserve(hierarchy.size());
    for (const auto& [subgroupOrder, itemGroups] : hierarchy) {
        (void)subgroupOrder;
        std::vector<LocalBlockLayout> itemBlocks;
        itemBlocks.reserve(itemGroups.size());
        for (const auto& [itemOrder, groupedItems] : itemGroups) {
            (void)itemOrder;
            auto itemBlock = PackItemsAsShelf(
                groupedItems, widthLimit, dimensionVariant);
            if (!itemBlock) return std::nullopt;
            itemBlocks.push_back(std::move(*itemBlock));
        }
        if (dimensionVariant != 0) {
            std::stable_sort(
                itemBlocks.begin(),
                itemBlocks.end(),
                [dimensionVariant](
                        const LocalBlockLayout& left,
                        const LocalBlockLayout& right) {
                    const auto key = [&](const LocalBlockLayout& block) {
                        const auto area = static_cast<std::int32_t>(
                            block.width * block.height);
                        if (dimensionVariant == 1) {
                            return std::tuple{
                                -area,
                                -static_cast<std::int32_t>(block.height),
                                -static_cast<std::int32_t>(block.width)};
                        }
                        if (dimensionVariant == 2) {
                            return std::tuple{
                                -area,
                                -static_cast<std::int32_t>(block.width),
                                -static_cast<std::int32_t>(block.height)};
                        }
                        return std::tuple{
                            -static_cast<std::int32_t>(
                                std::max(block.width, block.height)),
                            -area,
                            -static_cast<std::int32_t>(
                                std::min(block.width, block.height))};
                    };
                    return key(left) < key(right);
                });
            for (auto& itemBlock : itemBlocks) {
                itemBlock.usedPackingFallback = true;
            }
        }
        auto subgroup = PackBlocksAsShelf(itemBlocks, widthLimit);
        if (!subgroup) return std::nullopt;
        subgroupBlocks.push_back(std::move(*subgroup));
    }

    auto group = PackBlocksAsShelf(subgroupBlocks, widthLimit);
    if (!group || group->height > gridHeight) return std::nullopt;
    std::sort(
        group->placements.begin(),
        group->placements.end(),
        [](const Placement& left, const Placement& right) {
            return left.guid < right.guid;
        });
    return group;
}

inline bool SameLocalLayout(
        const LocalBlockLayout& left,
        const LocalBlockLayout& right) noexcept {
    if (left.width != right.width || left.height != right.height
            || left.placements.size() != right.placements.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.placements.size(); ++index) {
        if (left.placements[index].guid != right.placements[index].guid
                || left.placements[index].to
                    != right.placements[index].to) {
            return false;
        }
    }
    return true;
}

struct HierarchyGroup {
    Anchor anchor{Anchor::Middle};
    std::size_t groupOrder{};
    std::uint16_t occupiedArea{};
    std::uint8_t maximumItemDimension{};
    std::vector<LocalBlockLayout> layouts;
};

inline std::vector<HierarchyGroup> BuildHierarchyGroups(
        std::uint8_t gridWidth,
        std::uint8_t gridHeight,
        const std::vector<GridItem>& items) {
    std::map<std::pair<Anchor, std::size_t>, std::vector<GridItem>> grouped;
    for (const auto& item : items) {
        if (item.anchor != Anchor::Ignore) {
            grouped[{item.anchor, item.groupOrder}].push_back(item);
        }
    }

    std::vector<HierarchyGroup> result;
    result.reserve(grouped.size());
    constexpr unsigned DimensionVariantCount = 2;
    for (const auto& [key, members] : grouped) {
        HierarchyGroup group{
            .anchor = key.first,
            .groupOrder = key.second,
        };
        for (const auto& item : members) {
            group.occupiedArea = static_cast<std::uint16_t>(
                group.occupiedArea + item.width * item.height);
            group.maximumItemDimension = std::max(
                group.maximumItemDimension,
                std::max(item.width, item.height));
        }
        for (std::uint8_t widthLimit = 1;
             widthLimit <= gridWidth; ++widthLimit) {
            for (unsigned variant = 0;
                 variant < DimensionVariantCount; ++variant) {
                auto layout = BuildHierarchyGroupLayout(
                    members,
                    widthLimit,
                    gridHeight,
                    variant);
                if (!layout) continue;
                const auto duplicate = std::any_of(
                    group.layouts.begin(),
                    group.layouts.end(),
                    [&](const LocalBlockLayout& existing) {
                        return SameLocalLayout(existing, *layout);
                    });
                if (!duplicate) group.layouts.push_back(std::move(*layout));
            }
        }
        result.push_back(std::move(group));
    }
    return result;
}

inline bool RectangleFits(
        std::uint8_t width,
        std::uint8_t height,
        Position position,
        std::uint8_t gridWidth,
        std::uint8_t gridHeight,
        const std::vector<std::uint32_t>& occupancy) noexcept {
    if (position.x + width > gridWidth
            || position.y + height > gridHeight) {
        return false;
    }
    for (std::uint8_t y = 0; y < height; ++y) {
        for (std::uint8_t x = 0; x < width; ++x) {
            if (occupancy[
                    static_cast<std::size_t>(position.y + y) * gridWidth
                        + position.x + x] != 0) {
                return false;
            }
        }
    }
    return true;
}

inline void OccupyRectangle(
        std::uint8_t width,
        std::uint8_t height,
        Position position,
        std::uint8_t gridWidth,
        std::uint32_t marker,
        std::vector<std::uint32_t>& occupancy) noexcept {
    for (std::uint8_t y = 0; y < height; ++y) {
        for (std::uint8_t x = 0; x < width; ++x) {
            occupancy[
                static_cast<std::size_t>(position.y + y) * gridWidth
                    + position.x + x] = marker;
        }
    }
}

inline auto LayoutPreferenceKey(
        const LocalBlockLayout& layout,
        Anchor anchor,
        unsigned variant) noexcept {
    const auto area = static_cast<std::int32_t>(
        layout.width * layout.height);
    const auto horizontalAnchor = anchor == Anchor::TopLeft
        || anchor == Anchor::TopMiddle || anchor == Anchor::TopRight
        || anchor == Anchor::BottomLeft
        || anchor == Anchor::BottomMiddle
        || anchor == Anchor::BottomRight;
    switch (variant) {
    case 0:
        return std::tuple{
            area,
            static_cast<std::int32_t>(layout.height),
            static_cast<std::int32_t>(layout.width)};
    case 1:
        return std::tuple{
            static_cast<std::int32_t>(layout.height),
            area,
            static_cast<std::int32_t>(layout.width)};
    case 2:
        return std::tuple{
            static_cast<std::int32_t>(layout.width),
            area,
            static_cast<std::int32_t>(layout.height)};
    default:
        return horizontalAnchor
            ? std::tuple{
                static_cast<std::int32_t>(layout.height),
                area,
                static_cast<std::int32_t>(layout.width)}
            : std::tuple{
                static_cast<std::int32_t>(layout.width),
                area,
                static_cast<std::int32_t>(layout.height)};
    }
}

inline bool BetterCandidate(
        const CandidateLayout& left,
        const CandidateLayout& right,
        bool optimizeFreeSpace) noexcept {
    // Anchors are explicit user/modder placement preferences. Free-space
    // optimization may choose among equally faithful anchored layouts, but it
    // must never move a configured group elsewhere merely to enlarge a hole.
    if (left.anchorPenalty != right.anchorPenalty) {
        return left.anchorPenalty < right.anchorPenalty;
    }
    if (optimizeFreeSpace) {
        if (BetterFreeRectangle(left.freeRectangle, right.freeRectangle)) {
            return true;
        }
        if (BetterFreeRectangle(right.freeRectangle, left.freeRectangle)) {
            return false;
        }
    }
    return std::lexicographical_compare(
        left.placements.begin(),
        left.placements.end(),
        right.placements.begin(),
        right.placements.end(),
        [](const Placement& a, const Placement& b) {
            return std::tie(a.guid, a.to.y, a.to.x)
                < std::tie(b.guid, b.to.y, b.to.x);
        });
}

inline std::optional<CandidateLayout> BuildHierarchyCandidate(
        std::uint8_t gridWidth,
        std::uint8_t gridHeight,
        const std::vector<GridItem>& items,
        const std::vector<HierarchyGroup>& groups,
        unsigned blockOrderVariant,
        unsigned layoutPreferenceVariant,
        std::uint8_t reservedRightColumns) {
    std::vector<std::uint32_t> reservedOccupancy(
        static_cast<std::size_t>(gridWidth) * gridHeight);
    CandidateLayout candidate{};
    candidate.usedPackingFallback = blockOrderVariant != 0
        || layoutPreferenceVariant != 0;
    for (const auto& item : items) {
        if (item.anchor == Anchor::Ignore) {
            if (!Fits(
                    item,
                    item.current,
                    gridWidth,
                    gridHeight,
                    reservedOccupancy)) {
                return std::nullopt;
            }
            Occupy(item, item.current, gridWidth, reservedOccupancy);
            candidate.placements.push_back(Placement{
                .guid = item.guid,
                .width = item.width,
                .height = item.height,
                .from = item.current,
                .to = item.current,
                .anchor = item.anchor,
                .groupOrder = item.groupOrder,
                .subgroupOrder = item.subgroupOrder,
                .itemOrder = item.itemOrder,
            });
        }
    }
    const auto firstReservedX = static_cast<std::uint8_t>(
        gridWidth - reservedRightColumns);
    for (std::uint8_t y = 0; y < gridHeight; ++y) {
        for (std::uint8_t x = firstReservedX; x < gridWidth; ++x) {
            reservedOccupancy[static_cast<std::size_t>(y) * gridWidth + x] =
                std::numeric_limits<std::uint32_t>::max();
        }
    }

    std::vector<std::size_t> groupIndices;
    groupIndices.reserve(groups.size());
    for (std::size_t index = 0; index < groups.size(); ++index) {
        groupIndices.push_back(index);
    }
    std::sort(
        groupIndices.begin(),
        groupIndices.end(),
        [&](std::size_t leftIndex, std::size_t rightIndex) {
            const auto& left = groups[leftIndex];
            const auto& right = groups[rightIndex];
            const auto key = [&](const HierarchyGroup& group) {
                switch (blockOrderVariant) {
                case 0:
                    return std::tuple{
                        std::int64_t{0},
                        static_cast<std::int64_t>(group.groupOrder),
                        static_cast<std::int64_t>(group.anchor)};
                case 1:
                    return std::tuple{
                        -static_cast<std::int64_t>(group.occupiedArea),
                        static_cast<std::int64_t>(group.groupOrder),
                        static_cast<std::int64_t>(group.anchor)};
                case 2:
                    return std::tuple{
                        -static_cast<std::int64_t>(
                            group.maximumItemDimension),
                        -static_cast<std::int64_t>(group.occupiedArea),
                        static_cast<std::int64_t>(group.groupOrder)};
                default:
                    return std::tuple{
                        static_cast<std::int64_t>(group.anchor),
                        static_cast<std::int64_t>(group.groupOrder),
                        -static_cast<std::int64_t>(group.occupiedArea)};
                }
            };
            return std::tuple{key(left), leftIndex}
                < std::tuple{key(right), rightIndex};
        });

    for (const auto groupIndex : groupIndices) {
        const auto& group = groups[groupIndex];
        if (group.layouts.empty()) return std::nullopt;
        std::vector<std::size_t> layoutIndices;
        layoutIndices.reserve(group.layouts.size());
        for (std::size_t index = 0;
             index < group.layouts.size(); ++index) {
            layoutIndices.push_back(index);
        }
        std::sort(
            layoutIndices.begin(),
            layoutIndices.end(),
            [&](std::size_t left, std::size_t right) {
                return std::tuple{
                    LayoutPreferenceKey(
                        group.layouts[left],
                        group.anchor,
                        layoutPreferenceVariant),
                    left}
                    < std::tuple{
                        LayoutPreferenceKey(
                            group.layouts[right],
                            group.anchor,
                            layoutPreferenceVariant),
                        right};
            });

        const LocalBlockLayout* selectedLayout{};
        Position selectedPosition{};
        for (const auto layoutIndex : layoutIndices) {
            const auto& layout = group.layouts[layoutIndex];
            const GridItem footprint{
                .guid = 1,
                .width = layout.width,
                .height = layout.height,
                .anchor = group.anchor,
                .groupOrder = group.groupOrder,
            };
            const auto positions = CandidatePositions(
                footprint, gridWidth, gridHeight);
            const auto position = std::find_if(
                positions.begin(),
                positions.end(),
                [&](Position candidatePosition) {
                    return RectangleFits(
                        layout.width,
                        layout.height,
                        candidatePosition,
                        gridWidth,
                        gridHeight,
                        reservedOccupancy);
                });
            if (position == positions.end()) continue;
            selectedLayout = &layout;
            selectedPosition = *position;
            break;
        }
        if (!selectedLayout) return std::nullopt;

        OccupyRectangle(
            selectedLayout->width,
            selectedLayout->height,
            selectedPosition,
            gridWidth,
            static_cast<std::uint32_t>(groupIndex + 1),
            reservedOccupancy);
        for (const auto& relative : selectedLayout->placements) {
            auto translated = relative;
            const auto rightAligned = group.anchor == Anchor::TopRight
                || group.anchor == Anchor::MiddleRight
                || group.anchor == Anchor::BottomRight;
            const auto bottomAligned = group.anchor == Anchor::BottomLeft
                || group.anchor == Anchor::BottomMiddle
                || group.anchor == Anchor::BottomRight;
            const auto relativeX = rightAligned
                ? selectedLayout->width - relative.to.x - relative.width
                : relative.to.x;
            const auto relativeY = bottomAligned
                ? selectedLayout->height - relative.to.y - relative.height
                : relative.to.y;
            translated.to.x = static_cast<std::uint8_t>(
                relativeX + selectedPosition.x);
            translated.to.y = static_cast<std::uint8_t>(
                relativeY + selectedPosition.y);
            const GridItem penaltyItem{
                .guid = translated.guid,
                .width = translated.width,
                .height = translated.height,
                .current = translated.from,
                .anchor = translated.anchor,
                .groupOrder = translated.groupOrder,
                .subgroupOrder = translated.subgroupOrder,
                .itemOrder = translated.itemOrder,
            };
            candidate.anchorPenalty += AnchorPenalty(
                penaltyItem,
                translated.to,
                gridWidth,
                gridHeight);
            candidate.placements.push_back(translated);
        }
        candidate.usedPackingFallback = candidate.usedPackingFallback
            || selectedLayout->usedPackingFallback;
    }

    std::sort(
        candidate.placements.begin(),
        candidate.placements.end(),
        [](const Placement& left, const Placement& right) {
            return left.guid < right.guid;
        });
    if (!HasStrictHierarchicalCohesion(
            gridWidth, gridHeight, candidate.placements)) {
        return std::nullopt;
    }

    std::vector<std::uint32_t> actualOccupancy(
        static_cast<std::size_t>(gridWidth) * gridHeight);
    for (const auto& placement : candidate.placements) {
        const GridItem item{
            .guid = placement.guid,
            .width = placement.width,
            .height = placement.height,
            .current = placement.from,
            .anchor = placement.anchor,
            .groupOrder = placement.groupOrder,
            .subgroupOrder = placement.subgroupOrder,
            .itemOrder = placement.itemOrder,
        };
        if (!Fits(
                item,
                placement.to,
                gridWidth,
                gridHeight,
                actualOccupancy)) {
            return std::nullopt;
        }
        Occupy(item, placement.to, gridWidth, actualOccupancy);
    }
    for (std::uint8_t y = 0; y < gridHeight; ++y) {
        for (std::uint8_t x = firstReservedX; x < gridWidth; ++x) {
            actualOccupancy[static_cast<std::size_t>(y) * gridWidth + x] =
                std::numeric_limits<std::uint32_t>::max();
        }
    }
    candidate.freeRectangle = LargestFreeRectangle(
        gridWidth, gridHeight, actualOccupancy);
    return candidate;
}

inline Plan BuildPlan(
        std::uint8_t gridWidth,
        std::uint8_t gridHeight,
        const std::vector<GridItem>& items,
        bool optimizeFreeSpace = true,
        std::uint8_t reservedRightColumns = 0) {
    Plan result{};
    if (gridWidth == 0 || gridHeight == 0
            || static_cast<std::uint32_t>(gridWidth) * gridHeight > 4096) {
        result.error = "invalid grid dimensions";
        return result;
    }
    if (reservedRightColumns >= gridWidth) {
        result.error = "reserved right columns must leave at least one movable column";
        return result;
    }

    std::unordered_set<std::uint32_t> guids;
    std::vector<std::uint32_t> currentOccupancy(
        static_cast<std::size_t>(gridWidth) * gridHeight);
    for (const auto& item : items) {
        if (item.guid == 0 || !guids.insert(item.guid).second) {
            result.error = "item GUIDs must be nonzero and unique";
            return result;
        }
        if (item.width == 0 || item.height == 0
                || !Fits(
                    item,
                    item.current,
                    gridWidth,
                    gridHeight,
                    currentOccupancy)) {
            result.error = "current item layout is invalid";
            return result;
        }
        Occupy(item, item.current, gridWidth, currentOccupancy);
        const auto firstReservedX = static_cast<std::uint8_t>(
            gridWidth - reservedRightColumns);
        const auto itemRight = static_cast<std::uint16_t>(item.current.x)
            + item.width;
        const auto intersectsReserved = reservedRightColumns != 0
            && itemRight > firstReservedX;
        const auto fullyInsideReserved = intersectsReserved
            && item.current.x >= firstReservedX;
        if (intersectsReserved
                && (!fullyInsideReserved || item.anchor != Anchor::Ignore)) {
            result.error = "an item crossing or occupying a reserved column is not fixed";
            return result;
        }
    }

    const auto groups = BuildHierarchyGroups(gridWidth, gridHeight, items);
    if (std::any_of(
            groups.begin(),
            groups.end(),
            [](const HierarchyGroup& group) {
                return group.layouts.empty();
            })) {
        result.error = "a hierarchy block has no valid local layout";
        return result;
    }

    std::optional<CandidateLayout> best;
    constexpr unsigned BlockOrderVariantCount = 3;
    constexpr unsigned LayoutPreferenceVariantCount = 3;
    for (unsigned blockOrder = 0;
         blockOrder < BlockOrderVariantCount; ++blockOrder) {
        std::optional<CandidateLayout> phaseBest;
        for (unsigned layoutPreference = 0;
             layoutPreference < LayoutPreferenceVariantCount;
             ++layoutPreference) {
            auto candidate = BuildHierarchyCandidate(
                gridWidth,
                gridHeight,
                items,
                groups,
                blockOrder,
                layoutPreference,
                reservedRightColumns);
            if (!candidate) continue;
            if (!phaseBest || BetterCandidate(
                    *candidate, *phaseBest, optimizeFreeSpace)) {
                phaseBest = std::move(candidate);
            }
            if (!optimizeFreeSpace && phaseBest) break;
        }
        if (phaseBest) {
            best = std::move(phaseBest);
            break;
        }
    }
    if (!best) {
        result.error =
            "no deterministic compact layout preserves strict hierarchy";
        return result;
    }

    result.success = true;
    result.placements = std::move(best->placements);
    result.largestFreeRectangle = best->freeRectangle;
    result.anchorPenalty = best->anchorPenalty;
    result.usedPackingFallback = best->usedPackingFallback;
    for (const auto& placement : result.placements) {
        if (placement.from != placement.to) ++result.movedItemCount;
    }
    result.changed = result.movedItemCount != 0;
    return result;
}

} // namespace ruffneckk::autosort
