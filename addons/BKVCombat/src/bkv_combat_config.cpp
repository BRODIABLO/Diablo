#include "bkv_combat_config.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <unordered_set>

namespace RuffnecKk::BKVCombat {
namespace {

using Json = nlohmann::json;

void RequireObject(const Json& value, std::string_view context) {
    if (!value.is_object()) {
        throw std::invalid_argument(std::string(context) + " must be an object");
    }
}

void RequireExactKeys(
    const Json& object,
    std::initializer_list<std::string_view> expected,
    std::string_view context) {
    RequireObject(object, context);
    for (const auto key : expected) {
        if (!object.contains(std::string(key))) {
            throw std::invalid_argument(
                std::string(context) + " is missing required key '"
                + std::string(key) + "'");
        }
    }
    for (const auto& [key, value] : object.items()) {
        (void)value;
        if (std::find(expected.begin(), expected.end(), std::string_view(key))
            == expected.end()) {
            throw std::invalid_argument(
                std::string(context) + " contains unknown key '" + key + "'");
        }
    }
}

bool ReadBoolean(
    const Json& object,
    std::string_view key,
    std::string_view context) {
    const auto& value = object.at(std::string(key));
    if (!value.is_boolean()) {
        throw std::invalid_argument(
            std::string(context) + "." + std::string(key)
            + " must be a boolean");
    }
    return value.get<bool>();
}

std::int32_t ReadInteger(
    const Json& object,
    std::string_view key,
    std::int32_t minimum,
    std::int32_t maximum,
    std::string_view context) {
    const auto& value = object.at(std::string(key));
    if (!value.is_number_integer()) {
        throw std::invalid_argument(
            std::string(context) + "." + std::string(key)
            + " must be an integer");
    }

    std::int64_t parsed{};
    if (value.is_number_unsigned()) {
        const auto unsignedValue = value.get<std::uint64_t>();
        if (unsignedValue > static_cast<std::uint64_t>(maximum)) {
            throw std::invalid_argument(
                std::string(context) + "." + std::string(key)
                + " is outside the supported range");
        }
        parsed = static_cast<std::int64_t>(unsignedValue);
    } else {
        parsed = value.get<std::int64_t>();
    }
    if (parsed < minimum || parsed > maximum) {
        throw std::invalid_argument(
            std::string(context) + "." + std::string(key)
            + " is outside the supported range");
    }
    return static_cast<std::int32_t>(parsed);
}

bool IsCanonicalMonStatsKey(std::string_view value) noexcept {
    if (value.empty() || value.size() > 63) return false;
    return std::all_of(value.begin(), value.end(), [](const char character) {
        return (character >= 'a' && character <= 'z')
            || (character >= '0' && character <= '9')
            || character == '_';
    });
}

PolicyToggles ParsePolicies(const Json& object) {
    RequireExactKeys(
        object,
        {"criticalStrike", "deadlyStrike", "crushingBlow", "lifeSteal",
         "manaSteal", "openWounds"},
        "policies");
    return {
        .criticalStrike = ReadBoolean(object, "criticalStrike", "policies"),
        .deadlyStrike = ReadBoolean(object, "deadlyStrike", "policies"),
        .crushingBlow = ReadBoolean(object, "crushingBlow", "policies"),
        .lifeSteal = ReadBoolean(object, "lifeSteal", "policies"),
        .manaSteal = ReadBoolean(object, "manaSteal", "policies"),
        .openWounds = ReadBoolean(object, "openWounds", "policies"),
    };
}

StatIds ParseStatIds(const Json& object) {
    RequireExactKeys(object, {"crushingBlowEfficiencyStatId"}, "stats");
    return {
        .crushingBlowEfficiencyStatId = ReadInteger(
            object,
            "crushingBlowEfficiencyStatId",
            -1,
            std::numeric_limits<std::uint16_t>::max(),
            "stats"),
    };
}

Classifications ParseClassifications(const Json& object) {
    RequireExactKeys(object, {"majorBosses"}, "classifications");
    const auto& entries = object.at("majorBosses");
    if (!entries.is_array() || entries.size() != RequiredMajorBossCount) {
        throw std::invalid_argument(
            "classifications.majorBosses must contain exactly ten entries");
    }

    Classifications result;
    result.majorBosses.reserve(RequiredMajorBossCount);
    std::unordered_set<std::string> keys;
    std::unordered_set<std::int32_t> ids;
    for (std::size_t index{}; index < entries.size(); ++index) {
        const auto& entry = entries[index];
        const auto context =
            "classifications.majorBosses[" + std::to_string(index) + "]";
        RequireExactKeys(entry, {"monstats", "expectedId"}, context);

        const auto& keyValue = entry.at("monstats");
        if (!keyValue.is_string()) {
            throw std::invalid_argument(context + ".monstats must be a string");
        }
        auto key = keyValue.get<std::string>();
        if (!IsCanonicalMonStatsKey(key)) {
            throw std::invalid_argument(
                context + ".monstats must be a canonical lowercase key");
        }
        const auto expectedId = ReadInteger(
            entry,
            "expectedId",
            0,
            std::numeric_limits<std::uint16_t>::max(),
            context);
        if (!keys.insert(key).second) {
            throw std::invalid_argument(
                "classifications.majorBosses contains duplicate monstats key '"
                + key + "'");
        }
        if (!ids.insert(expectedId).second) {
            throw std::invalid_argument(
                "classifications.majorBosses contains duplicate expectedId "
                + std::to_string(expectedId));
        }
        result.majorBosses.push_back({
            .monstats = std::move(key),
            .expectedId = expectedId,
        });
    }
    return result;
}

} // namespace

Config ParseConfig(const nlohmann::json& root) {
    RequireExactKeys(
        root,
        {"schemaVersion", "enabled", "diagnosticLogging", "policies",
         "stats", "classifications"},
        "configuration root");

    Config result;
    result.schemaVersion = ReadInteger(
        root,
        "schemaVersion",
        CurrentSchemaVersion,
        CurrentSchemaVersion,
        "configuration root");
    result.enabled = ReadBoolean(root, "enabled", "configuration root");
    result.diagnosticLogging = ReadBoolean(
        root, "diagnosticLogging", "configuration root");
    result.policies = ParsePolicies(root.at("policies"));
    result.stats = ParseStatIds(root.at("stats"));
    result.classifications = ParseClassifications(root.at("classifications"));
    return result;
}

LoadResult LoadConfig(
    const std::vector<std::filesystem::path>& candidates) {
    for (const auto& path : candidates) {
        std::error_code error;
        const auto exists = std::filesystem::exists(path, error);
        if (error) {
            throw std::runtime_error(
                "cannot inspect BKVCombat configuration '" + path.string()
                + "': " + error.message());
        }
        if (!exists) continue;
        if (!std::filesystem::is_regular_file(path, error) || error) {
            throw std::runtime_error(
                "BKVCombat configuration path is not a readable file: "
                + path.string());
        }

        std::ifstream input(path, std::ios::binary);
        if (!input.is_open()) {
            throw std::runtime_error(
                "cannot open BKVCombat configuration '" + path.string() + "'");
        }
        try {
            return {
                .config = ParseConfig(Json::parse(input)),
                .source = path,
                .found = true,
            };
        } catch (const std::exception& exception) {
            throw std::runtime_error(
                "invalid BKVCombat configuration '" + path.string()
                + "': " + exception.what());
        }
    }
    return {};
}

std::vector<std::filesystem::path> BuildConfigCandidates(
    const std::filesystem::path& activeModConfigDirectory,
    const std::filesystem::path& scopeConfigDirectory,
    const std::filesystem::path& globalConfigDirectory) {
    std::vector<std::filesystem::path> result;
    const auto append = [&](const std::filesystem::path& directory) {
        if (directory.empty()) return;
        const auto candidate =
            (directory / std::filesystem::path(ConfigFileName)).lexically_normal();
        if (std::find(result.begin(), result.end(), candidate) == result.end()) {
            result.push_back(candidate);
        }
    };
    append(activeModConfigDirectory);
    append(scopeConfigDirectory);
    append(globalConfigDirectory);
    return result;
}

} // namespace RuffnecKk::BKVCombat
