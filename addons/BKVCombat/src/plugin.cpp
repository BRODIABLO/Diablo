#define NOMINMAX

#include "bkv_combat_config.hpp"
#include "bkv_combat_policy.hpp"
#include "bkv_combat_version.hpp"

#include <D2RLPlugin/api.h>
#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace RuffnecKk::BKVCombat {
namespace {

constexpr std::uint32_t SupportedBuild = 92777;
constexpr std::int32_t CriticalChanceCap = 75;
constexpr std::int32_t PassiveCriticalStatId = 337;
constexpr std::int32_t DeadlyStrikeStatId = 141;
constexpr std::uint16_t CriticalDeadlyResultFlag = 0x2000;
constexpr std::size_t DamageResultFlagsOffset = 0x04;
constexpr std::size_t DamagePhysicalOffset = 0x18;
constexpr std::size_t UnitClassIdOffset = 0x0C;
constexpr std::size_t CrushingBlowDamageContextOffset = 0x00;
constexpr std::size_t CrushingBlowRangedContextOffset = 0x14;
constexpr std::int32_t MonsterUnitType = 1;
constexpr std::int32_t HitpointsStatId = 6;
constexpr std::int32_t PhysicalResistanceStatId = 36;
constexpr std::int32_t CrushingBlowOverlayId = 147;
constexpr std::uint16_t LethalResultFlag = 0x0002;
constexpr std::uint16_t SuperUniqueMonsterFlag = 0x0002;
constexpr std::uint16_t ChampionMonsterFlag = 0x0004;
constexpr std::uint16_t UniqueMonsterFlag = 0x0008;
constexpr std::uint16_t HeraldMonsterFlag = 0x0040;
constexpr std::int32_t OpenWoundsStateId = 62;
constexpr std::int32_t OpenWoundsRegenStatId = 74;
constexpr std::int32_t CharacterLevelStatId = 12;
constexpr std::int32_t OpenWoundsDurationFrames = 125;
constexpr std::int32_t StatListExpireEventType = 12;
constexpr std::size_t GameFrameOffset = 0x170;
constexpr std::size_t StatListExHeadAOffset = 0x90;
constexpr std::size_t StatListExHeadBOffset = 0x98;
constexpr std::size_t StatListTargetOffset = 0x00;
constexpr std::size_t StatListSourceTypeOffset = 0x08;
constexpr std::size_t StatListSourceGuidOffset = 0x0C;
constexpr std::size_t StatListFlagsOffset = 0x1C;
constexpr std::size_t StatListStateOffset = 0x20;
constexpr std::size_t StatListExpiryOffset = 0x24;
constexpr std::size_t StatListNextOffset = 0x68;
constexpr std::size_t StatListRemoveCallbackOffset = 0x80;
constexpr std::uint32_t OpenWoundsAllowedStatListFlags = 0x080A;
constexpr std::uint32_t OpenWoundsRequiredStatListFlag = 0x0002;
constexpr std::size_t MaximumScannedStatLists = 512;
constexpr std::size_t MaximumOpenWoundsStacks = 3;

constexpr std::uintptr_t WeaponMasteryChanceCallRva = 0x44C2D3;
constexpr std::uintptr_t PassiveCriticalChanceCallRva = 0x44C32B;
constexpr std::uintptr_t DeadlyStrikeChanceCallRva = 0x44C37F;
constexpr std::uintptr_t CriticalDeadlyTailContextRva = 0x44C3C2;
constexpr std::uintptr_t CriticalDeadlyTailRva = 0x44C3C8;
constexpr std::uintptr_t CriticalDeadlyTailContinuationRva = 0x44C3D9;

constexpr std::uintptr_t ResolveActiveWeaponRva = 0x4242B0;
constexpr std::uintptr_t GetWeaponMasteryChanceRva = 0x33D4F0;
constexpr std::uintptr_t GetUnitStatRva = 0x2F5020;
constexpr std::uintptr_t GetLayeredStatRva = 0x2F5C60;
constexpr std::uintptr_t GetSeedRva = 0x34A1E0;
constexpr std::uintptr_t CrushingBlowCoreCallRva = 0x583257;
constexpr std::uintptr_t CrushingBlowCoreContextRva = 0x583247;
constexpr std::uintptr_t CrushingBlowCoreRva = 0x581BD0;
constexpr std::uintptr_t GetUnitTypeRva = 0x34B9D0;
constexpr std::uintptr_t GetHirelingTypeIdRva = 0x3AF240;
constexpr std::uintptr_t GetMinionOwnerRva = 0x4A53C0;
constexpr std::uintptr_t CheckMonsterTypeFlagRva = 0x38E870;
constexpr std::uintptr_t GetUnitRoomRva = 0x34B440;
constexpr std::uintptr_t GetPlayerCountBonusRva = 0x542F40;
constexpr std::uintptr_t SetUnitStatRva = 0x2F7D10;
constexpr std::uintptr_t SetOverlayRva = 0x349020;
constexpr std::uintptr_t OpenWoundsApplyCallRva = 0x5842F3;
constexpr std::uintptr_t ApplyStateStatListRva = 0x433D20;
constexpr std::uintptr_t GetUnitGameRva = 0x48FF00;
constexpr std::uintptr_t GetUnitGuidRva = 0x34A330;
constexpr std::uintptr_t GetUnitDataContextRva = 0x34A0E0;
constexpr std::uintptr_t GetStatListExRva = 0x34B870;
constexpr std::uintptr_t LookupStateStatListRva = 0x2F5940;
constexpr std::uintptr_t AllocateStatListRva = 0x2F7300;
constexpr std::uintptr_t SetStatListStateRva = 0x2F72F0;
constexpr std::uintptr_t SetStatListSkillIdRva = 0x2F79B0;
constexpr std::uintptr_t SetStatListSkillLevelRva = 0x2F7A00;
constexpr std::uintptr_t SetStatListStatRva = 0x2F7C00;
constexpr std::uintptr_t SetStatListRemoveCallbackRva = 0x2F7A50;
constexpr std::uintptr_t SetStatListExpiryRva = 0x2F7AB0;
constexpr std::uintptr_t PostStatListRva = 0x2F2CB0;
constexpr std::uintptr_t SetEventRva = 0x48B720;
constexpr std::uintptr_t DefaultStateRemoveCallbackRva = 0x436240;
constexpr std::uintptr_t UnlinkStatListRva = 0x2F7920;
constexpr std::uintptr_t FreeStatListRva = 0x2F4180;
constexpr std::uintptr_t LeechCallContextRva = 0x44D02C;
constexpr std::uintptr_t ApplyLifeAndManaLeechRva = 0x450C90;
constexpr std::uintptr_t LifeTapHandlerRva = 0x55BCD0;

constexpr auto WeaponMasteryChanceCallExpected =
    std::to_array<std::uint8_t>({0xE8,0x18,0x12,0xEF,0xFF});
constexpr auto PassiveCriticalChanceCallExpected =
    std::to_array<std::uint8_t>({0xE8,0xF0,0x8C,0xEA,0xFF});
constexpr auto DeadlyStrikeChanceCallExpected =
    std::to_array<std::uint8_t>({0xE8,0xDC,0x98,0xEA,0xFF});
constexpr auto CriticalDeadlyTailExpected = std::to_array<std::uint8_t>({
    0x8B,0x47,0x18,0x03,0xC0,0x89,0x47,0x18,0xB8,0x00,0x20,0x00,0x00,
    0x66,0x09,0x47,0x04,
});
constexpr auto CrushingBlowCoreCallExpected =
    std::to_array<std::uint8_t>({0xE8,0x74,0xE9,0xFF,0xFF});
constexpr auto OpenWoundsApplyCallExpected =
    std::to_array<std::uint8_t>({0xE8,0x28,0xFA,0xEA,0xFF});
constexpr auto CrushingBlowCoreContextExpected = std::to_array<std::uint8_t>({
    0x49,0x8B,0xCE,0xC7,0x44,0x24,0x50,0x0A,0x00,0x00,0x00,0x0F,0x94,
    0x44,0x24,0x54,0xE8,0x74,0xE9,0xFF,0xFF,0xEB,0x02,0x33,0xC0,
});

constexpr std::uintptr_t WeaponMasteryContextRva = 0x44C2C6;
constexpr auto WeaponMasteryContextExpected = std::to_array<std::uint8_t>({
    0x45,0x8D,0x4E,0x02,0x45,0x33,0xC0,0x48,0x8B,0xD0,0x48,0x8B,0xCE,
    0xE8,0x18,0x12,0xEF,0xFF,0x8B,0xD8,0x85,0xC0,0x7E,0x42,
});
constexpr std::uintptr_t PassiveCriticalContextRva = 0x44C320;
constexpr auto PassiveCriticalContextExpected = std::to_array<std::uint8_t>({
    0x45,0x33,0xC0,0xBA,0x51,0x01,0x00,0x00,0x48,0x8B,0xCE,
    0xE8,0xF0,0x8C,0xEA,0xFF,0x8B,0xD8,0x85,0xC0,0x7E,0x3E,
});
constexpr std::uintptr_t DeadlyStrikeContextRva = 0x44C374;
constexpr auto DeadlyStrikeContextExpected = std::to_array<std::uint8_t>({
    0x45,0x33,0xC0,0xBA,0x8D,0x00,0x00,0x00,0x48,0x8B,0xCE,
    0xE8,0xDC,0x98,0xEA,0xFF,0x8B,0xD8,0x85,0xC0,0x7E,0x4F,
});
constexpr auto CriticalDeadlyTailContextExpected =
    std::to_array<std::uint8_t>({
        0x2B,0xC1,0x3B,0xC3,0x7D,0x11,
        0x8B,0x47,0x18,0x03,0xC0,0x89,0x47,0x18,0xB8,0x00,0x20,0x00,0x00,
        0x66,0x09,0x47,0x04,0x8B,0x6F,0x20,
    });

constexpr auto ResolveActiveWeaponExpected = std::to_array<std::uint8_t>({
    0xE9,0x4B,0x6D,0xF2,0xFF,
});
constexpr auto GetWeaponMasteryChanceExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x55,0x56,0x57,0x41,0x56,0x41,0x57,0x48,0x81,0xEC,0x48,0x02,
    0x00,0x00,0x48,0x8B,0x05,0xC1,0xDD,0x68,0x02,0x48,0x33,0xC4,0x48,0x89,
    0x84,0x24,0x30,0x02,
});
constexpr auto GetUnitStatExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x6C,0x24,0x18,0x48,0x89,0x74,0x24,
    0x20,0x57,0x48,0x83,0xEC,0x20,0x41,0x0F,0xB7,0xE8,0x8B,0xFA,0x48,0x8B,
    0xD9,0x48,0x85,0xC9,
});
constexpr auto GetLayeredStatExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x6C,0x24,0x18,0x48,0x89,0x74,0x24,
    0x20,0x57,0x48,0x83,0xEC,0x20,0x41,0x0F,0xB7,0xE8,0x8B,0xDA,0x48,0x8B,
    0xF9,0x48,0x85,0xC9,
});
constexpr auto GetSeedExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0x48,0x85,0xC9,0x75,0x1D,
    0x88,0x4C,0x24,0x30,0x48,0x8D,0x4C,0x24,0x30,0xE8,0x94,0xBB,0xFF,0xFF,
    0x84,0xC0,0x74,0x01,
});
constexpr auto CrushingBlowCoreExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x54,0x24,0x10,0x48,0x89,0x4C,0x24,0x08,0x53,0x55,0x56,0x57,
    0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x48,0x41,0x8B,0x59,0x08,0x49,0x8B,
    0xC8,0x4D,0x8B,0xF9,
});
constexpr auto GetUnitTypeExpected = std::to_array<std::uint8_t>({
    0x48,0x83,0xEC,0x28,0x48,0x85,0xC9,0x75,0x1D,0x88,0x4C,0x24,0x30,0x48,
    0x8D,0x4C,0x24,0x30,0xE8,0x39,0x9E,0xFF,0xFF,0x84,0xC0,0x74,0x01,0xCC,
});
constexpr auto GetHirelingTypeIdExpected = std::to_array<std::uint8_t>({
    0x40,0x56,0x48,0x83,0xEC,0x20,0x48,0x8B,0xF1,0xE8,0x82,0xC7,0xF9,0xFF,
    0x83,0xF8,0x01,0x75,0x5D,0x48,0x89,0x5C,0x24,0x30,
});
constexpr auto GetMinionOwnerExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0xE8,0x02,0x66,0xEA,0xFF,
    0x83,0xF8,0x01,0x0F,0x85,0x9E,0x00,0x00,0x00,0x48,0x85,0xDB,0x74,0x0D,
    0x48,0x8B,0xCB,
});
constexpr auto CheckMonsterTypeFlagExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x57,0x48,0x83,0xEC,0x20,0x0F,0xB7,0xFA,0x48,
    0x8B,0xD9,0x48,0x85,0xC9,0x74,0x0A,0xE8,0x46,0xD1,0xFB,0xFF,0x83,0xF8,
    0x01,0x74,0x19,
});
constexpr auto GetUnitRoomExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0x48,0x85,0xC9,0x75,0x13,
    0x88,0x4C,0x24,0x30,0x48,0x8D,0x4C,0x24,0x30,0xE8,0x54,0xA7,0xFF,0xFF,
});
constexpr auto SetUnitStatExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x6C,0x24,0x18,0x56,0x57,0x41,0x54,
    0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x40,0x45,0x0F,0xB7,0xE1,0x45,0x8B,
    0xF0,0x8B,0xF2,0x48,
});
constexpr auto SetOverlayExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x18,0x55,0x57,0x41,0x56,0x48,0x83,0xEC,0x30,0x45,
    0x8B,0xF0,0x8B,0xFA,0x48,0x8B,0xD9,0x48,0x85,0xC9,
});
constexpr auto ApplyStateStatListExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x20,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,
    0x41,0x57,0x48,0x83,0xEC,0x50,0x48,0x8B,0x69,0x08,
});
constexpr auto GetUnitGameExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0x48,0x85,0xC9,0x75,0x20,
    0x88,0x4C,0x24,0x30,0x48,0x8D,0x4C,0x24,0x30,0xE8,0x44,0xD9,0xFF,0xFF,
    0x84,0xC0,0x74,0x01,
});
constexpr auto GetUnitGuidExpected = std::to_array<std::uint8_t>({
    0x48,0x83,0xEC,0x28,0x48,0x85,0xC9,0x75,0x1D,0x88,0x4C,0x24,0x30,0x48,
    0x8D,0x4C,0x24,0x30,0xE8,0x39,0xCA,0xFF,0xFF,0x84,
});
constexpr auto GetUnitDataContextExpected = std::to_array<std::uint8_t>({
    0x48,0x83,0xEC,0x28,0x48,0x85,0xC9,0x75,0x1A,0x88,0x4C,0x24,0x30,0x48,
    0x8D,0x4C,0x24,0x30,0xE8,0x49,0xC7,0xFF,0xFF,0x84,
});
constexpr auto GetStatListExExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0x48,0x85,0xC9,0x75,0x20,
    0x88,0x4C,0x24,0x30,0x48,0x8D,0x4C,0x24,0x30,0xE8,0x44,0x9B,0xFF,0xFF,
    0x84,0xC0,0x74,0x01,
});
constexpr auto LookupStateStatListExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x57,0x48,0x83,0xEC,0x20,0x8B,0xDA,0x48,0x8B,
    0xF9,0x48,0x85,0xC9,0x75,0x13,0x88,0x4C,0x24,0x30,0x48,0x8D,0x4C,0x24,
    0x30,0xE8,0x9E,0xB9,
});
constexpr auto AllocateStatListExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x7C,0x24,
    0x20,0x89,0x4C,0x24,0x08,0x55,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,
    0x48,0x8D,0x6C,0x24,
});
constexpr auto SetStatListStateExpected = std::to_array<std::uint8_t>({
    0x48,0x85,0xC9,0x74,0x03,0x89,0x51,0x20,0xC3,
});
constexpr auto SetStatListSkillIdExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,0xEC,0x20,0x8B,0xFA,0x48,0x8B,
    0xD9,0x48,0x85,0xC9,0x75,0x21,0x88,0x4C,0x24,0x38,0x48,0x8D,0x4C,0x24,
    0x38,0xE8,0x0E,0xA5,0xFF,0xFF,0x84,0xC0,
});
constexpr auto SetStatListSkillLevelExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,0xEC,0x20,0x8B,0xFA,0x48,0x8B,
    0xD9,0x48,0x85,0xC9,0x75,0x21,0x88,0x4C,0x24,0x38,0x48,0x8D,0x4C,0x24,
    0x38,0xE8,0x8E,0xA1,0xFF,0xFF,0x84,0xC0,
});
constexpr auto SetStatListStatExpected = std::to_array<std::uint8_t>({
    0x48,0x85,0xD2,0x74,0x0F,0x0F,0xB7,0x44,0x24,0x28,0x66,0x89,0x44,0x24,
    0x28,0xE9,0x1C,0xB4,0xFF,0xFF,0xC3,
});
constexpr auto SetStatListRemoveCallbackExpected = std::to_array<std::uint8_t>({
    0x48,0x85,0xC9,0x74,0x07,0x48,0x89,0x91,0x80,0x00,0x00,0x00,0xC3,
});
constexpr auto SetStatListExpiryExpected = std::to_array<std::uint8_t>({
    0x48,0x85,0xC9,0x74,0x14,0x85,0xD2,0x7E,0x04,0x83,0x49,0x1C,0x02,0x66,
    0x0F,0x6E,0xC2,0x0F,0x5B,0xC0,0xF3,0x0F,0x11,0x41,0x24,0xC3,
});
constexpr auto PostStatListExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x55,0x56,0x48,0x81,0xEC,0xD0,0x00,0x00,0x00,0x48,0x8B,0x05,
    0x06,0x86,0x6D,0x02,0x48,0x33,0xC4,0x48,0x89,0x84,0x24,0xA0,0x00,0x00,
    0x00,0x44,0x89,0x44,
});
constexpr auto SetEventExpected = std::to_array<std::uint8_t>({
    0x48,0x83,0xEC,0x48,0x8B,0x84,0x24,0x80,0x00,0x00,0x00,0x89,0x44,0x24,
    0x38,0x8B,0x44,0x24,0x78,0x89,0x44,0x24,0x30,0x8B,0x44,0x24,0x70,0x89,
    0x44,0x24,0x28,0x48,
});
constexpr auto DefaultStateRemoveCallbackExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,0xEC,0x20,0x48,0x8B,0xCA,0x41,
    0x8B,0xF8,0x48,0x8B,0xDA,0xE8,0xA8,0x9C,0x05,0x00,0x48,0x8B,0xC8,0x44,
    0x8B,0xCF,0x41,0xB8,
});
constexpr auto UnlinkStatListExpected = std::to_array<std::uint8_t>({
    0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xDA,0xE8,0xB2,0x27,0x05,0x00,
    0x0F,0xB6,0xC8,0x48,0x8B,0xD3,0x48,0x83,0xC4,0x20,0x5B,0xE9,
});
constexpr auto FreeStatListExpected = std::to_array<std::uint8_t>({
    0x48,0x85,0xD2,0x74,0x0A,0x83,0x7A,0x1C,0x00,0x0F,0x8D,0x21,0x5D,0x00,
    0x00,0xC3,
});
constexpr auto LeechCallContextExpected = std::to_array<std::uint8_t>({
    0x4C,0x8B,0xCF,0x4C,0x8B,0xC6,0x49,0x8B,0xD6,0x49,0x8B,0xCF,
    0xE8,0x53,0x3C,0x00,0x00,0x41,0x83,0xFD,0x01,0x75,0x11,0x44,
});
constexpr auto ApplyLifeAndManaLeechExpected = std::to_array<std::uint8_t>({
    0x40,0x55,0x56,0x57,0x41,0x56,0x48,0x83,0xEC,0x58,0x41,0x83,0xB9,0x20,
    0x01,0x00,0x00,0x00,0x49,0x8B,0xF1,0x49,0x8B,0xF8,0x4C,0x8B,0xF2,0x48,
    0x8B,0xE9,0x75,0x0E,
});
constexpr auto LifeTapHandlerExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,0x24,0x10,0x48,0x89,0x74,0x24,
    0x20,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x30,0x49,0x8B,0xF9,0x49,
    0x8B,0xD8,0x4C,0x8B,
});

constexpr std::size_t RelayStride = 16;
constexpr std::size_t MasteryRelayOffset = 0;
constexpr std::size_t PassiveRelayOffset = RelayStride;
constexpr std::size_t DeadlyRelayOffset = RelayStride * 2;
constexpr std::size_t CrushingBlowRelayOffset = RelayStride * 3;
constexpr std::size_t TailRelayOffset = RelayStride * 4;
constexpr std::size_t OpenWoundsRelayOffset = RelayStride * 7;
constexpr std::size_t RelayBytes = 128;

using GetWeaponMasteryChanceFn = std::int32_t(__fastcall*)(
    void*, void*, std::int32_t, std::int32_t) noexcept;
using GetStatFn = std::int32_t(__fastcall*)(
    void*, std::int32_t, std::int32_t) noexcept;
using ResolveActiveWeaponFn = void*(__fastcall*)(void*) noexcept;

struct NativeSeedPair {
    std::uint32_t low;
    std::uint32_t high;
};
using GetSeedFn = NativeSeedPair*(__fastcall*)(void*) noexcept;
using CrushingBlowCoreFn = std::int32_t(__fastcall*)(
    void*, void*, void*, void*) noexcept;
using GetUnitTypeFn = std::int32_t(__fastcall*)(void*) noexcept;
using GetHirelingTypeIdFn = std::int32_t(__fastcall*)(void*) noexcept;
using GetMinionOwnerFn = void*(__fastcall*)(void*) noexcept;
using CheckMonsterTypeFlagFn = bool(__fastcall*)(
    void*, std::uint16_t) noexcept;
using GetUnitRoomFn = void*(__fastcall*)(void*) noexcept;
using GetPlayerCountBonusFn = void(__fastcall*)(
    void*, std::int32_t*, void*, void*) noexcept;
using SetUnitStatFn = void(__fastcall*)(
    void*, std::int32_t, std::int32_t, std::int32_t) noexcept;
using SetOverlayFn = void(__fastcall*)(
    void*, std::int32_t, std::int32_t) noexcept;

using OpenWoundsRemoveCallbackFn = void(__fastcall*)(
    std::uint8_t, void*, std::int32_t, void*) noexcept;

struct OpenWoundsApplyArgs {
    void* source;
    void* target;
    std::int32_t skillId;
    std::int32_t skillLevel;
    std::int32_t duration;
    std::int32_t statId;
    std::int32_t statValue;
    std::int32_t state;
    OpenWoundsRemoveCallbackFn removeCallback;
};

static_assert(sizeof(OpenWoundsApplyArgs) == 0x30);
static_assert(offsetof(OpenWoundsApplyArgs, source) == 0x00);
static_assert(offsetof(OpenWoundsApplyArgs, target) == 0x08);
static_assert(offsetof(OpenWoundsApplyArgs, skillId) == 0x10);
static_assert(offsetof(OpenWoundsApplyArgs, skillLevel) == 0x14);
static_assert(offsetof(OpenWoundsApplyArgs, duration) == 0x18);
static_assert(offsetof(OpenWoundsApplyArgs, statId) == 0x1C);
static_assert(offsetof(OpenWoundsApplyArgs, statValue) == 0x20);
static_assert(offsetof(OpenWoundsApplyArgs, state) == 0x24);
static_assert(offsetof(OpenWoundsApplyArgs, removeCallback) == 0x28);

using ApplyStateStatListFn = void*(__fastcall*)(
    OpenWoundsApplyArgs*) noexcept;
using GetUnitGameFn = void*(__fastcall*)(void*) noexcept;
using GetUnitGuidFn = std::uint32_t(__fastcall*)(void*) noexcept;
using GetUnitDataContextFn = std::uint8_t(__fastcall*)(void*) noexcept;
using GetStatListExFn = void*(__fastcall*)(void*) noexcept;
using LookupStateStatListFn = void*(__fastcall*)(
    void*, std::int32_t) noexcept;
using AllocateStatListFn = void*(__fastcall*)(
    std::uint32_t, std::int32_t, std::int32_t, std::uint32_t) noexcept;
using SetStatListIntFn = void(__fastcall*)(void*, std::int32_t) noexcept;
using SetStatListStatFn = void(__fastcall*)(
    std::uint8_t, void*, std::int32_t, std::int32_t, std::uint16_t) noexcept;
using SetStatListRemoveCallbackFn = void(__fastcall*)(
    void*, OpenWoundsRemoveCallbackFn) noexcept;
using PostStatListFn = void(__fastcall*)(
    void*, void*, std::int32_t) noexcept;
using SetEventFn = void(__fastcall*)(
    void*, void*, std::int32_t, std::int32_t, std::int32_t,
    std::int32_t, std::int32_t) noexcept;
using UnlinkStatListFn = void(__fastcall*)(void*, void*) noexcept;
using FreeStatListFn = void(__fastcall*)(std::uint8_t, void*) noexcept;

struct MonsterCatalogEntry {
    std::string monstats;
    std::int32_t monstatsId{};
    bool primeEvil{};
    bool boss{};
};

enum class RollSource : std::uint8_t {
    None,
    Critical,
    Deadly,
};

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config ActiveConfig{};
std::filesystem::path ActiveConfigPath;
void* RelayPage{};
std::atomic_bool Operational{};
std::atomic_bool AnyMutationInstalled{};

GetWeaponMasteryChanceFn GetWeaponMasteryChance{};
GetStatFn GetUnitStat{};
GetStatFn GetLayeredStat{};
ResolveActiveWeaponFn ResolveActiveWeapon{};
GetSeedFn GetSeed{};
CrushingBlowCoreFn OriginalCrushingBlowCore{};
GetUnitTypeFn GetUnitType{};
GetHirelingTypeIdFn GetHirelingTypeId{};
GetMinionOwnerFn GetMinionOwner{};
CheckMonsterTypeFlagFn CheckMonsterTypeFlag{};
GetUnitRoomFn GetUnitRoom{};
GetPlayerCountBonusFn GetPlayerCountBonus{};
SetUnitStatFn SetUnitStat{};
SetOverlayFn SetOverlay{};
ApplyStateStatListFn OriginalApplyStateStatList{};
GetUnitGameFn GetUnitGame{};
GetUnitGuidFn GetUnitGuid{};
GetUnitDataContextFn GetUnitDataContext{};
GetStatListExFn GetStatListEx{};
LookupStateStatListFn LookupStateStatList{};
AllocateStatListFn AllocateStatList{};
SetStatListIntFn SetStatListState{};
SetStatListIntFn SetStatListSkillId{};
SetStatListIntFn SetStatListSkillLevel{};
SetStatListStatFn SetStatListStat{};
SetStatListRemoveCallbackFn SetStatListRemoveCallback{};
SetStatListIntFn SetStatListExpiry{};
PostStatListFn PostStatList{};
SetEventFn SetEvent{};
OpenWoundsRemoveCallbackFn DefaultStateRemoveCallback{};
UnlinkStatListFn UnlinkStatList{};
FreeStatListFn FreeStatList{};
std::unordered_map<std::int32_t, MonsterCatalogEntry> MonsterCatalog;
MajorBossRegistry ActiveMajorBossRegistry;

thread_local RollSource CurrentRollSource = RollSource::None;

template <typename Function>
Function At(const std::uintptr_t rva) noexcept {
    return reinterpret_cast<Function>(Base + rva);
}

template <typename Value>
Value Read(const void* object, const std::size_t offset) noexcept {
    static_assert(std::is_trivially_copyable_v<Value>);
    Value value{};
    if (object) {
        std::memcpy(
            &value,
            static_cast<const std::uint8_t*>(object) + offset,
            sizeof(value));
    }
    return value;
}

template <typename Value>
void Write(void* object, const std::size_t offset, const Value value) noexcept {
    static_assert(std::is_trivially_copyable_v<Value>);
    if (!object) return;
    std::memcpy(
        static_cast<std::uint8_t*>(object) + offset,
        &value,
        sizeof(value));
}

std::int32_t CappedChance(
        const std::int32_t value, const bool policyEnabled) noexcept {
    return policyEnabled && value > CriticalChanceCap
        ? CriticalChanceCap
        : value;
}

std::int32_t DoublePhysical(const std::int32_t physical) noexcept {
    const auto doubled = static_cast<std::uint32_t>(physical) * 2U;
    return static_cast<std::int32_t>(doubled);
}

std::int32_t DeadlyPhysical(const std::int32_t physical) noexcept {
    if (physical <= 0) return physical;
    const auto value = static_cast<std::uint32_t>(physical);
    return static_cast<std::int32_t>(value + value / 2U);
}

void ApplyResolvedMultiplier(void* damage, const RollSource source) noexcept {
    if (!damage) return;
    const auto physical = Read<std::int32_t>(damage, DamagePhysicalOffset);
    const auto deadlyPolicy =
        Operational.load(std::memory_order_acquire)
        && ActiveConfig.policies.deadlyStrike
        && source == RollSource::Deadly;
    Write(
        damage,
        DamagePhysicalOffset,
        deadlyPolicy ? DeadlyPhysical(physical) : DoublePhysical(physical));
    Write(
        damage,
        DamageResultFlagsOffset,
        static_cast<std::uint16_t>(
            Read<std::uint16_t>(damage, DamageResultFlagsOffset)
            | CriticalDeadlyResultFlag));
}

void __fastcall ApplyPrimaryCriticalDeadlyTail(void* damage) noexcept {
    const auto source = CurrentRollSource;
    CurrentRollSource = RollSource::None;
    ApplyResolvedMultiplier(damage, source);
}

std::int32_t __fastcall HookWeaponMasteryChance(
        void* attacker,
        void* weapon,
        const std::int32_t argument3,
        const std::int32_t argument4) noexcept {
    const auto value = GetWeaponMasteryChance(
        attacker, weapon, argument3, argument4);
    CurrentRollSource = RollSource::Critical;
    return CappedChance(
        value,
        Operational.load(std::memory_order_acquire)
            && ActiveConfig.policies.criticalStrike);
}

std::int32_t __fastcall HookPassiveCriticalChance(
        void* attacker,
        const std::int32_t statId,
        const std::int32_t layer) noexcept {
    const auto value = GetUnitStat(attacker, statId, layer);
    CurrentRollSource = statId == PassiveCriticalStatId
        ? RollSource::Critical
        : RollSource::None;
    return CappedChance(
        value,
        Operational.load(std::memory_order_acquire)
            && ActiveConfig.policies.criticalStrike
            && statId == PassiveCriticalStatId);
}

std::int32_t __fastcall HookDeadlyStrikeChance(
        void* attacker,
        const std::int32_t statId,
        const std::int32_t layer) noexcept {
    const auto value = GetLayeredStat(attacker, statId, layer);
    CurrentRollSource = statId == DeadlyStrikeStatId
        ? RollSource::Deadly
        : RollSource::None;
    return CappedChance(
        value,
        Operational.load(std::memory_order_acquire)
            && ActiveConfig.policies.deadlyStrike
            && statId == DeadlyStrikeStatId);
}

bool RollChance(void* attacker, const std::int32_t chance) noexcept {
    if (chance <= 0 || !GetSeed) return false;
    auto* seed = GetSeed(attacker);
    if (!seed) return false;
    const auto next =
        static_cast<std::uint64_t>(seed->low) * UINT64_C(0x6AC690C5)
        + static_cast<std::uint64_t>(seed->high);
    seed->low = static_cast<std::uint32_t>(next);
    seed->high = static_cast<std::uint32_t>(next >> 32U);
    return static_cast<std::int32_t>(seed->low % 100U) < chance;
}

std::vector<std::string> SplitTabs(const std::string& line) {
    std::vector<std::string> columns;
    std::size_t start{};
    while (start <= line.size()) {
        const auto end = line.find('\t', start);
        columns.emplace_back(line.substr(start, end - start));
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return columns;
}

bool ParseInt32(
        const std::string_view text, std::int32_t& value) noexcept {
    if (text.empty()) return false;
    const auto* first = text.data();
    const auto* last = first + text.size();
    const auto result = std::from_chars(first, last, value);
    return result.ec == std::errc{} && result.ptr == last;
}

bool ParseTxtFlag(
        const std::string_view text, bool& value) noexcept {
    if (text.empty() || text == "0") {
        value = false;
        return true;
    }
    if (text == "1") {
        value = true;
        return true;
    }
    return false;
}

std::optional<std::filesystem::path> LocateActiveExcelFile(
        const std::wstring_view fileName) {
    if (!Context || !Context->modDirectory
            || Context->modDirectory[0] == L'\0') {
        return std::nullopt;
    }
    const std::filesystem::path root(Context->modDirectory);
    std::vector<std::filesystem::path> candidates{
        root / L"data" / L"global" / L"excel"
            / std::filesystem::path(fileName),
    };
    if (Context->activeMod && Context->activeMod[0] != '\0') {
        candidates.push_back(
            root
            / (std::filesystem::path(Context->activeMod).wstring() + L".mpq")
            / L"data" / L"global" / L"excel"
            / std::filesystem::path(fileName));
    }
    std::error_code error;
    for (const auto& candidate : candidates) {
        if (std::filesystem::is_regular_file(candidate, error)) {
            return candidate;
        }
        error.clear();
    }
    return std::nullopt;
}

bool LoadCrushingBlowCatalog() noexcept {
    try {
        const auto path = LocateActiveExcelFile(L"monstats.txt");
        if (!path) {
            Context->LogError(
                "BKVCombat: Crushing Blow requires the active monstats.txt.");
            return false;
        }
        std::ifstream stream(*path, std::ios::binary);
        if (!stream) {
            Context->LogError(
                "BKVCombat: active monstats.txt could not be opened.");
            return false;
        }

        std::string line;
        if (!std::getline(stream, line)) {
            Context->LogError("BKVCombat: active monstats.txt is empty.");
            return false;
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto headers = SplitTabs(line);
        const auto findHeader = [&](const std::string_view name)
                -> std::optional<std::size_t> {
            const auto it = std::find(headers.begin(), headers.end(), name);
            if (it == headers.end()) return std::nullopt;
            return static_cast<std::size_t>(it - headers.begin());
        };
        const auto idColumn = findHeader("Id");
        const auto hcIdxColumn = findHeader("*hcIdx");
        const auto primeEvilColumn = findHeader("primeevil");
        const auto bossColumn = findHeader("boss");
        if (!idColumn || !hcIdxColumn || !primeEvilColumn || !bossColumn) {
            Context->LogError(
                "BKVCombat: monstats.txt requires Id, *hcIdx, primeevil, and "
                "boss columns.");
            return false;
        }
        const auto lastRequiredColumn = std::max({
            *idColumn, *hcIdxColumn, *primeEvilColumn, *bossColumn,
        });

        std::unordered_map<std::int32_t, MonsterCatalogEntry> catalog;
        std::unordered_set<std::string> monstatsKeys;
        std::vector<ResolvedMonStatsIdentity> identities;
        std::int32_t runtimeClassId{};
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const auto columns = SplitTabs(line);
            if (columns.size() <= lastRequiredColumn
                    || columns[*idColumn].empty()) {
                continue;
            }
            // Expansion is a TXT compiler marker, not a runtime MonStats row.
            // Runtime class IDs are the remaining row order; *hcIdx is
            // descriptive and may legitimately be duplicated by appended rows.
            if (columns[*idColumn] == "Expansion") continue;
            std::int32_t monstatsId{};
            bool primeEvil{};
            bool boss{};
            if (!ParseInt32(columns[*hcIdxColumn], monstatsId)
                    || monstatsId < 0
                    || !ParseTxtFlag(columns[*primeEvilColumn], primeEvil)
                    || !ParseTxtFlag(columns[*bossColumn], boss)) {
                Context->LogError(
                    "BKVCombat: monstats.txt contains an invalid governed "
                    "classification value.");
                return false;
            }
            auto entry = MonsterCatalogEntry{
                .monstats = columns[*idColumn],
                .monstatsId = monstatsId,
                .primeEvil = primeEvil,
                .boss = boss,
            };
            if (!monstatsKeys.insert(entry.monstats).second
                    || !catalog.emplace(runtimeClassId, entry).second) {
                Context->LogError(
                    "BKVCombat: monstats.txt contains a duplicate runtime "
                    "classification key.");
                return false;
            }
            identities.push_back({
                .monstats = entry.monstats,
                .monstatsId = entry.monstatsId,
            });
            if (runtimeClassId == std::numeric_limits<std::int32_t>::max()) {
                Context->LogError(
                    "BKVCombat: monstats.txt contains too many runtime rows.");
                return false;
            }
            ++runtimeClassId;
        }
        if (catalog.empty()) {
            Context->LogError(
                "BKVCombat: monstats.txt produced no runtime classifications.");
            return false;
        }

        auto registry = ValidateMajorBossRegistry(ActiveConfig, identities);
        if (!registry.valid) {
            char message[1024]{};
            std::snprintf(
                message,
                sizeof(message),
                "BKVCombat: Crushing Blow registry refused: %s",
                registry.error.c_str());
            Context->LogError(message);
            return false;
        }
        MonsterCatalog = std::move(catalog);
        ActiveMajorBossRegistry = std::move(registry);
        return true;
    } catch (const std::exception& exception) {
        char message[1024]{};
        std::snprintf(
            message,
            sizeof(message),
            "BKVCombat: Crushing Blow catalog refused: %s",
            exception.what());
        Context->LogError(message);
        return false;
    } catch (...) {
        Context->LogError(
            "BKVCombat: Crushing Blow catalog refused by an unknown error.");
        return false;
    }
}

bool ValidateLeechDifficultyData() noexcept {
    try {
        const auto path = LocateActiveExcelFile(L"difficultylevels.txt");
        if (!path) {
            Context->LogError(
                "BKVCombat: Life/Mana Steal requires the active "
                "difficultylevels.txt.");
            return false;
        }
        std::ifstream stream(*path, std::ios::binary);
        if (!stream) {
            Context->LogError(
                "BKVCombat: active difficultylevels.txt could not be opened.");
            return false;
        }

        std::string line;
        if (!std::getline(stream, line)) {
            Context->LogError(
                "BKVCombat: active difficultylevels.txt is empty.");
            return false;
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto headers = SplitTabs(line);
        const auto findHeader = [&](const std::string_view name)
                -> std::optional<std::size_t> {
            const auto it = std::find(headers.begin(), headers.end(), name);
            if (it == headers.end()) return std::nullopt;
            return static_cast<std::size_t>(it - headers.begin());
        };
        const auto nameColumn = findHeader("Name");
        const auto lifeColumn = findHeader("LifeStealDivisor");
        const auto manaColumn = findHeader("ManaStealDivisor");
        if (!nameColumn
                || (ActiveConfig.policies.lifeSteal && !lifeColumn)
                || (ActiveConfig.policies.manaSteal && !manaColumn)) {
            Context->LogError(
                "BKVCombat: difficultylevels.txt lacks a selected governed "
                "leech divisor column.");
            return false;
        }

        struct ExpectedDifficulty {
            std::string_view name;
            std::int32_t divisor;
            bool seen;
        };
        std::array<ExpectedDifficulty, 3> expected{{
            {"Normal", 1, false},
            {"Nightmare", 2, false},
            {"Hell", 3, false},
        }};
        const auto lastRequiredColumn = std::max({
            *nameColumn,
            lifeColumn.value_or(0),
            manaColumn.value_or(0),
        });
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const auto columns = SplitTabs(line);
            if (columns.size() <= lastRequiredColumn) continue;
            const auto expectedIt = std::find_if(
                expected.begin(),
                expected.end(),
                [&](const ExpectedDifficulty& value) {
                    return columns[*nameColumn] == value.name;
                });
            if (expectedIt == expected.end()) continue;
            if (expectedIt->seen) {
                Context->LogError(
                    "BKVCombat: difficultylevels.txt duplicates a governed "
                    "difficulty row.");
                return false;
            }
            std::int32_t divisor{};
            if ((ActiveConfig.policies.lifeSteal
                    && (!ParseInt32(columns[*lifeColumn], divisor)
                        || divisor != expectedIt->divisor))
                    || (ActiveConfig.policies.manaSteal
                        && (!ParseInt32(columns[*manaColumn], divisor)
                            || divisor != expectedIt->divisor))) {
                Context->LogError(
                    "BKVCombat: Life/Mana Steal requires divisors 1/2/3 in "
                    "Normal/Nightmare/Hell.");
                return false;
            }
            expectedIt->seen = true;
        }
        if (std::any_of(
                expected.begin(),
                expected.end(),
                [](const ExpectedDifficulty& value) { return !value.seen; })) {
            Context->LogError(
                "BKVCombat: difficultylevels.txt is missing a governed "
                "difficulty row.");
            return false;
        }
        return true;
    } catch (const std::exception& exception) {
        char message[1024]{};
        std::snprintf(
            message,
            sizeof(message),
            "BKVCombat: Life/Mana Steal data refused: %s",
            exception.what());
        Context->LogError(message);
        return false;
    } catch (...) {
        Context->LogError(
            "BKVCombat: Life/Mana Steal data refused by an unknown error.");
        return false;
    }
}

bool ValidateCrushingBlowEfficiencyStatData() noexcept {
    const auto configuredId =
        ActiveConfig.stats.crushingBlowEfficiencyStatId;
    if (configuredId < 0) return true;
    try {
        const auto path = LocateActiveExcelFile(L"itemstatcost.txt");
        if (!path) {
            Context->LogError(
                "BKVCombat: Crushing Blow Efficiency requires the active "
                "itemstatcost.txt.");
            return false;
        }
        std::ifstream stream(*path, std::ios::binary);
        if (!stream) {
            Context->LogError(
                "BKVCombat: active itemstatcost.txt could not be opened.");
            return false;
        }

        std::string line;
        if (!std::getline(stream, line)) {
            Context->LogError("BKVCombat: active itemstatcost.txt is empty.");
            return false;
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto headers = SplitTabs(line);
        const auto findHeader = [&](const std::string_view name)
                -> std::optional<std::size_t> {
            const auto it = std::find(headers.begin(), headers.end(), name);
            if (it == headers.end()) return std::nullopt;
            return static_cast<std::size_t>(it - headers.begin());
        };
        const auto statColumn = findHeader("Stat");
        const auto idColumn = findHeader("*ID");
        const auto damageRelatedColumn = findHeader("damagerelated");
        if (!statColumn || !idColumn || !damageRelatedColumn) {
            Context->LogError(
                "BKVCombat: itemstatcost.txt lacks the governed Crushing Blow "
                "Efficiency columns.");
            return false;
        }
        const auto lastRequiredColumn = std::max({
            *statColumn, *idColumn, *damageRelatedColumn,
        });
        constexpr std::string_view expectedStat =
            "item_crushingblow_efficiency";
        std::size_t matches{};
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const auto columns = SplitTabs(line);
            if (columns.size() <= lastRequiredColumn) continue;
            std::int32_t rowId{};
            const auto idParsed = ParseInt32(columns[*idColumn], rowId);
            const auto statMatches = columns[*statColumn] == expectedStat;
            const auto idMatches = idParsed && rowId == configuredId;
            if (!statMatches && !idMatches) continue;
            if (!statMatches || !idMatches
                    || columns[*damageRelatedColumn] != "1") {
                Context->LogError(
                    "BKVCombat: configured Crushing Blow Efficiency stat ID, "
                    "name, or damagerelated flag mismatches itemstatcost.txt.");
                return false;
            }
            ++matches;
        }
        if (matches != 1) {
            Context->LogError(
                "BKVCombat: itemstatcost.txt must contain exactly one governed "
                "Crushing Blow Efficiency row.");
            return false;
        }
        return true;
    } catch (const std::exception& exception) {
        char message[1024]{};
        std::snprintf(
            message,
            sizeof(message),
            "BKVCombat: Crushing Blow Efficiency data refused: %s",
            exception.what());
        Context->LogError(message);
        return false;
    } catch (...) {
        Context->LogError(
            "BKVCombat: Crushing Blow Efficiency data refused by an unknown "
            "error.");
        return false;
    }
}

std::int32_t CallOriginalCrushingBlow(
        void* game,
        void* attacker,
        void* target,
        void* crushingContext) noexcept {
    return OriginalCrushingBlowCore
        ? OriginalCrushingBlowCore(game, attacker, target, crushingContext)
        : 0;
}

std::optional<std::int32_t> ReadCrushingBlowEfficiency(
        void* attacker) noexcept {
    const auto statId = ActiveConfig.stats.crushingBlowEfficiencyStatId;
    if (statId < 0) return 0;
    if (!attacker || !GetUnitStat || !ResolveActiveWeapon) {
        return std::nullopt;
    }
    const auto global = GetUnitStat(attacker, statId, 0);
    auto local = std::int32_t{};
    if (auto* activeWeapon = ResolveActiveWeapon(attacker)) {
        local = GetUnitStat(activeWeapon, statId, 0);
    }
    const auto combined = static_cast<std::int64_t>(global) + local;
    if (combined > std::numeric_limits<std::int32_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::int32_t>(std::max<std::int64_t>(combined, 0));
}

std::int32_t __fastcall HookCrushingBlowCore(
        void* game,
        void* attacker,
        void* target,
        void* crushingContext) noexcept {
    if (!Operational.load(std::memory_order_acquire)
            || !ActiveConfig.policies.crushingBlow
            || !game || !attacker || !target || !crushingContext
            || !ActiveMajorBossRegistry.valid
            || !GetUnitType || !GetHirelingTypeId || !CheckMonsterTypeFlag
            || !GetUnitRoom || !GetPlayerCountBonus || !GetUnitStat
            || !SetUnitStat || !SetOverlay
            || GetUnitType(target) != MonsterUnitType
            || GetHirelingTypeId(target) != 0) {
        return CallOriginalCrushingBlow(
            game, attacker, target, crushingContext);
    }

    const auto classId = Read<std::int32_t>(target, UnitClassIdOffset);
    const auto catalogIt = MonsterCatalog.find(classId);
    if (catalogIt == MonsterCatalog.end()) {
        return CallOriginalCrushingBlow(
            game, attacker, target, crushingContext);
    }
    const auto& entry = catalogIt->second;
    const RuntimeMonsterFacts facts{
        .monstats = entry.monstats,
        .monstatsId = entry.monstatsId,
        .primeEvil = entry.primeEvil,
        .heraldOrAscendant = CheckMonsterTypeFlag(
            target, HeraldMonsterFlag),
        .champion = CheckMonsterTypeFlag(target, ChampionMonsterFlag),
        .unique = CheckMonsterTypeFlag(target, UniqueMonsterFlag),
        .superUnique = CheckMonsterTypeFlag(target, SuperUniqueMonsterFlag),
        .boss = entry.boss,
    };
    const auto classification = ClassifyCrushingBlow(
        ActiveMajorBossRegistry, facts);
    if (!classification) {
        return CallOriginalCrushingBlow(
            game, attacker, target, crushingContext);
    }

    const auto ranged = Read<std::uint8_t>(
        crushingContext, CrushingBlowRangedContextOffset) != 0;
    const auto fraction = CrushingBlowFraction(*classification, ranged);
    if (fraction.numerator <= 0 || fraction.denominator <= 0) {
        return CallOriginalCrushingBlow(
            game, attacker, target, crushingContext);
    }

    std::array<std::int32_t, 5> playerCountBonus{};
    GetPlayerCountBonus(
        game, playerCountBonus.data(), GetUnitRoom(target), target);
    if (playerCountBonus[0] < 0) {
        return CallOriginalCrushingBlow(
            game, attacker, target, crushingContext);
    }
    const auto hitpoints = GetUnitStat(target, HitpointsStatId, 0);
    const auto efficiency = ReadCrushingBlowEfficiency(attacker);
    if (hitpoints < 0 || !efficiency) {
        return CallOriginalCrushingBlow(
            game, attacker, target, crushingContext);
    }
    const auto computedDamage = ComputeCrushingBlowDamage(
        hitpoints,
        fraction,
        playerCountBonus[0],
        *efficiency,
        GetUnitStat(target, PhysicalResistanceStatId, 0));
    if (!computedDamage) {
        return CallOriginalCrushingBlow(
            game, attacker, target, crushingContext);
    }
    const auto crushingDamage = *computedDamage;
    const auto newHitpoints = static_cast<std::int32_t>(std::max<
        std::int64_t>(static_cast<std::int64_t>(hitpoints) - crushingDamage, 0));
    SetUnitStat(target, HitpointsStatId, newHitpoints, 0);

    if (newHitpoints <= 0) {
        if (auto* damage = Read<void*>(
                crushingContext, CrushingBlowDamageContextOffset)) {
            Write(
                damage,
                DamageResultFlagsOffset,
                static_cast<std::uint16_t>(
                    Read<std::uint16_t>(damage, DamageResultFlagsOffset)
                    | LethalResultFlag));
        }
    }
    if (crushingDamage > 0) {
        SetOverlay(target, CrushingBlowOverlayId, 0);
    }
    return 1;
}

struct OpenWoundsScanResult {
    std::array<void*, MaximumOpenWoundsStacks> owned{};
    std::size_t ownedCount{};
    std::uint32_t flags{};
    void* managedFallback{};
    bool foreign{};
    bool invalid{};
};

void __fastcall OpenWoundsRemoveCallback(
    std::uint8_t context,
    void* target,
    std::int32_t state,
    void* removed) noexcept;

bool ContainsPointer(
        const void* const* pointers,
        const std::size_t count,
        const void* value) noexcept {
    return std::find(pointers, pointers + count, value) != pointers + count;
}

bool HasGovernedOpenWoundsFlags(const std::uint32_t flags) noexcept {
    return (flags & OpenWoundsRequiredStatListFlag) != 0
        && (flags & ~OpenWoundsAllowedStatListFlags) == 0;
}

bool IsManagedOpenWoundsStatList(
        const void* list,
        const void* expectedTarget,
        const std::int32_t currentFrame,
        const std::int32_t expiryFrame,
        const bool requireAttachedTarget) noexcept {
    if (!list
            || Read<std::int32_t>(list, StatListStateOffset)
                != OpenWoundsStateId
            || Read<std::uintptr_t>(list, StatListRemoveCallbackOffset)
                != reinterpret_cast<std::uintptr_t>(
                    &OpenWoundsRemoveCallback)
            || !HasGovernedOpenWoundsFlags(
                Read<std::uint32_t>(list, StatListFlagsOffset))) {
        return false;
    }
    if (requireAttachedTarget
            && Read<void*>(list, StatListTargetOffset) != expectedTarget) {
        return false;
    }
    const auto expiry = Read<float>(list, StatListExpiryOffset);
    return std::isfinite(expiry)
        && expiry > static_cast<float>(currentFrame)
        && expiry <= static_cast<float>(expiryFrame);
}

bool IsOwnedOpenWoundsStatList(
        const void* list,
        const void* expectedTarget,
        const std::uint32_t sourceGuid,
        const std::int32_t currentFrame,
        const std::int32_t expiryFrame,
        const bool requireAttachedTarget) noexcept {
    return IsManagedOpenWoundsStatList(
            list,
            expectedTarget,
            currentFrame,
            expiryFrame,
            requireAttachedTarget)
        && Read<std::int32_t>(list, StatListSourceTypeOffset) == 0
        && Read<std::uint32_t>(list, StatListSourceGuidOffset) == sourceGuid;
}

OpenWoundsScanResult ScanOpenWoundsStatLists(
        void* target,
        const std::uint32_t sourceGuid,
        const std::int32_t currentFrame,
        const std::int32_t expiryFrame) noexcept {
    OpenWoundsScanResult result{};
    if (!GetStatListEx || !LookupStateStatList) {
        result.invalid = true;
        return result;
    }
    auto* statListEx = GetStatListEx(target);
    if (!statListEx) {
        if (LookupStateStatList(target, OpenWoundsStateId)) {
            result.foreign = true;
        }
        return result;
    }

    std::array<void*, MaximumScannedStatLists> visited{};
    std::size_t visitedCount{};
    constexpr std::array<std::size_t, 2> heads{
        StatListExHeadAOffset,
        StatListExHeadBOffset,
    };
    for (const auto head : heads) {
        std::array<void*, MaximumScannedStatLists> chain{};
        std::size_t chainCount{};
        auto* list = Read<void*>(statListEx, head);
        while (list) {
            if (chainCount >= chain.size()
                    || ContainsPointer(chain.data(), chainCount, list)) {
                result.invalid = true;
                return result;
            }
            chain[chainCount++] = list;
            if (ContainsPointer(visited.data(), visitedCount, list)) {
                break;
            }
            if (visitedCount >= visited.size()) {
                result.invalid = true;
                return result;
            }
            visited[visitedCount++] = list;

            if (Read<std::int32_t>(list, StatListStateOffset)
                    == OpenWoundsStateId) {
                if (!IsManagedOpenWoundsStatList(
                        list,
                        target,
                        currentFrame,
                        expiryFrame,
                        true)) {
                    result.foreign = true;
                } else {
                    if (!result.managedFallback) result.managedFallback = list;
                    if (Read<std::int32_t>(list, StatListSourceTypeOffset) != 0
                            || Read<std::uint32_t>(
                                list, StatListSourceGuidOffset) != sourceGuid) {
                        result.foreign = true;
                    } else {
                        if (result.ownedCount >= result.owned.size()) {
                            result.invalid = true;
                            return result;
                        }
                        const auto flags = Read<std::uint32_t>(
                            list, StatListFlagsOffset);
                        if (result.ownedCount != 0 && flags != result.flags) {
                            result.invalid = true;
                            return result;
                        }
                        if (result.ownedCount == 0) result.flags = flags;
                        result.owned[result.ownedCount++] = list;
                    }
                }
            }
            list = Read<void*>(list, StatListNextOffset);
        }
    }

    if (auto* stateList = LookupStateStatList(target, OpenWoundsStateId)) {
        if (!ContainsPointer(
                result.owned.data(), result.ownedCount, stateList)) {
            result.foreign = true;
        }
    }
    return result;
}

std::optional<std::int32_t> ComputeOpenWoundsRate(
        void* source,
        void* target,
        const bool quarterDamage) noexcept {
    if (!GetUnitStat) return std::nullopt;
    const auto level = GetUnitStat(source, CharacterLevelStatId, 0);
    if (level <= 0) return std::nullopt;

    const auto level64 = static_cast<std::int64_t>(level);
    std::int64_t base{};
    if (level <= 15) {
        base = 9 * level64 + 31;
    } else if (level <= 30) {
        base = 18 * level64 - 104;
    } else if (level <= 45) {
        base = 27 * level64 - 374;
    } else if (level <= 60) {
        base = 36 * level64 - 779;
    } else {
        base = 45 * level64 - 1319;
    }
    if (base <= 0) return std::nullopt;

    const auto resistance = std::clamp(
        GetUnitStat(target, PhysicalResistanceStatId, 0), -100, 100);
    const auto resistanceScale = static_cast<std::int64_t>(100 - resistance);
    if (resistanceScale != 0
            && base > std::numeric_limits<std::int64_t>::max()
                / resistanceScale) {
        return std::nullopt;
    }
    auto rate = base * resistanceScale / 100;
    if (quarterDamage) rate /= 4;
    rate = std::max<std::int64_t>(rate, 0);
    if (rate > std::numeric_limits<std::int32_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::int32_t>(rate);
}

void ScheduleOpenWoundsExpiry(
        void* game,
        void* target,
        const std::int32_t expiryFrame) noexcept {
    SetEvent(
        game,
        target,
        StatListExpireEventType,
        expiryFrame,
        0,
        0,
        0);
}

void* CreateAdditionalOpenWoundsStack(
        const OpenWoundsApplyArgs& args,
        void* game,
        const std::uint32_t sourceGuid,
        const std::uint32_t flags,
        const std::int32_t currentFrame,
        const std::int32_t expiryFrame,
        const std::int32_t statValue,
        const std::uint8_t context) noexcept {
    auto* list = AllocateStatList(flags, expiryFrame, 0, sourceGuid);
    if (!list) return nullptr;

    SetStatListState(list, OpenWoundsStateId);
    SetStatListSkillId(list, args.skillId);
    SetStatListSkillLevel(list, args.skillLevel);
    SetStatListStat(
        context,
        list,
        OpenWoundsRegenStatId,
        statValue,
        0);
    SetStatListRemoveCallback(list, OpenWoundsRemoveCallback);
    SetStatListExpiry(list, expiryFrame);
    if (!IsOwnedOpenWoundsStatList(
            list,
            nullptr,
            sourceGuid,
            currentFrame,
            expiryFrame,
            false)) {
        FreeStatList(context, list);
        return nullptr;
    }

    PostStatList(args.target, list, 1);
    if (!IsOwnedOpenWoundsStatList(
            list,
            args.target,
            sourceGuid,
            currentFrame,
            expiryFrame,
            true)) {
        UnlinkStatList(args.target, list);
        FreeStatList(context, list);
        return nullptr;
    }
    ScheduleOpenWoundsExpiry(game, args.target, expiryFrame);
    return list;
}

void __fastcall OpenWoundsRemoveCallback(
        const std::uint8_t context,
        void* target,
        const std::int32_t state,
        void* removed) noexcept {
    if (target && state == OpenWoundsStateId && LookupStateStatList
            && LookupStateStatList(target, OpenWoundsStateId)) {
        return;
    }
    if (DefaultStateRemoveCallback) {
        DefaultStateRemoveCallback(context, target, state, removed);
    }
}

void FailClosedFromNativeHook(const char* reason) noexcept {
    if (!Operational.exchange(false, std::memory_order_acq_rel)) return;
    if (Context) Context->LogError(reason);
}

void* __fastcall HookOpenWoundsApply(
        OpenWoundsApplyArgs* args) noexcept {
    const auto callOriginal = [&]() noexcept -> void* {
        return OriginalApplyStateStatList
            ? OriginalApplyStateStatList(args)
            : nullptr;
    };
    if (!args
            || !Operational.load(std::memory_order_acquire)
            || !ActiveConfig.policies.openWounds
            || !args->source || !args->target
            || args->state != OpenWoundsStateId
            || args->statId != OpenWoundsRegenStatId
            || args->statValue >= 0
            || !GetUnitType || !GetHirelingTypeId || !GetMinionOwner
            || !GetUnitGame || !GetUnitGuid || !GetUnitDataContext
            || !GetStatListEx || !LookupStateStatList || !AllocateStatList
            || !SetStatListState || !SetStatListSkillId
            || !SetStatListSkillLevel || !SetStatListStat
            || !SetStatListRemoveCallback || !SetStatListExpiry
            || !PostStatList || !SetEvent || !UnlinkStatList
            || !FreeStatList || !DefaultStateRemoveCallback
            || GetUnitType(args->source) != 0
            || GetUnitType(args->target) != MonsterUnitType) {
        return callOriginal();
    }

    auto* sourceGame = GetUnitGame(args->source);
    auto* targetGame = GetUnitGame(args->target);
    if (!sourceGame || sourceGame != targetGame) return callOriginal();
    const auto sourceGuid = GetUnitGuid(args->source);
    if (sourceGuid == std::numeric_limits<std::uint32_t>::max()) {
        return callOriginal();
    }
    const auto currentFrame = Read<std::int32_t>(
        sourceGame, GameFrameOffset);
    if (currentFrame < 0
            || currentFrame > std::numeric_limits<std::int32_t>::max()
                - OpenWoundsDurationFrames) {
        return callOriginal();
    }
    const auto expiryFrame = currentFrame + OpenWoundsDurationFrames;

    const auto hireling = GetHirelingTypeId(args->target) != 0;
    auto* minionOwner = GetMinionOwner(args->target);
    const auto pet = minionOwner && minionOwner != args->target;
    const auto rate = ComputeOpenWoundsRate(
        args->source, args->target, hireling || pet);
    if (!rate) return callOriginal();
    const auto statValue = -*rate;

    auto scan = ScanOpenWoundsStatLists(
        args->target, sourceGuid, currentFrame, expiryFrame);
    if (scan.invalid) return scan.managedFallback;
    if (scan.foreign) {
        return scan.managedFallback ? scan.managedFallback : callOriginal();
    }
    if (scan.ownedCount == 0) {
        auto customArgs = *args;
        customArgs.duration = OpenWoundsDurationFrames;
        customArgs.statValue = statValue;
        customArgs.removeCallback = OpenWoundsRemoveCallback;
        auto* list = OriginalApplyStateStatList(&customArgs);
        if (list && !IsOwnedOpenWoundsStatList(
                list,
                args->target,
                sourceGuid,
                currentFrame,
                expiryFrame,
                true)) {
            FailClosedFromNativeHook(
                "BKVCombat: first Open Wounds stat list failed post-apply "
                "validation; all policies are inert until cold restart.");
        }
        return list;
    }
    if (scan.ownedCount < MaximumOpenWoundsStacks) {
        const auto context = GetUnitDataContext(args->source);
        auto* list = CreateAdditionalOpenWoundsStack(
            *args,
            sourceGame,
            sourceGuid,
            scan.flags,
            currentFrame,
            expiryFrame,
            statValue,
            context);
        return list ? list : scan.owned[0];
    }

    for (std::size_t index{}; index < scan.ownedCount; ++index) {
        SetStatListExpiry(scan.owned[index], expiryFrame);
    }
    ScheduleOpenWoundsExpiry(sourceGame, args->target, expiryFrame);
    return scan.owned[0];
}

CriticalDeadlyOutcome __fastcall ResolveCriticalDeadly(
        void*,
        void* attacker,
        void*,
        void* damage,
        const std::int32_t damageMode,
        const std::uint32_t) noexcept {
    if (!Operational.load(std::memory_order_acquire)
            || (!ActiveConfig.policies.criticalStrike
                && !ActiveConfig.policies.deadlyStrike)
            || !attacker || !damage || !ResolveActiveWeapon
            || !GetWeaponMasteryChance || !GetUnitStat || !GetLayeredStat
            || !GetSeed) {
        return CriticalDeadlyOutcome::Unavailable;
    }
    if ((Read<std::uint16_t>(damage, DamageResultFlagsOffset)
            & CriticalDeadlyResultFlag) != 0) {
        return CriticalDeadlyOutcome::AlreadyResolved;
    }

    if (damageMode == 0) {
        auto* weapon = ResolveActiveWeapon(attacker);
        if (weapon) {
            const auto chance = CappedChance(
                GetWeaponMasteryChance(attacker, weapon, 0, 2),
                ActiveConfig.policies.criticalStrike);
            if (RollChance(attacker, chance)) {
                ApplyResolvedMultiplier(damage, RollSource::Critical);
                return CriticalDeadlyOutcome::Critical;
            }
        }
    }

    const auto criticalChance = CappedChance(
        GetUnitStat(attacker, PassiveCriticalStatId, 0),
        ActiveConfig.policies.criticalStrike);
    if (RollChance(attacker, criticalChance)) {
        ApplyResolvedMultiplier(damage, RollSource::Critical);
        return CriticalDeadlyOutcome::Critical;
    }

    const auto deadlyChance = CappedChance(
        GetLayeredStat(attacker, DeadlyStrikeStatId, 0),
        ActiveConfig.policies.deadlyStrike);
    if (RollChance(attacker, deadlyChance)) {
        ApplyResolvedMultiplier(damage, RollSource::Deadly);
        return CriticalDeadlyOutcome::Deadly;
    }
    return CriticalDeadlyOutcome::None;
}

std::uint64_t __cdecl GetActiveCapabilities() noexcept {
    if (!Operational.load(std::memory_order_acquire)
            || (!ActiveConfig.policies.criticalStrike
                && !ActiveConfig.policies.deadlyStrike)) {
        return 0;
    }
    return static_cast<std::uint64_t>(
        NativeCapability::CriticalDeadlyResolver);
}

constexpr NativeApiV1 Api{
    .size = sizeof(NativeApiV1),
    .version = NativeApiVersion,
    .compiledCapabilities = static_cast<std::uint64_t>(
        NativeCapability::CriticalDeadlyResolver),
    .resolveCriticalDeadly = ResolveCriticalDeadly,
    .getActiveCapabilities = GetActiveCapabilities,
};

bool LeechPolicyRequested() noexcept {
    return ActiveConfig.policies.lifeSteal
        || ActiveConfig.policies.manaSteal;
}

bool HookPolicyRequested() noexcept {
    return ActiveConfig.policies.criticalStrike
        || ActiveConfig.policies.deadlyStrike
        || ActiveConfig.policies.crushingBlow
        || ActiveConfig.policies.openWounds;
}

bool AnyPolicyRequested() noexcept {
    return HookPolicyRequested() || LeechPolicyRequested();
}

std::filesystem::path ExecutableDirectory() {
    std::array<wchar_t, 32768> buffer{};
    const auto length = GetModuleFileNameW(
        nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) return {};
    return std::filesystem::path(buffer.data(), buffer.data() + length)
        .parent_path();
}

bool LoadSettings() noexcept {
    try {
        const auto activeModConfig = Context->modSupportDirectory
            ? std::filesystem::path(Context->modSupportDirectory) / L"config"
            : std::filesystem::path{};
        const auto scopeConfig = Context->pluginConfigPath
            ? std::filesystem::path(Context->pluginConfigPath).parent_path()
            : (Context->pluginDirectory
                ? std::filesystem::path(Context->pluginDirectory)
                    .parent_path() / L"config"
                : std::filesystem::path{});
        const auto globalConfig = ExecutableDirectory()
            / L"d2rloader" / L"config";
        const auto loaded = LoadConfig(BuildConfigCandidates(
            activeModConfig, scopeConfig, globalConfig));
        ActiveConfig = loaded.config;
        ActiveConfigPath = loaded.source;
        return true;
    } catch (const std::exception& exception) {
        char message[1024]{};
        std::snprintf(
            message,
            sizeof(message),
            "BKVCombat: configuration refused: %s",
            exception.what());
        Context->LogError(message);
        return false;
    } catch (...) {
        Context->LogError(
            "BKVCombat: configuration refused by an unknown error.");
        return false;
    }
}

template <std::size_t Size>
bool Check(
        const std::uintptr_t rva,
        const std::array<std::uint8_t, Size>& expected,
        const char* name) noexcept {
    if (Context->CheckExpectedBytes(
            rva, expected.data(), static_cast<std::uint32_t>(expected.size()))) {
        return true;
    }
    char message[256]{};
    std::snprintf(
        message,
        sizeof(message),
        "BKVCombat: signature mismatch at %s (RVA 0x%llX).",
        name,
        static_cast<unsigned long long>(rva));
    Context->LogError(message);
    return false;
}

bool ValidateOwnedSites() noexcept {
    const auto criticalDeadlyRequested =
        ActiveConfig.policies.criticalStrike
        || ActiveConfig.policies.deadlyStrike;
    if (criticalDeadlyRequested && (!Check(
            ResolveActiveWeaponRva,
            ResolveActiveWeaponExpected,
            "active weapon resolver")
            || !Check(
                GetWeaponMasteryChanceRva,
                GetWeaponMasteryChanceExpected,
                "weapon mastery chance helper")
            || !Check(
                GetUnitStatRva,
                GetUnitStatExpected,
                "unit stat helper")
            || !Check(
                GetLayeredStatRva,
                GetLayeredStatExpected,
                "layered stat helper")
            || !Check(
                GetSeedRva,
                GetSeedExpected,
                "unit seed accessor"))) {
        return false;
    }
    if (criticalDeadlyRequested && !Check(
            WeaponMasteryContextRva,
            WeaponMasteryContextExpected,
            "weapon-mastery Critical call")) {
        return false;
    }
    if (criticalDeadlyRequested && !Check(
            PassiveCriticalContextRva,
            PassiveCriticalContextExpected,
            "passive Critical call")) {
        return false;
    }
    if (ActiveConfig.policies.deadlyStrike
            && (!Check(
                DeadlyStrikeContextRva,
                DeadlyStrikeContextExpected,
                "Deadly Strike call")
            || !Check(
                CriticalDeadlyTailContextRva,
                CriticalDeadlyTailContextExpected,
                "Critical/Deadly success tail"))) {
        return false;
    }
    if (ActiveConfig.policies.crushingBlow && !(Check(
            CrushingBlowCoreContextRva,
            CrushingBlowCoreContextExpected,
            "Event16 Crushing Blow core call")
        && Check(
            CrushingBlowCoreRva,
            CrushingBlowCoreExpected,
            "Crushing Blow core")
        && Check(
            GetUnitTypeRva,
            GetUnitTypeExpected,
            "unit type helper")
        && Check(
            GetHirelingTypeIdRva,
            GetHirelingTypeIdExpected,
            "hireling type helper")
        && Check(
            CheckMonsterTypeFlagRva,
            CheckMonsterTypeFlagExpected,
            "monster type flag helper")
        && Check(
            GetUnitRoomRva,
            GetUnitRoomExpected,
            "unit room helper")
        && Check(
            GetUnitStatRva,
            GetUnitStatExpected,
            "unit stat helper")
        && Check(
            SetUnitStatRva,
            SetUnitStatExpected,
            "unit stat setter")
        && Check(
            SetOverlayRva,
            SetOverlayExpected,
            "unit overlay helper"))) {
        return false;
    }
    if (ActiveConfig.policies.crushingBlow
            && ActiveConfig.stats.crushingBlowEfficiencyStatId >= 0
            && !Check(
                ResolveActiveWeaponRva,
                ResolveActiveWeaponExpected,
                "active weapon resolver for Crushing Blow Efficiency")) {
        return false;
    }
    if (LeechPolicyRequested() && (!Check(
            LeechCallContextRva,
            LeechCallContextExpected,
            "Life/Mana Steal authoritative call")
            || !Check(
                ApplyLifeAndManaLeechRva,
                ApplyLifeAndManaLeechExpected,
                "Life/Mana Steal native consumer")
            || !Check(
                LifeTapHandlerRva,
                LifeTapHandlerExpected,
                "Life Tap native handler"))) {
        return false;
    }
    if (!ActiveConfig.policies.openWounds) return true;
    return Check(
            OpenWoundsApplyCallRva,
            OpenWoundsApplyCallExpected,
            "Event15 Open Wounds apply call")
        && Check(
            ApplyStateStatListRva,
            ApplyStateStatListExpected,
            "state stat-list apply helper")
        && Check(
            GetUnitTypeRva,
            GetUnitTypeExpected,
            "unit type helper")
        && Check(
            GetHirelingTypeIdRva,
            GetHirelingTypeIdExpected,
            "hireling type helper")
        && Check(
            GetMinionOwnerRva,
            GetMinionOwnerExpected,
            "minion owner helper")
        && Check(
            GetUnitGameRva,
            GetUnitGameExpected,
            "unit game helper")
        && Check(
            GetUnitGuidRva,
            GetUnitGuidExpected,
            "unit GUID helper")
        && Check(
            GetUnitDataContextRva,
            GetUnitDataContextExpected,
            "unit data-context helper")
        && Check(
            GetUnitStatRva,
            GetUnitStatExpected,
            "unit stat helper")
        && Check(
            GetStatListExRva,
            GetStatListExExpected,
            "extended stat-list accessor")
        && Check(
            LookupStateStatListRva,
            LookupStateStatListExpected,
            "state stat-list lookup")
        && Check(
            AllocateStatListRva,
            AllocateStatListExpected,
            "stat-list allocator")
        && Check(
            SetStatListStateRva,
            SetStatListStateExpected,
            "stat-list state setter")
        && Check(
            SetStatListSkillIdRva,
            SetStatListSkillIdExpected,
            "stat-list skill-id setter")
        && Check(
            SetStatListSkillLevelRva,
            SetStatListSkillLevelExpected,
            "stat-list skill-level setter")
        && Check(
            SetStatListStatRva,
            SetStatListStatExpected,
            "stat-list stat setter")
        && Check(
            SetStatListRemoveCallbackRva,
            SetStatListRemoveCallbackExpected,
            "stat-list callback setter")
        && Check(
            SetStatListExpiryRva,
            SetStatListExpiryExpected,
            "stat-list expiry setter")
        && Check(
            PostStatListRva,
            PostStatListExpected,
            "stat-list post helper")
        && Check(
            SetEventRva,
            SetEventExpected,
            "event scheduler")
        && Check(
            DefaultStateRemoveCallbackRva,
            DefaultStateRemoveCallbackExpected,
            "default state remove callback")
        && Check(
            UnlinkStatListRva,
            UnlinkStatListExpected,
            "stat-list unlink helper")
        && Check(
            FreeStatListRva,
            FreeStatListExpected,
            "stat-list free helper");
}

void* AllocateRelayPageNear(void* hint) noexcept {
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const auto granularity = static_cast<std::uintptr_t>(
        systemInfo.dwAllocationGranularity);
    const auto base = reinterpret_cast<std::uintptr_t>(hint)
        & ~(granularity - 1U);
    for (std::uintptr_t delta = granularity;
            delta < UINT64_C(0x70000000); delta += granularity) {
        if (base > std::numeric_limits<std::uintptr_t>::max() - delta) break;
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

bool WriteAbsoluteJumpRelay(
        std::uint8_t* destination, const void* target) noexcept {
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

bool WriteTailRelay(std::uint8_t* destination) noexcept {
    if (!destination || !Base) return false;
    // The governed FillDamageValues frame has RSP 16-byte aligned here. Reserve
    // the Win64 shadow space, call the noexcept helper with damage=RDI, then
    // continue immediately after the replaced native multiplier/flag sequence.
    std::array<std::uint8_t, 35> stub{
        0x48,0x83,0xEC,0x20,
        0x48,0x8B,0xCF,
        0x48,0xB8,0,0,0,0,0,0,0,0,
        0xFF,0xD0,
        0x48,0x83,0xC4,0x20,
        0x48,0xB8,0,0,0,0,0,0,0,0,
        0xFF,0xE0,
    };
    const auto helper = reinterpret_cast<std::uintptr_t>(
        &ApplyPrimaryCriticalDeadlyTail);
    const auto continuation = reinterpret_cast<std::uintptr_t>(
        Base + CriticalDeadlyTailContinuationRva);
    std::memcpy(stub.data() + 9, &helper, sizeof(helper));
    std::memcpy(stub.data() + 25, &continuation, sizeof(continuation));
    std::memcpy(destination, stub.data(), stub.size());
    return true;
}

bool CreateRelays() noexcept {
    RelayPage = AllocateRelayPageNear(Base + WeaponMasteryChanceCallRva);
    if (!RelayPage) return false;
    auto* relays = static_cast<std::uint8_t*>(RelayPage);
    if (!WriteAbsoluteJumpRelay(
            relays + MasteryRelayOffset,
            reinterpret_cast<const void*>(&HookWeaponMasteryChance))
            || !WriteAbsoluteJumpRelay(
                relays + PassiveRelayOffset,
                reinterpret_cast<const void*>(&HookPassiveCriticalChance))
            || !WriteAbsoluteJumpRelay(
                relays + DeadlyRelayOffset,
                reinterpret_cast<const void*>(&HookDeadlyStrikeChance))
            || !WriteAbsoluteJumpRelay(
                relays + CrushingBlowRelayOffset,
                reinterpret_cast<const void*>(&HookCrushingBlowCore))
            || !WriteAbsoluteJumpRelay(
                relays + OpenWoundsRelayOffset,
                reinterpret_cast<const void*>(&HookOpenWoundsApply))
            || !WriteTailRelay(relays + TailRelayOffset)) {
        VirtualFree(RelayPage, 0, MEM_RELEASE);
        RelayPage = nullptr;
        return false;
    }

    DWORD previousProtection{};
    if (!VirtualProtect(
            RelayPage, RelayBytes, PAGE_EXECUTE_READ, &previousProtection)) {
        VirtualFree(RelayPage, 0, MEM_RELEASE);
        RelayPage = nullptr;
        return false;
    }
    FlushInstructionCache(GetCurrentProcess(), RelayPage, RelayBytes);
    const auto relay = reinterpret_cast<std::uintptr_t>(RelayPage);
    const auto base = reinterpret_cast<std::uintptr_t>(Base);
    if (relay < base
            || relay - base > std::numeric_limits<std::uint32_t>::max()) {
        VirtualFree(RelayPage, 0, MEM_RELEASE);
        RelayPage = nullptr;
        return false;
    }
    return true;
}

bool PatchCall(
        const std::uintptr_t rva,
        const std::array<std::uint8_t, 5>& expected,
        const std::size_t relayOffset) noexcept {
    const auto relayRva = reinterpret_cast<std::uintptr_t>(RelayPage)
        - reinterpret_cast<std::uintptr_t>(Base)
        + relayOffset;
    if (!Context->PatchCallRel32(
            rva,
            expected.data(),
            static_cast<std::uint32_t>(expected.size()),
            relayRva,
            static_cast<std::uint32_t>(expected.size()))) {
        return false;
    }
    AnyMutationInstalled.store(true, std::memory_order_release);
    return true;
}

bool InstallHooks() noexcept {
    if (!HookPolicyRequested()) return true;
    if (!CreateRelays()) return false;
    const auto criticalDeadlyRequested =
        ActiveConfig.policies.criticalStrike
        || ActiveConfig.policies.deadlyStrike;
    if (criticalDeadlyRequested && (!PatchCall(
            WeaponMasteryChanceCallRva,
            WeaponMasteryChanceCallExpected,
            MasteryRelayOffset)
            || !PatchCall(
                PassiveCriticalChanceCallRva,
                PassiveCriticalChanceCallExpected,
                PassiveRelayOffset))) {
        return false;
    }
    if (ActiveConfig.policies.deadlyStrike && !PatchCall(
            DeadlyStrikeChanceCallRva,
            DeadlyStrikeChanceCallExpected,
            DeadlyRelayOffset)) {
        return false;
    }
    if (ActiveConfig.policies.deadlyStrike) {
        const auto relayRva = reinterpret_cast<std::uintptr_t>(RelayPage)
            - reinterpret_cast<std::uintptr_t>(Base)
            + TailRelayOffset;
        if (!Context->PatchJmpRel32(
                CriticalDeadlyTailRva,
                CriticalDeadlyTailExpected.data(),
                static_cast<std::uint32_t>(CriticalDeadlyTailExpected.size()),
                relayRva,
                static_cast<std::uint32_t>(
                    CriticalDeadlyTailExpected.size()))) {
            return false;
        }
        AnyMutationInstalled.store(true, std::memory_order_release);
    }
    if (ActiveConfig.policies.crushingBlow
            && !PatchCall(
                CrushingBlowCoreCallRva,
                CrushingBlowCoreCallExpected,
                CrushingBlowRelayOffset)) {
        return false;
    }
    if (ActiveConfig.policies.openWounds
            && !PatchCall(
                OpenWoundsApplyCallRva,
                OpenWoundsApplyCallExpected,
                OpenWoundsRelayOffset)) {
        return false;
    }
    return true;
}

void ResolveNativeFunctions() noexcept {
    ResolveActiveWeapon = At<ResolveActiveWeaponFn>(ResolveActiveWeaponRva);
    GetWeaponMasteryChance = At<GetWeaponMasteryChanceFn>(
        GetWeaponMasteryChanceRva);
    GetUnitStat = At<GetStatFn>(GetUnitStatRva);
    GetLayeredStat = At<GetStatFn>(GetLayeredStatRva);
    GetSeed = At<GetSeedFn>(GetSeedRva);
    OriginalCrushingBlowCore = At<CrushingBlowCoreFn>(CrushingBlowCoreRva);
    GetUnitType = At<GetUnitTypeFn>(GetUnitTypeRva);
    GetHirelingTypeId = At<GetHirelingTypeIdFn>(GetHirelingTypeIdRva);
    GetMinionOwner = At<GetMinionOwnerFn>(GetMinionOwnerRva);
    CheckMonsterTypeFlag = At<CheckMonsterTypeFlagFn>(
        CheckMonsterTypeFlagRva);
    GetUnitRoom = At<GetUnitRoomFn>(GetUnitRoomRva);
    GetPlayerCountBonus = At<GetPlayerCountBonusFn>(GetPlayerCountBonusRva);
    SetUnitStat = At<SetUnitStatFn>(SetUnitStatRva);
    SetOverlay = At<SetOverlayFn>(SetOverlayRva);
    OriginalApplyStateStatList = At<ApplyStateStatListFn>(
        ApplyStateStatListRva);
    GetUnitGame = At<GetUnitGameFn>(GetUnitGameRva);
    GetUnitGuid = At<GetUnitGuidFn>(GetUnitGuidRva);
    GetUnitDataContext = At<GetUnitDataContextFn>(GetUnitDataContextRva);
    GetStatListEx = At<GetStatListExFn>(GetStatListExRva);
    LookupStateStatList = At<LookupStateStatListFn>(
        LookupStateStatListRva);
    AllocateStatList = At<AllocateStatListFn>(AllocateStatListRva);
    SetStatListState = At<SetStatListIntFn>(SetStatListStateRva);
    SetStatListSkillId = At<SetStatListIntFn>(SetStatListSkillIdRva);
    SetStatListSkillLevel = At<SetStatListIntFn>(
        SetStatListSkillLevelRva);
    SetStatListStat = At<SetStatListStatFn>(SetStatListStatRva);
    SetStatListRemoveCallback = At<SetStatListRemoveCallbackFn>(
        SetStatListRemoveCallbackRva);
    SetStatListExpiry = At<SetStatListIntFn>(SetStatListExpiryRva);
    PostStatList = At<PostStatListFn>(PostStatListRva);
    SetEvent = At<SetEventFn>(SetEventRva);
    DefaultStateRemoveCallback = At<OpenWoundsRemoveCallbackFn>(
        DefaultStateRemoveCallbackRva);
    UnlinkStatList = At<UnlinkStatListFn>(UnlinkStatListRva);
    FreeStatList = At<FreeStatListFn>(FreeStatListRva);
}

void ResetState() noexcept {
    Operational.store(false, std::memory_order_release);
    AnyMutationInstalled.store(false, std::memory_order_release);
    CurrentRollSource = RollSource::None;
    ActiveConfig = {};
    ActiveConfigPath.clear();
    RelayPage = nullptr;
    ResolveActiveWeapon = nullptr;
    GetWeaponMasteryChance = nullptr;
    GetUnitStat = nullptr;
    GetLayeredStat = nullptr;
    GetSeed = nullptr;
    OriginalCrushingBlowCore = nullptr;
    GetUnitType = nullptr;
    GetHirelingTypeId = nullptr;
    GetMinionOwner = nullptr;
    CheckMonsterTypeFlag = nullptr;
    GetUnitRoom = nullptr;
    GetPlayerCountBonus = nullptr;
    SetUnitStat = nullptr;
    SetOverlay = nullptr;
    OriginalApplyStateStatList = nullptr;
    GetUnitGame = nullptr;
    GetUnitGuid = nullptr;
    GetUnitDataContext = nullptr;
    GetStatListEx = nullptr;
    LookupStateStatList = nullptr;
    AllocateStatList = nullptr;
    SetStatListState = nullptr;
    SetStatListSkillId = nullptr;
    SetStatListSkillLevel = nullptr;
    SetStatListStat = nullptr;
    SetStatListRemoveCallback = nullptr;
    SetStatListExpiry = nullptr;
    PostStatList = nullptr;
    SetEvent = nullptr;
    DefaultStateRemoveCallback = nullptr;
    UnlinkStatList = nullptr;
    FreeStatList = nullptr;
    MonsterCatalog.clear();
    ActiveMajorBossRegistry = {};
}

} // namespace
} // namespace RuffnecKk::BKVCombat

namespace {

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "bkv-combat",
    .name = "BKVCombat",
    .version = RuffnecKk::BKVCombat::Version,
    .author = "RuffnecKk",
    .description = "Applies configurable combat mechanics.",
    .flags = D2RL::PluginFlags::NativeHooks,
};

} // namespace

extern "C" __declspec(dllexport)
auto __cdecl BKVCombat_GetApi(const std::uint32_t requestedVersion) noexcept
        -> const RuffnecKk::BKVCombat::NativeApiV1* {
    using namespace RuffnecKk::BKVCombat;
    return requestedVersion == NativeApiVersion ? &Api : nullptr;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept
        -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    using namespace RuffnecKk::BKVCombat;
    Context = context;
    Base = context ? reinterpret_cast<std::uint8_t*>(context->exeBase) : nullptr;
    ResetState();
    Context = context;
    Base = context ? reinterpret_cast<std::uint8_t*>(context->exeBase) : nullptr;
    if (!Context || !Base) return false;
    if (!LoadSettings()) return false;
    if (!ActiveConfig.enabled) {
        Context->LogInfo(
            "BKVCombat 0.1.0 by RuffnecKk loaded disabled; no hooks installed.");
        return true;
    }
    if (context->modDataVersionBuild != 0
            && context->modDataVersionBuild != SupportedBuild) {
        Context->LogError("BKVCombat: supports only D2R 3.2 build 92777.");
        return false;
    }
    if (!AnyPolicyRequested()) {
        Context->LogInfo(
            "BKVCombat: enabled with no proven Release 1 policy selected; no "
            "hooks installed.");
        return true;
    }
    if (ActiveConfig.policies.crushingBlow
            && !LoadCrushingBlowCatalog()) {
        return false;
    }
    if (ActiveConfig.policies.crushingBlow
            && !ValidateCrushingBlowEfficiencyStatData()) {
        return false;
    }
    if (LeechPolicyRequested() && !ValidateLeechDifficultyData()) {
        return false;
    }

    ResolveNativeFunctions();
    if (!ValidateOwnedSites()) {
        Context->LogError(
            "BKVCombat: 92777 ownership preflight failed; no hooks installed.");
        return false;
    }
    if (!InstallHooks()) {
        Operational.store(false, std::memory_order_release);
        if (AnyMutationInstalled.load(std::memory_order_acquire)) {
            Context->LogError(
                "BKVCombat: partial hook commit is pass-through and inert; keep "
                "the DLL loaded and cold-restart after resolving the conflict.");
            return true;
        }
        if (RelayPage) {
            VirtualFree(RelayPage, 0, MEM_RELEASE);
            RelayPage = nullptr;
        }
        Context->LogError(
            "BKVCombat: hook installation failed before mutation.");
        return false;
    }

    Operational.store(true, std::memory_order_release);
    if (ActiveConfig.diagnosticLogging) {
        char message[512]{};
        std::snprintf(
            message,
            sizeof(message),
            "BKVCombat diagnostics: Critical=%d Deadly=%d CB=%d CBEStat=%d "
            "LifeSteal=%d ManaSteal=%d OW=%d.",
            ActiveConfig.policies.criticalStrike ? 1 : 0,
            ActiveConfig.policies.deadlyStrike ? 1 : 0,
            ActiveConfig.policies.crushingBlow ? 1 : 0,
            ActiveConfig.stats.crushingBlowEfficiencyStatId,
            ActiveConfig.policies.lifeSteal ? 1 : 0,
            ActiveConfig.policies.manaSteal ? 1 : 0,
            ActiveConfig.policies.openWounds ? 1 : 0);
        Context->LogInfo(message);
    }
    Context->LogInfo(
        "BKVCombat 0.1.0 by RuffnecKk active; selected Release 1 policies "
        "validated and installed where ownership is required.");
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    using namespace RuffnecKk::BKVCombat;
    Operational.store(false, std::memory_order_release);
    CurrentRollSource = RollSource::None;
    Context = nullptr;
}
