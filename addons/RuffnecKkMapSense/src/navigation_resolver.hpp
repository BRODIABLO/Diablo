#pragma once

#include "mapsense_config.hpp"
#include "navigation_policy.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>

namespace D2RL {
struct PluginContext;
}

namespace RuffnecKk::MapSense {

namespace Detail {

struct NavigationWaypointPresetCandidate final {
    std::int32_t nativeTileX{};
    std::int32_t nativeTileY{};
    std::int32_t subtileX{};
    std::int32_t subtileY{};
    std::int32_t classId{};
};

struct NavigationRoomTileLinkCandidate final {
    std::int32_t sourcePresetId{};
    std::int32_t targetLevelId{UnknownNavigationLevelId};
};

struct NavigationRoomRectangle final {
    std::int32_t levelId{UnknownNavigationLevelId};
    std::int32_t tileX{};
    std::int32_t tileY{};
    std::int32_t width{};
    std::int32_t height{};
};

struct NavigationCollisionGridView final {
    std::int32_t originX{};
    std::int32_t originY{};
    std::int32_t width{};
    std::int32_t height{};
    std::span<const std::uint16_t> cells{};
};

struct NavigationOutdoorOpening final {
    std::int32_t subtileX{};
    std::int32_t subtileY{};
    std::int32_t spanSubtiles{};
};

enum class NavigationExitEvidence : std::uint8_t {
    OutdoorCollision = 2U,
    RoomTile = 3U,
    QuestPreset = 4U,
    BossPreset = 5U,
    RuntimeObject = 6U,
};

struct NavigationExitSelection final {
    NavigationExitCandidate candidate{};
    NavigationExitEvidence evidence{NavigationExitEvidence::OutdoorCollision};
    std::int32_t spanSubtiles{};
};

// Native waypoint output is quantized to game tiles. The exact endpoint must
// come from the waypoint PresetUnit that maps back to those native tile
// coordinates, never from the quantized output itself.
[[nodiscard]] inline auto SelectExactWaypointPreset(
        std::int32_t nativeTileX,
        std::int32_t nativeTileY,
        std::span<const NavigationWaypointPresetCandidate> candidates)
        noexcept -> std::optional<NavigationWaypointPresetCandidate> {
    for (const auto& candidate : candidates) {
        if (candidate.nativeTileX != nativeTileX
            || candidate.nativeTileY != nativeTileY
            || candidate.subtileX < 0
            || candidate.subtileY < 0) {
            continue;
        }
        // 0x3DAD90 stops on the first matching waypoint preset in the same
        // chain. Preserve that order if several subtiles quantize to one tile.
        return candidate;
    }
    return std::nullopt;
}

// A type-5 PresetUnit stores the source-side LvlWarp id. RoomTile+0 points to
// the destination DrlgRoom even when D2R has not created the reciprocal tile
// yet, so exact exit selection must not depend on that reciprocal state.
[[nodiscard]] inline auto SelectRoomTileTargetLevel(
        std::int32_t sourcePresetId,
        std::span<const NavigationRoomTileLinkCandidate> links) noexcept
        -> std::optional<std::int32_t> {
    if (sourcePresetId < 0) return std::nullopt;
    for (const auto& link : links) {
        if (link.sourcePresetId == sourcePresetId
            && link.targetLevelId > 0) {
            return link.targetLevelId;
        }
    }
    return std::nullopt;
}

[[nodiscard]] inline auto IsDirectVisibleTarget(
        std::int32_t currentLevelId,
        std::int32_t targetLevelId,
        std::span<const std::int32_t> visibleTargetLevelIds) noexcept -> bool {
    if (currentLevelId <= 0 || targetLevelId <= 0
        || targetLevelId == currentLevelId) {
        return false;
    }
    for (const auto visibleTargetLevelId : visibleTargetLevelIds) {
        if (visibleTargetLevelId == targetLevelId) return true;
    }
    return false;
}

// Finds the same walkable boundary opening that both collision maps expose to
// gameplay. The edge and adjacent inward cells must be free on each side, the
// shared opening must span at least three subtiles, and the widest exact
// intersection wins with a stable low-coordinate tie-breaker.
[[nodiscard]] inline auto FindOutdoorCollisionOpening(
        const NavigationRoomRectangle& source,
        const NavigationRoomRectangle& neighbour,
        const NavigationCollisionGridView& sourceGrid,
        const NavigationCollisionGridView& neighbourGrid,
        NavigationOutdoorOpening& output) noexcept -> bool {
    if (source.levelId <= 0 || neighbour.levelId <= 0
        || source.levelId == neighbour.levelId
        || source.tileX < 0 || source.tileY < 0
        || neighbour.tileX < 0 || neighbour.tileY < 0
        || source.width <= 0 || source.height <= 0
        || neighbour.width <= 0 || neighbour.height <= 0) {
        return false;
    }

    const auto sourceLeft = static_cast<std::int64_t>(source.tileX);
    const auto sourceTop = static_cast<std::int64_t>(source.tileY);
    const auto sourceRight = sourceLeft + source.width;
    const auto sourceBottom = sourceTop + source.height;
    const auto neighbourLeft = static_cast<std::int64_t>(neighbour.tileX);
    const auto neighbourTop = static_cast<std::int64_t>(neighbour.tileY);
    const auto neighbourRight = neighbourLeft + neighbour.width;
    const auto neighbourBottom = neighbourTop + neighbour.height;
    constexpr auto scale = std::int64_t{5};
    constexpr auto maximum = static_cast<std::int64_t>(
        (std::numeric_limits<std::int32_t>::max)());
    if (sourceRight > maximum || sourceBottom > maximum
        || neighbourRight > maximum || neighbourBottom > maximum
        || sourceRight * scale > maximum
        || sourceBottom * scale > maximum
        || neighbourRight * scale > maximum
        || neighbourBottom * scale > maximum) {
        return false;
    }
    const auto validGrid = [scale](
            const NavigationRoomRectangle& rectangle,
            const NavigationCollisionGridView& grid) noexcept {
        if (grid.originX < 0 || grid.originY < 0
            || grid.width < 2 || grid.height < 2) {
            return false;
        }
        const auto expectedOriginX =
            static_cast<std::int64_t>(rectangle.tileX) * scale;
        const auto expectedOriginY =
            static_cast<std::int64_t>(rectangle.tileY) * scale;
        const auto expectedWidth =
            static_cast<std::int64_t>(rectangle.width) * scale;
        const auto expectedHeight =
            static_cast<std::int64_t>(rectangle.height) * scale;
        if (grid.originX != expectedOriginX
            || grid.originY != expectedOriginY
            || grid.width != expectedWidth
            || grid.height != expectedHeight) {
            return false;
        }
        const auto requiredCellCount =
            static_cast<std::uint64_t>(grid.width)
            * static_cast<std::uint64_t>(grid.height);
        return requiredCellCount <= grid.cells.size();
    };
    if (!validGrid(source, sourceGrid)
        || !validGrid(neighbour, neighbourGrid)) {
        return false;
    }

    enum class Side : std::uint8_t { Left, Right, Top, Bottom };
    Side side{};
    std::int64_t overlapStart{};
    std::int64_t overlapEnd{};
    const auto verticalStart = sourceTop > neighbourTop
        ? sourceTop : neighbourTop;
    const auto verticalEnd = sourceBottom < neighbourBottom
        ? sourceBottom : neighbourBottom;
    const auto horizontalStart = sourceLeft > neighbourLeft
        ? sourceLeft : neighbourLeft;
    const auto horizontalEnd = sourceRight < neighbourRight
        ? sourceRight : neighbourRight;
    if (neighbourRight == sourceLeft && verticalEnd > verticalStart) {
        side = Side::Left;
        overlapStart = verticalStart * scale;
        overlapEnd = verticalEnd * scale;
    } else if (neighbourLeft == sourceRight
            && verticalEnd > verticalStart) {
        side = Side::Right;
        overlapStart = verticalStart * scale;
        overlapEnd = verticalEnd * scale;
    } else if (neighbourBottom == sourceTop
            && horizontalEnd > horizontalStart) {
        side = Side::Top;
        overlapStart = horizontalStart * scale;
        overlapEnd = horizontalEnd * scale;
    } else if (neighbourTop == sourceBottom
            && horizontalEnd > horizontalStart) {
        side = Side::Bottom;
        overlapStart = horizontalStart * scale;
        overlapEnd = horizontalEnd * scale;
    } else {
        return false;
    }

    if (overlapStart < 0 || overlapEnd <= overlapStart
        || overlapEnd > maximum) {
        return false;
    }

    const auto cell = [](const NavigationCollisionGridView& grid,
            std::int32_t x,
            std::int32_t y) noexcept {
        return grid.cells[
            static_cast<std::size_t>(y)
                * static_cast<std::size_t>(grid.width)
            + static_cast<std::size_t>(x)];
    };
    const auto isOpen = [&cell, &sourceGrid, &neighbourGrid, side](
            std::int32_t position) noexcept {
        const auto sourceVertical = position - sourceGrid.originY;
        const auto neighbourVertical = position - neighbourGrid.originY;
        const auto sourceHorizontal = position - sourceGrid.originX;
        const auto neighbourHorizontal = position - neighbourGrid.originX;
        switch (side) {
            case Side::Left:
                return (cell(sourceGrid, 0, sourceVertical) & 1U) == 0U
                    && (cell(sourceGrid, 1, sourceVertical) & 1U) == 0U
                    && (cell(neighbourGrid,
                            neighbourGrid.width - 1,
                            neighbourVertical) & 1U) == 0U
                    && (cell(neighbourGrid,
                            neighbourGrid.width - 2,
                            neighbourVertical) & 1U) == 0U;
            case Side::Right:
                return (cell(sourceGrid,
                            sourceGrid.width - 1,
                            sourceVertical) & 1U) == 0U
                    && (cell(sourceGrid,
                            sourceGrid.width - 2,
                            sourceVertical) & 1U) == 0U
                    && (cell(neighbourGrid, 0, neighbourVertical) & 1U) == 0U
                    && (cell(neighbourGrid, 1, neighbourVertical) & 1U) == 0U;
            case Side::Top:
                return (cell(sourceGrid, sourceHorizontal, 0) & 1U) == 0U
                    && (cell(sourceGrid, sourceHorizontal, 1) & 1U) == 0U
                    && (cell(neighbourGrid,
                            neighbourHorizontal,
                            neighbourGrid.height - 1) & 1U) == 0U
                    && (cell(neighbourGrid,
                            neighbourHorizontal,
                            neighbourGrid.height - 2) & 1U) == 0U;
            case Side::Bottom:
                return (cell(sourceGrid,
                            sourceHorizontal,
                            sourceGrid.height - 1) & 1U) == 0U
                    && (cell(sourceGrid,
                            sourceHorizontal,
                            sourceGrid.height - 2) & 1U) == 0U
                    && (cell(neighbourGrid, neighbourHorizontal, 0) & 1U)
                        == 0U
                    && (cell(neighbourGrid, neighbourHorizontal, 1) & 1U)
                        == 0U;
        }
        return false;
    };

    std::int32_t bestStart{-1};
    std::int32_t bestLength{};
    std::int32_t currentStart{-1};
    const auto considerRun = [&bestStart, &bestLength](
            std::int32_t start,
            std::int32_t end) noexcept {
        if (start < 0) return;
        const auto length = end - start;
        if (length >= 3 && length > bestLength) {
            bestStart = start;
            bestLength = length;
        }
    };
    for (auto position = static_cast<std::int32_t>(overlapStart);
            position < static_cast<std::int32_t>(overlapEnd);
            ++position) {
        if (isOpen(position)) {
            if (currentStart < 0) currentStart = position;
        } else {
            considerRun(currentStart, position);
            currentStart = -1;
        }
    }
    considerRun(currentStart, static_cast<std::int32_t>(overlapEnd));
    if (bestStart < 0) return false;

    const auto midpoint = bestStart + bestLength / 2;
    auto subtileX = neighbourGrid.originX;
    auto subtileY = neighbourGrid.originY;
    switch (side) {
        case Side::Left:
            subtileX += neighbourGrid.width - 1;
            subtileY += midpoint;
            subtileY -= neighbourGrid.originY;
            break;
        case Side::Right:
            subtileY = midpoint;
            break;
        case Side::Top:
            subtileX = midpoint;
            subtileY += neighbourGrid.height - 1;
            break;
        case Side::Bottom:
            subtileX = midpoint;
            break;
    }
    output = {
        .subtileX = subtileX,
        .subtileY = subtileY,
        .spanSubtiles = bestLength,
    };
    return true;
}

// One destination is published per target level. An exact active runtime
// object wins over generated boss/quest presets, which win over RoomTile and
// outdoor collision evidence; equal evidence uses a stable tie-breaker.
[[nodiscard]] inline auto UpsertExitSelection(
        std::int32_t currentLevelId,
        std::span<NavigationExitSelection> selections,
        std::size_t& count,
        NavigationExitSelection incoming) noexcept -> bool {
    const auto& candidate = incoming.candidate;
    if (currentLevelId <= 0
        || candidate.targetLevelId <= 0
        || candidate.targetLevelId == currentLevelId
        || candidate.subtileX < 0 || candidate.subtileY < 0) {
        return false;
    }
    for (std::size_t index = 0U; index < count; ++index) {
        auto& existing = selections[index];
        if (existing.candidate.targetLevelId != candidate.targetLevelId) {
            continue;
        }
        const auto incomingEvidence = static_cast<std::uint8_t>(
            incoming.evidence);
        const auto existingEvidence = static_cast<std::uint8_t>(
            existing.evidence);
        const auto replace = incomingEvidence > existingEvidence
            || (incomingEvidence == existingEvidence
                && (incoming.spanSubtiles > existing.spanSubtiles
                    || (incoming.spanSubtiles == existing.spanSubtiles
                        && (candidate.subtileX
                                < existing.candidate.subtileX
                            || (candidate.subtileX
                                    == existing.candidate.subtileX
                                && candidate.subtileY
                                    < existing.candidate.subtileY)))));
        if (replace) existing = incoming;
        return true;
    }
    if (count >= selections.size()) return false;
    selections[count++] = incoming;
    return true;
}

} // namespace Detail

enum class NavigationRefreshResult : std::uint8_t {
    Complete,
    PartialRetryable,
    Failed,
};

struct NavigationResolverCounters final {
    std::uint64_t refreshes{};
    std::uint64_t rooms{};
    std::uint64_t presets{};
    std::uint64_t exits{};
    std::uint64_t waypoints{};
    std::uint64_t published{};
    std::uint64_t unresolvedNames{};
    std::uint64_t failures{};
    std::uint64_t traversalLimits{};
    std::uint64_t partialRefreshes{};
    std::uint64_t visibilitySlots{};
    std::uint64_t visibilityPairs{};
    std::uint64_t pendingVisibilityTargets{};
    std::int32_t lastLevelId{UnknownNavigationLevelId};
    std::uint32_t lastDestinationCount{};
    std::int32_t lastWaypointX{};
    std::int32_t lastWaypointY{};
    std::int32_t lastProgressionX{};
    std::int32_t lastProgressionY{};
};

[[nodiscard]] auto InitializeNavigationResolver(
    const D2RL::PluginContext* context,
    bool diagnostics) noexcept -> bool;
void ShutdownNavigationResolver() noexcept;

// Runs only from D2RLoader's gameplay/UI thread. UnknownNavigationLevelId lets
// the resolver trust the local player's proven current Level; a concrete id is
// cross-checked against it before any destination is published.
[[nodiscard]] auto RefreshNavigationDestinations(
    std::uint64_t sessionGeneration,
    std::int32_t expectedLevelId,
    std::span<const CustomLevelTarget> customTargets) noexcept
    -> NavigationRefreshResult;

[[nodiscard]] auto IsNavigationResolverActive() noexcept -> bool;
[[nodiscard]] auto GetNavigationResolverCounters() noexcept
    -> NavigationResolverCounters;

} // namespace RuffnecKk::MapSense
