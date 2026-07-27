# Readable Items

Readable Items shows configured clue-scroll text when the player right-clicks an
item whose compiled `misc.txt` record uses the private `pSpell=-2` sentinel.

Version `0.5.0` is a mod-local gameplay witness for D2R 3.2.92777. It builds a
separate `ReadableItems.dll` and delegates tooltip, right-click, and drawing to
the current owners in ExtendedItemStats and Transmogrify. It installs no
additional D2R hook. This delegated setup is for testing only; the public
standalone fallback remains a later gate.

Version `0.5.0` makes `pSpell=-2` the authoritative opt-in. A matching JSON code
provides only the title, text, and optional audio payload. Vanilla `pSpell`
values are delegated unchanged, including `pSpell=2` on a Town Portal Scroll.
If a `pSpell=-2` item has no JSON entry, its use is consumed safely and logged
instead of reaching the native dispatcher with a private value.

Version `0.4.0` added native FLAC decoding while retaining the optional 16-bit
PCM WAV path introduced in `0.3.0`. FLAC input is decoded from memory to 32-bit
PCM before playback so source precision up to 32 bits is retained. Audio begins
when the reader opens and stops on `Close`, Escape, another readable item, the
console close command, or plugin unload. A missing or unreadable file is logged
without preventing the configured text from opening. Version `0.2.2` keeps the
top-screen dialogue introduced in `0.2.0`, slows the
reveal to 18 characters per second, makes the scrollbar track and thumb fully
clickable and draggable, and adds a framed `Close` button. Its delegated mouse
handler now consumes both halves of a click inside the panel so Close, the
scrollbar, its arrows, and text fast-forward cannot issue a movement command to
the player. Click the dialogue or press Space/Enter to reveal it immediately.
Version `0.1.1` fixed the runtime
lookup of one-to-three-character item codes by padding their compiled four-byte
ItemsTxt value with ASCII spaces; version `0.1.0` used null padding.

`ReadableItems.json` is searched from the active mod before the global game
directory. `dmy` remains the pure framework fixture. The dedicated `rds` item
is appended to BKVince `misc.txt` with `pSpell=-2`, reuses the scroll art, and is
sold as a permanent test item without replacing any existing Class ID. A mod
author must replace the witness with a real custom item after validation.

Configuration fields:

- `enabled`: enables the configured entries.
- `tooltip`: text added to every configured item tooltip.
- `items[].code`: visible ASCII item code, one to four bytes.
- `items[].title`: non-empty heading, at most 128 bytes.
- `items[].text`: non-empty body, at most 8,192 bytes.
- `items[].audioFile`: optional safe relative path to a mono or stereo 16-bit
  PCM `.wav` file or native `.flac` file, resolved from the directory containing
  `ReadableItems.json`.

For each real readable item, create or clone a `misc.txt` row with a unique item
code, set `useable=1` and `pSpell=-2`, then add the same code to the JSON. The
JSON entry alone never turns a vanilla item into a readable item.

Unknown settings, duplicate codes, absolute audio paths, parent traversal and
unsupported audio extensions are rejected. WAV and FLAC input is limited to
8-192 kHz, 64 MiB on disk and 128 MiB after FLAC decoding. A valid
configuration may omit `audioFile`.

FLAC decoding uses `dr_flac` 0.13.4 by David Reid, pinned at commit
`34a89ffe6bfc4d78db6888fef76cd408dba18185` and used under MIT-0. The decoder
is linked statically; no additional runtime DLL is required.

The current tests also cover progressive reveal state, automatic following,
manual review, opening and closing the reader, switching objects, scrolling in
both directions, clamping at both ends, viewport changes, and rejection of
invalid entries without replacing the active content.

Build and run the tests from this directory:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## In-game witness

The current DLL and JSON are already synchronized to the BKVince runtime.

1. Enter an offline BKVince game.
2. Buy one loose **Clue Scroll Test** from Akara. Do not use a Town Portal
   Scroll: `tsc` now remains completely vanilla.
3. Hover the scroll and verify `Right-click to read...` in its tooltip.
4. Note the scroll quantity, then right-click it once.
   Verify that the FLAC audio witness is clearly audible.
5. Verify that a dialogue-style panel opens near the top of the screen: black
   translucent background, double gold frame, large white Formal text, and no
   technical title, footer, instructions, or page counter.
6. Let the text reveal itself at the slower NPC-dialogue pace and verify that
   the latest line remains visible.
   Click inside the text, or press Space/Enter, to reveal all remaining text.
7. Click the scrollbar track and drag its gold thumb from top to bottom. Also
   use the gold arrows, Up/Down, and Page Up/Page Down; both ends must clamp,
   returning to the bottom must resume automatic following, and the character
   must not move after the track, thumb, or arrow clicks.
8. Click `Close`; reopen, then press Escape. Both actions must close the panel,
   each reopening must restart progressive reveal at the first line, and the
   character must not move toward the world position underneath `Close`. The
   audio must stop immediately on both close paths and restart from the
   beginning after reopening.
9. Right-click again and verify that progressive reveal restarts at the
   first line.
10. Verify that the scroll still exists in the same location. Also right-click a
    real Town Portal Scroll and verify that it creates a portal normally.
11. Hover another item and exercise one Transmogrify item to confirm coexistence.

The D2RLoader console command `readable-items preview` opens the panel without an
item. `readable-items status` reports the configuration and counters, and
`readable-items close` closes it. Report the first failed numbered step, the
container used, and whether the scroll moved or changed quantity.
