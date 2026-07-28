#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ruffneckk::transmute_hotkey {

struct Hotkey {
    std::uint32_t virtualKey{};
    bool control{};
    bool shift{};
    bool alt{};
};

inline std::string UpperTrim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    std::string result(value.substr(first, last - first + 1));
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return result;
}

inline bool ParseMainKey(const std::string& token, std::uint32_t& virtualKey) {
    if (token.size() == 1) {
        const auto ch = static_cast<unsigned char>(token.front());
        if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
            virtualKey = ch;
            return true;
        }
    }
    if (token.size() >= 2 && token.front() == 'F') {
        unsigned value{};
        for (std::size_t index = 1; index < token.size(); ++index) {
            if (token[index] < '0' || token[index] > '9') return false;
            value = value * 10 + static_cast<unsigned>(token[index] - '0');
        }
        if (value >= 1 && value <= 24) {
            virtualKey = 0x70U + value - 1U;
            return true;
        }
    }
    struct NamedKey {
        std::string_view name;
        std::uint32_t virtualKey;
    };
    constexpr NamedKey namedKeys[]{
        {"SPACE", 0x20}, {"TAB", 0x09}, {"INSERT", 0x2D},
        {"DELETE", 0x2E}, {"HOME", 0x24}, {"END", 0x23},
        {"PAGEUP", 0x21}, {"PAGEDOWN", 0x22},
    };
    for (const auto& key : namedKeys) {
        if (token == key.name) {
            virtualKey = key.virtualKey;
            return true;
        }
    }
    return false;
}

inline bool IsSafeHotkey(const Hotkey& hotkey) noexcept {
    if (hotkey.virtualKey == 0) return false;
    const auto printable = (hotkey.virtualKey >= 'A' && hotkey.virtualKey <= 'Z')
        || (hotkey.virtualKey >= '0' && hotkey.virtualKey <= '9')
        || hotkey.virtualKey == 0x20;
    return !printable || hotkey.control || hotkey.alt;
}

inline bool ParseHotkey(std::string_view text, Hotkey& hotkey) {
    Hotkey parsed{};
    bool hasMainKey{};
    std::size_t begin{};
    while (begin <= text.size()) {
        const auto separator = text.find('+', begin);
        const auto token = UpperTrim(text.substr(
            begin,
            separator == std::string_view::npos
                ? text.size() - begin
                : separator - begin
        ));
        if (token.empty()) return false;
        if (token == "CTRL" || token == "CONTROL") {
            if (parsed.control) return false;
            parsed.control = true;
        } else if (token == "SHIFT") {
            if (parsed.shift) return false;
            parsed.shift = true;
        } else if (token == "ALT") {
            if (parsed.alt) return false;
            parsed.alt = true;
        } else {
            if (hasMainKey || !ParseMainKey(token, parsed.virtualKey)) return false;
            hasMainKey = true;
        }
        if (separator == std::string_view::npos) break;
        begin = separator + 1;
    }
    if (!hasMainKey || !IsSafeHotkey(parsed)) return false;
    hotkey = parsed;
    return true;
}

inline bool ExactModifiersMatch(
    const Hotkey& hotkey,
    bool control,
    bool shift,
    bool alt
) noexcept {
    return hotkey.control == control
        && hotkey.shift == shift
        && hotkey.alt == alt;
}

inline bool IsFreshRequest(
    std::uint64_t now,
    std::uint64_t requestedAt,
    std::uint64_t maximumAge
) noexcept {
    return requestedAt != 0 && now >= requestedAt && now - requestedAt <= maximumAge;
}

} // namespace ruffneckk::transmute_hotkey
