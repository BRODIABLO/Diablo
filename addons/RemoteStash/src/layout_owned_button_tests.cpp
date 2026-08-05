#include "layout_owned_button.hpp"

using ruffneckk::remote_stash::IsUsableLayoutOwnedButton;
using ruffneckk::remote_stash::WidgetRect;

#define REQUIRE(condition) do { if (!(condition)) return __LINE__; } while (false)

int main() {
    REQUIRE(IsUsableLayoutOwnedButton({95, 1656, 176, 112}));
    REQUIRE(IsUsableLayoutOwnedButton({-500, -250, 88, 56}));
    REQUIRE(IsUsableLayoutOwnedButton({2000000000, 2000000000, 1, 1}));
    REQUIRE(!IsUsableLayoutOwnedButton({95, 1656, 0, 112}));
    REQUIRE(!IsUsableLayoutOwnedButton({95, 1656, 176, 0}));
    REQUIRE(!IsUsableLayoutOwnedButton({95, 1656, -1, 112}));
    return 0;
}
