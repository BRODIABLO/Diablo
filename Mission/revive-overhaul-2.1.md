# Revive Overhaul 2.1 — native targeting, aura activation and AI scope

Date: 2026-08-31

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
2. Own the narrow client corpse-eligibility decision needed to make eligible
   high-rank corpses targetable while retaining server authority.
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
