# Automap Serialization Fix

Automap Serialization Fix is a config-free D2RLoader plugin by RuffnecKk.
It prevents crashes when D2R serializes more than 32,767 bytes of explored
automap cells during an area or automap-layer transition.

## Installation

Install `d2rl-ruffneckk-automap-serialization-fix.dll` in either location:

- global: `<D2R>/d2rloader/plugins/`;
- mod-local: `<D2R>/mods/<mod>/d2rloader/plugins/`.

Use only one copy. The plugin is active by its presence and has no configuration
file. Restart D2R after adding or removing it.

## What it fixes

D2R stores each emitted automap record as three 16-bit values: frame, X and Y.
The vanilla serializer calculates the resulting byte length through a signed
16-bit intermediate. Once the payload reaches 32,768 bytes, that length becomes
negative and can reach the memory-copy path as a huge unsigned value.

The plugin replaces only that final 13-byte calculation with a checked 32-bit
calculation. It preserves the existing record layout and sidecar format. It
does not enlarge levels, change map generation, alter character saves or add
new automap data.

Use the `automap-serialization-fix` console command to display whether the
correction is active and the diagnostic build name reported by D2RLoader.

## Compatibility and safety

The plugin validates a complete native fingerprint before writing any byte.
It refuses to load if the serializer, record layout, four callsites, return
path or sidecar commit witness differs. Build names and distribution channels
are logged for diagnosis only and never decide compatibility.

The same native evidence covers D2R 3.2.92777 and Battle.net D2R 3.3.93847.
The plugin has completed full-stack cold starts on Battle.net D2R 3.3.93847
with D2RLoader 1.2.0-beta and 1.2.1-beta preview 10, in both mod-local and
global plugin scopes. On D2RLoader 1.2.1 preview 10, a deterministic gameplay
test also serialized a 36,000-byte single-tree payload, changed from automap
layer 0 to layer 1, returned to layer 0 and verified all 6,000 witness cells
restored through the native sidecar without a crash. Steam D2R 3.3.93787
remains untested until every used surface is proved byte-identical or qualified
separately.

MapSense 1.0.0 remains optional. Its generated atlas cells already use D2R's
restored-cell tag and are excluded from this sidecar. MapSense accepts both the
complete vanilla calculation and this plugin's complete corrected calculation,
so either plugin load order is supported.

## Credits

- RuffnecKk — implementation, D2R 3.3 analysis and plugin integration.
- D2MOO — semantic reference for legacy Diablo II automap behavior. No D2MOO
  address, structure layout or 32-bit ABI is reused in this D2R plugin.
