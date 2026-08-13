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

Floating Damage 1.2.9 requires D2RLoader and targets `D2R.exe 3.2.92777`. It
refuses unsupported game builds instead of installing an unverified native
hook.

Version 1.2.9 also makes DirectX 12 overlay ownership deterministic when
`plugin-items.dll` embeds ExtendedItemStats. Floating Damage detects that
renderer, lets its fallback hook install first, then installs the shared
Floating Damage overlay as the outer hook. This preserves both tooltips and
combat overlays regardless of plugin scan order.

## Damage format and resolution scaling

D2R stores hit points and damage in 8.8 fixed-point units. Floating Damage now
measures the target's whole-number HP immediately before and after the native HP
commit. The popup therefore matches the HP loss visible to the player,
including fractional carry between consecutive hits. It no longer truncates
each physical, fire, lightning, magic, cold, or poison component separately.

For example, a fixed-point hit just below `4.0` can move the visible target HP
from `20` to `16`. Version 1.2.1 displays `4`, while the old component-level
conversion displayed `3`.

Every gameplay popup stores only the target monster's client unit identifier.
Version 1.2.8 places every damaged target in a bounded atomic request registry.
After D2R updates its gameplay camera on the native client frame, the plugin
resolves and projects every requested target through D2R's original projection
function, then publishes the latest coordinates through a fixed atomic cache.
The DirectX overlay only reads that cache; it never calls the renderer from the
wrong thread. Native UI coordinates are converted to the active overlay
dimensions, supporting 720p, 1080p, 1440p, 4K, and ultrawide displays without
guessed isometric coefficients. No pointer to a living or dead monster is
retained.

Versions 1.2.3 and 1.2.4 attempted to reconstruct D2R's camera transform from
world coordinates and the local player position. Version 1.2.5 removed that
approximation but called the native projection from the DirectX Present thread,
where D2R's thread-local render context was unavailable. Version 1.2.6 kept the
exact projection on its original game thread, but passively observed only the
units that D2R selected for its own UI. That caused group hits without popups
and allowed a popup to become hidden after the 250 ms cache window. Version
1.2.7 actively projected every damaged target, but still waited for one of
D2R's conditional UI consumers to enter the projection function. Moving the
camera could therefore leave only the principal target refreshed. Version
1.2.8 instead services the complete request registry from D2R's unique
per-frame camera update, after the camera offsets are current, so group targets
do not depend on labels, hover state, or the selected monster.

The capture is independent of the damage source. Hits from any summon, Revive,
reanimated ally, mercenary, trap, skill, missile, melee attack, or ranged
attack use the same target-unit projection. A missing target, failed native
projection, or coordinate outside the current display is hidden, so off-screen
deaths never fall back to a player-centered position.

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
height receive the same text and world-projection scale. Widescreen width only
changes the horizontal centre; it does not stretch the isometric geometry.

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

The plugin redirects only the unique native main-HP commit call, reads the
target's HP through the native unit-stat accessor, invokes the original setter
with unchanged arguments, and renders the resulting visible loss through
DirectX 12/ImGui. The HP hook queues only the monster's type and identifier.
The native camera-frame hook first invokes D2R's original camera update
unchanged. It then obtains that thread's active renderer context and services a
deduplicated registry of up to 1,024 requested monster IDs. Each client unit is
resolved only for the duration of that frame call. A failed or off-screen
projection invalidates that target instead of reusing stale screen pixels. The
overlay converts cached native UI coordinates to its active dimensions. The
commit context, camera update, renderer context, stat accessors, client-unit
lookup, projection, and native-dimension signatures are locked to build 92777.
It is visual-only: it does not change combat values,
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
