#pragma once

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <compare>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ruffneckk::extended_act_level_ids {

inline constexpr std::size_t LevelsIdOffset = 0x00;
inline constexpr std::size_t LevelsActOffset = 0x0D;
inline constexpr std::uint32_t LevelsRowSize = 0x18C;
inline constexpr std::uint8_t MinimumDataContext = 1;
inline constexpr std::uint8_t MaximumDataContext = 3;
inline constexpr std::uint8_t MaximumAct = 4;

struct Config {
    bool enabled{true};
};

struct ActEntry {
    std::int32_t levelId{};
    std::uint8_t act{};

    auto operator<=>(const ActEntry&) const noexcept = default;
};

inline std::vector<std::filesystem::path> BuildConfigCandidates(
        const std::filesystem::path& activeModConfigDirectory,
        const std::filesystem::path& scopeConfigDirectory,
        const std::filesystem::path& globalConfigDirectory,
        const std::filesystem::path& fileName) {
    std::vector<std::filesystem::path> candidates;
    const auto append = [&](const std::filesystem::path& directory) {
        if (directory.empty()) return;
        const auto candidate = (directory / fileName).lexically_normal();
        if (std::find(candidates.begin(), candidates.end(), candidate)
                == candidates.end()) {
            candidates.emplace_back(candidate);
        }
    };
    append(activeModConfigDirectory);
    append(scopeConfigDirectory);
    append(globalConfigDirectory);
    return candidates;
}

inline bool ParseConfig(
        std::string_view text,
        Config& output,
        std::string& error) noexcept {
    output = {};
    error.clear();
    try {
        const auto json = nlohmann::json::parse(
            text.begin(), text.end(), nullptr, true, false);
        if (!json.is_object()) {
            error = "configuration root must be an object";
            return false;
        }
        for (const auto& [key, value] : json.items()) {
            (void)value;
            if (key != "enabled") {
                error = "unknown setting: " + key;
                return false;
            }
        }
        if (json.contains("enabled")) {
            if (!json.at("enabled").is_boolean()) {
                error = "enabled must be a boolean";
                return false;
            }
            output.enabled = json.at("enabled").get<bool>();
        }
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    } catch (...) {
        error = "unknown configuration error";
        return false;
    }
}

constexpr bool IsSupportedDataContext(std::uint8_t dataContext) noexcept {
    return dataContext >= MinimumDataContext
        && dataContext <= MaximumDataContext;
}

inline std::optional<std::uint8_t> FindAct(
        std::span<const ActEntry> entries,
        std::int32_t levelId) noexcept {
    const auto found = std::lower_bound(
        entries.begin(),
        entries.end(),
        levelId,
        [](const ActEntry& entry, std::int32_t value) {
            return entry.levelId < value;
        });
    if (found == entries.end()
            || found->levelId != levelId
            || found->act > MaximumAct) {
        return std::nullopt;
    }
    return found->act;
}

inline bool HasValidAnchorActs(std::span<const ActEntry> entries) noexcept {
    constexpr std::array<ActEntry, 5> anchors{{
        {1, 0},
        {40, 1},
        {75, 2},
        {103, 3},
        {109, 4},
    }};
    return std::all_of(
        anchors.begin(),
        anchors.end(),
        [&](const ActEntry& anchor) {
            return FindAct(entries, anchor.levelId) == anchor.act;
        });
}

} // namespace ruffneckk::extended_act_level_ids
