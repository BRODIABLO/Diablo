#include "repeatable-services-policy.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

using namespace RuffnecKk::RepeatableServices;

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Fn>
void RequireThrows(Fn&& function, const char* message) {
    try {
        function();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(message);
}

} // namespace

int main() {
    const auto defaults = ParseConfig(nlohmann::json::object());
    Require(!AnyActive(defaults), "absent config must delegate every service");

    const auto configured = ParseConfig(nlohmann::json::parse(R"json(
        {
          "quests": {
            "repeatableServices": {
              "respec": { "mode": "paid", "goldPerLevel": 3000, "minimumGold": 5000 },
              "imbue": { "mode": "free" },
              "socketing": { "mode": "paid", "goldPerLevel": 1000, "minimumGold": 20000 },
              "personalization": { "mode": "disabled" },
              "diagnostics": true
            }
          }
        }
    )json"));
    Require(AnyActive(configured), "configured services must activate the feature");
    Require(For(configured, Service::Respec).mode == Mode::Paid, "respec mode mismatch");
    Require(For(configured, Service::Imbue).mode == Mode::Free, "imbue mode mismatch");
    Require(Price(For(configured, Service::Respec), 1) == 5000, "minimum price mismatch");
    Require(Price(For(configured, Service::Respec), 10) == 30000, "scaled price mismatch");
    Require(Price(For(configured, Service::Imbue), 99) == 0, "free service must cost zero");
    Require(Price(For(configured, Service::Socketing), -1) == 20000, "negative level clamp mismatch");
    Require(configured.diagnostics, "diagnostics mismatch");
    Require(AvailableGold(25, 75) == 100, "available gold sum mismatch");
    Require(AvailableGold(-1, 75) == 75, "negative carried gold must clamp to zero");
    Require(HasEnoughGold(25, 75, 100), "exact funds must be accepted");
    Require(!HasEnoughGold(25, 74, 100), "insufficient funds must be rejected");
    const ServiceConfig paidDeposit{Mode::Paid, 10, 100};
    Require(
        ShouldBlockItemDeposit(true, true, paidDeposit, 10, 25, 74),
        "an unaffordable paid repeat deposit must be blocked"
    );
    Require(
        !ShouldBlockItemDeposit(true, true, paidDeposit, 10, 25, 75),
        "an exactly affordable paid repeat deposit must remain allowed"
    );
    Require(
        !ShouldBlockItemDeposit(false, true, paidDeposit, 10, 0, 0),
        "removing an item from the service slot must remain allowed"
    );
    Require(
        !ShouldBlockItemDeposit(true, false, paidDeposit, 10, 0, 0),
        "the first native quest reward must remain allowed"
    );
    Require(
        !ShouldBlockItemDeposit(
            true,
            true,
            ServiceConfig{Mode::Free, 10, 100},
            10,
            0,
            0
        ),
        "a free repeat deposit must remain allowed"
    );

    RequireThrows([] {
        ParseConfig(nlohmann::json::parse(R"json(
            { "quests": { "repeatableServices": { "respec": { "mode": "legacy" } } } }
        )json"));
    }, "unknown modes must fail closed");
    RequireThrows([] {
        ParseConfig(nlohmann::json::parse(R"json(
            { "quests": { "repeatableServices": { "imbue": { "goldPerLevel": -1 } } } }
        )json"));
    }, "negative prices must fail closed");

    std::cout << "repeatable-services policy tests passed\n";
    return EXIT_SUCCESS;
}
