REMOTE STASH 0.1.5
INTER-MOD DEVELOPER KIT

Author: RuffnecKk


WHAT IT DOES

Adds a Remote Stash button to the desktop inventory screen.

The mod provides the button through its inventory layout.
RemoteStash.dll places the button and opens the native stash panel.


REQUIREMENTS

- Diablo II: Resurrected 3.2.92777
- D2RLoader
- Desktop mouse layout

Controller layouts are not supported yet.


IMPORTANT

Version 0.1.5 opens the stash panel on the client.
Remote item moves, gold transactions, and persistence are not fully validated.
Use a disposable test character and do not test with valuable items.


INSTALLATION

1. Install RemoteStash.dll.

Global installation:

<D2R>/d2rloader/plugins/RemoteStash.dll

Mod-local installation:

<D2R>/mods/<ModName>/d2rloader/plugins/RemoteStash.dll

2. Copy the contents of mod-data/data into:

<D2R>/mods/<ModName>/<ModName>.mpq/data/

3. Open the active original desktop inventory layout.

Merge the object from:

merge-snippets/playerinventory-button.json

into the PlayerInventoryPanel children array.

Do not replace the complete inventory layout.

4. Check the expansion layout.

If it inherits the original children, nothing else is required.

If it replaces the children array, add:

merge-snippets/playerinventory-expansion-child.json

If the expansion layout is fully standalone, add the complete button object
instead.


CUSTOM SPRITE

The kit already includes a ready-to-use chest button:

remotestashbutton.sprite
remotestashbutton.lowend.sprite

To use different artwork, replace both files and keep the same names.

You may also use another path by changing the filename field in the main button
snippet.

The supplied button uses four visual states:

- High resolution: 176 x 112
- Low-end: 88 x 56

Keep hoveredFrame set to 3 when using the same frame layout.


TOOLTIP

The default tooltip is:

@OpenCurrentStashLegend

This is a Blizzard string and is translated automatically.

A mod may use its own localization key instead.


POSITIONING

The DLL places the button automatically.

It uses these layout names:

grid
gold_button
gold_amount
remote_stash

Do not rename them.

The button is placed below the inventory grid and beside the gold controls.
The original gold button is never moved.

The x and y values in the JSON are only starting values. The DLL replaces them
at runtime. Custom position offsets are not available in version 0.1.5.

If there is no safe space, the button is hidden.


DO NOT CHANGE

The button name must remain:

remote_stash

The click message must remain:

PlayerInventoryPanelMessage:DropGold

This message is used only to identify the Remote Stash button. The real gold
button continues to work normally.


TEST CHECKLIST

- The Remote Stash button appears.
- It does not overlap the inventory grid.
- The original gold button has not moved.
- The tooltip appears in the selected language.
- Clicking Remote Stash does not open the Drop Gold window.
- Clicking Remote Stash opens the stash panel.
- Closing the stash and inventory still works normally.

Please include the mod name, game language, resolution, screenshot, and fresh
D2RLoader log when reporting a problem.
