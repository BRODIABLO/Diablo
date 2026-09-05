#include "extended_act_level_ids_policy.hpp"

#include <array>
#ifdef NDEBUG
#undef NDEBUG
#endif
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
    static_assert(MaximumKeyedValidationLevelId == 255);
    static_assert(DynamicTownPortalClassId == 59);
    static_assert(MaximumClientPortalEntries == 1024);
    static_assert(!IsExtendedLevelId(255));
    static_assert(IsExtendedLevelId(256));
    static_assert(IsExtendedLevelId(1022));
    static_assert(!IsExtendedLevelId(1023));
    static_assert(LowLevelId(256) == 0);
    static_assert(LowLevelId(1022) == 254);
    static_assert(!HasValidLevelRecordCount(0));
    static_assert(HasValidLevelRecordCount(1));
    static_assert(HasValidLevelRecordCount(1023));
    static_assert(!HasValidLevelRecordCount(1024));
    static_assert(KeyedValidationRowCount(0) == 0);
    static_assert(KeyedValidationRowCount(1) == 1);
    static_assert(KeyedValidationRowCount(255) == 255);
    static_assert(KeyedValidationRowCount(256) == 256);
    static_assert(KeyedValidationRowCount(257) == 256);
    static_assert(KeyedValidationRowCount(1023) == 256);
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

    constexpr PortalEndpointDescriptor sourcePortal{
        .sessionGeneration = 7,
        .gameIdentity = 0x1234,
        .guid = 100,
        .counterpartGuid = 101,
        .classId = DynamicTownPortalClassId,
        .destinationLevelId = 109,
        .nativeLowLevelId = 109,
    };
    constexpr PortalEndpointDescriptor linkedPortal{
        .sessionGeneration = 7,
        .gameIdentity = 0x1234,
        .guid = 101,
        .counterpartGuid = 100,
        .classId = DynamicTownPortalClassId,
        .destinationLevelId = 256,
        .nativeLowLevelId = 0,
    };
    static_assert(IsValidPortalEndpoint(sourcePortal));
    static_assert(IsValidPortalEndpoint(linkedPortal));
    static_assert(IsReciprocalPortalPair(sourcePortal, linkedPortal));
    constexpr auto stalePortal = PortalEndpointDescriptor{
        .sessionGeneration = 8,
        .gameIdentity = linkedPortal.gameIdentity,
        .guid = linkedPortal.guid,
        .counterpartGuid = linkedPortal.counterpartGuid,
        .classId = linkedPortal.classId,
        .destinationLevelId = linkedPortal.destinationLevelId,
        .nativeLowLevelId = linkedPortal.nativeLowLevelId,
    };
    static_assert(!IsReciprocalPortalPair(sourcePortal, stalePortal));
    constexpr auto truncatedPortal = PortalEndpointDescriptor{
        .sessionGeneration = linkedPortal.sessionGeneration,
        .gameIdentity = linkedPortal.gameIdentity,
        .guid = linkedPortal.guid,
        .counterpartGuid = linkedPortal.counterpartGuid,
        .classId = linkedPortal.classId,
        .destinationLevelId = linkedPortal.destinationLevelId,
        .nativeLowLevelId = 1,
    };
    static_assert(!IsValidPortalEndpoint(truncatedPortal));

    constexpr ClientPortalDescriptor clientPortal256{
        .sessionGeneration = 7,
        .guid = 200,
        .destinationLevelId = 256,
        .nativeLowLevelId = 0,
        .ownerRoomLevelId = 256,
        .ownerRoomNativeLowLevelId = 0,
    };
    constexpr ClientPortalDescriptor clientPortal1022{
        .sessionGeneration = 7,
        .guid = 201,
        .destinationLevelId = 1022,
        .nativeLowLevelId = 254,
        .ownerRoomLevelId = 1022,
        .ownerRoomNativeLowLevelId = 254,
    };
    static_assert(IsValidClientPortalDescriptor(clientPortal256));
    static_assert(IsValidClientPortalDescriptor(clientPortal1022));
    constexpr auto invalidClientPortal = ClientPortalDescriptor{
        .sessionGeneration = 7,
        .guid = 202,
        .destinationLevelId = 1022,
        .nativeLowLevelId = 253,
    };
    static_assert(!IsValidClientPortalDescriptor(invalidClientPortal));
    constexpr ClientPortalDescriptor clientPortalVanillaDestination{
        .sessionGeneration = 7,
        .guid = 203,
        .destinationLevelId = 109,
        .nativeLowLevelId = 109,
        .ownerRoomLevelId = 256,
        .ownerRoomNativeLowLevelId = 0,
    };
    static_assert(IsValidClientPortalDescriptor(
        clientPortalVanillaDestination));

    std::vector<ClientPortalDescriptor> clientPortals;
    assert(UpsertClientPortalDescriptor(clientPortals, clientPortal256));
    assert(clientPortals.size() == 1);
    auto clientLookup = DecideClientPortalLookup(
        clientPortals,
        7,
        200,
        0,
        0,
        true,
        false,
        true);
    assert(clientLookup.decision
        == ClientPortalLookupDecision::FullLevelId);
    assert(clientLookup.levelId == 256);
    auto ownerRoomLookup = DecideClientPortalOwnerRoomLookup(
        clientPortals,
        7,
        200,
        0,
        0,
        true,
        false,
        true);
    assert(ownerRoomLookup.decision
        == ClientPortalLookupDecision::FullLevelId);
    assert(ownerRoomLookup.levelId == 256);

    constexpr std::array clientPortal1022Entries{clientPortal1022};
    ownerRoomLookup = DecideClientPortalOwnerRoomLookup(
        clientPortal1022Entries,
        7,
        201,
        254,
        254,
        true,
        false,
        true);
    assert(ownerRoomLookup.decision
        == ClientPortalLookupDecision::FullLevelId);
    assert(ownerRoomLookup.levelId == 1022);

    ownerRoomLookup = DecideClientPortalOwnerRoomLookup(
        clientPortals,
        7,
        200,
        1,
        1,
        true,
        false,
        true);
    assert(ownerRoomLookup.decision == ClientPortalLookupDecision::Refuse);
    ownerRoomLookup = DecideClientPortalOwnerRoomLookup(
        clientPortals,
        8,
        200,
        0,
        0,
        true,
        false,
        true);
    assert(ownerRoomLookup.decision == ClientPortalLookupDecision::Refuse);
    ownerRoomLookup = DecideClientPortalOwnerRoomLookup(
        clientPortals,
        7,
        200,
        0,
        0,
        true,
        true,
        true);
    assert(ownerRoomLookup.decision == ClientPortalLookupDecision::Refuse);
    ownerRoomLookup = DecideClientPortalOwnerRoomLookup(
        {},
        7,
        300,
        109,
        109,
        true,
        false,
        false);
    assert(ownerRoomLookup.decision == ClientPortalLookupDecision::Original);
    ownerRoomLookup = DecideClientPortalOwnerRoomLookup(
        {},
        7,
        300,
        0,
        0,
        true,
        false,
        false);
    assert(ownerRoomLookup.decision == ClientPortalLookupDecision::Refuse);
    constexpr ClientPortalDescriptor ownerUnknown{
        .sessionGeneration = 7,
        .guid = 204,
        .destinationLevelId = 256,
        .nativeLowLevelId = 0,
    };
    static_assert(IsValidClientPortalDescriptor(ownerUnknown));
    constexpr std::array ownerUnknownEntries{ownerUnknown};
    ownerRoomLookup = DecideClientPortalOwnerRoomLookup(
        ownerUnknownEntries,
        7,
        204,
        0,
        0,
        true,
        false,
        false);
    assert(ownerRoomLookup.decision == ClientPortalLookupDecision::Refuse);
    ownerRoomLookup = DecideClientPortalOwnerRoomLookup(
        ownerUnknownEntries,
        7,
        204,
        1,
        1,
        true,
        false,
        false);
    assert(ownerRoomLookup.decision == ClientPortalLookupDecision::Refuse);

    clientLookup = DecideClientPortalLookup(
        {},
        7,
        300,
        109,
        109,
        true,
        false,
        false);
    assert(clientLookup.decision == ClientPortalLookupDecision::Original);
    assert(clientLookup.levelId == 109);
    constexpr std::array vanillaDestinationEntries{
        clientPortalVanillaDestination};
    clientLookup = DecideClientPortalLookup(
        vanillaDestinationEntries,
        7,
        203,
        109,
        109,
        true,
        false,
        true);
    assert(clientLookup.decision == ClientPortalLookupDecision::Original);
    ownerRoomLookup = DecideClientPortalOwnerRoomLookup(
        vanillaDestinationEntries,
        7,
        203,
        0,
        0,
        true,
        false,
        true);
    assert(ownerRoomLookup.decision
        == ClientPortalLookupDecision::FullLevelId);
    assert(ownerRoomLookup.levelId == 256);
    clientLookup = DecideClientPortalLookup(
        {},
        7,
        300,
        0,
        0,
        true,
        false,
        false);
    assert(clientLookup.decision == ClientPortalLookupDecision::Refuse);
    clientLookup = DecideClientPortalLookup(
        {},
        7,
        300,
        0,
        0,
        false,
        true,
        false);
    assert(clientLookup.decision == ClientPortalLookupDecision::Original);
    clientLookup = DecideClientPortalLookup(
        clientPortals,
        7,
        200,
        1,
        1,
        true,
        false,
        true);
    assert(clientLookup.decision == ClientPortalLookupDecision::Refuse);
    clientLookup = DecideClientPortalLookup(
        clientPortals,
        8,
        200,
        0,
        0,
        true,
        false,
        true);
    assert(clientLookup.decision == ClientPortalLookupDecision::Refuse);

    auto reusedClientPortal = clientPortal1022;
    reusedClientPortal.guid = clientPortal256.guid;
    assert(UpsertClientPortalDescriptor(clientPortals, reusedClientPortal));
    assert(clientPortals.size() == 1);
    clientLookup = DecideClientPortalLookup(
        clientPortals,
        7,
        200,
        254,
        254,
        true,
        false,
        true);
    assert(clientLookup.decision
        == ClientPortalLookupDecision::FullLevelId);
    assert(clientLookup.levelId == 1022);
    assert(EraseClientPortalGuid(clientPortals, 200) == 1);
    assert(clientPortals.empty());

    std::vector<ClientPortalDescriptor> boundedClientPortals;
    for (std::size_t index = 0;
            index < MaximumClientPortalEntries;
            ++index) {
        const auto levelId = static_cast<std::int32_t>(
            256 + (index % (MaximumLevelId - 255)));
        assert(UpsertClientPortalDescriptor(
            boundedClientPortals,
            ClientPortalDescriptor{
                .sessionGeneration = 9,
                .guid = static_cast<std::uint32_t>(index + 1),
                .destinationLevelId = levelId,
                .nativeLowLevelId = LowLevelId(levelId),
            }));
    }
    assert(boundedClientPortals.size() == MaximumClientPortalEntries);
    assert(!UpsertClientPortalDescriptor(
        boundedClientPortals,
        ClientPortalDescriptor{
            .sessionGeneration = 9,
            .guid = 5000,
            .destinationLevelId = 256,
            .nativeLowLevelId = 0,
        }));
    assert(boundedClientPortals.size() == MaximumClientPortalEntries);
    assert(UpsertClientPortalDescriptor(
        boundedClientPortals,
        ClientPortalDescriptor{
            .sessionGeneration = 10,
            .guid = 5001,
            .destinationLevelId = 256,
            .nativeLowLevelId = 0,
        }));
    assert(boundedClientPortals.size() == 1);
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
    const auto physicalPass = pluginText.find("rowIndex < table.rowCount");
    const auto keyedPass = pluginText.find("rowIndex < keyedRowCount");
    const auto keyedLookup = pluginText.find("DataTables->findRowById");
    if (physicalPass == std::string::npos
            || keyedPass == std::string::npos
            || keyedLookup == std::string::npos
            || physicalPass >= keyedPass
            || keyedPass >= keyedLookup
            || pluginText.find("DataTables->findRowById", keyedLookup + 1)
                != std::string::npos) {
        return 1;
    }
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
    assert(pluginText.find("D2GAME_CreateLinkPortal") != std::string::npos);
    assert(pluginText.find("CLIENT_HandlePacket0x51") != std::string::npos);
    assert(pluginText.find("CLIENT_HandlePacket0x60") != std::string::npos);
    assert(pluginText.find("CLIENT_GetUnitByIdAndTypeRva")
        != std::string::npos);
    assert(pluginText.find("ClientPortalLabelLevelsRecordCallRva")
        != std::string::npos);
    assert(pluginText.find("ClientPortalLabelContextWitnessExpected")
        != std::string::npos);
    assert(pluginText.find("ClientTownPortalUsabilityExpected")
        != std::string::npos);
    assert(pluginText.find("ClientTownPortalLevelsRecordCallRva")
        != std::string::npos);
    assert(pluginText.find("ClientTownPortalLevelDefRecordCallRva")
        != std::string::npos);
    assert(pluginText.find("ClientPortalStateOwnerLevelWitnessExpected")
        != std::string::npos);
    assert(pluginText.find("PortalStatePrimarySendCallRva")
        != std::string::npos);
    assert(pluginText.find("PortalStateSecondarySendCallRva")
        != std::string::npos);
    assert(pluginText.find("PortalEndpointDescriptor") != std::string::npos);
    assert(pluginText.find("ClientPortalDescriptor") != std::string::npos);
    assert(pluginText.find("ClientPortalEndpoints") != std::string::npos);
    assert(pluginText.find(
        "event->kind == D2RL::Lifecycle::GameplayEventKind::GameJoined")
        != std::string::npos);
    assert(pluginText.find("MaximumClientPortalEntries")
        != std::string::npos);
    assert(pluginText.find("InstallPortalRecordCallRedirects")
        != std::string::npos);
    assert(pluginText.find("WriteClientPortalLabelRelay")
        != std::string::npos);
    assert(pluginText.find("0x49,0x89,0xF0") != std::string::npos);
    assert(pluginText.find("WriteClientTownPortalRelay")
        != std::string::npos);
    assert(pluginText.find("0x49,0x89,0xD8") != std::string::npos);
    assert(pluginText.find("WritePortalStateFullLevelRelay")
        != std::string::npos);
    assert(pluginText.find("PatchCallRel32") != std::string::npos);
    assert(pluginText.find("PortalSessionPoisoned") != std::string::npos);
    assert(pluginText.find("if (needsExtension && !eligible)")
        != std::string::npos);
    assert(pluginText.find("IsLocalPlayerUnit(owner)") != std::string::npos);
    assert(pluginText.find("native behavior retained") == std::string::npos);
    assert(pluginText.find("packet[11] != packet[2]") == std::string::npos);
    assert(pluginText.find("linkedLevelId != endpoint->nativeLowLevelId")
        == std::string::npos);
    const auto portalPacketHooksBegin = pluginText.find(
        "void __fastcall HookSendObjectSpawnPacket");
    const auto portalPacketHooksEnd = pluginText.find(
        "bool IsCompatiblePlayer",
        portalPacketHooksBegin);
    assert(portalPacketHooksBegin != std::string::npos);
    assert(portalPacketHooksEnd != std::string::npos);
    const auto portalPacketHooks = pluginText.substr(
        portalPacketHooksBegin,
        portalPacketHooksEnd - portalPacketHooksBegin);
    assert(portalPacketHooks.find("EncodeLevelCoordinate")
        == std::string::npos);
    assert(portalPacketHooks.find("DecodeLevelCoordinate")
        == std::string::npos);
    assert(portalPacketHooks.find("CodecMarkerMask") == std::string::npos);
    assert(portalPacketHooks.find("encoded->x") == std::string::npos);
    assert(pluginText.find("HookSendPortalStatePacket(")
        == std::string::npos);
    assert(pluginText.find("outside the local codec contract")
        == std::string::npos);
    assert(pluginText.find("2.1.2") != std::string::npos);
    assert(pluginText.find("KeyedValidationRowCount") != std::string::npos);
    assert(pluginText.find("serviceResult=%u") != std::string::npos);
    assert(pluginText.find("rowIndex=%u, levelId=%d") != std::string::npos);
    assert(pluginText.find("rowMatch=%u") != std::string::npos);
    assert(pluginText.find("ResolveProbe") != std::string::npos);
    assert(pluginText.find("source=%s") != std::string::npos);
    assert(pluginText.find("ConfigFileName") == std::string::npos);
    assert(pluginText.find("LoadConfig") == std::string::npos);
    assert(pluginText.find("Settings.enabled") == std::string::npos);
    assert(pluginText.find("nlohmann") == std::string::npos);

    return 0;
}
