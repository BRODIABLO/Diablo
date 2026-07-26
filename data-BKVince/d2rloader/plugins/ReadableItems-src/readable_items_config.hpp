#pragma once

#include <nlohmann/json.hpp>

#include <cctype>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace ruffneckk::readable_items {

inline constexpr std::size_t MaximumItems = 256;
inline constexpr std::size_t MaximumTooltipBytes = 128;
inline constexpr std::size_t MaximumTitleBytes = 128;
inline constexpr std::size_t MaximumTextBytes = 8192;
inline constexpr std::string_view DefaultTooltip = "Right-click to read...";

struct Entry {
    std::uint32_t packedCode{};
    std::string code;
    std::string title;
    std::string text;
};

struct Config {
    bool enabled{true};
    std::string tooltip{DefaultTooltip};
    std::vector<Entry> items;
};

inline void ValidateObject(
    const nlohmann::json& value,
    std::string_view label,
    std::initializer_list<std::string_view> allowed
) {
    if (!value.is_object()) {
        throw std::invalid_argument(std::string(label) + " must be an object");
    }
    for (const auto& [key, ignored] : value.items()) {
        (void)ignored;
        bool known{};
        for (const auto candidate : allowed) {
            if (key == candidate) {
                known = true;
                break;
            }
        }
        if (!known) {
            throw std::invalid_argument(std::string(label) + " has unknown setting: " + key);
        }
    }
}

inline std::string ReadBoundedString(
    const nlohmann::json& object,
    std::string_view key,
    std::string_view label,
    std::size_t maximumBytes
) {
    if (!object.contains(key) || !object.at(key).is_string()) {
        throw std::invalid_argument(std::string(label) + " must be a string");
    }
    auto value = object.at(key).get<std::string>();
    if (value.empty() || value.size() > maximumBytes) {
        throw std::out_of_range(
            std::string(label) + " must contain 1 to " + std::to_string(maximumBytes) + " bytes");
    }
    return value;
}

inline std::uint32_t PackItemCode(std::string_view code) {
    if (code.empty() || code.size() > 4) {
        throw std::out_of_range("item code must contain 1 to 4 ASCII bytes");
    }
    for (const unsigned char character : code) {
        if (character < 0x21 || character > 0x7E) {
            throw std::invalid_argument("item code must contain visible ASCII characters only");
        }
    }

    std::uint32_t packed{};
    std::memcpy(&packed, code.data(), code.size());
    return packed;
}

inline Config ParseConfig(const nlohmann::json& root) {
    ValidateObject(root, "configuration root", {"enabled", "tooltip", "items"});

    Config result;
    if (root.contains("enabled")) {
        if (!root.at("enabled").is_boolean()) {
            throw std::invalid_argument("enabled must be a boolean");
        }
        result.enabled = root.at("enabled").get<bool>();
    }
    if (root.contains("tooltip")) {
        result.tooltip = ReadBoundedString(
            root, "tooltip", "tooltip", MaximumTooltipBytes);
    }
    if (!root.contains("items") || !root.at("items").is_array()) {
        throw std::invalid_argument("items must be an array");
    }
    if (root.at("items").size() > MaximumItems) {
        throw std::out_of_range("items exceeds the 256-entry limit");
    }

    std::unordered_set<std::uint32_t> codes;
    for (std::size_t index{}; index < root.at("items").size(); ++index) {
        const auto& item = root.at("items").at(index);
        const auto label = std::string("items[") + std::to_string(index) + "]";
        ValidateObject(item, label, {"code", "title", "text"});

        Entry entry;
        entry.code = ReadBoundedString(item, "code", label + ".code", 4);
        entry.packedCode = PackItemCode(entry.code);
        entry.title = ReadBoundedString(
            item, "title", label + ".title", MaximumTitleBytes);
        entry.text = ReadBoundedString(
            item, "text", label + ".text", MaximumTextBytes);
        if (!codes.insert(entry.packedCode).second) {
            throw std::invalid_argument("duplicate item code: " + entry.code);
        }
        result.items.push_back(std::move(entry));
    }

    if (result.enabled && result.items.empty()) {
        throw std::invalid_argument("enabled configuration must contain at least one item");
    }
    return result;
}

inline const Entry* FindEntry(const Config& config, std::uint32_t packedCode) noexcept {
    for (const auto& item : config.items) {
        if (item.packedCode == packedCode) return &item;
    }
    return nullptr;
}

} // namespace ruffneckk::readable_items
