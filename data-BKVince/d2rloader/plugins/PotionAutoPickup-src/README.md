# PotionAutoPickup

Hybrid D2RLoader plugin source for BKVince. It can be installed globally or in
a mod plugin folder. The native adapter targets `D2R.exe 3.2.92777`, scans
server-side ground items, and invokes the same server pickup routine used by
vanilla automatic gold pickup.

The selected item is carried through the synchronous native pickup path by its
server GUID. This keeps belt routing exact even when the engine resolves a
different in-memory address for the same ground item. The
`potion-auto-pickup` console command reports routing counters and per-code
`seen/selected/picked` totals for live diagnosis.

Potion families are classified through D2R's native packed item code (`hp2`,
`mp2`, `rvs`, and so on), not through calculated TXT row IDs. This keeps the
runtime behavior stable when a mod changes the compiled item-table layout.

Automatic scans are triggered by the normal player-action packet range
`0x01`-`0x12`, including movement and skill actions. The legacy
`minimum_interval_frames` key remains accepted, but new configurations should
use `minimum_interval_actions` because the throttle counts those actions.

The runtime signatures are checked by D2RLoader before the hooks are installed.
Pickup distance is capped at the vanilla gold value of `4`; collision and
ground-mode checks are performed before pickup.

`PotionAutoPickup.toml` exposes all healing (`hp1`-`hp5`), mana (`mp1`-`mp5`)
and rejuvenation (`rvs`/`rvl`) types:

- `tiers` selects each exact potion type that can be picked up;
- `columns` selects the ordered belt columns available to that family;
- `overflow_tiers` selects each exact type that may fall back to inventory;
- `tier_priority` and `family_priority` control the scan order.

`overflow_to_inventory` remains a compatibility fallback for older configs.
When `overflow_tiers` is present, the per-type list is authoritative. An empty
`columns` list combined with enabled overflow types provides inventory-only
routing.

The routing implementation was informed by D2MOO's semantic inventory model;
all D2R 3.2 addresses, signatures, structures, and ABI details were independently
verified against build `3.2.92777`.

Build and policy tests:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
