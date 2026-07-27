# Advanced Item Tooltips

Hybrid D2RLoader plugin for D2R 3.2.92777. It can be installed either in the
global `d2rloader/plugins` directory or in a mod-local plugin directory.

The plugin formats a vanilla-white `Max Sockets: N` line for every item whose
native `ITEMS_GetMaxSockets` capacity is non-zero and whose current socket stat
is zero. Items already displayed as `Socketed (N)` receive no additional line.
Transmogrify's existing final-tooltip pipeline inserts the line below the full
weapon damage block or below an armor's defense line.

D2R stores final-tooltip lines in bottom-to-top display order. Transmogrify
therefore inserts the socket line immediately before the lowest displayed
`Damage:` line or the `Defense:` line in the internal buffer.

Advanced Item Tooltips installs no hook and rejects unsupported D2R builds.
Transmogrify remains the sole owner of the strict final-tooltip hook.
