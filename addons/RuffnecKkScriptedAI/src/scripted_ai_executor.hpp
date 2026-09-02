#pragma once

#include <cstdint>

namespace ruffneckk::scripted_ai {

enum class ActionKind : std::uint8_t {
    Idle,
    Wander,
    AttackTarget,
    ChaseTarget,
    RetreatFromTarget,
    CastOnTarget,
};

struct ActionIntent {
    ActionKind kind{ActionKind::Idle};
    std::uint32_t argument{};

    [[nodiscard]] friend constexpr auto operator==(
        const ActionIntent&,
        const ActionIntent&) noexcept -> bool = default;
};

enum class CapabilityResult : std::uint8_t {
    Rejected,
    Accepted,
    FallbackScheduled,
    Error,
};

class ThinkCapabilities {
public:
    virtual ~ThinkCapabilities() noexcept = default;

    [[nodiscard]] virtual auto TryAction(
        const ActionIntent& intent) noexcept -> CapabilityResult = 0;
};

struct ThinkSnapshot {
    bool hasTarget{};
    bool inCombat{};
    std::uint32_t targetDistance{};
};

using MicrosecondClock = std::uint64_t (*)(void* userData) noexcept;

struct ThinkTiming {
    MicrosecondClock now{};
    void* userData{};
};

[[nodiscard]] auto ReadMicroseconds(const ThinkTiming& timing) noexcept
    -> std::uint64_t;

[[nodiscard]] constexpr auto IsValidActionIntent(
        const ActionIntent& intent) noexcept -> bool {
    switch (intent.kind) {
    case ActionKind::Idle:
    case ActionKind::Wander:
    case ActionKind::RetreatFromTarget:
        return intent.argument >= 1U && intent.argument <= 255U;
    case ActionKind::AttackTarget:
    case ActionKind::ChaseTarget:
        return intent.argument == 0U;
    case ActionKind::CastOnTarget:
        return intent.argument <= 65'535U;
    }
    return false;
}

class EphemeralThinkHandle final {
public:
    EphemeralThinkHandle(
        std::uint64_t sessionGeneration,
        std::uint64_t thinkToken,
        ThinkSnapshot snapshot,
        ThinkCapabilities& capabilities,
        ThinkTiming timing = {}) noexcept;

    EphemeralThinkHandle(const EphemeralThinkHandle&) = delete;
    auto operator=(const EphemeralThinkHandle&)
        -> EphemeralThinkHandle& = delete;
    EphemeralThinkHandle(EphemeralThinkHandle&&) = delete;
    auto operator=(EphemeralThinkHandle&&) -> EphemeralThinkHandle& = delete;
    ~EphemeralThinkHandle() noexcept;

    [[nodiscard]] auto SessionGeneration() const noexcept -> std::uint64_t;
    [[nodiscard]] auto ThinkToken() const noexcept -> std::uint64_t;
    [[nodiscard]] auto Snapshot() const noexcept -> ThinkSnapshot;
    [[nodiscard]] auto IsValid() const noexcept -> bool;

    [[nodiscard]] auto TryAction(const ActionIntent& intent) noexcept
        -> CapabilityResult;
    void Invalidate() noexcept;

    [[nodiscard]] auto HasCommittedAction() const noexcept -> bool;
    [[nodiscard]] auto CommittedAction() const noexcept -> ActionIntent;
    [[nodiscard]] auto HasScheduledFallback() const noexcept -> bool;
    [[nodiscard]] auto ActionAttempts() const noexcept -> std::uint32_t;
    [[nodiscard]] auto CapabilityMicroseconds() const noexcept
        -> std::uint64_t;
    [[nodiscard]] auto HadCapabilityError() const noexcept -> bool;
    [[nodiscard]] auto HadStaleAccess() const noexcept -> bool;
    [[nodiscard]] auto HadSecondActionAttempt() const noexcept -> bool;

private:
    std::uint64_t sessionGeneration_{};
    std::uint64_t thinkToken_{};
    ThinkSnapshot snapshot_{};
    ThinkCapabilities* capabilities_{};
    ThinkTiming timing_{};
    bool valid_{true};
    bool committed_{};
    bool fallbackScheduled_{};
    bool capabilityError_{};
    bool staleAccess_{};
    bool secondActionAttempt_{};
    ActionIntent committedAction_{};
    std::uint32_t actionAttempts_{};
    std::uint64_t capabilityMicroseconds_{};
};

enum class ThinkDisposition : std::uint8_t {
    Action,
    Fallback,
};

enum class FallbackReason : std::uint8_t {
    None,
    ExplicitFallback,
    NoAction,
    CapabilityRejected,
    CapabilityFallbackScheduled,
    CapabilityError,
    LuaError,
    InstructionBudget,
    AllocationBudget,
    StaleHandle,
    InvalidNativeContext,
    Quarantined,
    MissingBinding,
    InternalError,
};

struct ThinkDecision {
    ThinkDisposition disposition{ThinkDisposition::Fallback};
    FallbackReason fallbackReason{FallbackReason::InternalError};
    ActionIntent action{};
    std::uint16_t fallbackAi{};
    std::uint32_t scriptErrors{};
    std::uint32_t slowStrikes{};
    std::uint64_t luaMicroseconds{};
    bool enteredLua{};
    bool handleInvalidated{};
    bool fallbackScheduled{};
    bool quarantined{};
};

} // namespace ruffneckk::scripted_ai
