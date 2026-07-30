#include "layout_owned_button.hpp"

#include <cassert>

using ruffneckk::remote_stash::IsUsableLayoutOwnedButton;
using ruffneckk::remote_stash::WidgetRect;

int main() {
    assert(IsUsableLayoutOwnedButton({95, 1656, 176, 112}));
    assert(IsUsableLayoutOwnedButton({-500, -250, 88, 56}));
    assert(IsUsableLayoutOwnedButton({2000000000, 2000000000, 1, 1}));
    assert(!IsUsableLayoutOwnedButton({95, 1656, 0, 112}));
    assert(!IsUsableLayoutOwnedButton({95, 1656, 176, 0}));
    assert(!IsUsableLayoutOwnedButton({95, 1656, -1, 112}));
    return 0;
}
