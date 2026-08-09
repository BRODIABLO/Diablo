# Native hook report — D2R 3.2.92777

All RVAs are relative to the verified 3.2.92777 image. The plugin validates
expected bytes before any mutation. Disabled configuration installs no hooks.

## Capture and lifecycle

| Surface | Role |
|---|---|
| `0x44C030` | Capture the eligible `FillDamageValues` frame at the exact melee caller. |
| `0x44C2AC` | Narrow call owner: capture the exact pre-Critical physical return from `ApplyDamageBonuses`. |
| `0x44CB9E` | Narrow call owner: snapshot before primary Event 3 when that conditional event executes. |
| `0x4507B0` | Associate the snapshot with the exact newly linked combat node. |
| `0x44B2B0` | Select and retire the matching melee combat-node sidecar. |
| `0x44CE80` | Observe the canonical primary Execute call without changing it. |

Secondary resolution starts immediately after the primary `0x44CE80` call has
returned with an authoritative successful-hit result, while `0x44B2B0` still
owns the primary combat node. This is before durability, reactive melee events,
thorns, primary finalization, and node destruction. The source snapshot and
combat node are correlated by the exact source `D2Damage*`, attacker/defender
type+GUID, thread, and newly linked node address. A failed or range-deferred
consume keeps its sidecar for the later canonical consume.

The two narrow callsites use process-lifetime `FF 25` tail-jump relays allocated
within rel32 range. They are installed last after every owned byte range and
relay has been prevalidated. The SDK has no hot-unpatch contract for these
calls, so supported rollback is a cold process restart with the plugin disabled
or removed. If a write fails after another write succeeded, every installed
wrapper remains loaded but pass-through with the operational flag false.

## Expected bytes at written sites

Every entry hook validates its governed prologue before D2RLoader installs the
trampoline. The two narrow calls additionally validate their unique surrounding
contexts; their five written bytes are:

| Site | Expected bytes written/owned |
|---:|---|
| `0x44C2AC` | `E8 5F 14 00 00` (`21`-byte context unique) |
| `0x44CB9E` | `E8 CD 09 00 00` (`34`-byte context from `0x44CB89`) |

The exact governed entry signatures are:

| RVA | Logical identity | Expected bytes |
|---:|---|---|
| `0x44C030` | `SUNITDMG_FillDamageValues` | `40 56 57 41 54 48 81 EC 10 05 00 00 48 8B 05 85 F2 57 02 48 33 C4 48 89 84 24 D0 04 00 00` |
| `0x4507B0` | `SUNITDMG_AllocateAndLinkCombatRecord` | `40 55 57 41 56 41 57 48 81 EC C8 01 00 00 48 8B 05 03 AB 57 02 48 33 C4 48 89 84 24 B0 01 00 00` |
| `0x44B2B0` | `SUNITDMG_ConsumeMeleeCombatRecord` | `40 53 55 56 57 41 54 41 56 48 81 EC C8 01 00 00 48 8B 05 01 00 58 02 48 33 C4 48 89 84 24 B0 01 00 00` |
| `0x44CE80` | `SUNITDMG_ExecuteEvents` | `40 55 53 56 57 41 56 41 57 48 8D AC 24 E8 FE FF FF 48 81 EC 18 02 00 00 48 8B 05 29 E4 57 02` |
| `0x583E00` | EventFunc7 Knockback | `48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 20 49 8B F1 49 8B E8 4C 8B F1` |
| `0x583400` | EventFunc9 Blind | `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 83 EC 20 49 8B E9 49 8B F0` |
| `0x583580` | EventFunc14 ItemFreeze | `40 55 53 56 57 41 54 41 56 41 57 48 8D AC 24 40 FF FF FF 48 81 EC C0 01 00 00 48 8B 05 27 7D 44 02` |
| `0x584170` | EventFunc15 OpenWounds | `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 50 49 8B F9 49` |
| `0x583150` | EventFunc16 CrushingBlow | `48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 83 EC 60 49 8B F1 49 8B F8 44 8B FA 4C 8B F1` |
| `0x5849D0` | EventFunc19 Slow | `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 83 EC 60 49 8B F1 49 8B E8 4C 8B F1` |
| `0x583B30` | EventFunc20 SkillOnAttackHitKill | `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 54 41 56 41 57 48 83 EC 40 49 8B F1` |
| `0x5837F0` | EventFunc21 SkillOnGetHit | `48 89 5C 24 08 48 89 6C 24 10 56 57 41 54 41 56 41 57 48 83 EC 40 4D 8B F1 49 8B F8 4C 8B F9` |

`0x583E00..0x583F0F`, `0x583400..0x58357B`,
`0x5849D0..0x584B56`, and `0x5837F0..0x583995` are the logical bounds for
the newly governed exclusion handlers. Each exact signature has one match in
the verified analysis image. Their direct-xref count is zero, as expected for
the indirect event-list dispatcher; the EventFunc number is a semantic mapping,
not a recovered static callback table.

## ABI and caller gates

| Surface | ABI and caller qualification |
|---|---|
| Fill | `void(game, attacker, defender, D2Damage*, mode, SrcDam)`; only return address `0x44B6A0` is captured. |
| Allocate | `void(game, attacker, defender, const D2Damage*)`; only return address `0x44B708` is associated. |
| Consume | `uint8(game, attacker, defender, rangeBonus)`; sidecars are keyed by game, node, thread, and both type/GUID pairs. |
| Execute | `void(game, attacker, defender, bMissile, D2Damage*)`; only return address `0x44B3FF`, `bMissile=0`, and post-call `SUCCESS` trigger distribution. |
| EventFuncs | Nine-position callback frame: game, event, attacker, target, damage, three node payloads, auxiliary pointer; return value in `EAX`. |
| `0x44C2AC` | Original eight-position `ApplyDamageBonuses` ABI is preserved by a tail-jump relay; the wrapper captures `EAX` before native Critical/Deadly. |
| `0x44CB9E` | `int32(game, event, defender, attacker, damage)`; the wrapper snapshots before the conditional Event 3 and returns the original `EAX`. |

A mismatch means another owner or another build; the plugin does not overwrite
it. The static caller/callee and damage-layout evidence for the shared combat
surfaces remains governed in `Mission/mechanics-native-proof-92777.md`.

## Synthetic-event policy

The native nine-position callback ABI is preserved. During a synthetic
secondary resolution, EventFuncs 7 (knockback), 9 (blind), 14 (freeze),
19 (slow), 20 (offensive skill/legacy splash), and 21 (get-hit skill) are
filtered. EventFuncs 15 (Open Wounds) and 16 (Crushing Blow) remain live and
produce independent native rolls for every target.

Prevent Monster Heal is excluded structurally rather than by an EventFunc
filter: 92777 applies it in the primary success/AR resolver, which synthetic
targets never call. It is not carried by `D2Damage` and does not appear in the
synthetic Calculate → Execute → Finalize subgraph. Synthetic result flags are
rebuilt from an allowlist (`SUCCESS`, target-specific `GETHIT`, then per-target
Critical/Deadly and recalculated `WILLDIE`) so primary knockback and defensive
outcomes cannot leak to a secondary.

## Native helpers

The plugin uses governed functions for deep-copy/destruction of the `0x180`
damage record, unit enumeration, target permission, server unit re-resolution,
active weapon and actual used-skill lookup, stat access, per-target resistance
calculation, damage-event execution, and finalization. Those helper identities,
callers, signatures, and ABIs are proven statically for 92777, but their entry
bytes are deliberately not required to remain vanilla at plugin load: calling
their current entry allows an already-installed compatible owner to stay in the
chain. `GetItemsTxtRecord 0x314110`, for example, is a legitimate hook surface
for other workspace components. Strict runtime byte ownership is reserved for
the sites MeleeSplash itself writes.

The per-target Critical/Deadly adapter reproduces the current 92777 order:
weapon mastery, passive Critical, then Deadly; each positive chance consumes
its own attacker-seed roll and the first success short-circuits the rest. It is
not the future PD2 cap/multiplier resolver.

## Ownership and coexistence

The owned spans have no exact overlap in the audited 139-site PluginPack
manifest, the 57 governed BKVince patch sites, FloatingDamage's observer owner,
or source-visible standalone addons in the current workspace. A static scan of
current workspace binaries found no target signature collision. Black-box and
future binaries still require the full active-stack cold-start/smoke matrix;
the plugin refuses any byte mismatch rather than chaining an unknown owner.
