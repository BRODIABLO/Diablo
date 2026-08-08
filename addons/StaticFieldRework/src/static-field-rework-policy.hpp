#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace RuffnecKk::StaticFieldRework {

constexpr std::int32_t StaticFieldSkillId = 42;

constexpr bool ShouldApplyDebuff(
        bool enabled,
        std::int32_t nativeResult,
        std::int32_t skillId,
        std::int32_t skillLevel) noexcept {
    return enabled
        && nativeResult != 0
        && skillId == StaticFieldSkillId
        && skillLevel > 0;
}

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

} // namespace RuffnecKk::StaticFieldRework
