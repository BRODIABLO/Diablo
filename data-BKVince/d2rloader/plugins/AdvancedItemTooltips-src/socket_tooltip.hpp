#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace tcp::tooltips {

inline constexpr auto NoSocketLineInsertion = static_cast<std::size_t>(-1);

std::string FormatMaxSocketsLine(unsigned maximumSockets, int currentSockets);

std::size_t FindMaxSocketsInsertion(std::string_view tooltip);

} // namespace tcp::tooltips
