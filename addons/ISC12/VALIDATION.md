# ISC12 validation gates

## Foundation and governance

- [x] Governed producer/consumer/sentinel ledger inventories 63 sites across
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

- [x] Nine one-byte width/sentinel/seed mutations have unique exact-or-masked
  signatures; the subsequent-reader call displacement is the single masked
  witness.
- [x] The first reader preserves the source-level `previousStatId = -1`
  invariant.
- [ ] Preflight and commit all nine sites as one quiescent-startup group.
- [ ] Prove both load orders with the existing `plugin-items` entry-hook owner.
- [ ] Keep G1 unpublished until G10 and G9 are closed.

## G2–G4 — player, save and preview codecs

- [ ] Pair every reader, writer and terminator with unique signatures.
- [ ] Preserve legacy-version rejection through G10 rather than implicit
  migration.
- [ ] Boundary and round-trip fixtures cover every player/stat preview path.

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
- [ ] Prove D2S outer-load unwrap ABI and failure cleanup.
- [ ] Prove D2S final writer, checksum and atomic disk commit.
- [ ] Prove D2I whole-file load before any item decode and full-sector preflight.
- [ ] Prove D2I whole-file writer and atomic disk commit.
- [ ] Freeze envelope magic, version, canonical schema descriptor and golden
  vectors only after all four file-level seams are ready.
- [ ] Vanilla and mismatched stores fail before payload decode.
- [ ] Disposable D2S and shared-stash fixtures round-trip twice.

## Runtime and gameplay

- [x] Foundation 0.1.0: two byte-identical Release builds with `/W4 /WX`,
  CTest `1/1`, PE x64, version 0.1.0 and three exports.
- [x] Loader stage 0.2.0: two byte-identical 179,200-byte Release builds,
  SHA-256 `C2B461CF8373CD3FD49D125A1DA9B195E6D917A62EE24CFEBFABD1FA0D1A4D93`,
  `/W4 /WX`, CTest `1/1`, PE x64 and three exports.
- [ ] Complete-stack global and mod-local cold starts.
- [ ] Disposable new-save gameplay and save/reload matrix.
- [ ] Matching host/joiner passes; mismatches fail closed.

No runtime deployment or real save is authorized before the applicable gates
are closed.
