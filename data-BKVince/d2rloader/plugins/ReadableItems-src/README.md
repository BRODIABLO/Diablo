# Readable Items

Readable Items will show configured clue-scroll text when the player right-clicks an item.

Version `0.2.1` is a mod-local gameplay witness for D2R 3.2.92777. It builds a
separate `ReadableItems.dll` and delegates tooltip, right-click, and drawing to
the current owners in ExtendedItemStats and Transmogrify. It installs no
additional D2R hook. This delegated setup is for testing only; the public
standalone fallback remains a later gate.

Version `0.2.1` keeps the top-screen dialogue introduced in `0.2.0`, slows the
reveal to 18 characters per second, makes the scrollbar track and thumb fully
clickable and draggable, and adds a framed `Close` button. Click the dialogue or
press Space/Enter to reveal it immediately. Version `0.1.1` fixed the runtime
lookup of one-to-three-character item codes by padding their compiled four-byte
ItemsTxt value with ASCII spaces; version `0.1.0` used null padding.

`ReadableItems.json` is searched from the active mod before the global game
directory. `dmy` remains the pure framework fixture. The temporary `tsc` entry
uses a loose Town Portal Scroll because it is easy to buy and right-click in
game. A mod author must replace both entries with real custom items after the
runtime prototype passes.

Phase one fields:

- `enabled`: enables the configured entries.
- `tooltip`: text added to every configured item tooltip.
- `items[].code`: visible ASCII item code, one to four bytes.
- `items[].title`: non-empty heading, at most 128 bytes.
- `items[].text`: non-empty body, at most 8,192 bytes.

Unknown settings and duplicate codes are rejected. Audio is deliberately not
accepted until phase two implements deterministic playback and interruption.

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
2. Buy one loose **Scroll of Town Portal** from Akara. Do not test a tome.
3. Hover the scroll and verify `Right-click to read...` in its tooltip.
4. Note the scroll quantity, then right-click it once.
5. Verify that a dialogue-style panel opens near the top of the screen: black
   translucent background, double gold frame, large white Formal text, and no
   technical title, footer, instructions, or page counter.
6. Let the text reveal itself at the slower NPC-dialogue pace and verify that
   the latest line remains visible.
   Click inside the text, or press Space/Enter, to reveal all remaining text.
7. Click the scrollbar track and drag its gold thumb from top to bottom. Also
   use the gold arrows, Up/Down, and Page Up/Page Down; both ends must clamp,
   and returning to the bottom must resume automatic following.
8. Click `Close`; reopen, then press Escape. Both actions must close the panel,
   and each reopening must restart progressive reveal at the first line.
9. Right-click again and verify that progressive reveal restarts at the
   first line.
10. Verify that the scroll still exists with the same quantity and location.
11. Hover another item and exercise one Transmogrify item to confirm coexistence.

The D2RLoader console command `readable-items preview` opens the panel without an
item. `readable-items status` reports the configuration and counters, and
`readable-items close` closes it. Report the first failed numbered step, the
container used, and whether the scroll moved or changed quantity.
