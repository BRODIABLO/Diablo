#pragma once

#include <toml++/toml.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace ruffneckk::book_of_lore {

inline constexpr char ConfigFileName[] = "BookOfLore.toml";
inline constexpr std::uint32_t DefaultScrollSpeed = 18;
inline constexpr std::size_t MaximumMessages = 4096;
inline constexpr std::size_t MaximumMessageIdBytes = 64;
inline constexpr std::size_t MaximumTitleBytes = 128;
inline constexpr std::size_t MaximumTextBytes = 65536;

struct MessageFilters {
    std::optional<std::uint32_t> difficulty;
    std::optional<std::uint32_t> act;
    std::optional<std::uint32_t> area;
    std::optional<std::string> quest;
    std::optional<std::string> playerClass;
    std::optional<std::uint32_t> minLevel;
    std::optional<std::uint32_t> maxLevel;
    bool town{};
};

struct Message {
    std::string id;
    std::string title;
    std::string text;
    std::uint32_t scrollSpeed{DefaultScrollSpeed};
    bool allSame{};
    MessageFilters filters;
};

struct Config {
    bool enabled{};
    std::vector<Message> messages;
};

struct LoadedConfiguration {
    Config config;
    std::optional<std::filesystem::path> path;
};

inline void RejectUnknownKeys(
        const toml::table& table,
        std::initializer_list<std::string_view> allowed,
        std::string_view label) {
    for (const auto& [key, value] : table) {
        (void)value;
        bool known{};
        for (const auto candidate : allowed) {
            if (key.str() == candidate) {
                known = true;
                break;
            }
        }
        if (!known) {
            throw std::invalid_argument(
                std::string(label) + " contains unknown key: "
                + std::string(key.str())
            );
        }
    }
}

inline std::string RequireString(
        const toml::table& table,
        std::string_view key,
        std::size_t maximumBytes) {
    const auto* node = table.get(key);
    const auto value = node ? node->value<std::string>() : std::nullopt;
    if (!value) {
        throw std::invalid_argument(std::string(key) + " must be a string");
    }
    if (value->empty()) {
        throw std::invalid_argument(std::string(key) + " must not be empty");
    }
    if (value->size() > maximumBytes) {
        throw std::invalid_argument(std::string(key) + " exceeds its byte limit");
    }
    return *value;
}

inline std::optional<std::uint32_t> OptionalUnsigned(
        const toml::table& table,
        std::string_view key,
        std::uint32_t minimum,
        std::uint32_t maximum) {
    const auto* node = table.get(key);
    if (!node) return std::nullopt;
    const auto value = node->value<std::int64_t>();
    if (!value) {
        throw std::invalid_argument(std::string(key) + " must be an integer");
    }
    if (*value < static_cast<std::int64_t>(minimum)
        || *value > static_cast<std::int64_t>(maximum)) {
        throw std::invalid_argument(
            std::string(key) + " is outside the accepted range"
        );
    }
    return static_cast<std::uint32_t>(*value);
}

inline bool OptionalBoolean(
        const toml::table& table,
        std::string_view key,
        bool fallback = false) {
    const auto* node = table.get(key);
    if (!node) return fallback;
    const auto value = node->value<bool>();
    if (!value) {
        throw std::invalid_argument(std::string(key) + " must be a boolean");
    }
    return *value;
}

inline bool IsSafeMessageId(std::string_view id) noexcept {
    for (const unsigned char value : id) {
        const bool alphaNumeric = (value >= 'a' && value <= 'z')
            || (value >= 'A' && value <= 'Z')
            || (value >= '0' && value <= '9');
        if (!alphaNumeric && value != '.' && value != '_' && value != '-') return false;
    }
    return !id.empty();
}

inline bool IsPlayerClassCode(std::string_view code) noexcept {
    if (code.size() != 3) return false;
    for (const unsigned char value : code) {
        if (value < 'a' || value > 'z') return false;
    }
    return true;
}

inline MessageFilters ParseFilters(const toml::table& table) {
    RejectUnknownKeys(
        table,
        {"difficulty", "act", "area", "quest", "player_class",
         "min_level", "max_level", "town"},
        "filters"
    );
    MessageFilters filters;
    filters.difficulty = OptionalUnsigned(table, "difficulty", 1, 3);
    filters.act = OptionalUnsigned(table, "act", 1, 5);
    filters.area = OptionalUnsigned(table, "area", 1, 0x7FFFFFFFu);
    filters.minLevel = OptionalUnsigned(table, "min_level", 1, 255);
    filters.maxLevel = OptionalUnsigned(table, "max_level", 1, 255);
    filters.town = OptionalBoolean(table, "town");

    if (table.contains("quest")) {
        filters.quest = RequireString(table, "quest", MaximumMessageIdBytes);
    }
    if (table.contains("player_class")) {
        auto playerClass = RequireString(table, "player_class", 3);
        if (!IsPlayerClassCode(playerClass)) {
            throw std::invalid_argument(
                "player_class must contain exactly three lowercase ASCII letters"
            );
        }
        filters.playerClass = std::move(playerClass);
    }
    return filters;
}

inline Message ParseMessage(const toml::table& table) {
    RejectUnknownKeys(
        table,
        {"id", "title", "text", "scroll_speed", "all_same", "filters"},
        "message"
    );
    Message message;
    message.id = RequireString(table, "id", MaximumMessageIdBytes);
    if (!IsSafeMessageId(message.id)) {
        throw std::invalid_argument(
            "id may contain only ASCII letters, digits, dots, underscores and hyphens"
        );
    }
    message.title = RequireString(table, "title", MaximumTitleBytes);
    message.text = RequireString(table, "text", MaximumTextBytes);
    message.scrollSpeed = OptionalUnsigned(table, "scroll_speed", 1, 1000)
        .value_or(DefaultScrollSpeed);
    message.allSame = OptionalBoolean(table, "all_same");
    if (const auto* filters = table.get_as<toml::table>("filters")) {
        message.filters = ParseFilters(*filters);
    } else if (table.contains("filters")) {
        throw std::invalid_argument("filters must be a table");
    }
    return message;
}

inline Config ParseConfig(const toml::table& root) {
    RejectUnknownKeys(root, {"enabled", "messages"}, "configuration root");
    Config config;
    config.enabled = OptionalBoolean(root, "enabled");

    const auto* messagesNode = root.get("messages");
    if (!messagesNode) return config;
    const auto* messages = messagesNode->as_array();
    if (!messages) {
        throw std::invalid_argument("messages must be an array of tables");
    }
    if (messages->size() > MaximumMessages) {
        throw std::invalid_argument("messages exceeds the entry limit");
    }

    std::unordered_set<std::string> ids;
    for (const auto& entry : *messages) {
        const auto* table = entry.as_table();
        if (!table) {
            throw std::invalid_argument("each message must be a table");
        }
        auto message = ParseMessage(*table);
        if (!ids.emplace(message.id).second) {
            throw std::invalid_argument("duplicate message id: " + message.id);
        }
        config.messages.push_back(std::move(message));
    }
    return config;
}

inline Config LoadConfigFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("could not open configuration file");
    }
    return ParseConfig(toml::parse(input, path.string()));
}

inline std::vector<std::filesystem::path> BuildConfigCandidates(
        const std::optional<std::filesystem::path>& modDirectory,
        const std::filesystem::path& globalDirectory = {}) {
    std::vector<std::filesystem::path> candidates;
    if (modDirectory && !modDirectory->empty()) {
        candidates.emplace_back(*modDirectory / ConfigFileName);
    }
    const auto globalPath = globalDirectory.empty()
        ? std::filesystem::path(ConfigFileName)
        : globalDirectory / ConfigFileName;
    if (candidates.empty() || candidates.back() != globalPath) {
        candidates.emplace_back(globalPath);
    }
    return candidates;
}

inline LoadedConfiguration LoadConfiguration(
        const std::optional<std::filesystem::path>& modDirectory,
        const std::filesystem::path& globalDirectory = {}) {
    for (const auto& path : BuildConfigCandidates(modDirectory, globalDirectory)) {
        std::error_code error;
        const auto exists = std::filesystem::exists(path, error);
        if (error) {
            throw std::runtime_error(
                "could not inspect " + path.string() + " (" + error.message() + ")"
            );
        }
        if (!exists) continue;
        if (!std::filesystem::is_regular_file(path, error) || error) {
            throw std::runtime_error(path.string() + " is not a readable regular file");
        }
        try {
            return {LoadConfigFile(path), path};
        } catch (const std::exception& exception) {
            throw std::invalid_argument(
                "invalid " + path.string() + " (" + exception.what() + ")"
            );
        }
    }
    return {};
}

} // namespace ruffneckk::book_of_lore
