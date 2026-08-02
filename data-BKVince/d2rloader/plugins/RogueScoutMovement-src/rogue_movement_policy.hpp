#pragma once

#include <cstdint>

namespace ruffneckk::rogue_movement {

constexpr std::int32_t MonsterUnitType = 1;
constexpr std::int32_t RogueHireClassId = 271;
constexpr std::int32_t FollowMotionType = 0;
constexpr std::int32_t CatchUpMotionType = 1;
constexpr std::int32_t BkvinceBaseVelocity = 11;
constexpr std::int32_t MinimumVelocity = 3;
constexpr std::int32_t MaximumVelocity = 24;

struct Policy {
    bool enabled = true;
    bool walkInTown = true;
    bool runOutsideTown = true;
    std::int32_t townVelocity = BkvinceBaseVelocity;
    std::int32_t outsideVelocity = BkvinceBaseVelocity;
};

struct Decision {
    std::int32_t run{};
    std::int32_t velocityBonusPercent{};
    bool overrideVelocity{};
    bool applies{};
};

constexpr bool IsSupportedVelocity(std::int32_t velocity) noexcept {
    return velocity >= MinimumVelocity && velocity <= MaximumVelocity;
}

constexpr std::int32_t VelocityToBonusPercent(std::int32_t velocity) noexcept {
    const auto scaled = (velocity - BkvinceBaseVelocity) * 100;
    if (scaled > 0) return (scaled + BkvinceBaseVelocity / 2) / BkvinceBaseVelocity;
    if (scaled < 0) return (scaled - BkvinceBaseVelocity / 2) / BkvinceBaseVelocity;
    return 0;
}

constexpr Decision Decide(
    const Policy& policy,
    std::int32_t unitType,
    std::int32_t classId,
    std::int32_t motionType,
    bool inTown,
    std::int32_t nativeRun
) noexcept {
    Decision decision{.run = nativeRun};
    if (!policy.enabled
        || unitType != MonsterUnitType
        || classId != RogueHireClassId
        || (motionType != FollowMotionType && motionType != CatchUpMotionType)) {
        return decision;
    }

    if (inTown && policy.walkInTown) {
        decision.run = 0;
        decision.velocityBonusPercent = VelocityToBonusPercent(policy.townVelocity);
        decision.overrideVelocity = true;
        decision.applies = true;
        return decision;
    }

    if (!inTown && policy.runOutsideTown) {
        decision.run = 1;
        decision.velocityBonusPercent = VelocityToBonusPercent(policy.outsideVelocity);
        decision.overrideVelocity = true;
        decision.applies = true;
    }
    return decision;
}

} // namespace ruffneckk::rogue_movement
