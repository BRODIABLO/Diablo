#include "isc12_publication_coordinator.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <utility>

namespace {

using ruffneckk::isc12::NativePublicationQuiescenceLease;
using ruffneckk::isc12::PublicationCommitOutcome;
using ruffneckk::isc12::PublicationCoordinator;
using ruffneckk::isc12::PublicationCoordinatorCallbacks;
using ruffneckk::isc12::PublicationCoordinatorState;
using ruffneckk::isc12::PublicationCoordinatorStatus;

enum class Event : std::uint8_t {
    None,
    PreflightG0,
    PreflightG10,
    PreflightCodec,
    Reserve,
    CommitG0,
    CommitG10,
    CommitCodec,
    PublishReadiness,
    MarkPoisoned,
};

struct Fixture {
    bool leaseHeld{true};
    Event revokeAt{Event::None};
    Event mutateAt{Event::None};
    Event reenterAt{Event::None};
    bool preflightG0{true};
    bool preflightG10{true};
    bool preflightCodec{true};
    PublicationCommitOutcome commitG0{PublicationCommitOutcome::Committed};
    PublicationCommitOutcome commitG10{PublicationCommitOutcome::Committed};
    PublicationCommitOutcome commitCodec{
        PublicationCommitOutcome::Committed};
    PublicationCoordinator* coordinator{};
    const NativePublicationQuiescenceLease* activeLease{};
    const PublicationCoordinatorCallbacks* activeCallbacks{};
    std::array<Event, 16> events{};
    std::size_t eventCount{};
    std::size_t readinessPublishCount{};
    std::size_t poisonCount{};
    std::size_t nativeMutationCount{};
    std::size_t reentryCount{};
    std::size_t releaseCount{};
    bool readinessWasFalseAtPublish{};
    bool readinessWasFalseAtPoison{};
    PublicationCoordinatorStatus reentryStatus{
        PublicationCoordinatorStatus::Active};
};

auto Check(bool condition, const char* expression, int line) -> void {
    if (condition) return;
    std::cerr << "CHECK failed at line " << line << ": " << expression
              << '\n';
    std::exit(EXIT_FAILURE);
}

#define CHECK(expression) Check((expression), #expression, __LINE__)

auto Record(Fixture& fixture, Event event) noexcept -> void {
    if (fixture.eventCount < fixture.events.size()) {
        fixture.events[fixture.eventCount] = event;
        ++fixture.eventCount;
    }
    if (fixture.revokeAt == event) fixture.leaseHeld = false;
    if (fixture.mutateAt == event) ++fixture.nativeMutationCount;
    if (fixture.reenterAt == event
            && fixture.coordinator
            && fixture.activeLease
            && fixture.activeCallbacks) {
        ++fixture.reentryCount;
        fixture.reentryStatus = fixture.coordinator->Publish(
            *fixture.activeLease,
            *fixture.activeCallbacks);
    }
}

auto ValidateLease(void* context) noexcept -> bool {
    return static_cast<Fixture*>(context)->leaseHeld;
}

auto ReleaseLease(void* context) noexcept -> void {
    ++static_cast<Fixture*>(context)->releaseCount;
}

auto PreflightG0(void* context) noexcept -> bool {
    auto& fixture = *static_cast<Fixture*>(context);
    Record(fixture, Event::PreflightG0);
    return fixture.preflightG0;
}

auto PreflightG10(void* context) noexcept -> bool {
    auto& fixture = *static_cast<Fixture*>(context);
    Record(fixture, Event::PreflightG10);
    return fixture.preflightG10;
}

auto PreflightCodec(void* context) noexcept -> bool {
    auto& fixture = *static_cast<Fixture*>(context);
    Record(fixture, Event::PreflightCodec);
    return fixture.preflightCodec;
}

auto Reserve(void* context) noexcept -> void {
    auto& fixture = *static_cast<Fixture*>(context);
    Record(fixture, Event::Reserve);
}

auto CommitG0(void* context) noexcept -> PublicationCommitOutcome {
    auto& fixture = *static_cast<Fixture*>(context);
    Record(fixture, Event::CommitG0);
    return fixture.commitG0;
}

auto CommitG10(void* context) noexcept -> PublicationCommitOutcome {
    auto& fixture = *static_cast<Fixture*>(context);
    Record(fixture, Event::CommitG10);
    return fixture.commitG10;
}

auto CommitCodec(void* context) noexcept -> PublicationCommitOutcome {
    auto& fixture = *static_cast<Fixture*>(context);
    Record(fixture, Event::CommitCodec);
    return fixture.commitCodec;
}

auto PublishReadiness(void* context) noexcept -> void {
    auto& fixture = *static_cast<Fixture*>(context);
    Record(fixture, Event::PublishReadiness);
    ++fixture.readinessPublishCount;
    fixture.readinessWasFalseAtPublish = fixture.coordinator
        && !fixture.coordinator->IsReadinessPublished();
}

auto MarkPoisoned(void* context) noexcept -> void {
    auto& fixture = *static_cast<Fixture*>(context);
    Record(fixture, Event::MarkPoisoned);
    ++fixture.poisonCount;
    fixture.readinessWasFalseAtPoison = fixture.coordinator
        && !fixture.coordinator->IsReadinessPublished()
        && fixture.coordinator->State()
            == PublicationCoordinatorState::Poisoned;
}

auto Callbacks(Fixture& fixture) noexcept
        -> PublicationCoordinatorCallbacks {
    return PublicationCoordinatorCallbacks{
        .context = &fixture,
        .preflightG0 = &PreflightG0,
        .preflightG10 = &PreflightG10,
        .preflightCodec = &PreflightCodec,
        .reserveProcessLifetime = &Reserve,
        .commitG0 = &CommitG0,
        .commitG10 = &CommitG10,
        .commitCodec = &CommitCodec,
        .publishReadiness = &PublishReadiness,
        .markPoisoned = &MarkPoisoned,
    };
}

auto Lease(Fixture& fixture) noexcept
        -> NativePublicationQuiescenceLease {
    return NativePublicationQuiescenceLease::ForTesting(
        &fixture,
        &ValidateLease,
        &ReleaseLease);
}

template <std::size_t Size>
auto ExpectEvents(
        const Fixture& fixture,
        const std::array<Event, Size>& expected) -> void {
    CHECK(fixture.eventCount == expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        CHECK(fixture.events[index] == expected[index]);
    }
}

auto TestAbsentLeaseRequiresQuiescence() -> void {
    PublicationCoordinator coordinator;
    Fixture fixture{.coordinator = &coordinator};
    NativePublicationQuiescenceLease absent;

    CHECK(coordinator.Publish(absent, Callbacks(fixture))
        == PublicationCoordinatorStatus::QuiescenceRequired);
    CHECK(coordinator.State() == PublicationCoordinatorState::Fresh);
    CHECK(!coordinator.IsProcessLifetimeReserved());
    CHECK(!coordinator.IsReadinessPublished());
    CHECK(fixture.eventCount == 0U);
    CHECK(fixture.poisonCount == 0U);
}

auto TestEveryPreflightFailureRejectsBeforeReservation() -> void {
    for (std::size_t rejectedIndex = 0U; rejectedIndex < 3U;
            ++rejectedIndex) {
        PublicationCoordinator coordinator;
        Fixture fixture{.coordinator = &coordinator};
        if (rejectedIndex == 0U) fixture.preflightG0 = false;
        if (rejectedIndex == 1U) fixture.preflightG10 = false;
        if (rejectedIndex == 2U) fixture.preflightCodec = false;
        auto lease = Lease(fixture);

        CHECK(coordinator.Publish(lease, Callbacks(fixture))
            == PublicationCoordinatorStatus::RejectedBeforeMutation);
        CHECK(fixture.eventCount == rejectedIndex + 1U);
        CHECK(fixture.events[0] == Event::PreflightG0);
        if (rejectedIndex >= 1U) {
            CHECK(fixture.events[1] == Event::PreflightG10);
        }
        if (rejectedIndex >= 2U) {
            CHECK(fixture.events[2] == Event::PreflightCodec);
        }
        CHECK(coordinator.State() == PublicationCoordinatorState::Fresh);
        CHECK(!coordinator.IsProcessLifetimeReserved());
        CHECK(!coordinator.IsReadinessPublished());
        CHECK(fixture.readinessPublishCount == 0U);
        CHECK(fixture.poisonCount == 0U);
    }
}

auto TestPreflightOrderAndRetry() -> void {
    PublicationCoordinator coordinator;
    Fixture fixture{
        .preflightG10 = false,
        .coordinator = &coordinator,
    };
    {
        auto lease = Lease(fixture);
        CHECK(coordinator.Publish(lease, Callbacks(fixture))
            == PublicationCoordinatorStatus::RejectedBeforeMutation);
    }
    ExpectEvents(fixture, std::array{
        Event::PreflightG0,
        Event::PreflightG10,
    });
    CHECK(coordinator.State() == PublicationCoordinatorState::Fresh);
    CHECK(!coordinator.IsProcessLifetimeReserved());
    CHECK(!coordinator.IsReadinessPublished());
    CHECK(fixture.readinessPublishCount == 0U);
    CHECK(fixture.poisonCount == 0U);

    fixture.preflightG10 = true;
    fixture.eventCount = 0U;
    {
        auto lease = Lease(fixture);
        CHECK(coordinator.Publish(lease, Callbacks(fixture))
            == PublicationCoordinatorStatus::Active);
    }
    ExpectEvents(fixture, std::array{
        Event::PreflightG0,
        Event::PreflightG10,
        Event::PreflightCodec,
        Event::Reserve,
        Event::CommitG0,
        Event::CommitG10,
        Event::CommitCodec,
        Event::PublishReadiness,
    });
    CHECK(coordinator.State() == PublicationCoordinatorState::Active);
    CHECK(coordinator.IsProcessLifetimeReserved());
    CHECK(coordinator.IsReadinessPublished());
    CHECK(fixture.readinessPublishCount == 1U);
    CHECK(fixture.readinessWasFalseAtPublish);
    CHECK(fixture.poisonCount == 0U);

    const auto previousEventCount = fixture.eventCount;
    auto activeLease = Lease(fixture);
    CHECK(coordinator.Publish(activeLease, Callbacks(fixture))
        == PublicationCoordinatorStatus::Active);
    CHECK(fixture.eventCount == previousEventCount);
    CHECK(fixture.readinessPublishCount == 1U);
}

auto TestInitiallyRevokedLeaseRequiresQuiescence() -> void {
    PublicationCoordinator coordinator;
    Fixture fixture{
        .leaseHeld = false,
        .coordinator = &coordinator,
    };
    auto lease = Lease(fixture);

    CHECK(coordinator.Publish(lease, Callbacks(fixture))
        == PublicationCoordinatorStatus::QuiescenceRequired);
    CHECK(coordinator.State() == PublicationCoordinatorState::Fresh);
    CHECK(!coordinator.IsProcessLifetimeReserved());
    CHECK(!coordinator.IsReadinessPublished());
    CHECK(fixture.eventCount == 0U);
    CHECK(fixture.poisonCount == 0U);
}

auto TestLeaseLossAtEveryPreflightBoundaryRequiresQuiescence() -> void {
    constexpr std::array preflightEvents{
        Event::PreflightG0,
        Event::PreflightG10,
        Event::PreflightCodec,
    };
    for (std::size_t revokedIndex = 0U;
            revokedIndex < preflightEvents.size(); ++revokedIndex) {
        PublicationCoordinator coordinator;
        Fixture fixture{
            .revokeAt = preflightEvents[revokedIndex],
            .coordinator = &coordinator,
        };
        auto lease = Lease(fixture);

        CHECK(coordinator.Publish(lease, Callbacks(fixture))
            == PublicationCoordinatorStatus::QuiescenceRequired);
        CHECK(fixture.eventCount == revokedIndex + 1U);
        CHECK(fixture.events[0] == Event::PreflightG0);
        if (revokedIndex >= 1U) {
            CHECK(fixture.events[1] == Event::PreflightG10);
        }
        if (revokedIndex >= 2U) {
            CHECK(fixture.events[2] == Event::PreflightCodec);
        }
        CHECK(coordinator.State() == PublicationCoordinatorState::Fresh);
        CHECK(!coordinator.IsProcessLifetimeReserved());
        CHECK(!coordinator.IsReadinessPublished());
        CHECK(fixture.readinessPublishCount == 0U);
        CHECK(fixture.poisonCount == 0U);
    }
}

auto TestRevokedAfterReservationIsTerminalWithoutMutation() -> void {
    PublicationCoordinator coordinator;
    Fixture fixture{
        .revokeAt = Event::Reserve,
        .coordinator = &coordinator,
    };
    {
        auto lease = Lease(fixture);
        CHECK(coordinator.Publish(lease, Callbacks(fixture))
            == PublicationCoordinatorStatus::ReservedWithoutMutation);
    }
    ExpectEvents(fixture, std::array{
        Event::PreflightG0,
        Event::PreflightG10,
        Event::PreflightCodec,
        Event::Reserve,
    });
    CHECK(coordinator.State() == PublicationCoordinatorState::Reserved);
    CHECK(coordinator.IsProcessLifetimeReserved());
    CHECK(!coordinator.IsReadinessPublished());
    CHECK(fixture.readinessPublishCount == 0U);

    fixture.leaseHeld = true;
    fixture.revokeAt = Event::None;
    const auto previousEventCount = fixture.eventCount;
    auto secondLease = Lease(fixture);
    CHECK(coordinator.Publish(secondLease, Callbacks(fixture))
        == PublicationCoordinatorStatus::ReservedWithoutMutation);
    CHECK(fixture.eventCount == previousEventCount);
    CHECK(fixture.poisonCount == 0U);
}

auto TestFirstCommitCanProveNoMutation() -> void {
    PublicationCoordinator coordinator;
    Fixture fixture{
        .commitG0 = PublicationCommitOutcome::RejectedWithoutMutation,
        .coordinator = &coordinator,
    };
    auto lease = Lease(fixture);

    CHECK(coordinator.Publish(lease, Callbacks(fixture))
        == PublicationCoordinatorStatus::ReservedWithoutMutation);
    ExpectEvents(fixture, std::array{
        Event::PreflightG0,
        Event::PreflightG10,
        Event::PreflightCodec,
        Event::Reserve,
        Event::CommitG0,
    });
    CHECK(coordinator.State() == PublicationCoordinatorState::Reserved);
    CHECK(coordinator.IsProcessLifetimeReserved());
    CHECK(!coordinator.IsReadinessPublished());
    CHECK(fixture.readinessPublishCount == 0U);
    CHECK(fixture.poisonCount == 0U);
}

auto TestUncertainOrPostCommitFailurePoisons() -> void {
    {
        PublicationCoordinator coordinator;
        Fixture fixture{
            .commitG0 = PublicationCommitOutcome::Uncertain,
            .coordinator = &coordinator,
        };
        auto lease = Lease(fixture);
        CHECK(coordinator.Publish(lease, Callbacks(fixture))
            == PublicationCoordinatorStatus::Poisoned);
        CHECK(coordinator.State() == PublicationCoordinatorState::Poisoned);
        CHECK(coordinator.IsProcessLifetimeReserved());
        CHECK(!coordinator.IsReadinessPublished());
        CHECK(fixture.readinessPublishCount == 0U);
        CHECK(fixture.poisonCount == 1U);
        CHECK(fixture.readinessWasFalseAtPoison);
    }
    {
        PublicationCoordinator coordinator;
        Fixture fixture{
            .commitG10 = PublicationCommitOutcome::RejectedWithoutMutation,
            .coordinator = &coordinator,
        };
        auto lease = Lease(fixture);
        CHECK(coordinator.Publish(lease, Callbacks(fixture))
            == PublicationCoordinatorStatus::Poisoned);
        ExpectEvents(fixture, std::array{
            Event::PreflightG0,
            Event::PreflightG10,
            Event::PreflightCodec,
            Event::Reserve,
            Event::CommitG0,
            Event::CommitG10,
            Event::MarkPoisoned,
        });
        CHECK(coordinator.State() == PublicationCoordinatorState::Poisoned);
        CHECK(!coordinator.IsReadinessPublished());
        CHECK(fixture.readinessPublishCount == 0U);
        CHECK(fixture.poisonCount == 1U);
        CHECK(fixture.readinessWasFalseAtPoison);

        const auto previousEventCount = fixture.eventCount;
        CHECK(coordinator.Publish(lease, Callbacks(fixture))
            == PublicationCoordinatorStatus::Poisoned);
        CHECK(fixture.eventCount == previousEventCount);
        CHECK(fixture.poisonCount == 1U);
    }
}

auto TestLeaseLossAfterNativeCommitPoisons() -> void {
    PublicationCoordinator coordinator;
    Fixture fixture{
        .revokeAt = Event::CommitCodec,
        .coordinator = &coordinator,
    };
    auto lease = Lease(fixture);

    CHECK(coordinator.Publish(lease, Callbacks(fixture))
        == PublicationCoordinatorStatus::Poisoned);
    ExpectEvents(fixture, std::array{
        Event::PreflightG0,
        Event::PreflightG10,
        Event::PreflightCodec,
        Event::Reserve,
        Event::CommitG0,
        Event::CommitG10,
        Event::CommitCodec,
        Event::MarkPoisoned,
    });
    CHECK(coordinator.State() == PublicationCoordinatorState::Poisoned);
    CHECK(coordinator.IsProcessLifetimeReserved());
    CHECK(!coordinator.IsReadinessPublished());
    CHECK(fixture.readinessPublishCount == 0U);
    CHECK(fixture.poisonCount == 1U);
    CHECK(fixture.readinessWasFalseAtPoison);
}

auto TestLeaseLossAtEveryCommitBoundaryPoisonsOnce() -> void {
    constexpr std::array commitEvents{
        Event::CommitG0,
        Event::CommitG10,
        Event::CommitCodec,
    };
    for (const auto revokeAt : commitEvents) {
        PublicationCoordinator coordinator;
        Fixture fixture{
            .revokeAt = revokeAt,
            .coordinator = &coordinator,
        };
        auto lease = Lease(fixture);

        CHECK(coordinator.Publish(lease, Callbacks(fixture))
            == PublicationCoordinatorStatus::Poisoned);
        CHECK(coordinator.State() == PublicationCoordinatorState::Poisoned);
        CHECK(coordinator.IsProcessLifetimeReserved());
        CHECK(!coordinator.IsReadinessPublished());
        CHECK(fixture.readinessPublishCount == 0U);
        CHECK(fixture.poisonCount == 1U);
        CHECK(fixture.events[fixture.eventCount - 1U]
            == Event::MarkPoisoned);
        CHECK(fixture.readinessWasFalseAtPoison);

        fixture.leaseHeld = true;
        const auto previousEventCount = fixture.eventCount;
        CHECK(coordinator.Publish(lease, Callbacks(fixture))
            == PublicationCoordinatorStatus::Poisoned);
        CHECK(fixture.eventCount == previousEventCount);
        CHECK(fixture.poisonCount == 1U);
    }
}

auto TestLeaseLossDuringReadinessPoisonsAndClearsPublication() -> void {
    PublicationCoordinator coordinator;
    Fixture fixture{
        .revokeAt = Event::PublishReadiness,
        .coordinator = &coordinator,
    };
    auto lease = Lease(fixture);

    CHECK(coordinator.Publish(lease, Callbacks(fixture))
        == PublicationCoordinatorStatus::Poisoned);
    ExpectEvents(fixture, std::array{
        Event::PreflightG0,
        Event::PreflightG10,
        Event::PreflightCodec,
        Event::Reserve,
        Event::CommitG0,
        Event::CommitG10,
        Event::CommitCodec,
        Event::PublishReadiness,
        Event::MarkPoisoned,
    });
    CHECK(coordinator.State() == PublicationCoordinatorState::Poisoned);
    CHECK(coordinator.IsProcessLifetimeReserved());
    CHECK(!coordinator.IsReadinessPublished());
    CHECK(fixture.readinessPublishCount == 1U);
    CHECK(fixture.poisonCount == 1U);
    CHECK(fixture.readinessWasFalseAtPublish);
    CHECK(fixture.readinessWasFalseAtPoison);
}

auto TestMutateThenReturnUncertainPoisons() -> void {
    PublicationCoordinator coordinator;
    Fixture fixture{
        .mutateAt = Event::CommitG0,
        .commitG0 = PublicationCommitOutcome::Uncertain,
        .coordinator = &coordinator,
    };
    auto lease = Lease(fixture);

    CHECK(coordinator.Publish(lease, Callbacks(fixture))
        == PublicationCoordinatorStatus::Poisoned);
    CHECK(fixture.nativeMutationCount == 1U);
    CHECK(fixture.poisonCount == 1U);
    CHECK(fixture.readinessPublishCount == 0U);
    CHECK(!coordinator.IsReadinessPublished());
    CHECK(fixture.readinessWasFalseAtPoison);
}

auto TestReentryAfterReservationIsRejected() -> void {
    PublicationCoordinator coordinator;
    Fixture fixture{
        .reenterAt = Event::Reserve,
        .coordinator = &coordinator,
    };
    auto callbacks = Callbacks(fixture);
    auto lease = Lease(fixture);
    fixture.activeLease = &lease;
    fixture.activeCallbacks = &callbacks;

    CHECK(coordinator.Publish(lease, callbacks)
        == PublicationCoordinatorStatus::Active);
    CHECK(fixture.reentryCount == 1U);
    CHECK(fixture.reentryStatus
        == PublicationCoordinatorStatus::RejectedBeforeMutation);
    CHECK(fixture.readinessPublishCount == 1U);
    CHECK(fixture.poisonCount == 0U);
}

auto TestLeaseMoveReleasesExactlyOnce() -> void {
    PublicationCoordinator coordinator;
    Fixture fixture{.coordinator = &coordinator};
    {
        auto source = Lease(fixture);
        NativePublicationQuiescenceLease moved{std::move(source)};
        CHECK(!source.IsHeld());
        CHECK(moved.IsHeld());

        NativePublicationQuiescenceLease assigned;
        assigned = std::move(moved);
        CHECK(!moved.IsHeld());
        CHECK(assigned.IsHeld());
        CHECK(coordinator.Publish(assigned, Callbacks(fixture))
            == PublicationCoordinatorStatus::Active);
        CHECK(fixture.releaseCount == 0U);
    }
    CHECK(fixture.releaseCount == 1U);
}

auto TestIncompleteCallbacksRejectBeforeMutation() -> void {
    PublicationCoordinator coordinator;
    Fixture fixture{.coordinator = &coordinator};
    auto callbacks = Callbacks(fixture);
    callbacks.commitCodec = nullptr;
    auto lease = Lease(fixture);

    CHECK(coordinator.Publish(lease, callbacks)
        == PublicationCoordinatorStatus::RejectedBeforeMutation);
    CHECK(coordinator.State() == PublicationCoordinatorState::Fresh);
    CHECK(!coordinator.IsProcessLifetimeReserved());
    CHECK(!coordinator.IsReadinessPublished());
    CHECK(fixture.eventCount == 0U);
    CHECK(fixture.poisonCount == 0U);
}

} // namespace

int main() {
    TestAbsentLeaseRequiresQuiescence();
    TestEveryPreflightFailureRejectsBeforeReservation();
    TestPreflightOrderAndRetry();
    TestInitiallyRevokedLeaseRequiresQuiescence();
    TestLeaseLossAtEveryPreflightBoundaryRequiresQuiescence();
    TestRevokedAfterReservationIsTerminalWithoutMutation();
    TestFirstCommitCanProveNoMutation();
    TestUncertainOrPostCommitFailurePoisons();
    TestLeaseLossAfterNativeCommitPoisons();
    TestLeaseLossAtEveryCommitBoundaryPoisonsOnce();
    TestLeaseLossDuringReadinessPoisonsAndClearsPublication();
    TestMutateThenReturnUncertainPoisons();
    TestReentryAfterReservationIsRejected();
    TestLeaseMoveReleasesExactlyOnce();
    TestIncompleteCallbacksRejectBeforeMutation();
    return EXIT_SUCCESS;
}
