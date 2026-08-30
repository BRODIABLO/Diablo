#pragma once

#include "isc12_schema.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace ruffneckk::isc12 {

inline constexpr std::uint16_t PlayerStatSectionMarker = 0x6667;
inline constexpr std::size_t MaximumPlayerStatEntries = 512;

enum class PlayerStatStreamKind : std::uint8_t {
    Auxiliary,
    Regular,
};

enum class PlayerStatPreflightError : std::uint8_t {
    None,
    InvalidArgument,
    InvalidMarker,
    UnsafeSchema,
    InvalidStatId,
    UnserializedStat,
    TooManyEntries,
    Truncated,
    MissingSentinel,
};

struct PlayerStatPreflightResult {
    std::size_t consumedBits{};
    std::size_t entryCount{};
};

// Validates one complete 0x6667 player-stat section without allocating or
// publishing decoded state. Bits use the native least-significant-bit-first
// order. Trailing bytes are allowed because the section is embedded in larger
// D2S/D2I payloads. On failure, output remains unchanged.
auto PreflightPlayerStatStream(
    std::span<const std::uint8_t> bytes,
    std::span<const ItemStatSemanticRow> schema,
    PlayerStatStreamKind kind,
    PlayerStatPreflightResult& output) noexcept -> PlayerStatPreflightError;

} // namespace ruffneckk::isc12
