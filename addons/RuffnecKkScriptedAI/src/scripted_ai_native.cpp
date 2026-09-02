#include "scripted_ai_native.hpp"

#include <limits>

namespace ruffneckk::scripted_ai {
namespace {

[[nodiscard]] constexpr auto RequiresTarget(ActionKind action) noexcept
        -> bool {
    switch (action) {
    case ActionKind::AttackTarget:
    case ActionKind::ChaseTarget:
    case ActionKind::RetreatFromTarget:
    case ActionKind::CastOnTarget:
        return true;
    case ActionKind::Idle:
    case ActionKind::Wander:
        return false;
    }
    return true;
}

[[nodiscard]] constexpr auto MapResult(NativeCallResult result) noexcept
        -> CapabilityResult {
    switch (result) {
    case NativeCallResult::Rejected:
        return CapabilityResult::Rejected;
    case NativeCallResult::Accepted:
        return CapabilityResult::Accepted;
    case NativeCallResult::FallbackScheduled:
        return CapabilityResult::FallbackScheduled;
    case NativeCallResult::Error:
        return CapabilityResult::Error;
    }
    return CapabilityResult::Error;
}

class NativeThinkCapabilities final : public ThinkCapabilities {
public:
    NativeThinkCapabilities(
            const NativeThinkContext& context,
            NativeActionAdapter& adapter) noexcept
        : context_(context), adapter_(adapter) {}

    [[nodiscard]] auto TryAction(
            const ActionIntent& intent) noexcept -> CapabilityResult override {
        if (!IsValidActionIntent(intent)) return CapabilityResult::Error;
        if (!adapter_.IsValidMode(context_, intent.kind)) {
            return CapabilityResult::Error;
        }
        if (RequiresTarget(intent.kind)) {
            if (context_.target == nullptr
                    || !adapter_.IsValidTarget(context_)) {
                return CapabilityResult::Rejected;
            }
        }
        if (intent.kind == ActionKind::CastOnTarget
                && !adapter_.IsValidSkill(
                    context_,
                    static_cast<std::uint16_t>(intent.argument))) {
            return CapabilityResult::Rejected;
        }

        NativeCallResult result{NativeCallResult::Error};
        switch (intent.kind) {
        case ActionKind::Idle:
            idleAttempted_ = true;
            result = adapter_.TryIdle(
                context_,
                static_cast<std::uint8_t>(intent.argument));
            idleResult_ = result;
            break;
        case ActionKind::Wander:
            result = adapter_.TryWander(
                context_,
                static_cast<std::uint8_t>(intent.argument));
            break;
        case ActionKind::AttackTarget:
            result = adapter_.TryAttackTarget(context_);
            break;
        case ActionKind::ChaseTarget:
            result = adapter_.TryChaseTarget(context_);
            break;
        case ActionKind::RetreatFromTarget:
            result = adapter_.TryRetreatFromTarget(
                context_,
                static_cast<std::uint8_t>(intent.argument));
            break;
        case ActionKind::CastOnTarget:
            result = adapter_.TryCastOnTarget(
                context_,
                static_cast<std::uint16_t>(intent.argument));
            break;
        }
        if (adapterCalls_ < std::numeric_limits<std::uint32_t>::max()) {
            ++adapterCalls_;
        }
        return MapResult(result);
    }

    [[nodiscard]] auto ScheduleFallbackIdle() noexcept -> NativeCallResult {
        if (idleAttempted_) return idleResult_;
        idleAttempted_ = true;
        if (!adapter_.IsValidMode(context_, ActionKind::Idle)) {
            idleResult_ = NativeCallResult::Error;
            return idleResult_;
        }
        idleResult_ = adapter_.TryIdle(context_, NativeFallbackIdleFrames);
        if (adapterCalls_ < std::numeric_limits<std::uint32_t>::max()) {
            ++adapterCalls_;
        }
        return idleResult_;
    }

    [[nodiscard]] auto IdleAttempted() const noexcept -> bool {
        return idleAttempted_;
    }

    [[nodiscard]] auto IdleResult() const noexcept -> NativeCallResult {
        return idleResult_;
    }

    [[nodiscard]] auto AdapterCalls() const noexcept -> std::uint32_t {
        return adapterCalls_;
    }

private:
    const NativeThinkContext& context_;
    NativeActionAdapter& adapter_;
    NativeCallResult idleResult_{NativeCallResult::Error};
    std::uint32_t adapterCalls_{};
    bool idleAttempted_{};
};

} // namespace

auto SelectPreCallbackRoute(
        std::int32_t specialState,
        bool generationMatches,
        const BindingRuntimeState& binding) noexcept -> PreCallbackDecision {
    if (specialState != 0 || !binding.bound
            || binding.fallbackAi >= StockAiCount) {
        return {};
    }
    if (generationMatches && binding.scriptReady && !binding.quarantined) {
        return {
            .route = PreCallbackRoute::ScriptedBridge,
            .fallbackAi = binding.fallbackAi,
        };
    }
    return {
        .route = PreCallbackRoute::StockFallback,
        .fallbackAi = binding.fallbackAi,
    };
}

auto ExecuteNativeThink(
        const SessionGeneration& generation,
        ScriptBank bank,
        const NativeThinkContext& context,
        NativeActionAdapter& adapter,
        ThinkTiming timing,
        std::string& error) -> NativeThinkExecution {
    NativeThinkExecution execution{};
    if (context.game == nullptr || context.unit == nullptr
            || context.sessionGeneration == 0U || context.thinkToken == 0U
            || (context.target != nullptr && context.targetDistance < 0)
            || !adapter.IsAuthoritativeContext(context)
            || !adapter.IsValidMonster(context)) {
        execution.decision.fallbackReason =
            FallbackReason::InvalidNativeContext;
        error = "native think context failed authority or unit validation";
        return execution;
    }

    NativeThinkCapabilities capabilities(context, adapter);
    execution.decision = generation.EvaluateThink(
        bank,
        context.monStatsId,
        context.sessionGeneration,
        context.thinkToken,
        {
            .hasTarget = context.target != nullptr,
            .inCombat = context.inCombat,
            .targetDistance = context.targetDistance > 0
                ? static_cast<std::uint32_t>(context.targetDistance)
                : 0U,
        },
        capabilities,
        timing,
        error);

    if (execution.decision.disposition == ThinkDisposition::Action) {
        execution.continuation = NativeContinuation::ActionPipeline;
        execution.adapterCalls = capabilities.AdapterCalls();
        return execution;
    }
    if (execution.decision.fallbackScheduled) {
        execution.continuation =
            NativeContinuation::CapabilityFallbackScheduled;
        execution.fallbackResult = NativeCallResult::FallbackScheduled;
        execution.fallbackAttempted = true;
        execution.adapterCalls = capabilities.AdapterCalls();
        return execution;
    }

    execution.fallbackAttempted = true;
    execution.fallbackResult = capabilities.IdleAttempted()
        ? capabilities.IdleResult()
        : capabilities.ScheduleFallbackIdle();
    execution.adapterCalls = capabilities.AdapterCalls();
    execution.continuation = execution.fallbackResult
            == NativeCallResult::Accepted
            || execution.fallbackResult
                == NativeCallResult::FallbackScheduled
        ? NativeContinuation::FallbackIdleScheduled
        : NativeContinuation::FallbackIdleFailed;
    return execution;
}

} // namespace ruffneckk::scripted_ai
