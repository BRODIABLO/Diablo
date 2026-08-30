#pragma once

#include <toml++/toml.hpp>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ruffneckk::isc12 {

inline constexpr std::int64_t ConfigVersion = 1;

struct Config {
    bool enabled{false};
    bool diagnostics{false};
};

inline auto ParseToml(
        std::string_view input,
        Config& result,
        std::string& error) -> bool {
    try {
        const auto root = toml::parse(input);
        for (const auto& [key, value] : root) {
            (void)value;
            if (key != "config_version" && key != "enabled"
                    && key != "diagnostics") {
                error = "unknown top-level setting or section: "
                    + std::string(key.str());
                return false;
            }
        }

        const auto* versionNode = root.get("config_version");
        const auto* enabledNode = root.get("enabled");
        const auto* diagnosticsNode = root.get("diagnostics");
        const auto version = versionNode
            ? versionNode->value<std::int64_t>()
            : std::optional<std::int64_t>{};
        if (!versionNode || !versionNode->is_integer() || !version
                || *version != ConfigVersion) {
            error = "config_version must be integer 1";
            return false;
        }
        if (!enabledNode || !enabledNode->is_boolean()) {
            error = "enabled must be a boolean";
            return false;
        }
        const auto* diagnostics = diagnosticsNode
            ? diagnosticsNode->as_table() : nullptr;
        if (!diagnostics) {
            error = "diagnostics must be a table";
            return false;
        }
        for (const auto& [key, value] : *diagnostics) {
            (void)value;
            if (key != "enabled") {
                error = "unknown diagnostics setting: "
                    + std::string(key.str());
                return false;
            }
        }
        const auto* diagnosticsEnabled = diagnostics->get("enabled");
        if (!diagnosticsEnabled || !diagnosticsEnabled->is_boolean()) {
            error = "diagnostics.enabled must be a boolean";
            return false;
        }

        result.enabled = *enabledNode->value<bool>();
        result.diagnostics = *diagnosticsEnabled->value<bool>();
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
        const auto candidate = (directory / fileName).lexically_normal();
        if (std::find(result.begin(), result.end(), candidate) == result.end()) {
            result.push_back(candidate);
        }
    }
    return result;
}

} // namespace ruffneckk::isc12
