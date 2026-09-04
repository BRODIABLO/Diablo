# Revive Overhaul 2.1 — native targeting, aura activation and AI scope

Date: 2026-08-31

## 2.1.4 automatic callback bridge — 2026-09-02

The live 2.1.3 test returned `selector high-rank calls=44943`,
`selector accepts=37560`, `selector rejects=7383` and
`server admissions=0`. The complete client selection predicate therefore
accepts high-rank corpses, but the stock `CltStFunc 24` route still submits no
Revive action. This closes the selector hypothesis and invalidates the retained
assumption that `21/58/24/blank/3` could support high-rank casting through a
predicate-only hook.

Version 2.1.4 automatically applies the externally proven callback route after
each data-table load. It uses PluginSDK lifecycle and table services to locate
Revive id 95 in the Classic, LoD and RotW compiled Skills banks, requires row
size `0x2EC` and an exact native or already-bridged tuple, then changes only
`21/58/24/blank/3` to `blank/58/39/36/2` in memory. Source TXT files remain
untouched. Unknown tuples and partial writes fail closed; the authoritative
`SrvDoFunc 58` validator hook retains the high-rank eligibility checks.

Two independent Release builds are byte-identical at 195,072 bytes, SHA-256
`31B2FFC30F90540C37C6B9222DB4D022E3500E9CE917DFCC1A228D5E24F63508`,
and the policy suite passes 1/1. The first complete-stack 93847 cold start
passes with 38 plugins, 17 patches and startup 24/24. The lifecycle callback
accepted revision 1 and logged that the automatic route is active without a
`Skills.txt` edit. Champion/Unique gameplay remains pending.

## 2.1.2 client admission correction — 2026-09-02

The live BKVince 3.3.93847 test proved that normal corpse selection still
works, while Champions, Uniques and SuperUniques can be identified under the
cursor but right-click submits no Revive action. Rakanishu's active `fallen2`
records independently retain `corpseSel=1`, `revive=1` and `switchai=1`, with
no boss or prime-evil flag. The failure is therefore assigned to the common
client high-rank admission path, not to monster softcode.

Revive Overhaul remains an autonomous hybrid RuffnecKk D2RLoader Suite DLL.
Version 2.1.2 will replace the shared `AIUTIL_CanUnitSwitchAi 0x34C730` entry
hook and `_ReturnAddress()` discriminator with a managed `PatchCallRel32`
redirect owned only at the proven Revive client callsite `0x96648`. Its relay
changes only `checkUnique` for the rank mask `0x000E`; every other native
selector restriction and the independent authoritative server fallback remain
unchanged. The existing TOML remains justified by its AI distance, velocity,
high-rank and aura policy settings. No TXT/TSV edit, save migration,
PluginPack modification or eezstreet redistribution is introduced.

The first 2.1.2 BKVince cold start on 2026-09-02 passed with the complete
active stack: 38 plugins, 17 memory patches, startup 24/24 and no Revive hook
rejection. The plugin loaded mod-local with diagnostics enabled and generated
the full multiplayer environment fingerprint. Champion, Unique, SuperUnique,
aura and AI gameplay gates remain open for Vincent's live test.

The subsequent Champion and Unique attempts failed despite
`client gate calls=738056`, `client high-rank candidates=157480` and
`client selections=157480`, while `server admissions=0`. This proves the
owned `0x96648` relay is reached, recognizes both tested ranks and receives a
successful result from `AIUTIL_CanUnitSwitchAi`, but does not prove the final
return of `CLIENT_ValidateReviveTarget`. Version 2.1.3 therefore adds a
pass-through diagnostic hook on the complete selector entry `0x96600` and
counts final high-rank accepts/rejects. It changes no additional admission
policy and performs no TXT or compiled-table mutation.

## Autonomous Suite decision

Revive Overhaul remains an independent RuffnecKk D2RLoader Suite plugin with
its own version, TOML configuration, DLL and release archive. It remains
hybrid global/mod-local, does not merge into an eezstreet DLL and does not
modify or redistribute any eezstreet component.

The runtime matrix is executed on the official D2R 3.3.93847 baseline. Build
92777 is covered by governed native equivalence only while every surface used
by the plugin remains byte-identical. Build names are diagnostic only. Loading
is governed by a complete fail-closed native fingerprint covering every hook,
helper, callsite and layout witness used by the plugin.

## 2.1.1 hook-ownership correction

The complete BKVince stack proved a deterministic Suite collision on
2026-08-31. Cast Triggers loaded first and accepted its native fingerprint,
then Revive Overhaul refused because the vanilla bytes at
`SUNIT_GetTargetUnit 0x48FE20` had already been replaced. The mod-local logs
name the exact failure as `signature mismatch at GetTargetUnit` on every fresh
startup.

Cast Triggers remains the unique owner of the `0x48FE20` entry. Revive
Overhaul 2.1.1 removes that RVA, signature, function pointer and direct call
from its native contract. During `SKILLS_SrvDo058_Revive`, it instead arms a
thread-local capture frame. The already-owned
`SKILLS_ValidateReviveTarget(game, caster, target)` hook receives the exact
corpse selected by vanilla, captures its Aura Enchanted tuple after final
admission and before the native Revive mutation, then the outer SrvDo58 hook
reactivates that tuple only after successful completion.

This design adds no dependency on Cast Triggers, works when that optional
plugin is absent, preserves a complete fail-closed fingerprint and avoids an
order-sensitive exception for an entry owned by another Suite component.

## Runtime evidence invalidating 2.0.1

The external 92777 tester established the following facts:

- the current D2R `Revive` client fields cannot highlight Champion or Unique
  corpses;
- the legacy client setup can submit the corpse and the server high-rank
  fallback admits it;
- the status command reported high-rank admissions and two aura captures plus
  two aura restores, but the revived Aura Enchanted monster had no active aura;
- Revive AI and scatter counters remained at zero while the minion still moved
  away from its owner at close range.

Therefore 2.0.1 is not release-ready. Its aura restore counter proves only a
right-skill pointer assignment, not a functioning aura. Its AI filter also
does not cover the observed custom pet-type path.

## Retained design

1. Keep vanilla D2R `SrvStFunc 21` and `SrvDoFunc 58`; do not require the
   legacy `CltStFunc 39` / `CltDoFunc 36` workaround.
2. Bridge the proven high-rank client callback route automatically after table
   compilation while retaining `SrvDoFunc 58` server authority.
3. Preserve the exact pre-Revive Aura Enchanted skill without rerolling the
   aura pool, and reactivate it through the proven high-level assignment path.
4. Scope AI improvements to a native Revive marker set by `SrvDoFunc 58`, not
   solely to pet type or AI special state 7.
5. Retain native boss, prime-evil, scripted-unit, corpse, mode, state and
   `SwitchAI` protections.
6. Ship no TXT/TSV edits, no `SrvDoFunc 49` fix and no ROADMAP edit.

## Audit gates

- [x] Identify and prove the D2R client target validator and its ABI.
- [x] Prove the high-level aura assignment function used by Aura Enchanted.
- [x] Prove a Revive marker and safe AI-time predicate independent of pet type.
- [x] Audit hook ownership against the pinned five-plugin eezstreet baseline
      and the active RuffnecKk Suite; Cast Triggers uniquely owns `0x48FE20`.
- [x] Remove every Revive Overhaul dependency on `0x48FE20` and cover the
      validator-mediated aura capture path with policy tests.
- [x] Remove the build-name allowlist and add positive/negative fingerprint
      policy tests.
- [x] Build Release x64 with zero warnings and verify the three loader exports.
- [x] Prepare the BKVince-integrated testing harness, full extension stack,
      debug Monster Spawner workflow and protected launcher without starting
      D2R.
- [ ] Cold-start the full active stack on 93847 in mod-local and global scopes.
- [ ] Confirm 92777 coverage by governed native equivalence after the 93847
      runtime matrix passes.
- [ ] Validate client targeting, aura behavior and owner-following in gameplay.

## Prepared BKVince 93847 testing harness

`C:\Games\Diablo II Resurrected\mods\BKVince\testing\revive-overhaul` augments
the existing BKVince runtime; it is not a separate mod and does not create a
separate save path. It contains the 2.1.1 test configuration, diagnostics,
static guards, evidence collection and the gameplay matrix while preserving
the complete global and mod-local extension stack. The built-in D2RLoader
debug mode and Monster Spawner are the planned source of normal, Champion,
Unique, SuperUnique and Aura Enchanted corpses.

The launch script requires an explicit `-ConfirmGO` switch and refuses to run
when the static manifest, runtime image, loader image, Revive TXT contract or
extension inventories drift. It also rejects the obsolete separate
`ReviveOverhaulLab93847` profile if it reappears. Preparation performed no cold
start or gameplay test. Act-boss cases remain a second phase after the main
matrix passes.

Two independent MSVC Release builds are byte-identical at 190,976 bytes,
SHA-256 `22137F62C0293B3349F27DC480DADC06D365C880FDD1836E0BA031CCE862AEDC`.
CTest passes `1/1`, the binary exposes the three required D2RLoader exports,
reports version 2.1.1 and embeds PluginSDK API manifest 3. The package and
BKVince mod-local copies match that hash. No D2R 2.1.1 cold start or gameplay
test has been run. The final BKVince harness preflight passes every static
guard and reports `readyForAuthorizedTest: true` with no runtime process active.

The build pins PluginSDK commit
`4933e2c42cb2592958cd0df3b6dc5003102252d1`. The installed runtime baseline is
D2RLoader 1.2.0-beta, while the plugin manifest remains API 3. The pinned
eezstreet PluginPack reference
`dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` is clean and contains no Revive
surface. Cast Triggers is optional: its presence or absence changes no Revive
Overhaul ABI or startup requirement.

## Rollback

The rollback artifact is Revive Overhaul 2.0.1. Removing or disabling the new
DLL restores the previous runtime behavior; the plugin writes no save data and
ships no table changes.
