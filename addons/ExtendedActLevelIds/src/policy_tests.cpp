#include "extended_act_level_ids_policy.hpp"

#include <array>
#include <cassert>
#include <fstream>
#include <iterator>
#include <span>
#include <string>

int main() {
    using namespace ruffneckk::extended_act_level_ids;

    static_assert(LevelsIdOffset == 0x00);
    static_assert(LevelsActOffset == 0x0D);
    static_assert(LevelsRowSize == 0x18C);
    static_assert(MaximumCompiledLevelRecords == 1023);
    static_assert(MaximumLevelId == 1022);
    static_assert(!HasValidLevelRecordCount(0));
    static_assert(HasValidLevelRecordCount(1));
    static_assert(HasValidLevelRecordCount(1023));
    static_assert(!HasValidLevelRecordCount(1024));
    static_assert(IsCanonicalLevelId(0, 0));
    static_assert(IsCanonicalLevelId(1022, 1022));
    static_assert(!IsCanonicalLevelId(-1, 0));
    static_assert(!IsCanonicalLevelId(1, 0));
    static_assert(!IsCanonicalLevelId(1023, 1023));
    static_assert(CoordinateValueMask == 0x1FFF);
    static_assert(EncodedCoordinateMask == 0xFFFF);
    static_assert(!EncodeLevelCoordinate(255, 5200));
    static_assert(!EncodeLevelCoordinate(1023, 5200));
    static_assert(!EncodeLevelCoordinate(256, 8192));

    constexpr std::array<std::int32_t, 7> codecLevelIds{
        256, 511, 512, 767, 768, 1021, 1022};
    for (const auto levelId : codecLevelIds) {
        const auto encoded = EncodeLevelCoordinate(levelId, 5200);
        assert(encoded);
        const auto decoded = DecodeLevelCoordinate(
            encoded->lowLevelId,
            encoded->x);
        assert(decoded);
        assert(decoded->levelId == levelId);
        assert(decoded->x == 5200);
    }
    assert(!DecodeLevelCoordinate(0, 5200));
    assert(!DecodeLevelCoordinate(0, 0x8000));
    assert(!DecodeLevelCoordinate(0xFF, 0xFFFF));
    static_assert(!IsSupportedDataContext(0));
    static_assert(IsSupportedDataContext(1));
    static_assert(IsSupportedDataContext(2));
    static_assert(IsSupportedDataContext(3));
    static_assert(!IsSupportedDataContext(4));

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
    assert(pluginText.find("HasValidLevelRecordCount") != std::string::npos);
    assert(pluginText.find("IsCanonicalLevelId") != std::string::npos);
    assert(pluginText.find("HasValidAnchorActs") != std::string::npos);
    assert(pluginText.find("SendRoomInSightPacketRva") != std::string::npos);
    assert(pluginText.find("SendRoomOutOfSightPacketRva") != std::string::npos);
    assert(pluginText.find("SetClientInSightRva") != std::string::npos);
    assert(pluginText.find("UnsetClientInSightRva") != std::string::npos);
    assert(pluginText.find("D2ClientPlayerIdOffset") != std::string::npos);
    assert(pluginText.find("NetworkServiceV1") != std::string::npos);
    assert(pluginText.find("NetworkCompatibilityToken") != std::string::npos);
    assert(pluginText.find("localPlayerId == playerId") != std::string::npos);
    assert(pluginText.find("EncodeLevelCoordinate") != std::string::npos);
    assert(pluginText.find("DecodeLevelCoordinate") != std::string::npos);
    assert(pluginText.find("2.0.0") != std::string::npos);
    assert(pluginText.find("ResolveProbe") != std::string::npos);
    assert(pluginText.find("source=%s") != std::string::npos);
    assert(pluginText.find("ConfigFileName") == std::string::npos);
    assert(pluginText.find("LoadConfig") == std::string::npos);
    assert(pluginText.find("Settings.enabled") == std::string::npos);
    assert(pluginText.find("nlohmann") == std::string::npos);

    return 0;
}
