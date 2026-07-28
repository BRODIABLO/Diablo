#pragma once

#include <array>
#include <cstdint>

namespace ruffneckk::equipped_item_to_cube {

inline constexpr std::uint64_t EligibilityGuardBranchRva = 0x228B98;

inline constexpr std::array<std::uint8_t, 6> EligibilityGuardBranchExpected{
    0x0F, 0x84, 0x55, 0x01, 0x00, 0x00,
};

inline constexpr std::array<std::uint8_t, 6> EligibilityGuardBranchReplacement{
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
};

constexpr bool ShouldInstallPatch(bool enabled) noexcept {
    return enabled;
}

} // namespace ruffneckk::equipped_item_to_cube
