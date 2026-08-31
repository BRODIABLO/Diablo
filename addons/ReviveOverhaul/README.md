# Revive Overhaul 2.1.1

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
- Triggers native catch-up sooner and uses configurable follow and velocity
  values only on monsters carrying the native Revive state. This remains true
  for custom skills that intentionally use a non-vanilla `PetType`.
- Allows Champions, Uniques and SuperUniques only when the corpse still passes
  the native `Revive`, corpse-consumption, state, mode and `SwitchAI` checks.
- Reactivates the exact aura skill already selected by Aura Enchanted through
  the same complete native assignment path used by the UMod; it does not
  choose, create or reroll an aura.

Cast Triggers remains the unique owner of the shared native target resolver.
Revive Overhaul obtains the exact corpse from the vanilla Revive target
validator instead, so neither plugin depends on load order and Revive Overhaul
also remains autonomous when Cast Triggers is absent.

Act bosses, prime evils, scripted or quest-controlled monsters, monsters with
no selectable consumable corpse and monsters without `SwitchAI` remain
protected by the native engine. No TXT/TSV edit is included.

Use the `revive-overhaul` D2RLoader console command to inspect the active
configuration and counters.

## Skills.txt setup

Keep the current D2R Revive callback contract for Revive or a copied custom
skill:

```text
SrvStFunc = 21
SrvDoFunc = 58
CltStFunc = 24
CltDoFunc = blank
SelectProc = 3
TargetCorpse = 1
```

Do not apply the legacy workaround that clears `SrvStFunc`, uses
`CltStFunc = 39`, `CltDoFunc = 36` and `SelectProc = 2`. That route bypasses
the current client/server Revive contract and is not the configuration this
plugin validates.

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
