#include "socket_tooltip.hpp"

#include <array>
#include <string>
#include <string_view>

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

    struct LocaleCase {
        std::string_view defense;
        std::string_view maxSockets;
        std::string_view baseDefense;
    };
    constexpr std::array locales{
        LocaleCase{"Defense: %d", "Max Sockets: %d", "Base Defense: %d"},
        LocaleCase{"防禦：%d", "最大鑲孔數：%d", "基礎防禦：%d"},
        LocaleCase{"Verteidigung: %d", "Maximale Sockel: %d", "Grundverteidigung: %d"},
        LocaleCase{"Defensa: %d", "Engarces máximos: %d", "Defensa base: %d"},
        LocaleCase{"Défense : %d", "Châsses maximales : %d", "Défense de base : %d"},
        LocaleCase{"Difesa: %d", "Castoni massimi: %d", "Difesa base: %d"},
        LocaleCase{"방어력: %d", "최대 홈: %d", "기본 방어력: %d"},
        LocaleCase{"Obrona: %d", "Maksymalna liczba gniazd: %d", "Bazowa obrona: %d"},
        LocaleCase{"Defensa: %d", "Engarces máximos: %d", "Defensa base: %d"},
        LocaleCase{"防御力: %d", "最大ソケット数: %d", "基本防御力: %d"},
        LocaleCase{"Defesa: %d", "Soquetes máximos: %d", "Defesa base: %d"},
        LocaleCase{"Защита: %d", "Максимум гнезд: %d", "Базовая защита: %d"},
        LocaleCase{"防御: %d", "最大镶孔数：%d", "基础防御：%d"},
    };
    for (const auto& locale : locales) {
        const auto localized = tcp::tooltips::BuildTooltipLocalization({},
            [&](std::string_view key) {
                return key == "ItemStats1h" ? std::string(locale.defense) : std::string{};
            });
        CHECK(localized.nativeReady);
        CHECK(localized.maxSocketsFormat == locale.maxSockets);
        CHECK(localized.baseDefenseFormat == locale.baseDefense);
        CHECK(tcp::tooltips::FormatMaxSocketsLine(4, 0, localized)
            == white + tcp::tooltips::FormatLocalizedInteger(locale.maxSockets, 4));
        const auto defenseLine = tcp::tooltips::FormatLocalizedInteger(locale.defense, 99);
        const auto localizedTooltip = std::string("Item\n") + defenseLine + "\nName";
        CHECK(tcp::tooltips::FindMaxSocketsInsertion(localizedTooltip, localized)
            == localizedTooltip.find(defenseLine));
    }

    CHECK(tcp::tooltips::MatchesLocalizedTemplate(
        "방어력 +55% 증가", "방어력 %+d%% 증가"));
    CHECK(!tcp::tooltips::MatchesLocalizedTemplate(
        "방어력 +55% 증가 / 투사체", "방어력 %+d%% 증가"));
    CHECK(tcp::tooltips::MatchesLocalizedTemplate(
        "装備時にオーラ〈聖なる衝撃〉（レベル15）を発動",
        "装備時にオーラ〈%1〉（レベル%0）を発動"));
#undef CHECK
}
