#include "hotkey_policy.hpp"

#include <fstream>
#include <stdexcept>

#define REQUIRE(condition) do { if (!(condition)) return __LINE__; } while (false)

namespace {

template<class Callback>
bool Throws(Callback&& callback) {
    try {
        callback();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    using namespace ruffneckk::remote_stash;

    Hotkey hotkey{};
    REQUIRE(ParseHotkey("CTRL+SHIFT+S", hotkey));
    REQUIRE(hotkey.virtualKey == 'S');
    REQUIRE(hotkey.device == InputDevice::Keyboard);
    REQUIRE(hotkey.control && hotkey.shift && !hotkey.alt);
    REQUIRE(ExactModifiersMatch(hotkey, true, true, false));
    REQUIRE(!ExactModifiersMatch(hotkey, true, true, true));
    REQUIRE(ParseHotkey("MOUSE4", hotkey));
    REQUIRE(hotkey.virtualKey == 0x05 && IsMouseHotkey(hotkey));
    REQUIRE(ParseHotkey("ctrl + mouse 5", hotkey));
    REQUIRE(hotkey.virtualKey == 0x06 && hotkey.control);
    REQUIRE(ParseHotkey("F12", hotkey));
    REQUIRE(hotkey.virtualKey == 0x7B);
    REQUIRE(!ParseHotkey("CTRL+CTRL+S", hotkey));
    REQUIRE(!ParseHotkey("S+H", hotkey));
    REQUIRE(!ParseHotkey("F25", hotkey));

    REQUIRE(ShouldConsumeMatchedHotkey(true, true));
    REQUIRE(!ShouldConsumeMatchedHotkey(true, false));
    REQUIRE(!ShouldConsumeMatchedHotkey(false, true));
    REQUIRE(ShouldRestoreIndependentInventory(true, false));
    REQUIRE(!ShouldRestoreIndependentInventory(true, true));
    REQUIRE(!ShouldRestoreIndependentInventory(false, false));
    REQUIRE(!ShouldRestoreIndependentInventory(false, true));
    REQUIRE(!ResolveRemoteStashTransitionFlag(true, true));
    REQUIRE(!ResolveRemoteStashTransitionFlag(true, false));
    REQUIRE(ResolveRemoteStashTransitionFlag(false, true));
    REQUIRE(!ResolveRemoteStashTransitionFlag(false, false));
    REQUIRE(ResolveHotkeyDispatch(false, false) == HotkeyDispatch::Open);
    REQUIRE(ResolveHotkeyDispatch(true, false) == HotkeyDispatch::Close);
    REQUIRE(ResolveHotkeyDispatch(false, true) == HotkeyDispatch::Refuse);
    REQUIRE(ResolveHotkeyDispatch(true, true) == HotkeyDispatch::Refuse);
    REQUIRE(ShouldSuppressRemoteStashClose(true, true, false, false));
    REQUIRE(!ShouldSuppressRemoteStashClose(false, true, false, false));
    REQUIRE(!ShouldSuppressRemoteStashClose(true, false, false, false));
    REQUIRE(!ShouldSuppressRemoteStashClose(true, true, true, false));
    REQUIRE(!ShouldSuppressRemoteStashClose(true, true, false, true));
    REQUIRE(ShouldSuppressMovementInventoryClose(true, true, true, true));
    REQUIRE(!ShouldSuppressMovementInventoryClose(false, true, true, true));
    REQUIRE(!ShouldSuppressMovementInventoryClose(true, false, true, true));
    REQUIRE(!ShouldSuppressMovementInventoryClose(true, true, false, true));
    REQUIRE(!ShouldSuppressMovementInventoryClose(true, true, true, false));

    REQUIRE(ResolveCompanionInventoryClose(0, true, 100)
        == CompanionInventoryCloseDecision::Wait);
    REQUIRE(ResolveCompanionInventoryClose(200, false, 100)
        == CompanionInventoryCloseDecision::Wait);
    REQUIRE(ResolveCompanionInventoryClose(200, true, 100)
        == CompanionInventoryCloseDecision::Close);
    REQUIRE(ResolveCompanionInventoryClose(200, true, 200)
        == CompanionInventoryCloseDecision::Close);
    REQUIRE(ResolveCompanionInventoryClose(200, true, 201)
        == CompanionInventoryCloseDecision::Expire);


    const auto modLocalPaths = BuildConfigCandidates(
        "C:/D2R/mods/TestMod/d2rloader/config",
        "C:/D2R/mods/TestMod/d2rloader/config",
        "C:/D2R/d2rloader/config",
        "RemoteStash.json");
    REQUIRE(modLocalPaths.size() == 2);
    REQUIRE(modLocalPaths[0].generic_string()
        == "C:/D2R/mods/TestMod/d2rloader/config/RemoteStash.json");
    REQUIRE(modLocalPaths[1].generic_string()
        == "C:/D2R/d2rloader/config/RemoteStash.json");

    const auto globalPaths = BuildConfigCandidates(
        {},
        "C:/D2R/d2rloader/config",
        "C:/D2R/d2rloader/config",
        "RemoteStash.json");
    REQUIRE(globalPaths.size() == 1);
    REQUIRE(globalPaths[0].generic_string()
        == "C:/D2R/d2rloader/config/RemoteStash.json");

    const auto defaults = ParseHotkeyConfig(nlohmann::json::object());
    REQUIRE(!defaults.enabled);
    REQUIRE(defaults.hotkeyText == "S");
    REQUIRE(defaults.consume);

    const auto enabled = ParseHotkeyConfig(nlohmann::json::parse(
        R"json({"enabled":true,"hotkey":"MOUSE4","consume":false})json"));
    REQUIRE(enabled.enabled && IsMouseHotkey(enabled.hotkey));
    REQUIRE(!enabled.consume);
    REQUIRE(Throws([] {
        ParseHotkeyConfig(nlohmann::json::parse(
            R"json({"enabled":true,"unknown":1})json"));
    }));
    REQUIRE(Throws([] {
        ParseHotkeyConfig(nlohmann::json::parse(
            R"json({"enabled":"true"})json"));
    }));
    REQUIRE(Throws([] {
        ParseHotkeyConfig(nlohmann::json::parse(
            R"json({"hotkey":12})json"));
    }));
    REQUIRE(Throws([] {
        ParseHotkeyConfig(nlohmann::json::parse(
            R"json({"consume":"yes"})json"));
    }));

    REQUIRE(argc == 2);
    std::ifstream shippedConfig(argv[1]);
    REQUIRE(shippedConfig.is_open());
    const auto shipped = ParseHotkeyConfig(
        nlohmann::json::parse(shippedConfig, nullptr, true, true));
    REQUIRE(!shipped.enabled);
    REQUIRE(shipped.hotkeyText == "S");
    REQUIRE(shipped.consume);
    return 0;
}
