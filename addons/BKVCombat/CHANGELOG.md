# Changelog

## 0.1.0 - unreleased

- Added a standalone, hybrid D2RLoader Release 1 plugin scaffold for D2R
  3.2.92777.
- Added independent Critical Strike, Deadly Strike, Crushing Blow, Life Steal,
  Mana Steal, and Open Wounds policy toggles with atomic fail-closed activation.
- Added collision-safe, configurable Crushing Blow Efficiency with BKVince stat
  393, property 312, and string 65030.
- Added live player-count composition and checked rational Crushing Blow math.
- Added five-second, three-stack Open Wounds stat-list lifecycle with physical
  resistance and mercenary/owned-pet quartering.
- Kept Life/Mana Steal on the validated native 1/1, 1/2, 1/3 difficulty and
  Drain baseline without installing a redundant leech hook.
- Added data-driven MajorBoss validation against active `monstats.txt` keys and
  IDs.
- Classified Heralds and Ascendants as Crushing Blow Elites.
- Added governed internal seam, ownership, compatibility, and rollback
  documentation.
- Kept the shipped configuration disabled pending the complete runtime matrix.
