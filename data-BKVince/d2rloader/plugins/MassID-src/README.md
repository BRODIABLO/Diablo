# MassID 0.1.0

MassID identifies eligible items in the player's inventory and Horadric Cube
when the player Shift-right-clicks a Tome of Identify.

## Configuration

`MassID.json` is resolved from the active mod first and then from the global
working directory. Unknown keys or non-boolean values refuse the plugin.

```json
{
  "enabled": true,
  "freeIdentification": false
}
```

When `freeIdentification` is `false`, the server identifies inventory items
first and Cube items second, stopping at the Tome's current quantity and
consuming one charge per newly identified item. Already identified items cost
nothing. When it is `true`, every eligible item is identified without changing
the Tome quantity, including when the Tome is empty.

## Compatibility and authority

The standalone incubation DLL supports both global and mod-local D2RLoader
plugin folders. It targets D2R build 92777, checks strict signatures before
installing hooks and keeps item mutation and charge consumption server-side.

The private request is multiplexed through the native 21-byte Cain identify
callback. Normal Cain packets are forwarded unchanged. MassID does not hook the
EntityAction callback owned by Vendor Stock Refresh, the use-item handler owned
by Transmogrify, or the generic outgoing queue used by EquippedItemToCube.

Future PluginPack owner: `plugin-items.dll`, key `items.massIdentify`.
