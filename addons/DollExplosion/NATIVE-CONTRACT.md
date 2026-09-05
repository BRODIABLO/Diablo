# Doll Explosion native contract

## Scope and ownership

Doll Explosion owns exactly two D2R entry hooks:

| RVA | Role | ABI | Ownership |
|---:|---|---|---|
| `0x444F50` | monster death-mode callback | `int32(game, modeChange)` | exclusive, original called exactly once |
| `0x466B40` | generic missile callback used by `pSrvDo=1` | `int32(game, missile)` | exclusive, foreign missiles delegate exactly |

It does not hook `MONSTERMODE_EventHandler 0x447420`, which is an owned Burn
Damage Fix surface in the audited Suite baseline. It does not hook or own
`srvdofunc169`, modify PluginPack, or link against an eezstreet DLL.

When DiagnosticsService v1 is available, both owned ranges must report
`Unchanged` with zero owners before installation. Without that optional
service, strict vanilla entry bytes are still mandatory. Any partial or
untracked mutation refuses loading.

## Static fingerprint

The fingerprint in `src/native_fingerprint.hpp` covers every called helper and
every layout used by the plugin:

| RVA | Contract |
|---:|---|
| `0x445016` | game data-context and `MonStats+0x3E` death-damage guard |
| `0x445322` | eleven-argument native creation of missile 117 |
| `0x4333F0` | `CreateSkillMissile(game,id,owner,skill,level,counter,ox,oy,tx,ty,check)` |
| `0x455750` | basic `pSrvDo=1` handler that delegates to `0x466B40` |
| `0x466CE0`, `0x466E46` | `Missiles+0x2C` dispatcher and return-2 removal branch |
| `0x2380E80` | live pSrvDo table; entry 1 must equal `D2R+0x455750` |
| `0x166040`, `0x0976E0` | context-aware Missiles and MonStats record resolvers |
| `0x34B9D0`, `0x349860`, `0x34A330` | unit type, class and GUID |
| `0x341A20`, `0x341A30` | dynamic-path X and Y |
| `0x3351B0`, `0x55EB48` | state lookup and native Revive-state-96 witness |
| `0x34A1E0`, `0x153B00` | unit seed and bounded `[0,n)` RNG |
| `0x2F5020` | layered unit stat lookup; stat 7 is maximum life |
| `0x404270` | next game counter for missile creation |
| `0x44A120` | native area-damage enumeration and missile damage application |
| `0x4496E0` | native `D2Damage` destructor |
| `0x44DF40` | `Game+0x104` difficulty and `Game+0x106` data context |
| `0x44556A`, `0x4455D2` | `D2Damage` initialization, physical field and ten-argument AoE call |
| `0x3BB18A`, `0x3BC411` | missile data pointer, total-frame and current-frame layouts |
| `0x3BB1E0`, `0x3BC3E0` | current-frame and total-frame getters |
| `0x3BD450`, `0x3BDBC0` | current-frame and total-frame setters |

These signatures are byte-identical in the governed common corpus. Build names
are logged only for diagnosis and never authorize or deny loading.

## Death flow

For every death callback, the plugin performs only a unit-type and class lookup.
All non-targets call the original immediately. A configured target is eligible
only when all of the following hold before the original callback:

- unit type is monster;
- state 96 (Revive) is absent;
- difficulty is 0, 1 or 2;
- the compiled MonStats record exists;
- the record's `deathDmg` bit at byte `+0x3E` is clear;
- the dynamic path and coordinates are available.

The original death callback then runs exactly once. Custom work starts only
after a successful original return. This ordering preserves corpse creation and
all other native death bookkeeping. A configured record that already has
`deathDmg` is deliberately skipped to prevent double explosions.

## Damage and RNG

The fixed formula performs one inclusive roll in the configured difficulty
range using the dying unit's native seed, then shifts displayed hit points left
by eight. The percent formula performs the same inclusive integer-percent roll
and multiplies D2R maximum-life fixed point by that percentage with a 64-bit
intermediate. Configuration bounds prevent signed overflow.

The 0x180-byte `D2Damage` record is zero-initialized and reproduces the governed
native small-buffer layout:

- physical damage: `+0x18`;
- inline storage pointer: `+0x40 -> +0x58`;
- inline length: `+0x48 = 0`;
- inline capacity marker: `+0x50 = 0x8000000000000010`;
- owned subobject sentinel: `+0x158 = 1`;
- terminal field: `+0x178 = 0`.

Area damage receives `(game, sourceMissile, x, y, radius, damage, 0, 0,
nullptr, 0x581)`. The native destructor runs after the call.

## Delay carrier and sidecar

Delay zero first requires the compiled `monstercorpseexplode` record 117 to
retain `pSrvDo=1` and `pSrvHit=0`, then creates that missile and applies damage
immediately, matching the native visual-before-damage gate. Every created
visual is also validated as a type-3, class-117 missile.

A positive delay creates `baalcorpseexplodedelay` missile 587 owned by the dead
Doll. Before use, its compiled record must have `pSrvDo=1` and `pSrvHit=0`. The
new instance must have unit type 3, class 587, a valid GUID, native total frames
10 and native current frames 10. The plugin then calls the fingerprinted native
setters for both counters and verifies both values through the fingerprinted
getters before accepting the configured delay.

A fixed 256-slot SRW-locked sidecar stores `(game, missile pointer, GUID, x, y,
damage, radius)`. Native hooks allocate no heap memory. The generic hook calls
the original first. A non-2 result keeps the sidecar; result 2 atomically takes
it, creates missile 117 for the visual, applies the AoE with the still-live
carrier as damage source, and returns the original result so the native
dispatcher removes the carrier.

The required D2RLoader Lifecycle service clears all slots on both `GameJoined`
and `GameLeft`. This prevents carriers removed by whole-game teardown from
leaking bounded sidecar capacity into later sessions.

If the sidecar is full, the already-created ID 587 carrier remains harmless and
no custom damage occurs. Unload disables new work and clears all slots before
the loader removes the hooks; orphaned ID 587 carriers expire with their native
no-hit behavior.

## Fail-closed boundary

A startup fingerprint or ownership mismatch refuses loading before the first
hook. A later protected native read/call fault or live carrier-contract mismatch
atomically disables custom explosions. Original callbacks remain the pass-
through path. Per-event missile allocation failure skips that event rather than
fabricating an unproven fallback.

The following remain runtime gates, not static claims: exact frame timing in a
live game, damage attribution, corpse consumption interactions, resistance and
player-count results, TCP/IP authority, unload during an active carrier, and
full-Suite coexistence.
