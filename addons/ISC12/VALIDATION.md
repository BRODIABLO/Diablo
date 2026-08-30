# ISC12 validation gates

## Foundation and governance

- [x] Governed producer/consumer/sentinel ledger inventories 172 sites across
  14 atomic or exclusion groups.
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
- [ ] Prove quiescent or transactional publication of the non-aligned
  eight-byte `PatchJmpRel32` seam.
- [ ] Native relay preserves the same order and stack safety at runtime.

## G1 — generic item codec

- [x] Nine one-byte width/sentinel/seed mutations across four unique windows
  have exact signatures; the subsequent-reader interior CALL resolves exactly
  to governed `BITSTREAM_ReadBitsThunk 0xA1B6C0`, and retargeting is rejected
  before any write.
- [x] The first reader preserves the source-level `previousStatId = -1`
  invariant.
- [x] Integrate all nine mutations into the canonical G1–G4 full-set preflight.
- [x] Require a valid production-issued quiescence lease before G1 can commit
  or publish with the canonical set.
- [ ] Prove both load orders with the existing `plugin-items` entry-hook owner.
- [x] Keep G1 unpublished: no production lease issuer or commit caller exists,
  and G9 plus the remaining activation gates are not closed.

## G2–G4 — player, save and preview codecs

- [x] Combine G1 with these three groups in one canonical prepared set totaling
  20 exact mutable windows, 49 governed byte slots and 51 runtime witnesses.
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
- [x] Keep all 40 G2–G4 slots and all 49 canonical mutation slots unpublished
  (`PublishedCodecMutationCount == 0`).
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
- [ ] Obtain a documented loader-owned production issuer for that lease;
  reserve the relay process-lifetime before the first codec write and fast-fail
  on any uncertain write/flush before quiescence ends.
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
- [ ] `0x9C/0x9D` compose with the existing transport owner.
- [ ] No one-byte packet length wraps.

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
- [x] G10-B P3b plus the canonical G1–G4 planner: current Release build passes
  `/W4 /WX` and CTest `3/3`; the governed ledger is `VALID` at 172 sites / 14
  groups, with 20 mutable codec windows, 49 mutation slots, 51 runtime
  witnesses and zero publication.
- [ ] Complete-stack global and mod-local cold starts.
- [ ] Disposable new-save gameplay and save/reload matrix.
- [ ] Matching host/joiner passes; mismatches fail closed.

No runtime deployment or real save is authorized before the applicable gates
are closed.
