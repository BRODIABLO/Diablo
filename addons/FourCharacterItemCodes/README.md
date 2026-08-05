# Four Character Item Codes

`four-character-item-codes.json` restores HD inventory graphics for
four-character base-item codes on D2R 3.2.92777. D2R already compiles,
resolves, saves, and synchronizes the full four-byte code; the patch corrects
the HD `items.json` loader that otherwise replaces the fourth character with a
space.

The patch is mod-agnostic. It does not depend on BKVince or any other mod's
tables, assets, folders, or save files.

Install the JSON in either the global `<D2R>/d2rloader/patches/` directory or a
mod-local `<D2R>/mods/<mod>/d2rloader/patches/` directory. A cold start is
required because `items.json` is compiled during startup.

There is no configuration file. The patch preserves vanilla space padding for
one-, two-, and three-character codes and changes only four-character keys. Its
26-byte expected signature makes an unsupported or modified build fail closed.

The public ZIP intentionally contains only the memory patch JSON.

Author: RuffnecKk
