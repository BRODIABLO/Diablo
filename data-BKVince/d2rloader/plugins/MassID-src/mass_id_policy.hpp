#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace ruffneckk::mass_id {

inline constexpr std::size_t RequestPacketSize = 21;
inline constexpr std::uint8_t CainIdentifyOpcode = 0x34;
inline constexpr std::uint32_t RequestMarker = 0x3144494Du; // "MID1"
inline constexpr std::uint32_t RequestGuard = 0x314B4B52u;  // "RKK1"
inline constexpr std::uint32_t IdentifyTomeCode = 0x206B6269u; // "ibk "
inline constexpr std::uint32_t IdentifiedItemFlag = 0x00000010u;
inline constexpr std::uint32_t QuantityStat = 70;
inline constexpr std::uint8_t InventoryPage = 0;
inline constexpr std::uint8_t CubePage = 3;
inline constexpr std::int32_t LeftClickMouseState = 5;

using RequestPacket = std::array<std::uint8_t, RequestPacketSize>;

constexpr std::uint32_t ReadU32(
        const std::uint8_t* bytes, std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

constexpr void WriteU32(
        RequestPacket& packet, std::size_t offset,
        std::uint32_t value) noexcept {
    packet[offset] = static_cast<std::uint8_t>(value);
    packet[offset + 1] = static_cast<std::uint8_t>(value >> 8);
    packet[offset + 2] = static_cast<std::uint8_t>(value >> 16);
    packet[offset + 3] = static_cast<std::uint8_t>(value >> 24);
}

constexpr RequestPacket MakeRequest(std::uint32_t tomeGuid) noexcept {
    RequestPacket packet{};
    packet[0] = CainIdentifyOpcode;
    WriteU32(packet, 1, tomeGuid);
    WriteU32(packet, 5, RequestMarker);
    WriteU32(packet, 9, RequestGuard);
    return packet;
}

constexpr bool IsPrivateRequest(
        const std::uint8_t* packet, std::int32_t size) noexcept {
    return packet
        && size == static_cast<std::int32_t>(RequestPacketSize)
        && packet[0] == CainIdentifyOpcode
        && ReadU32(packet, 5) == RequestMarker
        && ReadU32(packet, 9) == RequestGuard;
}

constexpr bool IsSupportedInventoryPage(std::uint8_t page) noexcept {
    return page == InventoryPage || page == CubePage;
}

constexpr bool IsRightClickState(std::int32_t mouseState) noexcept {
    return mouseState != LeftClickMouseState;
}

constexpr bool ShouldCaptureGesture(
        bool enabled,
        bool shiftDown,
        bool rightClick,
        bool cursorEmpty,
        bool localOwner,
        bool ownedByLocalInventory,
        std::int32_t unitType,
        std::uint32_t itemCode,
        std::uint8_t page) noexcept {
    return enabled
        && shiftDown
        && rightClick
        && cursorEmpty
        && localOwner
        && ownedByLocalInventory
        && unitType == 4
        && itemCode == IdentifyTomeCode
        && IsSupportedInventoryPage(page);
}

constexpr std::int32_t IdentificationBudget(
        bool freeIdentification, std::int32_t quantity) noexcept {
    if (freeIdentification) return std::numeric_limits<std::int32_t>::max();
    return quantity > 0 ? quantity : 0;
}

} // namespace ruffneckk::mass_id
