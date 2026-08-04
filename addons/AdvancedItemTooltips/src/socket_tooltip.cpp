#include "socket_tooltip.hpp"

#include <array>
#include <string>

namespace tcp::tooltips {
namespace {

bool IsPrimaryItemStatLine(std::string_view line) {
    constexpr std::array patterns{
        std::string_view{"Damage:"},
        std::string_view{"DAMAGE:"},
        std::string_view{"Defense:"},
        std::string_view{"DEFENSE:"},
        std::string_view{"D\xC3\xA9" "g\xC3\xA2" "ts :"},
        std::string_view{"D\xC3\x89" "G\xC3\x82" "TS :"},
        std::string_view{"D\xC3\xA9" "fense :"},
        std::string_view{"D\xC3\x89" "FENSE :"},
    };
    for (const auto pattern : patterns) {
        if (line.find(pattern) != std::string_view::npos) return true;
    }
    return false;
}

} // namespace

std::string FormatMaxSocketsLine(unsigned maximumSockets, int) {
    if (maximumSockets == 0) return {};

    std::string result;
    result.reserve(32);
    result += "\xEE\x81\xBE" "0Max Sockets: ";
    result += std::to_string(maximumSockets);
    return result;
}

std::size_t FindMaxSocketsInsertion(std::string_view tooltip) {
    std::size_t start{};
    while (start < tooltip.size()) {
        const auto end = tooltip.find('\n', start);
        const auto lineEnd = end == std::string_view::npos ? tooltip.size() : end;
        if (IsPrimaryItemStatLine(tooltip.substr(start, lineEnd - start))) {
            return start;
        }
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return NoSocketLineInsertion;
}

} // namespace tcp::tooltips
