# RuffnecKk Cast Triggers — D2R 3.3.93847

## Mission

Create a public, intermod D2RLoader plugin that lets mod authors attach native
chance-to-cast effects to items and trigger them when a player casts another
skill. The plugin is an autonomous member of the RuffnecKk D2RLoader Suite and
is not part of BKVince.

Vincent approved implementation on **24 August 2026**. The first bounded
release supports:

- a fixed target-skill level through `cast-skill`;
- the effective source-skill level through `cast-skill-same-level`;
- successful manual player casts using the player cast animation;
- non-repeating skills only;
- native item chance rolls and native item-skill targeting.

The first release deliberately excludes attacks, repeating/channelled skills,
trigger chains, critical/crushing/open-wounds triggers and timed while-
channeling execution. Those mechanisms require separate native proof and
balancing gates.

## Product contract

- DLL: `d2rl-ruffneckk-cast-triggers.dll`
- Config: `ruffneckk-cast-triggers.toml`
- Author: `RuffnecKk`
- Scope: global or mod-local, never both at once
- Consumer data: the active mod owns its ItemStatCost, Properties, item rows,
  localization keys and permanent numeric IDs
- Plugin data: no item mapping and no BKVince table dependency
- Saves: native item-stat encoding only; the plugin adds no proprietary save
  payload
- Multiplayer: the authoritative server skill handler dispatches the proc

## Native contract

The implementation targets D2R `3.3.93847` and uses the governed native corpus
whose historical image provenance is `3.2.92777`.

- `0x43ACB0`: central server skill handler. A successful manual execution uses
  `a5=1`, `a6=0`, `a7=0`.
- `0x44D570`: safe wrapper around the unit-stat event dispatcher. Event `4`
  is `doactive` (unit used a skill).
- `0x583B30`: native EventFunc20 chance-to-cast callback. Melee Splash already
  owns this entry and forwards the native callback outside its synthetic
  context; Cast Triggers never hooks this entry.
- `0x5896E0` and `0x589820`: native target and position item-skill casters.
  Cast Triggers owns these two entries and changes level zero only while its
  own `doactive` dispatch is active.
- `0x097790`: context-aware SkillsTxt lookup. The compiled record stride is
  `0x2EC`; flags are at `+0x24`, player animation at `+0x30` and sequence
  transition at `+0x32`.

The source filter accepts player cast mode `SC` and non-repeating sequence mode
`SQ` transitioning to `SC`. It rejects the native `repeat` flag, weapon attack
animations, non-player units, triggered item casts and nested dispatches.

## Encoding contract

Mod authors add two ItemStatCost rows using `doactive` and EventFunc20, plus
two Properties rows using native property function 11. The fixed property uses
its normal `max` level. The same-level property reserves `max=64`: native
function 11 masks the encoded level to six bits, producing the internal level
zero marker. Cast Triggers replaces that marker with the successful source
skill's effective level at the final native item-skill caster.

Every consuming mod chooses and permanently freezes its own free numeric IDs.
The DLL never embeds those IDs.

## Validation gates

1. Policy and TOML tests pass.
2. Release x64 DLL builds against the governed PluginSDK baseline.
3. Hook signatures and call ownership are exact for build 93847.
4. A disposable intermod fixture proves fixed level, same source level, 25%
   chance, 100% chance, attack exclusion, repeating-skill exclusion and no
   proc chains.
5. Full-stack cold start retains every active RuffnecKk Suite component and all
   five eezstreet plugins/features.
6. Global and mod-local installation scopes are both qualified, without a
   duplicate installation.
7. The public ZIP contains only the DLL and TOML; README files remain beside
   the ZIP for Vincent's review.

## Rollback

Remove the DLL and TOML. Items keep their native encoded stats, but the custom
`doactive` event has no plugin-generated dispatch and therefore produces no
cast-on-cast effect. No character or shared-stash migration is required.

## Implementation status — 24 August 2026

The autonomous DLL, strict TOML parser, intermod guide, native fixture builder
and Release tests are implemented. Both global and mod-local loader scopes
accept the plugin on D2R 3.3.93847, and the separate fixture compiles its three
TXT overlays without touching BKVince.

Publication remains blocked by two external/runtime gates:

- D2Prism reports `Present failed` after frontend startup, leaving no visible
  gameplay frame; the fixed-level, same-level, exclusion and chain cases are
  therefore still blocked or not run.
- a Resistance Floor DLL deployed into the active BKVince stack during this
  work currently refuses its own first relay before Cast Triggers loads. The
  earlier 31-plugin full-stack run passed before that component appeared, but
  the mandatory current Suite matrix cannot be declared clean.

Cast Triggers itself continues to load after that independent failure. Do not
publish version 0.1.0 until the gameplay matrix and the current complete Suite
matrix both pass.
