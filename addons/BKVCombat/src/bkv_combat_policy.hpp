#pragma once

#include "bkv_combat_config.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace RuffnecKk::BKVCombat {

enum class PolicyKind {
    CriticalStrike,
    DeadlyStrike,
    CrushingBlow,
    LifeSteal,
    ManaSteal,
    OpenWounds,
};

enum class MonsterClass {
    Ordinary,
    Elite,
    PrimeEvil,
    MajorBoss,
};

struct DamageFraction {
    std::int32_t numerator{1};
    std::int32_t denominator{1};

    friend constexpr bool operator==(
        const DamageFraction&, const DamageFraction&) noexcept = default;
};

struct ResolvedMonStatsIdentity {
    std::string monstats;
    std::int32_t monstatsId{};
};

struct RuntimeMonsterFacts {
    std::string_view monstats;
    std::int32_t monstatsId{};
    bool primeEvil{false};
    bool heraldOrAscendant{false};
    bool champion{false};
    bool unique{false};
    bool superUnique{false};
    bool boss{false};
};

struct MajorBossRegistry {
    bool valid{false};
    std::vector<MajorBossEntry> entries;
    std::string error;
};

struct ActivationPlan {
    bool accepted{true};
    PolicyToggles active;
    std::string error;
};

bool IsPolicyEnabled(const Config& config, PolicyKind policy) noexcept;

MajorBossRegistry ValidateMajorBossRegistry(
    const Config& config,
    std::span<const ResolvedMonStatsIdentity> resolvedMonStats);

bool IsMajorBoss(
    const MajorBossRegistry& registry,
    const RuntimeMonsterFacts& monster) noexcept;

std::optional<MonsterClass> ClassifyCrushingBlow(
    const MajorBossRegistry& registry,
    const RuntimeMonsterFacts& monster) noexcept;

// Compatibility alias for policy consumers created during the incubation lot.
std::optional<MonsterClass> ClassifyMonster(
    const MajorBossRegistry& registry,
    const RuntimeMonsterFacts& monster) noexcept;

DamageFraction CrushingBlowFraction(
    MonsterClass classification,
    bool ranged) noexcept;

std::optional<std::int64_t> ComputeCrushingBlowDamage(
    std::int32_t currentHitpoints,
    DamageFraction fraction,
    std::int32_t playerCountBonusPercent,
    std::int32_t crushingBlowEfficiencyPercent,
    std::int32_t physicalResistancePercent) noexcept;

ActivationPlan BuildActivationPlan(
    const Config& config,
    const MajorBossRegistry& registry);

} // namespace RuffnecKk::BKVCombat
