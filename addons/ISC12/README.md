# ISC12 0.2.0

> Experimental loader stage: disabled by default. Do not distribute, deploy,
> or use with real saves.

ISC12 is a clean-sheet D2RLoader format for overhaul mods that need more than
511 `ItemStatCost` rows. It reserves serialized IDs `0..4094` for stats and
`0xFFF` as the list terminator.

Version 0.2.0 implements the first guarded native stage in source, but the
pinned D2RLoader SDK exposes no loader-owned publication-quiescence authority.
Consequently, `enabled=true` is currently refused before mutex acquisition,
preparation or any native write. A future loader-issued authority would permit
the prepared transaction to replace the fixed 512-entry `DescFunc` sorting tail
with bounded 4,095-entry storage, then raise the row cap from `0x1FF` to
`0xFFF`. The tail must be attempted first. Relay/state lifetime becomes
process-bound before that patch API call because a false result does not prove
that the non-aligned target bytes remained untouched. Any uncertain tail or cap
write requires a cold restart; hot rollback is forbidden.

The live process remains entirely 9-bit because the prepared generic-item,
player-save and full-item transport transaction is unpublished. The packet
transport now exists in source but is not reachable in production.
Consequently, this stage must not be used to create, load, save or transmit
ISC12 data. The shipped TOML and embedded fallback both keep the experiment
disabled.

The current off-runtime source also prepares the G10-B persistence boundary:
exact D2S/D2I objects can be unwrapped only after the 96-byte envelope, schema
hash and complete inner payload pass validation, while writes use an ISC12-owned
sibling-temp/full-write/flush/atomic-replace transaction. Persistent RX relays,
native cleanup continuations and rundown state are prepared but deliberately
unpublished. `InstalledHookCount` remains zero and `codecReady` is never set, so
neither save seam can execute until the 12-bit item/player codecs are complete.

G1–G4 remain one unpublished canonical four-group transaction, ordered G2, G4,
G1, G3 so overflow-status publication remains final. A critical static audit
invalidated G1's earlier narrow writer patch: the native serializer owner has a
511-DWORD compound-suppression table, and an ID at or above 511 would index
past that table into adjacent frame storage before G9 could stage the packet.
The source planner now rewrites the exact 42-byte body
`[0x37F17C,0x37F1A6)` in place. IDs `0..510` preserve the original table
comparison and compound suppression; IDs `511..4094` bypass the unsafe lookup;
the writer emits `min(ID, 0xFFF)` at 12 bits and retains the unchanged native
CALL at `0x37F1A1`. The schema continues to reserve `0xFFF` as the terminator.
The subsequent-reader window likewise must resolve its interior CALL exactly to
the governed native `BITSTREAM_ReadBitsThunk`; arbitrary retargeting fails
before any write. Fourteen new exact witnesses cover the owner frame, its sole
caller, the 511-DWORD clear, adjacent snapshot layout and all eight low-ID
compound writes.
The complete prepared source plan now contains 24 mutable sites, 102
differing-byte mutations and 77 witnesses. G1–G4 retain 20 sites and 84 slots;
G9 adds four transport sites and brings its governed surface to twelve
witnesses. Release `/W4 /WX`, CTest `4/4`, unique signatures and the expanded
`211/15` native ledger pass statically. The governed Release DLL SHA-256 is
`FF8D16AF4A6DBCB9BD3AD86A6A6DFCBB4553D26A200DF161B1065C6A5DFE5286`.
This is not a native publication or runtime claim.

Twelve exact native witnesses govern G9: the 0x9C/0x9D producers serialize one
node with recursion disabled, a root 0x9C has exactly 244 native payload bytes,
and every 0x9D node has exactly 239. Socketed descendants are emitted as
separate 0x9D packets. The added exact and unique epilogue witnesses are
`[0x479E23,0x479E41)` and `[0x47A019,0x47A037)`; they cover the cookie check,
stack deallocation, nonvolatile pops and `RET`. Vincent selected autonomous native-only G9-A on
30 August 2026. The pure snapshot planner now validates every node before its
first callback: exact payload caps, record/sentinel counts, packed child lists,
occupied-child count versus socket capacity, indices, shared children, cycles
and unreachable nodes. Accepted visits use the native depth-first preorder;
every rejection invokes zero callbacks. Partially filled socketed items remain
valid because occupied children may be fewer than the socket capacity. Native
D2R evidence bounds the occupied children of any one item to seven, but it does
not establish a global bound on the total nodes or depth of a recursively
socketed tree.

The same static audit bounds each node to at most seven emitted stat lists
(base, five set slots and runeword) and each list snapshot to 511 records. Six
low-ID compound primaries suppress eight partner records after emitting one ID
token with multiple values. The resulting 3,577-ID-token plus seven-sentinel
ceiling is structural only, not a realizable item or worst-case byte guarantee.
The prepared G9 path therefore validates the bytes copied from the one real
native serialization rather than authorizing a flush from record estimates.

Scratch serialization at producer entry remains rejected because both producers
temporarily alter item state before their real serialization. G9-A is now
source-prepared as the first canonical patch group. It publishes in the exact
fail-closed order queue 0x9C, queue 0x9D, entry 0x9C, entry 0x9D, so both native
queue calls are redirected before either producer entry can begin staging. The
previous long witnesses are split around the mutable CALLs at `0x479E10` and
`0x47A001`.

One allocation-free thread-local transaction copies the real packet bytes into
fixed storage capped at 64 packets and `0x4000` total bytes, with depth 16 and
seven immediate children per node. Only after the root returns does it validate
the complete captured batch, packet headers and lengths, preorder, parents,
cycles and all four caps. Rejection discards the batch before any real queue;
acceptance replays it through the unchanged native queue at `0x4817F0`.

The producer entries invoke registered 10-byte trampolines that preserve the
overwritten prologues and continue in the original functions. Their live unwind
metadata is registered with `RtlAddFunctionTable` before any publication can be
attempted; failure is fail-closed. SEH `finally` paths abort and discard the
transaction on abnormal producer exit. The source loader also requires
`RtlLookupFunctionEntry` to return matching live unwind metadata at both the
producer body and its `RET`: identical Begin/End/UnwindData, an End within the
image and a range covering the governed epilogue. The statically rehydrated
`.pdata` is protected and non-authoritative, so these live checks are not yet
attested and may reject the official runtime. These source/static contracts
pass, but the four sites remain unpublished and have not run in D2R.

For provenance only, the earlier Extended Item Transport prototype belongs to
RuffnecKk ExtendedItemStats and was exercised in an experimental RuffDood fork
build. It was never part of the pinned upstream eezstreet PluginPack and is not
an ISC12 product dependency or gate.

The separate G0-BBE gate fingerprints ten proof-only native sites. Seven
compiler call sites converge through one shared ItemStatCost resolver: it keeps
the ordinal in a dword, the generic compiler encodes every valid ISC12 ID
`512..4094` as token 5 plus a 16-bit operand, the evaluator restores it, and the
stat callback forwards the full dword to native stat accessors. A governed
read-only capture of D2R `3.3.93847` closes the two memberships hidden by the
protected static RDATA: `0x1CFA68C` contains `stat\0`, and record 5 of the nine
16-byte callback-info records at `0x1D09C50` points to `0x3B33F0` with arity 2.
Both code anchors matched exactly, no mutation right was requested and no write
was attempted. The ASLR-normalized table SHA-256 is
`E8B9B76F9D7320BDFA8129F8D22B0CB19B450AC4D655F53A8D2E56E47CF224ED`.
D2MOO remains semantic corroboration only. The official eezstreet
`items.playerConditionCalc` feature is only a coexistence surface and does not
own or close this proof.

The clean-sheet schema rejects `CsvBits > 32` and `CsvParamBits > 16`. The G2
and G3 exhaustive callers are prepared to enter process-lifetime RX relays,
then no-throw FRAME wrappers that accept only v105, validate marker, IDs,
widths, truncation, the 512-entry cap and terminator, and invoke the untouched
native readers under the immutable schema snapshot. Rejection returns the
native malformed status `0x12`; a successful native cursor that differs from
the preflight used-end fast-fails. G4 leaves its legacy branch A in 9-bit form,
changes only the v105 branch B, and prepares the exact copy CALL so the original
SaveObject copy runs once before whole-D2S validation. Invalid magic, version,
size, checksum, data context, marker, ID, payload bound or sentinel returns zero
through the native buffer-free/false exit.

The G3 finalize leaf preserves the native used-end result in RAX, returns the
sticky overrun flag in EDX, and is prepared without using D2R padding or
unwind-owned bytes. Its CALL is flushed before `mov eax, edx` is published as
the final site. A move-only opaque RAII lease now gates every full-set
fingerprint, write and flush, including loss-of-authority handling before and
after the first attempted mutation. The pinned D2RLoader SDK provides no
production issuer for that lease. Its public service IDs stop at 15 and neither
Lifecycle nor ThreadService excludes every D2R consumer; `D2RLoaderLoadPlugin`
is not a proven lease. A production-neutral one-shot coordinator is now
compiled and unit-tested without a production caller. It requires preflight in
the fixed order G0, G10, codec; reserves process lifetime once; commits those
three domains in the same order; publishes readiness only after all commits;
and makes `Poisoned` terminal without rollback. Its codec domain retains the
internal G9, G2, G4, G1, G3 order. Tests cover missing/revoked leases, every
preflight rejection, reentry, mutate-then-uncertain results, monotone terminal
states and move/release ownership. Publication still requires an upstream
synchronous loader-owned transaction that serializes publishers and blocks
resumption on poison, plus immutable adapters that split and bind the current
G0, G10 and codec preflight/commit paths. The process-lifetime relay page and G9
staging path are prepared, but no production Commit caller exists. All 102
mutations remain unpublished, `PublishedCodecMutationCount` is zero, both
`itemTransportReady` and `codecReady` stay false. ISC12 publication/cold start,
save/reload and multiplayer are NOT RUN; the only D2R interaction was the
external read-only G0-BBE capture described above.

## Planned release installation contract

After a future release is fully qualified, install the DLL and TOML in exactly
one D2RLoader scope.

Global:

```text
<D2R>/d2rloader/plugins/d2rl-ruffneckk-isc12.dll
<D2R>/d2rloader/config/ruffneckk-isc12.toml
```

Mod-local:

```text
<D2R>/mods/<mod>/d2rloader/plugins/d2rl-ruffneckk-isc12.dll
<D2R>/mods/<mod>/d2rloader/config/ruffneckk-isc12.toml
```

ISC12 saves will require a versioned marker and an `ItemStatCost` schema
fingerprint. Vanilla saves and mismatched schemas will be rejected before any
payload decode. Migration is intentionally delegated to a separate external
tool.

The 0.2.0 binary is an internal incubation artifact, not an installable
release. Runtime qualification will use disposable profiles and saves only
after the clean-sheet and network gates are implemented.

## Credits

- Author: `RuffnecKk`.
- D2MOO is credited for semantic knowledge of the historical ItemStatCost
  compiler and stat-list format. All addresses, signatures and x64 ABI are
  independently proven against the governed current D2R corpus.
- D2RLoader and PluginSDK provide the autonomous plugin runtime.
