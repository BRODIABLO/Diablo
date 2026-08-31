# Revive Overhaul native contract

Qualification targets: D2R builds 92777 and 93847. The governed native corpus
retains its historical `d2r-3.2.92777` path because its verified executable
identity supplies the common native contract. The reported build name is
diagnostic only. Every owned entry, return site, ABI witness and callable helper
must pass its strict byte fingerprint before the first hook is installed.

## Owned hooks

| RVA | Contract | Scope |
|---|---|---|
| `0x4A3A20` | `D2GAME_MONSTERS_AiFunction03(game, monster, tickParam)` | Arms a thread-local guard only when native state 96 marks `monster` as a Revive. |
| `0x596720` | native unit distance helper | Transforms only the return to `0x4A3C0A` while the Revive guard is active. |
| `0x4A7270` | `AITACTICS_SetVelocity(monster, mode, velocity, bonus)` | Adjusts only the return to `0x4A3C4F` while the Revive guard is active. |
| `0x4A8090` | owner-follow helper | Adjusts only the return to `0x4A3C66` while the Revive guard is active. |
| `0x55A510` | Revive target validator `(game, caster, target) -> int32` | Calls vanilla first; the fallback keeps every native check except the unique-rank rejection. While SrvDo58 has armed aura capture, the finally admitted target supplies the pre-Revive Aura Enchanted tuple. |
| `0x34C730` | `AIUTIL_CanUnitSwitchAi(monster, state143, unique, boss, switchAI)` | Changes only the unique argument at the exact client Revive return site `0x9664D`; every other caller is pass-through. |
| `0x55E7E0` | `SKILLS_SrvDo058_Revive(game, caster, skillId, level) -> int32` | Captures and fully reactivates the exact Aura Enchanted right-skill tuple around vanilla execution. |

Every owned entry and callable native helper used by the fallback is checked
against a strict build signature before the first mutation. The target class id
is read from the governed `UnitAny + 0x04` layout instead of calling the shared
`UNITS_GetClassId` entry, so a compatible hook owned by an earlier plugin cannot
create a false signature conflict. A partial installation stays
loaded but operationally inert so an installed trampoline can only pass through
to its original function until a cold restart.

## Shared hook ownership

Cast Triggers is the unique Suite owner of `SUNIT_GetTargetUnit 0x48FE20`.
Revive Overhaul neither fingerprints nor calls that entry. Vanilla
`SKILLS_SrvDo058_Revive` resolves the corpse and passes the exact pointer to
`SKILLS_ValidateReviveTarget`; the validator hook therefore provides the
target without an order-sensitive shared-entry dependency. Revive Overhaul
remains fully functional when Cast Triggers is absent and requires no service
ABI from it.

The AI scope additionally fingerprints `STATES_CheckState 0x3351B0` and the
unique native Revive marker witness at `0x55EB48`. That witness enables unit
flag `0x80000000` and state 96. The scope therefore does not depend on a skill's
`pettype` or on AI special state 7.

## High-rank admission

The fallback applies only to rank mask `0x000E` (Champion, SuperUnique or
Unique). It rechecks the current MonStats2 record for `CorpseSel` and `Revive`,
requires a dead consumable corpse, then calls native `AIUTIL_CanUnitSwitchAi`
at `0x34C730` as:

```text
(target, checkState143=true, checkUnique=false,
 checkBoss=true, checkSwitchAI=true)
```

Only `checkUnique` differs from the original validator. Native boss/prime-evil,
state 143, state 54, unit-mode, unit-flag and `SwitchAI` protections remain
authoritative.

The client selector at `0x96600` invokes the same predicate with every optional
gate enabled through the unique 24-byte witness at `0x96635`. The plugin owns no
bytes in the selector. Its hook substitutes `checkUnique=false` only when the
return address is exactly `0x9664D` and the target has rank mask `0x000E`, then
the selector continues through all remaining native restrictions.

## Aura preservation

Aura Enchanted is detected by native MonUMod id 30 in the target's existing
UMod list. The outer `SrvDoFunc 58` hook arms a thread-local capture frame, and
the target validator fills it only after final corpse admission. This captures
the skill id from the target's skill record and owner GUID from its active-skill
node before vanilla clears the right active skill. After a successful Revive,
the outer hook calls native
`D2GAME_AssignSkill 0x438A70(target, 0, skillId, ownerGuid)` with the same tuple.
This is the complete route used by Aura Enchanted itself at `0x495F64`: it sets
the right active skill and continues through native activation and
synchronization. A different right skill installed by native code is never
overwritten.

No aura table, skill list or aura selection logic is duplicated. The method
therefore preserves modded native aura pools without knowing their skill ids.

## Provenance boundary

D2MOO commit `19019806df7f3e877fa105b05395d1e3597e2316` supplies semantic legacy
names and control-flow references only. No 32-bit address, ordinal, structure
layout or calling convention is reused in this D2R plugin.
