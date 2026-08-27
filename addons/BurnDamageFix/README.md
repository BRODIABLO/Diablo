# Burn Damage Fix

Burn Damage Fix restores generic Burn damage production and makes applied Burn
respect the game's live Fire Resistance rules while keeping a moving flame on
the affected target.

The plugin has no Diablo II: Resurrected build or version allowlist. It records
the observed build name for diagnostics only, then validates every native RVA,
signature, layout, ABI witness and hook surface it uses. A matching unnamed or
future build may load; any fingerprint difference refuses safely before the
first hook is installed.

The plugin corrects two independent native defects:

- generic Burn producers use `burningmin`, `burningmax` and
  `passive_fire_mastery` instead of adding the numeric stat ID `316` as flat
  damage;
- Burn is resolved through the current Fire Resistance path before the
  resulting negative life regeneration is stored;
- the target-bound `fire_hit` effect is replayed while the real native
  `burning` state remains active, and the stationary native ground flame is
  suppressed process-locally.

Resistance, maximum resistance, difficulty, immunity and attacker Fire pierce
remain native. Magic Damage Reduction and Fire Absorb are intentionally
excluded. Burn duration, tick scheduling, saves, packets, kill attribution and
experience are not changed.

The overlay replacement never creates or extends Burn. It only observes the
native `burning` state, replays `fire_hit` on the affected unit, and suppresses
the original stationary overlay when that row still contains vanilla ID `224`.
A pre-suppressed row is accepted and any custom overlay ID is preserved.

## Installation

Remove the historical `BurnFireResistance.dll` first. Never load it together
with `BurnDamageFix.dll`.

Install `BurnDamageFix.dll` in either:

- `<D2R>/d2rloader/plugins/`; or
- `<D2R>/mods/<mod>/d2rloader/plugins/`.

Place `burn-damage-fix.toml` in the matching `d2rloader/config/` directory.
The embedded defaults are safe if the file is absent. Mod-local configuration
takes priority over scope-local and global configuration.

The console command `burn-damage-fix` reports the observed build,
configuration, suppression state and optional diagnostic counters.

## Compatibility contract

Burn Damage Fix owns only the internal generic-production seam `0x44CB32` and
the Burn-application entry `0x451380`, plus the player and monster event
dispatchers used for periodic overlay replay. It deliberately does not own the
shared resolver at `0x4523E0`; it calls the live address so Monster Display and
Resistance Floor remain in the chain regardless of load order.

The D2RLoader API version check is retained because it validates the loader ABI,
not the Diablo build. Removing that check could interpret an incompatible
plugin context layout and crash before the native fingerprint can be evaluated.

Version 2.2.0 was tested on the official D2R 3.3.93847 runtime. D2R 3.2.92777 is
covered by governed byte-identical native surfaces; this evidence coverage does
not restrict loading to either named version.

Static ownership checks found no overlap with Bind And Summon, Melee Splash,
the five eezstreet PluginPack DLLs, or the active RuffnecKk Suite. See
`NATIVE-HOOKS.md` and `Mission/burn-damage-fix.md` for the governed evidence and
the runtime matrix.

The independent `ruffneckk-thorns-burn-kill-credit.json` memory patch remains
responsible for kill attribution and experience credit.

## Release candidate 2.2.0

- DLL size: 193,536 bytes
- DLL SHA-256:
  `5A7B4D3304CE1802DAB56EEB2787AFC16DB066F42EB5DE9BD2E999AA2F8F6752`
- TOML SHA-256:
  `8C2831F4A1BE9647757DDBDE7C9FE089FFF7008181DB9A0B5AD45FA4E9BB17C9`
- ZIP SHA-256:
  `DA90303028A3A2D1030A8935B042151F4DA209F3075FDA008D335E36425B6FA8`
- ZIP contents: `BurnDamageFix.dll`, `burn-damage-fix.toml`

The ZIP stays beside this README and does not contain the README. Runtime and
gameplay evidence and any remaining release gates are tracked in the mission.

## Credits

Authored by RuffnecKk. D2MOO provided the pinned semantic reference used to
identify Burn stat production, native random-range behavior and Burn state
application. No D2MOO address, structure or 32-bit ABI is reused.
