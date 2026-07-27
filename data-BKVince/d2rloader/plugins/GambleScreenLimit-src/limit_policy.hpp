#pragma once

#include <cstdint>

namespace tcp::gamble_screen_limit {

inline constexpr std::uint32_t VanillaLimit = 14;
inline constexpr std::uint32_t ExpandedLimit = 32;

inline constexpr std::uint32_t EffectiveLimit(bool enabled) noexcept {
    return enabled ? ExpandedLimit : VanillaLimit;
}

} // namespace tcp::gamble_screen_limit
