# Player Sequence Tables 0.1.0

> Development build: do not publish until every gate in
> [VALIDATION.md](VALIDATION.md) is closed.

Player Sequence Tables gives D2R player skills the same kind of editable TXT
sequence data that monsters already have through `monseq.txt`. It loads two
tables from the active mod, validates the complete dataset, compiles immutable
native records, and redirects only the 25 player-sequence group pointers.

## Install

Install the DLL and TOML in one D2RLoader scope. Use the global scope or the
mod-local scope, never both at once.

Global scope:

```text
<D2R>/d2rloader/plugins/d2rl-ruffneckk-player-sequence-tables.dll
<D2R>/d2rloader/config/ruffneckk-player-sequence-tables.toml
```

Mod-local scope:

```text
<D2R>/mods/<mod>/d2rloader/plugins/d2rl-ruffneckk-player-sequence-tables.dll
<D2R>/mods/<mod>/d2rloader/config/ruffneckk-player-sequence-tables.toml
```

Copy both supplied TXT files into the active mod's Excel directory:

```text
<D2R>/mods/<mod>/<mod>.mpq/data/global/excel/playerseqmap.txt
<D2R>/mods/<mod>/<mod>.mpq/data/global/excel/playerseq.txt
```

D2RLoader layouts that expose `mods/<mod>/data/global/excel` directly are also
supported. The two files must always be installed together.

## Table contract

`playerseqmap.txt` has exactly one route for every combination of player
sequence number `1..25` and native weapon class:

| Column | Meaning |
|---|---|
| `seqnum` | The byte value read from `Skills.txt` `seqnum` |
| `*sequence` | Human-readable comment; ignored by the loader |
| `weaponclass` | One of the 14 native routing codes |
| `recordset` | A name from `playerseq.txt`, or empty for an unavailable route |
| `*eol` | Must be `0` |

The native weapon-class order is:

```text
HTH 1HT 2HT 1HS 2HS BOW XBW STF 1JS 1JT 1SS 1ST HT1 HT2
```

`playerseq.txt` contains the ordered animation records used by each named
record set:

| Column | Meaning |
|---|---|
| `recordset` | ASCII identifier shared by one or more routes |
| `mode` | Native player mode code such as `A1`, `SC`, `S1` or `TH` |
| `frame` | Animation frame byte, `0..255` |
| `dir` | Direction byte, `0..255` |
| `event` | `0` none, `1` melee, `2` missile, `3` sound, `4` trigger skill |
| `*eol` | Must be `0` |

The supplied baseline exactly reproduces D2R 3.3.93847: 350 routes, 235
available routes, 115 null routes, 47 independently named native record sets
and 808 native records. Those record sets currently contain 44 unique byte
sequences. Several routes may deliberately share one record set, but distinct
native arrays remain separately editable even when their current records are
byte-identical.

## Loading and failure behavior

- If both tables are absent, the plugin succeeds without changing vanilla.
- If only one table exists, or any present row is invalid, loading fails before
  any game pointer is changed.
- TXT edits require a full game restart; hot reload is intentionally unsupported.
- The compiled arena remains valid until process exit because D2R caches record
  pointers in live units.
- `player-sequences` prints the active paths, counts and combined SHA-256 hash.

Every multiplayer participant must use the same two tables and therefore the
same hash. The plugin does not negotiate or synchronize data over the network.

## Compatibility, saves and removal

The plugin installs no code hook. It owns only the 200-byte pointer range for
sequence groups 1 through 25 at D2R 3.3.93847 RVA `0x2386658..0x238671F`.
Slot 0, the native resolver, the weapon-class selector and every eezstreet DLL
remain untouched. Installation fails closed if another component already owns
that pointer range.

No custom character or stash payload is written. To roll back, remove the DLL,
TOML and both TXT files, then restart D2R.

## Credits

- Author: `RuffnecKk`.
- D2MOO is credited for semantic names and historical player-sequence topology.
  Every current address, byte layout and x64 behavior was independently proven
  against the governed D2R 3.3 target corpus.
- D2RLoader and its PluginSDK provide the autonomous plugin runtime.
