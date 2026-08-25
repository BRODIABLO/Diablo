#pragma once

#include <toml++/toml.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ruffneckk::player_sequence_tables {

inline constexpr std::size_t SequenceCount = 25;
inline constexpr std::size_t WeaponClassCount = 14;
inline constexpr std::size_t RouteCount = SequenceCount * WeaponClassCount;
inline constexpr std::size_t MaximumRecordsPerSet = 255;
inline constexpr std::size_t MaximumTableBytes = 1024 * 1024;

inline constexpr std::array<std::string_view, WeaponClassCount> WeaponClasses{
    "HTH", "1HT", "2HT", "1HS", "2HS", "BOW", "XBW",
    "STF", "1JS", "1JT", "1SS", "1ST", "HT1", "HT2",
};

inline constexpr std::array<std::string_view, 20> PlayerModes{
    "DT", "NU", "WL", "RN", "GH", "TN", "TW", "A1", "A2", "BL",
    "SC", "TH", "KK", "S1", "S2", "S3", "S4", "DD", "SQ", "KB",
};

struct Config {
    bool enabled{true};
    bool diagnostics{};
};

struct SequenceRecordInput {
    std::uint8_t mode{};
    std::uint8_t frame{};
    std::uint8_t direction{};
    std::uint8_t event{};
};

struct RecordSet {
    std::string name;
    std::vector<SequenceRecordInput> records;
};

struct ParsedTables {
    std::array<std::int16_t, RouteCount> recordSetByRoute{};
    std::vector<RecordSet> recordSets;
    std::size_t availableRoutes{};
};

inline bool ParseToml(
        std::string_view input,
        Config& result,
        std::string& error) {
    try {
        const auto root = toml::parse(input);
        for (const auto& [key, value] : root) {
            (void)value;
            if (key != "enabled" && key != "diagnostics") {
                error = "unknown top-level setting or section: "
                    + std::string(key.str());
                return false;
            }
        }

        Config parsed{};
        if (const auto* node = root.get("enabled")) {
            const auto value = node->value<bool>();
            if (!value) {
                error = "enabled must be true or false";
                return false;
            }
            parsed.enabled = *value;
        }
        if (const auto* diagnosticsNode = root.get("diagnostics")) {
            const auto* diagnostics = diagnosticsNode->as_table();
            if (!diagnostics) {
                error = "diagnostics must be a TOML table";
                return false;
            }
            for (const auto& [key, value] : *diagnostics) {
                (void)value;
                if (key != "enabled") {
                    error = "unknown setting: diagnostics."
                        + std::string(key.str());
                    return false;
                }
            }
            if (const auto* node = diagnostics->get("enabled")) {
                const auto value = node->value<bool>();
                if (!value) {
                    error = "diagnostics.enabled must be true or false";
                    return false;
                }
                parsed.diagnostics = *value;
            }
        }
        result = parsed;
        error.clear();
        return true;
    } catch (const toml::parse_error& exception) {
        error = exception.description();
        return false;
    }
}

namespace detail {

inline bool SplitLines(
        std::string_view input,
        std::vector<std::string_view>& lines,
        std::string& error) {
    lines.clear();
    if (input.empty()) {
        error = "table is empty";
        return false;
    }
    if (input.size() >= 3
            && static_cast<unsigned char>(input[0]) == 0xEF
            && static_cast<unsigned char>(input[1]) == 0xBB
            && static_cast<unsigned char>(input[2]) == 0xBF) {
        error = "UTF-8 BOM is not supported";
        return false;
    }
    if (input.find('\0') != std::string_view::npos) {
        error = "NUL bytes are not supported";
        return false;
    }
    std::size_t start{};
    while (start < input.size()) {
        const auto newline = input.find('\n', start);
        const auto end = newline == std::string_view::npos ? input.size() : newline;
        auto line = input.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        } else if (line.find('\r') != std::string_view::npos) {
            error = "lone carriage return is not supported";
            return false;
        }
        if (line.empty()) {
            error = "blank lines are not supported";
            return false;
        }
        lines.emplace_back(line);
        if (newline == std::string_view::npos) break;
        start = newline + 1;
    }
    return !lines.empty();
}

inline std::vector<std::string_view> SplitTabs(std::string_view line) {
    std::vector<std::string_view> fields;
    std::size_t start{};
    for (;;) {
        const auto tab = line.find('\t', start);
        fields.emplace_back(line.substr(
            start,
            tab == std::string_view::npos ? line.size() - start : tab - start));
        if (tab == std::string_view::npos) break;
        start = tab + 1;
    }
    return fields;
}

inline bool HeaderEquals(
        const std::vector<std::string_view>& fields,
        std::initializer_list<std::string_view> expected) {
    return fields.size() == expected.size()
        && std::equal(fields.begin(), fields.end(), expected.begin());
}

inline bool ParseUnsigned(
        std::string_view text,
        std::uint32_t maximum,
        std::uint32_t& result) {
    if (text.empty()) return false;
    std::uint32_t value{};
    const auto [end, status] = std::from_chars(
        text.data(), text.data() + text.size(), value);
    if (status != std::errc{} || end != text.data() + text.size()
            || value > maximum) {
        return false;
    }
    result = value;
    return true;
}

inline bool IsRecordSetName(std::string_view name) {
    if (name.empty() || name.size() > 64) return false;
    const auto isAlpha = [](char character) {
        return (character >= 'A' && character <= 'Z')
            || (character >= 'a' && character <= 'z');
    };
    const auto isDigit = [](char character) {
        return character >= '0' && character <= '9';
    };
    if (!isAlpha(name.front())) return false;
    for (const char character : name) {
        if (!isAlpha(character) && !isDigit(character) && character != '_') {
            return false;
        }
    }
    return true;
}

inline int WeaponClassIndex(std::string_view code) {
    for (std::size_t index = 0; index < WeaponClasses.size(); ++index) {
        if (WeaponClasses[index] == code) return static_cast<int>(index);
    }
    return -1;
}

inline int PlayerModeIndex(std::string_view code) {
    for (std::size_t index = 0; index < PlayerModes.size(); ++index) {
        if (PlayerModes[index] == code) return static_cast<int>(index);
    }
    return -1;
}

inline bool FailLine(
        std::string_view table,
        std::size_t line,
        std::string_view message,
        std::string& error) {
    error = std::string(table) + " line " + std::to_string(line)
        + ": " + std::string(message);
    return false;
}

} // namespace detail

inline bool ParsePlayerSequenceTables(
        std::string_view routeText,
        std::string_view recordText,
        ParsedTables& result,
        std::string& error) {
    if (routeText.size() > MaximumTableBytes
            || recordText.size() > MaximumTableBytes) {
        error = "table exceeds the 1 MiB safety limit";
        return false;
    }

    std::vector<std::string_view> routeLines;
    std::vector<std::string_view> recordLines;
    if (!detail::SplitLines(routeText, routeLines, error)) {
        error = "playerseqmap.txt: " + error;
        return false;
    }
    if (!detail::SplitLines(recordText, recordLines, error)) {
        error = "playerseq.txt: " + error;
        return false;
    }
    if (!detail::HeaderEquals(
            detail::SplitTabs(routeLines.front()),
            {"seqnum", "*sequence", "weaponclass", "recordset", "*eol"})) {
        error = "playerseqmap.txt: unexpected header";
        return false;
    }
    if (!detail::HeaderEquals(
            detail::SplitTabs(recordLines.front()),
            {"recordset", "mode", "frame", "dir", "event", "*eol"})) {
        error = "playerseq.txt: unexpected header";
        return false;
    }
    if (routeLines.size() != RouteCount + 1) {
        error = "playerseqmap.txt must contain exactly 350 data rows";
        return false;
    }

    std::array<std::string, RouteCount> routeNames;
    std::array<bool, RouteCount> seen{};
    std::size_t availableRoutes{};
    for (std::size_t row = 1; row < routeLines.size(); ++row) {
        const auto fields = detail::SplitTabs(routeLines[row]);
        if (fields.size() != 5) {
            return detail::FailLine(
                "playerseqmap.txt", row + 1, "expected 5 fields", error);
        }
        std::uint32_t sequence{};
        if (!detail::ParseUnsigned(fields[0], SequenceCount, sequence)
                || sequence == 0) {
            return detail::FailLine(
                "playerseqmap.txt", row + 1, "seqnum must be from 1 to 25", error);
        }
        const auto weaponIndex = detail::WeaponClassIndex(fields[2]);
        if (weaponIndex < 0) {
            return detail::FailLine(
                "playerseqmap.txt", row + 1, "unknown weaponclass", error);
        }
        if (fields[4] != "0") {
            return detail::FailLine(
                "playerseqmap.txt", row + 1, "*eol must be 0", error);
        }
        if (!fields[3].empty() && !detail::IsRecordSetName(fields[3])) {
            return detail::FailLine(
                "playerseqmap.txt", row + 1, "invalid recordset name", error);
        }
        const auto routeIndex = (sequence - 1) * WeaponClassCount
            + static_cast<std::size_t>(weaponIndex);
        if (seen[routeIndex]) {
            return detail::FailLine(
                "playerseqmap.txt", row + 1, "duplicate seqnum/weaponclass route", error);
        }
        seen[routeIndex] = true;
        routeNames[routeIndex] = std::string(fields[3]);
        if (!fields[3].empty()) ++availableRoutes;
    }
    if (!std::all_of(seen.begin(), seen.end(), [](bool value) { return value; })) {
        error = "playerseqmap.txt does not cover all 350 routes";
        return false;
    }

    std::vector<RecordSet> recordSets;
    std::unordered_map<std::string, std::size_t> recordSetIndexes;
    for (std::size_t row = 1; row < recordLines.size(); ++row) {
        const auto fields = detail::SplitTabs(recordLines[row]);
        if (fields.size() != 6) {
            return detail::FailLine(
                "playerseq.txt", row + 1, "expected 6 fields", error);
        }
        if (!detail::IsRecordSetName(fields[0])) {
            return detail::FailLine(
                "playerseq.txt", row + 1, "invalid recordset name", error);
        }
        const auto mode = detail::PlayerModeIndex(fields[1]);
        if (mode < 0) {
            return detail::FailLine(
                "playerseq.txt", row + 1, "unknown player mode", error);
        }
        std::uint32_t frame{};
        std::uint32_t direction{};
        std::uint32_t event{};
        if (!detail::ParseUnsigned(fields[2], 255, frame)) {
            return detail::FailLine(
                "playerseq.txt", row + 1, "frame must be from 0 to 255", error);
        }
        if (!detail::ParseUnsigned(fields[3], 255, direction)) {
            return detail::FailLine(
                "playerseq.txt", row + 1, "dir must be from 0 to 255", error);
        }
        if (!detail::ParseUnsigned(fields[4], 4, event)) {
            return detail::FailLine(
                "playerseq.txt", row + 1, "event must be from 0 to 4", error);
        }
        if (fields[5] != "0") {
            return detail::FailLine(
                "playerseq.txt", row + 1, "*eol must be 0", error);
        }

        const std::string name(fields[0]);
        auto [entry, inserted] = recordSetIndexes.emplace(name, recordSets.size());
        if (inserted) recordSets.push_back(RecordSet{name, {}});
        auto& records = recordSets[entry->second].records;
        if (records.size() >= MaximumRecordsPerSet) {
            return detail::FailLine(
                "playerseq.txt", row + 1, "recordset exceeds 255 records", error);
        }
        records.push_back(SequenceRecordInput{
            static_cast<std::uint8_t>(mode),
            static_cast<std::uint8_t>(frame),
            static_cast<std::uint8_t>(direction),
            static_cast<std::uint8_t>(event),
        });
    }

    ParsedTables parsed{};
    parsed.recordSetByRoute.fill(-1);
    std::unordered_set<std::size_t> referenced;
    for (std::size_t index = 0; index < routeNames.size(); ++index) {
        if (routeNames[index].empty()) continue;
        const auto recordSet = recordSetIndexes.find(routeNames[index]);
        if (recordSet == recordSetIndexes.end()) {
            error = "playerseqmap.txt references undefined recordset: " + routeNames[index];
            return false;
        }
        if (recordSet->second > static_cast<std::size_t>(
                std::numeric_limits<std::int16_t>::max())) {
            error = "too many recordsets";
            return false;
        }
        parsed.recordSetByRoute[index] = static_cast<std::int16_t>(recordSet->second);
        referenced.insert(recordSet->second);
    }
    if (referenced.size() != recordSets.size()) {
        for (std::size_t index = 0; index < recordSets.size(); ++index) {
            if (!referenced.contains(index)) {
                error = "playerseq.txt defines unused recordset: " + recordSets[index].name;
                return false;
            }
        }
    }

    parsed.recordSets = std::move(recordSets);
    parsed.availableRoutes = availableRoutes;
    result = std::move(parsed);
    error.clear();
    return true;
}

} // namespace ruffneckk::player_sequence_tables
