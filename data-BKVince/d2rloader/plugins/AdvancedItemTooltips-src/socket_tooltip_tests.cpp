#include "socket_tooltip.hpp"

#include <string>

int main() {
#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)
    const std::string white = "\xEE\x81\xBE" "0";
    CHECK(tcp::tooltips::FormatMaxSocketsLine(6, 0) == white + "Max Sockets: 6");
    CHECK(tcp::tooltips::FormatMaxSocketsLine(3, 0) == white + "Max Sockets: 3");
    CHECK(tcp::tooltips::FormatMaxSocketsLine(0, 0).empty());

    // Socketing an item must not suppress this line. Besides keeping the
    // information useful, this guarantees that socket handling cannot short-
    // circuit the rest of the final-tooltip enhancement pipeline.
    CHECK(tcp::tooltips::FormatMaxSocketsLine(3, 3) == white + "Max Sockets: 3");

    const std::string weaponTooltip =
        "Required Dexterity: 25\n"
        "Durability: 32 of 32\n"
        "One-Hand Damage: 5 to 12\n"
        "Sabre [12]";
    CHECK(tcp::tooltips::FindMaxSocketsInsertion(weaponTooltip)
        == weaponTooltip.find("One-Hand Damage:"));

    const std::string throwingWeaponTooltip =
        "Durability: 24 of 24\n"
        "One-Hand Damage: 6 to 11\n"
        "Throw Damage: 12 to 30\n"
        "Throwing Axe";
    CHECK(tcp::tooltips::FindMaxSocketsInsertion(throwingWeaponTooltip)
        == throwingWeaponTooltip.find("One-Hand Damage:"));

    const std::string armorTooltip =
        "Required Strength: 30\n"
        "Durability: 24 of 24\n"
        "Defense: 48\n"
        "Scale Mail";
    CHECK(tcp::tooltips::FindMaxSocketsInsertion(armorTooltip)
        == armorTooltip.find("Defense:"));

    CHECK(tcp::tooltips::FindMaxSocketsInsertion("Required Level: 5\nRing")
        == tcp::tooltips::NoSocketLineInsertion);
#undef CHECK
}
