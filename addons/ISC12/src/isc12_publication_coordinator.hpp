#pragma once

#include "isc12_codec_patch.hpp"

#include <cstdint>

namespace ruffneckk::isc12 {

enum class PublicationCoordinatorStatus : std::uint8_t {
    Active,
    RejectedBeforeMutation,
    QuiescenceRequired,
    ReservedWithoutMutation,
    Poisoned,
};

enum class PublicationCoordinatorState : std::uint8_t {
    Fresh,
    Reserved,
    Active,
    Poisoned,
};

// A commit callback may report RejectedWithoutMutation only when it can prove
// that it did not attempt or publish any native mutation. Every ambiguous
// result is Uncertain and permanently poisons the coordinator.
enum class PublicationCommitOutcome : std::uint8_t {
    Committed,
    RejectedWithoutMutation,
    Uncertain,
};

using PreflightPublicationStageFn = bool (*)(void* context) noexcept;
using ReservePublicationLifetimeFn = void (*)(void* context) noexcept;
using CommitPublicationStageFn = PublicationCommitOutcome (*)(
    void* context) noexcept;
using PublishPublicationReadinessFn = void (*)(void* context) noexcept;
using MarkPublicationPoisonedFn = void (*)(void* context) noexcept;

struct PublicationCoordinatorCallbacks {
    void* context{};
    PreflightPublicationStageFn preflightG0{};
    PreflightPublicationStageFn preflightG10{};
    PreflightPublicationStageFn preflightCodec{};
    // Infallibly makes every prepared relay/state allocation process-bound.
    // It runs exactly once and strictly before the first commit callback.
    ReservePublicationLifetimeFn reserveProcessLifetime{};
    CommitPublicationStageFn commitG0{};
    CommitPublicationStageFn commitG10{};
    CommitPublicationStageFn commitCodec{};
    // Infallibly publishes readiness after every canonical group commits.
    PublishPublicationReadinessFn publishReadiness{};
    // Infallibly keeps readiness/operational false and sets the future
    // integration's cold-restart-required state. It runs exactly once when an
    // attempted publication first becomes poisoned.
    MarkPublicationPoisonedFn markPoisoned{};
};

// Production-neutral orchestration for the one-shot ISC12 native publication.
// The future loader-owned transaction remains responsible for serializing
// callers and issuing the non-forgeable lease. This class deliberately owns no
// native mutation implementation and exposes no rollback path.
class PublicationCoordinator final {
public:
    [[nodiscard]] auto Publish(
        const NativePublicationQuiescenceLease& quiescence,
        const PublicationCoordinatorCallbacks& callbacks) noexcept
        -> PublicationCoordinatorStatus;

    [[nodiscard]] auto State() const noexcept
        -> PublicationCoordinatorState {
        return state_;
    }

    [[nodiscard]] auto IsProcessLifetimeReserved() const noexcept -> bool {
        return processLifetimeReserved_;
    }

    [[nodiscard]] auto IsReadinessPublished() const noexcept -> bool {
        return readinessPublished_;
    }

private:
    PublicationCoordinatorState state_{PublicationCoordinatorState::Fresh};
    bool attemptInProgress_{};
    bool processLifetimeReserved_{};
    bool readinessPublished_{};
};

} // namespace ruffneckk::isc12
