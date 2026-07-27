# Ground Item Label Limit

Raises the number of simultaneous ground item labels in D2R 3.2.92777. The
plugin works from either the global or a mod-local D2RLoader plugin directory.

The vanilla renderer keeps two synchronized collections at exactly 32 entries:
a dynamic array of `0x144`-byte label records and a linked layout list. The
plugin validates and patches all seven coupled constants as one governed patch
set. If any original signature differs, loading is refused before memory is
modified.

The standalone incubation build reads `GroundItemLabelLimit.json` from the
active mod first and the game directory second. Its `limit` accepts exactly 64
or 128; every other value is refused. Set `enabled` to `false` to retain the
vanilla limit of 32.

This DLL is owned by `RuffnecKk`, remains hybrid, and does not modify or link an
eezstreet DLL. Its confirmed future PluginPack owner is `plugin-items.dll`, with
the planned configuration key `items.groundItemLabels`. After that future
merge, the standalone DLL and JSON must be removed.
