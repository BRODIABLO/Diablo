# MeleeSplash 0.1.0

Splashes melee damage onto nearby monsters.

`MeleeSplash.dll` is a standalone RuffnecKk plugin for D2RLoader. It is
generic, disabled by default, and contains no required mod name, path, custom
skill, or custom stat ID.

## Supported scope

- Diablo II: Resurrected 3.2.92777 only;
- D2RLoader, installed globally or inside a mod;
- offline/local single-player;
- player melee attacks against monsters;
- normal attack and explicitly filterable skill IDs.

Not supported or not qualified in 0.1.0: multiplayer, PvP, mercenaries,
summons, monster attackers, unapproved multi-hit skills, or any other D2R
build. Hits whose native physical-conversion field is active are rejected by
0.1.0 because the 92777 conversion is inline and cannot be safely replayed from
the captured pre-critical packet. Public does not mean universal support.

## Install

Copy the strict public package to either scope:

```text
<D2R>/d2rloader/plugins/MeleeSplash.dll
<D2R>/d2rloader/config/MeleeSplash.json

<D2R>/mods/<mod>/d2rloader/plugins/MeleeSplash.dll
<D2R>/mods/<mod>/d2rloader/config/MeleeSplash.json
```

The shipped JSON has `"enabled": false`. Review the options and perform a cold
restart after changing it. Removing the DLL and configuration restores the
host's original behavior on the next process launch; hot unload is not a
supported rollback path.

## Gameplay contract

On an eligible successful melee hit, the plugin captures one native offensive
packet before Critical/Deadly modifies its physical component. It excludes the
primary target, enumerates nearby monsters at 360 degrees, deduplicates them by
type/GUID, and re-resolves every target immediately before applying damage.

For each secondary target independently, 0.1.0:

- rolls the current authoritative D2R 3.2.92777 Critical/Deadly sequence;
- recalculates resistances, reductions, and absorb;
- leaves native Crushing Blow and Open Wounds enabled, producing new rolls;
- applies half normal life and mana leech;
- finalizes reactions, death, packets, and kill credit through native code.

The pre-critical physical, magic, elemental, burn, and poison packet is shared
and scaled before the per-target Critical/Deadly roll; the Critical/Deadly,
Crushing Blow, and Open Wounds outcomes are not. The
92777 Critical/Deadly adapter is deliberately isolated so a future host-owned
resolver can replace it without making this plugin the owner of general combat
formulas. Because current D2R exposes no callable Critical/Deadly resolver, a
future PD2 resolver will require a deliberate provider integration and a new
MeleeSplash build; 0.1.0 cannot discover a new ×1.5 Deadly formula from the
shared native result bit alone.

Synthetic splash resolution never launches a missile, creates Next Hit Delay,
reruns durability or thorns, or recursively creates another splash. Knockback,
blind, freeze, slow, offensive CTC, get-hit CTC, stun, and secondary missiles
are filtered from synthetic targets. The primary hit is passed through once.

## Configuration

See [OPTIONS.md](OPTIONS.md) for every key and
[`examples/MeleeSplash.enabled.example.json`](examples/MeleeSplash.enabled.example.json)
for a generic enabled profile. Invalid JSON, unknown keys, invalid values,
unsupported builds, signature mismatches, or hook ownership conflicts fail
closed.

The plugin captures the skill that actually owns the hit; it does not rely on
the player's currently selected skill. `excludedSkillIds` always wins. The
optional gate, radius, and damage stats are IDs supplied by the host profile;
the public DLL hardcodes none of them.

## Compatibility and ownership

The plugin does not modify, link, or redistribute any eezstreet DLL. Its owned
write sites are documented in [NATIVE-HOOKS.md](NATIVE-HOOKS.md), validated
before activation, and compared against the current workspace's PluginPack
manifest, FloatingDamage owner, standalone addons, and active BKVince patches.
No exact collision is accepted. Compatibility claims apply only to the
explicitly tested versions and full active stack recorded for this release,
not to unknown future plugins.

The generic DLL does not disable another mod's legacy splash. A host may opt
into a narrowly configured EventFunc20 suppression token; BKVince's separate
default-off profile uses this reversible integration without changing its
historical skill or missile tables. The reserved IDs, owned files, deployment
paths, and cold rollback are recorded in
[BKVINCE-INTEGRATION.md](BKVINCE-INTEGRATION.md).

## Diagnostics

Set `diagnosticLogging` to `true` only for a short local test. The log records
the attacker, actual skill, weapon tier, primary and secondary identities,
radius, splash percentage, shared packet, per-target Critical/Deadly, CB, OW,
post-resistance damage, raw/splash/native-adjusted leech percentages, rejection
reasons, recursion guard, and setup errors. The native leech consumer does not
leave the separately credited life/mana amounts in the damage record, so 0.1.0
does not mislabel those unavailable amounts. Normal operation avoids per-hit
diagnostic spam.

The read-only `melee-splash` console command reports whether the plugin is
disabled, active, or fail-closed and shows bounded counters. It never changes
gameplay.

## Credits

Authored by **RuffnecKk**.

Project Diablo 2 provided the gameplay reference. D2MOO provided semantic
reference material for legacy damage events, unit enumeration, item/skill
selection, and damage lifecycle responsibilities. No D2MOO address, structure,
ABI, or source code is transplanted into the D2R 3.2.92777 implementation.

The strict public ZIP contains only `MeleeSplash.dll` and `MeleeSplash.json`.
Sources, this README, symbols, logs, and reverse-engineering evidence remain
outside that archive.
