#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace RuffnecKk::BKVCombat {

inline constexpr std::int32_t CurrentSchemaVersion = 2;
inline constexpr std::size_t RequiredMajorBossCount = 10;
inline constexpr std::string_view ConfigFileName = "BKVCombat.json";

struct PolicyToggles {
    bool criticalStrike{false};
    bool deadlyStrike{false};
    bool crushingBlow{false};
    bool lifeSteal{false};
    bool manaSteal{false};
    bool openWounds{false};

    friend constexpr bool operator==(
        const PolicyToggles&, const PolicyToggles&) noexcept = default;
};

struct MajorBossEntry {
    std::string monstats;
    std::int32_t expectedId{};

    friend bool operator==(
        const MajorBossEntry&, const MajorBossEntry&) noexcept = default;
};

struct Classifications {
    std::vector<MajorBossEntry> majorBosses;
};

struct StatIds {
    std::int32_t crushingBlowEfficiencyStatId{-1};

    friend constexpr bool operator==(
        const StatIds&, const StatIds&) noexcept = default;
};

struct Config {
    // A missing configuration deliberately produces this inert state.
    std::int32_t schemaVersion{CurrentSchemaVersion};
    bool enabled{false};
    bool diagnosticLogging{false};
    PolicyToggles policies;
    StatIds stats;
    Classifications classifications;
};

struct LoadResult {
    Config config;
    std::filesystem::path source;
    bool found{false};
};

Config ParseConfig(const nlohmann::json& root);

LoadResult LoadConfig(
    const std::vector<std::filesystem::path>& candidates);

std::vector<std::filesystem::path> BuildConfigCandidates(
    const std::filesystem::path& activeModConfigDirectory,
    const std::filesystem::path& scopeConfigDirectory,
    const std::filesystem::path& globalConfigDirectory);

} // namespace RuffnecKk::BKVCombat
