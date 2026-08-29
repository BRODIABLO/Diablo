#pragma once

#include <toml++/toml.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <initializer_list>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace RuffnecKk::MapSense {

inline constexpr std::int64_t CurrentConfigSchemaVersion = 9;

inline constexpr float MinimumMonsterMarkerSize = 3.0F;
inline constexpr float MaximumMonsterMarkerSize = 40.0F;
inline constexpr float MinimumMonsterMarkerThickness = 1.0F;
inline constexpr float MaximumMonsterMarkerThickness = 5.0F;
inline constexpr float MinimumImmunityIndicatorSize = 8.0F;
inline constexpr float DefaultImmunityIndicatorSize = 14.0F;
inline constexpr float MaximumImmunityIndicatorSize = 32.0F;
inline constexpr float MinimumImmunityHaloThickness = 1.0F;
inline constexpr float DefaultImmunityHaloThickness = 2.0F;
inline constexpr float MaximumImmunityHaloThickness = 6.0F;
inline constexpr float MinimumNavigationLineThickness = 1.0F;
inline constexpr float DefaultNavigationLineThickness = 2.0F;
inline constexpr float MaximumNavigationLineThickness = 8.0F;
inline constexpr std::int32_t MinimumCustomLevelId = 1;
inline constexpr std::int32_t MaximumCustomLevelId = 65'535;
inline constexpr std::size_t MaximumCustomLevelTargets = 128U;
inline constexpr std::size_t MaximumCustomLevelNameLength = 128U;

struct RgbaColor {
    float red{};
    float green{};
    float blue{};
    float alpha{1.0F};
};

inline constexpr RgbaColor DefaultPhysicalImmunityColor{
    216.0F / 255.0F,
    195.0F / 255.0F,
    154.0F / 255.0F,
    1.0F,
};
inline constexpr RgbaColor LegacyPhysicalImmunityColor{
    199.0F / 255.0F,
    199.0F / 255.0F,
    199.0F / 255.0F,
    1.0F,
};

struct OverlayOptions {
    bool enabled{true};
    bool diagnosticPreview{};
    bool followNativeAutomap{true};
    bool startMenuOpen{};
    float opacity{1.0F};
    float scale{1.0F};
    std::int32_t frameRate{60};
};

enum class MonsterMarkerShape : std::uint8_t {
    X,
    PlayerCross,
    Dot,
};

struct MonsterMarkerStyle {
    RgbaColor color{};
    float size{18.0F};
    MonsterMarkerShape shape{MonsterMarkerShape::PlayerCross};
    float thickness{2.0F};
};

struct MonsterOptions {
    MonsterMarkerStyle normal{{1.0F, 1.0F, 1.0F, 1.0F}, 18.0F};
    MonsterMarkerStyle minion{{1.0F, 0.831F, 0.231F, 1.0F}, 18.0F};
    MonsterMarkerStyle champion{{0.239F, 0.545F, 1.0F, 1.0F}, 20.0F};
    MonsterMarkerStyle unique{{1.0F, 0.541F, 0.141F, 1.0F}, 22.0F};
    MonsterMarkerStyle superUniqueBoss{{1.0F, 0.231F, 0.188F, 1.0F}, 24.0F};
};

enum class ImmunityDisplayStyle : std::uint8_t {
    ColoredI,
    SplitHalo,
};

struct ImmunityOptions {
    bool enabled{true};
    ImmunityDisplayStyle style{ImmunityDisplayStyle::ColoredI};
    float indicatorSize{DefaultImmunityIndicatorSize};
    float haloThickness{DefaultImmunityHaloThickness};
    RgbaColor physical{DefaultPhysicalImmunityColor};
    RgbaColor fire{0.95F, 0.24F, 0.12F, 1.0F};
    RgbaColor cold{0.20F, 0.65F, 1.0F, 1.0F};
    RgbaColor lightning{1.0F, 0.88F, 0.18F, 1.0F};
    RgbaColor poison{0.34F, 0.88F, 0.24F, 1.0F};
    RgbaColor magic{0.78F, 0.36F, 1.0F, 1.0F};
};

using CustomLevelTarget = std::variant<std::int32_t, std::string>;

struct NavigationLineOptions {
    bool enabled{true};
    RgbaColor color{};
};

struct CustomLevelLineOptions {
    bool enabled{};
    RgbaColor color{0.78F, 0.36F, 1.0F, 1.0F};
    std::vector<CustomLevelTarget> targets{};
};

struct NavigationOptions {
    float lineThickness{DefaultNavigationLineThickness};
    NavigationLineOptions waypoint{
        true,
        {0.239F, 0.545F, 1.0F, 1.0F},
    };
    NavigationLineOptions progression{
        true,
        {0.34F, 0.88F, 0.24F, 1.0F},
    };
    NavigationLineOptions quests{
        true,
        {1.0F, 0.231F, 0.188F, 1.0F},
    };
    CustomLevelLineOptions customLevels{};
};

struct HudOptions {
    bool mercenaryHealth{};
    bool sessionTimer{};
    bool experienceTracker{};
    bool showWithAutomapOnly{};
};

struct MenuOptions {
    bool showLauncher{true};
    bool startExpanded{};
    bool rememberPosition{true};
    float positionX{0.86F};
    float positionY{0.04F};
};

struct Config {
    bool enabled{true};
    bool diagnostics{};
    OverlayOptions overlay{};
    MonsterOptions monsters{};
    ImmunityOptions immunities{};
    NavigationOptions navigation{};
    HudOptions hud{};
    MenuOptions menu{};
};

inline auto IsAllowedKey(
        std::string_view key,
        std::initializer_list<std::string_view> allowed) noexcept -> bool {
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
                "Unknown MapSense key in " + std::string(section)
                + ": " + std::string(key.str()));
        }
    }
}

template <class T>
inline auto ReadOptional(
        const toml::table& table,
        std::string_view key,
        T fallback) -> T {
    const auto* node = table.get(key);
    if (node == nullptr) return fallback;
    if constexpr (std::is_same_v<T, bool>) {
        if (!node->is_boolean()) {
            throw std::runtime_error(
                "MapSense key has the wrong type: " + std::string(key));
        }
    } else if constexpr (std::is_integral_v<T>) {
        if (!node->is_integer()) {
            throw std::runtime_error(
                "MapSense key has the wrong type: " + std::string(key));
        }
    }
    const auto value = node->value<T>();
    if (!value) {
        throw std::runtime_error(
            "MapSense key has the wrong type: " + std::string(key));
    }
    return *value;
}

inline auto ReadOptionalFloat(
        const toml::table& table,
        std::string_view key,
        float fallback,
        float minimum,
        float maximum) -> float {
    const auto* node = table.get(key);
    if (node == nullptr) return fallback;
    double value{};
    if (const auto floating = node->value<double>()) {
        value = *floating;
    } else if (const auto integer = node->value<std::int64_t>()) {
        value = static_cast<double>(*integer);
    } else {
        throw std::runtime_error(
            "MapSense key has the wrong type: " + std::string(key));
    }
    if (!std::isfinite(value) || value < minimum || value > maximum) {
        throw std::runtime_error(
            "MapSense key is outside its supported range: "
            + std::string(key));
    }
    return static_cast<float>(value);
}

inline auto ReadOptionalTable(
        const toml::table& root,
        std::string_view key) -> const toml::table* {
    const auto* node = root.get(key);
    if (node == nullptr) return nullptr;
    const auto* table = node->as_table();
    if (table == nullptr) {
        throw std::runtime_error(
            "MapSense section must be a table: " + std::string(key));
    }
    return table;
}

inline auto HexNibble(char character) -> std::uint8_t {
    if (character >= '0' && character <= '9') {
        return static_cast<std::uint8_t>(character - '0');
    }
    if (character >= 'a' && character <= 'f') {
        return static_cast<std::uint8_t>(character - 'a' + 10);
    }
    if (character >= 'A' && character <= 'F') {
        return static_cast<std::uint8_t>(character - 'A' + 10);
    }
    throw std::runtime_error("MapSense color contains a non-hex character");
}

inline auto ParseColor(std::string_view text) -> RgbaColor {
    if ((text.size() != 7 && text.size() != 9) || text.front() != '#') {
        throw std::runtime_error(
            "MapSense colors must use #RRGGBB or #RRGGBBAA");
    }
    const auto component = [text](std::size_t offset) {
        return static_cast<std::uint8_t>(
            (HexNibble(text[offset]) << 4U) | HexNibble(text[offset + 1]));
    };
    constexpr float denominator = 255.0F;
    return {
        static_cast<float>(component(1)) / denominator,
        static_cast<float>(component(3)) / denominator,
        static_cast<float>(component(5)) / denominator,
        text.size() == 9
            ? static_cast<float>(component(7)) / denominator
            : 1.0F,
    };
}

inline auto ReadOptionalColor(
        const toml::table& table,
        std::string_view key,
        RgbaColor fallback) -> RgbaColor {
    const auto* node = table.get(key);
    if (node == nullptr) return fallback;
    const auto value = node->value<std::string>();
    if (!value) {
        throw std::runtime_error(
            "MapSense color must be a string: " + std::string(key));
    }
    return ParseColor(*value);
}

inline auto MonsterMarkerShapeToString(
        MonsterMarkerShape shape) noexcept -> std::string_view {
    switch (shape) {
        case MonsterMarkerShape::X:
            return "x";
        case MonsterMarkerShape::PlayerCross:
            return "player_cross";
        case MonsterMarkerShape::Dot:
            return "dot";
    }
    return "player_cross";
}

inline auto ParseMonsterMarkerShape(
        std::string_view text) -> MonsterMarkerShape {
    if (text == "x") return MonsterMarkerShape::X;
    if (text == "player_cross") return MonsterMarkerShape::PlayerCross;
    if (text == "dot") return MonsterMarkerShape::Dot;
    throw std::runtime_error(
        "MapSense marker shapes must use x, player_cross, or dot");
}

inline auto ReadOptionalMonsterMarkerShape(
        const toml::table& table,
        std::string_view key,
        MonsterMarkerShape fallback) -> MonsterMarkerShape {
    const auto* node = table.get(key);
    if (node == nullptr) return fallback;
    const auto value = node->value<std::string>();
    if (!value) {
        throw std::runtime_error(
            "MapSense marker shape must be a string: " + std::string(key));
    }
    return ParseMonsterMarkerShape(*value);
}

inline auto ImmunityDisplayStyleToString(
        ImmunityDisplayStyle style) noexcept -> std::string_view {
    switch (style) {
        case ImmunityDisplayStyle::ColoredI:
            return "colored_i";
        case ImmunityDisplayStyle::SplitHalo:
            return "split_halo";
    }
    return "colored_i";
}

inline auto ParseImmunityDisplayStyle(
        std::string_view text) -> ImmunityDisplayStyle {
    if (text == "colored_i") return ImmunityDisplayStyle::ColoredI;
    if (text == "split_halo") return ImmunityDisplayStyle::SplitHalo;
    throw std::runtime_error(
        "MapSense immunity styles must use colored_i or split_halo");
}

inline auto ReadOptionalImmunityDisplayStyle(
        const toml::table& table,
        std::string_view key,
        ImmunityDisplayStyle fallback) -> ImmunityDisplayStyle {
    const auto* node = table.get(key);
    if (node == nullptr) return fallback;
    const auto value = node->value<std::string>();
    if (!value) {
        throw std::runtime_error(
            "MapSense immunity style must be a string: "
            + std::string(key));
    }
    return ParseImmunityDisplayStyle(*value);
}

[[nodiscard]] constexpr auto IsSameRgbaColor(
        const RgbaColor& left,
        const RgbaColor& right) noexcept -> bool {
    return left.red == right.red
        && left.green == right.green
        && left.blue == right.blue
        && left.alpha == right.alpha;
}

inline auto ReadMonsterMarkerStyle(
        const toml::table& monsters,
        std::string_view key,
        MonsterMarkerStyle fallback,
        bool readShape,
        bool readThickness) -> MonsterMarkerStyle {
    const auto* style = ReadOptionalTable(monsters, key);
    if (style == nullptr) return fallback;
    if (readShape) {
        fallback.shape = ReadOptionalMonsterMarkerShape(
            *style,
            "shape",
            fallback.shape);
    }
    if (readThickness && fallback.shape != MonsterMarkerShape::Dot) {
        RejectUnknownKeys(
            *style,
            {"shape", "color", "size", "thickness"},
            key);
        fallback.thickness = ReadOptionalFloat(
            *style,
            "thickness",
            fallback.thickness,
            MinimumMonsterMarkerThickness,
            MaximumMonsterMarkerThickness);
    } else if (readShape) {
        RejectUnknownKeys(*style, {"shape", "color", "size"}, key);
    } else {
        RejectUnknownKeys(*style, {"color", "size"}, key);
    }
    fallback.color = ReadOptionalColor(*style, "color", fallback.color);
    fallback.size = ReadOptionalFloat(
        *style,
        "size",
        fallback.size,
        MinimumMonsterMarkerSize,
        MaximumMonsterMarkerSize);
    return fallback;
}

inline auto ReadNavigationLineOptions(
        const toml::table& navigation,
        std::string_view key,
        NavigationLineOptions fallback) -> NavigationLineOptions {
    const auto* line = ReadOptionalTable(navigation, key);
    if (line == nullptr) return fallback;
    RejectUnknownKeys(*line, {"enabled", "color"}, key);
    fallback.enabled = ReadOptional(*line, "enabled", fallback.enabled);
    fallback.color = ReadOptionalColor(*line, "color", fallback.color);
    return fallback;
}

inline void ValidateCustomLevelName(std::string_view name) {
    if (name.empty() || name.size() > MaximumCustomLevelNameLength) {
        throw std::runtime_error(
            "MapSense custom level names must contain 1 to 128 characters");
    }
    const auto isWhitespace = [](char character) noexcept {
        return std::isspace(static_cast<unsigned char>(character)) != 0;
    };
    if (isWhitespace(name.front()) || isWhitespace(name.back())) {
        throw std::runtime_error(
            "MapSense custom level names cannot start or end with whitespace");
    }
    for (const auto character : name) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte < 0x20U || byte == 0x7FU) {
            throw std::runtime_error(
                "MapSense custom level names cannot contain control characters");
        }
    }
}

inline auto ReadCustomLevelTargets(
        const toml::table& customLevels) -> std::vector<CustomLevelTarget> {
    const auto* targetsNode = customLevels.get("targets");
    if (targetsNode == nullptr) return {};
    const auto* targetsArray = targetsNode->as_array();
    if (targetsArray == nullptr) {
        throw std::runtime_error(
            "MapSense navigation.custom_levels.targets must be an array");
    }
    if (targetsArray->size() > MaximumCustomLevelTargets) {
        throw std::runtime_error(
            "MapSense supports at most 128 custom level targets");
    }

    std::vector<CustomLevelTarget> targets;
    targets.reserve(targetsArray->size());
    for (const auto& targetNode : *targetsArray) {
        const auto* target = targetNode.as_table();
        if (target == nullptr) {
            throw std::runtime_error(
                "Each MapSense custom level target must be a table");
        }
        RejectUnknownKeys(*target, {"level_id", "level_name"},
            "navigation.custom_levels.targets");
        const auto hasLevelId = target->contains("level_id");
        const auto hasLevelName = target->contains("level_name");
        if (hasLevelId == hasLevelName) {
            throw std::runtime_error(
                "Each MapSense custom level target must contain exactly one "
                "of level_id or level_name");
        }

        CustomLevelTarget parsedTarget;
        if (hasLevelId) {
            const auto* levelIdNode = target->get("level_id");
            if (levelIdNode == nullptr || !levelIdNode->is_integer()) {
                throw std::runtime_error(
                    "MapSense custom level_id must be an integer");
            }
            const auto levelId = levelIdNode->value<std::int32_t>();
            if (!levelId || *levelId < MinimumCustomLevelId
                    || *levelId > MaximumCustomLevelId) {
                throw std::runtime_error(
                    "MapSense custom level_id is outside its supported range");
            }
            parsedTarget = *levelId;
        } else {
            const auto* levelNameNode = target->get("level_name");
            const auto levelName = levelNameNode != nullptr
                ? levelNameNode->value<std::string>()
                : std::optional<std::string>{};
            if (!levelName) {
                throw std::runtime_error(
                    "MapSense custom level_name must be a string");
            }
            ValidateCustomLevelName(*levelName);
            parsedTarget = *levelName;
        }

        if (std::find(targets.begin(), targets.end(), parsedTarget)
                != targets.end()) {
            throw std::runtime_error(
                "MapSense custom level targets cannot contain duplicates");
        }
        targets.push_back(std::move(parsedTarget));
    }
    return targets;
}

inline auto ReadCustomLevelLineOptions(
        const toml::table& navigation,
        CustomLevelLineOptions fallback) -> CustomLevelLineOptions {
    const auto* customLevels = ReadOptionalTable(
        navigation,
        "custom_levels");
    if (customLevels == nullptr) return fallback;
    RejectUnknownKeys(
        *customLevels,
        {"enabled", "color", "targets"},
        "navigation.custom_levels");
    fallback.enabled = ReadOptional(
        *customLevels,
        "enabled",
        fallback.enabled);
    fallback.color = ReadOptionalColor(
        *customLevels,
        "color",
        fallback.color);
    fallback.targets = ReadCustomLevelTargets(*customLevels);
    return fallback;
}

inline auto ParseConfig(const toml::table& root) -> Config {
    RejectUnknownKeys(
        root,
        {"schema_version", "general", "overlay", "monsters", "immunities", "navigation", "hud", "menu", "diagnostics"},
        "root");

    const auto* schemaNode = root.get("schema_version");
    const auto schemaVersion = schemaNode != nullptr && schemaNode->is_integer()
        ? schemaNode->value<std::int64_t>()
        : std::optional<std::int64_t>{};
    if (!schemaVersion) {
        throw std::runtime_error(
            "MapSense schema_version is required and must be an integer");
    }
    if (*schemaVersion < 1
            || *schemaVersion > CurrentConfigSchemaVersion) {
        throw std::runtime_error("Unsupported MapSense schema_version");
    }
    if (*schemaVersion < 6 && root.contains("navigation")) {
        throw std::runtime_error(
            "MapSense navigation settings require schema_version 6");
    }

    Config config{};
    if (*schemaVersion < 4) {
        // Legacy schemas used the hollow X exclusively. Preserve that
        // appearance while migrating into schema 4.
        config.monsters.normal.shape = MonsterMarkerShape::X;
        config.monsters.minion.shape = MonsterMarkerShape::X;
        config.monsters.champion.shape = MonsterMarkerShape::X;
        config.monsters.unique.shape = MonsterMarkerShape::X;
        config.monsters.superUniqueBoss.shape = MonsterMarkerShape::X;
    }
    if (const auto* general = ReadOptionalTable(root, "general")) {
        RejectUnknownKeys(*general, {"enabled"}, "general");
        config.enabled = ReadOptional(*general, "enabled", config.enabled);
    }
    if (const auto* overlay = ReadOptionalTable(root, "overlay")) {
        RejectUnknownKeys(
            *overlay,
            {"enabled", "diagnostic_preview", "follow_native_automap", "start_menu_open", "opacity", "scale", "frame_rate"},
            "overlay");
        config.overlay.enabled = ReadOptional(
            *overlay, "enabled", config.overlay.enabled);
        config.overlay.diagnosticPreview = ReadOptional(
            *overlay, "diagnostic_preview", config.overlay.diagnosticPreview);
        config.overlay.followNativeAutomap = ReadOptional(
            *overlay, "follow_native_automap", config.overlay.followNativeAutomap);
        config.overlay.startMenuOpen = ReadOptional(
            *overlay, "start_menu_open", config.overlay.startMenuOpen);
        config.overlay.opacity = ReadOptionalFloat(
            *overlay, "opacity", config.overlay.opacity, 0.10F, 1.0F);
        config.overlay.scale = ReadOptionalFloat(
            *overlay, "scale", config.overlay.scale, 0.50F, 2.0F);
        config.overlay.frameRate = ReadOptional<std::int32_t>(
            *overlay, "frame_rate", config.overlay.frameRate);
        if (config.overlay.frameRate < 15 || config.overlay.frameRate > 240) {
            throw std::runtime_error(
                "MapSense key is outside its supported range: frame_rate");
        }
    }
    if (const auto* monsters = ReadOptionalTable(root, "monsters")) {
        if (*schemaVersion == 1 || *schemaVersion == 2) {
            RejectUnknownKeys(
                *monsters,
                {"detection_radius", "normal", "minion", "champion", "unique", "super_unique", "marker_size"},
                "monsters");

            // Schema 1/2 visibility switches are intentionally ignored during
            // migration: schema 4 always displays every monster category.
            (void)ReadOptional(*monsters, "normal", true);
            (void)ReadOptional(*monsters, "minion", true);
            (void)ReadOptional(*monsters, "champion", true);
            (void)ReadOptional(*monsters, "unique", true);
            (void)ReadOptional(*monsters, "super_unique", true);
            const auto legacySize = ReadOptionalFloat(
                *monsters,
                "marker_size",
                config.monsters.normal.size,
                MinimumMonsterMarkerSize,
                MaximumMonsterMarkerSize);
            if (monsters->contains("marker_size")) {
                config.monsters.normal.size = legacySize;
                config.monsters.minion.size = legacySize;
                config.monsters.champion.size = legacySize;
                config.monsters.unique.size = legacySize;
                config.monsters.superUniqueBoss.size = legacySize;
            }
        } else {
            RejectUnknownKeys(
                *monsters,
                {"detection_radius", "normal", "minion", "champion", "unique", "super_unique_boss"},
                "monsters");
            // detection_radius is a retired compatibility key. Keep accepting
            // it in every supported schema, but never read, validate, migrate,
            // or persist its value.
            config.monsters.normal = ReadMonsterMarkerStyle(
                *monsters,
                "normal",
                config.monsters.normal,
                *schemaVersion >= 4,
                *schemaVersion >= 9);
            config.monsters.minion = ReadMonsterMarkerStyle(
                *monsters,
                "minion",
                config.monsters.minion,
                *schemaVersion >= 4,
                *schemaVersion >= 9);
            config.monsters.champion = ReadMonsterMarkerStyle(
                *monsters,
                "champion",
                config.monsters.champion,
                *schemaVersion >= 4,
                *schemaVersion >= 9);
            config.monsters.unique = ReadMonsterMarkerStyle(
                *monsters,
                "unique",
                config.monsters.unique,
                *schemaVersion >= 4,
                *schemaVersion >= 9);
            config.monsters.superUniqueBoss = ReadMonsterMarkerStyle(
                *monsters,
                "super_unique_boss",
                config.monsters.superUniqueBoss,
                *schemaVersion >= 4,
                *schemaVersion >= 9);
        }
    }
    if (const auto* immunities = ReadOptionalTable(root, "immunities")) {
        if (*schemaVersion >= 5) {
            RejectUnknownKeys(
                *immunities,
                {"enabled", "style", "indicator_size", "halo_thickness", "physical", "fire", "cold", "lightning", "poison", "magic"},
                "immunities");
        } else {
            RejectUnknownKeys(
                *immunities,
                {"enabled", "physical", "fire", "cold", "lightning", "poison", "magic"},
                "immunities");
        }
        config.immunities.enabled = ReadOptional(
            *immunities, "enabled", config.immunities.enabled);
        if (*schemaVersion >= 5) {
            config.immunities.style = ReadOptionalImmunityDisplayStyle(
                *immunities,
                "style",
                config.immunities.style);
            config.immunities.indicatorSize = ReadOptionalFloat(
                *immunities,
                "indicator_size",
                config.immunities.indicatorSize,
                MinimumImmunityIndicatorSize,
                MaximumImmunityIndicatorSize);
            config.immunities.haloThickness = ReadOptionalFloat(
                *immunities,
                "halo_thickness",
                config.immunities.haloThickness,
                MinimumImmunityHaloThickness,
                MaximumImmunityHaloThickness);
        }
        config.immunities.physical = ReadOptionalColor(
            *immunities, "physical", config.immunities.physical);
        config.immunities.fire = ReadOptionalColor(
            *immunities, "fire", config.immunities.fire);
        config.immunities.cold = ReadOptionalColor(
            *immunities, "cold", config.immunities.cold);
        config.immunities.lightning = ReadOptionalColor(
            *immunities, "lightning", config.immunities.lightning);
        config.immunities.poison = ReadOptionalColor(
            *immunities, "poison", config.immunities.poison);
        config.immunities.magic = ReadOptionalColor(
            *immunities, "magic", config.immunities.magic);

        // Schema 1-4 shipped Physical as neutral grey. Upgrade only that exact
        // legacy default; any user-selected color remains untouched.
        if (*schemaVersion < 5
                && IsSameRgbaColor(
                    config.immunities.physical,
                    LegacyPhysicalImmunityColor)) {
            config.immunities.physical = DefaultPhysicalImmunityColor;
        }
    }
    if (const auto* navigation = ReadOptionalTable(root, "navigation")) {
        RejectUnknownKeys(
            *navigation,
            {"line_thickness", "waypoint", "progression", "quests", "custom_levels"},
            "navigation");
        config.navigation.lineThickness = ReadOptionalFloat(
            *navigation,
            "line_thickness",
            config.navigation.lineThickness,
            MinimumNavigationLineThickness,
            MaximumNavigationLineThickness);
        config.navigation.waypoint = ReadNavigationLineOptions(
            *navigation,
            "waypoint",
            config.navigation.waypoint);
        config.navigation.progression = ReadNavigationLineOptions(
            *navigation,
            "progression",
            config.navigation.progression);
        config.navigation.quests = ReadNavigationLineOptions(
            *navigation,
            "quests",
            config.navigation.quests);
        config.navigation.customLevels = ReadCustomLevelLineOptions(
            *navigation,
            std::move(config.navigation.customLevels));
    }
    if (*schemaVersion < 8) {
        // Quest lines were reserved and hidden through schema 7. Enable the
        // first real quest adapter once during migration; schema 8 keeps any
        // explicit user choice made in the menu.
        config.navigation.quests.enabled = true;
    }
    if (const auto* hud = ReadOptionalTable(root, "hud")) {
        RejectUnknownKeys(
            *hud,
            {"mercenary_health", "session_timer", "experience_tracker", "show_with_automap_only"},
            "hud");
        config.hud.mercenaryHealth = ReadOptional(
            *hud, "mercenary_health", config.hud.mercenaryHealth);
        config.hud.sessionTimer = ReadOptional(
            *hud, "session_timer", config.hud.sessionTimer);
        config.hud.experienceTracker = ReadOptional(
            *hud, "experience_tracker", config.hud.experienceTracker);
        config.hud.showWithAutomapOnly = ReadOptional(
            *hud, "show_with_automap_only", config.hud.showWithAutomapOnly);
    }
    if (const auto* menu = ReadOptionalTable(root, "menu")) {
        RejectUnknownKeys(
            *menu,
            {"show_launcher", "start_expanded", "remember_position", "position_x", "position_y"},
            "menu");
        config.menu.showLauncher = ReadOptional(
            *menu, "show_launcher", config.menu.showLauncher);
        config.menu.startExpanded = ReadOptional(
            *menu, "start_expanded", config.menu.startExpanded);
        config.menu.rememberPosition = ReadOptional(
            *menu, "remember_position", config.menu.rememberPosition);
        config.menu.positionX = ReadOptionalFloat(
            *menu, "position_x", config.menu.positionX, 0.0F, 1.0F);
        config.menu.positionY = ReadOptionalFloat(
            *menu, "position_y", config.menu.positionY, 0.0F, 1.0F);
    }
    if (const auto* diagnostics = ReadOptionalTable(root, "diagnostics")) {
        RejectUnknownKeys(*diagnostics, {"enabled"}, "diagnostics");
        config.diagnostics = ReadOptional(
            *diagnostics, "enabled", config.diagnostics);
    }
    return config;
}

inline auto ParseConfig(std::string_view text) -> Config {
    return ParseConfig(toml::parse(text));
}

inline auto ColorToHex(RgbaColor color) -> std::string {
    const auto channel = [](float value) {
        return static_cast<unsigned int>(std::lround(
            std::clamp(value, 0.0F, 1.0F) * 255.0F));
    };
    std::ostringstream output;
    output << '#' << std::uppercase << std::hex << std::setfill('0')
           << std::setw(2) << channel(color.red)
           << std::setw(2) << channel(color.green)
           << std::setw(2) << channel(color.blue)
           << std::setw(2) << channel(color.alpha);
    return output.str();
}

inline auto EscapeTomlBasicString(std::string_view text) -> std::string {
    std::string escaped;
    escaped.reserve(text.size());
    for (const auto character : text) {
        switch (character) {
            case '\b':
                escaped += "\\b";
                break;
            case '\t':
                escaped += "\\t";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\f':
                escaped += "\\f";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            default:
                escaped += character;
                break;
        }
    }
    return escaped;
}

inline void SerializeCustomLevelTargets(
        std::ostringstream& output,
        const std::vector<CustomLevelTarget>& targets) {
    output << "targets = [\n";
    for (const auto& target : targets) {
        output << "  { ";
        if (const auto* levelId = std::get_if<std::int32_t>(&target)) {
            output << "level_id = " << *levelId;
        } else {
            const auto& levelName = std::get<std::string>(target);
            output << "level_name = \""
                   << EscapeTomlBasicString(levelName)
                   << "\"";
        }
        output << " },\n";
    }
    output << "]\n";
}

inline auto SerializeConfig(const Config& config) -> std::string {
    std::ostringstream output;
    output << std::boolalpha << std::fixed << std::setprecision(2);
    output
        << "# RuffnecKk MapSense configuration.\n"
        << "# Changes made in the MapSense menu are saved here.\n\n"
        << "schema_version = " << CurrentConfigSchemaVersion << "\n\n"
        << "[general]\n"
        << "enabled = " << config.enabled << "\n\n"
        << "[overlay]\n"
        << "enabled = " << config.overlay.enabled << "\n"
        << "diagnostic_preview = " << config.overlay.diagnosticPreview << "\n"
        << "follow_native_automap = " << config.overlay.followNativeAutomap << "\n"
        << "opacity = " << config.overlay.opacity << "\n"
        << "scale = " << config.overlay.scale << "\n"
        << "frame_rate = " << config.overlay.frameRate << "\n\n"
        << "[monsters]\n\n"
        << "[monsters.normal]\n"
        << "shape = \"" << MonsterMarkerShapeToString(config.monsters.normal.shape)
        << "\"\n"
        << "color = \"" << ColorToHex(config.monsters.normal.color) << "\"\n"
        << "size = " << config.monsters.normal.size << "\n";
    if (config.monsters.normal.shape != MonsterMarkerShape::Dot) {
        output << "thickness = " << config.monsters.normal.thickness << "\n";
    }
    output
        << "\n"
        << "[monsters.minion]\n"
        << "shape = \"" << MonsterMarkerShapeToString(config.monsters.minion.shape)
        << "\"\n"
        << "color = \"" << ColorToHex(config.monsters.minion.color) << "\"\n"
        << "size = " << config.monsters.minion.size << "\n";
    if (config.monsters.minion.shape != MonsterMarkerShape::Dot) {
        output << "thickness = " << config.monsters.minion.thickness << "\n";
    }
    output
        << "\n"
        << "[monsters.champion]\n"
        << "shape = \"" << MonsterMarkerShapeToString(config.monsters.champion.shape)
        << "\"\n"
        << "color = \"" << ColorToHex(config.monsters.champion.color) << "\"\n"
        << "size = " << config.monsters.champion.size << "\n";
    if (config.monsters.champion.shape != MonsterMarkerShape::Dot) {
        output << "thickness = " << config.monsters.champion.thickness << "\n";
    }
    output
        << "\n"
        << "[monsters.unique]\n"
        << "shape = \"" << MonsterMarkerShapeToString(config.monsters.unique.shape)
        << "\"\n"
        << "color = \"" << ColorToHex(config.monsters.unique.color) << "\"\n"
        << "size = " << config.monsters.unique.size << "\n";
    if (config.monsters.unique.shape != MonsterMarkerShape::Dot) {
        output << "thickness = " << config.monsters.unique.thickness << "\n";
    }
    output
        << "\n"
        << "[monsters.super_unique_boss]\n"
        << "shape = \""
        << MonsterMarkerShapeToString(config.monsters.superUniqueBoss.shape)
        << "\"\n"
        << "color = \""
        << ColorToHex(config.monsters.superUniqueBoss.color) << "\"\n"
        << "size = " << config.monsters.superUniqueBoss.size << "\n";
    if (config.monsters.superUniqueBoss.shape != MonsterMarkerShape::Dot) {
        output << "thickness = "
               << config.monsters.superUniqueBoss.thickness << "\n";
    }
    output
        << "\n"
        << "[immunities]\n"
        << "enabled = " << config.immunities.enabled << "\n"
        << "style = \""
        << ImmunityDisplayStyleToString(config.immunities.style)
        << "\"\n"
        << "indicator_size = " << config.immunities.indicatorSize << "\n"
        << "halo_thickness = " << config.immunities.haloThickness << "\n"
        << "physical = \"" << ColorToHex(config.immunities.physical) << "\"\n"
        << "fire = \"" << ColorToHex(config.immunities.fire) << "\"\n"
        << "cold = \"" << ColorToHex(config.immunities.cold) << "\"\n"
        << "lightning = \"" << ColorToHex(config.immunities.lightning) << "\"\n"
        << "poison = \"" << ColorToHex(config.immunities.poison) << "\"\n"
        << "magic = \"" << ColorToHex(config.immunities.magic) << "\"\n\n"
        << "[navigation]\n"
        << "line_thickness = " << config.navigation.lineThickness << "\n\n"
        << "[navigation.waypoint]\n"
        << "enabled = " << config.navigation.waypoint.enabled << "\n"
        << "color = \"" << ColorToHex(config.navigation.waypoint.color)
        << "\"\n\n"
        << "[navigation.progression]\n"
        << "enabled = " << config.navigation.progression.enabled << "\n"
        << "color = \"" << ColorToHex(config.navigation.progression.color)
        << "\"\n\n"
        << "[navigation.quests]\n"
        << "enabled = " << config.navigation.quests.enabled << "\n"
        << "color = \"" << ColorToHex(config.navigation.quests.color)
        << "\"\n\n"
        << "[navigation.custom_levels]\n"
        << "enabled = " << config.navigation.customLevels.enabled << "\n"
        << "color = \"" << ColorToHex(config.navigation.customLevels.color)
        << "\"\n"
        << "# Edit only this destination list manually. Each entry must use\n"
        << "# exactly one level_id or level_name.\n";
    SerializeCustomLevelTargets(
        output,
        config.navigation.customLevels.targets);
    output
        << "\n"
        << "[menu]\n"
        << "show_launcher = " << config.menu.showLauncher << "\n"
        << "start_expanded = " << config.menu.startExpanded << "\n"
        << "remember_position = " << config.menu.rememberPosition << "\n"
        << "position_x = " << config.menu.positionX << "\n"
        << "position_y = " << config.menu.positionY << "\n\n"
        << "[diagnostics]\n"
        << "enabled = " << config.diagnostics << "\n";
    return output.str();
}

} // namespace RuffnecKk::MapSense
