#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace RuffnecKk::MapSense {

inline constexpr std::uint16_t ExternalAtlasGeometryProtocolVersion = 1U;
inline constexpr std::uint16_t ExternalAtlasStandardCampaignFlag = 1U;
inline constexpr std::size_t ExternalAtlasGeometryHeaderBytes = 32U;
inline constexpr std::size_t ExternalAtlasGeometryMaximumBytes =
    16U * 1'024U * 1'024U;
inline constexpr std::size_t ExternalAtlasGeometryMaximumLevels = 512U;
inline constexpr std::size_t ExternalAtlasGeometryMaximumCells = 1'000'000U;

struct ExternalAtlasGeometryCell final {
    std::int32_t frame{};
    std::int32_t tileX{};
    std::int32_t tileY{};
    bool wall{};
};

struct ExternalAtlasGeometryLevel final {
    std::int32_t levelId{};
    std::uint8_t layer{};
    std::uint32_t firstCell{};
    std::uint32_t cellCount{};
};

struct ExternalAtlasGeometry final {
    std::uint32_t seed{};
    std::uint8_t difficulty{};
    std::uint8_t act{};
    std::uint16_t flags{};
    std::uint64_t digest{};
    std::vector<ExternalAtlasGeometryLevel> levels;
    std::vector<ExternalAtlasGeometryCell> cells;
};

enum class ExternalAtlasGeometryParseError : std::uint8_t {
    None,
    InvalidRequest,
    InvalidLength,
    InvalidMagic,
    UnsupportedVersion,
    UnsupportedFlags,
    IdentityMismatch,
    InvalidReservedBytes,
    InvalidCount,
    InvalidLevel,
    InvalidCell,
    DigestMismatch,
};

// Parses one bounded MSA1 artifact and validates its complete identity and
// semantic digest before publishing any geometry. Output remains empty on
// failure, so a corrupt or stale cache entry can never partially reach the
// renderer.
[[nodiscard]] auto ParseExternalAtlasGeometry(
    std::span<const std::uint8_t> bytes,
    std::uint32_t expectedSeed,
    std::uint8_t expectedDifficulty,
    std::uint8_t expectedAct,
    ExternalAtlasGeometry& output,
    ExternalAtlasGeometryParseError* error = nullptr) -> bool;

} // namespace RuffnecKk::MapSense
