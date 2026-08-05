#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace RuffnecKk::RepeatableServices {

enum class Mode : std::uint8_t {
    Disabled,
    Free,
    Paid,
};

enum class Service : std::uint8_t {
    Respec,
    Imbue,
    Socketing,
    Personalization,
    Count,
};

struct ServiceConfig {
    Mode mode{Mode::Disabled};
    std::uint32_t goldPerLevel{};
    std::uint32_t minimumGold{};
};

struct Config {
    std::array<ServiceConfig, static_cast<std::size_t>(Service::Count)> services{};
    bool diagnostics{};
};

constexpr std::size_t Index(Service service) noexcept {
    return static_cast<std::size_t>(service);
}

inline const ServiceConfig& For(const Config& config, Service service) noexcept {
    return config.services[Index(service)];
}

inline ServiceConfig& For(Config& config, Service service) noexcept {
    return config.services[Index(service)];
}

constexpr std::string_view ServiceName(Service service) noexcept {
    switch (service) {
    case Service::Respec: return "respec";
    case Service::Imbue: return "imbue";
    case Service::Socketing: return "socketing";
    case Service::Personalization: return "personalization";
    default: return "unknown";
    }
}

constexpr bool IsActive(const ServiceConfig& config) noexcept {
    return config.mode != Mode::Disabled;
}

constexpr bool AnyActive(const Config& config) noexcept {
    for (const auto& service : config.services) {
        if (IsActive(service)) return true;
    }
    return false;
}

constexpr std::uint32_t Price(const ServiceConfig& config, std::int32_t playerLevel) noexcept {
    if (config.mode != Mode::Paid) return 0;
    const auto level = static_cast<std::uint64_t>(std::max(playerLevel, 0));
    const auto scaled = level * config.goldPerLevel;
    const auto selected = std::max<std::uint64_t>(config.minimumGold, scaled);
    return static_cast<std::uint32_t>(std::min<std::uint64_t>(
        selected,
        static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())
    ));
}

constexpr std::uint64_t AvailableGold(
    std::int32_t carriedGold,
    std::int32_t personalStashGold
) noexcept {
    return static_cast<std::uint64_t>(std::max(carriedGold, 0))
        + static_cast<std::uint64_t>(std::max(personalStashGold, 0));
}

constexpr bool HasEnoughGold(
    std::int32_t carriedGold,
    std::int32_t personalStashGold,
    std::uint32_t price
) noexcept {
    return AvailableGold(carriedGold, personalStashGold) >= price;
}

constexpr bool ShouldBlockItemDeposit(
    bool emptySlot,
    bool repeat,
    const ServiceConfig& config,
    std::int32_t playerLevel,
    std::int32_t carriedGold,
    std::int32_t personalStashGold
) noexcept {
    return emptySlot
        && repeat
        && config.mode == Mode::Paid
        && !HasEnoughGold(
            carriedGold,
            personalStashGold,
            Price(config, playerLevel)
        );
}

inline Mode ParseMode(std::string_view value) {
    if (value == "disabled") return Mode::Disabled;
    if (value == "free") return Mode::Free;
    if (value == "paid") return Mode::Paid;
    throw std::invalid_argument("mode must be disabled, free, or paid");
}

inline std::uint32_t ReadUnsigned(
    const nlohmann::json& object,
    const char* key,
    std::uint32_t fallback
) {
    const auto entry = object.find(key);
    if (entry == object.end()) return fallback;
    if (!entry->is_number_unsigned() && !entry->is_number_integer()) {
        throw std::invalid_argument(std::string(key) + " must be a non-negative integer");
    }
    const auto value = entry->get<std::int64_t>();
    if (value < 0 || value > 100000000) {
        throw std::invalid_argument(std::string(key) + " must be between 0 and 100000000");
    }
    return static_cast<std::uint32_t>(value);
}

inline ServiceConfig ParseService(
    const nlohmann::json& parent,
    const char* key,
    std::uint32_t defaultPerLevel,
    std::uint32_t defaultMinimum
) {
    const auto entry = parent.find(key);
    if (entry == parent.end()) {
        return ServiceConfig{Mode::Disabled, defaultPerLevel, defaultMinimum};
    }
    if (!entry->is_object()) {
        throw std::invalid_argument(std::string(key) + " must be an object");
    }
    const auto modeEntry = entry->find("mode");
    if (modeEntry != entry->end() && !modeEntry->is_string()) {
        throw std::invalid_argument(std::string(key) + ".mode must be a string");
    }
    return ServiceConfig{
        ParseMode(entry->value("mode", "disabled")),
        ReadUnsigned(*entry, "goldPerLevel", defaultPerLevel),
        ReadUnsigned(*entry, "minimumGold", defaultMinimum),
    };
}

inline Config ParseConfig(const nlohmann::json& root) {
    if (!root.is_object()) {
        throw std::invalid_argument("configuration root must be an object");
    }
    const auto quests = root.find("quests");
    if (quests == root.end()) return {};
    if (!quests->is_object()) {
        throw std::invalid_argument("quests must be an object");
    }
    const auto repeatable = quests->find("repeatableServices");
    if (repeatable == quests->end()) return {};
    if (!repeatable->is_object()) {
        throw std::invalid_argument("quests.repeatableServices must be an object");
    }

    Config config{};
    For(config, Service::Respec) = ParseService(*repeatable, "respec", 3000, 5000);
    For(config, Service::Imbue) = ParseService(*repeatable, "imbue", 500, 5000);
    For(config, Service::Socketing) = ParseService(*repeatable, "socketing", 1000, 20000);
    For(config, Service::Personalization) = ParseService(
        *repeatable,
        "personalization",
        500,
        10000
    );
    const auto diagnostics = repeatable->find("diagnostics");
    if (diagnostics != repeatable->end() && !diagnostics->is_boolean()) {
        throw std::invalid_argument("quests.repeatableServices.diagnostics must be a boolean");
    }
    config.diagnostics = repeatable->value("diagnostics", false);
    return config;
}

} // namespace RuffnecKk::RepeatableServices
