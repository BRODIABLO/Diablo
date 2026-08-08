#pragma once
#include <array>
#include <cstdint>
#include <limits>
#include <string_view>

namespace ruffneckk::potion_auto_pickup {
enum class Family : std::uint8_t { Healing, Mana, Rejuvenation, Unknown };
struct Item { std::string_view code; Family family; std::uint8_t tier; };
inline constexpr std::array Items{
    Item{"hp1",Family::Healing,1}, Item{"hp2",Family::Healing,2}, Item{"hp3",Family::Healing,3}, Item{"hp4",Family::Healing,4}, Item{"hp5",Family::Healing,5},
    Item{"mp1",Family::Mana,1}, Item{"mp2",Family::Mana,2}, Item{"mp3",Family::Mana,3}, Item{"mp4",Family::Mana,4}, Item{"mp5",Family::Mana,5},
    Item{"rvs",Family::Rejuvenation,1}, Item{"rvl",Family::Rejuvenation,2},
};
inline constexpr std::uint32_t PackItemCode(std::string_view code) noexcept {
    std::uint32_t packed=0x20202020;
    for(std::size_t index=0;index<4 && index<code.size();++index) {
        const auto shift=static_cast<std::uint32_t>(index*8);
        packed=(packed & ~(0xFFu<<shift))
            | (static_cast<std::uint32_t>(static_cast<std::uint8_t>(code[index]))<<shift);
    }
    return packed;
}
inline constexpr Item Classify(std::string_view code) noexcept {
    for (const auto& item : Items) if (item.code == code) return item;
    return {code, Family::Unknown, 0};
}
struct Policy {
    bool enabled{};
    std::array<bool,6> tiers{};
    std::array<std::uint8_t,4> columns{};
    std::uint8_t columnCount{};
    std::array<bool,6> overflowTiers{};
    constexpr bool Accepts(Item item) const noexcept { return enabled && item.family != Family::Unknown && item.tier < tiers.size() && tiers[item.tier]; }
    constexpr bool AllowsOverflow(Item item) const noexcept { return Accepts(item) && item.tier < overflowTiers.size() && overflowTiers[item.tier]; }
};
struct BeltSlot { bool occupied{}; Family family{Family::Unknown}; };
struct RoutingToken {
    static constexpr std::uint32_t InvalidGuid=std::numeric_limits<std::uint32_t>::max();
    std::uint32_t itemGuid{InvalidGuid};
    constexpr bool Matches(std::uint32_t actualGuid) const noexcept {
        return itemGuid!=InvalidGuid && itemGuid==actualGuid;
    }
    constexpr void Reset() noexcept { itemGuid=InvalidGuid; }
};
enum class Destination : std::int8_t { Ground=-1, Inventory=0, Column1=1, Column2=2, Column3=3, Column4=4 };
struct RouteResult { Destination destination{Destination::Ground}; std::int8_t beltSlot{-1}; };
inline constexpr std::int8_t ChooseBeltSlot(
    const Policy& policy,
    Item item,
    const std::array<BeltSlot,16>& slots,
    std::uint8_t capacity) noexcept {
    if (!policy.Accepts(item)) return -1;
    if (capacity < 4 || capacity > slots.size() || capacity % 4 != 0) return -1;
    const auto rows = static_cast<std::uint8_t>(capacity / 4);
    for (std::uint8_t index=0; index<policy.columnCount; ++index) {
        const auto column = policy.columns[index];
        if (column < 1 || column > 4) continue;
        const auto bottom = static_cast<std::uint8_t>(column - 1);
        if (!slots[bottom].occupied || slots[bottom].family != item.family) continue;
        for (std::uint8_t row=1; row<rows; ++row) {
            const auto slot = static_cast<std::uint8_t>(bottom + row * 4);
            if (!slots[slot].occupied) return static_cast<std::int8_t>(slot);
        }
    }
    for (std::uint8_t index=0; index<policy.columnCount; ++index) {
        const auto column = policy.columns[index];
        if (column < 1 || column > 4) continue;
        const auto bottom = static_cast<std::uint8_t>(column - 1);
        if (!slots[bottom].occupied) return static_cast<std::int8_t>(bottom);
    }
    return -1;
}
inline constexpr RouteResult Route(
    const Policy& policy,
    Item item,
    const std::array<BeltSlot,16>& slots,
    std::uint8_t capacity,
    bool inventoryHasRoom) noexcept {
    const auto slot = ChooseBeltSlot(policy,item,slots,capacity);
    if (slot >= 0) {
        return {static_cast<Destination>((slot % 4) + 1),slot};
    }
    if (policy.AllowsOverflow(item) && inventoryHasRoom) return {Destination::Inventory,-1};
    return {Destination::Ground,-1};
}
}
