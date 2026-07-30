# Remote Stash 0.2.26 — BKVince

Author: RuffnecKk

Remote Stash opens the native Personal, Shared, and Crafting Stash from the
BKVince inventory, including outside town.

## BKVince layout

This variant keeps the existing BKVince behavior:

- dynamic desktop placement;
- the current chest sprite and tooltip;
- the vanilla gold button position;
- no configuration file.

The placement code uses the existing `grid`, `gold_button`, `gold_amount`, and
`remote_stash` widgets. Version 0.2.26 does not change those layouts or assets.

## PluginPack compatibility

Version 0.2.26 leaves the shared native function entries available to
PluginPack. It can run with these features enabled:

- Equipped Item to Cube;
- Bulk Skill Point Allocation in `plugin-skills.dll`;
- Prevent Merc Death in Town.

Remote Stash redirects only its required D2R call sites and calls through the
live PluginPack-owned entries. No load order is required for these features.

## Build

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Target: Diablo II: Resurrected 3.2.92777 with D2RLoader.
