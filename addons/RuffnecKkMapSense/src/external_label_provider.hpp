#pragma once

#include "external_atlas_geometry.hpp"
#include "reveal_engine.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace D2RL {
struct PluginContext;
}

namespace RuffnecKk::MapSense {

struct ExternalAtlasTopologyEdge final {
    std::int32_t sourceLevelId{};
    std::int32_t targetLevelId{};
    // 0 is a warp/entrance into another map space. 1 is a continuous outdoor
    // seam whose two levels share one automap coordinate space.
    std::int32_t kind{};
};

inline constexpr std::size_t ExternalAtlasLevelCapacity = 512U;
inline constexpr std::size_t ExternalAtlasEdgeCapacity = 16'384U;

// Returns only the levels that share the current level's continuous automap
// space. Warp targets deliberately remain outside this set and are presented
// at their source entrance instead of importing their unrelated coordinates.
[[nodiscard]] inline auto CollectExternalAtlasVisibleLevels(
    std::int32_t currentLevelId,
    const ExternalAtlasTopologyEdge* edges,
    std::size_t edgeCount,
    std::vector<std::int32_t>& output) -> bool {
    output.clear();
    if (currentLevelId <= 0
        || (edgeCount != 0U && edges == nullptr)
        || edgeCount > ExternalAtlasEdgeCapacity) {
        return false;
    }
    output.push_back(currentLevelId);
    for (;;) {
        const auto before = output.size();
        for (std::size_t index = 0U; index < edgeCount; ++index) {
            const auto& edge = edges[index];
            if (edge.kind != 1 || edge.sourceLevelId <= 0
                || edge.targetLevelId <= 0) {
                continue;
            }
            const bool sourceVisible = std::find(
                output.begin(), output.end(), edge.sourceLevelId)
                != output.end();
            const bool targetVisible = std::find(
                output.begin(), output.end(), edge.targetLevelId)
                != output.end();
            if (sourceVisible && !targetVisible) {
                output.push_back(edge.targetLevelId);
            } else if (targetVisible && !sourceVisible) {
                output.push_back(edge.sourceLevelId);
            }
            if (output.size() > ExternalAtlasLevelCapacity) {
                output.clear();
                return false;
            }
        }
        if (output.size() == before) break;
    }
    std::sort(output.begin(), output.end());
    output.erase(
        std::unique(output.begin(), output.end()),
        output.end());
    return true;
}

struct ExternalLabelProviderCounters final {
    std::uint64_t requests{};
    std::uint64_t processesStarted{};
    std::uint64_t atlasesPublished{};
    std::uint64_t staleResponses{};
    std::uint64_t failedResponses{};
    std::uint64_t timeouts{};
    std::uint64_t geometryCacheHits{};
    std::uint64_t geometryAtlasesGenerated{};
    std::uint64_t geometryFailures{};
    std::uint64_t geometrySnapshotsPublished{};
    std::uint64_t publishedGeometryCells{};
};

// Immutable seed-scoped geometry for the current continuous automap space.
// The worker owns generation and validation; Present only holds this shared
// value while drawing and never touches the helper or D2R's DRLG structures.
struct ExternalAtlasGeometrySnapshot final {
    std::uint64_t sessionGeneration{};
    std::uint32_t seed{};
    std::uint8_t difficulty{};
    std::uint8_t act{};
    std::int32_t currentLevelId{};
    std::vector<std::int32_t> visibleLevelIds;
    std::shared_ptr<const ExternalAtlasGeometry> geometry;
};

using ExternalLabelAtlasResultCallback = void(*)(
    std::uint64_t sessionGeneration,
    std::uint8_t difficulty,
    std::int32_t act,
    std::int32_t currentLevelId,
    bool published,
    void* userData) noexcept;

// Starts one private worker. The helper is launched hidden and only from that
// worker; D2R's gameplay, UI and render threads never wait for map generation.
[[nodiscard]] auto InitializeExternalLabelProvider(
    const D2RL::PluginContext* context,
    bool diagnostics,
    ExternalLabelAtlasResultCallback resultCallback,
    void* resultUserData) noexcept -> bool;
void ShutdownExternalLabelProvider() noexcept;

// Invalidates pending work and its cache. A completed response can publish
// only when both this generation and the most recent request serial still
// match.
void ResetExternalLabelProviderSession(
    std::uint64_t sessionGeneration) noexcept;

// Copies only the current Level's already-generated room rectangles, then
// queues a seed-scoped act atlas. No ActiveRoom, collision, unit or monster is
// created by this operation.
[[nodiscard]] auto RequestExternalLabelAtlas(
    std::uint64_t sessionGeneration,
    const ClientLevelView& current) noexcept -> bool;

[[nodiscard]] auto IsExternalLabelProviderActive() noexcept -> bool;
[[nodiscard]] auto AcquireExternalAtlasGeometrySnapshot() noexcept
    -> std::shared_ptr<const ExternalAtlasGeometrySnapshot>;
[[nodiscard]] auto WantsExternalAtlasGeometryFrame() noexcept -> bool;
[[nodiscard]] auto HasPersistedExternalRevealMapIntent(
    std::uint32_t seed,
    std::uint8_t difficulty) noexcept -> bool;
[[nodiscard]] auto SetPersistedExternalRevealMapIntent(
    std::uint32_t seed,
    std::uint8_t difficulty,
    bool enabled) noexcept -> bool;
[[nodiscard]] auto GetExternalLabelProviderCounters() noexcept
    -> ExternalLabelProviderCounters;

} // namespace RuffnecKk::MapSense
