# Book of Lore

Book of Lore is a permanent standalone D2RLoader plugin by RuffnecKk. It is
designed for both global and mod-local installation and uses its own independent
`BookOfLore.toml` configuration.

Version 0.2.0 adds a fail-closed client witness for D2R 3.2.92777. When the
plugin is explicitly enabled, it intercepts only an ordinary object packet
`0x27` carrying the vanilla Tower Tome string id `127`, then redirects only the
native scroll UI's internal localization call to the first configured message.
It does not hook the global localization function and does not install a custom
renderer, input handler, or panel.

This witness proves that configured text can reach the native scrolling-lore UI.
It does not yet select messages on the server: filters, per-game history,
`all_same`, variable expansion, and `scroll_speed` are implemented and tested as
policy but are not active in gameplay. The bundled configuration therefore
remains disabled. Enabling it is for a controlled Tower Tome runtime test only.

`BookOfLore.toml` is searched first in the active mod directory and then in the
global game directory. A present but invalid configuration is rejected without
falling back silently. A missing configuration uses disabled built-in defaults.
TOML was chosen for modder-friendly comments, multiline lore text, and readable
arrays of messages while retaining strict parsing and validation.

Message fields:

- `id`: unique stable ASCII identifier.
- `title`: non-empty title, at most 128 bytes.
- `text`: non-empty lore text, at most 65,536 bytes.
- `scroll_speed`: optional positive value, default 18.
- `all_same`: shares a selected adventure message with other eligible players.
- `filters`: optional difficulty, act, area, quest, player class, level, and
  town rules.

Outside town, configured difficulty, act, and area values are minimums. In town,
difficulty and act are exact and area is ignored. Quest, class, and level rules
remain strict. A `max_level` below `min_level` is ignored, matching the historical
behavior. Adventure-book selections live only for the current game; town books
are selected again on every read. No character-save persistence is implemented.

The status command reports whether hooks are enabled and counts intercepted
Tower Tome packets and text substitutions. All native entry points and the one
patched call site are protected by exact build-92777 signatures; any mismatch
refuses activation.

Build and test from this directory:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Historical credits are preserved separately: Myhrginoc authored the Diablo II
Messaging System, SVR produced the D2Mod conversion and act fix, and afj666
authored the Custom TBL Plugin used by that legacy implementation. The D2R
rewrite is authored by RuffnecKk and does not reuse or redistribute those
legacy binaries.
