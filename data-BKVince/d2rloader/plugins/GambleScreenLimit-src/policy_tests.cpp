#include "limit_policy.hpp"

#include <cassert>

int main() {
    using namespace tcp::gamble_screen_limit;
    static_assert(VanillaLimit == 14);
    static_assert(ExpandedLimit == 32);
    assert(EffectiveLimit(false) == VanillaLimit);
    assert(EffectiveLimit(true) == ExpandedLimit);
    return 0;
}
