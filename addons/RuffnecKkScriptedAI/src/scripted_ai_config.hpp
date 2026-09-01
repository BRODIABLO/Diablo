#pragma once

#include "scripted_ai_limits.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace ruffneckk::scripted_ai {

inline constexpr std::int64_t CurrentConfigSchemaVersion = 1;
inline constexpr wchar_t ConfigFileName[] =
    L"ruffneckk-scripted-ai.toml";

struct Config {
    bool enabled{};
    bool diagnostics{};
    std::string scriptDirectory{"scripts/ruffneckk-scripted-ai"};
    SandboxLimits limits{};
};

[[nodiscard]] auto ParseConfig(std::string_view text) -> Config;

[[nodiscard]] auto BuildConfigCandidates(
    const std::filesystem::path& activeModConfigDirectory,
    const std::filesystem::path& scopeConfigDirectory,
    const std::filesystem::path& globalConfigDirectory)
    -> std::vector<std::filesystem::path>;

} // namespace ruffneckk::scripted_ai
