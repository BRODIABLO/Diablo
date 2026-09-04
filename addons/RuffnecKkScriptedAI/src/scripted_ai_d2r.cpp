#include "scripted_ai_d2r.hpp"

#include <cstring>
#include <limits>

namespace ruffneckk::scripted_ai {
namespace {

template <typename Function>
[[nodiscard]] auto At(
        std::uintptr_t moduleBase,
        std::uintptr_t rva) noexcept -> Function {
    if (moduleBase == 0U
            || rva > std::numeric_limits<std::uintptr_t>::max()
                - moduleBase) {
        return nullptr;
    }
    return reinterpret_cast<Function>(moduleBase + rva);
}

[[nodiscard]] constexpr auto KnownAction(ActionKind action) noexcept -> bool {
    switch (action) {
    case ActionKind::Idle:
    case ActionKind::Wander:
    case ActionKind::AttackTarget:
    case ActionKind::ChaseTarget:
    case ActionKind::RetreatFromTarget:
    case ActionKind::CastOnTarget:
    case ActionKind::FollowOwner:
        return true;
    }
    return false;
}

[[nodiscard]] constexpr auto ResultFromAccepted(
        std::int32_t accepted) noexcept -> NativeCallResult {
    return accepted != 0
        ? NativeCallResult::Accepted
        : NativeCallResult::Rejected;
}

} // namespace

auto ReadUnitMonStatsRecord(const void* unit) noexcept -> const void* {
    if (unit == nullptr) return nullptr;
    const auto unitAddress = reinterpret_cast<std::uintptr_t>(unit);
    if (unitAddress > std::numeric_limits<std::uintptr_t>::max()
            - UnitMonsterDataOffset) {
        return nullptr;
    }
    const auto* const monsterData = *reinterpret_cast<void* const*>(
        unitAddress + UnitMonsterDataOffset);
    if (monsterData == nullptr) return nullptr;
    return *static_cast<void* const*>(monsterData);
}

auto ClassifyMonStatsRecord(
        const void* monStats,
        std::uint32_t classId,
        std::span<const NativeTableBankView> banks) noexcept
        -> ResolvedNativeBank {
    if (monStats == nullptr) return {};
    const auto recordAddress = reinterpret_cast<std::uintptr_t>(monStats);
    for (const auto& bank : banks) {
        if (bank.rows == nullptr || bank.rowSize == 0U
                || classId >= bank.rowCount) {
            continue;
        }
        const auto rowsAddress = reinterpret_cast<std::uintptr_t>(bank.rows);
        const auto offset = static_cast<std::uint64_t>(classId)
            * bank.rowSize;
        if (offset > std::numeric_limits<std::uintptr_t>::max()
                - rowsAddress) {
            continue;
        }
        if (recordAddress == rowsAddress
                + static_cast<std::uintptr_t>(offset)) {
            return {
                .scriptBank = bank.scriptBank,
                .skillRowCount = bank.skillRowCount,
                .monStats = monStats,
                .found = true,
            };
        }
    }
    return {};
}

auto StockAiRecord(
        std::uintptr_t moduleBase,
        std::uint16_t aiIndex) noexcept -> const D2AiTableRecord* {
    if (moduleBase == 0U || aiIndex >= StockAiCount) return nullptr;
    constexpr auto recordSize = sizeof(D2AiTableRecord);
    const auto offset = static_cast<std::uintptr_t>(aiIndex) * recordSize;
    if (NormalAiTableRva > std::numeric_limits<std::uintptr_t>::max()
            - offset
            || moduleBase > std::numeric_limits<std::uintptr_t>::max()
                - (NormalAiTableRva + offset)) {
        return nullptr;
    }
    return reinterpret_cast<const D2AiTableRecord*>(
        moduleBase + NormalAiTableRva + offset);
}

auto SelectReviveTacticalLoadout(
        const void* monStats,
        std::uint32_t skillRowCount,
        std::uint8_t startingSlot) noexcept -> ReviveTacticalLoadout {
    ReviveTacticalLoadout loadout{};
    if (monStats == nullptr || startingSlot >= MonStatsSkillSlotCount) {
        return loadout;
    }
    const auto* const bytes = static_cast<const std::byte*>(monStats);
    std::uint32_t flags{};
    std::memcpy(&flags, bytes + MonStatsFlagsOffset, sizeof(flags));
    loadout.profile = (flags & MonStatsIsMeleeFlag) != 0U
        ? TacticalProfile::MeleeVanguard
        : TacticalProfile::RangedSkirmisher;

    for (std::size_t offset{}; offset < MonStatsSkillSlotCount; ++offset) {
        const auto slot = static_cast<std::uint8_t>(
            (startingSlot + offset) % MonStatsSkillSlotCount);
        std::uint16_t skill{};
        std::int16_t mode{};
        std::memcpy(
            &skill,
            bytes + MonStatsSkillIdsOffset + slot * sizeof(skill),
            sizeof(skill));
        std::memcpy(
            &mode,
            bytes + MonStatsSkillModesOffset + slot * sizeof(mode),
            sizeof(mode));
        if (skill == 0U || skill >= skillRowCount
                || mode < MonsterCastMode || mode >= MonsterModeCount) {
            continue;
        }
        loadout.profile = TacticalProfile::CasterArtillery;
        loadout.preferredSkill = skill;
        loadout.preferredSlot = slot;
        loadout.hasPreferredSkill = true;
        break;
    }
    return loadout;
}

auto ResolveD2NativeFunctions(
        std::uintptr_t moduleBase) noexcept -> D2NativeFunctions {
    return {
        .getClassId = At<D2NativeFunctions::GetUnitValue>(
            moduleBase, GetClassIdRva),
        .getUnitId = At<D2NativeFunctions::GetUnitValue>(
            moduleBase, GetUnitIdRva),
        .getUnitType = At<D2NativeFunctions::GetUnitValue>(
            moduleBase, GetUnitTypeRva),
        .isUnitDead = At<D2NativeFunctions::GetUnitValue>(
            moduleBase, IsUnitDeadRva),
        .getServerUnit = At<D2NativeFunctions::GetServerUnit>(
            moduleBase, GetServerUnitRva),
        .distance = At<D2NativeFunctions::Distance>(
            moduleBase, UnitDistanceRva),
        .idle = At<D2NativeFunctions::Idle>(
            moduleBase, IdleInNeutralModeRva),
        .wander = At<D2NativeFunctions::Wander>(
            moduleBase, WalkCloseToUnitRva),
        .attack = At<D2NativeFunctions::Attack>(
            moduleBase, ChangeModeAndTargetUnitRva),
        .chase = At<D2NativeFunctions::Chase>(
            moduleBase, WalkToTargetUnitRva),
        .retreat = At<D2NativeFunctions::Retreat>(
            moduleBase, EscapeRva),
        .cast = At<D2NativeFunctions::Cast>(moduleBase, UseSkillRva),
    };
}

D2NativeActionAdapter::D2NativeActionAdapter(
        D2NativeFunctions functions,
        std::uint64_t authoritativeSession,
        std::uint32_t authoritativeThread,
        std::uint32_t skillRowCount,
        const void* monStats,
        ThreadIdProvider threadIdProvider) noexcept
    : functions_(functions),
      authoritativeSession_(authoritativeSession),
      authoritativeThread_(authoritativeThread),
      skillRowCount_(skillRowCount),
      monStats_(static_cast<const std::byte*>(monStats)),
      threadIdProvider_(threadIdProvider) {}

auto D2NativeActionAdapter::IsAuthoritativeContext(
        const NativeThinkContext& context) noexcept -> bool {
    return authoritativeSession_ != 0U
        && context.sessionGeneration == authoritativeSession_
        && authoritativeThread_ != 0U
        && threadIdProvider_ != nullptr
        && threadIdProvider_() == authoritativeThread_;
}

auto D2NativeActionAdapter::IsValidMonster(
        const NativeThinkContext& context) noexcept -> bool {
    return context.unit != nullptr && monStats_ != nullptr
        && ReadUnitMonStatsRecord(context.unit) == monStats_
        && functions_.getUnitType != nullptr
        && functions_.getClassId != nullptr
        && functions_.getUnitType(context.unit)
            == static_cast<std::int32_t>(MonsterUnitType)
        && functions_.getClassId(context.unit)
            == static_cast<std::int32_t>(context.monStatsId);
}

auto D2NativeActionAdapter::IsValidTarget(
        const NativeThinkContext& context) noexcept -> bool {
    if (context.target == nullptr || context.target == context.unit
            || functions_.getUnitType == nullptr
            || functions_.isUnitDead == nullptr
            || functions_.isUnitDead(context.target) != 0) {
        return false;
    }
    const auto type = functions_.getUnitType(context.target);
    return type >= 0
        && type <= static_cast<std::int32_t>(MaximumUnitType);
}

auto D2NativeActionAdapter::IsValidOwner(
        const NativeThinkContext& context) noexcept -> bool {
    if (context.owner == nullptr || context.owner == context.unit
            || functions_.getUnitType == nullptr
            || functions_.isUnitDead == nullptr
            || functions_.isUnitDead(context.owner) != 0) {
        return false;
    }
    const auto type = functions_.getUnitType(context.owner);
    return type >= 0
        && type <= static_cast<std::int32_t>(MaximumUnitType);
}

auto D2NativeActionAdapter::IsValidMode(
        const NativeThinkContext&,
        ActionKind action) noexcept -> bool {
    return KnownAction(action)
        && (action != ActionKind::AttackTarget
            || MonsterAttack1Mode < MonsterModeCount);
}

auto D2NativeActionAdapter::IsValidSkill(
        const NativeThinkContext& context,
        std::uint16_t skillId) noexcept -> bool {
    hasValidatedSkill_ = false;
    if (skillRowCount_ == 0U || skillId >= skillRowCount_
            || monStats_ == nullptr
            || ReadUnitMonStatsRecord(context.unit) != monStats_) {
        return false;
    }
    for (std::size_t slot{}; slot < MonStatsSkillSlotCount; ++slot) {
        std::uint16_t candidate{};
        std::int16_t mode{};
        std::memcpy(
            &candidate,
            monStats_ + MonStatsSkillIdsOffset
                + slot * sizeof(candidate),
            sizeof(candidate));
        if (candidate != skillId) continue;
        std::memcpy(
            &mode,
            monStats_ + MonStatsSkillModesOffset
                + slot * sizeof(mode),
            sizeof(mode));
        if (mode < 0 || mode >= MonsterModeCount) return false;
        validatedSkillId_ = skillId;
        validatedSkillMode_ = static_cast<std::uint8_t>(mode);
        hasValidatedSkill_ = true;
        return true;
    }
    return false;
}

auto D2NativeActionAdapter::TryIdle(
        const NativeThinkContext& context,
        std::uint8_t frames) noexcept -> NativeCallResult {
    if (functions_.idle == nullptr || context.game == nullptr
            || context.unit == nullptr || frames == 0U) {
        return NativeCallResult::Error;
    }
    functions_.idle(context.game, context.unit, frames);
    return NativeCallResult::Accepted;
}

auto D2NativeActionAdapter::TryWander(
        const NativeThinkContext& context,
        std::uint8_t radius) noexcept -> NativeCallResult {
    if (functions_.wander == nullptr || radius == 0U) {
        return NativeCallResult::Error;
    }
    return ResultFromAccepted(
        functions_.wander(context.game, context.unit, radius));
}

auto D2NativeActionAdapter::TryAttackTarget(
        const NativeThinkContext& context) noexcept -> NativeCallResult {
    if (functions_.attack == nullptr) return NativeCallResult::Error;
    return ResultFromAccepted(functions_.attack(
        context.game,
        context.unit,
        MonsterAttack1Mode,
        context.target));
}

auto D2NativeActionAdapter::TryChaseTarget(
        const NativeThinkContext& context) noexcept -> NativeCallResult {
    if (functions_.chase == nullptr) return NativeCallResult::Error;
    return ResultFromAccepted(functions_.chase(
        context.game,
        context.unit,
        context.target,
        0U));
}

auto D2NativeActionAdapter::TryRetreatFromTarget(
        const NativeThinkContext& context,
        std::uint8_t distance) noexcept -> NativeCallResult {
    if (functions_.retreat == nullptr || distance == 0U) {
        return NativeCallResult::Error;
    }
    return ResultFromAccepted(functions_.retreat(
        context.game,
        context.unit,
        context.target,
        distance,
        1));
}

auto D2NativeActionAdapter::TryCastOnTarget(
        const NativeThinkContext& context,
        std::uint16_t skillId) noexcept -> NativeCallResult {
    if (functions_.cast == nullptr || !hasValidatedSkill_
            || validatedSkillId_ != skillId) {
        return NativeCallResult::Error;
    }
    hasValidatedSkill_ = false;
    return functions_.cast(
        context.game,
        context.unit,
        validatedSkillMode_,
        skillId,
        context.target,
        0,
        0,
        0U) != 0
        ? NativeCallResult::Accepted
        : NativeCallResult::FallbackScheduled;
}

} // namespace ruffneckk::scripted_ai
