# Extended Act Level IDs 2.0.2

Extended Act Level IDs targets making `levels.txt` usable up to D2R's native
limit of 1023 compiled records: canonical Level IDs `0` through `1022`.

D2R normally resolves an act from the contiguous ranges compiled from
`actinfo.txt`. As a result, a new Level ID appended after the Act V range is
treated as Act V even when its `levels.txt` row declares another act. This
Version 2.0.2 keeps that fix and extends the two six-byte room-visibility
packets whose stock Level ID field is only one byte. It preserves the original
packet size and carries the two high Level ID bits in a versioned coordinate
marker understood only by another compatible v2 instance.

## Installation

Install the DLL in one scope only.

Global installation:

```text
<D2R>/d2rloader/plugins/d2rl-ruffneckk-extended-act-level-ids.dll
```

Mod-local installation:

```text
<D2R>/mods/<mod>/d2rloader/plugins/d2rl-ruffneckk-extended-act-level-ids.dll
```

The plugin supports both scopes but refuses a duplicate global and mod-local
installation in the same process. It has no configuration file because it has
no modder-facing setting: the installed DLL is active, and removing it disables
the plugin. Restart D2RLoader after adding, removing, or replacing the DLL.

## Runtime contract

The plugin owns five native hooks: the central Act resolver, the server packet
0x07/0x08 builders, and the matching client room-in-sight/out-of-sight paths.
After each `DataTablesLoaded` event it uses PluginSDK API v3 to copy the
Classic, LoD, and RotW `Levels` tables into plugin-owned immutable caches. It
validates all of the following before using them:

- the complete 48-byte native resolver fingerprint;
- the native `< 0x400` compiled-record guard, which admits at most 1023 rows;
- all four room-visibility hook fingerprints and the `D2Client` identity
  layout witness;
- the PluginSDK service versions;
- compiled `Levels` row size `0x18C`;
- a record count between `1` and `1023` in every bank;
- every physical row through `getRow`, including revision, index, pointer, and
  row-size checks;
- canonical contiguous IDs, where every `Id` equals its physical row index;
- the `Id` field through a keyed service round-trip for vanilla IDs `0..255`;
- the `Act` field at `+0x0D`, including the five vanilla act boundaries;
- every act value is between `0` and `4`.

For Level IDs `0..255`, packet bytes and coordinates stay on the original
vanilla path. For IDs `256..1022`, the private PluginSDK channel first matches
the receiving D2R client to the `LocalPlayerReady` player ID. Only a compatible
peer receives an encoded packet. The codec preserves all six packet bytes,
uses X coordinates `0..8191`, and restores the full Level ID and X coordinate
before the original client function runs.

An unsupported data context, table revision, Level ID, coordinate, peer,
signature, service ABI, or hook owner never triggers a truncated fallback.
Extended packets fail closed. Build names are logged for diagnostics only and
are not an allowlist.

Version 2.0.2 first validates the complete physical table, then repeats the
physical lookup and keyed identity round-trip only for IDs `0..255`. This keeps
the proven vanilla service check without calling the current keyed lookup past
its observed boundary. A keyed failure still logs the first row index, Level
ID, service result, and returned-versus-expected metadata without exposing a
pointer. Any failure rejects the whole bank and retains the original resolver.

The console command `extended-act-level-ids` reports cache state, table
revision, row counts, compatible peers, encoded/decoded/refused packets,
resolutions, fallbacks, and the diagnostic build name.
`extended-act-level-ids resolve <level-id>
[data-context]` calls the hooked central resolver and reports the zero-based
Act index; the optional data context defaults to RotW (`3`).

## Saves, portals, and waypoints

Version 2.0.2 does not hook save loading or writing and does not change the
D2S/D2I format. It is therefore unlike ISC12: installing or removing this DLL
does not widen a serialized field. Existing saves remain structurally
compatible. A character or mod that depends on custom level rows still needs
the same data and plugin to revisit those areas, so use disposable characters
until the gameplay matrix is complete.

The plugin does not enlarge the fixed waypoint bitset, invent waypoint slots,
or enlarge portal-flag storage. Extended Level IDs may use the stock wider
travel requests only within those existing capacities and semantics. Version
2.0.2 creates no level, transition, waypoint, portal, or quest by itself.

## Release candidate

`RuffnecKk-Extended-Act-Level-IDs-2.0.2.zip` is the planned public archive,
not a public release. Packaging remains blocked until the v2 runtime matrix is
complete. The archive will contain only the DLL; keep this README beside it.

Back up saves and use a disposable character for tests involving custom level
data. Start D2RLoader with the normal complete plugin stack, then run:

```text
extended-act-level-ids
```

The status must report `active` and `cache=ready`. The decisive test requires a
real playable area whose canonical ID is above 255. Run:

```text
extended-act-level-ids resolve <new-level-id>
```

The reported Act must match the row's zero-based `Act` value and the source
must be `Levels.txt`. Then test room streaming, travel in both directions, Town
Portal, automap, save/reload, and host/joiner behavior.

Please report the D2R and D2RLoader versions, installation scope, mod name,
Level ID and declared Act, both console outputs, gameplay result, and the fresh
D2RLoader/plugin logs. A successful cold start without an out-of-range level is
useful compatibility evidence but does not close the playable-area release
gate.

## Compatibility

Version 2.0.2 targets the promoted public D2RLoader 1.2.1 baseline (whose PE
version is `1.2.1-beta`) and PluginSDK API v3 commit
`4933e2c42cb2592958cd0df3b6dc5003102252d1`. Runtime qualification targets the
official D2R `3.3.93847` build. D2R `3.2.92777` is covered only by governed
byte-exact equivalence of every native surface used by the plugin.

Local games and private TCP/IP games are the supported extended-ID network
modes. Every peer must run the same v2 protocol. PluginSDK private channels are
unavailable on Battle.net, so Level IDs above 255 deliberately fail closed
there; this plugin does not alter Battle.net's protocol.

The DLL is an autonomous member of the RuffnecKk D2RLoader Suite. It does not
modify, link, merge, or redistribute any eezstreet plugin.

## Credits

D2MOO documented the historical fixed-threshold behavior and explicitly noted
that the act should be looked up from `Levels.txt`. D2MOO is used as a semantic
reference only; no legacy 32-bit address, structure, or ABI is reused.

Implementation and D2R 3.3 integration: `RuffnecKk`.
