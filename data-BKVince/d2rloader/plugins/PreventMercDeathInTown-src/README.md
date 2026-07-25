# Prevent Merc Death in Town

Keeps the five native mercenary classes alive when a persistent negative-life
tick would cross directly from positive life to zero while they are in town.
The standalone plugin targets D2R 3.2.92777 and supports both global and
mod-local D2RLoader installations.

`PreventMercDeathInTown.json` is loaded from the active mod first, then from the
game directory. Missing configuration preserves vanilla behavior; malformed
configuration refuses the plugin instead of guessing.

The governed hook targets `D2GAME_MONSTER_ApplyStatRegen` at RVA `0x448C00`.
Vanilla checks whether current life is already below one point before applying
the negative tick. The plugin checks the projected life for mercenary class IDs
271, 338, 359, 560 and 561, confirms the room is in town, suppresses only a
lethal transition, and schedules the next stat-regeneration event. All other
units, positive regeneration and nonlethal ticks remain on the original path.

This plugin is incubated for a future merge under
`misc.preventMercDeathInTown` in eezstreet's `plugin-misc.dll`; it does not
modify, link or redistribute that DLL.
