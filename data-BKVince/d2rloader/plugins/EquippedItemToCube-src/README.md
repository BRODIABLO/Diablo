# Equipped Item to Cube

Restores the native Ctrl-click action that moves an equipped item directly to
the open Horadric Cube.

## Scope

- D2R build: `3.2.92777`
- author: `RuffnecKk`
- plugin id: `equipped-item-to-cube`
- autonomous DLL: `EquippedItemToCube.dll`
- future PluginPack owner: `plugin-misc.dll`
- future setting key: `misc.equippedItemToCube`

The plugin is hybrid: it can load from either the global D2RLoader plugin
directory or a mod-local plugin directory. It does not modify, link, or
redistribute any eezstreet DLL.

## Configuration

Place `EquippedItemToCube.json` in the active mod data root
(`<D2R>/mods/<mod>/<mod>.mpq/`), or in `<D2R>/` when installed globally:

```json
{
  "enabled": true
}
```

Unknown keys, malformed JSON, and non-boolean `enabled` values are rejected.
No TOML configuration is used.

## Technical evidence

The 2.4 equipped-item hover handler reaches the Cube placement eligibility
helper immediately after the Ctrl state check. Build 3.2.92777 adds an
additional client eligibility branch at RVA `0x228B98` before that same Ctrl
path. When enabled, this plugin removes only that conditional exit. The Cube
space check, the exclusion of the Cube itself, the native item transaction,
and every server-side validation remain unchanged.

The loader requires the surrounding 47-byte handler signature and the exact
six-byte branch before applying the patch. Any mismatch refuses the plugin.

Collision audit:

- `CubeQuickMove` patches `0x4BBA73`;
- eezstreet `plugin-misc.dll` hooks `0x542F40`;
- this plugin patches only `0x228B98..0x228B9D`.

## Build and test

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
