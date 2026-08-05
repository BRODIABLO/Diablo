# MassID 1.0.0

MassID identifies eligible items in the player's inventory, Horadric Cube,
personal stash and shared stash when the player Shift-right-clicks a Tome of
Identify.
The Tome tooltip shows a gray Mass ID reminder below the vanilla drop hint.
It follows D2R's active language automatically across all thirteen shipped
locales, with no language option in the JSON.

## Configuration

`MassID.json` is resolved from the active mod's D2RLoader `config` directory
first, then from the plugin scope and the global `d2rloader/config` directory.
Unknown keys or non-boolean values refuse the plugin.

```json
{
  "enabled": true,
  "freeIdentification": false
}
```

When `freeIdentification` is `false`, the server identifies inventory items
first, Cube items second, personal-stash items third and shared-stash items last,
stopping at the Tome's current quantity and consuming one charge per newly
identified item. Already identified items cost nothing. When it is `true`, every
eligible item is identified without changing the Tome quantity, including when
the Tome is empty.

## Compatibility and authority

The standalone incubation DLL supports both global and mod-local D2RLoader
plugin folders. It targets D2R build 92777, checks strict signatures before
installing hooks and keeps item mutation and charge consumption server-side.

The private request is multiplexed through the native 21-byte Cain identify
callback. Normal Cain packets are forwarded unchanged. MassID does not hook the
EntityAction callback owned by Vendor Stock Refresh, the use-item handler owned
by Transmogrify, or the generic outgoing queue used by EquippedItemToCube.

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

Version 0.2.5 tracks the Identify Tome already supplied to the tooltip and
intercepts Shift-right-button messages at the game window before the UI can build
the vanilla Identify cursor. Packet dispatch is deferred to the next tooltip
frame so it runs on Diablo's client thread instead of the Windows message thread.
It submits an explicitly packed 21-byte request through the live outgoing queue
and intercepts the verified opcode `0x34` server-dispatch slot before
private-marker validation. Version 0.2.4 corrects the previous callback RVA,
which belonged to opcode `0x2E` despite accepting the same 21-byte ABI.
A targeting-worker hook remains as a fallback for alternate input paths. Shift-use
resolves the current Tome by GUID, sends the private MassID request and suppresses
the cursor; ordinary right-click use remains vanilla. The
multilingual reminder is appended by
the exact `InventoryItemTooltipAppenderDrop` and
`InventoryItemTooltipAppenderMove` call sites instead of changing the main item
text. The appended line inherits the appender's native gray instead of forcing a
separate color code, so opening the Cube keeps both the line and the vanilla
color. Ownership and page validation remain server-side. Cain's complete native
identify path keeps item updates, inventory refresh and the vanilla identify
sound synchronized.
