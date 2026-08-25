#pragma once

#include <toml++/toml.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace ruffneckk::cast_triggers {

inline constexpr std::uint8_t PlayerUnitType = 0;
inline constexpr std::uint8_t PlayerModeCast = 10;
inline constexpr std::uint8_t PlayerModeSequence = 18;
inline constexpr std::uint64_t SkillFlagRepeat = 1ULL << 11;
inline constexpr std::int32_t SameLevelMarker = 0;
inline constexpr std::int32_t MinimumSkillId = 0;
inline constexpr std::int32_t MaximumSkillId = 65535;

struct Config {
    bool enabled{true};
    std::vector<std::int32_t> includeSkillIds;
    std::vector<std::int32_t> excludeSkillIds;
    bool diagnostics{};
};

constexpr bool IsCastAnimation(
        std::uint8_t animation,
        std::uint8_t sequenceTransition) noexcept {
    return animation == PlayerModeCast
        || (animation == PlayerModeSequence
            && sequenceTransition == PlayerModeCast);
}

constexpr bool IsEligibleSkillRecord(
        std::uint8_t animation,
        std::uint8_t sequenceTransition,
        std::uint64_t flags) noexcept {
    return IsCastAnimation(animation, sequenceTransition)
        && (flags & SkillFlagRepeat) == 0;
}

inline bool Contains(
        const std::vector<std::int32_t>& values,
        std::int32_t value) noexcept {
    return std::binary_search(values.begin(), values.end(), value);
}

inline bool IsConfiguredSourceSkill(
        const Config& config,
        std::int32_t skillId) noexcept {
    if (!config.includeSkillIds.empty()
            && !Contains(config.includeSkillIds, skillId)) {
        return false;
    }
    return !Contains(config.excludeSkillIds, skillId);
}

constexpr bool IsManualPlayerCast(
        std::int32_t nativeResult,
        std::int32_t consumeResources,
        std::int32_t itemCast,
        std::int32_t itemEffect,
        std::int32_t unitType) noexcept {
    return nativeResult != 0
        && consumeResources == 1
        && itemCast == 0
        && itemEffect == 0
        && unitType == PlayerUnitType;
}

inline std::vector<std::filesystem::path> BuildConfigCandidates(
        const std::vector<std::filesystem::path>& directories,
        const std::filesystem::path& fileName) {
    std::vector<std::filesystem::path> result;
    for (const auto& directory : directories) {
        if (directory.empty()) continue;
        const auto candidate = (directory / fileName).lexically_normal();
        if (std::find(result.begin(), result.end(), candidate) == result.end()) {
            result.emplace_back(candidate);
        }
    }
    return result;
}

inline bool ReadSkillIdArray(
        const toml::table& table,
        const char* key,
        std::vector<std::int32_t>& destination,
        std::string& error) {
    const auto* node = table.get(key);
    if (!node) return true;
    const auto* array = node->as_array();
    if (!array) {
        error = std::string("on_cast.") + key + " must be an integer array";
        return false;
    }
    destination.clear();
    for (const auto& entry : *array) {
        const auto value = entry.value<std::int64_t>();
        if (!value || *value < MinimumSkillId
                || *value > MaximumSkillId) {
            error = std::string("on_cast.") + key
                + " entries must be integers from 0 to 65535";
            return false;
        }
        destination.emplace_back(static_cast<std::int32_t>(*value));
    }
    std::sort(destination.begin(), destination.end());
    destination.erase(
        std::unique(destination.begin(), destination.end()),
        destination.end());
    return true;
}

inline bool ParseToml(
        std::string_view input,
        Config& result,
        std::string& error) {
    try {
        const auto root = toml::parse(input);
        for (const auto& [key, value] : root) {
            (void)value;
            if (key != "enabled" && key != "on_cast"
                    && key != "diagnostics") {
                error = "unknown top-level setting or section: "
                    + std::string(key.str());
                return false;
            }
        }

        Config parsed{};
        if (const auto* enabledNode = root.get("enabled")) {
            const auto enabled = enabledNode->value<bool>();
            if (!enabled) {
                error = "enabled must be true or false";
                return false;
            }
            parsed.enabled = *enabled;
        }

        if (const auto* onCastNode = root.get("on_cast")) {
            const auto* onCast = onCastNode->as_table();
            if (!onCast) {
                error = "on_cast must be a TOML table";
                return false;
            }
            for (const auto& [key, value] : *onCast) {
                (void)value;
                if (key != "include_skill_ids"
                        && key != "exclude_skill_ids") {
                    error = "unknown setting: on_cast."
                        + std::string(key.str());
                    return false;
                }
            }
            if (!ReadSkillIdArray(
                    *onCast,
                    "include_skill_ids",
                    parsed.includeSkillIds,
                    error)
                    || !ReadSkillIdArray(
                        *onCast,
                        "exclude_skill_ids",
                        parsed.excludeSkillIds,
                        error)) {
                return false;
            }
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
                const auto enabled = node->value<bool>();
                if (!enabled) {
                    error = "diagnostics.enabled must be true or false";
                    return false;
                }
                parsed.diagnostics = *enabled;
            }
        }

        result = std::move(parsed);
        error.clear();
        return true;
    } catch (const toml::parse_error& exception) {
        error = exception.description();
        return false;
    }
}

} // namespace ruffneckk::cast_triggers
