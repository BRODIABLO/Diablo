#pragma once

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ruffneckk::advanced_item_tooltips {

enum class PropertyRangeColor {
    ChronicleColor,
    BHDarkGreen,
};

enum class RangeDisplayMode {
    Always,
    HoldShift,
};

inline RangeDisplayMode ParseRangeDisplayMode(std::string_view value) {
    if (value == "Always") return RangeDisplayMode::Always;
    if (value == "HoldShift") return RangeDisplayMode::HoldShift;
    throw std::invalid_argument(
        "rangeDisplayMode must be Always or HoldShift");
}

inline constexpr std::string_view RangeDisplayModeName(RangeDisplayMode value) noexcept {
    return value == RangeDisplayMode::Always
        ? std::string_view{"Always"}
        : std::string_view{"HoldShift"};
}

inline constexpr bool ShouldDisplayRanges(
    RangeDisplayMode mode, bool shiftDown) noexcept {
    return mode == RangeDisplayMode::Always || shiftDown;
}

inline PropertyRangeColor ParsePropertyRangeColor(std::string_view value) {
    if (value == "ChronicleColor") return PropertyRangeColor::ChronicleColor;
    if (value == "BHDarkGreen") return PropertyRangeColor::BHDarkGreen;
    throw std::invalid_argument(
        "propertyRangeColor must be ChronicleColor or BHDarkGreen");
}

inline constexpr std::string_view PropertyRangeColorName(PropertyRangeColor value) noexcept {
    return value == PropertyRangeColor::ChronicleColor
        ? std::string_view{"ChronicleColor"}
        : std::string_view{"BHDarkGreen"};
}

inline constexpr char PropertyRangeColorCode(PropertyRangeColor value) noexcept {
    // Chronicle uses D2R's U color (teal/light blue). BH's legacy dark-green
    // palette entry is ':' and was used by the first plugin releases.
    return value == PropertyRangeColor::ChronicleColor ? 'U' : ':';
}

struct Config {
    bool enabled{true};
    bool showMaxSockets{true};
    bool showMaxSocketsOnSocketedItems{false};
    bool showBaseDefenseRange{true};
    bool showPropertyRanges{true};
    bool includeSocketedContributionsInRanges{false};
    PropertyRangeColor propertyRangeColor{PropertyRangeColor::ChronicleColor};
    RangeDisplayMode rangeDisplayMode{RangeDisplayMode::Always};
};

inline Config ParseConfig(const nlohmann::json& root) {
    if (!root.is_object()) throw std::invalid_argument("configuration root must be an object");

    constexpr std::array allowed{
        "enabled",
        "showMaxSockets",
        "showMaxSocketsOnSocketedItems",
        "showBaseDefenseRange",
        "showPropertyRanges",
        "includeSocketedContributionsInRanges",
        "_rangeDisplayModeHelp",
        "rangeDisplayMode",
        "_propertyRangeColorHelp",
        "propertyRangeColor",
    };
    for (auto entry = root.begin(); entry != root.end(); ++entry) {
        if (std::find(allowed.begin(), allowed.end(), entry.key()) == allowed.end()) {
            throw std::invalid_argument("unknown configuration key: " + entry.key());
        }
        if (entry.key() == "_propertyRangeColorHelp"
            || entry.key() == "_rangeDisplayModeHelp") {
            if (!entry.value().is_string()) {
                throw std::invalid_argument(entry.key() + " must be a string");
            }
            continue;
        }
        if (entry.key() == "propertyRangeColor"
            || entry.key() == "rangeDisplayMode") {
            if (!entry.value().is_string()) {
                throw std::invalid_argument(entry.key() + " must be a string");
            }
            continue;
        }
        if (!entry.value().is_boolean()) {
            throw std::invalid_argument(entry.key() + " must be a boolean");
        }
    }

    Config config;
    config.enabled = root.value("enabled", config.enabled);
    config.showMaxSockets = root.value("showMaxSockets", config.showMaxSockets);
    config.showMaxSocketsOnSocketedItems = root.value(
        "showMaxSocketsOnSocketedItems", config.showMaxSocketsOnSocketedItems);
    config.showBaseDefenseRange = root.value(
        "showBaseDefenseRange", config.showBaseDefenseRange);
    config.showPropertyRanges = root.value("showPropertyRanges", config.showPropertyRanges);
    config.includeSocketedContributionsInRanges = root.value(
        "includeSocketedContributionsInRanges",
        config.includeSocketedContributionsInRanges);
    if (const auto entry = root.find("propertyRangeColor"); entry != root.end()) {
        config.propertyRangeColor = ParsePropertyRangeColor(entry->get<std::string>());
    }
    if (const auto entry = root.find("rangeDisplayMode"); entry != root.end()) {
        config.rangeDisplayMode = ParseRangeDisplayMode(entry->get<std::string>());
    }
    return config;
}

struct LoadResult {
    Config config;
    std::string source{"built-in defaults"};
};

inline std::vector<std::filesystem::path> ConfigCandidates(
    const std::filesystem::path& modDirectory,
    const std::filesystem::path& pluginConfigPath,
    const std::filesystem::path& executableDirectory,
    const std::filesystem::path& fileName
) {
    std::vector<std::filesystem::path> candidates;
    if (!modDirectory.empty())
        candidates.emplace_back(modDirectory / "d2rloader" / "config" / fileName);
    if (!pluginConfigPath.empty())
        candidates.emplace_back(pluginConfigPath.parent_path() / fileName);
    if (!executableDirectory.empty())
        candidates.emplace_back(executableDirectory / "d2rloader" / "config" / fileName);

    std::vector<std::filesystem::path> unique;
    for (const auto& candidate : candidates) {
        const auto normalized = candidate.lexically_normal();
        if (std::find(unique.begin(), unique.end(), normalized) == unique.end())
            unique.push_back(normalized);
    }
    return unique;
}

inline LoadResult LoadConfig(const std::vector<std::filesystem::path>& candidates) {
    for (const auto& path : candidates) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) continue;

        std::ifstream input(path);
        if (!input.is_open()) throw std::runtime_error("cannot open " + path.string());
        try {
            const auto root = nlohmann::json::parse(input, nullptr, true, true);
            return {ParseConfig(root), path.string()};
        } catch (const std::exception& exception) {
            throw std::runtime_error(
                "invalid " + path.string() + " (" + exception.what() + ")");
        }
    }
    return {};
}

} // namespace ruffneckk::advanced_item_tooltips
