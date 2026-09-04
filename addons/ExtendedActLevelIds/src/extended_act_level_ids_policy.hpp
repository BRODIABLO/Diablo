#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <compare>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace ruffneckk::extended_act_level_ids {

inline constexpr std::size_t LevelsIdOffset = 0x00;
inline constexpr std::size_t LevelsActOffset = 0x0D;
inline constexpr std::uint32_t LevelsRowSize = 0x18C;
inline constexpr std::uint8_t MinimumDataContext = 1;
inline constexpr std::uint8_t MaximumDataContext = 3;
inline constexpr std::uint8_t MaximumAct = 4;
inline constexpr std::uint32_t MaximumCompiledLevelRecords = 1023;
inline constexpr std::int32_t MaximumLevelId =
    static_cast<std::int32_t>(MaximumCompiledLevelRecords - 1);
inline constexpr std::uint32_t MaximumKeyedValidationLevelId = 0xFF;
inline constexpr std::int32_t MaximumVanillaNetworkLevelId = 0xFF;
inline constexpr std::uint32_t DynamicTownPortalClassId = 59;
inline constexpr std::uint32_t InvalidUnitGuid = 0xFFFFFFFFU;
inline constexpr std::size_t MaximumClientPortalEntries = 1024;
inline constexpr std::uint16_t CoordinateValueMask = 0x1FFF;
inline constexpr std::uint16_t CodecMarkerMask = 0x8000;
inline constexpr std::uint16_t LevelIdPayloadMask = 0x6000;
inline constexpr std::uint16_t EncodedCoordinateMask =
    CoordinateValueMask | CodecMarkerMask | LevelIdPayloadMask;
inline constexpr unsigned LevelIdPayloadShift = 13;

struct ActEntry {
    std::int32_t levelId{};
    std::uint8_t act{};

    auto operator<=>(const ActEntry&) const noexcept = default;
};

constexpr bool IsSupportedDataContext(std::uint8_t dataContext) noexcept {
    return dataContext >= MinimumDataContext
        && dataContext <= MaximumDataContext;
}

constexpr bool HasValidLevelRecordCount(std::uint32_t rowCount) noexcept {
    return rowCount > 0 && rowCount <= MaximumCompiledLevelRecords;
}

constexpr std::uint32_t KeyedValidationRowCount(
        std::uint32_t rowCount) noexcept {
    return std::min(
        rowCount,
        MaximumKeyedValidationLevelId + 1U);
}

constexpr bool IsCanonicalLevelId(
        std::int32_t levelId,
        std::uint32_t rowIndex) noexcept {
    return levelId >= 0
        && levelId <= MaximumLevelId
        && static_cast<std::uint32_t>(levelId) == rowIndex;
}

struct EncodedLevelCoordinate {
    std::uint8_t lowLevelId{};
    std::uint16_t x{};

    auto operator<=>(const EncodedLevelCoordinate&) const noexcept = default;
};

struct DecodedLevelCoordinate {
    std::int32_t levelId{};
    std::uint16_t x{};

    auto operator<=>(const DecodedLevelCoordinate&) const noexcept = default;
};

struct PortalEndpointDescriptor {
    std::uint64_t sessionGeneration{};
    std::uintptr_t gameIdentity{};
    std::uint32_t guid{InvalidUnitGuid};
    std::uint32_t counterpartGuid{InvalidUnitGuid};
    std::uint32_t classId{};
    std::int32_t destinationLevelId{};
    std::uint8_t nativeLowLevelId{};

    auto operator<=>(const PortalEndpointDescriptor&) const noexcept = default;
};

struct ClientPortalDescriptor {
    std::uint64_t sessionGeneration{};
    std::uint32_t guid{InvalidUnitGuid};
    std::int32_t destinationLevelId{};
    std::uint8_t nativeLowLevelId{};

    auto operator<=>(const ClientPortalDescriptor&) const noexcept = default;
};

enum class ClientPortalLookupDecision : std::uint8_t {
    Original,
    FullLevelId,
    Refuse,
};

struct ClientPortalLookupResult {
    ClientPortalLookupDecision decision{ClientPortalLookupDecision::Refuse};
    std::int32_t levelId{};

    auto operator<=>(const ClientPortalLookupResult&) const noexcept = default;
};

constexpr bool IsExtendedLevelId(std::int32_t levelId) noexcept {
    return levelId > MaximumVanillaNetworkLevelId
        && levelId <= MaximumLevelId;
}

constexpr std::uint8_t LowLevelId(std::int32_t levelId) noexcept {
    return static_cast<std::uint8_t>(levelId);
}

constexpr bool IsValidPortalEndpoint(
        const PortalEndpointDescriptor& endpoint) noexcept {
    return endpoint.gameIdentity != 0
        && endpoint.guid != InvalidUnitGuid
        && endpoint.counterpartGuid != InvalidUnitGuid
        && endpoint.guid != endpoint.counterpartGuid
        && endpoint.classId == DynamicTownPortalClassId
        && endpoint.destinationLevelId >= 0
        && endpoint.destinationLevelId <= MaximumLevelId
        && endpoint.nativeLowLevelId
            == LowLevelId(endpoint.destinationLevelId);
}

constexpr bool IsValidClientPortalDescriptor(
        const ClientPortalDescriptor& descriptor) noexcept {
    return descriptor.guid != InvalidUnitGuid
        && IsExtendedLevelId(descriptor.destinationLevelId)
        && descriptor.nativeLowLevelId
            == LowLevelId(descriptor.destinationLevelId);
}

inline bool UpsertClientPortalDescriptor(
        std::vector<ClientPortalDescriptor>& entries,
        const ClientPortalDescriptor& descriptor,
        std::size_t maximumEntries = MaximumClientPortalEntries) {
    if (!IsValidClientPortalDescriptor(descriptor)
            || maximumEntries == 0) {
        return false;
    }
    std::erase_if(
        entries,
        [&](const ClientPortalDescriptor& entry) {
            return entry.sessionGeneration != descriptor.sessionGeneration
                || entry.guid == descriptor.guid;
        });
    if (entries.size() >= maximumEntries) return false;
    entries.push_back(descriptor);
    return true;
}

inline std::size_t EraseClientPortalGuid(
        std::vector<ClientPortalDescriptor>& entries,
        std::uint32_t guid) noexcept {
    const auto previousSize = entries.size();
    std::erase_if(
        entries,
        [guid](const ClientPortalDescriptor& entry) {
            return entry.guid == guid;
        });
    return previousSize - entries.size();
}

inline ClientPortalLookupResult DecideClientPortalLookup(
        std::span<const ClientPortalDescriptor> entries,
        std::uint64_t sessionGeneration,
        std::uint32_t guid,
        std::uint8_t nativeLowLevelId,
        std::int32_t requestedLevelId,
        bool isDynamicTownPortal,
        bool sessionPoisoned,
        bool fullLevelIdKnown) noexcept {
    if (!isDynamicTownPortal) {
        return {ClientPortalLookupDecision::Original, requestedLevelId};
    }
    if (sessionPoisoned
            || requestedLevelId < 0
            || requestedLevelId > MaximumVanillaNetworkLevelId
            || nativeLowLevelId != requestedLevelId) {
        return {ClientPortalLookupDecision::Refuse, requestedLevelId};
    }
    const auto found = std::find_if(
        entries.begin(),
        entries.end(),
        [&](const ClientPortalDescriptor& descriptor) {
            return descriptor.sessionGeneration == sessionGeneration
                && descriptor.guid == guid;
        });
    if (found == entries.end()) {
        return nativeLowLevelId == 0
            ? ClientPortalLookupResult{
                ClientPortalLookupDecision::Refuse,
                requestedLevelId}
            : ClientPortalLookupResult{
                ClientPortalLookupDecision::Original,
                requestedLevelId};
    }
    if (!IsValidClientPortalDescriptor(*found)
            || found->nativeLowLevelId != nativeLowLevelId
            || !fullLevelIdKnown) {
        return {ClientPortalLookupDecision::Refuse, requestedLevelId};
    }
    return {
        ClientPortalLookupDecision::FullLevelId,
        found->destinationLevelId};
}

constexpr bool IsReciprocalPortalPair(
        const PortalEndpointDescriptor& first,
        const PortalEndpointDescriptor& second) noexcept {
    return IsValidPortalEndpoint(first)
        && IsValidPortalEndpoint(second)
        && first.sessionGeneration == second.sessionGeneration
        && first.gameIdentity == second.gameIdentity
        && first.guid == second.counterpartGuid
        && first.counterpartGuid == second.guid
        && (IsExtendedLevelId(first.destinationLevelId)
            || IsExtendedLevelId(second.destinationLevelId));
}

constexpr std::optional<EncodedLevelCoordinate> EncodeLevelCoordinate(
        std::int32_t levelId,
        std::uint16_t x) noexcept {
    if (levelId <= MaximumVanillaNetworkLevelId
            || levelId > MaximumLevelId
            || (x & ~CoordinateValueMask) != 0) {
        return std::nullopt;
    }
    const auto highLevelId = static_cast<std::uint16_t>(levelId >> 8);
    return EncodedLevelCoordinate{
        .lowLevelId = static_cast<std::uint8_t>(levelId),
        .x = static_cast<std::uint16_t>(
            x
            | CodecMarkerMask
            | (highLevelId << LevelIdPayloadShift)),
    };
}

constexpr std::optional<DecodedLevelCoordinate> DecodeLevelCoordinate(
        std::int32_t lowLevelId,
        std::int32_t encodedX) noexcept {
    if (lowLevelId < 0
            || lowLevelId > MaximumVanillaNetworkLevelId
            || encodedX < 0
            || encodedX > 0xFFFF) {
        return std::nullopt;
    }
    const auto wireX = static_cast<std::uint16_t>(encodedX);
    if ((wireX & CodecMarkerMask) == 0) return std::nullopt;
    const auto highLevelId = static_cast<std::int32_t>(
        (wireX & LevelIdPayloadMask) >> LevelIdPayloadShift);
    if (highLevelId == 0) return std::nullopt;
    const auto levelId = (highLevelId << 8) | lowLevelId;
    if (levelId > MaximumLevelId) return std::nullopt;
    return DecodedLevelCoordinate{
        .levelId = levelId,
        .x = static_cast<std::uint16_t>(wireX & CoordinateValueMask),
    };
}

inline std::optional<std::uint8_t> FindAct(
        std::span<const ActEntry> entries,
        std::int32_t levelId) noexcept {
    const auto found = std::lower_bound(
        entries.begin(),
        entries.end(),
        levelId,
        [](const ActEntry& entry, std::int32_t value) {
            return entry.levelId < value;
        });
    if (found == entries.end()
            || found->levelId != levelId
            || found->act > MaximumAct) {
        return std::nullopt;
    }
    return found->act;
}

inline bool HasValidAnchorActs(std::span<const ActEntry> entries) noexcept {
    constexpr std::array<ActEntry, 5> anchors{{
        {1, 0},
        {40, 1},
        {75, 2},
        {103, 3},
        {109, 4},
    }};
    return std::all_of(
        anchors.begin(),
        anchors.end(),
        [&](const ActEntry& anchor) {
            return FindAct(entries, anchor.levelId) == anchor.act;
        });
}

} // namespace ruffneckk::extended_act_level_ids
