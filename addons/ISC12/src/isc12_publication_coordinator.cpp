#include "isc12_publication_coordinator.hpp"

#include <array>

namespace ruffneckk::isc12 {
namespace {

[[nodiscard]] auto HasCompleteCallbackSet(
        const PublicationCoordinatorCallbacks& callbacks) noexcept -> bool {
    return callbacks.preflightG0
        && callbacks.preflightG10
        && callbacks.preflightCodec
        && callbacks.reserveProcessLifetime
        && callbacks.commitG0
        && callbacks.commitG10
        && callbacks.commitCodec
        && callbacks.publishReadiness
        && callbacks.markPoisoned;
}

} // namespace

auto PublicationCoordinator::Publish(
        const NativePublicationQuiescenceLease& quiescence,
        const PublicationCoordinatorCallbacks& callbacks) noexcept
        -> PublicationCoordinatorStatus {
    if (attemptInProgress_) {
        return PublicationCoordinatorStatus::RejectedBeforeMutation;
    }
    if (state_ == PublicationCoordinatorState::Active) {
        return PublicationCoordinatorStatus::Active;
    }
    if (state_ == PublicationCoordinatorState::Reserved) {
        return PublicationCoordinatorStatus::ReservedWithoutMutation;
    }
    if (state_ == PublicationCoordinatorState::Poisoned) {
        return PublicationCoordinatorStatus::Poisoned;
    }
    if (!quiescence.IsHeld()) {
        return PublicationCoordinatorStatus::QuiescenceRequired;
    }
    if (!HasCompleteCallbackSet(callbacks)) {
        return PublicationCoordinatorStatus::RejectedBeforeMutation;
    }

    attemptInProgress_ = true;
    const auto finishFresh = [this](
            PublicationCoordinatorStatus status) noexcept {
        attemptInProgress_ = false;
        return status;
    };

    // The fixed array is the canonical preflight order. No reservation or
    // native write callback is reachable until all three stages pass.
    const std::array<PreflightPublicationStageFn, 3> preflights{
        callbacks.preflightG0,
        callbacks.preflightG10,
        callbacks.preflightCodec,
    };
    for (const auto preflight : preflights) {
        if (!quiescence.IsHeld()) {
            return finishFresh(
                PublicationCoordinatorStatus::QuiescenceRequired);
        }
        const bool accepted = preflight(callbacks.context);
        if (!quiescence.IsHeld()) {
            return finishFresh(
                PublicationCoordinatorStatus::QuiescenceRequired);
        }
        if (!accepted) {
            return finishFresh(
                PublicationCoordinatorStatus::RejectedBeforeMutation);
        }
    }
    if (!quiescence.IsHeld()) {
        return finishFresh(PublicationCoordinatorStatus::QuiescenceRequired);
    }

    // Enter the terminal reserved phase before invoking the callback. This
    // rejects re-entrance and guarantees that the reservation is attempted at
    // most once. From here onward the prepared relay/state lifetime can never
    // be released before process exit, even when no native byte was changed.
    state_ = PublicationCoordinatorState::Reserved;
    processLifetimeReserved_ = true;
    callbacks.reserveProcessLifetime(callbacks.context);

    const auto finishReservedWithoutMutation = [this]() noexcept {
        attemptInProgress_ = false;
        readinessPublished_ = false;
        return PublicationCoordinatorStatus::ReservedWithoutMutation;
    };
    const auto poison = [this, &callbacks]() noexcept {
        state_ = PublicationCoordinatorState::Poisoned;
        attemptInProgress_ = false;
        readinessPublished_ = false;
        callbacks.markPoisoned(callbacks.context);
        return PublicationCoordinatorStatus::Poisoned;
    };

    if (!quiescence.IsHeld()) {
        return finishReservedWithoutMutation();
    }

    const std::array<CommitPublicationStageFn, 3> commits{
        callbacks.commitG0,
        callbacks.commitG10,
        callbacks.commitCodec,
    };
    std::size_t committedStages = 0U;
    for (const auto commit : commits) {
        if (!quiescence.IsHeld()) {
            return committedStages == 0U
                ? finishReservedWithoutMutation()
                : poison();
        }

        const auto outcome = commit(callbacks.context);
        if (outcome == PublicationCommitOutcome::Uncertain) {
            return poison();
        }
        if (outcome
                == PublicationCommitOutcome::RejectedWithoutMutation) {
            return committedStages == 0U
                ? finishReservedWithoutMutation()
                : poison();
        }
        if (outcome != PublicationCommitOutcome::Committed) {
            return poison();
        }

        ++committedStages;
        if (!quiescence.IsHeld()) {
            return poison();
        }
    }

    if (!quiescence.IsHeld()) {
        return poison();
    }

    // Readiness is the sole final publication step. The callback contract is
    // intentionally infallible; introducing a fallible readiness write would
    // make a zero-readiness failure guarantee impossible after native writes.
    callbacks.publishReadiness(callbacks.context);
    if (!quiescence.IsHeld()) {
        return poison();
    }
    readinessPublished_ = true;
    state_ = PublicationCoordinatorState::Active;
    attemptInProgress_ = false;
    return PublicationCoordinatorStatus::Active;
}

} // namespace ruffneckk::isc12
