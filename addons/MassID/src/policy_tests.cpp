#include "mass_id_policy.hpp"

#include <array>
#include <cassert>
#include <limits>

int main() {
    using namespace ruffneckk::mass_id;

    const auto modLocalConfigPaths = BuildConfigCandidates(
        "C:/D2R/mods/TestMod/d2rloader/config",
        "C:/D2R/mods/TestMod/d2rloader/config",
        "C:/D2R/d2rloader/config",
        "MassID.json");
    assert(modLocalConfigPaths.size() == 2);
    assert(modLocalConfigPaths[0].generic_string()
        == "C:/D2R/mods/TestMod/d2rloader/config/MassID.json");
    assert(modLocalConfigPaths[1].generic_string()
        == "C:/D2R/d2rloader/config/MassID.json");

    const auto globalConfigPaths = BuildConfigCandidates(
        {},
        "C:/D2R/d2rloader/config",
        "C:/D2R/d2rloader/config",
        "MassID.json");
    assert(globalConfigPaths.size() == 1);
    assert(globalConfigPaths[0].generic_string()
        == "C:/D2R/d2rloader/config/MassID.json");

    const auto packet = MakeRequest(0x12345678u);
    assert(IsPrivateRequest(packet.data(), static_cast<std::int32_t>(packet.size())));
    assert(ReadU32(packet.data(), 1) == 0x12345678u);
    assert(!IsPrivateRequest(packet.data(), 20));

    auto wrongMarker = packet;
    WriteU32(wrongMarker, 9, 0);
    assert(!IsPrivateRequest(
        wrongMarker.data(), static_cast<std::int32_t>(wrongMarker.size())));

    assert(IsSupportedInventoryPage(InventoryPage));
    assert(IsSupportedInventoryPage(CubePage));
    assert(!IsSupportedInventoryPage(4));
    assert(IsMassIdentifyTargetPage(InventoryPage));
    assert(IsMassIdentifyTargetPage(CubePage));
    assert(IsMassIdentifyTargetPage(StashPage));
    assert(!IsMassIdentifyTargetPage(2));

    constexpr TargetSelection allTargets{};
    static_assert(IncludesTarget(allTargets, TargetContainer::Inventory));
    static_assert(IncludesTarget(allTargets, TargetContainer::Cube));
    static_assert(IncludesTarget(allTargets, TargetContainer::PersonalStash));
    static_assert(IncludesTarget(allTargets, TargetContainer::SharedStash));

    constexpr TargetSelection inventoryOnly{false, false, false};
    static_assert(IncludesTarget(inventoryOnly, TargetContainer::Inventory));
    static_assert(!IncludesTarget(inventoryOnly, TargetContainer::Cube));
    static_assert(!IncludesTarget(
        inventoryOnly, TargetContainer::PersonalStash));
    static_assert(!IncludesTarget(
        inventoryOnly, TargetContainer::SharedStash));

    constexpr TargetSelection inventoryAndCube{true, false, false};
    static_assert(IncludesTarget(
        inventoryAndCube, TargetContainer::Inventory));
    static_assert(IncludesTarget(inventoryAndCube, TargetContainer::Cube));
    static_assert(!IncludesTarget(
        inventoryAndCube, TargetContainer::PersonalStash));
    static_assert(!IncludesTarget(
        inventoryAndCube, TargetContainer::SharedStash));

    std::array<std::uint8_t, ItemDataInventoryPageOffset + 1> itemData{};
    itemData[ItemDataInventoryPageOffset] = StashPage;
    assert(ReadInventoryPageFromItemData(itemData.data()) == StashPage);
    assert(ReadInventoryPageFromItemData(nullptr) == InvalidInventoryPage);

    assert(ShouldCaptureGesture(
        true, false, true, true, true, 4, IdentifyTomeCode));
    assert(!ShouldCaptureGesture(
        true, false, false, true, true, 4, IdentifyTomeCode));
    assert(ShouldCaptureGesture(
        true, true, false, true, true, 4, IdentifyTomeCode));
    assert(ShouldCaptureGesture(
        true, true, true, true, true, 4, IdentifyTomeCode));
    assert(!ShouldCaptureGesture(
        true, true, true, false, true, 4, IdentifyTomeCode));
    assert(!ShouldCaptureGesture(
        true, true, true, true, false, 4, IdentifyTomeCode));
    assert(!ShouldCaptureGesture(
        true, true, true, true, true, 4, 0x206B6274u));

    static_assert(ShouldShowMassIdTooltip(true, false));
    static_assert(!ShouldShowMassIdTooltip(true, true));
    static_assert(!ShouldShowMassIdTooltip(false, false));

    constexpr std::string_view text = "Shift + Right Click to Mass ID";
    const auto tooltip = AddMassIdTooltipLine("Ctrl + Left Click to Drop", text);
    assert(tooltip == "Ctrl + Left Click to Drop\nShift + Right Click to Mass ID");
    assert(AddMassIdTooltipLine(tooltip, text) == tooltip);

    assert(IdentificationBudget(false, -1) == 0);
    assert(IdentificationBudget(false, 0) == 0);
    assert(IdentificationBudget(false, 7) == 7);
    assert(IdentificationBudget(true, 0)
        == std::numeric_limits<std::int32_t>::max());
    return 0;
}
