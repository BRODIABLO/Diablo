# Floating Damage

Shows floating combat numbers and rolling damage per second in D2R.

## Installation

Install the DLL and TOML in one of the following locations.

### Global installation

```text
<Diablo II Resurrected>\d2rloader\plugins\FloatingDamage.dll
<Diablo II Resurrected>\d2rloader\config\floating-damage.toml
```

### Per-mod installation

```text
<Diablo II Resurrected>\mods\<mod>\d2rloader\plugins\FloatingDamage.dll
<Diablo II Resurrected>\mods\<mod>\d2rloader\config\floating-damage.toml
```

Choose only one installation location. Do not load global and per-mod copies at
the same time. If the TOML file is missing, the plugin creates it automatically
from the defaults embedded in the DLL.

Floating Damage 1.2.0 requires D2RLoader and targets `D2R.exe 3.2.92777`. It
refuses unsupported game builds instead of installing an unverified native
hook.

## Damage format and resolution scaling

Damage values use compact notation from 1,000 onward:

```text
999       -> 999
1,000     -> 1k
1,250     -> 1.3k
10,000    -> 10k
1,000,000 -> 1m
1,250,000 -> 1.3m
```

Text sizes in the TOML are 4K/2160p reference pixels. With the default normal
and critical sizes of `38` and `48`, the rendered sizes are approximately:

| Display height | Normal | Critical |
|---:|---:|---:|
| 720p | 12.7 | 16 |
| 1080p | 19 | 24 |
| 1440p | 25.3 | 32 |
| 2160p / 4K | 38 | 48 |

Scaling uses display height, so standard and ultrawide screens with the same
height receive the same text scale.

## Toggle hotkey

The session-only toggle defaults to `CTRL+SHIFT+D`:

```toml
[hotkey]
toggle_hotkey_enabled = true
toggle_hotkey = "CTRL+SHIFT+D"
```

The hotkey is a tap toggle: press it once to hide the overlay and once again to
show it. Supported bindings include `A`-`Z`, `0`-`9`, `F1`-`F24`, navigation
and editing keys, common punctuation, `MOUSE3`, `MOUSE4`, and `MOUSE5`, with
optional exact `CTRL`, `SHIFT`, and `ALT` modifiers.

The plugin only polls the binding while the game window has focus. It never
consumes keyboard or mouse input, so D2R and chat still receive the same input.
An unmodified printable key can therefore toggle the overlay while it is typed
in chat.

## Configuration and console command

The TOML controls appearance, animation, hit combining, number layout, the DPS
counter, preview values, colors, and the toggle hotkey. Reload it with:

```text
floating-damage reload
```

Available commands:

```text
floating-damage [status|on|off|toggle|preview|reload|reset]
```

The hotkey changes visibility for the current session without rewriting the
TOML. The console `on`, `off`, and `toggle` commands persist the `enabled`
setting.

## Technical notes

The plugin captures client-observed post-resistance damage and renders it
through DirectX 12/ImGui. It is visual-only: it does not change combat values,
packets, kills, experience, loot, or server simulation. Version 1.1.0 added the
named overlay registry used by compatible RuffnecKk plugins such as CharmZone.

The public ZIP contains only `FloatingDamage.dll` and
`floating-damage.toml`. Source code, documentation, logs, and build artifacts
are intentionally excluded from the archive.

## Credits

- Original D2RLAN/D2RHUD floating-damage renderer: d2rlan.
- D2RLoader 3.2 port and plugin integration: RuffnecKk.
- Diablo II combat reference: D2MOO project.

## Building from source

Build with Visual Studio 2022 and CMake 3.28 or newer:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```
