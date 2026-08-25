# Burn Damage Fix

Burn Damage Fix restores generic Burn damage production and makes applied Burn
respect the game's live Fire Resistance rules. One DLL supports exactly Diablo
II: Resurrected builds 3.2.92777 and 3.3.93847 and refuses every other build.

The plugin corrects two independent native defects:

- generic Burn producers use `burningmin`, `burningmax` and
  `passive_fire_mastery` instead of adding the numeric stat ID `316` as flat
  damage;
- Burn is resolved through the current Fire Resistance path before the
  resulting negative life regeneration is stored.

Resistance, maximum resistance, difficulty, immunity and attacker Fire pierce
remain native. Magic Damage Reduction and Fire Absorb are intentionally
excluded. Burn duration, tick scheduling, saves, packets, kill attribution and
experience are not changed.

When diagnostics are enabled, the plugin also observes the native `burning`
state after a positive Burn application. This overlay witness reports
active/missing counters only: it never forces the state or creates an overlay.

## Installation

Remove the historical `BurnFireResistance.dll` first. Never load it together
with `BurnDamageFix.dll`.

Install `BurnDamageFix.dll` in either:

- `<D2R>/d2rloader/plugins/`; or
- `<D2R>/mods/<mod>/d2rloader/plugins/`.

Place `burn-damage-fix.toml` in the matching `d2rloader/config/` directory.
The embedded defaults are safe if the file is absent. Mod-local configuration
takes priority over scope-local and global configuration.

The console command `burn-damage-fix` reports the active build, configuration
and optional diagnostic counters.

## Compatibility contract

Burn Damage Fix owns only the internal generic-production seam `0x44CB32` and
the Burn-application entry `0x451380`. It deliberately does not own the shared
resolver at `0x4523E0`; it calls the live address so Monster Display and
Resistance Floor remain in the chain regardless of load order. The optional
overlay witness borrows `STATES_CheckState` at `0x3351B0` without patching it.

Static ownership checks found no overlap with Bind And Summon, Melee Splash,
the five eezstreet PluginPack DLLs, or the active RuffnecKk Suite. See
`NATIVE-HOOKS.md` and `Mission/burn-damage-fix.md` for the governed evidence and
the runtime matrix.

The independent `thorns-and-burn-kill-credit.json` memory patch remains
responsible for kill attribution and experience credit.

## Release candidate 2.0.0

- DLL size: 164,864 bytes
- DLL SHA-256:
  `56555AA60FC284CA6A691526827124E9BFA7E9D8C6312C09523AC7AFD522C48B`
- ZIP SHA-256:
  `4C4C118D40656CADB395F41B671EE47C2820D2D0FB3C60E7F3B2E55C4B3196F0`
- ZIP contents: `BurnDamageFix.dll`, `burn-damage-fix.toml`

The ZIP stays beside this README and does not contain the README. Runtime and
gameplay release gates are tracked in the mission and must be green before
publication.

## Credits

Authored by RuffnecKk. D2MOO provided the pinned semantic reference used to
identify Burn stat production, native random-range behavior and Burn state
application. No D2MOO address, structure or 32-bit ABI is reused.
