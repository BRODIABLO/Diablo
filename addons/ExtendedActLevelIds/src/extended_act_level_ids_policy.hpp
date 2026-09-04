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
