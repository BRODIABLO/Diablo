# BKVCombat native contract — D2R 3.2.92777

This contract governs the first `BKVCombat.dll` release. It applies only to
`D2R.exe` build `3.2.92777`, canonical image SHA-256
`CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`.
D2MOO is a semantic reference only; no D2MOO address, layout, or ABI is used.

## Authoritative damage path

The player damage path is distributed rather than owned by one universal
resolver:

1. `SUNITDMG_FillDamageValues` (`0x44C030`) builds the target-specific
   `D2Damage` record and resolves player Critical/Deadly Strike.
2. `SUNITDMG_CalculateTotalDamage` (`0x44DF10`) applies target resistances,
   reductions, and absorb policy.
3. `SUNITDMG_ExecuteEvents` (`0x44CE80`) dispatches attacker event 5, defender
   event 1, ordinary life/mana leech, and the authoritative HP mutation.
4. `SUNITDMG_FinalizeDamage` (`0x44A9B0`) handles reactions, death, and packets.

`0x44CE80` is shared by melee and missile paths and is already owned by
MeleeSplash in the supported full stack. BKVCombat must not hook it.

`D2Damage` is `0x180` bytes. Governed fields used by Release 1 are result flags
at `+0x04`, physical damage at `+0x18`, life steal at `+0x120`, and mana steal
at `+0x124`. Critical and Deadly Strike share result bit `0x2000`; that bit does
not preserve which branch succeeded.

## Release 1 seams

| Policy | Narrow seam | Expected bytes / ABI | Owner and disposition |
|---|---|---|---|
| Critical chance | calls at `0x44C2D3` and `0x44C32B` | `E8 18 12 EF FF`; `E8 F0 8C EA FF`; preserve each native callee ABI | BKVCombat; cap chance at 75 only when enabled |
| Deadly chance | call at `0x44C37F` | `E8 DC 98 EA FF`; preserve layered-stat ABI | BKVCombat; cap chance at 75 only when enabled |
| Deadly multiplier | branch at `0x44C3C2` | `2B C1 3B C3 7D 11`; preserve failure and Critical-success branches | BKVCombat; Deadly uses 1.5x while Critical retains native 2x |
| Crushing Blow | EventFunc16 core call at `0x583257` | `E8 74 E9 FF FF`; `(game, attacker, target, context) -> int32` | BKVCombat; EventFunc16 entry remains owned by MeleeSplash |
| Open Wounds | EventFunc15 helper call at `0x5842F3` | `E8 28 FA EA FF`; `ApplyArgs* -> StatList*` | BKVCombat; EventFunc15 entry remains owned by MeleeSplash |
| Life/Mana steal | sole call at `0x44D038` | context `0x44D02C`; `(game, attacker, defender, D2Damage*) -> void` | validated native baseline; no BKVCombat write or wrapper |

All expected contexts and all directly called helpers are validated before the
first write. Wrappers remain pass-through until every write succeeds and the
plugin publishes one atomic `operational` state. D2RLoader provides no hot
unpatch transaction: a partial commit stays loaded and inert until a cold
restart. Removal of the DLL and configuration is the cold rollback.

## Critical and Deadly Strike

The native player resolver is lazy and short-circuited:

1. weapon mastery Critical;
2. passive Critical (`stat 337`);
3. item Deadly Strike (`stat 141`).

Only a failed positive chance advances to the next branch. Each attempted
branch consumes the attacker's server RNG. A Critical success doubles physical
damage. A Deadly success uses 1.5x physical damage. Both set result bit `0x2000`.
Arithmetic preserves the native fixed-point/integer order and 32-bit behavior.

MeleeSplash rolls Critical/Deadly independently for each synthetic target.
BKVCombat therefore exposes an optional versioned C ABI resolver. MeleeSplash
uses it when available and operational; otherwise it retains its governed
vanilla-92777 fallback. CB and OW are not exposed through that API because the
synthetic path already traverses EventFunc16 and EventFunc15 and must not proc
twice.

## Crushing Blow classification and scaling

Target priority is exactly:

`MajorBoss > PrimeEvil > Elite > Ordinary`.

Heralds and Ascendants are `Elite`. The runtime Herald/ghostly marker is treated
as an Elite marker alongside champion, unique, superunique, and ordinary boss.
No C++ boss-name allowlist exists. MajorBoss identities come from configuration
and are validated against the active `monstats.txt` key and `*hcIdx`; PrimeEvil
and boss facts also come from the active data/runtime classification.

Fractions are:

| Class | Melee | Ranged |
|---|---:|---:|
| Ordinary | 1/6 | 1/9 |
| Elite, including Herald/Ascendant | 1/8 | 1/12 |
| PrimeEvil | 1/16 | 1/24 |
| MajorBoss | 1/20 | 1/30 |

The ranged divisor is 1.5 times the melee divisor. BKVCombat calls the live
`GAME_GetPlayerCountBonus` entry (`0x542F40`) instead of owning it, so the
PluginPack player-count cap remains authoritative for the HP bonus actually
used by the monster. Crushing Blow Efficiency is read from the attacker-global
and active-weapon-local configured stat, with negative totals treated as zero
and no invented upper cap. For current HP `H`, class fraction `n/d`, live HP
bonus `p`, and CBE `e`, the checked integer policy is:

`trunc(H * n * (100 + e) / (d * (100 + p)))`.

Positive physical resistance is applied after that amount; flat physical
damage reduction is not. The native event chance roll remains independent per
target, and CBE never changes its RNG or chance.

## Open Wounds and statlist lifecycle

EventFunc15 is `0x584170`; its generic statlist helper is `0x433D20`. Native
proof covers roll, formula, state 62, allocation, refresh, replacement,
expiry event 12, unlink/free, and the default removal callback `0x436240`.
The stat ID is transported by the runtime helper arguments and must be witnessed
as `STAT_HPREGEN` (`74`) before the policy becomes operational. It is not read
from the zero-initialized canonical-image BSS tuple and is never hardcoded as
stat 7.

Release 1 owns only the internal helper call. It accepts offline/local
player-to-monster applications whose runtime tuple is exactly state `62`, stat
`74`, and a negative rate. The first stack is created through the native
helper with a BKVCombat removal callback; the second and third use the proved
native allocation, stat, post, expiry-event, unlink, and free primitives. Each
application lasts 125 frames. When already capped, a new application refreshes
all three managed expiries to `currentFrame + 125` without adding a fourth
rate. The custom callback removes state 62 only after the final managed list is
gone, so the unit aggregate naturally sums all live stat-74 contributions.

The rate curve is recomputed before the old boss-halving branch, then ordered
as `base -> physical resistance -> mercenary/owned-pet quarter -> negation`.
Physical resistance is clamped to `[-100,100]`; integer division truncates
toward zero. This deterministic 92777-compatible composition is the BKVince
Release 1 policy, not a claim of bit-identical PD2 rounding. Flat Open Wounds
DPS is deferred and no stat ID is invented for it.

Foreign native state-62 lists are never rewritten by the custom path. A
BKVCombat-managed list from another source is preserved and blocks mutation;
multi-attacker behavior remains outside the supported offline/local scope.
No unit or owner pointer is retained outside the call. Because live stat lists
store the plugin callback address, Open Wounds supports cold disable/removal
only after the game process exits.

## Life Tap and leech

Life Tap stays native at EventFunc05 `0x55BCD0..0x55BFAC`. It computes its own
credit from resolved physical damage and is not ordinary stat-60 life steal.
BKVCombat must neither replace nor double it.

Ordinary life/mana leech is uniquely called at `0x44D038` after attacker and
defender events. It consumes `D2Damage+0x120/+0x124`, monster Drain values, and
difficulty divisors. The native Normal/Nightmare/Hell divisors `1/1`, `1/2`,
and `1/3` already match the approved Release 1 baseline. Enabling either leech
toggle therefore validates the call context, consumer, Life Tap handler, and
active `difficultylevels.txt`, then installs no leech hook. Skill-specific PD2
reductions remain deferred until each authoritative source context is proved.
MeleeSplash already applies its one-half factor before the same native
consumer, so BKVCombat must not scale it again.

## RNG, ordering, and coexistence

`UNITS_GetSeed` (`0x34A1E0`) returns the unit seed at `unit+0x28`. Native
Critical/Deadly, CB, and OW rolls advance the attacker server seed using the
92777 LCG; there is no proved callable universal roll owner. BKVCombat preserves
the native lazy consumption order.

The proved melee ordering is Critical/Deadly, total-damage calculation,
attacker event 5 (CB/OW; node-relative order is not globally fixed), defender
event 1 (Life Tap), ordinary leech, absorbed life/main HP mutation, then final
death/reaction processing.

The selected seams do not overlap the five PluginPack hook manifests,
FloatingDamage (`0x427150`), BurnFireResistance (`0x451380`), or the current
BKVince JSON patches. BKVCombat never modifies or links an eezstreet DLL.
Compatibility claims still require a fresh full-stack runtime matrix in both
relevant load orders; pre-existing third-party ownership failures cannot be
relabelled as BKVCombat compatibility proof.

## Deferred Release 2/3 gates

No governed common resolver yet proves dual-wield IAS-minus-WSM selection,
attack-rate/breakpoint ownership, or all skill hit-frame consumers in 92777.
Release 2 remains disabled until those exact seams are proved. Release 3 true
melee splash remains separate and must consume the versioned combat policies;
it must not turn this Release 1 DLL into an owner of every damage stage.
