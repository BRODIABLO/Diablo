#pragma once

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace RuffnecKk::MeleeSplash {

inline constexpr std::int32_t NormalAttackSkillId = 0;
inline constexpr std::int32_t DisabledStatId = -1;
inline constexpr std::int32_t AnyLayer = -1;
inline constexpr std::int32_t MaximumNativeId = 0xFFFF;

enum class ActivationMode {
    AllEligibleMelee,
    Whitelist,
    Blacklist,
};

struct SkillOverride {
    std::optional<bool> enabled;
    std::optional<std::int32_t> baseRadiusTiles;
    std::optional<std::int32_t> baseSplashDamagePercent;
    std::optional<bool> requireGateStat;
};

struct LegacyEvent20Suppression {
    bool enabled{false};
    std::int32_t statId{DisabledStatId};
    std::int32_t layer{AnyLayer};
    bool playerAttackersOnly{true};
};

struct Config {
    // Missing configuration is deliberately safe: the plugin stays loaded but
    // does not install an active gameplay policy.
    bool enabled{false};
    ActivationMode activationMode{ActivationMode::AllEligibleMelee};
    bool allowNormalAttack{true};
    std::vector<std::int32_t> includedSkillIds;
    std::vector<std::int32_t> excludedSkillIds;
    bool requireGateStat{false};
    std::int32_t gateStatId{DisabledStatId};
    std::int32_t increasedRadiusStatId{DisabledStatId};
    std::int32_t radiusPercentPerTile{20};
    std::int32_t splashDamagePercentStatId{DisabledStatId};
    std::int32_t baseSplashDamagePercent{100};
    std::int32_t baseRadiusNormalWeapon{4};
    std::int32_t baseRadiusExceptionalEliteWeapon{5};
    std::int32_t maximumRadiusTiles{0};
    bool diagnosticLogging{false};
    std::map<std::int32_t, SkillOverride> skillOverrides;
    LegacyEvent20Suppression legacyEvent20Suppression;
};

struct LoadResult {
    Config config{};
    std::filesystem::path source;
    bool found{false};
};

struct UnitIdentity {
    std::int32_t type{};
    std::uint32_t guid{};

    friend constexpr bool operator==(
            const UnitIdentity&, const UnitIdentity&) noexcept = default;
};

inline constexpr std::string_view ActivationModeName(
        ActivationMode mode) noexcept {
    switch (mode) {
    case ActivationMode::AllEligibleMelee:
        return "allEligibleMelee";
    case ActivationMode::Whitelist:
        return "whitelist";
    case ActivationMode::Blacklist:
        return "blacklist";
    }
    return "allEligibleMelee";
}

inline ActivationMode ParseActivationMode(std::string_view value) {
    if (value == "allEligibleMelee") {
        return ActivationMode::AllEligibleMelee;
    }
    if (value == "whitelist") return ActivationMode::Whitelist;
    if (value == "blacklist") return ActivationMode::Blacklist;
    throw std::invalid_argument(
        "activationMode must be allEligibleMelee, whitelist, or blacklist");
}

inline void RequireAllowedKeys(
        const nlohmann::json& object,
        std::initializer_list<std::string_view> allowed,
        std::string_view context) {
    if (!object.is_object()) {
        throw std::invalid_argument(std::string(context) + " must be an object");
    }
    for (const auto& [key, value] : object.items()) {
        (void)value;
        if (std::find(allowed.begin(), allowed.end(), std::string_view(key))
                == allowed.end()) {
            throw std::invalid_argument(
                std::string(context) + " contains unknown key '" + key + "'");
        }
    }
}

inline bool ReadBoolean(
        const nlohmann::json& object,
        std::string_view key,
        bool defaultValue,
        std::string_view context = {}) {
    const auto entry = object.find(std::string(key));
    if (entry == object.end()) return defaultValue;
    if (!entry->is_boolean()) {
        const auto prefix = context.empty()
            ? std::string{}
            : std::string(context) + ".";
        throw std::invalid_argument(prefix + std::string(key) + " must be a boolean");
    }
    return entry->get<bool>();
}

inline std::int32_t ReadInteger(
        const nlohmann::json& object,
        std::string_view key,
        std::int32_t defaultValue,
        std::int32_t minimum,
        std::int32_t maximum,
        std::string_view context = {}) {
    const auto entry = object.find(std::string(key));
    if (entry == object.end()) return defaultValue;
    if (!entry->is_number_integer()) {
        const auto prefix = context.empty()
            ? std::string{}
            : std::string(context) + ".";
        throw std::invalid_argument(prefix + std::string(key) + " must be an integer");
    }

    bool inRange{};
    std::int64_t signedValue{};
    if (entry->is_number_unsigned()) {
        const auto value = entry->get<std::uint64_t>();
        inRange = value <= static_cast<std::uint64_t>(maximum);
        if (inRange) signedValue = static_cast<std::int64_t>(value);
    } else {
        signedValue = entry->get<std::int64_t>();
        inRange = signedValue >= minimum && signedValue <= maximum;
    }
    if (!inRange) {
        const auto prefix = context.empty()
            ? std::string{}
            : std::string(context) + ".";
        throw std::invalid_argument(
            prefix + std::string(key) + " is outside the supported range");
    }
    return static_cast<std::int32_t>(signedValue);
}

inline std::int32_t ParseSkillIdKey(std::string_view key) {
    std::int32_t skillId{};
    const auto result = std::from_chars(
        key.data(), key.data() + key.size(), skillId);
    if (result.ec != std::errc{} || result.ptr != key.data() + key.size()
            || skillId < 0 || skillId > MaximumNativeId
            || std::to_string(skillId) != key) {
        throw std::invalid_argument(
            "skillOverrides keys must be canonical decimal skill IDs");
    }
    return skillId;
}

inline std::vector<std::int32_t> ParseSkillIds(
        const nlohmann::json& root, std::string_view key) {
    const auto entry = root.find(std::string(key));
    if (entry == root.end()) return {};
    if (!entry->is_array()) {
        throw std::invalid_argument(std::string(key) + " must be an array");
    }

    std::vector<std::int32_t> values;
    values.reserve(entry->size());
    for (const auto& item : *entry) {
        if (!item.is_number_integer()) {
            throw std::invalid_argument(
                std::string(key) + " entries must be integer skill IDs");
        }
        std::uint64_t unsignedValue{};
        if (item.is_number_unsigned()) {
            unsignedValue = item.get<std::uint64_t>();
        } else {
            const auto signedValue = item.get<std::int64_t>();
            if (signedValue < 0) {
                throw std::invalid_argument(
                    std::string(key) + " entries must be non-negative");
            }
            unsignedValue = static_cast<std::uint64_t>(signedValue);
        }
        if (unsignedValue > static_cast<std::uint64_t>(MaximumNativeId)) {
            throw std::invalid_argument(
                std::string(key) + " contains an unsupported skill ID");
        }
        const auto skillId = static_cast<std::int32_t>(unsignedValue);
        if (std::find(values.begin(), values.end(), skillId) == values.end()) {
            values.emplace_back(skillId);
        }
    }
    return values;
}

inline SkillOverride ParseSkillOverride(
        const nlohmann::json& object, std::int32_t skillId) {
    const auto context = "skillOverrides." + std::to_string(skillId);
    RequireAllowedKeys(
        object,
        {"enabled", "baseRadiusTiles", "baseSplashDamagePercent",
         "requireGateStat"},
        context);

    SkillOverride result;
    if (object.contains("enabled")) {
        result.enabled = ReadBoolean(object, "enabled", false, context);
    }
    if (object.contains("baseRadiusTiles")) {
        result.baseRadiusTiles = ReadInteger(
            object,
            "baseRadiusTiles",
            0,
            0,
            std::numeric_limits<std::int32_t>::max(),
            context);
    }
    if (object.contains("baseSplashDamagePercent")) {
        result.baseSplashDamagePercent = ReadInteger(
            object,
            "baseSplashDamagePercent",
            0,
            0,
            std::numeric_limits<std::int32_t>::max(),
            context);
    }
    if (object.contains("requireGateStat")) {
        result.requireGateStat = ReadBoolean(
            object, "requireGateStat", false, context);
    }
    return result;
}

inline Config ParseConfig(const nlohmann::json& root) {
    RequireAllowedKeys(
        root,
        {"enabled", "activationMode", "allowNormalAttack",
         "includedSkillIds", "excludedSkillIds", "requireGateStat",
         "gateStatId", "increasedRadiusStatId", "radiusPercentPerTile",
         "splashDamagePercentStatId", "baseSplashDamagePercent",
         "baseRadiusNormalWeapon", "baseRadiusExceptionalEliteWeapon",
         "maximumRadiusTiles", "diagnosticLogging", "skillOverrides",
         "legacyEvent20Suppression"},
        "configuration root");

    Config result;
    result.enabled = ReadBoolean(root, "enabled", result.enabled);
    result.allowNormalAttack = ReadBoolean(
        root, "allowNormalAttack", result.allowNormalAttack);
    result.requireGateStat = ReadBoolean(
        root, "requireGateStat", result.requireGateStat);
    result.diagnosticLogging = ReadBoolean(
        root, "diagnosticLogging", result.diagnosticLogging);

    if (const auto entry = root.find("activationMode"); entry != root.end()) {
        if (!entry->is_string()) {
            throw std::invalid_argument("activationMode must be a string");
        }
        result.activationMode = ParseActivationMode(entry->get<std::string>());
    }

    result.includedSkillIds = ParseSkillIds(root, "includedSkillIds");
    result.excludedSkillIds = ParseSkillIds(root, "excludedSkillIds");
    result.gateStatId = ReadInteger(
        root, "gateStatId", result.gateStatId, DisabledStatId, MaximumNativeId);
    result.increasedRadiusStatId = ReadInteger(
        root,
        "increasedRadiusStatId",
        result.increasedRadiusStatId,
        DisabledStatId,
        MaximumNativeId);
    result.radiusPercentPerTile = ReadInteger(
        root,
        "radiusPercentPerTile",
        result.radiusPercentPerTile,
        1,
        std::numeric_limits<std::int32_t>::max());
    result.splashDamagePercentStatId = ReadInteger(
        root,
        "splashDamagePercentStatId",
        result.splashDamagePercentStatId,
        DisabledStatId,
        MaximumNativeId);
    result.baseSplashDamagePercent = ReadInteger(
        root,
        "baseSplashDamagePercent",
        result.baseSplashDamagePercent,
        0,
        std::numeric_limits<std::int32_t>::max());
    result.baseRadiusNormalWeapon = ReadInteger(
        root,
        "baseRadiusNormalWeapon",
        result.baseRadiusNormalWeapon,
        0,
        std::numeric_limits<std::int32_t>::max());
    result.baseRadiusExceptionalEliteWeapon = ReadInteger(
        root,
        "baseRadiusExceptionalEliteWeapon",
        result.baseRadiusExceptionalEliteWeapon,
        0,
        std::numeric_limits<std::int32_t>::max());
    result.maximumRadiusTiles = ReadInteger(
        root,
        "maximumRadiusTiles",
        result.maximumRadiusTiles,
        0,
        std::numeric_limits<std::int32_t>::max());

    if (const auto entry = root.find("skillOverrides"); entry != root.end()) {
        if (!entry->is_object()) {
            throw std::invalid_argument("skillOverrides must be an object");
        }
        for (const auto& [key, value] : entry->items()) {
            const auto skillId = ParseSkillIdKey(key);
            result.skillOverrides.emplace(
                skillId, ParseSkillOverride(value, skillId));
        }
    }

    if (const auto entry = root.find("legacyEvent20Suppression");
            entry != root.end()) {
        RequireAllowedKeys(
            *entry,
            {"enabled", "statId", "layer", "playerAttackersOnly"},
            "legacyEvent20Suppression");
        auto& suppression = result.legacyEvent20Suppression;
        suppression.enabled = ReadBoolean(
            *entry,
            "enabled",
            suppression.enabled,
            "legacyEvent20Suppression");
        suppression.statId = ReadInteger(
            *entry,
            "statId",
            suppression.statId,
            DisabledStatId,
            MaximumNativeId,
            "legacyEvent20Suppression");
        suppression.layer = ReadInteger(
            *entry,
            "layer",
            suppression.layer,
            AnyLayer,
            MaximumNativeId,
            "legacyEvent20Suppression");
        suppression.playerAttackersOnly = ReadBoolean(
            *entry,
            "playerAttackersOnly",
            suppression.playerAttackersOnly,
            "legacyEvent20Suppression");
    }

    bool anyPolicyRequiresGate = result.requireGateStat;
    for (const auto& [skillId, override] : result.skillOverrides) {
        (void)skillId;
        if (override.requireGateStat.value_or(false)) {
            anyPolicyRequiresGate = true;
        }
    }
    if (anyPolicyRequiresGate && result.gateStatId == DisabledStatId) {
        throw std::invalid_argument(
            "gateStatId must be configured when a policy requires the gate stat");
    }
    if (result.legacyEvent20Suppression.enabled
            && (result.legacyEvent20Suppression.statId == DisabledStatId
                || result.legacyEvent20Suppression.layer == AnyLayer)) {
        throw std::invalid_argument(
            "legacyEvent20Suppression requires exact statId and layer values");
    }
    return result;
}

inline bool ContainsSkill(
        const std::vector<std::int32_t>& skills,
        std::int32_t skillId) noexcept {
    return std::find(skills.begin(), skills.end(), skillId) != skills.end();
}

inline const SkillOverride* FindSkillOverride(
        const Config& config, std::int32_t skillId) noexcept {
    const auto entry = config.skillOverrides.find(skillId);
    return entry == config.skillOverrides.end() ? nullptr : &entry->second;
}

inline bool IsSkillEnabled(
        const Config& config, std::int32_t skillId) noexcept {
    if (!config.enabled || skillId < 0
            || ContainsSkill(config.excludedSkillIds, skillId)) {
        return false;
    }
    if (skillId == NormalAttackSkillId && !config.allowNormalAttack) {
        return false;
    }
    if (config.activationMode == ActivationMode::Whitelist
            && !ContainsSkill(config.includedSkillIds, skillId)) {
        return false;
    }

    const auto* override = FindSkillOverride(config, skillId);
    return !override || override->enabled.value_or(true);
}

inline bool RequiresGateStat(
        const Config& config, std::int32_t skillId) noexcept {
    if (const auto* override = FindSkillOverride(config, skillId);
            override && override->requireGateStat.has_value()) {
        return *override->requireGateStat;
    }
    return config.requireGateStat;
}

inline std::int32_t SaturatingNonNegative(std::int64_t value) noexcept {
    if (value <= 0) return 0;
    if (value >= std::numeric_limits<std::int32_t>::max()) {
        return std::numeric_limits<std::int32_t>::max();
    }
    return static_cast<std::int32_t>(value);
}

inline std::int32_t ResolveRadiusTiles(
        const Config& config,
        std::int32_t skillId,
        bool exceptionalOrEliteWeapon,
        std::int32_t totalRadiusPercent) noexcept {
    std::int32_t baseRadius = exceptionalOrEliteWeapon
        ? config.baseRadiusExceptionalEliteWeapon
        : config.baseRadiusNormalWeapon;
    if (const auto* override = FindSkillOverride(config, skillId);
            override && override->baseRadiusTiles.has_value()) {
        baseRadius = *override->baseRadiusTiles;
    }

    const auto nonNegativePercent = std::max(totalRadiusPercent, 0);
    const auto safePercentPerTile = std::max(config.radiusPercentPerTile, 1);
    const auto bonus = nonNegativePercent / safePercentPerTile;
    auto result = SaturatingNonNegative(
        static_cast<std::int64_t>(baseRadius) + bonus);
    if (config.maximumRadiusTiles > 0) {
        result = std::min(result, config.maximumRadiusTiles);
    }
    return result;
}

inline std::int32_t ResolveSplashDamagePercent(
        const Config& config,
        std::int32_t skillId,
        std::int32_t totalSplashDamagePercent) noexcept {
    std::int32_t basePercent = config.baseSplashDamagePercent;
    if (const auto* override = FindSkillOverride(config, skillId);
            override && override->baseSplashDamagePercent.has_value()) {
        basePercent = *override->baseSplashDamagePercent;
    }
    return SaturatingNonNegative(
        static_cast<std::int64_t>(basePercent) + totalSplashDamagePercent);
}

inline bool MatchesLegacyEvent20Suppression(
        const LegacyEvent20Suppression& suppression,
        std::uint32_t packedStatAndLayer,
        bool attackerIsPlayer) noexcept {
    if (!suppression.enabled
            || suppression.statId < 0
            || suppression.layer < 0
            || (suppression.playerAttackersOnly && !attackerIsPlayer)) {
        return false;
    }
    const auto statId = static_cast<std::uint16_t>(packedStatAndLayer >> 16);
    const auto layer = static_cast<std::uint16_t>(packedStatAndLayer);
    return statId == static_cast<std::uint16_t>(suppression.statId)
        && layer == static_cast<std::uint16_t>(suppression.layer);
}

inline bool MatchesLegacyEvent20Suppression(
        const Config& config,
        std::uint32_t packedStatAndLayer,
        bool attackerIsPlayer) noexcept {
    return config.enabled
        && MatchesLegacyEvent20Suppression(
            config.legacyEvent20Suppression,
            packedStatAndLayer,
            attackerIsPlayer);
}

inline bool AppendUniqueSecondaryTarget(
        std::vector<UnitIdentity>& targets,
        UnitIdentity candidate,
        UnitIdentity attacker,
        UnitIdentity primary) {
    if (candidate == attacker || candidate == primary
            || std::find(targets.begin(), targets.end(), candidate)
                != targets.end()) {
        return false;
    }
    targets.emplace_back(candidate);
    return true;
}

inline std::vector<std::filesystem::path> BuildConfigCandidates(
        const std::filesystem::path& activeModConfigDirectory,
        const std::filesystem::path& scopeConfigDirectory,
        const std::filesystem::path& globalConfigDirectory,
        const std::filesystem::path& fileName = "MeleeSplash.json") {
    std::vector<std::filesystem::path> candidates;
    const auto append = [&](const std::filesystem::path& directory) {
        if (directory.empty()) return;
        const auto candidate = (directory / fileName).lexically_normal();
        if (std::find(candidates.begin(), candidates.end(), candidate)
                == candidates.end()) {
            candidates.emplace_back(candidate);
        }
    };
    // First existing file wins: active mod, then the DLL's own loader scope,
    // then the global loader scope.
    append(activeModConfigDirectory);
    append(scopeConfigDirectory);
    append(globalConfigDirectory);
    return candidates;
}

inline LoadResult LoadConfig(
        const std::vector<std::filesystem::path>& candidates) {
    for (const auto& path : candidates) {
        std::error_code error;
        const auto regularFile = std::filesystem::is_regular_file(path, error);
        if (error) {
            if (error == std::errc::no_such_file_or_directory) continue;
            throw std::runtime_error(
                "cannot inspect MeleeSplash configuration " + path.string());
        }
        if (!regularFile) continue;

        std::ifstream input(path, std::ios::binary);
        if (!input.is_open()) {
            throw std::runtime_error(
                "cannot open MeleeSplash configuration " + path.string());
        }
        try {
            const auto root = nlohmann::json::parse(input);
            return {ParseConfig(root), path, true};
        } catch (const std::exception& exception) {
            throw std::runtime_error(
                "invalid MeleeSplash configuration " + path.string()
                + " (" + exception.what() + ")");
        }
    }
    return {};
}

} // namespace RuffnecKk::MeleeSplash
