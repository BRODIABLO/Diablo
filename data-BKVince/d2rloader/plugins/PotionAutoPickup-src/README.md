# Potion Auto Pickup

Hybrid D2RLoader plugin for D2R 3.2.92777. It may be installed globally or in
a mod plugin folder and uses the same plugin identifier in both scopes.

The plugin scans server-side ground items on authoritative player-action
callbacks, verifies distance and collision, then routes the selected potion
through D2R's native pickup path. A GUID-scoped hook preserves the configured
belt column or forces the permitted inventory fallback without affecting normal
manual pickups.

## Player configuration

`PotionAutoPickup.toml` uses exact values from the `code` column in
`misc.txt`:

- `potion_codes` selects the potion codes and defines their pickup priority;
- `belt_columns` lists preferred belt columns from left to right;
- `inventory_fallback_potion_codes` lists the selected codes that may enter
  character inventory when no configured belt column has room;
- `pickup_family_order` orders `health`, `mana`, and `rejuvenation`;
- `pickup_range` accepts 1 through 4;
- `advanced.scan_every_player_actions` accepts 1 through 25.

Inventory fallback codes must also appear in the corresponding
`potion_codes` list. Unknown codes, duplicate columns, missing settings, and
mixed legacy/player-friendly schemas are rejected.

The 1.1 configuration names remain accepted as one complete legacy schema and
produce a migration warning. New and legacy names may not be mixed.

## Diagnostics and safety

The `potion-auto-pickup` console command reports routing counters. Optional
rate-limited diagnostics expose selected codes, belt/inventory destinations,
and pickup results.

D2RLoader validates all expected bytes before hooks are installed. The plugin
accepts only build 92777 and installs no native mutation when disabled or when
configuration or signatures are invalid.

The routing implementation was informed by D2MOO's semantic inventory model;
all D2R 3.2 addresses, signatures, structures, and ABI details were independently
verified against build 3.2.92777.

## Build and tests

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
