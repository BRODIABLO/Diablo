# Advanced Item Tooltips

Hybrid D2RLoader plugin for D2R 3.2.92777. It can be installed either in the
global `d2rloader/plugins` directory or in a mod-local plugin directory.

The plugin formats a vanilla-white `Max Sockets: N` line for every item whose
native `ITEMS_GetMaxSockets` capacity is non-zero and whose current socket stat
is zero. Items already displayed as `Socketed (N)` receive no additional line.
Transmogrify's existing final-tooltip pipeline inserts the line below the full
weapon damage block or below an armor's defense line.

For identified items, the plugin also reads the loose TXT tables from the
currently loaded mod and appends exact variable roll ranges in green. The
calculation starts from the affix identifiers stored on the item, combines all
sources that render as the same stat, and includes fixed `cubemain.txt` crafted
properties. For example, a fixed crafted `5-10% Faster Cast Rate` property and
a `+10% Faster Cast Rate` suffix are displayed as `[15 - 20]`.

Craft recipe variants are filtered against the displayed roll. If multiple
remaining recipes disagree, or if a property function is not modeled safely,
the range is omitted. This fail-closed policy also applies to base defense when
Enhanced Defense prevents an exact reconstruction. Plain armor can receive a
white `Base Defense: N [min - max]` line immediately below its Defense line.

The range suffix restores the color active on the original modifier line. It
does not replace the item's font or recolor existing modifiers.

D2R stores final-tooltip lines in bottom-to-top display order. Transmogrify
therefore inserts the socket line immediately before the lowest displayed
`Damage:` line or the `Defense:` line in the internal buffer.

Advanced Item Tooltips installs no hook and rejects unsupported D2R builds.
Transmogrify remains the sole owner of the strict final-tooltip hook.
