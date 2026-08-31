#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace D2RL {
struct PluginContext;
}

namespace RuffnecKk::MapSense {

inline constexpr std::uint32_t NativeMissileUnitType = 3U;
inline constexpr std::size_t NativeClientUnitHashBucketCount = 128U;
inline constexpr std::size_t NativeClientUnitHashTypeStride =
    NativeClientUnitHashBucketCount * sizeof(void*);
inline constexpr std::size_t MaximumNativeAutomapMissiles = 32'768U;
inline constexpr std::size_t MaximumNativeMissilesPerBucket = 8'192U;
inline constexpr std::uint64_t NativeAutomapMissileLifetimeMilliseconds = 250U;

namespace Detail {

[[nodiscard]] constexpr auto NativeClientUnitHashTableOffsetForType(
        std::uint32_t unitType) noexcept -> std::size_t {
    return static_cast<std::size_t>(unitType)
        * NativeClientUnitHashTypeStride;
}

[[nodiscard]] constexpr auto MayVisitNativeMissileUnit(
        std::size_t totalUnits,
        std::size_t bucketUnits) noexcept -> bool {
    return totalUnits < MaximumNativeAutomapMissiles
        && bucketUnits < MaximumNativeMissilesPerBucket;
}

[[nodiscard]] constexpr auto IsNativeAutomapMissileSnapshotFresh(
        std::uint64_t observedTick,
        std::uint64_t currentTick,
        std::uint64_t snapshotEpoch,
        std::uint64_t currentEpoch) noexcept -> bool {
    return snapshotEpoch == currentEpoch
        && observedTick != 0U
        && currentTick >= observedTick
        && currentTick - observedTick
            <= NativeAutomapMissileLifetimeMilliseconds;
}

} // namespace Detail

// Transient values copied by the existing local-player automap hook. The
// borrowed AutomapContext is valid only for the synchronous Observe call and
// is never retained by the missile collector.
struct NativeAutomapMissilePlayerPass final {
    void* automapContext{};
    std::uint16_t playerSubtileX{};
    std::uint16_t playerSubtileY{};
    std::int32_t nativeWidth{};
    std::int32_t nativeHeight{};
    std::int32_t clipLeft{};
    std::int32_t clipTop{};
    std::int32_t clipWidth{};
    std::int32_t clipHeight{};
    std::uint64_t observedTick{};
};

// Renderer-facing value copy. Class ids intentionally remain raw active-table
// ordinals; the later data-policy layer will map them through the current mod
// instead of compiling a vanilla or BKVince-specific enum into this collector.
struct NativeAutomapMissileSnapshot final {
    std::uint32_t unitId{};
    std::int32_t classId{-1};
    std::int32_t x{};
    std::int32_t y{};
    std::uint16_t worldSubtileX{};
    std::uint16_t worldSubtileY{};
    std::uint16_t playerSubtileX{};
    std::uint16_t playerSubtileY{};
    std::int32_t nativeWidth{};
    std::int32_t nativeHeight{};
    std::uint64_t observedTick{};
    std::uint64_t epoch{};
    std::uint64_t sequence{};
};

struct NativeAutomapMissileCounters final {
    std::uint64_t automapPulses{};
    std::uint64_t clientTableScans{};
    std::uint64_t bucketsVisited{};
    std::uint64_t traversalLimits{};
    std::uint64_t cyclesRejected{};
    std::uint64_t unitsObserved{};
    std::uint64_t unitTypeRejected{};
    std::uint64_t invalidUnitIds{};
    std::uint64_t invalidClassIds{};
    std::uint64_t pathRejected{};
    std::uint64_t projectionRejected{};
    std::uint64_t nativeClipRejected{};
    std::uint64_t framesPublished{};
    std::uint64_t missilesPublished{};
    std::uint64_t writerContentionDrops{};
    std::uint64_t readerContentionDrops{};
    std::uint64_t accessFaults{};
    std::uint64_t maximumScanMicroseconds{};
    std::uint64_t totalScanMicroseconds{};
    std::uint64_t scanTimingSamples{};
    std::uint64_t currentPublished{};
};

auto InitializeNativeAutomapMissile(
    const D2RL::PluginContext* context) noexcept -> bool;
void ShutdownNativeAutomapMissile() noexcept;
void ResetNativeAutomapMissile() noexcept;
void InvalidateNativeAutomapMissileFrame() noexcept;
void SetNativeAutomapMissileEnabled(bool enabled) noexcept;

// Called only by the existing AUTOMAP_RenderUnit owner during the local-player
// pass. It performs one bounded client type-3 table scan and publishes values.
void ObserveNativeAutomapMissilePlayerPass(
    const NativeAutomapMissilePlayerPass& pass) noexcept;

auto AcquireNativeAutomapMissiles(
    std::vector<NativeAutomapMissileSnapshot>& snapshots) noexcept
    -> std::size_t;

auto WantsNativeAutomapMissileFrame() noexcept -> bool;

auto GetNativeAutomapMissileCounters() noexcept
    -> NativeAutomapMissileCounters;

} // namespace RuffnecKk::MapSense
