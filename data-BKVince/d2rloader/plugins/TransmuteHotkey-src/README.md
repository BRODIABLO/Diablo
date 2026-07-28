# Transmute Hotkey

`TransmuteHotkey.dll` triggers the native `convert` button exposed by the
standalone Horadric Cube panel or BKVince's integrated stash/Cube panel. It does
not synthesize a network packet and does not alter the visible button, mouse,
controller, recipe validation, sound, or animation paths.

## Compatibility and safety

- Supported executable: `D2R.exe` build `3.2.92777`.
- Supported scopes: global D2RLoader plugins and mod-local D2RLoader plugins.
- Configuration: `TransmuteHotkey.json`, mod-local first and global fallback.
- Built-in default hotkey: `CTRL+SHIFT+T`; the BKVince profile uses `MOUSE4`.
- One activation is posted to the game UI thread per physical press; repeats are ignored.
- The request expires after 250 ms and executes only on a Cube panel update.
- The native button must be visible, enabled, and carry a non-empty click message.
- Chat and known text-entry or confirmation modals refuse the request.
- The UI dispatcher remains owned by the existing broker; this plugin only calls it.

Printable keyboard keys require `CTRL` or `ALT`. Supported names are `A-Z`,
`0-9`, `F1-F24`, `SPACE`, `TAB`, `INSERT`, `DELETE`, `HOME`, `END`, `PAGEUP`,
and `PAGEDOWN`. Mouse bindings accept `MOUSE3`, `MOUSE4`, and `MOUSE5`, with
`MIDDLE`, `XBUTTON1`, and `XBUTTON2` as aliases. Any binding may include exact
`CTRL`, `SHIFT`, or `ALT` modifiers.

Version 0.2.0 replaces the 120 ms pre-armed keyboard window with a targeted
`WH_GETMESSAGE` handoff. Keyboard and mouse hooks only post an accepted request;
the native button is revalidated and dispatched on the game UI thread.

## Build

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The D2RLoader PluginSDK is pinned to
`efcfaaa52eeec9e379b3fc2aad1013bb3dddc970`; nlohmann/json is pinned to
`v3.11.3`.
