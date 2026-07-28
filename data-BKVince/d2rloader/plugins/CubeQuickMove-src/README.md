# Cube Quick Move

`CubeQuickMove.dll` makes every item moved indirectly into the Horadric Cube use
the bottom-right placement order, independent of item height.

## Contract

- D2R 3.2 build 92777 only.
- Hybrid D2RLoader plugin: global or mod-local installation.
- Author: `RuffnecKk`.
- Configuration: `CubeQuickMove.json`, searched in the active mod first and the
  global game directory second.
- No TOML and no modification, linkage or redistribution of an eezstreet DLL.

The only supported setting is:

```jsonc
{
    // Place every quick-moved Cube item from the bottom-right.
    "enabled": true
}
```

Missing configuration uses `enabled=true`. An invalid or unknown setting refuses
the plugin instead of silently changing behavior.

## Native scope

The plugin inventories all 36 direct calls to `INVENTORY_FindFreePosition`.
Nine calls are proven to pass only page `0`, `2`, or `4` and remain untouched.
The remaining 27 calls include eight explicit Cube-page calls and nineteen
dynamic-page calls, including the gameplay-proven client placement producer at
`0x15F94F`.
They share one rel32-near relay, while the shared function at `0x3865B0`
remains untouched. The wrapper changes coordinates only when the runtime page
is exactly `3`.

The wrapper runs the vanilla search first. For items taller than one cell, it
reuses the 92777 helpers that obtain the item dimensions and resolve the temporary
occupancy grid. It then scans valid anchors from right to left and bottom to top,
matching the vanilla order used for one-row items without relying on the native
owner-weighting branch. Any failed precondition or native exception restores the
vanilla coordinates.

Every call-site and helper carries strict expected bytes. A mismatched build
is refused before any patch is written.

## Build and test

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The public archive must contain only `CubeQuickMove.dll` and
`CubeQuickMove.json`.

## Future PluginPack merge

After standalone validation, the feature can move to `plugin-misc.dll` under
`misc.cubeQuickMoveBottomRight` and the single PluginPack `D2RPlugins.json`.
The standalone DLL and JSON must then be removed.

PluginSDK is pinned to `efcfaaa52eeec9e379b3fc2aad1013bb3dddc970`.
The near-relay technique was audited against the MIT-licensed eezstreet
PluginPack commit `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`; no PluginPack
source or binary is linked or redistributed.
