#pragma once

#include "scripted_ai_native.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace ruffneckk::scripted_ai {

inline constexpr std::uintptr_t GetClassIdRva = 0x349860U;
inline constexpr std::uintptr_t GetUnitIdRva = 0x34A330U;
inline constexpr std::uintptr_t GetUnitTypeRva = 0x34B9D0U;
inline constexpr std::uintptr_t IsUnitDeadRva = 0x34C2C0U;
inline constexpr std::uintptr_t GetServerUnitRva = 0x48FE80U;
inline constexpr std::uintptr_t UnitDistanceRva = 0x596720U;
inline constexpr std::uintptr_t NormalAiTableRva = 0x2396E90U;
inline constexpr std::uintptr_t IdleInNeutralModeRva = 0x4A6D10U;
inline constexpr std::uintptr_t ChangeModeAndTargetUnitRva = 0x4A78E0U;
inline constexpr std::uintptr_t UseSkillRva = 0x4A7BC0U;
inline constexpr std::uintptr_t EscapeRva = 0x4A7DF0U;
inline constexpr std::uintptr_t WalkCloseToUnitRva = 0x4A8320U;
inline constexpr std::uintptr_t WalkToTargetUnitRva = 0x4A8740U;
inline constexpr std::size_t UnitMonsterDataOffset = 0x10U;
inline constexpr std::size_t MonStatsSkillIdsOffset = 0x1C4U;
inline constexpr std::size_t MonStatsSkillModesOffset = 0x1DCU;
inline constexpr std::size_t MonStatsFlagsOffset = 0x3CU;
inline constexpr std::size_t MonStatsSkillSlotCount = 8U;
inline constexpr std::size_t MonStatsRequiredRecordSize =
    MonStatsSkillModesOffset
    + MonStatsSkillSlotCount * sizeof(std::int16_t);
inline constexpr std::uint32_t MonsterUnitType = 1U;
inline constexpr std::uint32_t MaximumUnitType = 5U;
inline constexpr std::uint8_t MonsterAttack1Mode = 4U;
inline constexpr std::uint8_t MonsterCastMode = 7U;
inline constexpr std::uint8_t MonsterModeCount = 16U;
inline constexpr std::uint32_t MonStatsIsMeleeFlag = 1U << 12U;
inline constexpr std::uint32_t InvalidUnitId = 0xFFFFFFFFU;

struct D2AiTickParam {
    void* aiControl{};
    std::uint64_t reserved08{};
    void* target{};
    std::uint64_t reserved18{};
    std::int32_t targetDistance{};
    std::int32_t inCombat{};
    const void* monStats{};
    const void* monStats2{};
};

using D2AiCallback = void(__fastcall*)(
    void* game,
    void* unit,
    D2AiTickParam* tick) noexcept;

struct D2AiTableRecord {
    std::int32_t targetProfile{};
    std::uint32_t reserved04{};
    D2AiCallback initialize{};
    D2AiCallback think{};
    D2AiCallback transition{};
};

struct NativeTableBankView {
    const std::byte* rows{};
    std::uint32_t rowCount{};
    std::uint32_t rowSize{};
    std::uint32_t skillRowCount{};
    ScriptBank scriptBank{ScriptBank::Base};
};

struct ResolvedNativeBank {
    ScriptBank scriptBank{ScriptBank::Base};
    std::uint32_t skillRowCount{};
    const void* monStats{};
    bool found{};
};

[[nodiscard]] auto ReadUnitMonStatsRecord(const void* unit) noexcept
    -> const void*;

[[nodiscard]] auto ClassifyMonStatsRecord(
    const void* monStats,
    std::uint32_t classId,
    std::span<const NativeTableBankView> banks) noexcept
    -> ResolvedNativeBank;

[[nodiscard]] auto StockAiRecord(
    std::uintptr_t moduleBase,
    std::uint16_t aiIndex) noexcept -> const D2AiTableRecord*;

struct ReviveTacticalLoadout {
    TacticalProfile profile{TacticalProfile::None};
    std::uint16_t preferredSkill{};
    std::uint8_t preferredSlot{};
    bool hasPreferredSkill{};
};

[[nodiscard]] auto SelectReviveTacticalLoadout(
    const void* monStats,
    std::uint32_t skillRowCount,
    std::uint8_t startingSlot) noexcept -> ReviveTacticalLoadout;

struct D2NativeFunctions {
    using GetUnitValue = std::int32_t(__fastcall*)(const void*) noexcept;
    using GetServerUnit = void*(__fastcall*)(
        void*, std::int32_t, std::int32_t) noexcept;
    using Distance = std::int32_t(__fastcall*)(void*, void*) noexcept;
    using Idle = void(__fastcall*)(void*, void*, std::int32_t) noexcept;
    using Wander = std::int32_t(__fastcall*)(
        void*, void*, std::uint8_t) noexcept;
    using Attack = std::int32_t(__fastcall*)(
        void*, void*, std::int32_t, void*) noexcept;
    using Chase = std::int32_t(__fastcall*)(
        void*, void*, void*, std::uint16_t) noexcept;
    using Retreat = std::int32_t(__fastcall*)(
        void*, void*, void*, std::uint8_t, std::int32_t) noexcept;
    using Cast = std::int32_t(__fastcall*)(
        void*, void*, std::uint8_t, std::int32_t, void*,
        std::int32_t, std::int32_t, std::uint8_t) noexcept;

    GetUnitValue getClassId{};
    GetUnitValue getUnitId{};
    GetUnitValue getUnitType{};
    GetUnitValue isUnitDead{};
    GetServerUnit getServerUnit{};
    Distance distance{};
    Idle idle{};
    Wander wander{};
    Attack attack{};
    Chase chase{};
    Retreat retreat{};
    Cast cast{};
};

[[nodiscard]] auto ResolveD2NativeFunctions(
    std::uintptr_t moduleBase) noexcept -> D2NativeFunctions;

using ThreadIdProvider = std::uint32_t(*)() noexcept;

class D2NativeActionAdapter final : public NativeActionAdapter {
public:
    D2NativeActionAdapter(
        D2NativeFunctions functions,
        std::uint64_t authoritativeSession,
        std::uint32_t authoritativeThread,
        std::uint32_t skillRowCount,
        const void* monStats,
        ThreadIdProvider threadIdProvider) noexcept;

    [[nodiscard]] auto IsAuthoritativeContext(
        const NativeThinkContext& context) noexcept -> bool override;
    [[nodiscard]] auto IsValidMonster(
        const NativeThinkContext& context) noexcept -> bool override;
    [[nodiscard]] auto IsValidTarget(
        const NativeThinkContext& context) noexcept -> bool override;
    [[nodiscard]] auto IsValidOwner(
        const NativeThinkContext& context) noexcept -> bool override;
    [[nodiscard]] auto IsValidMode(
        const NativeThinkContext& context,
        ActionKind action) noexcept -> bool override;
    [[nodiscard]] auto IsValidSkill(
        const NativeThinkContext& context,
        std::uint16_t skillId) noexcept -> bool override;

    [[nodiscard]] auto TryIdle(
        const NativeThinkContext& context,
        std::uint8_t frames) noexcept -> NativeCallResult override;
    [[nodiscard]] auto TryWander(
        const NativeThinkContext& context,
        std::uint8_t radius) noexcept -> NativeCallResult override;
    [[nodiscard]] auto TryAttackTarget(
        const NativeThinkContext& context) noexcept
        -> NativeCallResult override;
    [[nodiscard]] auto TryChaseTarget(
        const NativeThinkContext& context) noexcept
        -> NativeCallResult override;
    [[nodiscard]] auto TryRetreatFromTarget(
        const NativeThinkContext& context,
        std::uint8_t distance) noexcept -> NativeCallResult override;
    [[nodiscard]] auto TryCastOnTarget(
        const NativeThinkContext& context,
        std::uint16_t skillId) noexcept -> NativeCallResult override;
private:
    D2NativeFunctions functions_{};
    std::uint64_t authoritativeSession_{};
    std::uint32_t authoritativeThread_{};
    std::uint32_t skillRowCount_{};
    const std::byte* monStats_{};
    std::uint16_t validatedSkillId_{};
    std::uint8_t validatedSkillMode_{};
    bool hasValidatedSkill_{};
    ThreadIdProvider threadIdProvider_{};
};

static_assert(offsetof(D2AiTickParam, aiControl) == 0x00U);
static_assert(offsetof(D2AiTickParam, target) == 0x10U);
static_assert(offsetof(D2AiTickParam, targetDistance) == 0x20U);
static_assert(offsetof(D2AiTickParam, inCombat) == 0x24U);
static_assert(offsetof(D2AiTickParam, monStats) == 0x28U);
static_assert(offsetof(D2AiTickParam, monStats2) == 0x30U);
static_assert(sizeof(D2AiTickParam) == 0x38U);
static_assert(offsetof(D2AiTableRecord, initialize) == 0x08U);
static_assert(offsetof(D2AiTableRecord, think) == 0x10U);
static_assert(offsetof(D2AiTableRecord, transition) == 0x18U);
static_assert(sizeof(D2AiTableRecord) == 0x20U);

} // namespace ruffneckk::scripted_ai
