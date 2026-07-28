#include "hotkey_policy.hpp"

#include <cassert>

int main() {
    using namespace ruffneckk::transmute_hotkey;

    Hotkey hotkey{};
    assert(ParseHotkey("CTRL+SHIFT+T", hotkey));
    assert(hotkey.virtualKey == 'T');
    assert(hotkey.device == InputDevice::Keyboard);
    assert(hotkey.control && hotkey.shift && !hotkey.alt);
    assert(ExactModifiersMatch(hotkey, true, true, false));
    assert(!ExactModifiersMatch(hotkey, true, true, true));
    assert(!ExactModifiersMatch(hotkey, true, false, false));

    assert(ParseHotkey("F12", hotkey));
    assert(hotkey.virtualKey == 0x7B);
    assert(ParseHotkey("alt + pageup", hotkey));
    assert(hotkey.virtualKey == 0x21 && hotkey.alt);

    assert(ParseHotkey("MOUSE4", hotkey));
    assert(hotkey.virtualKey == 0x05);
    assert(IsMouseHotkey(hotkey));
    assert(!hotkey.control && !hotkey.shift && !hotkey.alt);
    assert(ExactModifiersMatch(hotkey, false, false, false));
    assert(ParseHotkey("ctrl + mouse 5", hotkey));
    assert(hotkey.virtualKey == 0x06 && hotkey.control);
    assert(ParseHotkey("middle", hotkey));
    assert(hotkey.virtualKey == 0x04 && IsMouseHotkey(hotkey));

    assert(!ParseHotkey("T", hotkey));
    assert(!ParseHotkey("SHIFT+T", hotkey));
    assert(!ParseHotkey("CTRL+CTRL+T", hotkey));
    assert(!ParseHotkey("CTRL+T+U", hotkey));
    assert(!ParseHotkey("MOUSE4+T", hotkey));
    assert(!ParseHotkey("F25", hotkey));
    assert(!ParseHotkey("CTRL+", hotkey));

    assert(IsFreshRequest(1'100, 1'000, 250));
    assert(!IsFreshRequest(1'251, 1'000, 250));
    assert(!IsFreshRequest(999, 1'000, 250));
    assert(!IsFreshRequest(1'000, 0, 250));
    return 0;
}
