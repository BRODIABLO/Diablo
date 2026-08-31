# MassID 1.2.0

MassID identifies eligible items in the player's inventory, Horadric Cube,
personal stash and shared stash from a Tome of Identify. The default gesture is
Shift + right click. `rightClickMassIdentify=true` changes it to plain right
click and hides MassID's gray tooltip reminder because the Tome already carries
D2R's white right-click instruction. Shift remains accepted and produces only
one action in direct mode.

The same configured gesture is captured from the normal inventory, vendor and
player-trade screens without activating the vanilla Identify cursor. In the
default mode, the gray reminder follows D2R's active language automatically
across all thirteen shipped locales, with no language option in the JSON.
MassID coexists with both the public standalone Bulk Skill Point Allocation
plugin and its D2RLoader PluginPack version. It calls the active localization
chain without replacing it, regardless of load order.

## Configuration

`MassID.json` is resolved from the active mod's D2RLoader `config` directory
first, then from the plugin scope and the global `d2rloader/config` directory.
Unknown keys or non-boolean values refuse the plugin.

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

Character inventory is always enabled. The three `include...` switches select
whether Cube, personal-stash and shared-stash items participate. Setting all
three to `false` gives inventory-only behavior; leaving only `includeCube` true
gives inventory-plus-Cube behavior.

`rightClickMassIdentify=false` preserves the historical Shift gesture and its
localized gray reminder. `true` consumes a plain right click and leaves the
native white Tome instruction as the only reminder.

When `freeIdentification` is `false`, the server identifies inventory items
first, Cube items second, personal-stash items third and shared-stash items last,
stopping at the Tome's current quantity and consuming one charge per newly
identified item. Already identified items cost nothing. When it is `true`, every
eligible item is identified without changing the Tome quantity, including when
the Tome is empty.

## Compatibility and authority

The standalone DLL supports both global and mod-local D2RLoader plugin folders.
Version 1.2.0 compiles against PluginSDK API v4 at commit
`6eb8f8b6192868214706bd6d528c5294f2f551b7`. Build names are diagnostic only;
the plugin checks its complete governed native fingerprint before installing
hooks and keeps item mutation and charge consumption server-side.

`ItemInteractionServiceV1` is the primary client entry for keyboard/mouse item
activation in inventory, Cube, personal-stash and current custom-page grids.
`ItemServiceV1::getItemInfo` supplies the code and runtime GUID without escaping
a native item pointer. MassID consumes the SDK event only after validating the
configured gesture, Identify Tome code and empty cursor. Controller events are
left unchanged.

PluginSDK v4 deliberately excludes vendor and player-trade interactions. The
existing deferred WndProc path therefore remains only for those two screens,
fed by the governed Sell/Give tooltip callsites and trade-state probe. The old
generic targeting-worker inline hook is removed. Client requests use the
strictly fingerprinted native 21-byte Cain packet builder at `0xEC820`; the
authoritative server callback and shared-stash routing are unchanged.

The private request is multiplexed through the native 21-byte Cain identify
callback. Normal Cain packets are forwarded unchanged. MassID does not hook the
EntityAction callback owned by Vendor Stock Refresh, the use-item handler owned
by Transmogrify, or the generic outgoing queue used by EquippedItemToCube.

Version 1.1.0 added independent Cube, personal-stash and shared-stash target
switches while preserving the existing identification priority. It also probes
the common modern inventory-tooltip path after the hovered item is loaded, so
the deferred game-thread request remains available when vendor and trade states
suppress the normal Drop and Move tooltip appenders. The probe does not touch
the `ITEMS_BuildItemTooltip` callsites owned by AdvancedItemTooltips. Its call
to `UI_IsStateOpen` remains composable with the inline hook owned by Remote
Stash. Version 1.2.0 now accepts only either the exact vanilla bytes or one
loader-tracked inline owner named `ruffneckk-remote-stash`. The localization
entry follows the same fail-closed rule for `eezstreet-plugin-skills`.
The dedicated `InventoryItemTooltipAppenderSell` and
`InventoryItemTooltipAppenderGive` callsites reuse the same localized wrapper,
placing the gray Mass ID reminder below the vendor Sell or player-trade Give
hint without injecting a separate color code.

Version 1.0.0 reads the inventory page from the verified native item-data
field instead of calling the `ITEMS_GetInvPage` entry owned by plugins such as
Cube Output Quantity. This preserves strict build validation while allowing
both plugins to load in either scope.

Version 0.2.9 moves configuration discovery to D2RLoader's standard `config`
directories and supports both global and mod-local installations. An active
mod's configuration takes priority, followed by the plugin scope and the global
configuration.

Version 0.2.8 routes each shared-stash identification through the verified
state-`0xBA` proxy player that owns that page. D2R's native item-update sender
uses this actor to address the owning client's shared container; using the main
player instead creates a personal-inventory ghost even though the authoritative
item remains in the shared-stash save.

Version 0.2.7 corrects the shared-proxy marker validation to use D2R's native
`STATES_CheckState(proxy, 0xBA)` predicate. Version 0.2.6 incorrectly treated
`0xBA` as a stat id, which rejected every valid shared-stash proxy.

Version 0.2.6 discovers D2R's native shared-stash proxy players from the owning
player's auxiliary-unit list and identifies page-4 items in each verified proxy
inventory. The proxy marker and inventory owner are both validated before any
item mutation, and the same scroll budget spans inventory, Cube, personal stash
and shared stash. It adds no shared-transfer hook and therefore does not compete
with RemoteStash.

Version 0.2.5 tracked the Identify Tome already supplied to the tooltip and
intercepts Shift-right-button messages at the game window before the UI can build
the vanilla Identify cursor. Packet dispatch is deferred to the next tooltip
frame so it runs on Diablo's client thread instead of the Windows message thread.
It submitted an explicitly packed 21-byte request through the live outgoing queue
and intercepts the verified opcode `0x34` server-dispatch slot before
private-marker validation. Version 0.2.4 corrects the previous callback RVA,
which belonged to opcode `0x2E` despite accepting the same 21-byte ABI.
That historical targeting-worker fallback is removed in 1.2.0 in favor of the
typed PluginSDK interaction service. The
multilingual reminder is appended by
the exact `InventoryItemTooltipAppenderDrop` and
`InventoryItemTooltipAppenderMove` call sites instead of changing the main item
text. The appended line inherits the appender's native gray instead of forcing a
separate color code, so opening the Cube keeps both the line and the vanilla
color. Ownership and page validation remain server-side. Cain's complete native
identify path keeps item updates, inventory refresh and the vanilla identify
sound synchronized.

## Credits

D2MOO informed the native item and identification semantics used during this
work. The D2R 3.2 integration, guarded hooks and standalone implementation are
maintained by RuffnecKk.
