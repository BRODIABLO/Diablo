# Cube Quick Move

`CubeQuickMove.dll` makes every item moved indirectly into the Horadric Cube use
the native bottom-right placement order, independent of item height.

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

The plugin verifies the unique five-byte call at RVA `0x4BBA73` and redirects
only that Cube-transfer call through a rel32-near relay. The shared
`INVENTORY_FindFreePosition` function at `0x3865B0` remains untouched.

The wrapper runs the vanilla search first. For items taller than one cell, it
reuses the 92777 helpers that obtain the item dimensions, build the temporary
occupancy grid and invoke the native bottom-right search at `0x38D8F0`. Any
failed precondition or native exception restores the vanilla coordinates.

The call-site and every helper carry strict expected bytes. A mismatched build
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
