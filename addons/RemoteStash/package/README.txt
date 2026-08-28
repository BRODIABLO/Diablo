REMOTE STASH 2.2.0
Author: RuffnecKk
Reverse-engineering reference: D2MOO

Opens the player stash from a configurable hotkey or Inventory button,
including outside town. The active mod keeps control of its stash tabs and
Inventory layout.

REQUIREMENTS

- Diablo II: Resurrected 3.3.93847 (runtime qualified)
- Diablo II: Resurrected 3.2.92777 (covered by governed native equivalence)
- D2RLoader 1.1.0-beta or a compatible API v3 loader

Remote Stash is standalone. Install it globally or mod-locally, never in both
scopes at the same time.

INSTALL

Global installation:

- Copy plugins/d2rl-ruffneckk-remote-stash.dll to
  <D2R>/d2rloader/plugins/.
- Copy config/ruffneckk-remote-stash.toml to
  <D2R>/d2rloader/config/.

Mod-local installation:

- Copy the DLL to <D2R>/mods/<ModName>/d2rloader/plugins/.
- Copy the TOML to <D2R>/mods/<ModName>/d2rloader/config/.

CONFIGURATION

The hotkey always opens Remote Stash with the Inventory companion required by
D2R item routing. Set:

close_remote_stash_and_inventory_together = true

to close both panels with the hotkey. Set it to false to close only Remote
Stash and leave Inventory open. The physical Inventory button always closes
only Remote Stash.

Legacy 2.0.x values remain supported:

- hotkey_mode = "remoteOnly" maps to false.
- hotkey_mode = "remoteAndInventory" maps to true.

Do not declare the legacy and new settings together. Ambiguous or invalid
configuration is rejected explicitly.

ITEM ROUTING

When the Horadric Cube is visible, Remote Stash dismisses the Cube companion,
restores Inventory, and completes the native stash transition on the first
open. Drag-and-drop, held-item deposit, withdrawal, and Ctrl-click then share
the same remote-session routing.

The plugin-owned Inventory button and its default four-state RuffnecKk chest
artwork are embedded in the DLL. No Inventory JSON merge or sprite copy is
required.

CREDITS

D2MOO provided semantic reference material for historical Diablo II engine
behavior. Every D2R address, signature, layout witness, and runtime contract
used by this plugin was verified separately against the governed D2R corpus.
