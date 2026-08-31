# ISC12 0.2.0

> Experimental loader stage: disabled by default. Do not distribute, deploy,
> or use with real saves.

ISC12 is a clean-sheet D2RLoader format for overhaul mods that need more than
511 `ItemStatCost` rows. It reserves serialized IDs `0..4094` for stats and
`0xFFF` as the list terminator.

Version 0.2.0 now has a production-callable experimental publication path. It
uses a same-thread authority bounded to the synchronous initial
`D2RLoaderLoadPlugin` callback, preflights every G0, G10 and codec surface before
the first write, reserves relay/state lifetime, then commits G0, G10 and codec
in one startup window. This follows D2RLoader's official startup patching model;
it does not claim that the current SDK documents a global quiescence service.

The fixed 512-entry `DescFunc` sorting tail is replaced with bounded 4,095-entry
storage before the row cap moves from `0x1FF` to `0xFFF`. G10 persistence and
the G9/G2/G4/G1/G3 codec plan then commit before readiness is published. A false
patch result may follow a real write, so every post-mutation ambiguity poisons
readiness and terminates the process; hot rollback is forbidden. The shipped
TOML and embedded fallback remain disabled because runtime qualification has not
completed. Do not create, load, save or transmit ISC12 data outside an isolated
test profile with disposable saves.

The current source also includes the G10-B persistence boundary:
exact D2S/D2I objects can be unwrapped only after the 96-byte envelope, schema
hash and complete inner payload pass validation, while writes use an ISC12-owned
sibling-temp/full-write/flush/atomic-replace transaction. Persistent RX relays,
native cleanup continuations and rundown state now participate in the canonical
startup publication. The isolated D2RLoader 1.2 cold start published G0, G10,
G9 and G1-G4 together and reached complete frontend startup; their first
persistence execution is still NOT RUN.

G0 no longer treats the first compiled ItemStatCost table as globally
authoritative. D2R compiles both the 368-row Classic/base table and the 400-row
ISC12Lab expansion table in one load. ISC12 stages each exact compiled snapshot
with `SchemaReady=false`; D2RLoader's existing `DataTablesLoaded` event and
`DataTableServiceV1` RotW TableView then select the matching records pointer and
row count. Finalization decodes the active bytes again, so in-place loader
post-processing or pointer reuse cannot silently publish the earlier capture.
Only that authoritative snapshot is published. A later non-zero revision token
distinct from the previous load must match its semantic hash or the process
stops fail-closed. Listener shutdown is an atomic `Stopping` state; a callback
racing explicit unregister returns benignly while unregister waits for every
already-running callback.

G1–G4 remain one canonical four-group subplan, ordered G2, G4,
G1, G3 so overflow-status publication remains final. A critical static audit
invalidated G1's earlier narrow writer patch: the native serializer owner has a
511-DWORD compound-suppression table, and an ID at or above 511 would index
past that table into adjacent frame storage before G9 could stage the packet.
The source planner now rewrites the exact 42-byte body
`[0x37F17C,0x37F1A6)` in place. IDs `0..510` preserve the original table
comparison and compound suppression; IDs `511..4094` bypass the unsafe lookup;
the writer emits `min(ID, 0xFFF)` at 12 bits and retains the live CALL at
`0x37F1A1`. That CALL may target the canonical native bit writer directly or
the exactly attested D2RCore `WriteItemSaveStatId` provider; ISC12 never
rewrites the provider. The schema continues to reserve `0xFFF` as the
terminator.
The subsequent-reader window likewise must resolve its interior CALL exactly to
the governed native `BITSTREAM_ReadBitsThunk`; arbitrary retargeting fails
before any write. Fourteen new exact witnesses cover the owner frame, its sole
caller, the 511-DWORD clear, adjacent snapshot layout and all eight low-ID
compound writes.

D2RLoader 1.2 also composes the governed compiler, G3 stat writer and dynamic
player-save caller through D2RCore. ISC12 accepts either each canonical direct
native target or the corresponding exactly attested provider. Admission binds
the export/body, live PDATA and unwind contract, a bounded unconditional relay
chain, the live forward slot and the exact native destination. The dynamic
caller is accepted only as an indivisible pair: vanilla `0x8000` capacity plus
direct serializer, or D2RLoader `0xFFFF` capacity plus
`WritePlayerSaveWithEnvironmentCapture`. ISC12 preserves the pair and all
provider CALLs. The save-stat providers still forward IDs above 511 unchanged;
only D2RLoader's private 512-bit compatibility census omits those IDs, which is
a separate metadata/network-hardening gap.

The complete prepared source plan now contains 24 mutable sites, 102
differing-byte mutations and 77 witnesses. G1–G4 retain 20 sites and 84 slots;
G9 adds four transport sites and brings its governed surface to twelve
witnesses. Two Release `/W4 /WX` builds, CTest `5/5`, unique signatures and the
expanded `211/15` native ledger pass. The byte-identical cold-started DLL is
445,952 bytes with SHA-256
`EFCA4EBAECDC7E0EF7BE70D2BE741FD7D73DED0ACA85873507CCA2D2B625F3DB`.

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
temporarily alter item state before their real serialization. G9-A is the first
canonical patch group. Its startup commit order is
queue 0x9C, queue 0x9D, entry 0x9C, entry 0x9D, so both native queue calls are
redirected before either producer entry can begin staging. The
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
`.pdata` is protected and non-authoritative. The isolated cold start attested
the live PDATA/XDATA contract and published all four G9 sites; functional
0x9C/0x9D item cases remain open.

The specialized network groups are a separate open gate. Packet `0x3E` (G5),
`0xA8` (G6) and `0xAA` (G7) are present only in the reverse-engineering ledger;
`0xAC` (G8) is still blocked on cardinality, estimator and headroom proof. ISC12
has not implemented any fixed-byte `uint8 statId` to `uint16` packet expansion.
Therefore this build does not claim complete network coverage for IDs above the
vanilla range even if the initial `0x9C`/`0x9D` tests pass.

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
the final site. A borrowed, validate-only `NativePublicationLeaseView` now gates
every full-set fingerprint, write and flush. Its production instance is
constructed only by the initial-load window, owns no loader resource and has no
release operation. The DLL
still builds against the governed PluginSDK v3 commit
`4933e2c42cb2592958cd0df3b6dc5003102252d1`; a separate audit of PluginSDK v4
`6eb8f8b6192868214706bd6d528c5294f2f551b7` finds services through 17
(`Http = 16`, `ItemInteraction = 17`) but no native-publication service.

The one-shot coordinator is connected to real local G0, G10 and codec adapters
and has exactly one production caller in `D2RLoaderLoadPlugin`. Their immutable
plans are all preflighted under the same borrowed view before reservation or
write. The coordinator then reserves process lifetime once, commits G0, G10
and codec, and keeps codec order G9/G2/G4/G1/G3. It then stops in
`CommittedPendingReadiness` with all private readiness flags false. While the
same initial-load window remains active, a separate no-write step publishes
readiness exactly once and a postcondition check requires every G0/G10/codec
surface to be active. Tests cover every preflight domain, native
patch/write/flush failure, revocation, reentry, mutate-then-uncertain outcomes
and both startup-readiness paths.
An already-equal byte is a confirmed no-op; an actual attempted write makes any
ambiguity terminal. The optional upstream `NativePublication V1` proposal is
retained as future hardening rather than a prerequisite for this experiment.
An isolated full-stack cold start executed the exact DLL above on D2R
3.3.93847 with D2RLoader 1.2, the active RuffnecKk Suite and all five eezstreet
plugins. It accepted every D2RCore provider and live unwind contract, published
G0/G10/G9/G1-G4, compiled 190 TXT tables, selected the RotW ItemStatCost table
at revision 1 (`rows=400`, `G0-builds=2`, `SchemaReady=true`) and reached
`D2R startup complete`. D2RLoader reported 36 plugins loaded, two known
unrelated mod-local failures (Stash Search and Revive Overhaul) and 17 patches.
This closes mod-local publication
and cold-start only: persistence execution, functional 0x9C/0x9D cases,
save/reload, gameplay and multiplayer remain NOT RUN.

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
