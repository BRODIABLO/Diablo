# Revive Overhaul 2.3.0

Improves Necromancer Revive owner-following and safely allows eligible
Champions, Uniques and SuperUniques to be revived without losing their native
Aura Enchanted aura.

## Install

Copy the DLL and TOML to either the global or mod-local D2RLoader folders. Do
not install the same plugin in both scopes.

```text
d2rloader/plugins/d2rl-ruffneckk-revive-overhaul.dll
d2rloader/config/ruffneckk-revive-overhaul.toml
```

The legacy `ReviveOverhaul.toml` filename remains accepted for upgrades from
1.1.0. The new Suite filename takes precedence when both files exist in the
same scope.

The plugin does not accept or reject a runtime by build-name allowlist. It
records the reported identity, then refuses cleanly unless every native entry,
return site and ABI witness used by the enabled features matches its complete
fail-closed fingerprint. Builds 92777 and 93847 must still be qualified in
separate runtime matrices.

## Behavior

- Stops Revives from scattering when already beside their owner.
- While a Revive has no target and is not in combat, triggers native catch-up
  sooner and uses configurable follow and velocity values. As soon as combat
  or a target is present, vanilla AI runs without those leash transforms.
- Allows Champions, Uniques and SuperUniques only when the corpse still passes
  the native `Revive`, corpse-consumption, state, mode and `SwitchAI` checks.
- Reactivates the exact aura skill already selected by Aura Enchanted through
  the same complete native assignment path used by the UMod; it does not
  choose, create or reroll an aura.
- Optionally lets RuffnecKk Scripted AI execute one bounded target action for
  melee, ranged or caster Revives. A handled action replaces the original
  callback for that tick; every delegation still invokes it exactly once.

The client high-rank gate is redirected only at Revive's own native callsite;
the shared AI eligibility function remains unhooked and every non-rank client
restriction continues natively. Cast Triggers remains the unique owner of the
shared native target resolver.
Revive Overhaul obtains the exact corpse from the vanilla Revive target
validator instead, so neither plugin depends on load order and Revive Overhaul
also remains autonomous when Cast Triggers is absent.

Version 2.3.0 automatically bridges Revive's compiled callback route after
each table load. It verifies all three Skills banks before changing only the
Revive row in memory; no `Skills.txt` edit is required and the source tables
on disk remain untouched.

Version 2.3.0 also probes the optional Scripted AI Revive-tactics ABI V3 without
loading that DLL. The provider is accepted only when its version, magic,
interface size and tactical-action capability match. A temporary module
reference keeps each call safe across load order and unloading. Missing,
disabled, incompatible, unavailable or failing providers preserve native
Revive AI.

To test the shipped tactical director, install RuffnecKk Scripted AI `0.7.0`,
copy its `revive-companion.lua` sample to its configured script directory, and
set both `enabled = true` and `[domains.revive].enabled = true` in
`ruffneckk-scripted-ai.toml`. Revive Overhaul itself needs no new setting and
has no hard dependency on Scripted AI; its existing `[ai].enabled` setting must
remain `true` for the Revive AI hook to offer the optional tactical call.

The shipped tree automatically classifies the compiled monster record. Casters
rotate eligible native spell slots and kite, physical ranged Revives alternate
attack and retreat, and melee Revives aggressively chase a locked target. The
target and Revive must remain inside the owner's bounded combat radius. When no
target exists, or the hard leash is crossed, the plugin delegates to the native
follow/pathfinding logic; Scripted AI never chases the owner directly.

Act bosses, prime evils, scripted or quest-controlled monsters, monsters with
no selectable consumable corpse and monsters without `SwitchAI` remain
protected by the native engine. No TXT/TSV edit is included.

Use the `revive-overhaul` D2RLoader console command to inspect the active
configuration and counters.

## Skills.txt setup

No manual callback edit is required. Keep the normal current D2R Revive row on
disk:

```text
SrvStFunc = 21
SrvDoFunc = 58
CltStFunc = 24
CltDoFunc = blank
SelectProc = 3
TargetCorpse = 1
```

After compilation, the plugin verifies that exact source contract and applies
the proven high-rank route in memory (`SrvStFunc blank`, `SrvDoFunc 58`,
`CltStFunc 39`, `CltDoFunc 36`, `SelectProc 2`). The authoritative
`SrvDoFunc 58` validator hook still rechecks every admitted corpse. If any
bank contains an unknown callback combination, the automatic route refuses
instead of overwriting it.

For each monster row that should remain softcode-eligible, the current data
must still provide the native `CorpseSel`, `Revive` and `SwitchAI` permissions.
The plugin does not ship table edits and does not disable the engine's boss,
prime-evil, scripted-unit, state, mode or corpse-consumption protections.
`PetType = revive` is the safest vanilla setup; a custom pet type may be kept
when its summon limits and ownership behavior are intentional.

## Compatibility

- Runtime matrix: Diablo II: Resurrected 3.3.93847.
- Native-equivalence coverage: build 92777 while every used surface remains
  governed byte-identical.
- Loader: current governed D2RLoader/PluginSDK baseline.
- Scope: global or mod-local; the plugin is not mod-scoped-only.
- Ownership: hooks seven independently signed native entries and refuses to
  activate on any signature, return-site or ABI mismatch.

## Credits

- Author: `RuffnecKk`.
- Ogodei and the Phrozen Keep community for the original Revive AI and
  high-rank Revive investigations.
- D2MOO is credited as the semantic reference for the legacy Revive, minion AI,
  Aura Enchanted and active-skill behavior. All D2R addresses, signatures,
  structures and ABI were independently proven for the governed current
  runtime corpus.
