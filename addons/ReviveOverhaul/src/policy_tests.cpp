#include "revive_overhaul_policy.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace ruffneckk::revive_overhaul;

int main() {
    static_assert(NativeReviveState == 96);

    constexpr AiPolicy defaults{};
    static_assert(TransformLeashDistance(0, defaults) == 2);
    static_assert(TransformLeashDistance(1, defaults) == 2);
    static_assert(TransformLeashDistance(2, defaults) == 2);
    static_assert(TransformLeashDistance(12, defaults) == 12);
    static_assert(
        TransformLeashDistance(13, defaults) == ForcedCatchUpDistance);
    static_assert(
        TransformLeashDistance(20, defaults) == ForcedCatchUpDistance);
    static_assert(TransformLeashDistance(21, defaults) == 21);
    static_assert(TransformFollowDistance(19, defaults) == 8);
    static_assert(TransformVelocityBonus(40, defaults) == 40);

    static_assert(NeedsReviveTargetValidator({true, true}));
    static_assert(NeedsReviveTargetValidator({true, false}));
    static_assert(NeedsReviveTargetValidator({false, true}));
    static_assert(!NeedsReviveTargetValidator({false, false}));

    constexpr AiPolicy disabled{false, false, 20, 19, 40};
    static_assert(TransformLeashDistance(0, disabled) == 0);
    static_assert(TransformLeashDistance(15, disabled) == 15);
    static_assert(TransformFollowDistance(19, disabled) == 19);
    static_assert(TransformVelocityBonus(40, disabled) == 40);

    constexpr AiPolicy keepScatter{true, false, 12, 8, 80};
    static_assert(TransformLeashDistance(1, keepScatter) == 1);
    static_assert(TransformVelocityBonus(40, keepScatter) == 80);

    const std::uint8_t auraMods[]{5, 17, AuraEnchantedUMod, 0};
    const std::uint8_t noAuraMods[]{5, 17, 0};
    const std::uint8_t fullNoAuraMods[]{1, 2, 3, 4, 5, 6, 7, 8, 9};
    assert(HasAuraEnchantedUMod(auraMods));
    assert(!HasAuraEnchantedUMod(noAuraMods));
    assert(!HasAuraEnchantedUMod(fullNoAuraMods));
    assert(!HasAuraEnchantedUMod(nullptr));

    constexpr std::string_view validToml = R"toml(
enabled = true

[ai]
enabled = true
disable_owner_scatter = true
catch_up_distance = 12
follow_distance = 8
velocity_bonus = 40

[revive]
allow_high_rank_monsters = true
preserve_native_auras = true

[diagnostics]
enabled = false
)toml";
    Config config{};
    std::string error;
    assert(ParseToml(validToml, config, error));
    assert(config.enabled);
    assert(config.ai.enabled);
    assert(config.ai.disableOwnerScatter);
    assert(config.ai.catchUpDistance == 12);
    assert(config.ai.followDistance == 8);
    assert(config.ai.velocityBonus == 40);
    assert(config.revive.allowHighRankMonsters);
    assert(config.revive.preserveNativeAuras);
    assert(!config.diagnostics);

    constexpr std::string_view legacyToml = R"toml(
enabled = false
[ai]
disable_owner_scatter = false
catch_up_distance = 20
follow_distance = 19
velocity_bonus = 255
[diagnostics]
enabled = true
)toml";
    assert(ParseToml(legacyToml, config, error));
    assert(!config.enabled);
    assert(!config.ai.disableOwnerScatter);
    assert(config.ai.catchUpDistance == 20);
    assert(config.ai.followDistance == 19);
    assert(config.ai.velocityBonus == 255);
    assert(config.revive.allowHighRankMonsters);
    assert(config.revive.preserveNativeAuras);
    assert(config.diagnostics);

    assert(!ParseToml("unknown = true", config, error));
    assert(!ParseToml("[ai]\ncatch_up_distance = 21", config, error));
    assert(!ParseToml(
        "[ai]\ncatch_up_distance = 8\nfollow_distance = 8",
        config,
        error));
    assert(!ParseToml(
        "[revive]\npreserve_native_auras = 1",
        config,
        error));

    std::ifstream packagedConfig(
        REVIVE_OVERHAUL_CONFIG_FILE,
        std::ios::binary);
    assert(packagedConfig.is_open());
    const std::string packagedText{
        std::istreambuf_iterator<char>(packagedConfig),
        std::istreambuf_iterator<char>()};
    assert(ParseToml(packagedText, config, error));
    assert(config.enabled);
    assert(config.ai.enabled);
    assert(config.revive.allowHighRankMonsters);
    assert(config.revive.preserveNativeAuras);

    const std::vector<std::filesystem::path> directories{
        L"C:/game/mods/example/d2rloader/config",
        L"C:/game/d2rloader/config",
        L"C:/game/d2rloader/config",
    };
    const auto candidates = BuildConfigCandidates(
        directories,
        L"ruffneckk-revive-overhaul.toml",
        L"ReviveOverhaul.toml");
    assert(candidates.size() == 4);
    assert(candidates[0].filename() == L"ruffneckk-revive-overhaul.toml");
    assert(candidates[1].filename() == L"ReviveOverhaul.toml");
    assert(candidates[2].filename() == L"ruffneckk-revive-overhaul.toml");
    assert(candidates[3].filename() == L"ReviveOverhaul.toml");

    std::ifstream pluginSource(
        REVIVE_OVERHAUL_PLUGIN_SOURCE_FILE,
        std::ios::binary);
    assert(pluginSource.is_open());
    const std::string pluginText{
        std::istreambuf_iterator<char>(pluginSource),
        std::istreambuf_iterator<char>()};
    assert(pluginText.find("GetTargetUnitRva") == std::string::npos);
    assert(pluginText.find("GetTargetUnitExpected") == std::string::npos);
    assert(pluginText.find("GetTargetUnit(") == std::string::npos);
    assert(pluginText.find("ActiveAuraCaptureFrame") != std::string::npos);
    assert(pluginText.find("CaptureNativeAura(target)") != std::string::npos);
}
