#include "book_of_lore_state.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {
using namespace ruffneckk::book_of_lore;

Message MakeMessage(std::string id) {
    Message message;
    message.id = std::move(id);
    message.title = "Title";
    message.text = "Text";
    return message;
}
} // namespace

int main() {
    using namespace ruffneckk::book_of_lore;

    auto threshold = MakeMessage("threshold");
    threshold.filters.difficulty = 2;
    threshold.filters.act = 3;
    threshold.filters.area = 50;
    threshold.filters.quest = "a2q1";
    threshold.filters.playerClass = "ama";
    threshold.filters.minLevel = 20;
    threshold.filters.maxLevel = 40;

    PlayerContext player{
        .playerId = 7,
        .difficulty = 3,
        .act = 4,
        .area = 60,
        .level = 30,
        .playerClass = "ama",
        .completedQuests = {"a2q1"},
        .inTown = false,
    };
    assert(IsEligible(threshold, player));
    player.area = 49;
    assert(!IsEligible(threshold, player));
    player.area = 60;
    player.completedQuests.clear();
    assert(!IsEligible(threshold, player));
    player.completedQuests.insert("a2q1");

    auto ignoredMaximum = threshold;
    ignoredMaximum.filters.minLevel = 20;
    ignoredMaximum.filters.maxLevel = 10;
    player.level = 200;
    assert(IsEligible(ignoredMaximum, player));

    auto town = MakeMessage("town");
    town.filters.town = true;
    town.filters.difficulty = 2;
    town.filters.act = 3;
    town.filters.area = 9999;
    player.inTown = true;
    player.difficulty = 2;
    player.act = 3;
    player.area = 1;
    assert(IsEligible(town, player));
    player.act = 4;
    assert(!IsEligible(town, player));

    auto first = MakeMessage("first");
    auto second = MakeMessage("second");
    std::vector<Message> messages{first, second};
    player = {};
    player.playerId = 1;
    player.playerClass = "sor";
    player.inTown = false;

    SessionSelections state;
    const auto* selected = state.Resolve("book-a", player, messages, 1);
    assert(selected && selected->id == "second");
    selected = state.Resolve("book-a", player, messages, 0);
    assert(selected && selected->id == "second");
    assert(state.BookCount() == 1);

    messages[0].allSame = true;
    SessionSelections sharedState;
    selected = sharedState.Resolve("book-shared", player, messages, 0);
    assert(selected && selected->id == "first");
    auto secondPlayer = player;
    secondPlayer.playerId = 2;
    selected = sharedState.Resolve("book-shared", secondPlayer, messages, 1);
    assert(selected && selected->id == "first");

    auto townPlayer = player;
    townPlayer.inTown = true;
    messages[0].filters.town = true;
    messages[1].filters.town = true;
    SessionSelections townState;
    selected = townState.Resolve("town-book", townPlayer, messages, 0);
    assert(selected && selected->id == "first");
    selected = townState.Resolve("town-book", townPlayer, messages, 1);
    assert(selected && selected->id == "second");
    assert(townState.BookCount() == 0);

    const PlayerVariables variables{
        .name = "Alia",
        .className = "Amazon",
        .level = 32,
        .difficultyName = "Nightmare",
        .act = 3,
        .areaName = "Kurast Bazaar",
        .title = "Slayer",
    };
    assert(ExpandVariables(
        "##00|##01|##02|##03|##04|##05|##06", variables
    ) == "Alia|Amazon|Level 32|Nightmare|Act 3|Kurast Bazaar|Slayer");
    assert(BuildScrollText(first) == "Title\n\nText");

    std::array<std::uint8_t, 0x28> packet{};
    packet[1] = 2;
    packet[6] = 1;
    packet[8] = 0;
    packet[0x0A] = 127;
    assert(IsObjectScrollMessage(packet.data(), 127));
    packet[8] = 3;
    assert(!IsObjectScrollMessage(packet.data(), 127));
    packet[8] = 0;
    packet[1] = 0;
    assert(!IsObjectScrollMessage(packet.data(), 127));
    packet[1] = 2;
    assert(!IsObjectScrollMessage(packet.data(), 126));
    assert(!IsObjectScrollMessage(nullptr, 127));

    state.Clear();
    assert(state.BookCount() == 0);
    return 0;
}
