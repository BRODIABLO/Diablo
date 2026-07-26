#include "vendor_refresh_policy.hpp"

#include <cassert>

int main() {
    using namespace ruffneckk::vendor_stock_refresh;

    assert(RefreshActionForPanel(false) == 1);
    assert(RefreshActionForPanel(true) == 2);

    assert(ShouldArmNormalRefresh(true, 2, 2, 147, 147, true, true));
    assert(!ShouldArmNormalRefresh(false, 2, 2, 147, 147, true, true));
    assert(!ShouldArmNormalRefresh(true, 3, 2, 147, 147, true, true));
    assert(!ShouldArmNormalRefresh(true, 2, 3, 147, 147, true, true));
    assert(!ShouldArmNormalRefresh(true, 2, 2, 147, 148, true, true));
    assert(!ShouldArmNormalRefresh(true, 2, 2, 147, 147, false, true));
    assert(!ShouldArmNormalRefresh(true, 2, 2, 147, 147, true, false));
}
