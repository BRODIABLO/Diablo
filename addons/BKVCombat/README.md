# BKVCombat

`BKVCombat.dll` applies independently configurable PD2-inspired combat rules.

It is a standalone RuffnecKk plugin for D2RLoader. It can be installed globally
or inside a mod and does not modify, link, or redistribute any eezstreet DLL.
The first supported native build is D2R `3.2.92777` in offline/local play.
Multiplayer and PvP remain unqualified until an explicit host/joiner matrix is
completed.

## Release plan

Release 1 owns narrow policies, not the whole hit pipeline:

- Critical Strike: 75% cap and 2.0x physical damage;
- Deadly Strike: 75% cap and 1.5x physical damage, tested only after Critical;
- Crushing Blow: BKVince target classes, player-count scaling, ranged penalty,
  Crushing Blow Efficiency, current HP, and physical resistance;
- Life Steal and Mana Steal by validating and retaining the authoritative
  native PD2-equivalent difficulty/Drain baseline;
- Open Wounds: five-second duration, up to three native stat-list stacks,
  physical resistance, no old boss penalty, and quarter damage to mercenaries
  and owned pets.

Release 2 is reserved for the separately proved dual-wield/IAS/WSM attack
engine. Release 3 is reserved for true native PD2 melee splash. Resistances and
drops are outside Release 1.

## Crushing Blow classes

The exact priority is:

1. `MajorBoss`;
2. `PrimeEvil`;
3. `Elite`;
4. `Ordinary`.

Heralds and Ascendants are Elites for Crushing Blow. Champions, uniques,
superuniques, ordinary bosses, and the runtime Herald/ghostly marker are also
Elites. The ten MajorBoss identities live in JSON, are verified against the
active `monstats.txt` key and `*hcIdx`, and never appear as a compiled C++ boss
allowlist.

| Class | Melee | Ranged |
|---|---:|---:|
| Ordinary | 1/6 | 1/9 |
| Elite | 1/8 | 1/12 |
| PrimeEvil | 1/16 | 1/24 |
| MajorBoss | 1/20 | 1/30 |

Crushing Blow Efficiency increases the amount after the target class, ranged,
and live player-count factors are known. The public plugin uses a configurable
stat ID; the BKVince profile reserves stat `393`. No upper cap is invented.

Prime Evil buffs such as universal slow immunity belong to the Monster Merge,
not to BKVCombat's Crushing Blow classifier. BKVCombat only consumes the
classification; it does not own slow, curse-control, resistance, or drop rules.

## Installation

Copy `BKVCombat.dll` and `BKVCombat.json` into either:

```text
<D2R>/d2rloader/plugins/
<D2R>/d2rloader/config/
```

or:

```text
<D2R>/mods/<mod>/d2rloader/plugins/
<D2R>/mods/<mod>/d2rloader/config/
```

The mod-local plugin shadows a global copy with the same plugin ID. The shipped
configuration is disabled. Enable only the policies that have passed the gates
listed in [VALIDATION.md](VALIDATION.md), then cold-start D2R. D2RLoader has no
hot unpatch transaction for these internal callsites. Open Wounds callbacks
also require the DLL to remain loaded for the session; disable or remove the
plugin only before a cold start.

## Compatibility model

BKVCombat validates build identity, every expected byte context, every helper
signature, and every hook owner before its first write. All wrappers remain
pass-through until installation completes atomically. If a late write fails,
the DLL stays loaded but inert; a cold restart with the plugin disabled or
removed is the rollback.

Its internal callsites do not overlap the five PluginPack manifests,
FloatingDamage, BurnFireResistance, MeleeSplash's entry hooks, or current
BKVince JSON patches. MeleeSplash may consume BKVCombat's optional versioned
Critical/Deadly provider for synthetic targets; CB and OW continue through the
native event handlers and are never double-called through the provider.

This static ownership result is not a universal compatibility claim. A release
still requires the complete installed plugin stack, every PluginPack feature
enabled, both relevant load orders, and zero owner rejection. Pre-existing
third-party conflicts must remain visible rather than being hidden by disabling
components.

The current installed stack has now passed technical cold starts in both exact
BKVCombat/MeleeSplash orders with 19/19 plugins, 15/15 patchsets and frontend
24/24. This remains a bounded current-stack result: several PluginPack features
are disabled in the baseline, two pre-existing render failures remain open, and
the lazy MeleeSplash provider negotiation still needs a gameplay hit.

## Documentation

- [OPTIONS.md](OPTIONS.md) documents every JSON field.
- [NATIVE-CONTRACT.md](NATIVE-CONTRACT.md) records the governed 92777 seams,
  order, ABI, ownership, and fail-closed gates.
- [VALIDATION.md](VALIDATION.md) separates build/static results from pending
  runtime and gameplay evidence.
- [SMOKE-TEST.md](SMOKE-TEST.md) defines the bounded offline/local witnesses
  and cold rollback.
- [CHANGELOG.md](CHANGELOG.md) tracks public changes.

## Credits

Created by **RuffnecKk** for the D2RLoader community and BKVince.

D2MOO was used only as a semantic reference while identifying Diablo II combat
concepts. All D2R addresses, signatures, layouts, and x64 ABIs used by this
plugin were independently proved against D2R `3.2.92777`.
