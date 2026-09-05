# Extended Act Level IDs 2.1.2

Development stopped on 5 September 2026 at Vincent's request after he reported
that Dimentio integrated this feature into the new loader. This prototype is
retained for reference: 2.1.2 passed static checks but was not tested in game.
The standalone runtime and packaging plans below are historical and canceled.
The replacement loader's exact version and implementation were not audited in
this checkpoint.

Extended Act Level IDs targets making `levels.txt` usable up to D2R's native
limit of 1023 compiled records: canonical Level IDs `0` through `1022`.

D2R normally resolves an act from the contiguous ranges compiled from
`actinfo.txt`. As a result, a new Level ID appended after the Act V range is
treated as Act V even when its `levels.txt` row declares another act. This
Version 2.1.2 keeps that fix and extends the two six-byte room-visibility
packets whose stock Level ID field is only one byte. It preserves the original
packet size and carries the two high Level ID bits in a versioned coordinate
marker understood only by another compatible v2 instance. It also adds the
coordinate-free local/offline Town Portal contract for dynamic class-59 portal
pairs.

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

The plugin owns twelve native hooks: the central Act resolver, the server
packet 0x07/0x08 builders, the matching client room paths, four portal
creation/operation seams, and three portal packet 0x51/0x60 seams. Seven exact
direct calls are redirected through plugin-owned relays: two server portal
operation lookups, the client portal-label lookup at `0xC188E`, the two client
Town Portal usability lookups at `0xFE307/0xFE333`, and the two server portal
state sends at `0x5388E4/0x593012`. The shared `Levels` and `LevelDef` lookup
entries remain untouched for MapSense coexistence.
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

Version 2.1.2 first validates the complete physical table, then repeats the
physical lookup and keyed identity round-trip only for IDs `0..255`. This keeps
the proven vanilla service check without calling the current keyed lookup past
its observed boundary. A keyed failure still logs the first row index, Level
ID, service result, and returned-versus-expected metadata without exposing a
pointer. Any failure rejects the whole bank and retains the original resolver.

The console command `extended-act-level-ids` reports cache state, table
revision, row counts, compatible peers, server/client portal cache sizes,
portal-pair/operation/packet/refusal counters, client publication/eviction/full
lookup/fallback counters, room packet counters, resolutions, fallbacks, and the
diagnostic build name.
`extended-act-level-ids resolve <level-id>
[data-context]` calls the hooked central resolver and reports the zero-based
Act index; the optional data context defaults to RotW (`3`).

## Saves, portals, and waypoints

Version 2.1.2 does not hook save loading or writing and does not change the
D2S/D2I format. It is therefore unlike ISC12: installing or removing this DLL
does not widen a serialized field. Existing saves remain structurally
compatible. A character or mod that depends on custom level rows still needs
the same data and plugin to revisit those areas, so use disposable characters
until the gameplay matrix is complete.

The plugin does not enlarge the fixed waypoint bitset, invent waypoint slots,
or enlarge portal-flag storage. Extended Level IDs may use the stock wider
travel requests only within those existing capacities and semantics. Version
2.1.2 creates no level, transition, waypoint, portal, or quest by itself.

For a dynamic Town Portal whose source or destination exceeds 255, version
2.1.2 keeps a process-local immutable server sidecar containing the current
session, `Game*`, and the reciprocal source/linked GUID pair. Only the exact
class-59 creation path can present the low byte to the stock creation guard.
Portal owner resolution and the two portal-local `Levels` lookups recover the
full ID only after the live GUID pair, class, low byte, data context, and cached
row all match.

Town Portal packet 0x51 and 0x60 sizes, fields, X/Y coordinates, and stock
builders remain unchanged. Before a local 0x51 send, the authoritative server
endpoint publishes the full destination ID. Two exact-callsite relays at
`0x5388E4/0x593012` additionally capture the full owner-room ID still present
in R12D or EDI, publish it under the same GUID, and then call the original 0x60
builder with its original five arguments. No Level-ID bit is stored in a Town
Portal coordinate or packet field.

The resulting client map is keyed by `{session generation, portal GUID}` and
stores both full IDs plus their native low bytes. Publication merges each GUID
atomically under its copy-on-write lock; readers remain lock-free. The map
stores no `Unit*`, is bounded to 1024 entries, evicts a reused GUID on every
class-59 0x51 spawn, and is cleared on session turnover. Both stock client
handlers run first, after which their live portal object, class, GUID and low
bytes must agree with the published descriptor.

The client UI relay at `0xC188E` revalidates the live type-2/class-59 object,
GUID, native low byte, data context, and full cached row before passing the
full destination to the original `Levels` getter. A poisoned or mismatched
descriptor, or a portal with low byte zero and no descriptor, returns null to
the stock label fallback instead of reaching the native range assertion.
The scoped relays at `0xFE307/0xFE333` pass the live portal in RBX, validate the
separate owner-room ID from packet byte `+0x0B`/`Unit+0x1BA`, and substitute
only its full sidecar value into the original `Levels` or `LevelDef` getter.
They leave the surrounding native owner, party, online-Act and quest checks
intact. Unrelated objects and untouched vanilla portals retain the original
lookup. The owner-room low byte is deliberately never compared with
destination byte `+2`.

The first 2.1.1 runtime proved that its coordinate transport was not generally
usable for Town Portal. Its marked X format accepts only values through 8191,
while the Level 256 fixture occupies native subtile X `[16500,16700)`. The
stock 0x60 client handler also stores the separate owner-room low byte at
`Unit+0x1BA`; the Town Portal usability predicate consumes it through two
additional record lookups at `0xFE307` and `0xFE333`. Version 2.1.1 is therefore
a rejected candidate and must not be deployed. Version 2.1.2 implements the
coordinate-free process-local sidecar and exact-callsite relays; it has passed
static qualification but has not yet been deployed or exercised in game.

This Town Portal contract is intentionally **local/offline only**. Creation,
publication, and operation are allowed only for the `LocalPlayerReady`
identity. If an extended creation cannot satisfy that contract or publish its
reciprocal sidecar, it is refused before any unscoped call can reach the stock
byte guard and the session portal contract is poisoned. A remote user, unknown
row, stale session/GUID pair, low-byte mismatch, or malformed packet is refused
rather than truncated. TCP host/joiner and Battle.net Town Portal support are
not claimed. The sidecar is cleared on session change or game exit and is never
serialized.

## Release candidate

`RuffnecKk-Extended-Act-Level-IDs-2.1.2.zip` is the planned public archive,
not a public release. Packaging remains blocked until the v2 runtime matrix is
complete. The archive will contain only the DLL; keep this README beside it.

The preceding 2.0.2 candidate passed a complete-stack cold start with 1023
compiled `Levels` records and contiguous IDs `0..1022`. Its cache published
`Classic=137`, `LoD=137`, and `RotW=1023`; D2RLoader compiled all 192 TXT
tables, loaded 38 plugins, applied 17 patches, and reached `24/24` without a
fresh error. A controlled same-act fixture also entered playable Level 256
from Harrogath normally. The fresh runtime witness reported Level 256, Act 4,
and 35 rooms without a crash report. A later bounded run repeated physical
travel in both directions successfully, then stopped at the native Town Portal
byte guard when casting from Level 256. Version 2.1.0 then passed portal
creation and the first trip to Harrogath, but its false 0x60 equality check and
missing client GUID sidecar caused a client `Levels(0)` assertion before the
return. Version 2.1.1 corrects both defects and adds the exact client UI relay;
its first controlled runtime still refuses the local/offline `0x60` packet and
then asserts from a newly observed client caller at `0xFE30C`, outside the
`0xC188E` relay. The follow-up native census identifies the bounded coordinate
codec and the separate owner-room byte at `Unit+0x1BA` as the two blockers.
Version 2.1.2 removes both blockers statically while keeping room visibility's
separate 0x07/0x08 codec intact. Town Portal return, explicit transport
counters, waypoint, visual automap, persistence, and multiplayer remain
required before packaging. The disposable character and runtime inputs were
restored byte-exact after each previous test.

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
must be `Levels.txt`. Do not deploy or run the new Town Portal path until a
separate runtime-validation gate is authorized. That gate must use a disposable
character and test portal creation, Level 256 → Harrogath, Harrogath → Level
256, cleanup, and fresh portal counters before any persistence or network test.

Please report the D2R and D2RLoader versions, installation scope, mod name,
Level ID and declared Act, both console outputs, gameplay result, and the fresh
D2RLoader/plugin logs. A successful cold start without an out-of-range level is
useful compatibility evidence but does not close the playable-area release
gate.

## Compatibility

Version 2.1.2 targets the promoted public D2RLoader 1.2.1 baseline (whose PE
version is `1.2.1-beta`) and PluginSDK API v3 commit
`4933e2c42cb2592958cd0df3b6dc5003102252d1`. Runtime qualification targets the
official D2R `3.3.93847` build. D2R `3.2.92777` is covered only by governed
byte-exact equivalence of every native surface used by the plugin.

The existing room-visibility codec retains its private compatibility channel,
but TCP host/joiner runtime qualification remains open. Town Portal IDs above
255 are implemented only for the local player; remote portal packets and remote
portal operations fail closed. PluginSDK private channels are unavailable on
Battle.net, and this plugin does not alter Battle.net's protocol.

The DLL is an autonomous member of the RuffnecKk D2RLoader Suite. It does not
modify, link, merge, or redistribute any eezstreet plugin.

## Credits

D2MOO documented the historical fixed-threshold behavior and explicitly noted
that the act should be looked up from `Levels.txt`. D2MOO is used as a semantic
reference only; no legacy 32-bit address, structure, or ABI is reused.

Implementation and D2R 3.3 integration: `RuffnecKk`.
