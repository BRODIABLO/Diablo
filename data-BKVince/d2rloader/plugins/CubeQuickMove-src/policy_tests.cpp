#include "cube_quick_move_policy.hpp"

#include <array>
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

    using ruffneckk::cube_quick_move::TryFindBottomRight;
    std::array<std::uintptr_t, 12> cells{};
    std::int32_t x{-1};
    std::int32_t y{-1};

    assert(TryFindBottomRight(cells.data(), 3, 4, 2, 2, &x, &y));
    assert(x == 1 && y == 2);

    cells[2 + 3 * 3] = 1;
    assert(TryFindBottomRight(cells.data(), 3, 4, 2, 2, &x, &y));
    assert(x == 1 && y == 1);

    cells.fill(1);
    assert(!TryFindBottomRight(cells.data(), 3, 4, 2, 2, &x, &y));
    assert(!TryFindBottomRight(nullptr, 3, 4, 2, 2, &x, &y));

    return 0;
}
