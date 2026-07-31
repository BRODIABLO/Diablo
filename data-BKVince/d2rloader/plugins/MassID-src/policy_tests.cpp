#include "mass_id_policy.hpp"

#include <cassert>
#include <limits>

int main() {
    using namespace ruffneckk::mass_id;

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
    assert(IsRightClickState(4));
    assert(!IsRightClickState(LeftClickMouseState));

    assert(ShouldCaptureGesture(
        true, true, true, true, true, true, 4,
        IdentifyTomeCode, InventoryPage));
    assert(!ShouldCaptureGesture(
        true, false, true, true, true, true, 4,
        IdentifyTomeCode, InventoryPage));
    assert(!ShouldCaptureGesture(
        true, true, false, true, true, true, 4,
        IdentifyTomeCode, InventoryPage));
    assert(!ShouldCaptureGesture(
        true, true, true, false, true, true, 4,
        IdentifyTomeCode, InventoryPage));
    assert(!ShouldCaptureGesture(
        true, true, true, true, true, true, 4,
        0x206B6274u, InventoryPage));

    assert(IdentificationBudget(false, -1) == 0);
    assert(IdentificationBudget(false, 0) == 0);
    assert(IdentificationBudget(false, 7) == 7);
    assert(IdentificationBudget(true, 0)
        == std::numeric_limits<std::int32_t>::max());
    return 0;
}
