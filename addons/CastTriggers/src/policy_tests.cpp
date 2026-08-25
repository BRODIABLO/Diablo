#include "cast_triggers_policy.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace ruffneckk::cast_triggers;

int main() {
    static_assert(IsCastAnimation(PlayerModeCast, PlayerModeCast));
    static_assert(IsCastAnimation(PlayerModeSequence, PlayerModeCast));
    static_assert(!IsCastAnimation(PlayerModeSequence, PlayerModeSequence));
    static_assert(IsEligibleSkillRecord(
        PlayerModeCast,
        PlayerModeCast,
        0));
    static_assert(!IsEligibleSkillRecord(
        PlayerModeCast,
        PlayerModeCast,
        SkillFlagRepeat));
    static_assert(!IsEligibleSkillRecord(7, 7, 0));
    static_assert(IsManualPlayerCast(1, 1, 0, 0, PlayerUnitType));
    static_assert(!IsManualPlayerCast(0, 1, 0, 0, PlayerUnitType));
    static_assert(!IsManualPlayerCast(1, 0, 1, 0, PlayerUnitType));
    static_assert(!IsManualPlayerCast(1, 1, 0, 0, 1));

    constexpr std::string_view validToml = R"toml(
enabled = true
[on_cast]
include_skill_ids = [48, 44, 48]
exclude_skill_ids = [41]
[diagnostics]
enabled = true
)toml";
    Config config{};
    std::string error;
    assert(ParseToml(validToml, config, error));
    assert(config.enabled);
    assert(config.includeSkillIds == std::vector<std::int32_t>({44, 48}));
    assert(config.excludeSkillIds == std::vector<std::int32_t>({41}));
    assert(config.diagnostics);
    assert(IsConfiguredSourceSkill(config, 44));
    assert(!IsConfiguredSourceSkill(config, 41));
    assert(!IsConfiguredSourceSkill(config, 47));

    assert(!ParseToml("unknown = true", config, error));
    assert(!ParseToml(
        "[on_cast]\ninclude_skill_ids = [65536]",
        config,
        error));
    assert(!ParseToml(
        "[on_cast]\nexclude_skill_ids = [\"Inferno\"]",
        config,
        error));
    assert(!ParseToml(
        "[diagnostics]\nenabled = 1",
        config,
        error));

    std::ifstream packagedConfig(
        CAST_TRIGGERS_CONFIG_FILE,
        std::ios::binary);
    assert(packagedConfig.is_open());
    const std::string packagedText{
        std::istreambuf_iterator<char>(packagedConfig),
        std::istreambuf_iterator<char>()};
    assert(ParseToml(packagedText, config, error));
    assert(config.enabled);
    assert(config.includeSkillIds.empty());
    assert(config.excludeSkillIds.empty());
    assert(!config.diagnostics);

    const std::vector<std::filesystem::path> directories{
        L"C:/game/mods/example/d2rloader/config",
        L"C:/game/d2rloader/config",
        L"C:/game/d2rloader/config",
    };
    const auto candidates = BuildConfigCandidates(
        directories,
        L"ruffneckk-cast-triggers.toml");
    assert(candidates.size() == 2);
    assert(candidates.front().filename()
        == L"ruffneckk-cast-triggers.toml");

    std::ifstream pluginSource(CAST_TRIGGERS_PLUGIN_FILE, std::ios::binary);
    assert(pluginSource.is_open());
    const std::string source{
        std::istreambuf_iterator<char>(pluginSource),
        std::istreambuf_iterator<char>()};
    assert(source.find("ModScopedOnly") == std::string::npos);
    assert(source.find("D2RL::PluginFlags::Server") != std::string::npos);
    assert(source.find("IsEligibleSkillRecord") != std::string::npos);
    assert(source.find("SameLevelMarker") != std::string::npos);
}
