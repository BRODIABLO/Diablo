#pragma once

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace ruffneckk::advanced_item_tooltips {

struct Config {
    bool enabled{true};
    bool showMaxSockets{true};
    bool showMaxSocketsOnSocketedItems{false};
    bool showBaseDefenseRange{true};
    bool showPropertyRanges{true};
    bool includeSocketedContributionsInRanges{false};
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
    };
    for (auto entry = root.begin(); entry != root.end(); ++entry) {
        if (std::find(allowed.begin(), allowed.end(), entry.key()) == allowed.end()) {
            throw std::invalid_argument("unknown configuration key: " + entry.key());
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
