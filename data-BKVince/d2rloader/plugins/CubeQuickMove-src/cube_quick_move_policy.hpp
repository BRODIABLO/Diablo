#pragma once

#include <cstdint>

namespace ruffneckk::cube_quick_move {

inline constexpr std::uint8_t CubePage = 3;

constexpr bool ShouldRecomputeBottomRight(
    bool enabled,
    std::int32_t vanillaResult,
    std::uint8_t page,
    std::uint8_t width,
    std::uint8_t height
) noexcept {
    return enabled
        && vanillaResult != 0
        && page == CubePage
        && width != 0
        && height > 1;
}

} // namespace ruffneckk::cube_quick_move
