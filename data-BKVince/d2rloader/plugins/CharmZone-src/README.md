# CharmZone 0.3.1

Standalone D2RLoader add-on for **Diablo II: Resurrected 3.2.92777**.

CharmZone keeps charms movable everywhere in the player inventory but only
applies their gameplay effects when the complete item fits inside a configured
rectangle. Inactive charms are covered by a red overlay and display an
explanatory message on hover.

## Requirements

- Diablo II: Resurrected build `3.2.92777`
- D2RLoader with Plugin API v2 support
- `FloatingDamage.dll` 1.1.0 or newer, used as the shared DirectX overlay host

The gameplay hook remains authoritative if the visual host is unavailable. The
plugin verifies every native signature and refuses unsupported game builds.

## Public ZIP contents

- `CharmZone.dll`
- `charm-zone.toml`

## Installation

Install both files in either the global D2RLoader folders or the corresponding
folders of one mod:

```text
<D2R>\d2rloader\plugins\CharmZone.dll
<D2R>\d2rloader\config\charm-zone.toml
```

```text
<D2R>\mods\<mod>\d2rloader\plugins\CharmZone.dll
<D2R>\mods\<mod>\d2rloader\config\charm-zone.toml
```

Restart the game after changing the configuration.

## BKVince preset

BKVince uses an 11 by 8 inventory. The active charm zone is the complete lower
half: columns `0..10`, rows `4..7`. A multi-cell charm crossing the boundary is
fully inactive.

## Behaviour and compatibility

- Native charm eligibility is evaluated first; CharmZone only adds the spatial
  restriction.
- The rule applies to all charm properties handled by the native eligibility
  path, including attributes, resistances, skills, oskills, auras and triggers.
- Items outside the player inventory do not become active.
- The plugin does not write custom data to character saves.
- `FloatingDamage.dll` owns the single DirectX renderer and exposes a bounded
  named-overlay registry, avoiding a second ImGui or swap-chain hook.
- The plugin does not hook the five eezstreet PluginPack DLL entry points.

Use the D2RLoader console command `charm-zone` to display the loaded geometry,
overlay state and enforcement counters.

Author: RuffnecKk
