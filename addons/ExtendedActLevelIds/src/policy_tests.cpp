#include "extended_act_level_ids_policy.hpp"

#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>

int main() {
    using namespace ruffneckk::extended_act_level_ids;

    static_assert(LevelsIdOffset == 0x00);
    static_assert(LevelsActOffset == 0x0D);
    static_assert(LevelsRowSize == 0x18C);
    static_assert(!IsSupportedDataContext(0));
    static_assert(IsSupportedDataContext(1));
    static_assert(IsSupportedDataContext(2));
    static_assert(IsSupportedDataContext(3));
    static_assert(!IsSupportedDataContext(4));

    Config config{};
    std::string error;
    assert(ParseConfig(R"({"enabled":true})", config, error));
    assert(config.enabled);
    assert(ParseConfig(R"({"enabled":false})", config, error));
    assert(!config.enabled);
    assert(ParseConfig(R"({})", config, error));
    assert(config.enabled);
    assert(!ParseConfig(R"({"enabled":1})", config, error));
    assert(!ParseConfig(R"({"unknown":true})", config, error));
    assert(!ParseConfig(R"([true])", config, error));
    assert(!ParseConfig("{/* comment */\"enabled\":true}", config, error));
    assert(!ParseConfig("{", config, error));

    const auto modLocalCandidates = BuildConfigCandidates(
        "C:/D2R/mods/BKVince/d2rloader/config",
        "C:/D2R/mods/BKVince/d2rloader/config",
        "C:/D2R/d2rloader/config",
        "ruffneckk-extended-act-level-ids.json");
    assert(modLocalCandidates.size() == 2);
    assert(modLocalCandidates[0].generic_string()
        == "C:/D2R/mods/BKVince/d2rloader/config/ruffneckk-extended-act-level-ids.json");
    assert(modLocalCandidates[1].generic_string()
        == "C:/D2R/d2rloader/config/ruffneckk-extended-act-level-ids.json");

    const auto globalCandidates = BuildConfigCandidates(
        {},
        "C:/D2R/d2rloader/config",
        "C:/D2R/d2rloader/config",
        "ruffneckk-extended-act-level-ids.json");
    assert(globalCandidates.size() == 1);

    constexpr std::array<ActEntry, 6> entries{{
        {1, 0},
        {40, 1},
        {75, 2},
        {103, 3},
        {109, 4},
        {147, 0},
    }};
    const auto entrySpan = std::span<const ActEntry>(entries);
    assert(HasValidAnchorActs(entrySpan));
    assert(FindAct(entrySpan, 147) == 0);
    assert(!FindAct(entrySpan, 146));
    assert(!FindAct(entrySpan, -1));

    auto brokenAnchors = entries;
    brokenAnchors[3].act = 4;
    assert(!HasValidAnchorActs(
        std::span<const ActEntry>(brokenAnchors)));

    auto invalidAct = entries;
    invalidAct.back().act = 5;
    assert(!FindAct(std::span<const ActEntry>(invalidAct), 147));

    std::ifstream configInput(EXTENDED_ACT_CONFIG_FILE, std::ios::binary);
    assert(configInput.is_open());
    const std::string configText{
        std::istreambuf_iterator<char>(configInput),
        std::istreambuf_iterator<char>()};
    assert(ParseConfig(configText, config, error));
    assert(config.enabled);

    std::ifstream pluginInput(EXTENDED_ACT_PLUGIN_FILE, std::ios::binary);
    assert(pluginInput.is_open());
    const std::string pluginText{
        std::istreambuf_iterator<char>(pluginInput),
        std::istreambuf_iterator<char>()};
    assert(pluginText.find("PluginFlags::Shared | D2RL::PluginFlags::NativeHooks")
        != std::string::npos);
    assert(pluginText.find("ModScopedOnly") == std::string::npos);
    assert(pluginText.find("AllowedBuild") == std::string::npos);
    assert(pluginText.find("SupportedBuild") == std::string::npos);
    assert(pluginText.find("GetBuildName") != std::string::npos);
    assert(pluginText.find("ResolveActFromLevelIdExpected")
        != std::string::npos);
    assert(pluginText.find("LevelsRowSize") != std::string::npos);
    assert(pluginText.find("HasValidAnchorActs") != std::string::npos);
    assert(pluginText.find("ResolveProbe") != std::string::npos);
    assert(pluginText.find("source=%s") != std::string::npos);

    return 0;
}
