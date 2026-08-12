REMOTE STASH 1.2.0
Author: RuffnecKk
Reverse-engineering reference: D2MOO

Opens and closes the player stash from an inventory button or configurable
hotkey, including outside town. The installed mod keeps control of its stash
tabs and inventory layout.

The remote stash remains open while the character moves. Close it with the
same hotkey, Escape, or the stash close button.
If the inventory is also open, moving leaves both panels visible.
When the hotkey opens Remote Stash by itself, the companion inventory panel is
closed after D2R finishes constructing the stash. An inventory that was already
open remains open. Closing Remote Stash with its hotkey also leaves that
independently opened inventory visible.


REQUIREMENTS

- Diablo II: Resurrected 3.2.92777
- D2RLoader

Remote Stash is standalone. It does not require PluginPack or another plugin.


INSTALL THE PLUGIN

Global:
Copy the included d2rloader folder into your D2R installation folder.

Mod-local:
Copy RemoteStash.dll to:
<D2R>/mods/<ModName>/d2rloader/plugins/

Copy RemoteStash.json to:
<D2R>/mods/<ModName>/d2rloader/config/


HOTKEY

Edit RemoteStash.json and set enabled to true.
The default hotkey is S. Press it again to close the stash.
Set consume to true to prevent the same key from reaching the game.
Native D2R actions bound to that key are suppressed for that key press only.
Restart the game after changing the config.


ADD THE INVENTORY BUTTON

Merge merge-snippets/playerinventory-button.json into the children array of
your active playerinventoryoriginallayouthd.json.

Replace SET_X_FOR_YOUR_LAYOUT and SET_Y_FOR_YOUR_LAYOUT with coordinates for
your own inventory layout. Vanilla desktop reference: x 93, y 1347.

If your expansion layout replaces the original children, also merge:
merge-snippets/playerinventory-expansion-child.json

Do not change these values:
name: remote_stash
onClickMessage: PlayerInventoryPanelMessage:DropGold

The original gold button remains unchanged.


SPRITE

A ready-to-use chest sprite is included under mod-data/data/.
Merge that data folder into your mod's MPQ data folder. You may replace the
sprite files or change the filename in the button layout.


TEST

Test Personal, Shared, and any custom stash tabs outside town. Verify normal
drag-and-drop, Ctrl + left click, save and exit, and reconnecting the character.
Use a disposable character for the first test.
