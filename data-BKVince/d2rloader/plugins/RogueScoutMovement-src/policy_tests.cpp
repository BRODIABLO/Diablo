#include "rogue_movement_policy.hpp"

#include <cassert>

using namespace ruffneckk::rogue_movement;

int main() {
    constexpr Policy defaults{};
    static_assert(VelocityToBonusPercent(11) == 0);
    static_assert(VelocityToBonusPercent(12) == 9);
    static_assert(VelocityToBonusPercent(10) == -9);
    static_assert(IsSupportedVelocity(3));
    static_assert(IsSupportedVelocity(24));
    static_assert(!IsSupportedVelocity(2));
    static_assert(!IsSupportedVelocity(25));

    const auto town = Decide(defaults, MonsterUnitType, RogueHireClassId, 0, true, 1);
    assert(town.applies);
    assert(town.run == 0);
    assert(town.overrideVelocity);
    assert(town.velocityBonusPercent == 0);

    const auto wilderness = Decide(defaults, MonsterUnitType, RogueHireClassId, 1, false, 0);
    assert(wilderness.applies);
    assert(wilderness.run == 1);
    assert(wilderness.overrideVelocity);
    assert(wilderness.velocityBonusPercent == 0);

    const auto otherMercenary = Decide(defaults, MonsterUnitType, 338, 1, false, 0);
    assert(!otherMercenary.applies);
    assert(otherMercenary.run == 0);

    const auto combatMotion = Decide(defaults, MonsterUnitType, RogueHireClassId, 4, false, 0);
    assert(!combatMotion.applies);
    assert(combatMotion.run == 0);

    Policy disabled = defaults;
    disabled.enabled = false;
    assert(!Decide(disabled, MonsterUnitType, RogueHireClassId, 0, true, 1).applies);

    Policy nativeTown = defaults;
    nativeTown.walkInTown = false;
    const auto preservedTown = Decide(nativeTown, MonsterUnitType, RogueHireClassId, 0, true, 1);
    assert(!preservedTown.applies);
    assert(preservedTown.run == 1);

    Policy faster = defaults;
    faster.outsideVelocity = 14;
    const auto fastRun = Decide(faster, MonsterUnitType, RogueHireClassId, 1, false, 0);
    assert(fastRun.applies);
    assert(fastRun.velocityBonusPercent == 27);

    return 0;
}
