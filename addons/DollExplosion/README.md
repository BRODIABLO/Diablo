# Doll Explosion

Doll Explosion delays classic Stygian Doll death blasts and makes their
physical damage configurable.

Status: **statically qualified incubation 0.1.0**. The source, configuration,
strict build and policy tests pass, but this build is not a public release and
has not yet passed the D2R runtime matrix.

## Default behavior

The shipped TOML targets the seven classic BKVince Doll records (MonStats IDs
212, 213, 214, 215, 216, 690 and 691). The custom Rift Doll 777 is excluded and
keeps its existing native `deathDmg` behavior.

The default follows the Project Diablo 2 Season 13 data model:

| Difficulty | Physical damage | Delay | Radius |
|---|---:|---:|---:|
| Normal | 18-30 | 25 frames | 4 |
| Nightmare | 54-96 | 25 frames | 4 |
| Hell | 318-540 | 25 frames | 4 |

The fixed ranges are displayed hit points and are converted to D2R fixed-point
damage with `HitShift=8`. A bounded `source_max_life_percent` formula is also
available. Arbitrary expressions are intentionally unsupported.

## Configuration

Edit `ruffneckk-doll-explosion.toml`. The plugin resolves the first existing
file in this order:

1. the active mod's `d2rloader/config` directory;
2. the DLL's current load-scope configuration directory;
3. the global `<D2R>/d2rloader/config` directory.

An absent file uses the embedded PD2 defaults and is materialized in the
current scope when possible. A present but invalid file refuses plugin loading;
unknown keys, duplicate target IDs and unsafe bounds are errors.

The DLL is active by being present. There is no global `enabled` switch.

## Installation

The same DLL can be installed globally in `<D2R>/d2rloader/plugins/` or locally
in `<D2R>/mods/<mod>/d2rloader/plugins/`. Do not install both copies at once.

Use the `doll-explosion` console command to show the effective settings and
diagnostic counters.

## Native safety model

The plugin does not select compatibility from a D2R version or distribution
channel. Before installing either hook, it validates every used entry,
callsite, layout witness and dispatcher relationship against the governed
native fingerprint. It also requires both hook surfaces to be unowned when the
D2RLoader Diagnostics service is available.

The delayed carrier is the native invisible `baalcorpseexplodedelay` missile
(ID 587). Its governed D2R/BKVince row has no server hit callback or damage, so
an orphaned carrier expires harmlessly. Foreign monsters and missiles always
delegate to their original callbacks. The configured delay updates both the
native total-frame value and its current-frame countdown through fingerprinted
engine setters, then verifies both through fingerprinted getters.
Game-join and game-leave lifecycle events clear the bounded sidecar so abandoned
carriers from a destroyed session cannot consume slots in the next session.

See [NATIVE-CONTRACT.md](NATIVE-CONTRACT.md) for the complete ABI and ownership
contract and [VALIDATION.md](VALIDATION.md) for the current qualification state.

## Evidence and credits

The PD2 default was derived from the pinned Season 13 data chain, not from a
static disassembly of `ProjectDiablo.dll`: the 100% death-skill property selects
`DollMeteor`, its difficulty levels select the three fixed ranges, the skill's
calculation supplies radius 4, and `dollmeteorcenter` supplies a 25-frame
lifetime. The lifetime is a strong data-and-semantics inference until PD2's
proprietary runtime path is independently observed.

The D2R implementation itself was statically reverse engineered against the
governed common D2R 3.2.92777 / 3.3.93847 corpus, with exact signatures and ABI
witnesses. No PD2 code or assets are copied.

Credit to the [D2MOO](https://github.com/ThePhrozenKeep/D2MOO) project for the
Diablo II 1.10f semantic reference used to understand the historical monster
death and missile callback model. D2MOO addresses, 32-bit structures and ABI
are not reused.

Author: **RuffnecKk**
