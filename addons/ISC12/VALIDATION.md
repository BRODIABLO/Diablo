# ISC12 validation gates

## Foundation and governance

- [x] Governed producer/consumer/sentinel ledger inventories 211 sites across
  15 atomic, proof-only or exclusion groups.
- [x] The ledger validator rejects every `ready` site without a concrete,
  unique expected byte pattern.
- [ ] Quantity and State-ID exclusions are fingerprinted.
- [ ] Installed Suite and five eezstreet DLLs have no competing owner.
- [x] API v3, manifest resource, three exports and hybrid flags are scaffolded.
- [x] No build-name/version allowlist exists.
- [x] The 0.2.0 TOML and embedded fallback are disabled by default.
- [x] The 0.2.0 target is not eligible for a public archive.
- [x] Duplicate-scope mutex is PID-qualified: one owner per D2R process without
  blocking a second local host/joiner process.

## G0 — loader and DescFunc

- [x] Pure commit tests prove lifetime reservation → tail → conservative cap
  guard → operational gate → count cap; a false tail result performs no later
  write and enters the uncertain-commit path, while cap failure enters the
  guarded cold-restart state.
- [x] A false tail patch result keeps RX/RW resources process-lifetime, logs the
  observed eight-byte seam and immediately terminates fail-closed.
- [x] A failed cap API call is treated as potentially mutating; inactive relay
  fallback accepts only a proven row count at most 511.
- [x] Pure builder accepts 511, 512, 1023, 2047 and 4095 rows.
- [x] Pure builder rejects 4096 rows without changing its destination.
- [x] Dense and sparse fixtures preserve the native ascending signed-16-bit
  `DescPriority` order, including stat ID 4094 and priorities `0x7FFF`,
  `0x8000` and `0xFFFF`.
- [x] MASM restores the original RAX and R9 on vanilla fallback and all six
  remaining nonvolatile registers on success.
- [x] Callback rundown decrements only inside persistent RX success/vanilla
  exits, after the thread has left all DLL code.
- [x] Unsafe inactive-relay state and rundown timeout use Windows fast-fail,
  never a resumable `UD2` or an unsafe unload.
- [x] Require a live `NativePublicationQuiescenceLease` before G0 resource
  reservation or mutation; current `enabled=true` refuses with zero writes
  because the pinned SDK has no issuer. Lease loss before the first write and
  after an attempted write are unit-tested as distinct outcomes.
- [ ] Prove quiescent or transactional publication of the non-aligned
  eight-byte `PatchJmpRel32` seam.
- [ ] Native relay preserves the same order and stack safety at runtime.

### G0-BBE — separate native expression path

- [x] Census seven compiler callsites, three resolver frontends converging on
  one shared resolver core, seven D2R evaluator callsites using one protected
  callback table/count, and the eighth generic evaluator call using no table.
- [x] Prove the ItemStatCost branch loads `DataTables+0x1270`, resolves the
  name and stores the returned ordinal as a full dword without a `0x1FF` mask.
- [x] Prove IDs `512..4094` compile as callback token 5 plus a 16-bit operand,
  decode unchanged and positive into EDX, and remain full-width through the
  attested stat callback's row guard and three stat-accessor tails.
- [x] Govern ten exact proof-only sites under `G0-BBE-stat-formula`; the ledger
  validator rejects any target other than `unchanged` for this group.
- [x] Attest from governed read-only D2R `3.3.93847` memory that protected
  literal `0x1CFA68C` is the slot-5 `stat` keyword and record 5 of the nine
  16-byte callback-info records at `0x1D09C50` links that slot to callback
  `0x3B33F0` with arity 2. Both code anchors matched; no write right or
  mutation was used. Preserve the raw/session and ASLR-normalized hashes in the
  governed findings.
- [ ] During a future qualified cold start, compare the same protected 16/144
  bytes before TXT compilation and after `Gameplay data tables loaded` to
  strengthen lifecycle immutability; any divergence keeps runtime publication
  closed.
- [ ] Compile/evaluate disposable ID-512 and ID-4094 fixtures after the runtime
  gates open, without a real save.
- [x] Keep G0-BBE distinct from the official eezstreet
  `items.playerConditionCalc` feature; PluginPack coexistence cannot close the
  ISC12 native ordinal-width proof.

## G1 — generic item codec

- [x] Prove the critical bounded-serializer defect in the earlier plan: the
  owner has a 511-DWORD compound-suppression table and its lookup at
  `0x37F17C` would read beyond that table for every ID at or above 511.
- [x] Replace the superseded narrow writer-ID site in the source planner with
  the exact in-place 42-byte body `[0x37F17C,0x37F1A6)`, without double
  ownership. Preserve the table compare/suppression for IDs `0..510`; bypass
  the lookup for larger IDs;
  emit `min(ID,0xFFF)` at width 12; keep the native CALL at `0x37F1A1`
  unchanged and continue rejecting a real stat ID `0xFFF` in the schema.
- [x] The reader width/sentinel/seed mutations have exact signatures; the
  subsequent-reader interior CALL resolves exactly to governed
  `BITSTREAM_ReadBitsThunk 0xA1B6C0`, and retargeting is rejected before any
  write.
- [x] The first reader preserves the source-level `previousStatId = -1`
  invariant.
- [x] The canonical G1–G4 full-set preflight, bounded replacement semantics and
  corrupt-fingerprint tests pass with the 42-byte writer body integrated.
- [x] Require a valid production-issued quiescence lease before G1 can commit
  or publish with the canonical set.
- [x] Keep G1 unpublished: no production lease issuer or commit caller exists,
  and G9 plus the remaining activation gates are not published.

## G2–G4 — player, save and preview codecs

- [x] Historical bounded-G1 intermediate checkpoint after replacing the old
  narrow writer site: 20 mutable sites, 84 differing byte slots and 71
  witnesses. Bounded G1 contributes 44 slots plus 14 new owner/caller/table/
  snapshot/compound witnesses; G2–G4 retain 40 slots/51 witnesses and the
  intermediate total included six then-read-only G9 evidence surfaces.
- [x] Integrate G9-A as the first canonical group, bringing the whole prepared
  transaction to 24 mutable sites, 102 differing-byte mutations and 77 witnesses.
  Its internal order is queue `0x9C`, queue `0x9D`, entry `0x9C`, entry `0x9D`;
  G2, G4, G1 and G3 follow.
- [x] Pair every reader, writer and terminator with unique signatures; the G4
  owner and version dispatcher prove its four ID reads are exhaustive while
  its two schema-driven value reads remain unchanged.
- [x] Preserve legacy-version rejection through G10 rather than implicit
  migration.
- [x] Pure 12-bit ID fixtures cover boundary IDs, the `0xFFF` terminator and
  missing-terminator rejection without changing the caller's output.
- [x] Prepare 40 governed byte slots across 16 exact sites as three fail-closed
  atomic groups: all mutation signatures plus 51 unchanged
  owner/return/capacity/layout/cleanup/overrun witnesses
  preflight before the first write, publication requires the canonical live
  quiescence lease and any false write or flush result requires a cold restart.
- [x] Keep all 40 G2–G4 slots unpublished.
- [x] Confirm all 84 recomputed canonical G1–G4 slots remain unpublished with
  `PublishedCodecMutationCount == 0` after source integration.
- [x] Confirm all 102 slots in the G9 plus G1–G4 transaction remain unpublished;
  `itemTransportReady == 0`, `codecReady == 0`, and no production Commit caller
  exists.
- [x] Reject the native schema fail-closed when `CsvBits > 32` or
  `CsvParamBits > 16`, matching the native 32-bit value and 16-bit parameter
  widths. Under the unchanged 512-entry cap, a complete worst-case section is
  3,844 bytes including marker and fits G2's fixed `0x4000` buffer.
- [x] Pure G2/G3 preflight validates marker `0x6667`, schema/ID bounds, every
  param/value width, at most 512 entries, truncation and the `0xFFF` sentinel;
  failure leaves output unchanged.
- [x] Retarget the exhaustive G2/G3 CALLs `0x531A6D`, `0x52EC4A` and
  `0x530A34` in the prepared set, leaving native owners `0x530A00`/`0x533760`
  untouched. Process-lifetime RX entries, FRAME wrappers and no-throw helpers
  preserve the six-argument ABI, accept exact v105 only, return native error
  `0x12` on preflight rejection and fast-fail on native fault or successful
  cursor divergence. The schema shared lock spans preflight, native decode and
  postcheck.
- [x] Prepare governed propagation of the G3 bit-writer overrun flag: a copied
  RX leaf preserves the native used-end result, returns the DWORD at
  `[bitstream+0x20]` in EDX, redirects the exact CALL at `0x5353C2`, flushes
  it, then publishes and flushes `mov eax, edx` at `0x5353D2` as the final
  canonical G1–G4 site.
  Signed rel32 limits, opaque loader provenance, no-op displacement bytes,
  corrupt fingerprints, partial writes and both final flush failures are unit
  tested. No code cave or unwind-owned byte is used.
- [x] Replace the forgeable quiescence boolean with a move-only opaque RAII
  lease, validate it before every fingerprint/write/flush and unit-test absent
  or revoked authority plus partial-mutation cold-restart handling.
- [x] Add a production-neutral one-shot full-set coordinator with no production
  caller: preflight G0, G10 and codec before reservation/write; reserve process
  lifetime once; commit G0, G10 and codec; publish readiness strictly last;
  make `Poisoned` terminal and invoke the infallible poison-state callback
  exactly once without rollback. Unit-test absent/revoked leases, every
  preflight rejection, reentry, mutate-then-uncertain outcomes, monotone
  terminal states, lease move/release and loss of authority during final
  readiness publication; the post-readiness lease check poisons and clears the
  publication before any possible resume.
- [ ] Obtain a documented synchronous loader-owned production transaction for
  the lease; it must serialize publishers, exclude every native consumer and
  prevent runtime resumption on a poisoned result. Split G0, G10 and codec into
  immutable preflight/commit adapters, bind them to the tested coordinator,
  reserve the real relays/state before the first write and fast-fail on any
  uncertain write/flush before quiescence ends.
- [x] Retarget only preview CALL `0x61CF90` in the prepared set, call shared
  native copy owner `0xA1E110` exactly once, then validate whole v105 D2S,
  context `<4`, wrapper-required marker `0x6667`, every schema-bound ID/payload,
  cap 512 and `0xFFF` within copied `N<=0x4000`; rejection returns zero through
  the governed buffer-free/false exit `0x61D87D`. Legacy branch A stays 9-bit.
- [x] Unit-test G4 valid input plus bad magic/version/declared size/checksum,
  context, marker, ID, missing sentinel, native-underflow length 342 and
  capacity overflow, with failure output unchanged.

## G5–G9 — network

- [ ] `0x3E`, `0xA8` and `0xAA` pairs pass boundary tests.
- [ ] `0xAC` headroom and fallback are resolved.
- [x] Govern twelve exact G9 witnesses proving that 0x9C/0x9D serialize one node
  with recursion disabled, the root cap is 244 bytes for 0x9C, every 0x9D cap
  is 239 bytes, socketed descendants are separate 0x9D packets, and the
  unchanged real queue is `0x4817F0` with its span dispatch.
- [x] Add exact unique producer-epilogue witnesses `[0x479E23,0x479E41)` and
  `[0x47A019,0x47A037)`, covering cookie check, stack deallocation,
  nonvolatile pops and `RET`.
- [x] Purely account for every node's 12-bit record/sentinel tokens, root versus
  descendant packet kind and exact native boundaries: 244 bytes for a root
  0x9C and 239 bytes for every 0x9D, including a root sent as 0x9D.
- [x] Record Vincent's 30 August 2026 product decision: autonomous native-only
  G9-A, with the exact native caps as the sole product contract.
- [x] Implement and test an allocation-free pure snapshot planner that rejects
  invalid counts, packed child lists, socket capacity, indices, shared children,
  cycles, unreachable nodes and oversized late descendants before any callback.
- [x] Preserve native depth-first preorder and accept partially filled socketed
  items by separating occupied-child count from total socket capacity.
- [x] Prove the native per-item occupied-child upper bound `occupied <= 7`:
  the `0x37D60B` loop counts immediate children, clamps at seven and writes
  three bits at `0x37D66F..0x37D678`, independently from socket capacity.
  This local branching bound does not prove a global node or depth bound for a
  recursively socketed tree.
- [x] Impose an independent ISC12 staging contract of 64 packets, `0x4000`
  total captured bytes, depth 16 and seven immediate children per node. These
  fixed safety caps do not claim a native/global tree maximum.
- [x] Split the long 0x9C witness into
  `0x479D85+139` and `0x479E15+14`, and the long 0x9D witness into
  `0x479F76+139` and `0x47A006+19`, excluding the five-byte CALLs at
  `0x479E10` and `0x47A001` from every unchanged witness.
- [x] Fix the engineering sequence as bounded G1 serializer first, then G9
  staging. This sequencing is not permission to publish either group.
- [x] Prepare the four-site native staging transaction in the fail-closed order
  queue `0x9C`, queue `0x9D`, entry `0x9C`, entry `0x9D`. Copy actual native
  packet bytes into fixed thread-local storage; validate the complete batch
  before any real call to `0x4817F0`; discard every rejected batch with zero
  sends. Do not use a second scratch serialization at producer entry.
- [x] Use registered 10-byte producer trampolines, register live unwind metadata
  through `RtlAddFunctionTable` before publication, fail closed on registration
  failure, and abort/discard the transaction from SEH `finally` on abnormal
  producer exit.
- [x] Require source-side live `RtlLookupFunctionEntry` checks at each producer
  body and `RET`: matching Begin/End/UnwindData, End inside the image and a
  function range covering the governed epilogue.
- [ ] Attest those lookups against the official live image. Rehydrated static
  `.pdata` is protected and non-authoritative; the runtime may fail closed.
- [x] Prove the structural native cardinalities: at most seven emitted stat
  lists and sentinels per node, at most 511 snapshot records per list, and six
  low-ID compound primaries suppressing eight partner records. The resulting
  ceiling of 3,577 ID tokens plus seven sentinels is not a realizable item or a
  worst-case byte bound.
- [x] Validate actual captured packet bytes, packet headers/lengths, preorder,
  parent links, cycles and all four fixed caps across the complete batch before
  source logic permits replay.
- [ ] Exercise live-tree stability, TLS reentry, overflow, backpressure and
  delayed queue semantics in D2R before any runtime claim.
- [ ] Prove the G9-A preflight with the complete Suite and the five official
  eezstreet plugins without assigning transport ownership to any of them.
- [x] Source policy admits only native packet lengths through `0xFC`, so an
  accepted staged packet cannot wrap its one-byte length.

## G10 — clean-sheet format gate

- [x] D2S outer-load, magic/version dispatch, modern decoder and first
  player-stat decode boundary have exact unique signatures.
- [x] Version-only, padding/reserved-byte and sidecar marker designs are
  rejected as non-fail-closed.
- [x] Prove D2S outer-load replacement-buffer lifetime, native record cleanup,
  output nulling and failure destruction.
- [x] Identify the physical D2S/D2I state-1 writer and prove vanilla commit is
  direct, non-atomic and blind to failed or partial writes.
- [x] Prove D2I physical whole-file ingress and upload splitting before either
  native sector reader can mutate an item.
- [x] Implement an ISC12-owned sibling-temp, full-write, flush, atomic replace
  and rollback transaction for both physical stores.
- [x] Implement and unit-test the disconnected atomic-file primitive: existing
  and absent destinations, looping partial writes, flush, write and replace
  failures preserve the original and clean ordinary siblings.
- [x] Prove the `0x00B3` fragment format, client handler, collection terminator,
  whole-D2S validation and copy into object state `4`.
- [x] Trace object state `4` and the secondary concatenated store to the shared
  physical writer.
- [x] Freeze the writer job's status/callback/lock/tag/failure contract: every
  pre-commit ISC12 failure reuses native status `6` and the existing
  open-failure branch; no automatic retry is claimed.
- [x] Freeze exact object classification: path-free lower-case `.d2s`, but only
  the four canonical full shared-stash names for `.d2i`; every other manager
  object passes through vanilla.
- [x] Freeze reader rejection cleanup: resize buffer to zero, reset aux/state,
  then reuse native close/unlock/status-6 before either direct caller can
  publish the object.
- [x] Govern exact unique five-byte reader `0x9FC654` and writer `0x9F95A2`
  mutation seams without claiming either hook is installed.
- [x] Connect both seams off-runtime to process-lifetime RX relays and separate
  RW rundown state; the reader unwraps only a complete accepted envelope and
  the writer reaches the final path only through the atomic transaction.
- [x] Keep persistence publication impossible in this stage:
  `InstalledHookCount == 0`, `codecReady == false`, and neither seam is patched.
- [x] Preserve the tail-entered ABI contract without claiming general unwind:
  MASM metadata covers each stub's local allocation only; all callbacks are
  `noexcept`, expected faults are contained and unexpected failures fast-fail.
- [x] Freeze the 96-byte envelope v1, exact length, store kind, codec/sentinel,
  schema and payload SHA-256 fields plus deterministic D2S/D2I inner preflight.
- [x] Freeze canonical schema descriptor v1 with strict UTF-8 `Stat`, physical
  ordinal, normalized semantic flags, current v105 save fields, resolved
  references and effective `stuff`; `*ID`, legacy and display-only fields are
  excluded.
- [x] Prove that strict D2S/D2I version 105 selects only current save fields;
  the individual item tag does not reactivate the legacy ItemStatCost layout.
- [x] Golden fixtures prove rename, reorder and current semantic mutation while
  all three unreachable legacy fields leave the descriptor unchanged;
  wrong schema, truncation, trailing bytes, mixed D2I sectors and payload hash
  corruption; builders leave output unchanged on rejection.
- [x] Disconnected whole-store policy passes every non-target object through,
  rejects failed/short target reads and prepares valid target unwrap/wrap only
  after the complete envelope contract passes.
- [x] Recover the exact runtime `Stat` sequence from `DataTables+0x1270` through
  governed count/name functions, copy every linker-owned name immediately and
  publish the canonical snapshot/hash before any `DescFunc` mutation.
- [x] Reject a divergent schema reload fail-closed; an identical reload reuses
  the immutable published snapshot without a sidecar or generated manifest.
- [x] Pure policy and native-adapter fixtures reject vanilla, malformed and
  schema-mismatched target stores before replacing the native payload buffer.
- [ ] Disposable D2S and shared-stash fixtures round-trip twice.

## Runtime and gameplay

- [x] Foundation 0.1.0: two byte-identical Release builds with `/W4 /WX`,
  CTest `1/1`, PE x64, version 0.1.0 and three exports.
- [x] Loader stage 0.2.0: two byte-identical 179,200-byte Release builds,
  SHA-256 `C2B461CF8373CD3FD49D125A1DA9B195E6D917A62EE24CFEBFABD1FA0D1A4D93`,
  `/W4 /WX`, CTest `1/1`, PE x64 and three exports.
- [x] Last verified pre-discovery baseline: G10-B P3b plus the former canonical
  G1–G4 planner passed Release `/W4 /WX` and CTest `3/3`; the governed ledger
  was `VALID` at 178 sites / 14 groups, with 20 mutable codec windows, 49
  mutation slots, 57 witnesses and zero publication.
- [x] Intermediate bounded-G1 source/static gate: Release `/W4 /WX`, CTest `3/3`,
  governed ledger `VALID` at 193 sites / 14 groups and every added signature
  unique, with 20 mutable sites, 84 differing slots, 71 witnesses and zero
  native publication.
- [x] Current G9-A plus production-neutral coordinator source/static gate: the
  full Release `/W4 /WX` build and CTest `4/4` pass; the governed DLL SHA-256 is
  `FF8D16AF4A6DBCB9BD3AD86A6A6DFCBB4553D26A200DF161B1065C6A5DFE5286`; the
  ledger is `VALID` at 211 sites / 15 groups, the canonical transaction has
  24 mutable sites, 102 mutations, 77 witnesses and zero publication.
- [ ] Complete-stack global and mod-local cold starts.
- [ ] Live 0x9C/0x9D root/descendant, overflow, reentry, backpressure and
  coexistence cases.
- [ ] Disposable new-save gameplay and save/reload matrix.
- [ ] Matching host/joiner passes; mismatches fail closed.

No ISC12 publication runtime, real save or multiplayer validation has run for
the current G9-A source-prepared state. The external read-only G0-BBE capture
closes native membership only. The coordinator is source-prepared/tested but
has no production adapters or caller. A real loader-owned
publication-quiescence authority, the domain split/binding and the remaining
activation gates are still required before publication. Only after those gates
close may cold-start and the live matrix begin.
