#pragma once

#include "charm_zone_policy.hpp"

#include <toml++/toml.hpp>

#include <array>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ruffneckk::charm_zone {

struct VisualConfig {
    bool enabled{true};
    float overlayAlpha{0.45f};
    float cellSize{98.0f};
    std::string tooltip{"Inactive outside Charm Zone"};
};

struct Config {
    bool enabled{true};
    Zone zone{};
    VisualConfig visual{};
};

inline bool IsAllowedKey(
    std::string_view key,
    std::initializer_list<std::string_view> allowed) noexcept {
    for (const auto candidate : allowed) {
        if (key == candidate) return true;
    }
    return false;
}

inline void RejectUnknownKeys(
    const toml::table& table,
    std::initializer_list<std::string_view> allowed,
    std::string_view section) {
    for (const auto& [key, unused] : table) {
        (void)unused;
        if (!IsAllowedKey(key.str(), allowed)) {
            throw std::runtime_error(
                "Unknown CharmZone key in " + std::string(section)
                + ": " + std::string(key.str()));
        }
    }
}

template<class T>
T ReadOptional(const toml::table& table, std::string_view key, T fallback) {
    const auto* node = table.get(key);
    if (!node) return fallback;
    const auto value = node->value<T>();
    if (!value) {
        throw std::runtime_error(
            "CharmZone key has the wrong type: " + std::string(key));
    }
    return *value;
}

inline const toml::table* ReadOptionalTable(
    const toml::table& root,
    std::string_view key) {
    const auto* node = root.get(key);
    if (!node) return nullptr;
    const auto* table = node->as_table();
    if (!table) {
        throw std::runtime_error(
            "CharmZone section must be a table: " + std::string(key));
    }
    return table;
}

inline std::uint16_t CheckedU16(std::int64_t value, std::string_view key) {
    if (value < 0 || value > 65535) {
        throw std::runtime_error(
            "CharmZone integer is out of range: " + std::string(key));
    }
    return static_cast<std::uint16_t>(value);
}

inline Config ParseConfig(const toml::table& root) {
    RejectUnknownKeys(root, {"general", "zone", "visual"}, "root");
    Config config{};

    if (const auto* general = ReadOptionalTable(root, "general")) {
        RejectUnknownKeys(*general, {"enabled"}, "general");
        config.enabled = ReadOptional(*general, "enabled", config.enabled);
    }

    if (const auto* zone = ReadOptionalTable(root, "zone")) {
        RejectUnknownKeys(
            *zone,
            {"grid_width", "grid_height", "left", "top", "width", "height"},
            "zone");
        config.zone.gridWidth = CheckedU16(
            ReadOptional<std::int64_t>(*zone, "grid_width", config.zone.gridWidth),
            "grid_width");
        config.zone.gridHeight = CheckedU16(
            ReadOptional<std::int64_t>(*zone, "grid_height", config.zone.gridHeight),
            "grid_height");
        config.zone.left = CheckedU16(
            ReadOptional<std::int64_t>(*zone, "left", config.zone.left), "left");
        config.zone.top = CheckedU16(
            ReadOptional<std::int64_t>(*zone, "top", config.zone.top), "top");
        config.zone.width = CheckedU16(
            ReadOptional<std::int64_t>(*zone, "width", config.zone.width), "width");
        config.zone.height = CheckedU16(
            ReadOptional<std::int64_t>(*zone, "height", config.zone.height), "height");
    }

    if (const auto* visual = ReadOptionalTable(root, "visual")) {
        RejectUnknownKeys(
            *visual,
            {"enabled", "overlay_alpha", "cell_size", "tooltip"},
            "visual");
        config.visual.enabled = ReadOptional(
            *visual, "enabled", config.visual.enabled);
        config.visual.overlayAlpha = static_cast<float>(ReadOptional<double>(
            *visual, "overlay_alpha", config.visual.overlayAlpha));
        config.visual.cellSize = static_cast<float>(ReadOptional<double>(
            *visual, "cell_size", config.visual.cellSize));
        config.visual.tooltip = ReadOptional(
            *visual, "tooltip", config.visual.tooltip);
    }

    if (!IsZoneValid(config.zone)) {
        throw std::runtime_error("CharmZone zone must fit inside the configured grid");
    }
    if (config.visual.overlayAlpha < 0.0f || config.visual.overlayAlpha > 1.0f) {
        throw std::runtime_error("CharmZone overlay_alpha must be between 0 and 1");
    }
    if (config.visual.cellSize < 1.0f || config.visual.cellSize > 512.0f) {
        throw std::runtime_error("CharmZone cell_size must be between 1 and 512");
    }
    if (config.visual.tooltip.empty() || config.visual.tooltip.size() > 127) {
        throw std::runtime_error("CharmZone tooltip must contain 1 to 127 bytes");
    }
    return config;
}

inline Config ParseConfig(std::string_view text) {
    return ParseConfig(toml::parse(text));
}

} // namespace ruffneckk::charm_zone
