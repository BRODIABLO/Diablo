#include "equipped_item_to_cube_policy.hpp"

#include <algorithm>
#include <cassert>

int main() {
    using namespace ruffneckk::equipped_item_to_cube;

    assert(EligibilityGuardBranchRva == 0x228B98);
    assert(EligibilityGuardBranchExpected.size() == 6);
    assert(EligibilityGuardBranchExpected[0] == 0x0F);
    assert(EligibilityGuardBranchExpected[1] == 0x84);
    assert(std::all_of(
        EligibilityGuardBranchReplacement.begin(),
        EligibilityGuardBranchReplacement.end(),
        [](std::uint8_t value) { return value == 0x90; }
    ));
    assert(ShouldInstallPatch(true));
    assert(!ShouldInstallPatch(false));

    return 0;
}
