#include "bkv_combat_policy.hpp"

#include <algorithm>
#include <iterator>
#include <limits>

namespace RuffnecKk::BKVCombat {
namespace {

constexpr std::int32_t MeleeDenominator(MonsterClass classification) noexcept {
    switch (classification) {
    case MonsterClass::Ordinary:
        return 6;
    case MonsterClass::Elite:
        return 8;
    case MonsterClass::PrimeEvil:
        return 16;
    case MonsterClass::MajorBoss:
        return 20;
    }
    return 20;
}

constexpr std::int32_t RangedDenominator(
    const std::int32_t meleeDenominator) noexcept {
    // PD2/BKVince policy: ranged uses a 1.5x larger denominator.
    return meleeDenominator * 3 / 2;
}

static_assert(RangedDenominator(6) == 9);
static_assert(RangedDenominator(8) == 12);
static_assert(RangedDenominator(16) == 24);
static_assert(RangedDenominator(20) == 30);

} // namespace

bool IsPolicyEnabled(const Config& config, PolicyKind policy) noexcept {
    if (!config.enabled) return false;
    switch (policy) {
    case PolicyKind::CriticalStrike:
        return config.policies.criticalStrike;
    case PolicyKind::DeadlyStrike:
        return config.policies.deadlyStrike;
    case PolicyKind::CrushingBlow:
        return config.policies.crushingBlow;
    case PolicyKind::LifeSteal:
        return config.policies.lifeSteal;
    case PolicyKind::ManaSteal:
        return config.policies.manaSteal;
    case PolicyKind::OpenWounds:
        return config.policies.openWounds;
    }
    return false;
}

MajorBossRegistry ValidateMajorBossRegistry(
    const Config& config,
    std::span<const ResolvedMonStatsIdentity> resolvedMonStats) {
    MajorBossRegistry result;
    if (config.classifications.majorBosses.size() != RequiredMajorBossCount) {
        result.error = "the MajorBoss configuration does not contain ten entries";
        return result;
    }

    for (const auto& expected : config.classifications.majorBosses) {
        const auto first = std::find_if(
            resolvedMonStats.begin(),
            resolvedMonStats.end(),
            [&](const ResolvedMonStatsIdentity& actual) {
                return actual.monstats == expected.monstats;
            });
        if (first == resolvedMonStats.end()) {
            result.error =
                "missing monstats key '" + expected.monstats + "'";
            return result;
        }
        const auto duplicate = std::find_if(
            std::next(first),
            resolvedMonStats.end(),
            [&](const ResolvedMonStatsIdentity& actual) {
                return actual.monstats == expected.monstats;
            });
        if (duplicate != resolvedMonStats.end()) {
            result.error =
                "duplicate runtime monstats key '" + expected.monstats + "'";
            return result;
        }
        if (first->monstatsId != expected.expectedId) {
            result.error =
                "monstats ID mismatch for '" + expected.monstats + "'";
            return result;
        }
    }

    result.valid = true;
    result.entries = config.classifications.majorBosses;
    return result;
}

bool IsMajorBoss(
    const MajorBossRegistry& registry,
    const RuntimeMonsterFacts& monster) noexcept {
    if (!registry.valid) return false;
    return std::any_of(
        registry.entries.begin(),
        registry.entries.end(),
        [&](const MajorBossEntry& expected) {
            return expected.expectedId == monster.monstatsId
                && expected.monstats == monster.monstats;
        });
}

std::optional<MonsterClass> ClassifyCrushingBlow(
    const MajorBossRegistry& registry,
    const RuntimeMonsterFacts& monster) noexcept {
    if (!registry.valid) return std::nullopt;
    if (IsMajorBoss(registry, monster)) return MonsterClass::MajorBoss;
    if (monster.primeEvil) return MonsterClass::PrimeEvil;
    if (monster.heraldOrAscendant || monster.champion || monster.unique
        || monster.superUnique || monster.boss) {
        return MonsterClass::Elite;
    }
    return MonsterClass::Ordinary;
}

std::optional<MonsterClass> ClassifyMonster(
    const MajorBossRegistry& registry,
    const RuntimeMonsterFacts& monster) noexcept {
    return ClassifyCrushingBlow(registry, monster);
}

DamageFraction CrushingBlowFraction(
    MonsterClass classification,
    bool ranged) noexcept {
    const auto melee = MeleeDenominator(classification);
    return {
        .numerator = 1,
        .denominator = ranged ? RangedDenominator(melee) : melee,
    };
}

std::optional<std::int64_t> ComputeCrushingBlowDamage(
        const std::int32_t currentHitpoints,
        const DamageFraction fraction,
        const std::int32_t playerCountBonusPercent,
        const std::int32_t crushingBlowEfficiencyPercent,
        const std::int32_t physicalResistancePercent) noexcept {
    if (currentHitpoints < 0
            || fraction.numerator <= 0
            || fraction.denominator <= 0
            || playerCountBonusPercent < 0) {
        return std::nullopt;
    }
    const auto efficiency = std::max(crushingBlowEfficiencyPercent, 0);
    if (efficiency > std::numeric_limits<std::int32_t>::max() - 100
            || playerCountBonusPercent
                > std::numeric_limits<std::int32_t>::max() - 100) {
        return std::nullopt;
    }

    auto numerator = static_cast<std::int64_t>(currentHitpoints);
    if (numerator > std::numeric_limits<std::int64_t>::max()
            / fraction.numerator) {
        return std::nullopt;
    }
    numerator *= fraction.numerator;
    const auto efficiencyScale = static_cast<std::int64_t>(100 + efficiency);
    if (numerator > std::numeric_limits<std::int64_t>::max()
            / efficiencyScale) {
        return std::nullopt;
    }
    numerator *= efficiencyScale;

    auto denominator = static_cast<std::int64_t>(fraction.denominator);
    const auto playerScale = static_cast<std::int64_t>(
        100 + playerCountBonusPercent);
    if (denominator > std::numeric_limits<std::int64_t>::max()
            / playerScale) {
        return std::nullopt;
    }
    denominator *= playerScale;
    if (denominator <= 0) return std::nullopt;

    auto damage = numerator / denominator;
    const auto resistance = std::clamp(physicalResistancePercent, 0, 100);
    if (resistance != 0) {
        if (damage > std::numeric_limits<std::int64_t>::max() / resistance) {
            return std::nullopt;
        }
        damage -= damage * resistance / 100;
    }
    return std::max<std::int64_t>(damage, 0);
}

ActivationPlan BuildActivationPlan(
    const Config& config,
    const MajorBossRegistry& registry) {
    ActivationPlan result;
    if (!config.enabled) return result;
    if (config.policies.crushingBlow && !registry.valid) {
        result.accepted = false;
        result.error = "Crushing Blow rejected: " + registry.error;
        return result;
    }
    result.active = config.policies;
    return result;
}

} // namespace RuffnecKk::BKVCombat
