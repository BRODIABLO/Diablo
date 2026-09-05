#define NOMINMAX

#include <D2RLPlugin/api.h>

#include "extended_act_level_ids_policy.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace RuffnecKk::ExtendedActLevelIds {
namespace {

using namespace ruffneckk::extended_act_level_ids;

constexpr wchar_t SingletonName[] =
    L"Local\\RuffnecKk.ExtendedActLevelIds.Singleton";

constexpr std::uintptr_t ResolveActFromLevelIdRva = 0x326710;
constexpr std::uintptr_t LevelsCapacityGuardRva = 0x330446;
constexpr std::uintptr_t SendRoomInSightPacketRva = 0x47D2D0;
constexpr std::uintptr_t SendRoomOutOfSightPacketRva = 0x47EAF0;
constexpr std::uintptr_t SetClientInSightRva = 0x328680;
constexpr std::uintptr_t UnsetClientInSightRva = 0x328780;
constexpr std::uintptr_t D2ClientLayoutWitnessRva = 0x485B51;
constexpr std::uintptr_t D2GAME_CreateLinkPortalRva = 0x435DD0;
constexpr std::uintptr_t D2GAME_CreateLinkPortalCallerWitnessRva = 0x432F12;
constexpr std::uintptr_t D2GAME_CreateLinkPortalGuardWitnessRva = 0x436052;
constexpr std::uintptr_t DUNGEON_GetLevelIdFromRoomRva = 0x2EFC10;
constexpr std::uintptr_t DUNGEON_GetLevelIdFromRoomGuardReturnRva = 0x43605F;
constexpr std::uintptr_t SUNIT_GetPortalOwnerRva = 0x490070;
constexpr std::uintptr_t OBJECTS_OperateFunction15_PortalRva = 0x58F680;
constexpr std::uintptr_t PortalOperationContextWitnessRva = 0x58F7C0;
constexpr std::uintptr_t DATATBLS_GetLevelsTxtRecordRva = 0x32C4A0;
constexpr std::uintptr_t DATATBLS_GetLevelDefRecordRva = 0x32C200;
constexpr std::uintptr_t PortalLevelsRecordCallRva = 0x58F819;
constexpr std::uintptr_t PortalLevelDefRecordCallRva = 0x58F8EE;
constexpr std::uintptr_t D2GAME_SendPacket0x51_ObjectSpawnRva = 0x47CE40;
constexpr std::uintptr_t CLIENT_HandlePacket0x51_ObjectSpawnRva = 0x129D70;
constexpr std::uintptr_t D2GAME_SendPacket0x60_PortalStateRva = 0x47F620;
constexpr std::uintptr_t CLIENT_HandlePacket0x60_PortalStateRva = 0x1CB1C0;
constexpr std::uintptr_t CLIENT_GetUnitByIdAndTypeRva = 0x9A5D0;
constexpr std::uintptr_t ClientPortalLabelContextWitnessRva = 0xC187F;
constexpr std::uintptr_t ClientPortalLabelLevelsRecordCallRva = 0xC188E;
constexpr std::uintptr_t ClientTownPortalUsabilityRva = 0xFE1F0;
constexpr std::uintptr_t ClientTownPortalLevelsContextWitnessRva = 0xFE2FC;
constexpr std::uintptr_t ClientTownPortalLevelsRecordCallRva = 0xFE307;
constexpr std::uintptr_t ClientTownPortalLevelDefContextWitnessRva = 0xFE328;
constexpr std::uintptr_t ClientTownPortalLevelDefRecordCallRva = 0xFE333;
constexpr std::uintptr_t ClientPortalStateOwnerLevelWitnessRva = 0x1CB23A;
constexpr std::uintptr_t PortalUnitLayoutWitnessRva = 0x538821;
constexpr std::uintptr_t PortalStateSendCallerWitnessRva = 0x5388CA;
constexpr std::uintptr_t PortalStatePrimarySendCallRva = 0x5388E4;
constexpr std::uintptr_t PortalStateSecondaryCallerWitnessRva = 0x592FFA;
constexpr std::uintptr_t PortalStateSecondarySendCallRva = 0x593012;
constexpr std::size_t D2ClientPlayerIdOffset = 0x270;
constexpr std::size_t GameDataContextOffset = 0x106;
constexpr std::size_t UnitTypeOffset = 0x00;
constexpr std::size_t UnitClassIdOffset = 0x04;
constexpr std::size_t UnitGuidOffset = 0x08;
constexpr std::size_t UnitDataOffset = 0x10;
constexpr std::size_t ObjectInteractTypeOffset = 0x08;
constexpr std::size_t ClientPortalOwnerRoomLevelIdOffset = 0x1BA;
constexpr std::size_t OperateGameOffset = 0x00;
constexpr std::size_t OperateObjectOffset = 0x08;
constexpr std::size_t OperatePlayerOffset = 0x10;
constexpr std::uint32_t InvalidPlayerId = 0xFFFFFFFFU;
constexpr std::uint16_t CompatibilityHandshakeMessage = 1;
constexpr std::uint16_t NetworkLocalChannelId = 1;
constexpr std::uint64_t NetworkCompatibilityToken = 0x454C494456320001ULL;
constexpr auto ResolveActFromLevelIdExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x74,
    0x24,0x10,0x57,0x48,0x83,0xEC,0x30,0x8B,
    0xF2,0x0F,0xB6,0xD9,0xE8,0xB7,0x9F,0xFD,
    0xFF,0x0F,0xB6,0xCB,0x48,0x8B,0xF8,0xE8,
    0x5C,0xA3,0xFD,0xFF,0x8B,0x98,0x08,0x01,
    0x00,0x00,0x83,0xEB,0x01,0x78,0x43,0x90,
});
constexpr auto LevelsCapacityGuardExpected = std::to_array<std::uint8_t>({
    0x48,0x81,0xBE,0x68,0x14,0x00,0x00,0x00,
    0x04,0x00,0x00,0x72,0x0F,0x48,0x8D,0x4C,
    0x24,0x40,0xE8,0x83,0x9F,0xFF,0xFF,0x84,
    0xC0,0x74,0x01,0xCC,0x4C,0x69,0x7F,0x08,
    0x8C,0x01,0x00,0x00,
});
constexpr auto SendRoomInSightPacketExpected = std::to_array<std::uint8_t>({
    0x40,0x57,0x48,0x83,0xEC,0x40,0xC6,0x44,
    0x24,0x20,0x07,0x48,0x8B,0xF9,0x66,0x44,
    0x89,0x44,0x24,0x21,0x66,0x44,0x89,0x4C,
    0x24,0x23,0x88,0x54,0x24,0x25,0x48,0x85,
    0xC9,
});
constexpr auto SendRoomOutOfSightPacketExpected = std::to_array<std::uint8_t>({
    0x40,0x57,0x48,0x83,0xEC,0x40,0xC6,0x44,
    0x24,0x20,0x08,0x48,0x8B,0xF9,0x66,0x44,
    0x89,0x44,0x24,0x21,0x66,0x44,0x89,0x4C,
    0x24,0x23,0x88,0x54,0x24,0x25,0x48,0x85,
    0xC9,
});
constexpr auto SetClientInSightExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,
    0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,
    0x48,0x83,0xEC,0x30,0x41,0x8B,0xE9,0x41,
    0x8B,0xF8,0x48,0x8B,0xF2,0x0F,0xB6,0xD9,
});
constexpr auto UnsetClientInSightExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,
    0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,
    0x48,0x83,0xEC,0x30,0x41,0x8B,0xE9,0x41,
    0x8B,0xD8,0x48,0x8B,0xF2,
});
constexpr auto D2ClientLayoutWitnessExpected = std::to_array<std::uint8_t>({
    0xF6,0x87,0xE0,0x04,0x00,0x00,0x01,0x74,
    0x09,0x48,0x8B,0x87,0x78,0x02,0x00,0x00,
    0xEB,0x2E,0x4C,0x39,0xA7,0x78,0x02,0x00,
    0x00,0x74,0x3F,0x44,0x8B,0x87,0x70,0x02,
    0x00,0x00,0x8B,0x97,0x6C,0x02,0x00,0x00,
    0x48,0x8B,0x8F,0xB0,0x02,0x00,0x00,0xE8,
    0xFB,0xA2,0x00,0x00,
});
constexpr auto D2GAME_CreateLinkPortalExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x74,
    0x24,0x18,0x55,0x57,0x41,0x54,0x41,0x56,
    0x41,0x57,0x48,0x8B,0xEC,0x48,0x81,0xEC,
    0x80,0x00,0x00,0x00,0x4C,0x8B,0xF9,0x48,
});
constexpr auto D2GAME_CreateLinkPortalCallerWitnessExpected =
    std::to_array<std::uint8_t>({
        0x45,0x8B,0xCD,0x4C,0x8B,0xC3,0x89,0x44,
        0x24,0x20,0x49,0x8B,0xD6,0x48,0x8B,0xCD,
        0xE8,0xA9,0x2E,0x00,0x00,0x48,0x8B,0xD8,
        0x48,0x85,0xC0,0x74,0x38,0x48,0x8B,0xC8,
    });
constexpr auto D2GAME_CreateLinkPortalGuardWitnessExpected =
    std::to_array<std::uint8_t>({
        0xE8,0xE9,0x53,0xF1,0xFF,0x48,0x8B,0xC8,
        0xE8,0xB1,0x9B,0xEB,0xFF,0x8B,0xF8,0x3D,
        0xFF,0x00,0x00,0x00,0x7E,0x12,0x48,0x8D,
        0x4D,0x30,0xC6,0x45,0x30,0x00,0xE8,0x0B,
        0xBD,0xFF,0xFF,0x84,0xC0,0x74,0x01,0xCC,
        0x40,0x0F,0xB6,0xD7,0x48,0x8B,0xCB,0xE8,
    });
constexpr auto DUNGEON_GetLevelIdFromRoomExpected =
    std::to_array<std::uint8_t>({
        0x48,0x85,0xC9,0x75,0x03,0x33,0xC0,0xC3,
        0x48,0x8B,0x49,0x18,0xE9,0x9F,0x13,0x07,
        0x00,
    });
constexpr auto SUNIT_GetPortalOwnerExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x6C,
    0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57,
    0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x30,
    0x48,0x8B,0xFA,0x48,0x8B,0xD9,0x48,0x85,
});
constexpr auto OBJECTS_OperateFunction15_PortalExpected =
    std::to_array<std::uint8_t>({
        0x48,0x89,0x4C,0x24,0x08,0x55,0x53,0x56,
        0x57,0x41,0x56,0x41,0x57,0x48,0x8B,0xEC,
        0x48,0x83,0xEC,0x68,0x48,0x8B,0x49,0x08,
        0x48,0x8D,0x5D,0x38,0xE8,0xBF,0xB4,0xDB,
    });
constexpr auto PortalOperationContextWitnessExpected =
    std::to_array<std::uint8_t>({
        0x48,0x8B,0x4D,0x38,0x48,0x8B,0x51,0x08,
        0x48,0x8B,0x09,0xE8,0xA0,0x08,0xF0,0xFF,
        0x48,0x8B,0xF0,0x48,0x85,0xC0,0x0F,0x84,
        0xF8,0x04,0x00,0x00,0x48,0x8B,0xC8,0xE8,
        0xEC,0xC1,0xDB,0xFF,0x83,0xF8,0x02,0x74,
        0x12,0x48,0x8D,0x4D,0x48,0x44,0x88,0x7D,
        0x48,0xE8,0xCA,0x2C,0x00,0x00,0x84,0xC0,
        0x74,0x01,0xCC,0x48,0x8B,0x4D,0x38,0x48,
    });
constexpr auto DATATBLS_GetLevelsTxtRecordExpected =
    std::to_array<std::uint8_t>({
        0x48,0x89,0x5C,0x24,0x08,0x55,0x56,0x57,
        0x48,0x83,0xEC,0x30,0x48,0x63,0xDA,0x0F,
        0xB6,0xF1,0xE8,0xD9,0x45,0xFD,0xFF,0x48,
        0x8B,0xE8,0x85,0xDB,0x7E,0x34,0x40,0x0F,
    });
constexpr auto DATATBLS_GetLevelDefRecordExpected =
    std::to_array<std::uint8_t>({
        0x48,0x89,0x5C,0x24,0x08,0x55,0x56,0x57,
        0x48,0x83,0xEC,0x30,0x48,0x63,0xDA,0x0F,
        0xB6,0xF1,0xE8,0x79,0x48,0xFD,0xFF,0x48,
        0x8B,0xE8,0x85,0xDB,0x78,0x34,0x40,0x0F,
    });
constexpr auto PortalLevelsRecordCallExpected =
    std::to_array<std::uint8_t>({0xE8,0x82,0xCC,0xD9,0xFF});
constexpr auto PortalLevelDefRecordCallExpected =
    std::to_array<std::uint8_t>({0xE8,0x0D,0xC9,0xD9,0xFF});
constexpr auto D2GAME_SendPacket0x51_ObjectSpawnExpected =
    std::to_array<std::uint8_t>({
        0x48,0x89,0x5C,0x24,0x10,0x55,0x56,0x57,
        0x41,0x56,0x41,0x57,0x48,0x8B,0xEC,0x48,
        0x83,0xEC,0x70,0x48,0x8B,0x05,0x6E,0xE4,
        0x54,0x02,0x48,0x33,0xC4,0x48,0x89,0x45,
    });
constexpr auto CLIENT_HandlePacket0x51_ObjectSpawnExpected =
    std::to_array<std::uint8_t>({
        0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,
        0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48,
        0x89,0x7C,0x24,0x20,0x41,0x56,0x48,0x83,
        0xEC,0x50,0x44,0x8B,0x71,0x02,0x48,0x8B,
    });
constexpr auto D2GAME_SendPacket0x60_PortalStateExpected =
    std::to_array<std::uint8_t>({
        0x48,0x89,0x5C,0x24,0x18,0x56,0x57,0x41,
        0x56,0x48,0x83,0xEC,0x70,0x48,0x8B,0x05,
        0x94,0xBC,0x54,0x02,0x48,0x33,0xC4,0x48,
        0x89,0x44,0x24,0x60,0x4C,0x8B,0xF1,0xC6,
    });
constexpr auto CLIENT_HandlePacket0x60_PortalStateExpected =
    std::to_array<std::uint8_t>({
        0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,
        0xEC,0x20,0x48,0x8B,0xD9,0xBA,0x02,0x00,
        0x00,0x00,0x8B,0x49,0x03,0xE8,0xF6,0xF3,
        0xEC,0xFF,0x48,0x8B,0xF8,
    });
constexpr auto CLIENT_GetUnitByIdAndTypeExpected =
    std::to_array<std::uint8_t>({
        0x4C,0x63,0xCA,0x48,0x8D,0x05,0x36,0x93,
        0x98,0x02,0x8B,0xD1,0x44,0x8B,0xC1,0x49,
        0x8B,0xC9,0x83,0xE2,0x7F,0x48,0xC1,0xE1,
        0x0A,0x48,0x03,0xC8,0xE9,0x7F,0x4C,0x00,
        0x00,
    });
constexpr auto ClientPortalLabelContextWitnessExpected =
    std::to_array<std::uint8_t>({
        0x48,0x8B,0xCE,0xE8,0xB9,0x94,0x28,0x00,
        0x0F,0xB6,0xD0,0x40,0x0F,0xB6,0xCF,0xE8,
        0x0D,0xAC,0x26,0x00,0x48,0xC7,0xC3,0xFF,
        0xFF,0xFF,0xFF,
    });
constexpr auto ClientPortalLabelLevelsRecordCallExpected =
    std::to_array<std::uint8_t>({0xE8,0x0D,0xAC,0x26,0x00});
constexpr auto ClientTownPortalUsabilityExpected =
    std::to_array<std::uint8_t>({
        0x40,0x53,0x57,0x48,0x83,0xEC,0x28,0x48,
        0x8B,0xDA,0x48,0x8B,0xF9,0x48,0x85,0xC9,
        0x74,0x09,0xE8,0xC9,0xD7,0x24,0x00,0x85,
        0xC0,0x74,0x14,0x48,0x8D,0x4C,0x24,0x40,
    });
constexpr auto ClientTownPortalLevelsContextWitnessExpected =
    std::to_array<std::uint8_t>({
        0x0F,0xB6,0x93,0xBA,0x01,0x00,0x00,0x40,
        0x0F,0xB6,0xCE,0xE8,0x94,0xE1,0x22,0x00,
    });
constexpr auto ClientTownPortalLevelsRecordCallExpected =
    std::to_array<std::uint8_t>({0xE8,0x94,0xE1,0x22,0x00});
constexpr auto ClientTownPortalLevelDefContextWitnessExpected =
    std::to_array<std::uint8_t>({
        0x0F,0xB6,0x93,0xBA,0x01,0x00,0x00,0x40,
        0x0F,0xB6,0xCE,0xE8,0xC8,0xDE,0x22,0x00,
    });
constexpr auto ClientTownPortalLevelDefRecordCallExpected =
    std::to_array<std::uint8_t>({0xE8,0xC8,0xDE,0x22,0x00});
constexpr auto ClientPortalStateOwnerLevelWitnessExpected =
    std::to_array<std::uint8_t>({
        0x0F,0xB6,0x43,0x0B,0x88,0x87,0xBA,0x01,
        0x00,0x00,0x48,0x8B,0x5C,0x24,0x30,
    });
constexpr auto PortalUnitLayoutWitnessExpected =
    std::to_array<std::uint8_t>({
        0x48,0x8B,0x46,0x10,0x41,0xB0,0x02,0x44,
        0x8B,0x4E,0x08,0xB2,0x51,0x0F,0xB6,0x48,
        0x08,0x0F,0xB6,0x46,0x0C,0x88,0x4C,0x24,
        0x40,0x49,0x8B,0xCE,0x88,0x44,0x24,0x38,
        0x0F,0xB7,0x44,0x24,0x64,0x66,0x89,0x44,
        0x24,0x30,0x0F,0xB7,0x44,0x24,0x60,0x66,
        0x89,0x44,0x24,0x28,0x0F,0xB7,0x46,0x04,
        0x66,0x89,0x44,0x24,0x20,
    });
constexpr auto PortalStateSendCallerWitnessExpected =
    std::to_array<std::uint8_t>({
        0x0F,0xB7,0x4C,0x24,0x5C,0x45,0x0F,0xB6,
        0xC4,0x44,0x0F,0xB7,0x4C,0x24,0x58,0x48,
        0x8B,0xD6,0x66,0x89,0x4C,0x24,0x20,0x49,
        0x8B,0xCE,0xE8,0x37,0x6D,0xF4,0xFF,
    });
constexpr auto PortalStatePrimarySendCallExpected =
    std::to_array<std::uint8_t>({0xE8,0x37,0x6D,0xF4,0xFF});
constexpr auto PortalStateSecondaryCallerWitnessExpected =
    std::to_array<std::uint8_t>({
        0x44,0x0F,0xB6,0xC7,0x44,0x0F,0xB7,0x8C,
        0x24,0x80,0x00,0x00,0x00,0x48,0x8B,0xD6,
        0x66,0x89,0x4C,0x24,0x20,
    });
constexpr auto PortalStateSecondarySendCallExpected =
    std::to_array<std::uint8_t>({0xE8,0x09,0xC6,0xEE,0xFF});

using ResolveActFromLevelIdFn = std::uint8_t(__fastcall*)(
    std::uint8_t dataContext,
    std::int32_t levelId) noexcept;

using SendRoomVisibilityPacketFn = void(__fastcall*)(
    void* client,
    std::int32_t levelId,
    std::uint16_t x,
    std::uint16_t y) noexcept;

using UpdateClientSightFn = void(__fastcall*)(
    std::uint8_t dataContext,
    void* act,
    std::int32_t levelId,
    std::int32_t x,
    std::int32_t y,
    void* room) noexcept;

using CreateLinkPortalFn = void*(__fastcall*)(
    void* game,
    void* owner,
    void* sourcePortal,
    std::int32_t destinationLevelId,
    std::int32_t sourceLevelId) noexcept;

using GetLevelIdFromRoomFn = std::int32_t(__fastcall*)(
    void* room) noexcept;

using GetPortalOwnerFn = void*(__fastcall*)(
    void* game,
    void* portal) noexcept;

using OperatePortalFn = std::int32_t(__fastcall*)(
    void* operation,
    std::int32_t operate) noexcept;

using GetLevelRecordFn = void*(__fastcall*)(
    std::uint8_t dataContext,
    std::int32_t levelId) noexcept;

using ClientGetUnitByIdAndTypeFn = void*(__fastcall*)(
    std::uint32_t guid,
    std::int32_t unitType) noexcept;

using SendObjectSpawnPacketFn = void(__fastcall*)(
    void* client,
    std::uint8_t opcode,
    std::uint8_t unitType,
    std::uint32_t guid,
    std::uint16_t objectId,
    std::uint16_t x,
    std::uint16_t y,
    std::uint8_t animation,
    std::uint8_t interactType) noexcept;

using HandlePacketFn = void(__fastcall*)(
    const std::uint8_t* packet) noexcept;

using SendPortalStatePacketFn = void(__fastcall*)(
    void* client,
    void* portal,
    std::uint8_t ownerRoomLowLevelId,
    std::uint16_t ownerX,
    std::uint16_t ownerY) noexcept;

struct ActMap {
    std::uint64_t revision{};
    std::vector<ActEntry> entries;
};

struct CompatiblePeerEntry {
    D2RL::Network::PeerHandle peer{};
    std::uint32_t playerId{};
};

struct CompatiblePeerMap {
    std::vector<CompatiblePeerEntry> entries;
};

struct PortalEndpointMap {
    std::vector<PortalEndpointDescriptor> entries;
};

struct ClientPortalMap {
    std::vector<ClientPortalDescriptor> entries;
};

struct PortalCreationScope {
    std::int32_t sourceLevelId{};
};

struct PortalOperationScope {
    PortalEndpointDescriptor endpoint;
    bool ownerValidated{};
};

const D2RL::PluginContext* Context{};
const D2RL::DataTableServiceV1* DataTables{};
const D2RL::NetworkServiceV1* Network{};
D2RL::Network::ChannelHandle NetworkChannel{
    D2RL::Network::InvalidChannelHandle};
std::string RuntimeBuildName{"unknown"};
HANDLE SingletonHandle{};
void* PortalRecordRelayPage{};
ResolveActFromLevelIdFn OriginalResolveActFromLevelId{};
SendRoomVisibilityPacketFn OriginalSendRoomInSightPacket{};
SendRoomVisibilityPacketFn OriginalSendRoomOutOfSightPacket{};
UpdateClientSightFn OriginalSetClientInSight{};
UpdateClientSightFn OriginalUnsetClientInSight{};
CreateLinkPortalFn OriginalCreateLinkPortal{};
GetLevelIdFromRoomFn OriginalGetLevelIdFromRoom{};
GetPortalOwnerFn OriginalGetPortalOwner{};
OperatePortalFn OriginalOperatePortal{};
GetLevelRecordFn OriginalGetLevelsTxtRecord{};
GetLevelRecordFn OriginalGetLevelDefRecord{};
ClientGetUnitByIdAndTypeFn ClientGetUnitByIdAndType{};
SendObjectSpawnPacketFn OriginalSendObjectSpawnPacket{};
HandlePacketFn OriginalHandleObjectSpawnPacket{};
SendPortalStatePacketFn OriginalSendPortalStatePacket{};
HandlePacketFn OriginalHandlePortalStatePacket{};

std::atomic_bool Operational{};
std::atomic_bool CacheReady{};
std::atomic_uint64_t PublishedRevision{};
std::atomic_uint64_t ResolvedFromLevels{};
std::atomic_uint64_t OriginalFallbacks{};
std::atomic_uint64_t EncodedVisibilityPackets{};
std::atomic_uint64_t DecodedVisibilityPackets{};
std::atomic_uint64_t RefusedVisibilityPackets{};
std::atomic_uint64_t CompatiblePeerAnnouncements{};
std::atomic_uint64_t NetworkWarnings{};
std::atomic_uint64_t PortalPairsPublished{};
std::atomic_uint64_t PortalOperations{};
std::atomic_uint64_t PublishedPortalPackets{};
std::atomic_uint64_t ValidatedPortalPackets{};
std::atomic_uint64_t RefusedPortalClientIdentities{};
std::atomic_uint64_t RefusedPortalSidecars{};
std::atomic_uint64_t RefusedPortalOperations{};
std::atomic_uint64_t ClientPortalPublications{};
std::atomic_uint64_t ClientPortalEvictions{};
std::atomic_uint64_t ClientPortalFullLookups{};
std::atomic_uint64_t ClientPortalFallbacks{};
std::atomic_bool PortalSessionPoisoned{};
std::atomic_uint32_t LocalPlayerId{InvalidPlayerId};
std::atomic_uint64_t SessionGeneration{};
std::atomic<std::shared_ptr<const ActMap>> ClassicCache{};
std::atomic<std::shared_ptr<const ActMap>> LodCache{};
std::atomic<std::shared_ptr<const ActMap>> RotwCache{};
std::atomic<std::shared_ptr<const CompatiblePeerMap>> CompatiblePeers{};
std::atomic<std::shared_ptr<const PortalEndpointMap>> PortalEndpoints{};
std::atomic<std::shared_ptr<const ClientPortalMap>> ClientPortalEndpoints{};
std::mutex PortalPublicationMutex;
std::mutex ClientPortalPublicationMutex;
thread_local PortalCreationScope* ActivePortalCreation{};
thread_local PortalOperationScope* ActivePortalOperation{};
thread_local std::int32_t ScopedPortalLevelId{-1};

template <typename Value>
Value ReadValue(const void* base, std::size_t offset) noexcept {
    Value result{};
    if (base) {
        const auto* bytes = static_cast<const std::uint8_t*>(base);
        std::memcpy(&result, bytes + offset, sizeof(result));
    }
    return result;
}

template <typename Value>
void WriteValue(void* base, std::size_t offset, Value value) noexcept {
    if (base) {
        auto* bytes = static_cast<std::uint8_t*>(base);
        std::memcpy(bytes + offset, &value, sizeof(value));
    }
}

bool ReadPortalUnit(
        const void* unit,
        std::uint32_t& guid,
        std::uint8_t& nativeLowLevelId) noexcept {
    if (!unit
            || ReadValue<std::uint32_t>(unit, UnitTypeOffset) != 2
            || ReadValue<std::uint32_t>(unit, UnitClassIdOffset)
                != DynamicTownPortalClassId) {
        return false;
    }
    const auto data = ReadValue<const void*>(unit, UnitDataOffset);
    if (!data) return false;
    guid = ReadValue<std::uint32_t>(unit, UnitGuidOffset);
    nativeLowLevelId = ReadValue<std::uint8_t>(
        data,
        ObjectInteractTypeOffset);
    return guid != InvalidUnitGuid;
}

const char* BankName(D2RL::DataTables::Bank bank) noexcept {
    switch (bank) {
    case D2RL::DataTables::Bank::Classic: return "Classic";
    case D2RL::DataTables::Bank::Lod: return "Lod";
    case D2RL::DataTables::Bank::Rotw: return "RotW";
    default: return "Unknown";
    }
}

D2RL::DataTables::Bank BankForContext(
        std::uint8_t dataContext) noexcept {
    switch (dataContext) {
    case 1: return D2RL::DataTables::Bank::Classic;
    case 2: return D2RL::DataTables::Bank::Lod;
    case 3: return D2RL::DataTables::Bank::Rotw;
    default: return static_cast<D2RL::DataTables::Bank>(0);
    }
}

std::shared_ptr<const ActMap> LoadCache(
        std::uint8_t dataContext) noexcept {
    switch (dataContext) {
    case 1: return ClassicCache.load(std::memory_order_acquire);
    case 2: return LodCache.load(std::memory_order_acquire);
    case 3: return RotwCache.load(std::memory_order_acquire);
    default: return {};
    }
}

void ResetCaches() noexcept {
    CacheReady.store(false, std::memory_order_release);
    ClassicCache.store({}, std::memory_order_release);
    LodCache.store({}, std::memory_order_release);
    RotwCache.store({}, std::memory_order_release);
    PublishedRevision.store(0, std::memory_order_release);
}

bool AcquireSingleton() noexcept {
    SingletonHandle = CreateMutexW(nullptr, FALSE, SingletonName);
    if (!SingletonHandle) {
        Context->LogError(
            "ExtendedActLevelIds: process singleton could not be created.");
        return false;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(SingletonHandle);
        SingletonHandle = nullptr;
        Context->LogError(
            "ExtendedActLevelIds: duplicate global/mod-local installation refused.");
        return false;
    }
    return true;
}

void ReleaseSingleton() noexcept {
    if (SingletonHandle) {
        CloseHandle(SingletonHandle);
        SingletonHandle = nullptr;
    }
}

bool ValidateRuntime() noexcept {
    const auto* base = Context
        ? reinterpret_cast<const std::uint8_t*>(Context->exeBase)
        : nullptr;
    const auto matches = [base](
            std::uintptr_t rva,
            const auto& expected) noexcept {
        return base && std::memcmp(
            base + rva,
            expected.data(),
            expected.size()) == 0;
    };
    if (!matches(ResolveActFromLevelIdRva, ResolveActFromLevelIdExpected)) {
        Context->LogError(
            "ExtendedActLevelIds: central act resolver fingerprint mismatch; plugin refused.");
        return false;
    }
    if (!matches(LevelsCapacityGuardRva, LevelsCapacityGuardExpected)) {
        Context->LogError(
            "ExtendedActLevelIds: native 1023-record capacity witness mismatch; plugin refused.");
        return false;
    }
    if (!matches(SendRoomInSightPacketRva, SendRoomInSightPacketExpected)
            || !matches(
                SendRoomOutOfSightPacketRva,
                SendRoomOutOfSightPacketExpected)
            || !matches(SetClientInSightRva, SetClientInSightExpected)
            || !matches(UnsetClientInSightRva, UnsetClientInSightExpected)) {
        Context->LogError(
            "ExtendedActLevelIds: room-visibility codec fingerprint mismatch; plugin refused.");
        return false;
    }
    if (!matches(D2ClientLayoutWitnessRva, D2ClientLayoutWitnessExpected)) {
        Context->LogError(
            "ExtendedActLevelIds: D2Client player identity witness mismatch; plugin refused.");
        return false;
    }
    if (!matches(
                D2GAME_CreateLinkPortalRva,
                D2GAME_CreateLinkPortalExpected)
            || !matches(
                D2GAME_CreateLinkPortalCallerWitnessRva,
                D2GAME_CreateLinkPortalCallerWitnessExpected)
            || !matches(
                D2GAME_CreateLinkPortalGuardWitnessRva,
                D2GAME_CreateLinkPortalGuardWitnessExpected)
            || !matches(
                DUNGEON_GetLevelIdFromRoomRva,
                DUNGEON_GetLevelIdFromRoomExpected)) {
        Context->LogError(
            "ExtendedActLevelIds: Town Portal creation fingerprint mismatch; plugin refused.");
        return false;
    }
    if (!matches(SUNIT_GetPortalOwnerRva, SUNIT_GetPortalOwnerExpected)
            || !matches(
                OBJECTS_OperateFunction15_PortalRva,
                OBJECTS_OperateFunction15_PortalExpected)
            || !matches(
                PortalOperationContextWitnessRva,
                PortalOperationContextWitnessExpected)
            || !matches(
                DATATBLS_GetLevelsTxtRecordRva,
                DATATBLS_GetLevelsTxtRecordExpected)
            || !matches(
                DATATBLS_GetLevelDefRecordRva,
                DATATBLS_GetLevelDefRecordExpected)
            || !matches(
                PortalLevelsRecordCallRva,
                PortalLevelsRecordCallExpected)
            || !matches(
                PortalLevelDefRecordCallRva,
                PortalLevelDefRecordCallExpected)) {
        Context->LogError(
            "ExtendedActLevelIds: Town Portal operation fingerprint mismatch; plugin refused.");
        return false;
    }
    if (!matches(
                D2GAME_SendPacket0x51_ObjectSpawnRva,
                D2GAME_SendPacket0x51_ObjectSpawnExpected)
            || !matches(
                CLIENT_HandlePacket0x51_ObjectSpawnRva,
                CLIENT_HandlePacket0x51_ObjectSpawnExpected)
            || !matches(
                D2GAME_SendPacket0x60_PortalStateRva,
                D2GAME_SendPacket0x60_PortalStateExpected)
            || !matches(
                CLIENT_HandlePacket0x60_PortalStateRva,
                CLIENT_HandlePacket0x60_PortalStateExpected)
            || !matches(
                CLIENT_GetUnitByIdAndTypeRva,
                CLIENT_GetUnitByIdAndTypeExpected)
            || !matches(
                ClientPortalLabelContextWitnessRva,
                ClientPortalLabelContextWitnessExpected)
            || !matches(
                ClientPortalLabelLevelsRecordCallRva,
                ClientPortalLabelLevelsRecordCallExpected)
            || !matches(
                ClientTownPortalUsabilityRva,
                ClientTownPortalUsabilityExpected)
            || !matches(
                ClientTownPortalLevelsContextWitnessRva,
                ClientTownPortalLevelsContextWitnessExpected)
            || !matches(
                ClientTownPortalLevelsRecordCallRva,
                ClientTownPortalLevelsRecordCallExpected)
            || !matches(
                ClientTownPortalLevelDefContextWitnessRva,
                ClientTownPortalLevelDefContextWitnessExpected)
            || !matches(
                ClientTownPortalLevelDefRecordCallRva,
                ClientTownPortalLevelDefRecordCallExpected)
            || !matches(
                ClientPortalStateOwnerLevelWitnessRva,
                ClientPortalStateOwnerLevelWitnessExpected)
            || !matches(
                PortalUnitLayoutWitnessRva,
                PortalUnitLayoutWitnessExpected)
            || !matches(
                PortalStateSendCallerWitnessRva,
                PortalStateSendCallerWitnessExpected)
            || !matches(
                PortalStatePrimarySendCallRva,
                PortalStatePrimarySendCallExpected)
            || !matches(
                PortalStateSecondaryCallerWitnessRva,
                PortalStateSecondaryCallerWitnessExpected)
            || !matches(
                PortalStateSecondarySendCallRva,
                PortalStateSecondarySendCallExpected)) {
        Context->LogError(
            "ExtendedActLevelIds: Town Portal packet/layout fingerprint mismatch; plugin refused.");
        return false;
    }
    return true;
}

bool QueryServices(
        const D2RL::LifecycleServiceV1*& lifecycle) noexcept {
    const D2RL::DataTableServiceV1* tables{};
    if (Context->QueryService(
            D2RL::ServiceId::DataTable,
            D2RL::DataTableServiceV1Version,
            &tables) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasDataTableServiceV1Field(
                tables,
                D2RL::DataTableServiceV1RequiredSize)) {
        Context->LogError(
            "ExtendedActLevelIds: DataTableServiceV1 is unavailable or incompatible.");
        return false;
    }
    DataTables = tables;

    const D2RL::LifecycleServiceV1* lifecycleService{};
    if (Context->QueryService(
            D2RL::ServiceId::Lifecycle,
            D2RL::LifecycleServiceV1Version,
            &lifecycleService) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasLifecycleServiceV1Field(
                lifecycleService,
                D2RL::LifecycleServiceV1RequiredSize)) {
        Context->LogError(
            "ExtendedActLevelIds: LifecycleServiceV1 is unavailable or incompatible.");
        return false;
    }
    lifecycle = lifecycleService;

    const D2RL::NetworkServiceV1* networkService{};
    if (Context->QueryService(
            D2RL::ServiceId::Network,
            D2RL::NetworkServiceV1Version,
            &networkService) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasNetworkServiceV1Field(
                networkService,
                D2RL::NetworkServiceV1RequiredSize)) {
        Context->LogError(
            "ExtendedActLevelIds: NetworkServiceV1 is unavailable or incompatible.");
        return false;
    }
    Network = networkService;
    return true;
}

std::shared_ptr<const ActMap> BuildBankCache(
        const D2RL::PluginContext* context,
        D2RL::DataTables::Bank bank,
        std::uint64_t revision,
        std::string& error) {
    D2RL::DataTables::TableView table{
        .structSize = D2RL::DataTables::TableViewSize,
    };
    const auto tableResult = DataTables->getTable(
        context,
        bank,
        D2RL::DataTables::TableId::Levels,
        &table);
    if (tableResult != D2RL::DataTables::Result::Success
            || table.revision != revision
            || table.rows == nullptr
            || !HasValidLevelRecordCount(table.rowCount)
            || table.rowSize != LevelsRowSize) {
        error = "invalid Levels table view";
        return {};
    }

    auto map = std::make_shared<ActMap>();
    map->revision = revision;
    map->entries.reserve(table.rowCount);
    for (std::uint32_t rowIndex = 0; rowIndex < table.rowCount; ++rowIndex) {
        D2RL::DataTables::RowView physical{
            .structSize = D2RL::DataTables::RowViewSize,
        };
        if (DataTables->getRow(
                context,
                bank,
                D2RL::DataTables::TableId::Levels,
                rowIndex,
                &physical) != D2RL::DataTables::Result::Success
                || physical.revision != revision
                || physical.row == nullptr
                || physical.rowIndex != rowIndex
                || physical.rowSize != LevelsRowSize) {
            error = "physical Levels row lookup failed";
            return {};
        }

        const auto levelId = ReadValue<std::int32_t>(
            physical.row,
            LevelsIdOffset);
        const auto act = ReadValue<std::uint8_t>(
            physical.row,
            LevelsActOffset);
        if (!IsCanonicalLevelId(levelId, rowIndex)
                || act > MaximumAct) {
            error = "Levels rows must use contiguous Id values 0..1022 and Act values 0..4";
            return {};
        }
        map->entries.push_back({levelId, act});
    }

    const auto keyedRowCount = KeyedValidationRowCount(table.rowCount);
    for (std::uint32_t rowIndex = 0;
            rowIndex < keyedRowCount;
            ++rowIndex) {
        D2RL::DataTables::RowView physical{
            .structSize = D2RL::DataTables::RowViewSize,
        };
        if (DataTables->getRow(
                context,
                bank,
                D2RL::DataTables::TableId::Levels,
                rowIndex,
                &physical) != D2RL::DataTables::Result::Success
                || physical.revision != revision
                || physical.row == nullptr
                || physical.rowIndex != rowIndex
                || physical.rowSize != LevelsRowSize) {
            error = "physical Levels row lookup failed during keyed validation";
            return {};
        }

        const auto levelId = ReadValue<std::int32_t>(
            physical.row,
            LevelsIdOffset);
        if (!IsCanonicalLevelId(levelId, rowIndex)) {
            error = "Levels Id changed during keyed validation";
            return {};
        }

        D2RL::DataTables::RowView keyed{
            .structSize = D2RL::DataTables::RowViewSize,
        };
        const auto keyedResult = DataTables->findRowById(
            context,
            bank,
            D2RL::DataTables::TableId::Levels,
            static_cast<std::uint32_t>(levelId),
            &keyed);
        const auto revisionMatches = keyed.revision == revision;
        const auto rowMatches = keyed.row == physical.row;
        const auto rowIndexMatches = keyed.rowIndex == rowIndex;
        const auto rowSizeMatches = keyed.rowSize == LevelsRowSize;
        if (keyedResult != D2RL::DataTables::Result::Success
                || !revisionMatches
                || !rowMatches
                || !rowIndexMatches
                || !rowSizeMatches) {
            char diagnostic[512]{};
            std::snprintf(
                diagnostic,
                sizeof(diagnostic),
                "Levels Id service round-trip failed at rowIndex=%u, levelId=%d: serviceResult=%u, revision=%llu/%llu (match=%u), rowIndex=%u/%u (match=%u), rowSize=%u/%u (match=%u), rowMatch=%u",
                rowIndex,
                levelId,
                static_cast<unsigned int>(keyedResult),
                static_cast<unsigned long long>(keyed.revision),
                static_cast<unsigned long long>(revision),
                revisionMatches ? 1U : 0U,
                keyed.rowIndex,
                rowIndex,
                rowIndexMatches ? 1U : 0U,
                keyed.rowSize,
                LevelsRowSize,
                rowSizeMatches ? 1U : 0U,
                rowMatches ? 1U : 0U);
            error = diagnostic;
            return {};
        }
    }

    std::sort(map->entries.begin(), map->entries.end());
    if (std::adjacent_find(
            map->entries.begin(),
            map->entries.end(),
            [](const ActEntry& left, const ActEntry& right) {
                return left.levelId == right.levelId;
            }) != map->entries.end()) {
        error = "Levels contains duplicate Id values";
        return {};
    }
    if (!HasValidAnchorActs(std::span<const ActEntry>(map->entries))) {
        error = "Levels Act layout failed the anchor check";
        return {};
    }
    return map;
}

void __cdecl OnDataTablesLoaded(
        const D2RL::PluginContext* context,
        const D2RL::Lifecycle::DataTablesLoadedEvent* event,
        void*) noexcept {
    CacheReady.store(false, std::memory_order_release);
    try {
        if (!context
                || !DataTables
                || !D2RL::Lifecycle::HasDataTablesLoadedEventField(
                    event,
                    D2RL::Lifecycle::DataTablesLoadedEventRequiredSize)) {
            ResetCaches();
            if (context) context->LogError(
                "ExtendedActLevelIds: invalid DataTablesLoaded event; original resolver retained.");
            return;
        }

        std::array<std::shared_ptr<const ActMap>, 3> maps;
        constexpr std::array<D2RL::DataTables::Bank, 3> banks{
            D2RL::DataTables::Bank::Classic,
            D2RL::DataTables::Bank::Lod,
            D2RL::DataTables::Bank::Rotw,
        };
        for (std::size_t index = 0; index < banks.size(); ++index) {
            std::string error;
            maps[index] = BuildBankCache(
                context,
                banks[index],
                event->revision,
                error);
            if (!maps[index]) {
                ResetCaches();
                const auto message = std::string(
                    "ExtendedActLevelIds: ") + BankName(banks[index])
                    + " cache rejected (" + error
                    + "); original resolver retained.";
                context->LogError(message.c_str());
                return;
            }
        }

        ClassicCache.store(maps[0], std::memory_order_release);
        LodCache.store(maps[1], std::memory_order_release);
        RotwCache.store(maps[2], std::memory_order_release);
        PublishedRevision.store(event->revision, std::memory_order_release);
        CacheReady.store(true, std::memory_order_release);

        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "ExtendedActLevelIds: Levels revision %llu accepted; rows Classic=%zu, LoD=%zu, RotW=%zu.",
            static_cast<unsigned long long>(event->revision),
            maps[0]->entries.size(),
            maps[1]->entries.size(),
            maps[2]->entries.size());
        context->LogInfo(message);
    } catch (const std::exception& exception) {
        ResetCaches();
        const auto message = std::string(
            "ExtendedActLevelIds: cache build failed (")
            + exception.what() + "); original resolver retained.";
        context->LogError(message.c_str());
    } catch (...) {
        ResetCaches();
        context->LogError(
            "ExtendedActLevelIds: cache build failed; original resolver retained.");
    }
}

std::uint8_t __fastcall HookResolveActFromLevelId(
        std::uint8_t dataContext,
        std::int32_t levelId) noexcept {
    const auto effectiveLevelId = ScopedPortalLevelId >= 0
            && LowLevelId(ScopedPortalLevelId) == LowLevelId(levelId)
        ? ScopedPortalLevelId
        : levelId;
    if (Operational.load(std::memory_order_acquire)
            && CacheReady.load(std::memory_order_acquire)
            && IsSupportedDataContext(dataContext)
            && BankForContext(dataContext)
                != static_cast<D2RL::DataTables::Bank>(0)) {
        const auto cache = LoadCache(dataContext);
        if (cache) {
            const auto act = FindAct(
                std::span<const ActEntry>(cache->entries),
                effectiveLevelId);
            if (act) {
                ResolvedFromLevels.fetch_add(1, std::memory_order_relaxed);
                return *act;
            }
        }
    }
    OriginalFallbacks.fetch_add(1, std::memory_order_relaxed);
    return OriginalResolveActFromLevelId(dataContext, effectiveLevelId);
}

bool IsKnownLevelIdForContext(
        std::uint8_t dataContext,
        std::int32_t levelId) noexcept {
    if (levelId < 0
            || levelId > MaximumLevelId
            || !IsSupportedDataContext(dataContext)
            || !CacheReady.load(std::memory_order_acquire)) {
        return false;
    }
    const auto cache = LoadCache(dataContext);
    return cache && FindAct(
        std::span<const ActEntry>(cache->entries),
        levelId).has_value();
}

bool IsKnownExtendedLevelId(std::int32_t levelId) noexcept {
    if (levelId <= MaximumVanillaNetworkLevelId
            || levelId > MaximumLevelId
            || !CacheReady.load(std::memory_order_acquire)) {
        return false;
    }
    for (std::uint8_t dataContext = MinimumDataContext;
            dataContext <= MaximumDataContext;
            ++dataContext) {
        if (IsKnownLevelIdForContext(dataContext, levelId)) {
            return true;
        }
    }
    return false;
}

void RefusePortalContractOnce(const char* reason) noexcept {
    if (RefusedPortalOperations.fetch_add(1, std::memory_order_relaxed) == 0
            && Context) {
        Context->LogError(reason);
    }
}

void PoisonPortalSession(const char* reason) noexcept {
    PortalSessionPoisoned.store(true, std::memory_order_release);
    RefusePortalContractOnce(reason);
}

void PoisonClientPortalSession(
        std::uint64_t sessionGeneration,
        const char* reason) noexcept {
    if (sessionGeneration
            != SessionGeneration.load(std::memory_order_acquire)) {
        return;
    }
    PoisonPortalSession(reason);
}

bool EvictClientPortalGuid(
        std::uint32_t guid,
        std::uint64_t sessionGeneration) noexcept {
    if (guid == InvalidUnitGuid) return true;
    try {
        std::lock_guard lock(ClientPortalPublicationMutex);
        if (sessionGeneration
                != SessionGeneration.load(std::memory_order_acquire)) {
            return true;
        }
        const auto current = ClientPortalEndpoints.load(
            std::memory_order_acquire);
        if (!current) return true;
        auto mutableNext = std::make_shared<ClientPortalMap>();
        mutableNext->entries = current->entries;
        const auto erased = EraseClientPortalGuid(
            mutableNext->entries,
            guid);
        if (erased == 0) return true;
        ClientPortalEndpoints.store(
            mutableNext->entries.empty()
                ? std::shared_ptr<const ClientPortalMap>{}
                : std::shared_ptr<const ClientPortalMap>{mutableNext},
            std::memory_order_release);
        ClientPortalEvictions.fetch_add(erased, std::memory_order_relaxed);
        return true;
    } catch (...) {
        ClientPortalEndpoints.store({}, std::memory_order_release);
        PoisonPortalSession(
            "ExtendedActLevelIds: client Town Portal sidecar eviction failed; the session was refused.");
        return false;
    }
}

bool PublishClientPortal(
        const PortalEndpointDescriptor& endpoint,
        std::optional<std::int32_t> ownerRoomLevelId = std::nullopt) noexcept {
    try {
        std::lock_guard lock(ClientPortalPublicationMutex);
        if (endpoint.sessionGeneration
                != SessionGeneration.load(std::memory_order_acquire)) {
            return false;
        }
        const auto current = ClientPortalEndpoints.load(
            std::memory_order_acquire);
        auto mutableNext = std::make_shared<ClientPortalMap>();
        if (current) mutableNext->entries = current->entries;
        ClientPortalDescriptor descriptor{
            .sessionGeneration = endpoint.sessionGeneration,
            .guid = endpoint.guid,
            .destinationLevelId = endpoint.destinationLevelId,
            .nativeLowLevelId = endpoint.nativeLowLevelId,
        };
        const auto previous = std::find_if(
            mutableNext->entries.begin(),
            mutableNext->entries.end(),
            [&](const ClientPortalDescriptor& candidate) {
                return candidate.sessionGeneration
                        == endpoint.sessionGeneration
                    && candidate.guid == endpoint.guid
                    && candidate.destinationLevelId
                        == endpoint.destinationLevelId
                    && candidate.nativeLowLevelId
                        == endpoint.nativeLowLevelId;
            });
        if (previous != mutableNext->entries.end()) {
            descriptor.ownerRoomLevelId = previous->ownerRoomLevelId;
            descriptor.ownerRoomNativeLowLevelId =
                previous->ownerRoomNativeLowLevelId;
        }
        if (ownerRoomLevelId) {
            descriptor.ownerRoomLevelId = *ownerRoomLevelId;
            descriptor.ownerRoomNativeLowLevelId =
                LowLevelId(*ownerRoomLevelId);
        }
        if (!UpsertClientPortalDescriptor(
                mutableNext->entries,
                descriptor,
                MaximumClientPortalEntries)) {
            PoisonPortalSession(
                "ExtendedActLevelIds: client Town Portal sidecar publication was invalid or exceeded its bounded capacity; the session was refused.");
            return false;
        }
        ClientPortalEndpoints.store(
            std::shared_ptr<const ClientPortalMap>{mutableNext},
            std::memory_order_release);
        ClientPortalPublications.fetch_add(1, std::memory_order_relaxed);
        return true;
    } catch (...) {
        ClientPortalEndpoints.store({}, std::memory_order_release);
        PoisonPortalSession(
            "ExtendedActLevelIds: client Town Portal sidecar publication failed; the session was refused.");
        return false;
    }
}

std::optional<ClientPortalDescriptor> FindClientPortalDescriptor(
        std::uint64_t sessionGeneration,
        std::uint32_t guid) noexcept {
    const auto map = ClientPortalEndpoints.load(std::memory_order_acquire);
    if (!map) return std::nullopt;
    const auto found = std::find_if(
        map->entries.begin(),
        map->entries.end(),
        [&](const ClientPortalDescriptor& descriptor) {
            return descriptor.sessionGeneration == sessionGeneration
                && descriptor.guid == guid;
        });
    if (found == map->entries.end()) return std::nullopt;
    return *found;
}

std::optional<PortalEndpointDescriptor> FindPortalEndpoint(
        std::uintptr_t gameIdentity,
        std::uint32_t guid,
        std::uint32_t classId,
        std::uint8_t nativeLowLevelId) noexcept {
    const auto map = PortalEndpoints.load(std::memory_order_acquire);
    if (!map) return std::nullopt;
    const auto generation = SessionGeneration.load(std::memory_order_acquire);
    const auto found = std::find_if(
        map->entries.begin(),
        map->entries.end(),
        [&](const PortalEndpointDescriptor& endpoint) {
            return endpoint.sessionGeneration == generation
                && (!gameIdentity
                    || endpoint.gameIdentity == gameIdentity)
                && endpoint.guid == guid
                && endpoint.classId == classId
                && endpoint.nativeLowLevelId == nativeLowLevelId;
        });
    if (found == map->entries.end() || !IsValidPortalEndpoint(*found)) {
        return std::nullopt;
    }
    const auto counterpart = std::find_if(
        map->entries.begin(),
        map->entries.end(),
        [&](const PortalEndpointDescriptor& endpoint) {
            return endpoint.sessionGeneration == generation
                && endpoint.gameIdentity == found->gameIdentity
                && endpoint.guid == found->counterpartGuid;
        });
    if (counterpart == map->entries.end()
            || !IsReciprocalPortalPair(*found, *counterpart)) {
        return std::nullopt;
    }
    return *found;
}

std::optional<PortalEndpointDescriptor> FindPortalEndpointForUnit(
        const void* game,
        const void* portal) noexcept {
    std::uint32_t guid{};
    std::uint8_t nativeLowLevelId{};
    if (!ReadPortalUnit(portal, guid, nativeLowLevelId)) {
        return std::nullopt;
    }
    return FindPortalEndpoint(
        reinterpret_cast<std::uintptr_t>(game),
        guid,
        DynamicTownPortalClassId,
        nativeLowLevelId);
}

void InvalidatePortalGuids(
        std::uintptr_t gameIdentity,
        std::uint32_t firstGuid,
        std::uint32_t secondGuid) noexcept {
    try {
        std::lock_guard lock(PortalPublicationMutex);
        const auto current = PortalEndpoints.load(std::memory_order_acquire);
        if (!current) return;
        auto mutableNext = std::make_shared<PortalEndpointMap>();
        mutableNext->entries = current->entries;
        std::erase_if(
            mutableNext->entries,
            [&](const PortalEndpointDescriptor& endpoint) {
                return endpoint.gameIdentity == gameIdentity
                    && (endpoint.guid == firstGuid
                        || endpoint.guid == secondGuid);
            });
        PortalEndpoints.store(
            mutableNext->entries.empty()
                ? std::shared_ptr<const PortalEndpointMap>{}
                : std::shared_ptr<const PortalEndpointMap>{mutableNext},
            std::memory_order_release);
    } catch (...) {
        PortalEndpoints.store({}, std::memory_order_release);
    }
}

bool IsLocalClient(const void* client) noexcept {
    if (!client) return false;
    const auto localPlayerId = LocalPlayerId.load(std::memory_order_acquire);
    return localPlayerId != InvalidPlayerId
        && ReadValue<std::uint32_t>(client, D2ClientPlayerIdOffset)
            == localPlayerId;
}

bool IsLocalPlayerUnit(const void* player) noexcept {
    if (!player) return false;
    const auto localPlayerId = LocalPlayerId.load(std::memory_order_acquire);
    return localPlayerId != InvalidPlayerId
        && ReadValue<std::uint32_t>(player, UnitGuidOffset)
            == localPlayerId;
}

void* __fastcall HookCreateLinkPortal(
        void* game,
        void* owner,
        void* sourcePortal,
        std::int32_t destinationLevelId,
        std::int32_t sourceLevelId) noexcept {
    if (!OriginalCreateLinkPortal) return nullptr;

    std::uint32_t sourceGuid{};
    std::uint8_t sourceLowLevelId{};
    const auto dataContext = ReadValue<std::uint8_t>(
        game,
        GameDataContextOffset);
    const auto sourcePortalValid = ReadPortalUnit(
        sourcePortal,
        sourceGuid,
        sourceLowLevelId);
    const auto needsExtension = IsExtendedLevelId(destinationLevelId)
        || IsExtendedLevelId(sourceLevelId);
    const auto eligible = Operational.load(std::memory_order_acquire)
        && !PortalSessionPoisoned.load(std::memory_order_acquire)
        && needsExtension
        && IsLocalPlayerUnit(owner)
        && sourcePortalValid
        && sourceLowLevelId == LowLevelId(destinationLevelId)
        && IsKnownLevelIdForContext(dataContext, destinationLevelId)
        && IsKnownLevelIdForContext(dataContext, sourceLevelId);
    if (needsExtension && !eligible) {
        PortalSessionPoisoned.store(true, std::memory_order_release);
        RefusePortalContractOnce(
            "ExtendedActLevelIds: extended Town Portal creation refused before the native byte guard; the local sidecar contract was not satisfied.");
        return nullptr;
    }
    if (!eligible) {
        auto* linkedPortal = OriginalCreateLinkPortal(
            game,
            owner,
            sourcePortal,
            destinationLevelId,
            sourceLevelId);
        if (sourcePortalValid) {
            std::uint32_t linkedGuid{};
            std::uint8_t linkedLowLevelId{};
            if (ReadPortalUnit(
                    linkedPortal,
                    linkedGuid,
                    linkedLowLevelId)) {
                InvalidatePortalGuids(
                    reinterpret_cast<std::uintptr_t>(game),
                    sourceGuid,
                    linkedGuid);
            }
        }
        return linkedPortal;
    }

    void* linkedPortal{};
    bool originalInvoked{};
    try {
        std::lock_guard lock(PortalPublicationMutex);
        auto current = PortalEndpoints.load(std::memory_order_acquire);
        auto mutableNext = std::make_shared<PortalEndpointMap>();
        if (current) {
            mutableNext->entries = current->entries;
        }
        const auto generation = SessionGeneration.load(
            std::memory_order_acquire);
        std::erase_if(
            mutableNext->entries,
            [generation](const PortalEndpointDescriptor& endpoint) {
                return endpoint.sessionGeneration != generation;
            });
        mutableNext->entries.reserve(mutableNext->entries.size() + 2);

        PortalCreationScope creation{sourceLevelId};
        auto* previousCreation = ActivePortalCreation;
        ActivePortalCreation = &creation;
        originalInvoked = true;
        linkedPortal = OriginalCreateLinkPortal(
            game,
            owner,
            sourcePortal,
            destinationLevelId,
            sourceLevelId);
        ActivePortalCreation = previousCreation;
        if (!linkedPortal) return nullptr;

        std::uint32_t linkedGuid{};
        std::uint8_t linkedLowLevelId{};
        if (!ReadPortalUnit(
                linkedPortal,
                linkedGuid,
                linkedLowLevelId)
                || linkedLowLevelId != LowLevelId(sourceLevelId)) {
            PortalSessionPoisoned.store(true, std::memory_order_release);
            RefusePortalContractOnce(
                "ExtendedActLevelIds: linked Town Portal failed its native low-byte invariant.");
            return linkedPortal;
        }

        const PortalEndpointDescriptor source{
            .sessionGeneration = generation,
            .gameIdentity = reinterpret_cast<std::uintptr_t>(game),
            .guid = sourceGuid,
            .counterpartGuid = linkedGuid,
            .classId = DynamicTownPortalClassId,
            .destinationLevelId = destinationLevelId,
            .nativeLowLevelId = sourceLowLevelId,
        };
        const PortalEndpointDescriptor linked{
            .sessionGeneration = generation,
            .gameIdentity = reinterpret_cast<std::uintptr_t>(game),
            .guid = linkedGuid,
            .counterpartGuid = sourceGuid,
            .classId = DynamicTownPortalClassId,
            .destinationLevelId = sourceLevelId,
            .nativeLowLevelId = linkedLowLevelId,
        };
        if (!IsReciprocalPortalPair(source, linked)) {
            PortalSessionPoisoned.store(true, std::memory_order_release);
            RefusePortalContractOnce(
                "ExtendedActLevelIds: Town Portal GUID pair failed validation.");
            return linkedPortal;
        }
        std::erase_if(
            mutableNext->entries,
            [&](const PortalEndpointDescriptor& endpoint) {
                return endpoint.gameIdentity == source.gameIdentity
                    && (endpoint.guid == source.guid
                        || endpoint.guid == linked.guid);
            });
        mutableNext->entries.push_back(source);
        mutableNext->entries.push_back(linked);
        PortalEndpoints.store(
            std::shared_ptr<const PortalEndpointMap>{mutableNext},
            std::memory_order_release);
        PortalPairsPublished.fetch_add(1, std::memory_order_relaxed);
        return linkedPortal;
    } catch (...) {
        PortalSessionPoisoned.store(true, std::memory_order_release);
        RefusePortalContractOnce(
            "ExtendedActLevelIds: Town Portal sidecar allocation failed; the extended creation was refused.");
        if (originalInvoked) {
            return linkedPortal;
        }
        return nullptr;
    }
}

std::int32_t __fastcall HookGetLevelIdFromRoom(void* room) noexcept {
    const auto levelId = OriginalGetLevelIdFromRoom
        ? OriginalGetLevelIdFromRoom(room)
        : 0;
    const auto* expectedReturn = Context && Context->exeBase
        ? reinterpret_cast<const void*>(
            Context->exeBase
            + DUNGEON_GetLevelIdFromRoomGuardReturnRva)
        : nullptr;
    if (ActivePortalCreation
            && _ReturnAddress() == expectedReturn
            && levelId == ActivePortalCreation->sourceLevelId
            && IsExtendedLevelId(levelId)) {
        return LowLevelId(levelId);
    }
    return levelId;
}

void* __fastcall HookGetPortalOwner(
        void* game,
        void* portal) noexcept {
    if (!OriginalGetPortalOwner) return nullptr;
    const auto endpoint = FindPortalEndpointForUnit(game, portal);
    if (!endpoint || !IsExtendedLevelId(endpoint->destinationLevelId)) {
        return OriginalGetPortalOwner(game, portal);
    }
    const auto dataContext = ReadValue<std::uint8_t>(
        game,
        GameDataContextOffset);
    if (!IsKnownLevelIdForContext(
            dataContext,
            endpoint->destinationLevelId)) {
        RefusePortalContractOnce(
            "ExtendedActLevelIds: extended Town Portal owner lookup has no validated Levels row.");
        return nullptr;
    }

    const auto previousLevelId = ScopedPortalLevelId;
    ScopedPortalLevelId = endpoint->destinationLevelId;
    auto* owner = OriginalGetPortalOwner(game, portal);
    ScopedPortalLevelId = previousLevelId;

    std::uint32_t ownerGuid{};
    std::uint8_t ownerLowLevelId{};
    const auto ownerEndpoint = ReadPortalUnit(
            owner,
            ownerGuid,
            ownerLowLevelId)
        ? FindPortalEndpoint(
            reinterpret_cast<std::uintptr_t>(game),
            ownerGuid,
            DynamicTownPortalClassId,
            ownerLowLevelId)
        : std::nullopt;
    if (!ownerEndpoint
            || ownerGuid != endpoint->counterpartGuid
            || ownerEndpoint->counterpartGuid != endpoint->guid) {
        RefusePortalContractOnce(
            "ExtendedActLevelIds: extended Town Portal owner GUID pair was not reciprocal.");
        return nullptr;
    }
    if (ActivePortalOperation
            && ActivePortalOperation->endpoint.gameIdentity
                == endpoint->gameIdentity
            && ActivePortalOperation->endpoint.guid == endpoint->guid) {
        ActivePortalOperation->ownerValidated = true;
    }
    return owner;
}

std::int32_t __fastcall HookOperatePortal(
        void* operation,
        std::int32_t operate) noexcept {
    if (!OriginalOperatePortal || !operation) return 0;
    auto* game = ReadValue<void*>(operation, OperateGameOffset);
    auto* portal = ReadValue<void*>(operation, OperateObjectOffset);
    auto* player = ReadValue<void*>(operation, OperatePlayerOffset);
    if (PortalSessionPoisoned.load(std::memory_order_acquire)
            && portal
            && ReadValue<std::uint32_t>(portal, UnitClassIdOffset)
                == DynamicTownPortalClassId) {
        RefusePortalContractOnce(
            "ExtendedActLevelIds: Town Portal operation refused after a session sidecar failure.");
        return 0;
    }
    const auto endpoint = FindPortalEndpointForUnit(game, portal);
    if (!endpoint || !IsExtendedLevelId(endpoint->destinationLevelId)) {
        return OriginalOperatePortal(operation, operate);
    }
    const auto dataContext = ReadValue<std::uint8_t>(
        game,
        GameDataContextOffset);
    if (!IsLocalPlayerUnit(player)
            || !IsKnownLevelIdForContext(
                dataContext,
                endpoint->destinationLevelId)) {
        RefusePortalContractOnce(
            "ExtendedActLevelIds: extended Town Portal operation refused outside the local player contract.");
        return 0;
    }

    PortalOperationScope scope{*endpoint, false};
    auto* previousOperation = ActivePortalOperation;
    ActivePortalOperation = &scope;
    const auto result = OriginalOperatePortal(operation, operate);
    ActivePortalOperation = previousOperation;
    if (scope.ownerValidated) {
        PortalOperations.fetch_add(1, std::memory_order_relaxed);
    }
    return result;
}

std::int32_t ScopedOperationLevelId(
        std::uint8_t dataContext,
        std::int32_t nativeLevelId) noexcept {
    if (!ActivePortalOperation
            || !ActivePortalOperation->ownerValidated
            || !IsExtendedLevelId(
                ActivePortalOperation->endpoint.destinationLevelId)
            || LowLevelId(
                ActivePortalOperation->endpoint.destinationLevelId)
                != LowLevelId(nativeLevelId)
            || !IsKnownLevelIdForContext(
                dataContext,
                ActivePortalOperation->endpoint.destinationLevelId)) {
        return nativeLevelId;
    }
    return ActivePortalOperation->endpoint.destinationLevelId;
}

void* __fastcall HookGetLevelsTxtRecord(
        std::uint8_t dataContext,
        std::int32_t levelId) noexcept {
    return OriginalGetLevelsTxtRecord(
        dataContext,
        ScopedOperationLevelId(dataContext, levelId));
}

void* __fastcall HookGetLevelDefRecord(
        std::uint8_t dataContext,
        std::int32_t levelId) noexcept {
    return OriginalGetLevelDefRecord(
        dataContext,
        ScopedOperationLevelId(dataContext, levelId));
}

void* __fastcall HookClientPortalLabelGetLevelsTxtRecord(
        std::uint8_t dataContext,
        std::int32_t requestedLevelId,
        const void* unit) noexcept {
    if (!OriginalGetLevelsTxtRecord) return nullptr;
    const auto lookupGeneration = SessionGeneration.load(
        std::memory_order_acquire);

    const auto isDynamicTownPortal = unit
        && ReadValue<std::uint32_t>(unit, UnitTypeOffset) == 2
        && ReadValue<std::uint32_t>(unit, UnitClassIdOffset)
            == DynamicTownPortalClassId;
    if (!isDynamicTownPortal) {
        ClientPortalFallbacks.fetch_add(1, std::memory_order_relaxed);
        return OriginalGetLevelsTxtRecord(dataContext, requestedLevelId);
    }

    std::uint32_t guid{};
    std::uint8_t nativeLowLevelId{};
    if (!ReadPortalUnit(unit, guid, nativeLowLevelId)) {
        PoisonClientPortalSession(
            lookupGeneration,
            "ExtendedActLevelIds: client Town Portal UI lookup found an invalid live object; the session was refused.");
        return nullptr;
    }

    const auto map = ClientPortalEndpoints.load(std::memory_order_acquire);
    const auto entries = map
        ? std::span<const ClientPortalDescriptor>(map->entries)
        : std::span<const ClientPortalDescriptor>{};
    const auto poisoned = PortalSessionPoisoned.load(
        std::memory_order_acquire);
    const auto fullDescriptorPresent = std::any_of(
        entries.begin(),
        entries.end(),
        [&](const ClientPortalDescriptor& descriptor) {
            return descriptor.sessionGeneration == lookupGeneration
                && descriptor.guid == guid;
        });
    const auto fullLevelIdKnown = fullDescriptorPresent
        && std::any_of(
            entries.begin(),
            entries.end(),
            [&](const ClientPortalDescriptor& descriptor) {
                return descriptor.sessionGeneration == lookupGeneration
                    && descriptor.guid == guid
                    && IsValidClientPortalDescriptor(descriptor)
                    && IsKnownLevelIdForContext(
                        dataContext,
                        descriptor.destinationLevelId);
            });
    const auto decision = DecideClientPortalLookup(
        entries,
        lookupGeneration,
        guid,
        nativeLowLevelId,
        requestedLevelId,
        true,
        poisoned,
        fullLevelIdKnown);
    if (decision.decision == ClientPortalLookupDecision::FullLevelId) {
        if (lookupGeneration
                != SessionGeneration.load(std::memory_order_acquire)) {
            return nullptr;
        }
        ClientPortalFullLookups.fetch_add(1, std::memory_order_relaxed);
        return OriginalGetLevelsTxtRecord(dataContext, decision.levelId);
    }
    if (decision.decision == ClientPortalLookupDecision::Original) {
        ClientPortalFallbacks.fetch_add(1, std::memory_order_relaxed);
        return OriginalGetLevelsTxtRecord(dataContext, decision.levelId);
    }
    if (!poisoned
            && (fullDescriptorPresent
                || nativeLowLevelId != requestedLevelId)) {
        PoisonClientPortalSession(
            lookupGeneration,
            "ExtendedActLevelIds: client Town Portal UI sidecar mismatch; the session was refused.");
    }
    return nullptr;
}

void* ResolveClientTownPortalOwnerRecord(
        GetLevelRecordFn original,
        std::uint8_t dataContext,
        std::int32_t requestedLevelId,
        const void* unit) noexcept {
    if (!original) return nullptr;
    const auto lookupGeneration = SessionGeneration.load(
        std::memory_order_acquire);
    const auto isDynamicTownPortal = unit
        && ReadValue<std::uint32_t>(unit, UnitTypeOffset) == 2
        && ReadValue<std::uint32_t>(unit, UnitClassIdOffset)
            == DynamicTownPortalClassId;
    if (!isDynamicTownPortal) {
        ClientPortalFallbacks.fetch_add(1, std::memory_order_relaxed);
        return original(dataContext, requestedLevelId);
    }

    std::uint32_t guid{};
    std::uint8_t destinationLowLevelId{};
    if (!ReadPortalUnit(unit, guid, destinationLowLevelId)) {
        PoisonClientPortalSession(
            lookupGeneration,
            "ExtendedActLevelIds: client Town Portal owner-room lookup found an invalid live object; the session was refused.");
        return nullptr;
    }
    const auto ownerRoomNativeLowLevelId = ReadValue<std::uint8_t>(
        unit,
        ClientPortalOwnerRoomLevelIdOffset);
    const auto map = ClientPortalEndpoints.load(std::memory_order_acquire);
    const auto entries = map
        ? std::span<const ClientPortalDescriptor>(map->entries)
        : std::span<const ClientPortalDescriptor>{};
    const auto poisoned = PortalSessionPoisoned.load(
        std::memory_order_acquire);
    const auto descriptorPresent = std::any_of(
        entries.begin(),
        entries.end(),
        [&](const ClientPortalDescriptor& descriptor) {
            return descriptor.sessionGeneration == lookupGeneration
                && descriptor.guid == guid;
        });
    const auto ownerDescriptorKnown = descriptorPresent
        && std::any_of(
            entries.begin(),
            entries.end(),
            [&](const ClientPortalDescriptor& descriptor) {
                return descriptor.sessionGeneration == lookupGeneration
                    && descriptor.guid == guid
                    && descriptor.ownerRoomLevelId
                        != UnknownClientPortalLevelId;
            });
    const auto fullLevelIdKnown = ownerDescriptorKnown
        && std::any_of(
            entries.begin(),
            entries.end(),
            [&](const ClientPortalDescriptor& descriptor) {
                return descriptor.sessionGeneration == lookupGeneration
                    && descriptor.guid == guid
                    && IsValidClientPortalDescriptor(descriptor)
                    && descriptor.nativeLowLevelId
                        == destinationLowLevelId
                    && descriptor.ownerRoomLevelId
                        != UnknownClientPortalLevelId
                    && IsKnownLevelIdForContext(
                        dataContext,
                        descriptor.ownerRoomLevelId);
            });
    const auto decision = DecideClientPortalOwnerRoomLookup(
        entries,
        lookupGeneration,
        guid,
        ownerRoomNativeLowLevelId,
        requestedLevelId,
        true,
        poisoned,
        fullLevelIdKnown);
    if (decision.decision == ClientPortalLookupDecision::FullLevelId) {
        if (lookupGeneration
                != SessionGeneration.load(std::memory_order_acquire)) {
            return nullptr;
        }
        ClientPortalFullLookups.fetch_add(1, std::memory_order_relaxed);
        return original(dataContext, decision.levelId);
    }
    if (decision.decision == ClientPortalLookupDecision::Original) {
        ClientPortalFallbacks.fetch_add(1, std::memory_order_relaxed);
        return original(dataContext, decision.levelId);
    }
    if (!poisoned
            && (ownerDescriptorKnown
                || ownerRoomNativeLowLevelId != requestedLevelId)) {
        RefusedPortalSidecars.fetch_add(1, std::memory_order_relaxed);
        PoisonClientPortalSession(
            lookupGeneration,
            "ExtendedActLevelIds: client Town Portal owner-room sidecar mismatch; the session was refused.");
    }
    return nullptr;
}

void* __fastcall HookClientTownPortalGetLevelsTxtRecord(
        std::uint8_t dataContext,
        std::int32_t requestedLevelId,
        const void* unit) noexcept {
    return ResolveClientTownPortalOwnerRecord(
        OriginalGetLevelsTxtRecord,
        dataContext,
        requestedLevelId,
        unit);
}

void* __fastcall HookClientTownPortalGetLevelDefRecord(
        std::uint8_t dataContext,
        std::int32_t requestedLevelId,
        const void* unit) noexcept {
    return ResolveClientTownPortalOwnerRecord(
        OriginalGetLevelDefRecord,
        dataContext,
        requestedLevelId,
        unit);
}

void __fastcall HookSendPortalStatePacketWithFullId(
    void* client,
    void* portal,
    std::uint8_t ownerRoomLowLevelId,
    std::uint16_t ownerX,
    std::uint16_t ownerY,
    std::int32_t ownerRoomLevelId) noexcept;

void* AllocatePortalRecordRelayPage(void* hint) noexcept {
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const auto granularity = static_cast<std::uintptr_t>(
        systemInfo.dwAllocationGranularity);
    const auto base = reinterpret_cast<std::uintptr_t>(hint)
        & ~(granularity - 1);
    for (std::uintptr_t delta = granularity;
            delta < 0x70000000ULL;
            delta += granularity) {
        if (base > std::numeric_limits<std::uintptr_t>::max() - delta) {
            break;
        }
        if (auto* memory = VirtualAlloc(
                reinterpret_cast<void*>(base + delta),
                systemInfo.dwPageSize,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_READWRITE)) {
            return memory;
        }
    }
    return nullptr;
}

bool WritePortalRecordRelay(
        std::uint8_t* destination,
        const void* target) noexcept {
    if (!destination || !target) return false;
    std::array<std::uint8_t, 14> relay{
        0xFF,0x25,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };
    const auto address = reinterpret_cast<std::uintptr_t>(target);
    std::memcpy(relay.data() + 6, &address, sizeof(address));
    std::memcpy(destination, relay.data(), relay.size());
    return true;
}

bool WriteClientPortalLabelRelay(
        std::uint8_t* destination,
        const void* target) noexcept {
    if (!destination || !target) return false;
    std::array<std::uint8_t, 17> relay{
        0x49,0x89,0xF0,
        0xFF,0x25,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };
    const auto address = reinterpret_cast<std::uintptr_t>(target);
    std::memcpy(relay.data() + 9, &address, sizeof(address));
    std::memcpy(destination, relay.data(), relay.size());
    return true;
}

bool WriteClientTownPortalRelay(
        std::uint8_t* destination,
        const void* target) noexcept {
    if (!destination || !target) return false;
    std::array<std::uint8_t, 17> relay{
        0x49,0x89,0xD8,
        0xFF,0x25,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };
    const auto address = reinterpret_cast<std::uintptr_t>(target);
    std::memcpy(relay.data() + 9, &address, sizeof(address));
    std::memcpy(destination, relay.data(), relay.size());
    return true;
}

bool WritePortalStateFullLevelRelay(
        std::uint8_t* destination,
        const void* target,
        bool fullLevelIdInR12) noexcept {
    if (!destination || !target) return false;
    constexpr auto Prefix = std::to_array<std::uint8_t>({
        0x48,0x83,0xEC,0x38,
        0x0F,0xB7,0x44,0x24,0x60,
        0x66,0x89,0x44,0x24,0x20,
    });
    constexpr auto Suffix = std::to_array<std::uint8_t>({
        0x48,0xB8,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0xFF,0xD0,
        0x48,0x83,0xC4,0x38,
        0xC3,
    });
    std::memcpy(destination, Prefix.data(), Prefix.size());
    auto offset = Prefix.size();
    if (fullLevelIdInR12) {
        constexpr auto MoveFullLevelId = std::to_array<std::uint8_t>({
            0x44,0x89,0x64,0x24,0x28,
        });
        std::memcpy(
            destination + offset,
            MoveFullLevelId.data(),
            MoveFullLevelId.size());
        offset += MoveFullLevelId.size();
    } else {
        constexpr auto MoveFullLevelId = std::to_array<std::uint8_t>({
            0x89,0x7C,0x24,0x28,
        });
        std::memcpy(
            destination + offset,
            MoveFullLevelId.data(),
            MoveFullLevelId.size());
        offset += MoveFullLevelId.size();
    }
    std::memcpy(destination + offset, Suffix.data(), Suffix.size());
    const auto address = reinterpret_cast<std::uintptr_t>(target);
    std::memcpy(destination + offset + 2, &address, sizeof(address));
    return true;
}

bool IsRel32Reachable(
        std::uintptr_t callRva,
        std::uintptr_t targetRva) noexcept {
    const auto displacement = static_cast<std::int64_t>(targetRva)
        - static_cast<std::int64_t>(callRva + 5);
    return displacement >= std::numeric_limits<std::int32_t>::min()
        && displacement <= std::numeric_limits<std::int32_t>::max();
}

bool InstallPortalRecordCallRedirects() noexcept {
    constexpr std::size_t RelayStride = 64;
    constexpr std::size_t RelayCount = 7;
    constexpr std::size_t RelayBytes = RelayStride * RelayCount;
    if (!Context || !Context->exeBase) return false;

    auto* base = reinterpret_cast<std::uint8_t*>(Context->exeBase);
    PortalRecordRelayPage = AllocatePortalRecordRelayPage(
        base + PortalLevelsRecordCallRva);
    if (!PortalRecordRelayPage) return false;
    auto* relays = static_cast<std::uint8_t*>(PortalRecordRelayPage);
    if (!WritePortalRecordRelay(
            relays,
            reinterpret_cast<const void*>(&HookGetLevelsTxtRecord))
            || !WritePortalRecordRelay(
                relays + RelayStride,
                reinterpret_cast<const void*>(&HookGetLevelDefRecord))
            || !WriteClientPortalLabelRelay(
                relays + (RelayStride * 2),
                reinterpret_cast<const void*>(
                    &HookClientPortalLabelGetLevelsTxtRecord))
            || !WriteClientTownPortalRelay(
                relays + (RelayStride * 3),
                reinterpret_cast<const void*>(
                    &HookClientTownPortalGetLevelsTxtRecord))
            || !WriteClientTownPortalRelay(
                relays + (RelayStride * 4),
                reinterpret_cast<const void*>(
                    &HookClientTownPortalGetLevelDefRecord))
            || !WritePortalStateFullLevelRelay(
                relays + (RelayStride * 5),
                reinterpret_cast<const void*>(
                    &HookSendPortalStatePacketWithFullId),
                true)
            || !WritePortalStateFullLevelRelay(
                relays + (RelayStride * 6),
                reinterpret_cast<const void*>(
                    &HookSendPortalStatePacketWithFullId),
                false)) {
        VirtualFree(PortalRecordRelayPage, 0, MEM_RELEASE);
        PortalRecordRelayPage = nullptr;
        return false;
    }
    DWORD previousProtection{};
    if (!VirtualProtect(
            PortalRecordRelayPage,
            RelayBytes,
            PAGE_EXECUTE_READ,
            &previousProtection)) {
        VirtualFree(PortalRecordRelayPage, 0, MEM_RELEASE);
        PortalRecordRelayPage = nullptr;
        return false;
    }
    FlushInstructionCache(
        GetCurrentProcess(),
        PortalRecordRelayPage,
        RelayBytes);

    const auto relayAddress = reinterpret_cast<std::uintptr_t>(
        PortalRecordRelayPage);
    const auto baseAddress = reinterpret_cast<std::uintptr_t>(base);
    if (relayAddress < baseAddress
            || relayAddress - baseAddress
                > std::numeric_limits<std::uint32_t>::max()) {
        VirtualFree(PortalRecordRelayPage, 0, MEM_RELEASE);
        PortalRecordRelayPage = nullptr;
        return false;
    }
    const auto relayRva = relayAddress - baseAddress;
    if (!IsRel32Reachable(PortalLevelsRecordCallRva, relayRva)
            || !IsRel32Reachable(
                PortalLevelDefRecordCallRva,
                relayRva + RelayStride)
            || !IsRel32Reachable(
                ClientPortalLabelLevelsRecordCallRva,
                relayRva + (RelayStride * 2))
            || !IsRel32Reachable(
                ClientTownPortalLevelsRecordCallRva,
                relayRva + (RelayStride * 3))
            || !IsRel32Reachable(
                ClientTownPortalLevelDefRecordCallRva,
                relayRva + (RelayStride * 4))
            || !IsRel32Reachable(
                PortalStatePrimarySendCallRva,
                relayRva + (RelayStride * 5))
            || !IsRel32Reachable(
                PortalStateSecondarySendCallRva,
                relayRva + (RelayStride * 6))) {
        VirtualFree(PortalRecordRelayPage, 0, MEM_RELEASE);
        PortalRecordRelayPage = nullptr;
        return false;
    }
    if (!Context->PatchCallRel32(
            PortalLevelsRecordCallRva,
            PortalLevelsRecordCallExpected.data(),
            static_cast<std::uint32_t>(
                PortalLevelsRecordCallExpected.size()),
            relayRva,
            static_cast<std::uint32_t>(
                PortalLevelsRecordCallExpected.size()))) {
        VirtualFree(PortalRecordRelayPage, 0, MEM_RELEASE);
        PortalRecordRelayPage = nullptr;
        return false;
    }
    if (!Context->PatchCallRel32(
        PortalLevelDefRecordCallRva,
        PortalLevelDefRecordCallExpected.data(),
        static_cast<std::uint32_t>(
            PortalLevelDefRecordCallExpected.size()),
        relayRva + RelayStride,
        static_cast<std::uint32_t>(
            PortalLevelDefRecordCallExpected.size()))) {
        return false;
    }
    if (!Context->PatchCallRel32(
            ClientPortalLabelLevelsRecordCallRva,
            ClientPortalLabelLevelsRecordCallExpected.data(),
            static_cast<std::uint32_t>(
                ClientPortalLabelLevelsRecordCallExpected.size()),
            relayRva + (RelayStride * 2),
            static_cast<std::uint32_t>(
                ClientPortalLabelLevelsRecordCallExpected.size()))
            || !Context->PatchCallRel32(
                ClientTownPortalLevelsRecordCallRva,
                ClientTownPortalLevelsRecordCallExpected.data(),
                static_cast<std::uint32_t>(
                    ClientTownPortalLevelsRecordCallExpected.size()),
                relayRva + (RelayStride * 3),
                static_cast<std::uint32_t>(
                    ClientTownPortalLevelsRecordCallExpected.size()))
            || !Context->PatchCallRel32(
                ClientTownPortalLevelDefRecordCallRva,
                ClientTownPortalLevelDefRecordCallExpected.data(),
                static_cast<std::uint32_t>(
                    ClientTownPortalLevelDefRecordCallExpected.size()),
                relayRva + (RelayStride * 4),
                static_cast<std::uint32_t>(
                    ClientTownPortalLevelDefRecordCallExpected.size()))
            || !Context->PatchCallRel32(
                PortalStatePrimarySendCallRva,
                PortalStatePrimarySendCallExpected.data(),
                static_cast<std::uint32_t>(
                    PortalStatePrimarySendCallExpected.size()),
                relayRva + (RelayStride * 5),
                static_cast<std::uint32_t>(
                    PortalStatePrimarySendCallExpected.size()))) {
        return false;
    }
    return Context->PatchCallRel32(
        PortalStateSecondarySendCallRva,
        PortalStateSecondarySendCallExpected.data(),
        static_cast<std::uint32_t>(
            PortalStateSecondarySendCallExpected.size()),
        relayRva + (RelayStride * 6),
        static_cast<std::uint32_t>(
            PortalStateSecondarySendCallExpected.size()));
}

void __fastcall HookSendObjectSpawnPacket(
        void* client,
        std::uint8_t opcode,
        std::uint8_t unitType,
        std::uint32_t guid,
        std::uint16_t objectId,
        std::uint16_t x,
        std::uint16_t y,
        std::uint8_t animation,
        std::uint8_t interactType) noexcept {
    const auto isTownPortalPacket = opcode == 0x51
        && unitType == 2
        && objectId == DynamicTownPortalClassId;
    if (PortalSessionPoisoned.load(std::memory_order_acquire)
            && isTownPortalPacket) {
        RefusePortalContractOnce(
            "ExtendedActLevelIds: Town Portal object packet refused after a session sidecar failure.");
        return;
    }
    if (!isTownPortalPacket) {
        OriginalSendObjectSpawnPacket(
            client,
            opcode,
            unitType,
            guid,
            objectId,
            x,
            y,
            animation,
            interactType);
        return;
    }
    const auto endpoint = isTownPortalPacket
        ? FindPortalEndpoint(
            0,
            guid,
            objectId,
            interactType)
        : std::nullopt;
    const auto packetGeneration = SessionGeneration.load(
        std::memory_order_acquire);
    if (!endpoint) {
        if (IsLocalClient(client)
                && !EvictClientPortalGuid(guid, packetGeneration)) {
            return;
        }
        OriginalSendObjectSpawnPacket(
            client,
            opcode,
            unitType,
            guid,
            objectId,
            x,
            y,
            animation,
            interactType);
        return;
    }
    if (!IsLocalClient(client)) {
        RefusedPortalClientIdentities.fetch_add(
            1,
            std::memory_order_relaxed);
        RefusePortalContractOnce(
            "ExtendedActLevelIds: extended Town Portal object packet refused for a non-local client identity.");
        return;
    }
    const auto dataContext = ReadValue<std::uint8_t>(
        reinterpret_cast<const void*>(endpoint->gameIdentity),
        GameDataContextOffset);
    if (!IsKnownLevelIdForContext(
            dataContext,
            endpoint->destinationLevelId)) {
        RefusedPortalSidecars.fetch_add(1, std::memory_order_relaxed);
        PoisonPortalSession(
            "ExtendedActLevelIds: Town Portal object sidecar referenced an unknown destination Level ID; the session was refused.");
        return;
    }
    if (!PublishClientPortal(*endpoint)) {
        return;
    }
    PublishedPortalPackets.fetch_add(1, std::memory_order_relaxed);
    OriginalSendObjectSpawnPacket(
        client,
        opcode,
        unitType,
        guid,
        objectId,
        x,
        y,
        animation,
        interactType);
}

void __fastcall HookHandleObjectSpawnPacket(
        const std::uint8_t* packet) noexcept {
    if (!packet
            || packet[0] != 0x51
            || packet[1] != 2
            || ReadValue<std::uint16_t>(packet, 6)
                != DynamicTownPortalClassId) {
        OriginalHandleObjectSpawnPacket(packet);
        return;
    }
    const auto guid = ReadValue<std::uint32_t>(packet, 2);
    const auto packetGeneration = SessionGeneration.load(
        std::memory_order_acquire);
    const auto descriptor = FindClientPortalDescriptor(
        packetGeneration,
        guid);
    if (!descriptor) {
        if (!EvictClientPortalGuid(guid, packetGeneration)) return;
        OriginalHandleObjectSpawnPacket(packet);
        return;
    }
    if (PortalSessionPoisoned.load(std::memory_order_acquire)) {
        RefusePortalContractOnce(
            "ExtendedActLevelIds: extended Town Portal object packet refused after a session sidecar failure.");
        return;
    }
    OriginalHandleObjectSpawnPacket(packet);
    if (packetGeneration
            != SessionGeneration.load(std::memory_order_acquire)
            || !ClientGetUnitByIdAndType) {
        return;
    }
    const auto* unit = ClientGetUnitByIdAndType(guid, 2);
    std::uint32_t liveGuid{};
    std::uint8_t liveDestinationLowLevelId{};
    if (!unit
            || !ReadPortalUnit(
                unit,
                liveGuid,
                liveDestinationLowLevelId)
            || liveGuid != guid
            || !IsValidClientPortalDescriptor(*descriptor)
            || descriptor->nativeLowLevelId != packet[13]
            || liveDestinationLowLevelId != packet[13]) {
        RefusedPortalSidecars.fetch_add(1, std::memory_order_relaxed);
        PoisonClientPortalSession(
            packetGeneration,
            "ExtendedActLevelIds: client Town Portal object sidecar did not match the live object; the session was refused.");
        return;
    }
    ValidatedPortalPackets.fetch_add(1, std::memory_order_relaxed);
}

void __fastcall HookSendPortalStatePacketWithFullId(
        void* client,
        void* portal,
        std::uint8_t ownerRoomLowLevelId,
        std::uint16_t ownerX,
        std::uint16_t ownerY,
        std::int32_t ownerRoomLevelId) noexcept {
    if (!OriginalSendPortalStatePacket) return;
    if (PortalSessionPoisoned.load(std::memory_order_acquire)
            && portal
            && ReadValue<std::uint32_t>(portal, UnitClassIdOffset)
                == DynamicTownPortalClassId) {
        RefusePortalContractOnce(
            "ExtendedActLevelIds: Town Portal state packet refused after a session sidecar failure.");
        return;
    }
    const auto endpoint = FindPortalEndpointForUnit(nullptr, portal);
    if (!endpoint) {
        OriginalSendPortalStatePacket(
            client,
            portal,
            ownerRoomLowLevelId,
            ownerX,
            ownerY);
        return;
    }
    if (!IsLocalClient(client)) {
        RefusedPortalClientIdentities.fetch_add(
            1,
            std::memory_order_relaxed);
        RefusePortalContractOnce(
            "ExtendedActLevelIds: extended Town Portal state packet refused for a non-local client identity.");
        return;
    }
    const auto dataContext = ReadValue<std::uint8_t>(
        reinterpret_cast<const void*>(endpoint->gameIdentity),
        GameDataContextOffset);
    if (ownerRoomLevelId <= 0
            || ownerRoomLevelId > MaximumLevelId
            || ownerRoomLowLevelId != LowLevelId(ownerRoomLevelId)
            || !IsKnownLevelIdForContext(
                dataContext,
                endpoint->destinationLevelId)
            || !IsKnownLevelIdForContext(
                dataContext,
                ownerRoomLevelId)) {
        RefusedPortalSidecars.fetch_add(1, std::memory_order_relaxed);
        PoisonPortalSession(
            "ExtendedActLevelIds: Town Portal state sidecar rejected an invalid full owner-room Level ID; the session was refused.");
        return;
    }
    if (!PublishClientPortal(*endpoint, ownerRoomLevelId)) {
        return;
    }
    PublishedPortalPackets.fetch_add(1, std::memory_order_relaxed);
    OriginalSendPortalStatePacket(
        client,
        portal,
        ownerRoomLowLevelId,
        ownerX,
        ownerY);
}

void __fastcall HookHandlePortalStatePacket(
        const std::uint8_t* packet) noexcept {
    if (!packet || packet[0] != 0x60) {
        OriginalHandlePortalStatePacket(packet);
        return;
    }
    const auto guid = ReadValue<std::uint32_t>(packet, 3);
    const auto packetGeneration = SessionGeneration.load(
        std::memory_order_acquire);
    const auto descriptor = FindClientPortalDescriptor(
        packetGeneration,
        guid);
    if (!descriptor) {
        OriginalHandlePortalStatePacket(packet);
        return;
    }
    if (PortalSessionPoisoned.load(std::memory_order_acquire)) {
        RefusePortalContractOnce(
            "ExtendedActLevelIds: extended Town Portal state packet refused after a session sidecar failure.");
        return;
    }
    OriginalHandlePortalStatePacket(packet);
    if (packetGeneration
            != SessionGeneration.load(std::memory_order_acquire)) {
        return;
    }
    if (!ClientGetUnitByIdAndType) {
        PoisonClientPortalSession(
            packetGeneration,
            "ExtendedActLevelIds: client Town Portal resolver is unavailable; the session was refused.");
        return;
    }
    const auto* unit = ClientGetUnitByIdAndType(guid, 2);
    std::uint32_t liveGuid{};
    std::uint8_t liveDestinationLowLevelId{};
    const auto liveOwnerRoomLowLevelId = ReadValue<std::uint8_t>(
        unit,
        ClientPortalOwnerRoomLevelIdOffset);
    const auto ownerRoomKnown = descriptor->ownerRoomLevelId
        != UnknownClientPortalLevelId;
    if (!unit
            || !ReadPortalUnit(
                unit,
                liveGuid,
                liveDestinationLowLevelId)
            || liveGuid != guid
            || !IsValidClientPortalDescriptor(*descriptor)
            || descriptor->nativeLowLevelId != packet[2]
            || liveDestinationLowLevelId != packet[2]
            || liveOwnerRoomLowLevelId != packet[11]
            || (ownerRoomKnown
                && descriptor->ownerRoomNativeLowLevelId != packet[11])) {
        RefusedPortalSidecars.fetch_add(1, std::memory_order_relaxed);
        PoisonClientPortalSession(
            packetGeneration,
            "ExtendedActLevelIds: client Town Portal state sidecar did not match the live object; the session was refused.");
        return;
    }
    ValidatedPortalPackets.fetch_add(1, std::memory_order_relaxed);
}

bool IsCompatiblePlayer(std::uint32_t playerId) noexcept {
    if (playerId == InvalidPlayerId) return false;
    const auto localPlayerId = LocalPlayerId.load(std::memory_order_acquire);
    if (localPlayerId != InvalidPlayerId && localPlayerId == playerId) {
        return true;
    }
    const auto peers = CompatiblePeers.load(std::memory_order_acquire);
    return peers && std::any_of(
        peers->entries.begin(),
        peers->entries.end(),
        [playerId](const CompatiblePeerEntry& entry) {
            return entry.playerId == playerId;
        });
}

std::size_t CompatiblePeerCount() noexcept {
    const auto peers = CompatiblePeers.load(std::memory_order_acquire);
    return peers ? peers->entries.size() : 0;
}

void PublishCompatiblePeer(
        D2RL::Network::PeerHandle peer,
        std::uint32_t playerId) noexcept {
    if (peer == D2RL::Network::InvalidPeerHandle) return;
    try {
        auto current = CompatiblePeers.load(std::memory_order_acquire);
        for (;;) {
            auto mutableNext = std::make_shared<CompatiblePeerMap>();
            if (current) mutableNext->entries = current->entries;
            std::erase_if(
                mutableNext->entries,
                [peer](const CompatiblePeerEntry& entry) {
                    return entry.peer == peer;
                });
            mutableNext->entries.push_back({peer, playerId});
            std::shared_ptr<const CompatiblePeerMap> next{mutableNext};
            if (CompatiblePeers.compare_exchange_weak(
                    current,
                    next,
                    std::memory_order_release,
                    std::memory_order_acquire)) {
                CompatiblePeerAnnouncements.fetch_add(
                    1,
                    std::memory_order_relaxed);
                return;
            }
        }
    } catch (...) {
        if (Context) Context->LogError(
            "ExtendedActLevelIds: compatible peer map update failed.");
    }
}

void RemoveCompatiblePeer(D2RL::Network::PeerHandle peer) noexcept {
    if (peer == D2RL::Network::InvalidPeerHandle) return;
    try {
        auto current = CompatiblePeers.load(std::memory_order_acquire);
        while (current) {
            auto mutableNext = std::make_shared<CompatiblePeerMap>();
            mutableNext->entries = current->entries;
            std::erase_if(
                mutableNext->entries,
                [peer](const CompatiblePeerEntry& entry) {
                    return entry.peer == peer;
                });
            std::shared_ptr<const CompatiblePeerMap> next =
                mutableNext->entries.empty()
                    ? std::shared_ptr<const CompatiblePeerMap>{}
                    : std::shared_ptr<const CompatiblePeerMap>{mutableNext};
            if (CompatiblePeers.compare_exchange_weak(
                    current,
                    next,
                    std::memory_order_release,
                    std::memory_order_acquire)) {
                return;
            }
        }
    } catch (...) {
        if (Context) Context->LogError(
            "ExtendedActLevelIds: compatible peer removal failed.");
    }
}

void ResetNetworkSession(std::uint64_t sessionGeneration = 0) noexcept {
    CompatiblePeers.store({}, std::memory_order_release);
    {
        std::scoped_lock lock(
            PortalPublicationMutex,
            ClientPortalPublicationMutex);
        SessionGeneration.store(sessionGeneration, std::memory_order_release);
        PortalEndpoints.store({}, std::memory_order_release);
        ClientPortalEndpoints.store({}, std::memory_order_release);
        PortalSessionPoisoned.store(false, std::memory_order_release);
        LocalPlayerId.store(InvalidPlayerId, std::memory_order_release);
    }
}

void WarnNetworkUnavailableOnce(const char* message) noexcept {
    if (NetworkWarnings.fetch_add(1, std::memory_order_relaxed) == 0
            && Context) {
        Context->LogWarn(message);
    }
}

void SendCompatibilityHandshake(
        const D2RL::PluginContext* context) noexcept {
    if (!context
            || !Network
            || NetworkChannel == D2RL::Network::InvalidChannelHandle) {
        return;
    }
    const auto playerId = LocalPlayerId.load(std::memory_order_acquire);
    if (playerId == InvalidPlayerId) return;
    const auto result = Network->sendToHost(
        context,
        NetworkChannel,
        CompatibilityHandshakeMessage,
        &playerId,
        sizeof(playerId));
    if (result != D2RL::Network::Result::Success
            && result != D2RL::Network::Result::WrongRole
            && result != D2RL::Network::Result::NotConnected) {
        WarnNetworkUnavailableOnce(
            "ExtendedActLevelIds: compatibility handshake is unavailable; Level IDs above 255 will fail closed.");
    }
}

void ConnectOrAnnounce(const D2RL::PluginContext* context) noexcept {
    if (!context
            || !Network
            || NetworkChannel == D2RL::Network::InvalidChannelHandle) {
        return;
    }
    D2RL::Network::ChannelInfo info{
        .structSize = D2RL::Network::ChannelInfoSize,
    };
    if (Network->getChannelInfo(
            context,
            NetworkChannel,
            &info) == D2RL::Network::Result::Success
            && info.connectionState
                == D2RL::Network::ConnectionState::Connected) {
        SendCompatibilityHandshake(context);
        return;
    }
    const auto result = Network->connectToHost(context, NetworkChannel);
    if (result != D2RL::Network::Result::Success
            && result != D2RL::Network::Result::Busy
            && result != D2RL::Network::Result::NotConnected) {
        WarnNetworkUnavailableOnce(
            "ExtendedActLevelIds: no compatible private game channel; Level IDs above 255 will fail closed.");
    }
}

void __cdecl OnHostCompatibilityMessage(
        const D2RL::PluginContext* context,
        D2RL::Network::ChannelHandle channel,
        D2RL::Network::PeerHandle peer,
        std::uint16_t messageId,
        const void* data,
        std::uint32_t size,
        void*) noexcept {
    if (!context
            || channel != NetworkChannel
            || messageId != CompatibilityHandshakeMessage
            || !data
            || size != sizeof(std::uint32_t)) {
        return;
    }
    std::uint32_t playerId{};
    std::memcpy(&playerId, data, sizeof(playerId));
    PublishCompatiblePeer(peer, playerId);
}

void __cdecl OnClientCompatibilityMessage(
        const D2RL::PluginContext*,
        D2RL::Network::ChannelHandle,
        std::uint16_t,
        const void*,
        std::uint32_t,
        void*) noexcept {
}

void __cdecl OnNetworkConnectionState(
        const D2RL::PluginContext* context,
        const D2RL::Network::ConnectionEvent* event,
        void*) noexcept {
    if (!context
            || !D2RL::Network::HasConnectionEventField(
                event,
                D2RL::Network::ConnectionEventRequiredSize)
            || event->channel != NetworkChannel) {
        return;
    }
    if (event->state == D2RL::Network::ConnectionState::Connected) {
        SendCompatibilityHandshake(context);
    } else if (event->state
            == D2RL::Network::ConnectionState::Disconnected
            || event->state == D2RL::Network::ConnectionState::Rejected) {
        RemoveCompatiblePeer(event->peer);
    }
}

void __cdecl OnGameplayEvent(
        const D2RL::PluginContext* context,
        const D2RL::Lifecycle::GameplayEvent* event,
        void*) noexcept {
    if (!context
            || !D2RL::Lifecycle::HasGameplayEventField(
                event,
                D2RL::Lifecycle::GameplayEventRequiredSize)) {
        return;
    }
    if (event->kind == D2RL::Lifecycle::GameplayEventKind::GameLeft) {
        ResetNetworkSession();
        return;
    }
    if (event->kind == D2RL::Lifecycle::GameplayEventKind::GameJoined) {
        ResetNetworkSession(event->sessionGeneration);
        return;
    }
    if (event->sessionGeneration
            != SessionGeneration.load(std::memory_order_acquire)) {
        ResetNetworkSession(event->sessionGeneration);
    }
    if (event->kind
            == D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady) {
        LocalPlayerId.store(event->playerId, std::memory_order_release);
    }
    if (event->kind
                == D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady
            || event->kind == D2RL::Lifecycle::GameplayEventKind::ActChanged
            || event->kind == D2RL::Lifecycle::GameplayEventKind::LevelChanged
            || event->kind
                == D2RL::Lifecycle::GameplayEventKind::PlayerResurrected) {
        ConnectOrAnnounce(context);
    }
}

void RefuseVisibilityPacketOnce(const char* reason) noexcept {
    if (RefusedVisibilityPackets.fetch_add(1, std::memory_order_relaxed) == 0
            && Context) {
        Context->LogError(reason);
    }
}

void SendRoomVisibilityPacket(
        SendRoomVisibilityPacketFn original,
        void* client,
        std::int32_t levelId,
        std::uint16_t x,
        std::uint16_t y) noexcept {
    if (!original) return;
    if (levelId <= MaximumVanillaNetworkLevelId) {
        original(client, levelId, x, y);
        return;
    }
    if (!Operational.load(std::memory_order_acquire)
            || !client
            || !IsKnownExtendedLevelId(levelId)) {
        RefuseVisibilityPacketOnce(
            "ExtendedActLevelIds: unknown extended Level ID refused before packet encoding.");
        return;
    }
    const auto playerId = ReadValue<std::uint32_t>(
        client,
        D2ClientPlayerIdOffset);
    if (!IsCompatiblePlayer(playerId)) {
        RefuseVisibilityPacketOnce(
            "ExtendedActLevelIds: extended Level ID refused for a peer without the compatible v2 channel.");
        return;
    }
    const auto encoded = EncodeLevelCoordinate(levelId, x);
    if (!encoded) {
        RefuseVisibilityPacketOnce(
            "ExtendedActLevelIds: extended Level ID or room X coordinate exceeds the v2 codec contract.");
        return;
    }
    EncodedVisibilityPackets.fetch_add(1, std::memory_order_relaxed);
    original(client, levelId, encoded->x, y);
}

void __fastcall HookSendRoomInSightPacket(
        void* client,
        std::int32_t levelId,
        std::uint16_t x,
        std::uint16_t y) noexcept {
    SendRoomVisibilityPacket(
        OriginalSendRoomInSightPacket,
        client,
        levelId,
        x,
        y);
}

void __fastcall HookSendRoomOutOfSightPacket(
        void* client,
        std::int32_t levelId,
        std::uint16_t x,
        std::uint16_t y) noexcept {
    SendRoomVisibilityPacket(
        OriginalSendRoomOutOfSightPacket,
        client,
        levelId,
        x,
        y);
}

void UpdateClientSight(
        UpdateClientSightFn original,
        std::uint8_t dataContext,
        void* act,
        std::int32_t levelId,
        std::int32_t x,
        std::int32_t y,
        void* room) noexcept {
    if (!original) return;
    if ((static_cast<std::uint32_t>(x) & CodecMarkerMask) == 0) {
        original(dataContext, act, levelId, x, y, room);
        return;
    }
    const auto decoded = DecodeLevelCoordinate(levelId, x);
    if (!Operational.load(std::memory_order_acquire)
            || !decoded
            || !IsKnownExtendedLevelId(decoded->levelId)) {
        RefuseVisibilityPacketOnce(
            "ExtendedActLevelIds: malformed or unknown v2 room-visibility packet refused.");
        return;
    }
    DecodedVisibilityPackets.fetch_add(1, std::memory_order_relaxed);
    original(
        dataContext,
        act,
        decoded->levelId,
        decoded->x,
        y,
        room);
}

void __fastcall HookSetClientInSight(
        std::uint8_t dataContext,
        void* act,
        std::int32_t levelId,
        std::int32_t x,
        std::int32_t y,
        void* room) noexcept {
    UpdateClientSight(
        OriginalSetClientInSight,
        dataContext,
        act,
        levelId,
        x,
        y,
        room);
}

void __fastcall HookUnsetClientInSight(
        std::uint8_t dataContext,
        void* act,
        std::int32_t levelId,
        std::int32_t x,
        std::int32_t y,
        void* room) noexcept {
    UpdateClientSight(
        OriginalUnsetClientInSight,
        dataContext,
        act,
        levelId,
        x,
        y,
        room);
}

std::string_view Trim(std::string_view text) noexcept {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

bool ParseInteger(
        std::string_view text,
        std::int32_t& value) noexcept {
    text = Trim(text);
    if (text.empty()) return false;
    const auto parsed = std::from_chars(
        text.data(),
        text.data() + text.size(),
        value);
    return parsed.ec == std::errc{}
        && parsed.ptr == text.data() + text.size();
}

auto ResolveProbe(
        const D2RL::ConsoleCommandContext* command,
        std::string_view arguments) noexcept
        -> D2RL::ConsoleCommandResult {
    constexpr std::string_view verb{"resolve"};
    arguments = Trim(arguments);
    if (!arguments.starts_with(verb)
            || (arguments.size() > verb.size()
                && arguments[verb.size()] != ' ')) {
        return D2RL::ConsoleCommandResult::InvalidArguments;
    }
    arguments = Trim(arguments.substr(verb.size()));
    const auto separator = arguments.find_first_of(" \t");
    const auto levelText = separator == std::string_view::npos
        ? arguments
        : arguments.substr(0, separator);
    const auto contextText = separator == std::string_view::npos
        ? std::string_view{}
        : Trim(arguments.substr(separator + 1));

    std::int32_t levelId{};
    std::int32_t parsedContext{3};
    if (!ParseInteger(levelText, levelId)
            || (!contextText.empty()
                && !ParseInteger(contextText, parsedContext))
            || parsedContext < MinimumDataContext
            || parsedContext > MaximumDataContext
            || levelId < 0
            || levelId > MaximumLevelId) {
        return D2RL::ConsoleCommandResult::InvalidArguments;
    }
    if (!Operational.load(std::memory_order_acquire)
            || !CacheReady.load(std::memory_order_acquire)
            || !Context
            || !Context->exeBase) {
        command->plugin->WriteConsoleMessage(
            "Extended Act Level IDs: resolver is not operational.",
            D2RL::ConsoleMessageKind::Error);
        return D2RL::ConsoleCommandResult::Failed;
    }

    const auto dataContext = static_cast<std::uint8_t>(parsedContext);
    const auto cache = LoadCache(dataContext);
    const auto cachedAct = cache
        ? FindAct(std::span<const ActEntry>(cache->entries), levelId)
        : std::nullopt;
    const auto resolver = reinterpret_cast<ResolveActFromLevelIdFn>(
        Context->exeBase + ResolveActFromLevelIdRva);
    const auto resolvedAct = resolver(dataContext, levelId);

    char message[256]{};
    std::snprintf(
        message,
        sizeof(message),
        "Extended Act Level IDs: Level Id %d resolved to Act index %u (Act %u), data context %u, source=%s.",
        levelId,
        static_cast<unsigned>(resolvedAct),
        static_cast<unsigned>(resolvedAct) + 1,
        static_cast<unsigned>(dataContext),
        cachedAct ? "Levels.txt" : "original resolver");
    Context->LogInfo(message);
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}

auto Status(
        D2R::Game::Client*,
        const D2RL::ConsoleCommandContext* command,
        void*) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) {
        return D2RL::ConsoleCommandResult::Failed;
    }
    const auto arguments = command->args
        ? Trim(std::string_view(command->args, command->argsLength))
        : std::string_view{};
    if (!arguments.empty()) return ResolveProbe(command, arguments);
    const auto classic = ClassicCache.load(std::memory_order_acquire);
    const auto lod = LodCache.load(std::memory_order_acquire);
    const auto rotw = RotwCache.load(std::memory_order_acquire);
    const auto portals = PortalEndpoints.load(std::memory_order_acquire);
    const auto clientPortals = ClientPortalEndpoints.load(
        std::memory_order_acquire);
    char message[1024]{};
    std::snprintf(
        message,
        sizeof(message),
        "Extended Act Level IDs 2.1.2: %s; cache=%s; revision=%llu; rows=%zu/%zu/%zu; compatible peers=%zu; portal endpoints=%zu; client portal endpoints=%zu; portal blocked=%s; portal pairs=%llu; portal operations=%llu; portal packets publish/validate=%llu/%llu; portal identity/sidecar refusals=%llu/%llu; client portal publish/evict/full/fallback=%llu/%llu/%llu/%llu; portal refused=%llu; room packets=%llu/%llu/%llu; Levels resolutions=%llu; original fallbacks=%llu; build=%s.",
        Operational.load(std::memory_order_acquire) ? "active" : "inactive",
        CacheReady.load(std::memory_order_acquire) ? "ready" : "not ready",
        static_cast<unsigned long long>(
            PublishedRevision.load(std::memory_order_acquire)),
        classic ? classic->entries.size() : 0,
        lod ? lod->entries.size() : 0,
        rotw ? rotw->entries.size() : 0,
        CompatiblePeerCount(),
        portals ? portals->entries.size() : 0,
        clientPortals ? clientPortals->entries.size() : 0,
        PortalSessionPoisoned.load(std::memory_order_acquire)
            ? "yes" : "no",
        static_cast<unsigned long long>(
            PortalPairsPublished.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            PortalOperations.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            PublishedPortalPackets.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            ValidatedPortalPackets.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            RefusedPortalClientIdentities.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            RefusedPortalSidecars.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            ClientPortalPublications.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            ClientPortalEvictions.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            ClientPortalFullLookups.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            ClientPortalFallbacks.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            RefusedPortalOperations.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            EncodedVisibilityPackets.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            DecodedVisibilityPackets.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            RefusedVisibilityPackets.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            ResolvedFromLevels.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            OriginalFallbacks.load(std::memory_order_relaxed)),
        RuntimeBuildName.c_str());
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}

void ResetState() noexcept {
    Operational.store(false, std::memory_order_release);
    ResetCaches();
    ResetNetworkSession();
    ResolvedFromLevels.store(0, std::memory_order_relaxed);
    OriginalFallbacks.store(0, std::memory_order_relaxed);
    EncodedVisibilityPackets.store(0, std::memory_order_relaxed);
    DecodedVisibilityPackets.store(0, std::memory_order_relaxed);
    RefusedVisibilityPackets.store(0, std::memory_order_relaxed);
    CompatiblePeerAnnouncements.store(0, std::memory_order_relaxed);
    NetworkWarnings.store(0, std::memory_order_relaxed);
    PortalPairsPublished.store(0, std::memory_order_relaxed);
    PortalOperations.store(0, std::memory_order_relaxed);
    PublishedPortalPackets.store(0, std::memory_order_relaxed);
    ValidatedPortalPackets.store(0, std::memory_order_relaxed);
    RefusedPortalClientIdentities.store(0, std::memory_order_relaxed);
    RefusedPortalSidecars.store(0, std::memory_order_relaxed);
    RefusedPortalOperations.store(0, std::memory_order_relaxed);
    ClientPortalPublications.store(0, std::memory_order_relaxed);
    ClientPortalEvictions.store(0, std::memory_order_relaxed);
    ClientPortalFullLookups.store(0, std::memory_order_relaxed);
    ClientPortalFallbacks.store(0, std::memory_order_relaxed);
    PortalSessionPoisoned.store(false, std::memory_order_relaxed);
    DataTables = nullptr;
    Network = nullptr;
    NetworkChannel = D2RL::Network::InvalidChannelHandle;
    OriginalResolveActFromLevelId = nullptr;
    OriginalSendRoomInSightPacket = nullptr;
    OriginalSendRoomOutOfSightPacket = nullptr;
    OriginalSetClientInSight = nullptr;
    OriginalUnsetClientInSight = nullptr;
    OriginalCreateLinkPortal = nullptr;
    OriginalGetLevelIdFromRoom = nullptr;
    OriginalGetPortalOwner = nullptr;
    OriginalOperatePortal = nullptr;
    OriginalGetLevelsTxtRecord = nullptr;
    OriginalGetLevelDefRecord = nullptr;
    ClientGetUnitByIdAndType = nullptr;
    OriginalSendObjectSpawnPacket = nullptr;
    OriginalHandleObjectSpawnPacket = nullptr;
    OriginalSendPortalStatePacket = nullptr;
    OriginalHandlePortalStatePacket = nullptr;
    ActivePortalCreation = nullptr;
    ActivePortalOperation = nullptr;
    ScopedPortalLevelId = -1;
}

} // namespace
} // namespace RuffnecKk::ExtendedActLevelIds

namespace {

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "ruffneckk-extended-act-level-ids",
    .name = "Extended Act Level IDs",
    .version = "2.1.2",
    .author = "RuffnecKk",
    .description = "Extends functional level IDs to the native 1023-record limit.",
    .flags = D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks,
};

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept
        -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    using namespace RuffnecKk::ExtendedActLevelIds;
    Context = context;
    ResetState();
    if (!Context || !Context->exeBase) return false;
    if (!AcquireSingleton()) return false;

    const auto* runtimeBuild = D2RL::GetBuildName(Context);
    RuntimeBuildName = runtimeBuild ? runtimeBuild : "unknown";
    if (!Context->RegisterConsoleCommand(
            "extended-act-level-ids",
            Status,
            "Show status or resolve a cached Level ID.")) {
        Context->LogWarn(
            "ExtendedActLevelIds: status command could not be registered.");
    }
    if (!ValidateRuntime()) {
        ReleaseSingleton();
        return false;
    }

    const D2RL::LifecycleServiceV1* lifecycle{};
    if (!QueryServices(lifecycle)) {
        ReleaseSingleton();
        return false;
    }
    const D2RL::Network::ChannelRegistration channelRegistration{
        .structSize = D2RL::Network::ChannelRegistrationSize,
        .flags = 0,
        .localChannelId = NetworkLocalChannelId,
        .reserved = 0,
        .reserved2 = 0,
        .compatibilityToken = NetworkCompatibilityToken,
        .hostMessage = OnHostCompatibilityMessage,
        .clientMessage = OnClientCompatibilityMessage,
        .connectionState = OnNetworkConnectionState,
        .userData = nullptr,
    };
    if (Network->registerChannel(
            Context,
            &channelRegistration,
            &NetworkChannel) != D2RL::Network::Result::Success
            || NetworkChannel == D2RL::Network::InvalidChannelHandle) {
        Context->LogError(
            "ExtendedActLevelIds: v2 compatibility channel registration failed.");
        ReleaseSingleton();
        return false;
    }
    const D2RL::Lifecycle::DataTablesLoadedListener listener{
        .structSize = D2RL::Lifecycle::DataTablesLoadedListenerSize,
        .flags = 0,
        .callback = OnDataTablesLoaded,
        .userData = nullptr,
    };
    D2RL::Lifecycle::ListenerHandle listenerHandle{
        D2RL::Lifecycle::InvalidHandle};
    if (lifecycle->registerDataTablesLoadedListener(
            Context,
            &listener,
            &listenerHandle) != D2RL::Lifecycle::Result::Success
            || listenerHandle == D2RL::Lifecycle::InvalidHandle) {
        Context->LogError(
            "ExtendedActLevelIds: DataTablesLoaded listener registration failed.");
        (void)Network->unregisterChannel(Context, NetworkChannel);
        NetworkChannel = D2RL::Network::InvalidChannelHandle;
        ReleaseSingleton();
        return false;
    }

    constexpr std::array gameplayKinds{
        D2RL::Lifecycle::GameplayEventKind::GameJoined,
        D2RL::Lifecycle::GameplayEventKind::GameLeft,
        D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady,
        D2RL::Lifecycle::GameplayEventKind::ActChanged,
        D2RL::Lifecycle::GameplayEventKind::LevelChanged,
        D2RL::Lifecycle::GameplayEventKind::PlayerResurrected,
    };
    for (const auto kind : gameplayKinds) {
        const D2RL::Lifecycle::GameplayEventListener gameplayListener{
            .structSize = D2RL::Lifecycle::GameplayEventListenerSize,
            .flags = 0,
            .kind = kind,
            .reserved = 0,
            .callback = OnGameplayEvent,
            .userData = nullptr,
        };
        D2RL::Lifecycle::ListenerHandle gameplayHandle{
            D2RL::Lifecycle::InvalidHandle};
        if (lifecycle->registerGameplayEventListener(
                Context,
                &gameplayListener,
                &gameplayHandle) != D2RL::Lifecycle::Result::Success
                || gameplayHandle == D2RL::Lifecycle::InvalidHandle) {
            Context->LogError(
                "ExtendedActLevelIds: gameplay listener registration failed.");
            (void)Network->unregisterChannel(Context, NetworkChannel);
            NetworkChannel = D2RL::Network::InvalidChannelHandle;
            ReleaseSingleton();
            return false;
        }
    }

    if (!Context->InstallInlineHook(
            ResolveActFromLevelIdRva,
            ResolveActFromLevelIdExpected.data(),
            static_cast<std::uint32_t>(
                ResolveActFromLevelIdExpected.size()),
            HookResolveActFromLevelId,
            &OriginalResolveActFromLevelId)) {
        Context->LogError(
            "ExtendedActLevelIds: central resolver hook is already owned or unavailable.");
        (void)Network->unregisterChannel(Context, NetworkChannel);
        NetworkChannel = D2RL::Network::InvalidChannelHandle;
        ReleaseSingleton();
        return false;
    }

    OriginalGetLevelsTxtRecord = reinterpret_cast<GetLevelRecordFn>(
        Context->exeBase + DATATBLS_GetLevelsTxtRecordRva);
    OriginalGetLevelDefRecord = reinterpret_cast<GetLevelRecordFn>(
        Context->exeBase + DATATBLS_GetLevelDefRecordRva);
    ClientGetUnitByIdAndType =
        reinterpret_cast<ClientGetUnitByIdAndTypeFn>(
            Context->exeBase + CLIENT_GetUnitByIdAndTypeRva);
    OriginalSendPortalStatePacket =
        reinterpret_cast<SendPortalStatePacketFn>(
            Context->exeBase + D2GAME_SendPacket0x60_PortalStateRva);
    if (!InstallPortalRecordCallRedirects()) {
        Context->LogError(
            "ExtendedActLevelIds: portal-local Levels call redirects are unavailable.");
        (void)Network->unregisterChannel(Context, NetworkChannel);
        NetworkChannel = D2RL::Network::InvalidChannelHandle;
        ReleaseSingleton();
        return false;
    }

    if (!Context->InstallInlineHook(
            SendRoomInSightPacketRva,
            SendRoomInSightPacketExpected.data(),
            static_cast<std::uint32_t>(
                SendRoomInSightPacketExpected.size()),
            HookSendRoomInSightPacket,
            &OriginalSendRoomInSightPacket)
            || !Context->InstallInlineHook(
                SendRoomOutOfSightPacketRva,
                SendRoomOutOfSightPacketExpected.data(),
                static_cast<std::uint32_t>(
                    SendRoomOutOfSightPacketExpected.size()),
                HookSendRoomOutOfSightPacket,
                &OriginalSendRoomOutOfSightPacket)
            || !Context->InstallInlineHook(
                SetClientInSightRva,
                SetClientInSightExpected.data(),
                static_cast<std::uint32_t>(
                    SetClientInSightExpected.size()),
                HookSetClientInSight,
                &OriginalSetClientInSight)
            || !Context->InstallInlineHook(
                UnsetClientInSightRva,
                UnsetClientInSightExpected.data(),
                static_cast<std::uint32_t>(
                    UnsetClientInSightExpected.size()),
                HookUnsetClientInSight,
                &OriginalUnsetClientInSight)) {
        Context->LogError(
            "ExtendedActLevelIds: a v2 room-visibility hook is already owned or unavailable.");
        (void)Network->unregisterChannel(Context, NetworkChannel);
        NetworkChannel = D2RL::Network::InvalidChannelHandle;
        ReleaseSingleton();
        return false;
    }

    if (!Context->InstallInlineHook(
            DUNGEON_GetLevelIdFromRoomRva,
            DUNGEON_GetLevelIdFromRoomExpected.data(),
            static_cast<std::uint32_t>(
                DUNGEON_GetLevelIdFromRoomExpected.size()),
            HookGetLevelIdFromRoom,
            &OriginalGetLevelIdFromRoom)
            || !Context->InstallInlineHook(
                D2GAME_CreateLinkPortalRva,
                D2GAME_CreateLinkPortalExpected.data(),
                static_cast<std::uint32_t>(
                    D2GAME_CreateLinkPortalExpected.size()),
                HookCreateLinkPortal,
                &OriginalCreateLinkPortal)
            || !Context->InstallInlineHook(
                SUNIT_GetPortalOwnerRva,
                SUNIT_GetPortalOwnerExpected.data(),
                static_cast<std::uint32_t>(
                    SUNIT_GetPortalOwnerExpected.size()),
                HookGetPortalOwner,
                &OriginalGetPortalOwner)
            || !Context->InstallInlineHook(
                OBJECTS_OperateFunction15_PortalRva,
                OBJECTS_OperateFunction15_PortalExpected.data(),
                static_cast<std::uint32_t>(
                    OBJECTS_OperateFunction15_PortalExpected.size()),
                HookOperatePortal,
                &OriginalOperatePortal)) {
        Context->LogError(
            "ExtendedActLevelIds: a local Town Portal native hook is already owned or unavailable.");
        (void)Network->unregisterChannel(Context, NetworkChannel);
        NetworkChannel = D2RL::Network::InvalidChannelHandle;
        ReleaseSingleton();
        return false;
    }

    if (!Context->InstallInlineHook(
            D2GAME_SendPacket0x51_ObjectSpawnRva,
            D2GAME_SendPacket0x51_ObjectSpawnExpected.data(),
            static_cast<std::uint32_t>(
                D2GAME_SendPacket0x51_ObjectSpawnExpected.size()),
            HookSendObjectSpawnPacket,
            &OriginalSendObjectSpawnPacket)
            || !Context->InstallInlineHook(
                CLIENT_HandlePacket0x51_ObjectSpawnRva,
                CLIENT_HandlePacket0x51_ObjectSpawnExpected.data(),
                static_cast<std::uint32_t>(
                    CLIENT_HandlePacket0x51_ObjectSpawnExpected.size()),
                HookHandleObjectSpawnPacket,
                &OriginalHandleObjectSpawnPacket)
            || !Context->InstallInlineHook(
                CLIENT_HandlePacket0x60_PortalStateRva,
                CLIENT_HandlePacket0x60_PortalStateExpected.data(),
                static_cast<std::uint32_t>(
                    CLIENT_HandlePacket0x60_PortalStateExpected.size()),
                HookHandlePortalStatePacket,
                &OriginalHandlePortalStatePacket)) {
        Context->LogError(
            "ExtendedActLevelIds: a local Town Portal packet hook is already owned or unavailable.");
        (void)Network->unregisterChannel(Context, NetworkChannel);
        NetworkChannel = D2RL::Network::InvalidChannelHandle;
        ReleaseSingleton();
        return false;
    }

    Operational.store(true, std::memory_order_release);
    const auto message = std::string(
        "Extended Act Level IDs 2.1.2 by RuffnecKk active; native fingerprints, coordinate-free local Town Portal sidecars, and private compatibility channel accepted; build=")
        + RuntimeBuildName + ".";
    Context->LogInfo(message.c_str());
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    using namespace RuffnecKk::ExtendedActLevelIds;
    Operational.store(false, std::memory_order_release);
    if (Network
            && NetworkChannel != D2RL::Network::InvalidChannelHandle) {
        (void)Network->unregisterChannel(Context, NetworkChannel);
    }
    ResetState();
    ReleaseSingleton();
}
