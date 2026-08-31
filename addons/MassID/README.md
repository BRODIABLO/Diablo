# MassID 1.2.0

MassID identifies every eligible item from a Tome of Identify. By default, use
Shift + right click and MassID adds a localized gray reminder to the Tome
tooltip. Enable `rightClickMassIdentify` to use a plain right click instead;
the custom gray reminder is then hidden because D2R already shows its white
right-click instruction.

Both gestures suppress the vanilla Identify cursor after MassID accepts them,
play Diablo II: Resurrected's vanilla identify sound, and also work while a
vendor or player-trade screen is open.
MassID coexists with both the public standalone Bulk Skill Point Allocation
plugin and its D2RLoader PluginPack version. It calls the active localization
chain without replacing it, regardless of load order.

MassID 1.2.0 requires a D2RLoader release that implements PluginSDK API v4 and
its Item Interaction service.

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

With the default configuration, hold Shift and right-click a Tome of Identify.
With `rightClickMassIdentify: true`, simply right-click it. MassID identifies
unidentified items in this fixed order:

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
  "freeIdentification": false,
  "rightClickMassIdentify": false,
  "includeCube": true,
  "includePersonalStash": true,
  "includeSharedStash": true
}
```

- `enabled`: enables or disables MassID.
- `rightClickMassIdentify: false`: requires Shift + right click and shows the
  localized gray MassID reminder.
- `rightClickMassIdentify: true`: uses plain right click and hides only the
  custom gray reminder. Shift + right click still triggers one Mass ID action.
- Character inventory is always included.
- `includeCube`: includes or excludes the Horadric Cube.
- `includePersonalStash`: includes or excludes the personal stash.
- `includeSharedStash`: includes or excludes every shared-stash page.
- `freeIdentification: false`: the Tome quantity is the identification budget.
  One scroll is consumed for each newly identified item, and processing stops
  when no scroll remains. The priority above determines which items are handled
  first when the Tome contains fewer scrolls than eligible items.
- `freeIdentification: true`: every eligible item is identified for free. Tome
  quantity is ignored and no scroll is consumed, including when the Tome is
  empty.

Unknown settings or values that are not booleans cause the plugin to refuse its
configuration instead of silently applying an unintended value.

For inventory only, set all three `include...` options to `false`. To include
the Cube but exclude both stashes, keep `includeCube` set to `true` and set both
stash options to `false`.

When rolling back to MassID 1.1.1, restore its previous JSON as well. The older
strict parser does not recognize `rightClickMassIdentify`.

## Credits

Native game semantics were informed by the D2MOO project. MassID's D2R 3.2
integration and implementation are maintained by RuffnecKk.

Author: `RuffnecKk`
