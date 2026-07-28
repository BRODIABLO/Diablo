#include "cube_quick_move_policy.hpp"

#include <cassert>

int main() {
    using ruffneckk::cube_quick_move::ShouldRecomputeBottomRight;

    assert(ShouldRecomputeBottomRight(true, 1, 3, 1, 2));
    assert(ShouldRecomputeBottomRight(true, 1, 3, 2, 2));
    assert(ShouldRecomputeBottomRight(true, 1, 3, 2, 3));

    assert(!ShouldRecomputeBottomRight(false, 1, 3, 2, 2));
    assert(!ShouldRecomputeBottomRight(true, 0, 3, 2, 2));
    assert(!ShouldRecomputeBottomRight(true, 1, 0, 2, 2));
    assert(!ShouldRecomputeBottomRight(true, 1, 3, 1, 1));
    assert(!ShouldRecomputeBottomRight(true, 1, 3, 2, 1));
    assert(!ShouldRecomputeBottomRight(true, 1, 3, 0, 2));

    return 0;
}
