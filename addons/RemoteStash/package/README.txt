REMOTE STASH 0.2.26
INTER-MOD PUBLIC RELEASE

Author: RuffnecKk


WHAT IT DOES

Adds a Remote Stash button to a mod's desktop inventory layout.
The button opens the native Personal, Shared, and Crafting Stash from anywhere.

There is no configuration file.
The DLL never moves or resizes the button.


REQUIREMENTS

- Diablo II: Resurrected 3.2.92777
- D2RLoader
- A desktop inventory layout owned by your mod

Controller layouts are not included.


PLUGINPACK COMPATIBILITY

Remote Stash 0.2.26 can run with these PluginPack features enabled:

- Equipped Item to Cube
- Bulk Skill Point Allocation in plugin-skills.dll
- Prevent Merc Death in Town

No load order is required for these features.


INSTALL THE DLL

Copy:

d2rloader/plugins/RemoteStash.dll

to either:

<D2R>/d2rloader/plugins/RemoteStash.dll

or:

<D2R>/mods/<ModName>/d2rloader/plugins/RemoteStash.dll


ADD THE BUTTON

Open your active playerinventoryoriginallayouthd.json.

Merge the object from:

merge-snippets/playerinventory-button.json

into the PlayerInventoryPanel children array.

Do not replace the complete inventory layout.

If your expansion layout inherits the original children, nothing else is needed.
If it replaces them, merge:

merge-snippets/playerinventory-expansion-child.json


CUSTOMIZE IT

You own the button layout.

You may freely change:

- rect x and y
- rect width and height
- filename
- hoveredFrame
- tooltipString
- the sprite files

The DLL will use the exact rectangle supplied by your layout.

Keep these values unchanged:

name: remote_stash
onClickMessage: PlayerInventoryPanelMessage:DropGold

The real gold button is identified separately and continues to work normally.


DEFAULT SPRITE

A ready-to-use chest sprite is included under:

mod-data/data/hd/global/ui/panel/inventory/

Copy the contents of mod-data/data into your mod's MPQ data folder.

You may replace both sprite files or point filename to your own sprite.


TEST

1. Open the inventory.
2. Confirm the button appears exactly where your layout placed it.
3. Confirm the original gold button did not move.
4. Open Remote Stash outside town.
5. Test Personal and Shared Stash in both directions.
6. Test Ctrl + left click in both directions.
7. Save and exit, then confirm the items persisted.

Use a disposable character for the first test.
