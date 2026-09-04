#pragma once

#include <toml++/toml.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace ruffneckk::revive_overhaul {

inline constexpr std::int32_t NativeReviveState = 96;
inline constexpr std::int32_t NativeScatterMaximumDistance = 1;
inline constexpr std::int32_t NativeCatchUpDistance = 20;
inline constexpr std::int32_t ForcedCatchUpDistance =
    NativeCatchUpDistance + 1;
inline constexpr std::int32_t NativeVelocityBonus = 40;
inline constexpr std::uint16_t ChampionUniqueSuperUniqueMask = 0x000E;
inline constexpr std::uint8_t AuraEnchantedUMod = 30;
inline constexpr std::size_t MaximumUModCount = 9;

struct AiPolicy {
    bool enabled{true};
    bool disableOwnerScatter{true};
    std::int32_t catchUpDistance{8};
    std::int32_t followDistance{4};
    std::int32_t velocityBonus{80};
};

struct RevivePolicy {
    bool allowHighRankMonsters{true};
    bool preserveNativeAuras{true};
};

struct Config {
    bool enabled{true};
    AiPolicy ai{};
    RevivePolicy revive{};
    bool diagnostics{};
};

constexpr bool NeedsReviveTargetValidator(
        const RevivePolicy& policy) noexcept {
    return policy.allowHighRankMonsters || policy.preserveNativeAuras;
}

constexpr bool IsPeacefulReviveTick(
        bool hasTarget,
        bool inCombat) noexcept {
    return !hasTarget && !inCombat;
}

constexpr std::int32_t TransformLeashDistance(
        std::int32_t distance,
        const AiPolicy& policy) noexcept {
    if (!policy.enabled || distance < 0) return distance;
    if (policy.disableOwnerScatter
            && distance <= NativeScatterMaximumDistance) {
        return NativeScatterMaximumDistance + 1;
    }
    if (distance > policy.catchUpDistance
            && distance <= NativeCatchUpDistance) {
        return ForcedCatchUpDistance;
    }
    return distance;
}

constexpr std::uint8_t TransformFollowDistance(
        std::uint8_t distance,
        const AiPolicy& policy) noexcept {
    return policy.enabled
        ? static_cast<std::uint8_t>(policy.followDistance)
        : distance;
}

constexpr std::uint8_t TransformVelocityBonus(
        std::uint8_t bonus,
        const AiPolicy& policy) noexcept {
    return policy.enabled
        ? static_cast<std::uint8_t>(policy.velocityBonus)
        : bonus;
}

inline bool HasAuraEnchantedUMod(const std::uint8_t* uMods) noexcept {
    if (!uMods) return false;
    for (std::size_t index = 0; index < MaximumUModCount; ++index) {
        if (uMods[index] == 0) return false;
        if (uMods[index] == AuraEnchantedUMod) return true;
    }
    return false;
}

inline std::vector<std::filesystem::path> BuildConfigCandidates(
        const std::vector<std::filesystem::path>& directories,
        const std::filesystem::path& primaryFileName,
        const std::filesystem::path& legacyFileName) {
    std::vector<std::filesystem::path> candidates;
    const auto append = [&candidates](const std::filesystem::path& path) {
        if (path.empty()) return;
        const auto normalized = path.lexically_normal();
        if (std::find(candidates.begin(), candidates.end(), normalized)
                == candidates.end()) {
            candidates.emplace_back(normalized);
        }
    };
    for (const auto& directory : directories) {
        if (directory.empty()) continue;
        append(directory / primaryFileName);
        append(directory / legacyFileName);
    }
    return candidates;
}

inline bool ParseToml(
        std::string_view input,
        Config& result,
        std::string& error) {
    try {
        const auto root = toml::parse(input);
        for (const auto& [key, value] : root) {
            (void)value;
            if (key != "enabled"
                    && key != "ai"
                    && key != "revive"
                    && key != "diagnostics") {
                error = "unknown top-level setting or section: "
                    + std::string(key.str());
                return false;
            }
        }

        Config parsed{};
        const auto readRootBool = [&](const char* key, bool& destination) {
            const auto* node = root.get(key);
            if (!node) return true;
            if (!node->is_boolean()) {
                error = std::string(key) + " must be true or false";
                return false;
            }
            const auto value = node->value<bool>();
            if (!value) {
                error = std::string(key) + " must be true or false";
                return false;
            }
            destination = *value;
            return true;
        };
        if (!readRootBool("enabled", parsed.enabled)) return false;

        if (const auto* aiNode = root.get("ai")) {
            const auto* ai = aiNode->as_table();
            if (!ai) {
                error = "ai must be a TOML table";
                return false;
            }
            constexpr std::string_view allowedKeys[]{
                "enabled",
                "disable_owner_scatter",
                "catch_up_distance",
                "follow_distance",
                "velocity_bonus",
            };
            for (const auto& [key, value] : *ai) {
                (void)value;
                if (std::find(
                        std::begin(allowedKeys),
                        std::end(allowedKeys),
                        key.str()) == std::end(allowedKeys)) {
                    error = "unknown setting: ai." + std::string(key.str());
                    return false;
                }
            }
            const auto readBool = [&](const char* key, bool& destination) {
                const auto* node = ai->get(key);
                if (!node) return true;
                if (!node->is_boolean()) {
                    error = std::string("ai.") + key
                        + " must be true or false";
                    return false;
                }
                const auto value = node->value<bool>();
                if (!value) {
                    error = std::string("ai.") + key
                        + " must be true or false";
                    return false;
                }
                destination = *value;
                return true;
            };
            const auto readInt = [&error, ai](
                    const char* key,
                    std::int32_t minimum,
                    std::int32_t maximum,
                    std::int32_t& destination) {
                const auto* node = ai->get(key);
                if (!node) return true;
                const auto value = node->value<std::int64_t>();
                if (!value || *value < minimum || *value > maximum) {
                    error = std::string("ai.") + key
                        + " must be an integer from "
                        + std::to_string(minimum) + " to "
                        + std::to_string(maximum);
                    return false;
                }
                destination = static_cast<std::int32_t>(*value);
                return true;
            };
            if (!readBool("enabled", parsed.ai.enabled)
                    || !readBool(
                        "disable_owner_scatter",
                        parsed.ai.disableOwnerScatter)
                    || !readInt(
                        "catch_up_distance",
                        2,
                        NativeCatchUpDistance,
                        parsed.ai.catchUpDistance)
                    || !readInt(
                        "follow_distance",
                        1,
                        NativeCatchUpDistance - 1,
                        parsed.ai.followDistance)
                    || !readInt(
                        "velocity_bonus",
                        0,
                        255,
                        parsed.ai.velocityBonus)) {
                return false;
            }
            if (parsed.ai.followDistance >= parsed.ai.catchUpDistance) {
                error = "ai.follow_distance must be lower than "
                    "ai.catch_up_distance";
                return false;
            }
        }

        if (const auto* reviveNode = root.get("revive")) {
            const auto* revive = reviveNode->as_table();
            if (!revive) {
                error = "revive must be a TOML table";
                return false;
            }
            constexpr std::string_view allowedKeys[]{
                "allow_high_rank_monsters",
                "preserve_native_auras",
            };
            for (const auto& [key, value] : *revive) {
                (void)value;
                if (std::find(
                        std::begin(allowedKeys),
                        std::end(allowedKeys),
                        key.str()) == std::end(allowedKeys)) {
                    error = "unknown setting: revive."
                        + std::string(key.str());
                    return false;
                }
            }
            const auto readBool = [&](const char* key, bool& destination) {
                const auto* node = revive->get(key);
                if (!node) return true;
                if (!node->is_boolean()) {
                    error = std::string("revive.") + key
                        + " must be true or false";
                    return false;
                }
                const auto value = node->value<bool>();
                if (!value) {
                    error = std::string("revive.") + key
                        + " must be true or false";
                    return false;
                }
                destination = *value;
                return true;
            };
            if (!readBool(
                    "allow_high_rank_monsters",
                    parsed.revive.allowHighRankMonsters)
                    || !readBool(
                        "preserve_native_auras",
                        parsed.revive.preserveNativeAuras)) {
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
                if (!node->is_boolean()) {
                    error = "diagnostics.enabled must be true or false";
                    return false;
                }
                const auto value = node->value<bool>();
                if (!value) {
                    error = "diagnostics.enabled must be true or false";
                    return false;
                }
                parsed.diagnostics = *value;
            }
        }

        result = parsed;
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}

} // namespace ruffneckk::revive_overhaul
