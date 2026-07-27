#pragma once

#include "limit_policy.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>

namespace ruffneckk::ground_item_label_limit {

struct Config {
    bool enabled{true};
    std::uint32_t limit{DefaultLimit};
};

inline Config ParseConfig(const nlohmann::json& document) {
    if (!document.is_object()) {
        throw std::invalid_argument("configuration root must be an object");
    }
    for (auto entry = document.begin(); entry != document.end(); ++entry) {
        if (entry.key() != "enabled" && entry.key() != "limit") {
            throw std::invalid_argument(
                std::string("unknown configuration key: ") + entry.key()
            );
        }
    }
    if (document.contains("enabled") && !document.at("enabled").is_boolean()) {
        throw std::invalid_argument("enabled must be a boolean");
    }
    if (document.contains("limit") && !document.at("limit").is_number_integer()) {
        throw std::invalid_argument("limit must be an integer");
    }

    const auto configuredLimit = document.value<std::int64_t>("limit", DefaultLimit);
    if (configuredLimit != static_cast<std::int64_t>(DefaultLimit)
        && configuredLimit != static_cast<std::int64_t>(ExpandedLimit)) {
        throw std::invalid_argument("limit must be exactly 64 or 128");
    }

    return {
        .enabled = document.value("enabled", true),
        .limit = static_cast<std::uint32_t>(configuredLimit),
    };
}

} // namespace ruffneckk::ground_item_label_limit
