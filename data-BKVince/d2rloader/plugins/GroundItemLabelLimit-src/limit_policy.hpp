#pragma once

#include <cstdint>

namespace ruffneckk::ground_item_label_limit {

inline constexpr std::uint32_t VanillaLimit = 32;
inline constexpr std::uint32_t DefaultLimit = 64;
inline constexpr std::uint32_t ExpandedLimit = 128;
inline constexpr std::uint32_t LabelEntrySize = 0x144;

inline constexpr bool IsSupportedLimit(std::uint32_t value) noexcept {
    return value == DefaultLimit || value == ExpandedLimit;
}

inline constexpr std::uint32_t LabelArrayByteOffset(std::uint32_t limit) noexcept {
    return limit * LabelEntrySize;
}

} // namespace ruffneckk::ground_item_label_limit
