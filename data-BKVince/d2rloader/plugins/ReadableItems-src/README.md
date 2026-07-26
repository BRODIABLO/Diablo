# Readable Items

Readable Items will show configured clue-scroll text when the player right-clicks an item.

This directory currently contains the validated phase-one JSON contract and its
pure C++ tests. It intentionally does not build `ReadableItems.dll` yet. The
D2R 3.2.92777 client renderer and the coexistence protocol for the shared
right-click, tooltip, and render hooks must be proven first.

`ReadableItems.json` is searched from the active mod before the global game
directory. The initial `dmy` row is only a development fixture; a mod author
must replace it with a real item code after the runtime prototype passes.

Phase one fields:

- `enabled`: enables the configured entries.
- `tooltip`: text added to every configured item tooltip.
- `items[].code`: visible ASCII item code, one to four bytes.
- `items[].title`: non-empty heading, at most 128 bytes.
- `items[].text`: non-empty body, at most 8,192 bytes.

Unknown settings and duplicate codes are rejected. Audio is deliberately not
accepted until phase two implements deterministic playback and interruption.

Build the configuration tests with CMake and run `ctest --test-dir build -C Release`.
