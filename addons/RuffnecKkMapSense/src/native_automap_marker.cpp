#include "native_automap_marker.hpp"

#include "mapsense_config.hpp"
#include "navigation_engine.hpp"

#include <D2RLPlugin/api.h>

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace RuffnecKk::MapSense {
namespace {

constexpr std::uintptr_t GetLocalDataContextRva = 0x08B2D0;
constexpr std::uintptr_t GetLocalPlayerRva = 0x09A480;
constexpr std::uintptr_t GetNativeHeightRva = 0x07F4A0;
constexpr std::uintptr_t GetNativeWidthRva = 0x07F510;
constexpr std::uintptr_t ProjectClientToAutomapRva = 0x0D4910;
constexpr std::uintptr_t RenderAutomapUnitRva = 0x0D76E0;
constexpr std::uintptr_t GetUnitStatRva = 0x2F5020;
constexpr std::uintptr_t GetUnitAlignmentRva = 0x2F4190;
constexpr std::uintptr_t GetUnitIdRva = 0x34A330;
constexpr std::uintptr_t GetUnitClassIdRva = 0x349860;
constexpr std::uintptr_t GetUnitDataContextRva = 0x34A0E0;
constexpr std::uintptr_t GetUnitModeRva = 0x34AB60;
constexpr std::uintptr_t GetDynamicPathRva = 0x34AE80;
constexpr std::uintptr_t GetUnitClientXRva = 0x34AF60;
constexpr std::uintptr_t GetUnitClientYRva = 0x34AFB0;
constexpr std::uintptr_t PathGetXRva = 0x341A20;
constexpr std::uintptr_t PathGetYRva = 0x341A30;
constexpr std::uintptr_t GetMonStatsRecordRva = 0x0976E0;
constexpr std::uintptr_t MonsterRankWitnessRva = 0x51F280;
constexpr std::uintptr_t GetUnitRoomRva = 0x34B440;
constexpr std::uintptr_t UnitRoomLayoutWitnessRva = 0x34B461;
constexpr std::uintptr_t PathGetRoomRva = 0x341C30;
constexpr std::uintptr_t IsRoomInTownRva = 0x2F0750;
constexpr std::uintptr_t GetDrlgRoomLevelIdRva = 0x360FC0;
constexpr std::uintptr_t ClientUnitHashTableRva = 0x2A23910;
constexpr std::uintptr_t ClientUnitHashTableWitnessRva = 0x09A5D0;
constexpr std::uintptr_t ClientUnitHashLookupWitnessRva = 0x09F270;
constexpr std::size_t UnitTypeOffset = 0x00;
constexpr std::size_t UnitTypeDataOffset = 0x10;
constexpr std::size_t UnitFlagsOffset = 0x124;
constexpr std::size_t UnitHashNextOffset = 0x158;
constexpr std::size_t ActiveRoomDrlgRoomOffset = 0x18;
constexpr std::size_t MonsterRankFlagsOffset = 0x1A;
constexpr std::size_t MonStatsFlagsOffset = 0x3C;
constexpr std::size_t AutomapClipLeftOffset = 0x18;
constexpr std::size_t AutomapClipTopOffset = 0x1C;
constexpr std::size_t AutomapClipWidthOffset = 0x20;
constexpr std::size_t AutomapClipHeightOffset = 0x24;

constexpr std::uint32_t UnitMonster = 1;
constexpr std::int32_t MaximumSupportedLevelId = 65'535;
constexpr std::uint32_t MonsterModeDeath = 0;
constexpr std::uint32_t MonsterModeDead = 12;
constexpr std::int32_t PhysicalResistanceStatId = 36;
constexpr std::int32_t MagicResistanceStatId = 37;
constexpr std::int32_t FireResistanceStatId = 39;
constexpr std::int32_t LightningResistanceStatId = 41;
constexpr std::int32_t ColdResistanceStatId = 43;
constexpr std::int32_t PoisonResistanceStatId = 45;
constexpr std::uint64_t MarkerLifetimeMilliseconds = 250;
constexpr std::uint64_t MonsterTableScanIntervalMilliseconds = 50;
constexpr std::size_t UnitHashBucketCount = 128;
constexpr std::size_t UnitHashTypeStride = UnitHashBucketCount
    * sizeof(void*);
constexpr std::size_t MaximumUnitsPerMonsterTableScan = 32'768;
constexpr std::size_t MaximumUnitsPerMonsterBucket = 8'192;
constexpr std::size_t InitialMarkerReserve = 2'048;
constexpr std::uint64_t ObservationsPerChunk = 4'096;
constexpr std::uint64_t InitialObservationChunkCount = 16;
constexpr std::uint32_t ObservationBufferCount = 2;

struct NativePoint final {
    std::int32_t x{};
    std::int32_t y{};
};

struct Candidate final {
    std::uint32_t unitId{};
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t nativeWidth{};
    std::int32_t nativeHeight{};
    MonsterRank rank{MonsterRank::Normal};
    std::uint8_t immunityMask{};
    std::uint64_t distanceSquared{};
    std::uint64_t observedTick{};
    std::uint64_t epoch{};
};

struct MonsterScanContext final {
    void* automapContext{};
    std::uint16_t playerSubtileX{};
    std::uint16_t playerSubtileY{};
    std::int32_t nativeWidth{};
    std::int32_t nativeHeight{};
    std::int32_t clipLeft{};
    std::int32_t clipTop{};
    std::int32_t clipWidth{};
    std::int32_t clipHeight{};
    std::uint64_t currentTick{};
    std::uint64_t epoch{};
};

struct ObservationChunk final {
    std::array<Candidate, ObservationsPerChunk> observations{};
    std::atomic<ObservationChunk*> next{};
};

struct ObservationBuffer final {
    ObservationChunk* first{};
    std::atomic<std::uint64_t> count{};
    std::atomic<std::uint32_t> writers{};
    std::atomic<std::uint64_t> epoch{};

    ObservationBuffer() = default;
    ObservationBuffer(const ObservationBuffer&) = delete;
    auto operator=(const ObservationBuffer&) -> ObservationBuffer& = delete;

    ~ObservationBuffer() {
        auto* chunk = first;
        while (chunk != nullptr) {
            auto* const next = chunk->next.load(std::memory_order_relaxed);
            delete chunk;
            chunk = next;
        }
    }
};

using GetLocalDataContextFn = std::int32_t(__fastcall*)() noexcept;
using GetLocalPlayerFn = void*(__fastcall*)(std::int32_t) noexcept;
using GetNativeDimensionFn = std::int32_t(__fastcall*)() noexcept;
using GetUnitStatFn = std::int32_t(__fastcall*)(
    void* unit,
    std::int32_t statId,
    std::uint16_t layer) noexcept;
using GetUnitValueFn = std::uint32_t(__fastcall*)(void*) noexcept;
using GetUnitSignedValueFn = std::int32_t(__fastcall*)(void*) noexcept;
using GetUnitDataContextFn = std::uint8_t(__fastcall*)(void*) noexcept;
using GetUnitCoordinateFn = std::int32_t(__fastcall*)(void*) noexcept;
using GetNativePointerFn = void*(__fastcall*)(void*) noexcept;
using GetLevelIdFn = std::int32_t(__fastcall*)(void*) noexcept;
using IsRoomInTownFn = std::int32_t(__fastcall*)(void*) noexcept;
using GetMonStatsRecordFn = const std::uint8_t*(__fastcall*)(
    std::uint8_t dataContext,
    std::int32_t classId) noexcept;
using ProjectClientToAutomapFn = NativePoint*(__fastcall*)(
    void* automapContext,
    NativePoint* output,
    std::uint64_t packedClientCoordinates) noexcept;
using RenderAutomapUnitFn = void(__fastcall*)(
    void* unit,
    void* automapContext) noexcept;

std::uint8_t* Base{};
GetLocalDataContextFn GetLocalDataContext{};
GetLocalPlayerFn GetLocalPlayer{};
GetNativeDimensionFn GetNativeHeight{};
GetNativeDimensionFn GetNativeWidth{};
GetUnitStatFn GetUnitStat{};
GetUnitSignedValueFn GetUnitAlignment{};
GetUnitValueFn GetUnitId{};
GetUnitSignedValueFn GetUnitClassId{};
GetUnitDataContextFn GetUnitDataContext{};
GetUnitValueFn GetUnitMode{};
GetNativePointerFn GetDynamicPath{};
GetUnitCoordinateFn GetUnitClientX{};
GetUnitCoordinateFn GetUnitClientY{};
GetUnitCoordinateFn PathGetX{};
GetUnitCoordinateFn PathGetY{};
GetNativePointerFn GetUnitRoom{};
GetLevelIdFn GetDrlgRoomLevelId{};
IsRoomInTownFn IsRoomInTown{};
GetMonStatsRecordFn GetMonStatsRecord{};
ProjectClientToAutomapFn ProjectClientToAutomap{};
RenderAutomapUnitFn OriginalRenderAutomapUnit{};
const D2RL::PluginContext* DiagnosticContext{};

std::atomic_bool Active{};
std::atomic_bool CollectionEnabled{};
std::atomic_bool ImmunityCollectionEnabled{};
std::atomic<std::uint64_t> Epoch{};
std::atomic<std::uint64_t> LastAutomapPulseTick{};
std::atomic<std::uint64_t> LastMonsterTableScanTick{};
std::atomic<std::int32_t> MarkerRadius{DefaultNativeAutomapMarkerRadius};
std::atomic<std::uint64_t> PublishedSequence{};
std::atomic<std::uint64_t> TrackedMarkerCount{};
Detail::NavigationProjectionDiagnosticCache NavigationProjectionDiagnostics;
std::atomic_flag NavigationProjectionDiagnosticsLock = ATOMIC_FLAG_INIT;
std::array<ObservationBuffer, ObservationBufferCount> ObservationBuffers{};
std::atomic<std::uint64_t> ActiveObservationState{};
std::int32_t PendingObservationBuffer{-1};
std::uint64_t RendererEpoch{};
std::uint64_t LastPurgeTick{};
std::unordered_map<std::uint32_t, Candidate> MarkerCache;

std::atomic<std::uint64_t> AutomapPulses{};
std::atomic<std::uint64_t> MonsterTableScans{};
std::atomic<std::uint64_t> MonsterBucketsVisited{};
std::atomic<std::uint64_t> MonsterTraversalLimits{};
std::atomic<std::uint64_t> UnitsObserved{};
std::atomic<std::uint64_t> MonstersObserved{};
std::atomic<std::uint64_t> ModeRejected{};
std::atomic<std::uint64_t> UnitFlagRejected{};
std::atomic<std::uint64_t> ClassRejected{};
std::atomic<std::uint64_t> AlignmentRejected{};
std::atomic<std::uint64_t> MetadataFaults{};
std::atomic<std::uint64_t> HostilesObserved{};
std::atomic<std::uint64_t> HostilesThrough80{};
std::atomic<std::uint64_t> HostilesFrom81Through140{};
std::atomic<std::uint64_t> HostilesFrom141Through220{};
std::atomic<std::uint64_t> HostilesBeyond220{};
std::atomic<std::uint64_t> RadiusRejected{};
std::atomic<std::uint64_t> WithinRadius{};
std::atomic<std::uint64_t> ProjectionRejected{};
std::atomic<std::uint64_t> NativeClipRejected{};
std::atomic<std::uint64_t> CandidatesAccepted{};
std::atomic<std::uint64_t> MarkersInserted{};
std::atomic<std::uint64_t> MarkersRefreshed{};
std::atomic<std::uint64_t> MarkersExpired{};
std::atomic<std::uint64_t> ContentionWaits{};
std::atomic<std::uint64_t> StorageFailures{};
std::atomic<std::uint64_t> AccessFaults{};
std::atomic<std::uint64_t> MaximumHostileDistanceSquared{};
std::atomic<std::uint64_t> MaximumWithinRadiusDistanceSquared{};
std::atomic<std::uint64_t> MaximumAcceptedDistanceSquared{};
std::atomic<std::uint64_t> MaximumPublishedDistanceSquared{};
std::atomic<NativeAutomapLevelObservedCallback> LevelObservedCallback{};
std::atomic<void*> LevelObservedUserData{};

static_assert(std::is_trivially_copyable_v<NativeAutomapMarkerSnapshot>);
static_assert(std::is_standard_layout_v<NativeAutomapMarkerSnapshot>);
static_assert(sizeof(Candidate) == 48U);
static_assert(sizeof(NativeAutomapMarkerSnapshot) == 40U);
static_assert(sizeof(NativePoint) == sizeof(std::uint64_t));
static_assert(UnitHashTypeStride == 0x400U);
static_assert(
    DefaultNativeAutomapMarkerRadius == DefaultMonsterDetectionRadius);
static_assert(
    Detail::MaximumNavigationProjectionDiagnosticEntries
    >= MaximumNavigationDestinations);

template <typename Function>
auto At(std::uintptr_t rva) noexcept -> Function {
    return reinterpret_cast<Function>(Base + rva);
}

void UpdateMaximum(
        std::atomic<std::uint64_t>& destination,
        std::uint64_t candidate) noexcept {
    auto current = destination.load(std::memory_order_relaxed);
    while (current < candidate
        && !destination.compare_exchange_weak(
            current,
            candidate,
            std::memory_order_relaxed,
            std::memory_order_relaxed)) {
    }
}

auto DistanceFromSquared(std::uint64_t squared) noexcept -> std::uint32_t {
    return static_cast<std::uint32_t>(std::llround(
        std::sqrt(static_cast<long double>(squared))));
}

auto IsRecent(
        std::uint64_t tick,
        std::uint64_t currentTick) noexcept -> bool {
    return tick != 0 && currentTick >= tick
        && (currentTick - tick) <= MarkerLifetimeMilliseconds;
}

[[nodiscard]] auto IsAlignedPointer(const void* pointer) noexcept -> bool {
    return pointer != nullptr
        && (reinterpret_cast<std::uintptr_t>(pointer)
            & (alignof(void*) - 1U)) == 0U;
}

[[nodiscard]] __declspec(noinline) auto TryReadCurrentLevelId(
        void* player,
        std::int32_t& levelId,
        bool& inTown) noexcept -> bool {
    __try {
        if (!IsAlignedPointer(player)) return false;
        if (GetUnitRoom == nullptr || GetDrlgRoomLevelId == nullptr
            || IsRoomInTown == nullptr) {
            return false;
        }
        auto* const activeRoom = static_cast<std::uint8_t*>(
            GetUnitRoom(player));
        if (!IsAlignedPointer(activeRoom)) return false;
        auto* const drlgRoom = *reinterpret_cast<std::uint8_t**>(
            activeRoom + ActiveRoomDrlgRoomOffset);
        if (!IsAlignedPointer(drlgRoom)) return false;
        const auto candidate = GetDrlgRoomLevelId(drlgRoom);
        if (candidate <= 0 || candidate > MaximumSupportedLevelId) {
            return false;
        }
        levelId = candidate;
        inTown = IsRoomInTown(activeRoom) != 0;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

auto ProjectNavigationClient(
        void* borrowedAutomapContext,
        std::int32_t clientX,
        std::int32_t clientY,
        NavigationNativePoint& output) noexcept -> bool {
    __try {
        if (borrowedAutomapContext == nullptr
            || ProjectClientToAutomap == nullptr) {
            return false;
        }
        NativePoint projected{};
        const auto packedClientCoordinates =
            static_cast<std::uint64_t>(static_cast<std::uint32_t>(clientX))
            | (static_cast<std::uint64_t>(
                static_cast<std::uint32_t>(clientY)) << 32U);
        if (ProjectClientToAutomap(
                borrowedAutomapContext,
                &projected,
                packedClientCoordinates) != &projected) {
            return false;
        }
        output = {.x = projected.x, .y = projected.y};
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void MixNavigationDiagnosticValue(
        std::uint64_t& hash,
        std::uint64_t value) noexcept {
    for (std::uint32_t shift = 0U; shift < 64U; shift += 8U) {
        hash ^= static_cast<std::uint8_t>(value >> shift);
        hash *= UINT64_C(1099511628211);
    }
}

[[nodiscard]] auto ShouldLogNavigationProjectionDiagnostic(
        const NavigationProjectionDiagnostic& diagnostic,
        std::uint64_t fingerprint) noexcept -> bool {
    while (NavigationProjectionDiagnosticsLock.test_and_set(
            std::memory_order_acquire)) {
        YieldProcessor();
    }
    const auto shouldLog = NavigationProjectionDiagnostics.ShouldLog(
        diagnostic.currentLevelId,
        static_cast<std::uint8_t>(diagnostic.destination.kind),
        diagnostic.destination.destinationId,
        fingerprint);
    NavigationProjectionDiagnosticsLock.clear(std::memory_order_release);
    return shouldLog;
}

[[nodiscard]] auto NavigationKindName(
        NavigationLineKind kind) noexcept -> const char* {
    switch (kind) {
        case NavigationLineKind::Waypoint: return "waypoint";
        case NavigationLineKind::Progression: return "progression";
        case NavigationLineKind::CustomLevel: return "custom";
        case NavigationLineKind::Quest: return "quest";
    }
    return "unknown";
}

void LogNavigationProjectionDiagnostic(
        const NavigationProjectionDiagnostic& diagnostic,
        void* userData) noexcept {
    const auto* const context = static_cast<const D2RL::PluginContext*>(
        userData);
    if (context == nullptr || context != DiagnosticContext) return;

    auto fingerprint = UINT64_C(1469598103934665603);
    MixNavigationDiagnosticValue(
        fingerprint,
        static_cast<std::uint32_t>(diagnostic.currentLevelId));
    MixNavigationDiagnosticValue(
        fingerprint,
        static_cast<std::uint8_t>(diagnostic.destination.kind));
    MixNavigationDiagnosticValue(
        fingerprint,
        diagnostic.destination.destinationId);
    MixNavigationDiagnosticValue(
        fingerprint,
        static_cast<std::uint32_t>(diagnostic.destination.subtileX));
    MixNavigationDiagnosticValue(
        fingerprint,
        static_cast<std::uint32_t>(diagnostic.destination.subtileY));
    MixNavigationDiagnosticValue(
        fingerprint,
        diagnostic.playerProjected ? 1U : 0U);
    MixNavigationDiagnosticValue(
        fingerprint,
        diagnostic.destinationProjected ? 1U : 0U);
    MixNavigationDiagnosticValue(
        fingerprint,
        diagnostic.lineClipped ? 1U : 0U);
    MixNavigationDiagnosticValue(
        fingerprint,
        static_cast<std::uint32_t>(diagnostic.clipLeft));
    MixNavigationDiagnosticValue(
        fingerprint,
        static_cast<std::uint32_t>(diagnostic.clipTop));
    MixNavigationDiagnosticValue(
        fingerprint,
        static_cast<std::uint32_t>(diagnostic.clipWidth));
    MixNavigationDiagnosticValue(
        fingerprint,
        static_cast<std::uint32_t>(diagnostic.clipHeight));

    if (!ShouldLogNavigationProjectionDiagnostic(diagnostic, fingerprint)) {
        return;
    }

    char message[768]{};
    std::snprintf(
        message,
        sizeof(message),
        "MapSense diagnostic projection: level=%d kind=%s id=%llu; player-client=(%d,%d) projected=%d:(%d,%d); destination-subtile=(%d,%d) client=%d:(%d,%d) projected=%d:(%d,%d); clip=(%d,%d,%d,%d) accepted=%d line=(%d,%d)->(%d,%d).",
        diagnostic.currentLevelId,
        NavigationKindName(diagnostic.destination.kind),
        static_cast<unsigned long long>(
            diagnostic.destination.destinationId),
        diagnostic.playerClientX,
        diagnostic.playerClientY,
        diagnostic.playerProjected ? 1 : 0,
        diagnostic.projectedPlayer.x,
        diagnostic.projectedPlayer.y,
        diagnostic.destination.subtileX,
        diagnostic.destination.subtileY,
        diagnostic.destinationConverted ? 1 : 0,
        diagnostic.destinationClient.x,
        diagnostic.destinationClient.y,
        diagnostic.destinationProjected ? 1 : 0,
        diagnostic.projectedDestination.x,
        diagnostic.projectedDestination.y,
        diagnostic.clipLeft,
        diagnostic.clipTop,
        diagnostic.clipWidth,
        diagnostic.clipHeight,
        diagnostic.lineClipped ? 1 : 0,
        diagnostic.clippedStart.x,
        diagnostic.clippedStart.y,
        diagnostic.clippedEnd.x,
        diagnostic.clippedEnd.y);
    context->LogInfo(message);
}

[[nodiscard]] auto TryGetLocalPlayerPass(void* unit) noexcept -> void* {
    __try {
        if (unit == nullptr || GetLocalDataContext == nullptr
            || GetLocalPlayer == nullptr) {
            return nullptr;
        }
        const auto localDataContext = GetLocalDataContext();
        if (localDataContext < 0 || localDataContext >= 8) return nullptr;
        void* const player = GetLocalPlayer(localDataContext);
        return player == unit ? player : nullptr;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

void ObserveNavigationPlayerPass(
        void* player,
        void* automapContext) noexcept {
    __try {
        if (player == nullptr || automapContext == nullptr
            || GetNativeHeight == nullptr || GetNativeWidth == nullptr
            || GetUnitClientX == nullptr || GetUnitClientY == nullptr
            || ProjectClientToAutomap == nullptr) {
            return;
        }

        std::int32_t currentLevelId{UnknownNavigationLevelId};
        bool inTown{};
        if (!TryReadCurrentLevelId(player, currentLevelId, inTown)) return;

        const auto nativeWidth = GetNativeWidth();
        const auto nativeHeight = GetNativeHeight();
        if (nativeWidth <= 0 || nativeHeight <= 0
            || nativeWidth > 32768 || nativeHeight > 32768) {
            return;
        }
        const auto* const contextBytes = static_cast<const std::uint8_t*>(
            automapContext);
        const auto* const diagnosticContext = DiagnosticContext;
        const NavigationAutomapPass pass{
            .currentLevelId = currentLevelId,
            .inTown = inTown,
            .playerClientX = GetUnitClientX(player),
            .playerClientY = GetUnitClientY(player),
            .nativeWidth = nativeWidth,
            .nativeHeight = nativeHeight,
            .clipLeft = *reinterpret_cast<const std::int32_t*>(
                contextBytes + AutomapClipLeftOffset),
            .clipTop = *reinterpret_cast<const std::int32_t*>(
                contextBytes + AutomapClipTopOffset),
            .clipWidth = *reinterpret_cast<const std::int32_t*>(
                contextBytes + AutomapClipWidthOffset),
            .clipHeight = *reinterpret_cast<const std::int32_t*>(
                contextBytes + AutomapClipHeightOffset),
            .projectClient = ProjectNavigationClient,
            .borrowedAutomapContext = automapContext,
            .diagnostic = diagnosticContext != nullptr
                ? LogNavigationProjectionDiagnostic
                : nullptr,
            .diagnosticUserData = const_cast<D2RL::PluginContext*>(
                diagnosticContext),
        };
        const auto observation = ObserveNavigationAutomapPass(pass);
        const auto callback = LevelObservedCallback.load(
            std::memory_order_acquire);
        if (callback != nullptr) {
            callback(
                currentLevelId,
                ShouldRequestNavigationRefresh(observation, inTown),
                LevelObservedUserData.load(std::memory_order_acquire));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // Navigation is an optional observer. A bad transient native object
        // suppresses this pulse without affecting D2R or monster markers.
    }
}

void ResetPublishedMarkers(bool resetCounters) noexcept {
    Epoch.fetch_add(1U, std::memory_order_acq_rel);
    LastAutomapPulseTick.store(0U, std::memory_order_release);
    LastMonsterTableScanTick.store(0U, std::memory_order_release);
    TrackedMarkerCount.store(0U, std::memory_order_release);
    PublishedSequence.fetch_add(1U, std::memory_order_acq_rel);

    if (!resetCounters) return;
    AutomapPulses.store(0U, std::memory_order_relaxed);
    MonsterTableScans.store(0U, std::memory_order_relaxed);
    MonsterBucketsVisited.store(0U, std::memory_order_relaxed);
    MonsterTraversalLimits.store(0U, std::memory_order_relaxed);
    UnitsObserved.store(0U, std::memory_order_relaxed);
    MonstersObserved.store(0U, std::memory_order_relaxed);
    ModeRejected.store(0U, std::memory_order_relaxed);
    UnitFlagRejected.store(0U, std::memory_order_relaxed);
    ClassRejected.store(0U, std::memory_order_relaxed);
    AlignmentRejected.store(0U, std::memory_order_relaxed);
    MetadataFaults.store(0U, std::memory_order_relaxed);
    HostilesObserved.store(0U, std::memory_order_relaxed);
    HostilesThrough80.store(0U, std::memory_order_relaxed);
    HostilesFrom81Through140.store(0U, std::memory_order_relaxed);
    HostilesFrom141Through220.store(0U, std::memory_order_relaxed);
    HostilesBeyond220.store(0U, std::memory_order_relaxed);
    RadiusRejected.store(0U, std::memory_order_relaxed);
    WithinRadius.store(0U, std::memory_order_relaxed);
    ProjectionRejected.store(0U, std::memory_order_relaxed);
    NativeClipRejected.store(0U, std::memory_order_relaxed);
    CandidatesAccepted.store(0U, std::memory_order_relaxed);
    MarkersInserted.store(0U, std::memory_order_relaxed);
    MarkersRefreshed.store(0U, std::memory_order_relaxed);
    MarkersExpired.store(0U, std::memory_order_relaxed);
    ContentionWaits.store(0U, std::memory_order_relaxed);
    StorageFailures.store(0U, std::memory_order_relaxed);
    AccessFaults.store(0U, std::memory_order_relaxed);
    MaximumHostileDistanceSquared.store(0U, std::memory_order_relaxed);
    MaximumWithinRadiusDistanceSquared.store(0U, std::memory_order_relaxed);
    MaximumAcceptedDistanceSquared.store(0U, std::memory_order_relaxed);
    MaximumPublishedDistanceSquared.store(0U, std::memory_order_relaxed);
}

void BeginAutomapPulse(std::uint64_t currentTick) noexcept {
    AutomapPulses.fetch_add(1U, std::memory_order_relaxed);
    LastAutomapPulseTick.store(currentTick, std::memory_order_release);
}

auto EnsureObservationChunk(
        ObservationBuffer& buffer,
        std::uint64_t chunkIndex) noexcept -> ObservationChunk* {
    auto* chunk = buffer.first;
    if (chunk == nullptr) return nullptr;

    for (std::uint64_t index = 0U; index < chunkIndex; ++index) {
        auto* next = chunk->next.load(std::memory_order_acquire);
        if (next == nullptr) {
            auto* const allocated = new (std::nothrow) ObservationChunk{};
            if (allocated == nullptr) return nullptr;

            auto* expected = static_cast<ObservationChunk*>(nullptr);
            if (!chunk->next.compare_exchange_strong(
                    expected,
                    allocated,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                delete allocated;
                next = expected;
            } else {
                next = allocated;
            }
        }
        chunk = next;
    }
    return chunk;
}

constexpr auto ObservationBufferIndex(std::uint64_t state) noexcept
        -> std::uint32_t {
    return static_cast<std::uint32_t>(state & 1U);
}

constexpr auto NextObservationState(std::uint64_t state) noexcept
        -> std::uint64_t {
    const auto generation = (state >> 1U) + 1U;
    const auto nextIndex = ObservationBufferIndex(state) ^ 1U;
    return (generation << 1U) | nextIndex;
}

[[nodiscard]] auto BuildMonsterScanContext(
        void* player,
        void* automapContext,
        std::uint64_t currentTick,
        std::uint64_t epoch,
        MonsterScanContext& output) noexcept -> bool {
    __try {
        if (!IsAlignedPointer(player) || automapContext == nullptr
            || GetNativeHeight == nullptr || GetNativeWidth == nullptr
            || GetDynamicPath == nullptr || PathGetX == nullptr
            || PathGetY == nullptr) {
            return false;
        }

        void* const playerPath = GetDynamicPath(player);
        if (!IsAlignedPointer(playerPath)) return false;
        const auto playerSubtileX = PathGetX(playerPath);
        const auto playerSubtileY = PathGetY(playerPath);
        constexpr auto MaximumPathCoordinate = static_cast<std::int32_t>(
            (std::numeric_limits<std::uint16_t>::max)());
        if (playerSubtileX < 0 || playerSubtileX > MaximumPathCoordinate
            || playerSubtileY < 0 || playerSubtileY > MaximumPathCoordinate) {
            return false;
        }

        const auto nativeWidth = GetNativeWidth();
        const auto nativeHeight = GetNativeHeight();
        if (nativeWidth <= 0 || nativeHeight <= 0
            || nativeWidth > 32768 || nativeHeight > 32768) {
            return false;
        }

        const auto* const contextBytes = static_cast<const std::uint8_t*>(
            automapContext);
        const auto clipLeft = *reinterpret_cast<const std::int32_t*>(
            contextBytes + AutomapClipLeftOffset);
        const auto clipTop = *reinterpret_cast<const std::int32_t*>(
            contextBytes + AutomapClipTopOffset);
        const auto clipWidth = *reinterpret_cast<const std::int32_t*>(
            contextBytes + AutomapClipWidthOffset);
        const auto clipHeight = *reinterpret_cast<const std::int32_t*>(
            contextBytes + AutomapClipHeightOffset);
        if (clipWidth <= 0 || clipHeight <= 0) return false;

        output = {
            .automapContext = automapContext,
            .playerSubtileX = static_cast<std::uint16_t>(playerSubtileX),
            .playerSubtileY = static_cast<std::uint16_t>(playerSubtileY),
            .nativeWidth = nativeWidth,
            .nativeHeight = nativeHeight,
            .clipLeft = clipLeft,
            .clipTop = clipTop,
            .clipWidth = clipWidth,
            .clipHeight = clipHeight,
            .currentTick = currentTick,
            .epoch = epoch,
        };
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        AccessFaults.fetch_add(1U, std::memory_order_relaxed);
        return false;
    }
}

void RecordHostileDistanceBand(std::uint64_t distanceSquared) noexcept {
    switch (Detail::ClassifyWorldSubtileDistanceSquared(distanceSquared)) {
        case Detail::WorldSubtileDistanceBand::Through80:
            HostilesThrough80.fetch_add(1U, std::memory_order_relaxed);
            return;
        case Detail::WorldSubtileDistanceBand::From81Through140:
            HostilesFrom81Through140.fetch_add(1U, std::memory_order_relaxed);
            return;
        case Detail::WorldSubtileDistanceBand::From141Through220:
            HostilesFrom141Through220.fetch_add(1U, std::memory_order_relaxed);
            return;
        case Detail::WorldSubtileDistanceBand::Beyond220:
            HostilesBeyond220.fetch_add(1U, std::memory_order_relaxed);
            return;
    }
}

auto BuildCandidate(
        void* unit,
        const MonsterScanContext& scan,
        Candidate& candidate) noexcept -> bool {
    __try {
        if (!IsAlignedPointer(unit) || scan.automapContext == nullptr
            || GetUnitStat == nullptr || GetUnitAlignment == nullptr
            || GetUnitId == nullptr || GetUnitClassId == nullptr
            || GetUnitDataContext == nullptr || GetUnitMode == nullptr
            || GetDynamicPath == nullptr
            || GetUnitClientX == nullptr || GetUnitClientY == nullptr
            || PathGetX == nullptr || PathGetY == nullptr
            || GetMonStatsRecord == nullptr
            || ProjectClientToAutomap == nullptr) {
            return false;
        }

        UnitsObserved.fetch_add(1U, std::memory_order_relaxed);
        auto* const unitBytes = static_cast<std::uint8_t*>(unit);
        if (*reinterpret_cast<const std::uint32_t*>(
                unitBytes + UnitTypeOffset) != UnitMonster) {
            return false;
        }
        MonstersObserved.fetch_add(1U, std::memory_order_relaxed);

        const auto mode = GetUnitMode(unit);
        if (mode == MonsterModeDeath || mode == MonsterModeDead) {
            ModeRejected.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }

        // UnitMonster also covers mercenaries and asynchronous ambient actors.
        // D2R's native automap rejects those actors before consulting MonStats.
        const auto unitFlags = *reinterpret_cast<const std::uint32_t*>(
            unitBytes + UnitFlagsOffset);
        if (!Detail::IsEnemyMarkerUnitEligible(unitFlags)) {
            UnitFlagRejected.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }

        const auto classId = GetUnitClassId(unit);
        if (classId < 0) {
            MetadataFaults.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        const auto* const monStats = GetMonStatsRecord(
            GetUnitDataContext(unit),
            classId);
        if (monStats == nullptr) {
            MetadataFaults.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        const auto monStatsFlags = *reinterpret_cast<const std::uint32_t*>(
            monStats + MonStatsFlagsOffset);
        if (!Detail::IsEnemyMarkerClassEligible(monStatsFlags)) {
            ClassRejected.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }

        // Use the dedicated native alignment getter. Generic GetUnitStat
        // returns zero for a missing stat, which is indistinguishable from
        // Evil and previously admitted NPCs and scenery actors by accident.
        if (!Detail::IsEnemyMarkerAlignmentEligible(
                GetUnitAlignment(unit))) {
            AlignmentRejected.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        HostilesObserved.fetch_add(1U, std::memory_order_relaxed);

        auto* const monsterData = *reinterpret_cast<const std::uint8_t* const*>(
            unitBytes + UnitTypeDataOffset);
        if (monsterData == nullptr) {
            MetadataFaults.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        candidate.rank = Detail::ClassifyMonsterRankFlags(
            monsterData[MonsterRankFlagsOffset]);

        void* const unitPath = GetDynamicPath(unit);
        if (!IsAlignedPointer(unitPath)) {
            MetadataFaults.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        const auto unitSubtileX = PathGetX(unitPath);
        const auto unitSubtileY = PathGetY(unitPath);
        constexpr auto MaximumPathCoordinate = static_cast<std::int32_t>(
            (std::numeric_limits<std::uint16_t>::max)());
        if (unitSubtileX < 0 || unitSubtileX > MaximumPathCoordinate
            || unitSubtileY < 0 || unitSubtileY > MaximumPathCoordinate) {
            MetadataFaults.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        const auto deltaX = static_cast<std::int64_t>(unitSubtileX)
            - static_cast<std::int64_t>(scan.playerSubtileX);
        const auto deltaY = static_cast<std::int64_t>(unitSubtileY)
            - static_cast<std::int64_t>(scan.playerSubtileY);
        const auto distanceSquared = Detail::SquaredWorldSubtileDistance(
            static_cast<std::uint16_t>(unitSubtileX),
            static_cast<std::uint16_t>(unitSubtileY),
            scan.playerSubtileX,
            scan.playerSubtileY);
        UpdateMaximum(MaximumHostileDistanceSquared, distanceSquared);
        RecordHostileDistanceBand(distanceSquared);
        const auto radius = static_cast<std::int64_t>(
            MarkerRadius.load(std::memory_order_acquire));
        if (deltaX < -radius || deltaX > radius
            || deltaY < -radius || deltaY > radius) {
            RadiusRejected.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        const auto radiusSquared = static_cast<std::uint64_t>(radius * radius);
        if (distanceSquared > radiusSquared) {
            RadiusRejected.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        WithinRadius.fetch_add(1U, std::memory_order_relaxed);
        UpdateMaximum(MaximumWithinRadiusDistanceSquared, distanceSquared);

        const auto unitX = GetUnitClientX(unit);
        const auto unitY = GetUnitClientY(unit);
        NativePoint projected{};
        const auto packedClientCoordinates =
            static_cast<std::uint64_t>(static_cast<std::uint32_t>(unitX))
            | (static_cast<std::uint64_t>(
                static_cast<std::uint32_t>(unitY)) << 32U);
        if (ProjectClientToAutomap(
                scan.automapContext,
                &projected,
                packedClientCoordinates) != &projected) {
            ProjectionRejected.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }

        if (projected.x < scan.clipLeft || projected.y < scan.clipTop
            || static_cast<std::int64_t>(projected.x)
                >= static_cast<std::int64_t>(scan.clipLeft) + scan.clipWidth
            || static_cast<std::int64_t>(projected.y)
                >= static_cast<std::int64_t>(scan.clipTop) + scan.clipHeight) {
            NativeClipRejected.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }

        candidate.unitId = GetUnitId(unit);
        if (candidate.unitId == UINT32_MAX) return false;
        candidate.x = projected.x;
        candidate.y = projected.y;
        candidate.nativeWidth = scan.nativeWidth;
        candidate.nativeHeight = scan.nativeHeight;
        if (ImmunityCollectionEnabled.load(std::memory_order_acquire)) {
            candidate.immunityMask = Detail::BuildMonsterImmunityMask(
                std::array<std::int32_t, 6>{
                GetUnitStat(unit, PhysicalResistanceStatId, 0U),
                GetUnitStat(unit, FireResistanceStatId, 0U),
                GetUnitStat(unit, ColdResistanceStatId, 0U),
                GetUnitStat(unit, LightningResistanceStatId, 0U),
                GetUnitStat(unit, PoisonResistanceStatId, 0U),
                GetUnitStat(unit, MagicResistanceStatId, 0U),
            });
        }
        candidate.distanceSquared = distanceSquared;
        candidate.observedTick = scan.currentTick;
        candidate.epoch = scan.epoch;
        UpdateMaximum(MaximumAcceptedDistanceSquared, distanceSquared);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        AccessFaults.fetch_add(1U, std::memory_order_relaxed);
        return false;
    }
}

void ConsiderCandidate(const Candidate& candidate) noexcept {
    for (std::uint32_t attempt = 0U; attempt < 2U; ++attempt) {
        const auto observationState = ActiveObservationState.load(
            std::memory_order_acquire);
        const auto bufferIndex = ObservationBufferIndex(observationState);
        auto& buffer = ObservationBuffers[bufferIndex];
        buffer.writers.fetch_add(1U, std::memory_order_acq_rel);
        if (observationState != ActiveObservationState.load(
                std::memory_order_acquire)) {
            buffer.writers.fetch_sub(1U, std::memory_order_release);
            ContentionWaits.fetch_add(1U, std::memory_order_relaxed);
            continue;
        }
        if (candidate.epoch != Epoch.load(std::memory_order_acquire)) {
            buffer.writers.fetch_sub(1U, std::memory_order_acq_rel);
            return;
        }
        if (buffer.epoch.load(std::memory_order_acquire)
                != candidate.epoch) {
            buffer.writers.fetch_sub(1U, std::memory_order_acq_rel);
            return;
        }

        auto index = buffer.count.load(std::memory_order_acquire);
        ObservationChunk* chunk{};
        for (;;) {
            chunk = EnsureObservationChunk(
                buffer,
                index / ObservationsPerChunk);
            if (chunk == nullptr) {
                StorageFailures.fetch_add(1U, std::memory_order_relaxed);
                buffer.writers.fetch_sub(1U, std::memory_order_acq_rel);
                return;
            }
            if (buffer.count.compare_exchange_weak(
                    index,
                    index + 1U,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                break;
            }
        }
        chunk->observations[index % ObservationsPerChunk] = candidate;
        CandidatesAccepted.fetch_add(1U, std::memory_order_relaxed);
        PublishedSequence.fetch_add(1U, std::memory_order_acq_rel);
        buffer.writers.fetch_sub(1U, std::memory_order_acq_rel);
        return;
    }
    ContentionWaits.fetch_add(1U, std::memory_order_relaxed);
}

[[nodiscard]] auto ClaimMonsterTableScan(
        std::uint64_t currentTick) noexcept -> bool {
    auto previous = LastMonsterTableScanTick.load(std::memory_order_acquire);
    for (;;) {
        if (previous != 0U && currentTick >= previous
            && (currentTick - previous)
                < MonsterTableScanIntervalMilliseconds) {
            return false;
        }
        if (LastMonsterTableScanTick.compare_exchange_weak(
                previous,
                currentTick,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            return true;
        }
    }
}

void ScanClientMonsterTable(const MonsterScanContext& scan) noexcept {
    __try {
        if (Base == nullptr) return;
        auto** const buckets = reinterpret_cast<void**>(
            Base + ClientUnitHashTableRva + UnitHashTypeStride);
        MonsterTableScans.fetch_add(1U, std::memory_order_relaxed);

        std::size_t totalUnits{};
        for (std::size_t bucketIndex = 0U;
                bucketIndex < UnitHashBucketCount;
                ++bucketIndex) {
            MonsterBucketsVisited.fetch_add(1U, std::memory_order_relaxed);
            void* unit = buckets[bucketIndex];
            std::size_t bucketUnits{};
            while (unit != nullptr) {
                if (!IsAlignedPointer(unit)) {
                    AccessFaults.fetch_add(1U, std::memory_order_relaxed);
                    break;
                }
                if (totalUnits >= MaximumUnitsPerMonsterTableScan
                    || bucketUnits >= MaximumUnitsPerMonsterBucket) {
                    MonsterTraversalLimits.fetch_add(
                        1U,
                        std::memory_order_relaxed);
                    return;
                }

                auto* const unitBytes = static_cast<std::uint8_t*>(unit);
                void* const next = *reinterpret_cast<void**>(
                    unitBytes + UnitHashNextOffset);
                ++totalUnits;
                ++bucketUnits;

                Candidate candidate{};
                if (BuildCandidate(unit, scan, candidate)) {
                    ConsiderCandidate(candidate);
                }

                if (next == unit) {
                    MonsterTraversalLimits.fetch_add(
                        1U,
                        std::memory_order_relaxed);
                    break;
                }
                unit = next;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        AccessFaults.fetch_add(1U, std::memory_order_relaxed);
    }
}

__declspec(noinline) void __fastcall HookRenderAutomapUnit(
        void* unit,
        void* automapContext) noexcept {
    const auto original = OriginalRenderAutomapUnit;
    if (original == nullptr) return;

    // Preserve D2R behavior first and exactly once. Marker observation is a
    // fail-closed side effect that cannot suppress the native automap pass.
    original(unit, automapContext);
    if (!Active.load(std::memory_order_acquire)) {
        return;
    }

    // Navigation and the complete client-unit scan share this one canonical
    // automap hook. Both begin only on the local-player pass and retain no
    // native pointer or context after the pass returns.
    void* const player = TryGetLocalPlayerPass(unit);
    if (player == nullptr) return;
    ObserveNavigationPlayerPass(player, automapContext);
    if (!CollectionEnabled.load(std::memory_order_acquire)) return;

    const auto currentTick = static_cast<std::uint64_t>(GetTickCount64());
    BeginAutomapPulse(currentTick);
    if (!ClaimMonsterTableScan(currentTick)) return;
    const auto epoch = Epoch.load(std::memory_order_acquire);
    MonsterScanContext scan{};
    if (!BuildMonsterScanContext(
            player,
            automapContext,
            currentTick,
            epoch,
            scan)) {
        return;
    }
    ScanClientMonsterTable(scan);
}

auto ValidateRuntime(const D2RL::PluginContext* context) noexcept -> bool {
    constexpr std::array<std::uint8_t, 10> localContextExpected{
        0x8B, 0x05, 0x2E, 0x84, 0x99, 0x02, 0xC3, 0xCC,
        0xCC, 0xCC};
    constexpr std::array<std::uint8_t, 32> localPlayerExpected{
        0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83,
        0xEC, 0x20, 0x83, 0xF9, 0x08, 0x0F, 0x83, 0x85,
        0x00, 0x00, 0x00, 0x8B, 0xD9, 0x48, 0x89, 0x5C,
        0x24, 0x38, 0x48, 0x83, 0xFB, 0x08, 0x72, 0x19};
    constexpr std::array<std::uint8_t, 38> nativeHeightExpected{
        0x48, 0x83, 0xEC, 0x28, 0xE8, 0x67, 0x6D, 0x7C,
        0x00, 0x84, 0xC0, 0x74, 0x0E, 0xE8, 0x1E, 0x51,
        0x5D, 0x00, 0x48, 0xC1, 0xE8, 0x20, 0x48, 0x83,
        0xC4, 0x28, 0xC3, 0x8B, 0x05, 0xAB, 0x18, 0x22,
        0x02, 0x48, 0x83, 0xC4, 0x28, 0xC3};
    constexpr std::array<std::uint8_t, 33> nativeWidthExpected{
        0x48, 0x83, 0xEC, 0x28, 0xE8, 0xF7, 0x6C, 0x7C,
        0x00, 0x84, 0xC0, 0x74, 0x09, 0x48, 0x83, 0xC4,
        0x28, 0xE9, 0xAA, 0x50, 0x5D, 0x00, 0x8B, 0x05,
        0x3C, 0x18, 0x22, 0x02, 0x48, 0x83, 0xC4, 0x28,
        0xC3};
    constexpr std::array<std::uint8_t, 32> projectExpected{
        0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x6C,
        0x24, 0x18, 0x48, 0x89, 0x74, 0x24, 0x20, 0x57,
        0x41, 0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x20,
        0x66, 0x0F, 0x6E, 0x41, 0x10, 0x4C, 0x8B, 0xFA};
    constexpr std::array<std::uint8_t, 32> renderUnitExpected{
        0x48, 0x89, 0x6C, 0x24, 0x10, 0x57, 0x48, 0x83,
        0xEC, 0x40, 0x48, 0x8B, 0xFA, 0x4C, 0x8D, 0x44,
        0x24, 0x68, 0x48, 0x8D, 0x54, 0x24, 0x60, 0x48,
        0x8B, 0xE9, 0xE8, 0xF1, 0x01, 0x00, 0x00, 0x84};
    constexpr std::array<std::uint8_t, 32> getUnitStatExpected{
        0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x6C,
        0x24, 0x18, 0x48, 0x89, 0x74, 0x24, 0x20, 0x57,
        0x48, 0x83, 0xEC, 0x20, 0x41, 0x0F, 0xB7, 0xE8,
        0x8B, 0xFA, 0x48, 0x8B, 0xD9, 0x48, 0x85, 0xC9};
    constexpr std::array<std::uint8_t, 32> getUnitAlignmentExpected{
        0x48, 0x89, 0x5C, 0x24, 0x18, 0x48, 0x89, 0x74,
        0x24, 0x20, 0x57, 0x48, 0x83, 0xEC, 0x30, 0x48,
        0x8B, 0xF1, 0x48, 0x85, 0xC9, 0x75, 0x13, 0x88,
        0x4C, 0x24, 0x40, 0x48, 0x8D, 0x4C, 0x24, 0x40};
    constexpr std::array<std::uint8_t, 32> getXExpected{
        0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
        0xD9, 0x48, 0x85, 0xC9, 0x75, 0x13, 0x88, 0x4C,
        0x24, 0x30, 0x48, 0x8D, 0x4C, 0x24, 0x30, 0xE8,
        0x94, 0xA3, 0xFF, 0xFF, 0x84, 0xC0, 0x74, 0x01};
    constexpr std::array<std::uint8_t, 32> getYExpected{
        0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
        0xD9, 0x48, 0x85, 0xC9, 0x75, 0x13, 0x88, 0x4C,
        0x24, 0x30, 0x48, 0x8D, 0x4C, 0x24, 0x30, 0xE8,
        0x94, 0x9D, 0xFF, 0xFF, 0x84, 0xC0, 0x74, 0x01};
    constexpr std::array<std::uint8_t, 68> getDynamicPathExpected{
        0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
        0xD9, 0x48, 0x85, 0xC9, 0x75, 0x13, 0x88, 0x4C,
        0x24, 0x30, 0x48, 0x8D, 0x4C, 0x24, 0x30, 0xE8,
        0xD4, 0xB9, 0xFF, 0xFF, 0x84, 0xC0, 0x74, 0x01,
        0xCC, 0x83, 0x3B, 0x05, 0x75, 0x14, 0x48, 0x8D,
        0x4C, 0x24, 0x30, 0xC6, 0x44, 0x24, 0x30, 0x00,
        0xE8, 0xEB, 0xA4, 0xFF, 0xFF, 0x84, 0xC0, 0x74,
        0x01, 0xCC, 0x48, 0x8B, 0x43, 0x38, 0x48, 0x83,
        0xC4, 0x20, 0x5B, 0xC3};
    constexpr std::array<std::uint8_t, 16> pathGetXExpected{
        0x0F, 0xB7, 0x41, 0x02, 0xC3, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC};
    constexpr std::array<std::uint8_t, 16> pathGetYExpected{
        0x0F, 0xB7, 0x41, 0x06, 0xC3, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC};
    constexpr std::array<std::uint8_t, 46> getUnitIdExpected{
        0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75,
        0x1D, 0x88, 0x4C, 0x24, 0x30, 0x48, 0x8D, 0x4C,
        0x24, 0x30, 0xE8, 0x39, 0xCA, 0xFF, 0xFF, 0x84,
        0xC0, 0x74, 0x01, 0xCC, 0xB8, 0xFF, 0xFF, 0xFF,
        0xFF, 0x48, 0x83, 0xC4, 0x28, 0xC3, 0x8B, 0x41,
        0x08, 0x48, 0x83, 0xC4, 0x28, 0xC3};
    constexpr std::array<std::uint8_t, 32> getUnitClassIdExpected{
        0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75,
        0x1D, 0x88, 0x4C, 0x24, 0x30, 0x48, 0x8D, 0x4C,
        0x24, 0x30, 0xE8, 0x49, 0xCB, 0xFF, 0xFF, 0x84,
        0xC0, 0x74, 0x01, 0xCC, 0xB8, 0xFF, 0xFF, 0xFF};
    constexpr std::array<std::uint8_t, 47> getUnitDataContextExpected{
        0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75,
        0x1A, 0x88, 0x4C, 0x24, 0x30, 0x48, 0x8D, 0x4C,
        0x24, 0x30, 0xE8, 0x49, 0xC7, 0xFF, 0xFF, 0x84,
        0xC0, 0x74, 0x01, 0xCC, 0x32, 0xC0, 0x48, 0x83,
        0xC4, 0x28, 0xC3, 0x0F, 0xB6, 0x81, 0xBD, 0x01,
        0x00, 0x00, 0x48, 0x83, 0xC4, 0x28, 0xC3};
    constexpr std::array<std::uint8_t, 43> getUnitModeExpected{
        0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75,
        0x1A, 0x88, 0x4C, 0x24, 0x30, 0x48, 0x8D, 0x4C,
        0x24, 0x30, 0xE8, 0xF9, 0xA3, 0xFF, 0xFF, 0x84,
        0xC0, 0x74, 0x01, 0xCC, 0x33, 0xC0, 0x48, 0x83,
        0xC4, 0x28, 0xC3, 0x8B, 0x41, 0x0C, 0x48, 0x83,
        0xC4, 0x28, 0xC3};
    constexpr std::array<std::uint8_t, 27> monsterRankWitnessExpected{
        0x48, 0x8B, 0xCB, 0xE8, 0x48, 0xC7, 0xE2, 0xFF,
        0x83, 0xF8, 0x01, 0x75, 0x06, 0x48, 0x8B, 0x43,
        0x10, 0xEB, 0x02, 0x33, 0xC0, 0xF6, 0x40, 0x1A,
        0x0E, 0x75, 0x30};
    constexpr std::array<std::uint8_t, 32> getMonStatsRecordExpected{
        0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74,
        0x24, 0x20, 0x57, 0x48, 0x83, 0xEC, 0x30, 0x48,
        0x63, 0xF2, 0xE8, 0x99, 0x93, 0x26, 0x00, 0x48,
        0x8B, 0xF8, 0x48, 0x8B, 0xDE, 0x85, 0xF6, 0x78};
    constexpr std::array<std::uint8_t, 36> unitRoomLayoutWitnessExpected{
        0x8B, 0x0B, 0x83, 0xE9, 0x02, 0x74, 0x25, 0x83,
        0xE9, 0x02, 0x74, 0x20, 0x83, 0xF9, 0x01, 0x74,
        0x1B, 0x48, 0x8B, 0x4B, 0x38, 0x48, 0x85, 0xC9,
        0x74, 0x0A, 0x48, 0x83, 0xC4, 0x20, 0x5B, 0xE9,
        0xAB, 0x67, 0xFF, 0xFF};
    constexpr std::array<std::uint8_t, 5> pathGetRoomExpected{
        0x48, 0x8B, 0x41, 0x20, 0xC3};
    constexpr std::array<std::uint8_t, 25> isRoomInTownExpected{
        0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75,
        0x07, 0x33, 0xC0, 0x48, 0x83, 0xC4, 0x28, 0xC3,
        0x48, 0x8B, 0x49, 0x18, 0xE8, 0x57, 0x08, 0x07,
        0x00};
    constexpr std::array<std::uint8_t, 14> drlgRoomLevelIdExpected{
        0x48, 0x8B, 0x81, 0x90, 0x00, 0x00, 0x00, 0x8B,
        0x80, 0xF8, 0x01, 0x00, 0x00, 0xC3};
    constexpr std::array<std::uint8_t, 28> clientUnitHashTableExpected{
        0x4C, 0x63, 0xCA, 0x48, 0x8D, 0x05, 0x36, 0x93,
        0x98, 0x02, 0x8B, 0xD1, 0x44, 0x8B, 0xC1, 0x49,
        0x8B, 0xC9, 0x83, 0xE2, 0x7F, 0x48, 0xC1, 0xE1,
        0x0A, 0x48, 0x03, 0xC8};
    constexpr std::array<std::uint8_t, 41> clientUnitHashLookupExpected{
        0x48, 0x63, 0xC2, 0x48, 0x8B, 0x04, 0xC1, 0x48,
        0x85, 0xC0, 0x74, 0x1A, 0x44, 0x39, 0x40, 0x08,
        0x75, 0x05, 0x44, 0x39, 0x08, 0x74, 0x11, 0x48,
        0x8B, 0x88, 0x58, 0x01, 0x00, 0x00, 0x48, 0x8B,
        0xC1, 0x48, 0x85, 0xC9, 0xEB, 0xE4, 0x33, 0xC0,
        0xC3};
    const auto check = [context](
            std::uintptr_t rva,
            const auto& expected) noexcept {
        return context->CheckExpectedBytes(
            rva,
            expected.data(),
            static_cast<std::uint32_t>(expected.size()));
    };
    return check(GetLocalDataContextRva, localContextExpected)
        && check(GetLocalPlayerRva, localPlayerExpected)
        && check(GetNativeHeightRva, nativeHeightExpected)
        && check(GetNativeWidthRva, nativeWidthExpected)
        && check(ProjectClientToAutomapRva, projectExpected)
        && check(GetUnitStatRva, getUnitStatExpected)
        && check(GetUnitAlignmentRva, getUnitAlignmentExpected)
        && check(GetUnitIdRva, getUnitIdExpected)
        && check(GetUnitClassIdRva, getUnitClassIdExpected)
        && check(GetUnitDataContextRva, getUnitDataContextExpected)
        && check(GetUnitModeRva, getUnitModeExpected)
        && check(GetDynamicPathRva, getDynamicPathExpected)
        && check(GetUnitClientXRva, getXExpected)
        && check(GetUnitClientYRva, getYExpected)
        && check(PathGetXRva, pathGetXExpected)
        && check(PathGetYRva, pathGetYExpected)
        && check(GetMonStatsRecordRva, getMonStatsRecordExpected)
        && check(MonsterRankWitnessRva, monsterRankWitnessExpected)
        && check(UnitRoomLayoutWitnessRva, unitRoomLayoutWitnessExpected)
        && check(PathGetRoomRva, pathGetRoomExpected)
        && check(IsRoomInTownRva, isRoomInTownExpected)
        && check(GetDrlgRoomLevelIdRva, drlgRoomLevelIdExpected)
        && check(
            ClientUnitHashTableWitnessRva,
            clientUnitHashTableExpected)
        && check(
            ClientUnitHashLookupWitnessRva,
            clientUnitHashLookupExpected)
        && check(RenderAutomapUnitRva, renderUnitExpected);
}

} // namespace

auto InitializeNativeAutomapMarker(
        const D2RL::PluginContext* context,
        bool navigationProjectionDiagnosticsEnabled) noexcept -> bool {
    if (!D2RL::HasContext(context)
        || context->apiVersion != D2RL_PLUGIN_API_VERSION
        || context->exeBase == 0) {
        return false;
    }
    if (Active.load(std::memory_order_acquire)) return true;
    if (!ValidateRuntime(context)) {
        context->LogWarn(
            "MapSense: native automap marker signature or ABI mismatch; marker hook refused.");
        return false;
    }
    DiagnosticContext = navigationProjectionDiagnosticsEnabled
        ? context
        : nullptr;
    NavigationProjectionDiagnostics.Reset();
    for (auto& buffer : ObservationBuffers) {
        if (buffer.first == nullptr) {
            buffer.first = new (std::nothrow) ObservationChunk{};
        }
        if (buffer.first == nullptr
            || EnsureObservationChunk(
                buffer,
                InitialObservationChunkCount - 1U) == nullptr) {
            context->LogWarn(
                "MapSense: native automap observation reserve could not be allocated; marker hook refused.");
            return false;
        }
        buffer.count.store(0U, std::memory_order_relaxed);
        buffer.writers.store(0U, std::memory_order_relaxed);
        buffer.epoch.store(0U, std::memory_order_relaxed);
    }
    try {
        MarkerCache.clear();
        MarkerCache.reserve(InitialMarkerReserve);
    } catch (...) {
        context->LogWarn(
            "MapSense: renderer marker cache could not be reserved; marker hook refused.");
        return false;
    }
    ActiveObservationState.store(0U, std::memory_order_release);
    PendingObservationBuffer = -1;
    RendererEpoch = 0U;
    LastPurgeTick = 0U;

    Base = reinterpret_cast<std::uint8_t*>(context->exeBase);
    GetLocalDataContext = At<GetLocalDataContextFn>(
        GetLocalDataContextRva);
    GetLocalPlayer = At<GetLocalPlayerFn>(GetLocalPlayerRva);
    GetNativeHeight = At<GetNativeDimensionFn>(GetNativeHeightRva);
    GetNativeWidth = At<GetNativeDimensionFn>(GetNativeWidthRva);
    GetUnitStat = At<GetUnitStatFn>(GetUnitStatRva);
    GetUnitAlignment = At<GetUnitSignedValueFn>(GetUnitAlignmentRva);
    GetUnitId = At<GetUnitValueFn>(GetUnitIdRva);
    GetUnitClassId = At<GetUnitSignedValueFn>(GetUnitClassIdRva);
    GetUnitDataContext = At<GetUnitDataContextFn>(GetUnitDataContextRva);
    GetUnitMode = At<GetUnitValueFn>(GetUnitModeRva);
    GetDynamicPath = At<GetNativePointerFn>(GetDynamicPathRva);
    GetUnitClientX = At<GetUnitCoordinateFn>(GetUnitClientXRva);
    GetUnitClientY = At<GetUnitCoordinateFn>(GetUnitClientYRva);
    PathGetX = At<GetUnitCoordinateFn>(PathGetXRva);
    PathGetY = At<GetUnitCoordinateFn>(PathGetYRva);
    GetUnitRoom = At<GetNativePointerFn>(GetUnitRoomRva);
    GetDrlgRoomLevelId = At<GetLevelIdFn>(GetDrlgRoomLevelIdRva);
    IsRoomInTown = At<IsRoomInTownFn>(IsRoomInTownRva);
    GetMonStatsRecord = At<GetMonStatsRecordFn>(GetMonStatsRecordRva);
    ProjectClientToAutomap = At<ProjectClientToAutomapFn>(
        ProjectClientToAutomapRva);
    CollectionEnabled.store(false, std::memory_order_release);
    ImmunityCollectionEnabled.store(false, std::memory_order_release);
    ResetPublishedMarkers(true);

    constexpr std::array<std::uint8_t, 32> renderUnitExpected{
        0x48, 0x89, 0x6C, 0x24, 0x10, 0x57, 0x48, 0x83,
        0xEC, 0x40, 0x48, 0x8B, 0xFA, 0x4C, 0x8D, 0x44,
        0x24, 0x68, 0x48, 0x8D, 0x54, 0x24, 0x60, 0x48,
        0x8B, 0xE9, 0xE8, 0xF1, 0x01, 0x00, 0x00, 0x84};
    if (!context->InstallInlineHook(
            RenderAutomapUnitRva,
            renderUnitExpected.data(),
            static_cast<std::uint32_t>(renderUnitExpected.size()),
            HookRenderAutomapUnit,
            &OriginalRenderAutomapUnit)) {
        Base = nullptr;
        GetLocalDataContext = nullptr;
        GetLocalPlayer = nullptr;
        GetNativeHeight = nullptr;
        GetNativeWidth = nullptr;
        GetUnitStat = nullptr;
        GetUnitAlignment = nullptr;
        GetUnitId = nullptr;
        GetUnitClassId = nullptr;
        GetUnitDataContext = nullptr;
        GetUnitMode = nullptr;
        GetDynamicPath = nullptr;
        GetUnitClientX = nullptr;
        GetUnitClientY = nullptr;
        PathGetX = nullptr;
        PathGetY = nullptr;
        GetUnitRoom = nullptr;
        GetDrlgRoomLevelId = nullptr;
        IsRoomInTown = nullptr;
        GetMonStatsRecord = nullptr;
        ProjectClientToAutomap = nullptr;
        DiagnosticContext = nullptr;
        context->LogWarn(
            "MapSense: native automap marker hook installation was refused.");
        return false;
    }

    Active.store(true, std::memory_order_release);
    return true;
}

void ShutdownNativeAutomapMarker() noexcept {
    CollectionEnabled.store(false, std::memory_order_release);
    ImmunityCollectionEnabled.store(false, std::memory_order_release);
    Active.store(false, std::memory_order_release);
    LevelObservedCallback.store(nullptr, std::memory_order_release);
    LevelObservedUserData.store(nullptr, std::memory_order_release);
    ResetPublishedMarkers(false);
    // D2RLoader restores the inline hook after the unload callback. Keep the
    // trampoline and verified code addresses alive so an in-flight native call
    // can finish and pass through exactly once before the DLL is unmapped.
}

void ResetNativeAutomapMarker() noexcept {
    ResetPublishedMarkers(true);
}

void InvalidateNativeAutomapMarkerFrame() noexcept {
    ResetPublishedMarkers(false);
}

void SetNativeAutomapMarkerEnabled(bool enabled) noexcept {
    const auto previous = CollectionEnabled.exchange(
        enabled,
        std::memory_order_acq_rel);
    if (previous != enabled) ResetPublishedMarkers(false);
}

void SetNativeAutomapMarkerRadius(std::int32_t radius) noexcept {
    const auto clamped = std::clamp(
        radius,
        MinimumMonsterDetectionRadius,
        MaximumMonsterDetectionRadius);
    const auto previous = MarkerRadius.exchange(
        clamped,
        std::memory_order_acq_rel);
    if (previous != clamped) ResetPublishedMarkers(true);
}

void SetNativeAutomapImmunityCollectionEnabled(bool enabled) noexcept {
    const auto previous = ImmunityCollectionEnabled.exchange(
        enabled,
        std::memory_order_acq_rel);
    if (previous != enabled) ResetPublishedMarkers(false);
}

void SetNativeAutomapLevelObservedCallback(
        NativeAutomapLevelObservedCallback callback,
        void* userData) noexcept {
    if (callback == nullptr) {
        LevelObservedCallback.store(nullptr, std::memory_order_release);
        LevelObservedUserData.store(nullptr, std::memory_order_release);
        return;
    }
    LevelObservedUserData.store(userData, std::memory_order_release);
    LevelObservedCallback.store(callback, std::memory_order_release);
}

auto AcquireNativeAutomapMarkers(
        std::vector<NativeAutomapMarkerSnapshot>& snapshots) noexcept
        -> std::size_t {
    snapshots.clear();
    if (!Active.load(std::memory_order_acquire)
        || !CollectionEnabled.load(std::memory_order_acquire)) {
        return 0U;
    }

    const auto currentTick = static_cast<std::uint64_t>(GetTickCount64());
    if (!IsRecent(
            LastAutomapPulseTick.load(std::memory_order_acquire),
            currentTick)) {
        return 0U;
    }

    try {
        const auto epoch = Epoch.load(std::memory_order_acquire);
        if (RendererEpoch != epoch) {
            MarkerCache.clear();
            RendererEpoch = epoch;
            LastPurgeTick = 0U;
            PendingObservationBuffer = -1;
        }

        if (PendingObservationBuffer < 0) {
            const auto activeState = ActiveObservationState.load(
                std::memory_order_acquire);
            const auto next = ObservationBufferIndex(activeState) ^ 1U;
            auto& nextBuffer = ObservationBuffers[next];
            if (nextBuffer.writers.load(std::memory_order_acquire) == 0U) {
                nextBuffer.count.store(0U, std::memory_order_relaxed);
                nextBuffer.epoch.store(epoch, std::memory_order_release);
                const auto sealedState = ActiveObservationState.exchange(
                    NextObservationState(activeState),
                    std::memory_order_acq_rel);
                PendingObservationBuffer = static_cast<std::int32_t>(
                    ObservationBufferIndex(sealedState));
            } else {
                ContentionWaits.fetch_add(1U, std::memory_order_relaxed);
            }
        }

        if (PendingObservationBuffer >= 0) {
            auto& sealed = ObservationBuffers[
                static_cast<std::size_t>(PendingObservationBuffer)];
            if (sealed.writers.load(std::memory_order_acquire) == 0U) {
                std::atomic_thread_fence(std::memory_order_acquire);
                auto remaining = sealed.count.load(std::memory_order_acquire);
                auto* chunk = sealed.first;
                while (remaining != 0U && chunk != nullptr) {
                    const auto chunkCount = std::min(
                        remaining,
                        ObservationsPerChunk);
                    for (std::uint64_t index = 0U;
                            index < chunkCount;
                            ++index) {
                        const auto& marker = chunk->observations[index];
                        if (marker.epoch != epoch
                            || !IsRecent(marker.observedTick, currentTick)) {
                            continue;
                        }
                        const auto existing = MarkerCache.find(marker.unitId);
                        if (existing == MarkerCache.end()) {
                            MarkerCache.emplace(marker.unitId, marker);
                            MarkersInserted.fetch_add(
                                1U,
                                std::memory_order_relaxed);
                        } else {
                            existing->second = marker;
                            MarkersRefreshed.fetch_add(
                                1U,
                                std::memory_order_relaxed);
                        }
                    }
                    remaining -= chunkCount;
                    chunk = chunk->next.load(std::memory_order_acquire);
                }
                if (remaining != 0U) {
                    StorageFailures.fetch_add(1U, std::memory_order_relaxed);
                }
                PendingObservationBuffer = -1;
            } else {
                ContentionWaits.fetch_add(1U, std::memory_order_relaxed);
            }
        }

        if (LastPurgeTick == 0U || currentTick < LastPurgeTick
            || (currentTick - LastPurgeTick)
                >= MarkerLifetimeMilliseconds) {
            std::uint64_t expired{};
            for (auto iterator = MarkerCache.begin();
                    iterator != MarkerCache.end();) {
                const auto& marker = iterator->second;
                if (marker.epoch != epoch
                    || !IsRecent(marker.observedTick, currentTick)) {
                    iterator = MarkerCache.erase(iterator);
                    ++expired;
                } else {
                    ++iterator;
                }
            }
            if (expired != 0U) {
                MarkersExpired.fetch_add(expired, std::memory_order_relaxed);
            }
            LastPurgeTick = currentTick;
        }

        if (Epoch.load(std::memory_order_acquire) != epoch) {
            MarkerCache.clear();
            TrackedMarkerCount.store(0U, std::memory_order_release);
            return 0U;
        }

        snapshots.reserve(MarkerCache.size());
        const auto sequence = PublishedSequence.load(
            std::memory_order_acquire);
        for (const auto& [unitId, marker] : MarkerCache) {
            if (marker.epoch != epoch
                || !IsRecent(marker.observedTick, currentTick)) {
                continue;
            }
            snapshots.push_back(NativeAutomapMarkerSnapshot{
                .unitId = unitId,
                .x = marker.x,
                .y = marker.y,
                .nativeWidth = marker.nativeWidth,
                .nativeHeight = marker.nativeHeight,
                .rank = marker.rank,
                .immunityMask = marker.immunityMask,
                .epoch = marker.epoch,
                .sequence = sequence,
            });
            UpdateMaximum(
                MaximumPublishedDistanceSquared,
                marker.distanceSquared);
        }
        TrackedMarkerCount.store(
            static_cast<std::uint64_t>(snapshots.size()),
            std::memory_order_release);
        return snapshots.size();
    } catch (...) {
        snapshots.clear();
        StorageFailures.fetch_add(1U, std::memory_order_relaxed);
        return 0U;
    }
}

auto WantsNativeAutomapMarkerFrame() noexcept -> bool {
    if (!Active.load(std::memory_order_acquire)
        || !CollectionEnabled.load(std::memory_order_acquire)) {
        return false;
    }
    const auto currentTick = static_cast<std::uint64_t>(GetTickCount64());
    if (!IsRecent(
            LastAutomapPulseTick.load(std::memory_order_acquire),
            currentTick)) {
        return false;
    }
    // A recent native automap pulse is sufficient. This also lets the
    // renderer rotate the buffers immediately after an epoch reset, before
    // the first marker of the new gameplay epoch is published.
    return true;
}

auto GetNativeAutomapMarkerCounters() noexcept
        -> NativeAutomapMarkerCounters {
    return {
        .automapPulses = AutomapPulses.load(std::memory_order_relaxed),
        .monsterTableScans = MonsterTableScans.load(
            std::memory_order_relaxed),
        .monsterBucketsVisited = MonsterBucketsVisited.load(
            std::memory_order_relaxed),
        .monsterTraversalLimits = MonsterTraversalLimits.load(
            std::memory_order_relaxed),
        .unitsObserved = UnitsObserved.load(std::memory_order_relaxed),
        .monstersObserved = MonstersObserved.load(std::memory_order_relaxed),
        .modeRejected = ModeRejected.load(std::memory_order_relaxed),
        .unitFlagRejected = UnitFlagRejected.load(
            std::memory_order_relaxed),
        .classRejected = ClassRejected.load(
            std::memory_order_relaxed),
        .alignmentRejected = AlignmentRejected.load(
            std::memory_order_relaxed),
        .metadataFaults = MetadataFaults.load(
            std::memory_order_relaxed),
        .hostilesObserved = HostilesObserved.load(
            std::memory_order_relaxed),
        .hostilesThrough80 = HostilesThrough80.load(
            std::memory_order_relaxed),
        .hostilesFrom81Through140 = HostilesFrom81Through140.load(
            std::memory_order_relaxed),
        .hostilesFrom141Through220 = HostilesFrom141Through220.load(
            std::memory_order_relaxed),
        .hostilesBeyond220 = HostilesBeyond220.load(
            std::memory_order_relaxed),
        .radiusRejected = RadiusRejected.load(std::memory_order_relaxed),
        .withinRadius = WithinRadius.load(std::memory_order_relaxed),
        .projectionRejected = ProjectionRejected.load(
            std::memory_order_relaxed),
        .nativeClipRejected = NativeClipRejected.load(
            std::memory_order_relaxed),
        .candidatesAccepted = CandidatesAccepted.load(
            std::memory_order_relaxed),
        .markersInserted = MarkersInserted.load(
            std::memory_order_relaxed),
        .markersRefreshed = MarkersRefreshed.load(
            std::memory_order_relaxed),
        .markersExpired = MarkersExpired.load(
            std::memory_order_relaxed),
        .freshMarkers = TrackedMarkerCount.load(
            std::memory_order_relaxed),
        .contentionWaits = ContentionWaits.load(
            std::memory_order_relaxed),
        .storageFailures = StorageFailures.load(
            std::memory_order_relaxed),
        .accessFaults = AccessFaults.load(std::memory_order_relaxed),
        .configuredRadius = MarkerRadius.load(std::memory_order_relaxed),
        .maximumHostileDistance = DistanceFromSquared(
            MaximumHostileDistanceSquared.load(std::memory_order_relaxed)),
        .maximumWithinRadiusDistance = DistanceFromSquared(
            MaximumWithinRadiusDistanceSquared.load(
                std::memory_order_relaxed)),
        .maximumAcceptedDistance = DistanceFromSquared(
            MaximumAcceptedDistanceSquared.load(std::memory_order_relaxed)),
        .maximumPublishedDistance = DistanceFromSquared(
            MaximumPublishedDistanceSquared.load(std::memory_order_relaxed)),
    };
}

} // namespace RuffnecKk::MapSense
