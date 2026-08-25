#pragma once

#include <toml++/toml.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace ruffneckk::burn_damage_fix {

inline constexpr std::int64_t ConfigVersion = 1;

struct Config {
    bool enabled{true};
    bool normalizeGenericBurn{true};
    bool applyFireResistance{true};
    bool diagnostics{false};
};

inline auto IsSupportedBuild(std::string_view build) noexcept -> bool {
    return build == "92777" || build == "93847";
}

inline auto ShouldResolveBurn(
        const Config& config,
        std::int32_t burnDamage,
        std::int32_t burnLength) noexcept -> bool {
    return config.enabled && config.applyFireResistance
        && burnDamage > 0 && burnLength > 0;
}

inline auto ShouldWitnessBurningState(
        const Config& config,
        std::int32_t resolvedBurnDamage,
        std::int32_t burnLength) noexcept -> bool {
    return config.enabled && config.diagnostics
        && resolvedBurnDamage > 0 && burnLength > 0;
}

inline auto CalculatePercentage(
        std::int32_t value,
        std::int32_t percentage) noexcept -> std::int64_t {
    return static_cast<std::int64_t>(value)
        * static_cast<std::int64_t>(percentage) / 100;
}

inline auto NormalizeGenericNumerator(
        std::int32_t scaledExistingBurn,
        std::int32_t burningMin,
        std::int32_t burningMax,
        std::int32_t fireMastery,
        std::uint32_t advancedRandom) noexcept -> std::int32_t {
    std::int64_t addedBurn{};
    if (burningMin > 0 && burningMax > 0) {
        if (burningMin > burningMax) std::swap(burningMin, burningMax);
        auto minimum = static_cast<std::int64_t>(burningMin)
            + CalculatePercentage(burningMin, fireMastery);
        auto maximum = static_cast<std::int64_t>(burningMax)
            + CalculatePercentage(burningMax, fireMastery);
        minimum = std::clamp<std::int64_t>(
            minimum, 0, std::numeric_limits<std::int32_t>::max());
        maximum = std::clamp<std::int64_t>(
            maximum, 0, std::numeric_limits<std::int32_t>::max());
        if (minimum > maximum) std::swap(minimum, maximum);
        addedBurn = minimum;
        const auto range = static_cast<std::uint32_t>(maximum - minimum);
        if (range != 0) {
            const auto offset = (range & (range - 1U)) == 0
                ? advancedRandom & (range - 1U)
                : advancedRandom % range;
            addedBurn += offset;
        }
    }

    const auto result = static_cast<std::int64_t>(scaledExistingBurn) + addedBurn;
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(
        result, 0, std::numeric_limits<std::int32_t>::max()));
}

inline auto ParseToml(
        std::string_view input,
        Config& result,
        std::string& error) -> bool {
    try {
        const auto root = toml::parse(input);
        for (const auto& [key, value] : root) {
            (void)value;
            if (key != "config_version" && key != "enabled"
                    && key != "fixes" && key != "diagnostics") {
                error = "unknown top-level setting or section: "
                    + std::string(key.str());
                return false;
            }
        }

        const auto* versionNode = root.get("config_version");
        const auto* enabledNode = root.get("enabled");
        const auto* fixesNode = root.get("fixes");
        const auto* diagnosticsNode = root.get("diagnostics");
        if (!versionNode || !versionNode->is_integer()
                || versionNode->value<std::int64_t>() != ConfigVersion) {
            error = "config_version must be integer 1";
            return false;
        }
        if (!enabledNode || !enabledNode->is_boolean()) {
            error = "enabled must be a boolean";
            return false;
        }
        const auto* fixes = fixesNode ? fixesNode->as_table() : nullptr;
        const auto* diagnostics = diagnosticsNode
            ? diagnosticsNode->as_table() : nullptr;
        if (!fixes) {
            error = "fixes must be a table";
            return false;
        }
        if (!diagnostics) {
            error = "diagnostics must be a table";
            return false;
        }
        for (const auto& [key, value] : *fixes) {
            (void)value;
            if (key != "normalize_generic_burn"
                    && key != "apply_fire_resistance") {
                error = "unknown fixes setting: " + std::string(key.str());
                return false;
            }
        }
        for (const auto& [key, value] : *diagnostics) {
            (void)value;
            if (key != "enabled") {
                error = "unknown diagnostics setting: " + std::string(key.str());
                return false;
            }
        }

        const auto* normalizeNode = fixes->get("normalize_generic_burn");
        const auto* resistanceNode = fixes->get("apply_fire_resistance");
        const auto* diagnosticsEnabledNode = diagnostics->get("enabled");
        if (!normalizeNode || !normalizeNode->is_boolean()) {
            error = "fixes.normalize_generic_burn must be a boolean";
            return false;
        }
        if (!resistanceNode || !resistanceNode->is_boolean()) {
            error = "fixes.apply_fire_resistance must be a boolean";
            return false;
        }
        if (!diagnosticsEnabledNode || !diagnosticsEnabledNode->is_boolean()) {
            error = "diagnostics.enabled must be a boolean";
            return false;
        }

        Config parsed{};
        parsed.enabled = *enabledNode->value<bool>();
        parsed.normalizeGenericBurn = *normalizeNode->value<bool>();
        parsed.applyFireResistance = *resistanceNode->value<bool>();
        parsed.diagnostics = *diagnosticsEnabledNode->value<bool>();
        result = parsed;
        error.clear();
        return true;
    } catch (const toml::parse_error& exception) {
        error = exception.description();
        return false;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}

inline auto BuildConfigCandidates(
        const std::filesystem::path& activeModDirectory,
        const std::filesystem::path& scopeDirectory,
        const std::filesystem::path& globalDirectory,
        const std::filesystem::path& fileName)
        -> std::vector<std::filesystem::path> {
    std::vector<std::filesystem::path> result;
    for (const auto& directory : {
            activeModDirectory, scopeDirectory, globalDirectory}) {
        if (directory.empty()) continue;
        const auto candidate = directory / fileName;
        if (std::find(result.begin(), result.end(), candidate) == result.end()) {
            result.push_back(candidate);
        }
    }
    return result;
}

inline auto CanEncodeRel32(
        std::uintptr_t instruction,
        std::uintptr_t target) noexcept -> bool {
    const auto next = static_cast<std::int64_t>(instruction) + 5;
    const auto displacement = static_cast<std::int64_t>(target) - next;
    return displacement >= std::numeric_limits<std::int32_t>::min()
        && displacement <= std::numeric_limits<std::int32_t>::max();
}

} // namespace ruffneckk::burn_damage_fix
