#pragma once

#include "scripted_ai_bridge.hpp"

#include <cstdint>
#include <string>

namespace ruffneckk::scripted_ai {

inline constexpr std::uint8_t NativeFallbackIdleFrames = 10U;

struct NativeThinkContext {
    void* game{};
    void* unit{};
    void* target{};
    void* owner{};
    std::uint64_t sessionGeneration{};
    std::uint64_t thinkToken{};
    std::uint32_t monStatsId{};
    std::int32_t targetDistance{};
    std::int32_t ownerDistance{};
    std::int32_t targetOwnerDistance{};
    TacticalProfile tacticalProfile{TacticalProfile::None};
    bool hasLastAction{};
    ActionKind lastAction{ActionKind::Idle};
    bool hasPreferredSkill{};
    std::uint16_t preferredSkill{};
    bool inCombat{};
};

enum class NativeCallResult : std::uint8_t {
    Rejected,
    Accepted,
    FallbackScheduled,
    Error,
};

class NativeActionAdapter {
public:
    virtual ~NativeActionAdapter() noexcept = default;

    [[nodiscard]] virtual auto IsAuthoritativeContext(
        const NativeThinkContext& context) noexcept -> bool = 0;
    [[nodiscard]] virtual auto IsValidMonster(
        const NativeThinkContext& context) noexcept -> bool = 0;
    [[nodiscard]] virtual auto IsValidTarget(
        const NativeThinkContext& context) noexcept -> bool = 0;
    [[nodiscard]] virtual auto IsValidOwner(
        const NativeThinkContext& context) noexcept -> bool = 0;
    [[nodiscard]] virtual auto IsValidMode(
        const NativeThinkContext& context,
        ActionKind action) noexcept -> bool = 0;
    [[nodiscard]] virtual auto IsValidSkill(
        const NativeThinkContext& context,
        std::uint16_t skillId) noexcept -> bool = 0;

    [[nodiscard]] virtual auto TryIdle(
        const NativeThinkContext& context,
        std::uint8_t frames) noexcept -> NativeCallResult = 0;
    [[nodiscard]] virtual auto TryWander(
        const NativeThinkContext& context,
        std::uint8_t radius) noexcept -> NativeCallResult = 0;
    [[nodiscard]] virtual auto TryAttackTarget(
        const NativeThinkContext& context) noexcept -> NativeCallResult = 0;
    [[nodiscard]] virtual auto TryChaseTarget(
        const NativeThinkContext& context) noexcept -> NativeCallResult = 0;
    [[nodiscard]] virtual auto TryRetreatFromTarget(
        const NativeThinkContext& context,
        std::uint8_t distance) noexcept -> NativeCallResult = 0;
    [[nodiscard]] virtual auto TryCastOnTarget(
        const NativeThinkContext& context,
        std::uint16_t skillId) noexcept -> NativeCallResult = 0;
};

enum class PreCallbackRoute : std::uint8_t {
    DelegateOriginal,
    ScriptedBridge,
    StockFallback,
};

struct PreCallbackDecision {
    PreCallbackRoute route{PreCallbackRoute::DelegateOriginal};
    std::uint16_t fallbackAi{};
};

[[nodiscard]] auto SelectPreCallbackRoute(
    std::int32_t specialState,
    bool generationMatches,
    const BindingRuntimeState& binding) noexcept -> PreCallbackDecision;

enum class NativeContinuation : std::uint8_t {
    ActionPipeline,
    CapabilityFallbackScheduled,
    FallbackIdleScheduled,
    FallbackIdleFailed,
    InvalidContext,
};

struct NativeThinkExecution {
    ThinkDecision decision;
    NativeContinuation continuation{NativeContinuation::InvalidContext};
    NativeCallResult fallbackResult{NativeCallResult::Error};
    std::uint32_t adapterCalls{};
    bool fallbackAttempted{};
};

enum class RevivePolicyContinuation : std::uint8_t {
    DelegateOriginal,
    RequestNativeFollow,
    InvalidContext,
};

struct RevivePolicyExecution {
    ThinkDecision decision;
    RevivePolicyContinuation continuation{
        RevivePolicyContinuation::InvalidContext};
    std::uint32_t policyRequests{};
};

enum class ReviveTacticalContinuation : std::uint8_t {
    DelegateOriginal,
    Handled,
    InvalidContext,
};

struct ReviveTacticalExecution {
    ThinkDecision decision;
    ReviveTacticalContinuation continuation{
        ReviveTacticalContinuation::InvalidContext};
    ActionIntent attemptedAction{};
    std::uint32_t adapterCalls{};
};

[[nodiscard]] auto ExecuteNativeThink(
    const SessionGeneration& generation,
    ScriptBank bank,
    const NativeThinkContext& context,
    NativeActionAdapter& adapter,
    ThinkTiming timing,
    std::string& error) -> NativeThinkExecution;

[[nodiscard]] auto EvaluateRevivePolicyThink(
    const SessionGeneration& generation,
    const NativeThinkContext& context,
    ThinkTiming timing,
    std::string& error) -> RevivePolicyExecution;

[[nodiscard]] auto ExecuteReviveTacticalThink(
    const SessionGeneration& generation,
    const NativeThinkContext& context,
    NativeActionAdapter& adapter,
    ThinkTiming timing,
    std::string& error) -> ReviveTacticalExecution;

} // namespace ruffneckk::scripted_ai
