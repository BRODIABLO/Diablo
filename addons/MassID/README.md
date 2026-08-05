# MassID 1.0.0

MassID identifies every eligible item by Shift-right-clicking a Tome of
Identify. It keeps the normal cursor, plays Diablo II: Resurrected's vanilla
identify sound, and adds a localized Mass ID reminder to the Tome tooltip.

## Installation

MassID supports both global and mod-local installations. Install the DLL in
only one scope.

### Global

```text
<D2R>/d2rloader/plugins/MassID.dll
<D2R>/d2rloader/config/MassID.json
```

### Mod-local

```text
<D2R>/mods/<mod>/d2rloader/plugins/MassID.dll
<D2R>/mods/<mod>/d2rloader/config/MassID.json
```

When a mod is active, its `MassID.json` takes priority. If no mod-local
configuration exists, MassID falls back to the global configuration. This
allows a global DLL to use settings specific to the active mod without loading
a second copy of the plugin.

Restart D2RLoader after replacing the DLL or changing the configuration.

## Usage and identification priority

Hold Shift and right-click a Tome of Identify. MassID identifies unidentified
items in this fixed order:

1. Character inventory.
2. Horadric Cube.
3. Personal stash.
4. Shared stash, in the native shared-page order.

Already identified items are skipped and never consume a Tome charge. The
personal and shared stashes are included even though they use different native
containers.

## Configuration

```json
{
  "enabled": true,
  "freeIdentification": false
}
```

- `enabled`: enables or disables MassID.
- `freeIdentification: false`: the Tome quantity is the identification budget.
  One scroll is consumed for each newly identified item, and processing stops
  when no scroll remains. The priority above determines which items are handled
  first when the Tome contains fewer scrolls than eligible items.
- `freeIdentification: true`: every eligible item is identified for free. Tome
  quantity is ignored and no scroll is consumed, including when the Tome is
  empty.

Unknown settings or values that are not booleans cause the plugin to refuse its
configuration instead of silently applying an unintended value.

Author: `RuffnecKk`
