# Repair Costs Cap

Controls NPC repair prices and optional permanent durability wear in D2R
3.2.92777. The standalone plugin is compatible with global and mod-local
D2RLoader installations.

`RepairCostsCap.json` is loaded from the active mod first, then from the game
directory. Missing configuration keeps vanilla behavior; a malformed
configuration refuses the plugin instead of guessing.

Version 1.4.0 uses one `maximumGold` setting. It caps the cost of one repaired
item and the final total of a Repair All transaction without ever charging more
than vanilla. A value of `0` makes both repair actions free while preserving the
normal repair callback.

When `durabilityWear.enabled` is true, every successful physical repair rolls
`durabilityWear.chance` independently. A successful roll permanently removes
one point of maximum durability, never below 1, and leaves the item fully
repaired at its new maximum. A value of `0.10` means 10%. Charge-only repairs,
fully intact items, non-durable items and vendor item generation are excluded.
The same server repair hook covers individual repair and Repair All.

The governed item-cost hook targets the unique 29-byte function prologue at RVA
`0x36F0C0`. Its proven x64 ABI is `(player, item, difficulty, questFlags,
vendorId, transactionType) -> int32`, with repair transaction type `3`.

The Repair All total hook targets the unique 33-byte prologue at RVA
`0x375330`. Its proven x64 ABI is `(game, player, vendorId, difficulty,
questFlags, callback) -> int32`. Client quotes and both authoritative server
passes use this same function, preserving quote and debit consistency.

Zero-priced totals need one additional governed branch change. At RVA
`0x53FF65`, the unique signature
`3B C7 0F 82 AD 00 00 00 85 FF 74 6F 48 8B 55 48` is validated in full, then
only the conditional branch displacement changes from `6F` to `21`. Zero gold
therefore skips the debit but still reaches the normal repair callback.

The durability-wear hook targets the unique 32-byte prologue at RVA
`0x53BB50`. Its proven x64 ABI is `(game, item, player) -> void`; the individual
server handler calls it directly and Repair All passes its jump stub as the
per-item callback. The plugin lets the original repair complete, verifies that
durability actually reached the pre-repair maximum, rolls the item's native
seed, subtracts one from saved/base stat 73, recomputes the effective maximum,
then synchronizes stat 72 (current durability) to that effective maximum. This
preserves percentage durability modifiers while making the raw point permanent.
Because Durability Resistance may already hook the base-stat getter, this plugin
validates the unique untouched 50-byte body beginning at `0x2F48C5`; calls still
flow through the existing hook, whose return-address guard leaves this read raw.

The `repair-costs-cap` console command reports the live pricing and wear policy,
pricing counters, successful physical-repair evaluations and permanent points
lost. The reported gold reductions are diagnostic quote evaluations and may
include repeated client and server calculations; they are not a persistent
wallet ledger.

The plugin refuses unsupported builds or signature mismatches. Its source is
incubated for a future merge under `items.repairCostsCap` in eezstreet's
`plugin-items.dll`; it does not modify, link or redistribute that DLL.
