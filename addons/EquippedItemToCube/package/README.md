# Equipped Item to Cube 0.1.0

Restores Ctrl-click moves from equipped slots to the open Horadric Cube.

## Install

Copy the included files to either:

- DLL: `<D2R>/d2rloader/plugins/`; JSON: `<D2R>/EquippedItemToCube.json`; or
- DLL: `<D2R>/mods/<mod>/d2rloader/plugins/`; JSON:
  `<D2R>/mods/<mod>/<mod>.mpq/EquippedItemToCube.json`.

The DLL supports both global and mod-local loading.

## Configuration

`EquippedItemToCube.json` accepts one setting:

```json
{
  "enabled": true
}
```

The plugin supports D2R build `3.2.92777` and refuses unknown executable
signatures. Author: `RuffnecKk`.
