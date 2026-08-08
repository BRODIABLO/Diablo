#include "static-field-rework-policy.hpp"

#include <cassert>
#include <filesystem>

using namespace RuffnecKk::StaticFieldRework;

int main() {
    assert(ShouldApplyDebuff(true, 1, StaticFieldSkillId, 1));
    assert(!ShouldApplyDebuff(false, 1, StaticFieldSkillId, 1));
    assert(!ShouldApplyDebuff(true, 0, StaticFieldSkillId, 1));
    assert(!ShouldApplyDebuff(true, 1, StaticFieldSkillId + 1, 1));
    assert(!ShouldApplyDebuff(true, 1, StaticFieldSkillId, 0));

    const auto candidates = BuildConfigCandidates(
        L"C:/D2R/mods/BKVince/d2rloader/config",
        L"C:/D2R/mods/BKVince/d2rloader/config",
        L"C:/D2R/d2rloader/config",
        L"StaticFieldRework.toml");
    assert(candidates.size() == 2);
    assert(candidates[0].filename() == L"StaticFieldRework.toml");
    assert(candidates[1].filename() == L"StaticFieldRework.toml");
    return 0;
}
