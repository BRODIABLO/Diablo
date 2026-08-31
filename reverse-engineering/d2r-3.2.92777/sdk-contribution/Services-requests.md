# Further SDK Services requests

## PluginSDK v4 audit — 2026-08-31

PluginSDK v4 at commit `6eb8f8b6192868214706bd6d528c5294f2f551b7`
adds `ServiceId::Http = 16`, `ServiceId::ItemInteraction = 17`, and
`ItemServiceV1::executeExistingItemTransaction`. The four requests below now
have these governed statuses:

| Request | v4 status | Workspace consequence |
|---|---|---|
| Loader-owned native publication transaction | **Open** | ISC12 remains unable to publish any native mutation. The former provisional id 16 is invalid because v4 assigns it to `Http`; only D2RLoader may reserve a replacement id. |
| Typed item interaction events | **Delivered for the proven V1 surfaces** | `ItemInteractionServiceV1` provides semantic `Activate` events for inventory, Cube, personal stash and the current custom page. Shared stash and authoritative gameplay remain outside V1. |
| Atomic edits and moves for existing items | **Delivered for the supported V1 operations** | `executeExistingItemTransaction` atomically debits, edits or moves up to 64 identity-preserving items, with the container restrictions documented below. |
| Plugin-provided service discovery | **Open** | Inter-DLL providers still require an independently versioned and unload-safe discovery contract. |

`HttpServiceV1`, richer lifecycle events, Hardcore/Softcore character-creation
metadata and duplicate-Unique creation are useful v4 additions but do not close
either remaining request. No RuffnecKk plugin was migrated as part of this
audit.



## 1. Loader-owned native publication transaction

### Verified gap

The current header-only PluginSDK v4 at commit
`6eb8f8b6192868214706bd6d528c5294f2f551b7` exposes additive services through
`QueryService`, but its public `ServiceId` values stop at
`ItemInteraction = 17`. Patch and hook calls remain independent; Lifecycle,
ThreadService and the new item transaction do not exclude every D2R consumer
or serialize executable publishers. A plugin can preflight bytes, but cannot
prove that executable publication is quiescent, globally serialized with every
other publisher, or contained if a native write reports an uncertain result.

The minimum viable addition remains one optional synchronous service. Its
numeric ID must be officially reserved upstream. IDs 16 and 17 are already
assigned to `Http` and `ItemInteraction`; no local consumer may assume a
replacement value.

```cpp
namespace D2RL::NativePublication {

enum class CallbackOutcome : std::uint32_t {
    Committed = 0,
    Rejected = 1,
    Poisoned = 2,
};

enum class Result : std::uint32_t {
    Committed = 0,
    Rejected = 1,
    Poisoned = 2,
    InvalidArgument = 3,
    Unavailable = 4,
    Busy = 5,
    WrongPhase = 6,
    OwnerInactive = 7,
    CallbackFault = 8,
};

using ValidateLeaseFn = bool (__cdecl*)(
    const PluginContext* context,
    std::uint64_t epoch,
    std::uintptr_t token) noexcept;

struct LeaseV1 {
    std::uint32_t structSize;
    std::uint32_t flags;
    std::uint64_t epoch;
    std::uintptr_t token;
    ValidateLeaseFn validate;
};

using Callback = CallbackOutcome (__cdecl*)(
    const PluginContext* context,
    const LeaseV1* lease,
    void* userData) noexcept;

struct RequestV1 {
    std::uint32_t structSize;
    std::uint32_t flags;
    Callback callback;
    void* userData;
};

using ExecuteFn = Result (__cdecl*)(
    const PluginContext* context,
    const RequestV1* request) noexcept;

} // namespace D2RL::NativePublication

struct NativePublicationServiceV1 {
    std::uint32_t serviceSize;
    std::uint32_t serviceVersion;
    NativePublication::ExecuteFn executeStartupTransaction;
};
```

On x64 the proposed V1 sizes are 32 bytes for `LeaseV1`, 24 bytes for
`RequestV1`, and 16 bytes for `NativePublicationServiceV1`. The public header
must enforce fixed enum widths, standard layout, trivial copyability, size
gating and exact ABI tests.

### Normative Core behavior

- `executeStartupTransaction` invokes the callback synchronously during one
  documented startup phase in which no D2R consumer can execute.
- A single loader-owned gate serializes Patch, Hook, DLL-less JSON patches and
  every loader-internal executable publisher. Nested publication returns
  `Busy`.
- The lease is bound to the active plugin owner generation, callback thread and
  monotonic epoch. It validates only inside that invocation; copied fields are
  harmless after callback return.
- The callback may prepare private unpublished resources beforehand, then use
  transaction-aware Patch/Hook paths without reacquiring and deadlocking on the
  outer publication gate.
- `Rejected` is legal only while the loader can prove that no native write was
  attempted. A rejection after an attempted write is promoted to `Poisoned`.
- Every uncertain Patch/Hook result after an attempted write, callback fault or
  lost authority after the first write is `Poisoned`.
- `Poisoned` must log and terminate/fail-fast while quiescence remains held.
  D2R consumers must never resume. The enum remains observable for ABI and
  injected-fatal-handler tests.
- Only `Committed` and a demonstrably pre-write `Rejected` may release
  quiescence. No hot rollback or unload restoration is promised after a native
  publication has begun; process-lifetime relays remain owned until exit.

This is deliberately not a generic plugin-minted token or a thread-suspension
helper. The local D2RCore 1.1.0-beta binary contains thread suspend/resume and
native write machinery, but that proves only useful substrate—not a safe
boundary, no-new-thread guarantee, lock safety or public ownership contract.
Private Core ordinals and binary patches cannot substitute for a loader-owned
service.

### Required upstream slice

The SDK change remains additive: reserve a new service ID, add
`native_publication.h`, include it from `api.h`, list it in the SDK CMake public
headers, and add ABI tests and normative documentation. Existing API-v2,
API-v3 and API-v4 binaries remain compatible; an older loader returns
`UnknownService`, and the requesting plugin must refuse before any write.

The D2RLoader/Core implementation must add the service registry entry, startup
barrier, global publisher coordinator, owner/thread/epoch validation, TLS
`mutationAttempted`/poison state, transaction-aware Patch/Hook paths, callback
fault containment, no-resume poison handling and tests for wrong phase,
reentry, serialization, clean rejection, mutate-then-reject and uncertain
writes. SDK declarations must not ship as supported until that Core behavior is
implemented and tested.

ISC12 will consume this service through one full-set coordinator: preflight G0,
G10 and G9/G2/G4/G1/G3 before the first write; reserve all relays/state once for
process lifetime; commit those groups in that order; then publish readiness and
operational flags last. Until a real Core implementation exists, ISC12 keeps
its production lease unconstructible and refuses with zero native writes.

## 2. Typed item interaction events

### Status in PluginSDK v4

Delivered as `ServiceId::ItemInteraction = 17` and
`ItemInteractionServiceV1`. The loader emits a semantic `Activate` event on the
UI thread before normal handling and supplies generation-safe item/player
handles, the actual container, page and cell, keyboard modifiers and the active
keyboard/mouse or controller source. Priority ordering and `Consume` provide
the requested ordered interception boundary.

V1 deliberately covers normal inventory, Cube, personal stash and current
custom-page grids only. It does not emit shared-stash, vendor, trade, corpse,
ground, equipment, cursor or belt interactions, and it does not perform an
authoritative server mutation.

The read-only adoption audit identifies MassID as the strongest current
consumer: the service can replace its native client gesture seam and expose a
controller-aware activation, while the existing host request, server-side
validation and shared-stash path must remain. Transmogrify, Readable Items and
PSpell Framework can later share the same client boundary, subject to the same
authority and surface limits. No implementation changed during this audit.

### Original request retained for provenance

Input actions describe bindings, while tooltip listeners describe text.
Neither reports a semantic interaction with one specific item.

```cpp
D2RL::ItemInteractionListenerRegistration registration{
    .structSize = sizeof(registration),
    .priority = 100,
    .callback = [](const D2RL::ItemInteractionEvent* event,
                   void*) noexcept {
        if (event->phase != D2RL::InteractionPhase::Pressed) {
            return D2RL::InteractionResult::Continue;
        }

        if (event->action == D2RL::ItemAction::SecondaryClick
                && D2RL::HasModifier(
                    event->modifiers,
                    D2RL::InputModifier::Shift)) {
            QueueItemAction(
                event->item,
                event->surface,
                event->generation);

            return D2RL::InteractionResult::Consume;
        }

        return D2RL::InteractionResult::Continue;
    },
};

itemInteractions->registerListener(ctx, &registration);
```

The event should provide a generation-safe `ItemHandle`, originating UI
surface, logical action or physical button, active modifiers, and interaction
phase.

Returning `Consume` should stop lower-priority listeners and normal handling.
The event should run on a documented UI boundary, remain independent from
tooltip generation, and unregister automatically on unload.

## 3. Atomic edits and moves for existing items

### Status in PluginSDK v4

Delivered as the appended
`ItemServiceV1::executeExistingItemTransaction` function. A transaction accepts
up to 64 tagged `Debit`, `Edit` and `Move` operations, validates the entire set
before mutation, supports swap/chain placement through a temporary occupancy
model, preserves handles and native identity, and restores moved items and
edited values when a native placement or postcondition fails.

V1 moves cover normal inventory, Cube, personal stash and the current custom
page. Equipment, cursor, belt, shared stash, trade, corpse and ground moves are
rejected. A debit must leave a positive quantity, while atomic edits are limited
to durability, identified state and item level.

The read-only adoption audit identifies AutoSort as the strongest current
consumer for normal inventory, Cube, personal-stash and current custom-page
plans. Its shared-stash proxy scope and authoritative client-to-host request
still require a separate design, so the existing planner/packet path is not yet
removable. MassID may use atomic identified-state edits plus a tome debit, but
the last-charge case cannot be represented by a debit that must stay positive;
its current authoritative path therefore cannot be replaced wholesale. Gear
Swap Inventory remains outside V1 because equipment and experimental BodyLoc
13–20 moves are unsupported. No implementation changed during this audit.

### Original request retained for provenance

`ItemServiceV1` can edit one existing item, while its transaction model
consumes inputs and creates outputs. It cannot yet atomically edit several
existing items or move existing items between stock containers while
preserving identity.

```cpp
D2RL::ExistingItemTransaction transaction{
    .structSize = sizeof(transaction),
};

transaction.edit(itemA)
    .setIdentified(true);

transaction.edit(itemB)
    .setDurability(24);

transaction.debitStack(tome, 2);

transaction.move(
    equippedItem,
    D2RL::StockContainer::Equipment,
    D2RL::StockContainer::Cube,
    D2RL::PlacementMode::FindFreeSpace);

D2RL::ExistingItemTransactionResult result{};
const auto status = items->executeExistingItemTransaction(
    ctx,
    player,
    &transaction,
    &result);
```

Before committing, the Loader should revalidate every handle, player authority,
source container, destination, placement, stack debit, and edit.

Failure should change nothing. Success should preserve item handles, runtime
identity, seeds, socket contents, and untouched state, then use the normal
inventory, replication, and save paths.

A first version could support inventory, cube, equipment, cursor, and personal
or shared stash. Trade, vendors, corpses, ground items, and remote contexts
could remain unsupported until their ownership rules are proven.

## 4. Plugin-provided service discovery

### Status in PluginSDK v4

Open. V4 adds loader-owned services only; it does not add publication, query or
availability subscription for plugin-provided interfaces.

Plugins can expose versioned C APIs, but consumers must currently know the
provider module and export names and manually manage availability, load order,
ABI checks, and unload safety.

```cpp
struct DamagePolicyV1 {
    std::uint32_t structSize;
    std::uint32_t version;

    DamageResult(__cdecl* resolve)(
        const DamageRequest* request) noexcept;
};

const DamagePolicyV1 policy{
    .structSize = sizeof(policy),
    .version = 1,
    .resolve = &ResolveDamage,
};

D2RL::PluginServiceRegistration service{
    .structSize = sizeof(service),
    .name = "example.damage-policy",
    .version = 1,
    .interfaceSize = sizeof(policy),
    .interface = &policy,
};

serviceRegistry->publish(ctx, &service);
```

A consumer could query it without naming or linking to the provider DLL:

```cpp
const DamagePolicyV1* policy{};

if (serviceRegistry->query(
        ctx,
        "example.damage-policy",
        1,
        sizeof(DamagePolicyV1),
        reinterpret_cast<const void**>(&policy))
        == D2RL::PluginServiceResult::Success) {
    const auto result = policy->resolve(&request);
}
```

An optional availability subscription would also allow consumers loaded first
to discover providers loaded later:

```cpp
serviceRegistry->subscribe(
    ctx,
    "example.damage-policy",
    &OnProviderAvailable,
    &OnProviderUnavailable);
```

The Loader should track provider and consumer ownership, block new calls during
unload, wait for active calls to finish, invalidate the service, notify
subscribers, and remove registrations automatically.

For every accepted boundary, I would keep the API v3 model: optional service
queries, size-gated structures, safe handles, documented thread and multiplayer
authority, deterministic unload cleanup, and no partial state after failure.
