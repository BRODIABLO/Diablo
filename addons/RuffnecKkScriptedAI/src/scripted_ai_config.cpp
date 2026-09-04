#include "scripted_ai_config.hpp"

#include <toml++/toml.hpp>

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace ruffneckk::scripted_ai {
namespace {

[[nodiscard]] auto IsAllowedKey(
        std::string_view key,
        std::initializer_list<std::string_view> allowed) noexcept -> bool {
    return std::find(allowed.begin(), allowed.end(), key) != allowed.end();
}

void RejectUnknownKeys(
        const toml::table& table,
        std::initializer_list<std::string_view> allowed,
        std::string_view section) {
    for (const auto& [key, unused] : table) {
        (void)unused;
        if (!IsAllowedKey(key.str(), allowed)) {
            throw std::runtime_error(
                "Unknown Scripted AI key in " + std::string(section)
                + ": " + std::string(key.str()));
        }
    }
}

template<class T>
[[nodiscard]] auto ReadOptional(
        const toml::table& table,
        std::string_view key,
        T fallback) -> T {
    const auto* node = table.get(key);
    if (node == nullptr) return fallback;
    if constexpr (std::is_same_v<T, bool>) {
        if (!node->is_boolean()) {
            throw std::runtime_error(
                "Scripted AI key has the wrong type: " + std::string(key));
        }
    } else if constexpr (std::is_integral_v<T>) {
        if (!node->is_integer()) {
            throw std::runtime_error(
                "Scripted AI key has the wrong type: " + std::string(key));
        }
    }
    const auto value = node->value<T>();
    if (!value) {
        throw std::runtime_error(
            "Scripted AI key has the wrong type: " + std::string(key));
    }
    return *value;
}

[[nodiscard]] auto ReadOptionalSize(
        const toml::table& table,
        std::string_view key,
        std::size_t fallback) -> std::size_t {
    const auto value = ReadOptional<std::int64_t>(
        table,
        key,
        static_cast<std::int64_t>(fallback));
    if (value < 0
            || static_cast<std::uint64_t>(value)
                > std::numeric_limits<std::size_t>::max()) {
        throw std::runtime_error(
            "Scripted AI key is outside its supported range: "
            + std::string(key));
    }
    return static_cast<std::size_t>(value);
}

[[nodiscard]] auto ReadOptionalU32(
        const toml::table& table,
        std::string_view key,
        std::uint32_t fallback) -> std::uint32_t {
    const auto value = ReadOptional<std::int64_t>(table, key, fallback);
    if (value < 0
            || static_cast<std::uint64_t>(value)
                > std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error(
            "Scripted AI key is outside its supported range: "
            + std::string(key));
    }
    return static_cast<std::uint32_t>(value);
}

[[nodiscard]] auto ReadOptionalTable(
        const toml::table& root,
        std::string_view key) -> const toml::table* {
    const auto* node = root.get(key);
    if (node == nullptr) return nullptr;
    const auto* table = node->as_table();
    if (table == nullptr) {
        throw std::runtime_error(
            "Scripted AI section must be a table: " + std::string(key));
    }
    return table;
}

[[nodiscard]] auto IsSafeRelativeScriptDirectory(
        std::string_view value) noexcept -> bool {
    if (value.empty() || value.size() > 160U || value.front() == '/'
            || value.front() == '\\' || value.find(':') != value.npos
            || value.find('\\') != value.npos) {
        return false;
    }
    std::size_t start{};
    while (start <= value.size()) {
        const auto end = value.find('/', start);
        const auto segment = value.substr(
            start,
            end == value.npos ? value.size() - start : end - start);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        if (end == value.npos) break;
        start = end + 1U;
    }
    return true;
}

[[nodiscard]] auto IsSafeRelativeLuaScript(
        std::string_view value) noexcept -> bool {
    if (value.empty() || value.size() > 160U || value.front() == '/'
            || value.front() == '\\' || value.find(':') != value.npos
            || value.find('\\') != value.npos) {
        return false;
    }
    std::size_t start{};
    while (start <= value.size()) {
        const auto end = value.find('/', start);
        const auto segment = value.substr(
            start,
            end == value.npos ? value.size() - start : end - start);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        if (end == value.npos) break;
        start = end + 1U;
    }
    return value.size() >= 4U
        && value.substr(value.size() - 4U) == ".lua";
}

} // namespace

auto ParseConfig(std::string_view text) -> Config {
    const auto root = toml::parse(text);
    RejectUnknownKeys(
        root,
        {"schema_version", "enabled", "script_directory", "diagnostics",
         "domains", "sandbox"},
        "root");

    const auto* schemaNode = root.get("schema_version");
    const auto schemaVersion = schemaNode != nullptr
        && schemaNode->is_integer()
        ? schemaNode->value<std::int64_t>()
        : std::optional<std::int64_t>{};
    if (!schemaVersion || *schemaVersion != CurrentConfigSchemaVersion) {
        throw std::runtime_error(
            "Scripted AI schema_version must be exactly 1");
    }

    Config result{};
    result.enabled = ReadOptional(root, "enabled", result.enabled);
    result.diagnostics = ReadOptional(
        root,
        "diagnostics",
        result.diagnostics);
    if (const auto* node = root.get("script_directory")) {
        if (!node->is_string()) {
            throw std::runtime_error(
                "Scripted AI script_directory must be a string");
        }
        const auto value = node->value<std::string>();
        if (!value || !IsSafeRelativeScriptDirectory(*value)) {
            throw std::runtime_error(
                "Scripted AI script_directory must be a safe relative path using forward slashes");
        }
        result.scriptDirectory = *value;
    }

    if (const auto* domains = ReadOptionalTable(root, "domains")) {
        RejectUnknownKeys(*domains, {"revive"}, "domains");
        if (const auto* revive = ReadOptionalTable(*domains, "revive")) {
            RejectUnknownKeys(
                *revive,
                {"enabled", "script"},
                "domains.revive");
            result.revive.enabled = ReadOptional(
                *revive,
                "enabled",
                result.revive.enabled);
            if (const auto* node = revive->get("script")) {
                if (!node->is_string()) {
                    throw std::runtime_error(
                        "Scripted AI domains.revive.script must be a string");
                }
                const auto value = node->value<std::string>();
                if (!value || !IsSafeRelativeLuaScript(*value)) {
                    throw std::runtime_error(
                        "Scripted AI domains.revive.script must be a safe relative .lua path using forward slashes");
                }
                result.revive.script = *value;
            }
        }
    }

    if (const auto* sandbox = ReadOptionalTable(root, "sandbox")) {
        RejectUnknownKeys(
            *sandbox,
            {"max_source_bytes", "max_tree_nodes", "max_tree_depth",
             "max_children_per_node", "session_heap_bytes",
             "per_think_heap_growth_bytes", "max_instructions_per_think",
             "instruction_hook_interval", "max_load_instructions",
             "max_script_errors_per_session",
             "max_slow_strikes_per_session", "slow_think_microseconds",
             "detailed_log_interval_milliseconds"},
            "sandbox");
        auto& limits = result.limits;
        limits.maxSourceBytes = ReadOptionalSize(
            *sandbox,
            "max_source_bytes",
            limits.maxSourceBytes);
        limits.maxTreeNodes = ReadOptionalSize(
            *sandbox,
            "max_tree_nodes",
            limits.maxTreeNodes);
        limits.maxTreeDepth = ReadOptionalSize(
            *sandbox,
            "max_tree_depth",
            limits.maxTreeDepth);
        limits.maxChildrenPerNode = ReadOptionalSize(
            *sandbox,
            "max_children_per_node",
            limits.maxChildrenPerNode);
        limits.sessionHeapBytes = ReadOptionalSize(
            *sandbox,
            "session_heap_bytes",
            limits.sessionHeapBytes);
        limits.perThinkHeapGrowthBytes = ReadOptionalSize(
            *sandbox,
            "per_think_heap_growth_bytes",
            limits.perThinkHeapGrowthBytes);
        limits.maxInstructionsPerThink = ReadOptionalU32(
            *sandbox,
            "max_instructions_per_think",
            limits.maxInstructionsPerThink);
        limits.instructionHookInterval = ReadOptionalU32(
            *sandbox,
            "instruction_hook_interval",
            limits.instructionHookInterval);
        limits.maxLoadInstructions = ReadOptionalU32(
            *sandbox,
            "max_load_instructions",
            limits.maxLoadInstructions);
        limits.maxScriptErrorsPerSession = ReadOptionalU32(
            *sandbox,
            "max_script_errors_per_session",
            limits.maxScriptErrorsPerSession);
        limits.maxSlowStrikesPerSession = ReadOptionalU32(
            *sandbox,
            "max_slow_strikes_per_session",
            limits.maxSlowStrikesPerSession);
        limits.slowThinkMicroseconds = ReadOptionalU32(
            *sandbox,
            "slow_think_microseconds",
            limits.slowThinkMicroseconds);
        limits.detailedLogIntervalMilliseconds = ReadOptionalU32(
            *sandbox,
            "detailed_log_interval_milliseconds",
            limits.detailedLogIntervalMilliseconds);
    }

    if (!IsWithinHardLimits(result.limits)) {
        throw std::runtime_error(
            "Scripted AI sandbox values must stay within the compiled hard limits and align to the instruction-hook interval");
    }
    return result;
}

auto BuildConfigCandidates(
        const std::filesystem::path& activeModConfigDirectory,
        const std::filesystem::path& scopeConfigDirectory,
        const std::filesystem::path& globalConfigDirectory)
        -> std::vector<std::filesystem::path> {
    std::vector<std::filesystem::path> result;
    const auto append = [&](const std::filesystem::path& directory) {
        if (directory.empty()) return;
        const auto candidate =
            (directory / ConfigFileName).lexically_normal();
        if (std::find(result.begin(), result.end(), candidate) == result.end()) {
            result.push_back(candidate);
        }
    };
    append(activeModConfigDirectory);
    append(scopeConfigDirectory);
    append(globalConfigDirectory);
    return result;
}

} // namespace ruffneckk::scripted_ai
