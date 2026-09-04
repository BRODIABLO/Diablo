# Extended Act Level IDs validation

Version 2.0.2 implements the approved two-pass cache: it validates every
physical `Levels` row first, then retains the keyed identity round-trip only for
IDs `0..255`. Static qualification is complete. The exact 1023-record runtime,
Level ID 256+ gameplay, multiplayer, travel, persistence, and release gates
remain open.

## Version 2.0.2 two-pass implementation gate — 4 September 2026

- Vincent authorized `GO implantation cache physique deux passes` after the
  2.0.1 diagnostic proved that physical `getRow(256)` succeeds while
  `findRowById(256)` returns `NotFound`.
- Pass one validates and copies every physical record in the bank, preserving
  revision, pointer, row-index, row-size, `Id == rowIndex`, `Id <= 1022`, and
  `Act <= 4` checks.
- Pass two reacquires each physical row and preserves the complete keyed
  pointer/index/revision/row-size identity check for at most IDs `0..255`.
- A failure in either pass still rejects the whole bank. Classic, LoD, and RotW
  caches remain unpublished until all three banks pass.
- No native seam, hook, signature, compatibility token, codec, service ABI,
  save behavior, configuration, or plugin dependency changed.
- Boundary policy tests now cover row counts `255`, `256`, `257`, and `1023`.
- The Release test translation unit explicitly re-enables `assert`; codec,
  anchor, source-contract, and two-pass ordering checks therefore execute under
  the same Release configuration used for the candidate DLL.
- Two independent clean Release builds pass CTest `1/1` without compiler
  warnings and are byte-identical: 61,440 bytes, SHA-256
  `FDAF884B69AB879D8F3946E2CF036FF25AF3C03C7CBE479803761A19B13A6EC1`.
- PE metadata reports `RuffnecKk / 2.0.2`; the DLL exports exactly
  `D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin`, and
  `D2RLoaderUnloadPlugin`, retains its API v3 manifest resource declaration,
  and imports only the expected Windows/MSVC runtime set.
- The two local candidates are under
  `analysis-cache/extended-act-level-ids-v2/build-two-pass-a/Release/` and
  `analysis-cache/extended-act-level-ids-v2/build-two-pass-b/Release/`. Neither
  has been deployed, packaged, or promoted.
- The next gate is a complete-stack cold start with the exact 1023-record
  fixture. Success requires the RotW cache to publish all 1023 rows; no result
  is inferred from the static build.

## Version 2.0.0 static contract

- Compiled `Levels` accepts `1..1023` records and canonical IDs `0..1022`.
- Native capacity witness: `D2R.exe+0x330446`, where the unique exact sequence
  compares the compiled count with `0x400` and accepts only values below it.
- Server packet 0x07 builder: `D2R.exe+0x47D2D0`.
- Server packet 0x08 builder: `D2R.exe+0x47EAF0`.
- Both builder ABIs receive the complete Level ID in `EDX`, then stock D2R
  stores only `DL` at packet byte `+5`; both six-byte entry signatures are
  unique in the governed common corpus.
- Client room-in-sight path: `D2R.exe+0x328680`.
- Client room-out-of-sight path: `D2R.exe+0x328780`.
- Both client ABIs receive `(dataContext, act, levelId, x, y, room)` and pass
  the full 32-bit Level ID to `DRLG_GetLevel`; both entry signatures are unique.
- `D2Client` identity witness: `D2R.exe+0x485B51`, uniquely proving the player
  GUID at `D2Client+0x270` alongside unit type `+0x26C`, game `+0x2B0`, and the
  call to `SUNIT_GetServerUnit`.
- The v2 codec keeps the stock six-byte 0x07/0x08 packet. A `0x8000` marker and
  two Level-ID payload bits occupy X bits 15..13; X bits 12..0 remain the
  original coordinate. This supports Level IDs `256..1022` and X `0..8191`.
- PluginSDK `NetworkServiceV1` compatibility token
  `0x454C494456320001` gates encoding by the receiving player GUID. A missing,
  incompatible, Battle.net, unknown, or out-of-contract peer fails closed.
- Version 2.0.0 adds no save hook, changes no D2S/D2I bytes, and does not resize
  waypoint or portal-flag persistence.

## Version 2.0.0 automated gates

- [x] Release build succeeds with MSVC 19.44 / Windows SDK 10.0.26100.0.
- [x] Policy and codec boundary tests pass (`1/1`).
- [x] All six hook/layout signatures and the capacity witness have one exact
  match each in the governed common 92777/93847 corpus.
- [x] No build-name or version allowlist is present.
- [x] Two independent clean Release builds are byte-identical: 60,416 bytes,
  SHA-256 `1874623DA1B4914BE465A430D117B19174D986CF9E326E057FFEB115AD10C508`.
- [x] PE metadata reports `RuffnecKk / 2.0.0`; the DLL exports exactly the
  three D2RLoader entry points, embeds API v3 manifest resource `1001`, imports
  only the expected Windows/MSVC runtime set, and retains public-archive
  eligibility in CMake. No archive has been generated.
- [x] Official Battle.net D2R 3.3.93847 cold start with the complete active
  stack under the promoted public D2RLoader 1.2.1 baseline.
- [ ] Local-game Level ID 256+ encode/decode and room streaming.
- [ ] Private TCP host/joiner handshake, parity, disconnect, and reconnect.
- [ ] Incompatible-peer and Battle.net fail-closed observations.
- [ ] Travel, Town Portal, waypoint, automap, save/reload, mouse, and controller.
- [x] Duplicate arbitration selects the mod-local 2.0.0 instance and skips the
  older global 1.0.0 instance by plugin identity.
- [x] The exact 1023-record capacity fixture was executed and its failed result
  was preserved without converting it into a compatibility claim.

The two current 2.0.1 builds are local under
`analysis-cache/extended-act-level-ids-v2/build-repro-a/Release/` and
`analysis-cache/extended-act-level-ids-v2/build-repro-b/Release/`; neither is a
release artifact.

## Version 2.0.1 diagnostic gate — 4 September 2026

- Vincent authorized the next gate with `next gate go` after the 2.0.0 capacity
  rejection.
- The first failed `findRowById` comparison now reports the physical
  `rowIndex`, `levelId`, numeric service result, returned and expected revision,
  row index and row size, plus explicit match flags. It records only pointer
  equality as `rowMatch=0/1`; no address is logged.
- The same condition still rejects the complete bank cache immediately and
  retains the original resolver. No hook, codec, service call, table policy,
  save behavior, or compatibility token changed.
- Two independent Release builds are byte-identical: 60,928 bytes, SHA-256
  `A990980E762C8C0278DDCFE80F4BBCE6E5F042C859B9BC24B90B4C107D61F945`.
- Both builds pass CTest `1/1` without compiler warnings. PE metadata reports
  `RuffnecKk / 2.0.1`; the DLL still exports exactly the three D2RLoader entry
  points and imports only the expected Windows/MSVC runtime set.
- The exact 2.0.1 binary and fixture were deployed temporarily after Vincent's
  `GO runtime diagnostic 1023`. Battle.net D2R 3.3.93847, the promoted public
  D2RLoader 1.2.1 baseline, and the complete active stack were retained.
- D2RLoader compiled all 192 TXT tables, loaded 38 plugins, skipped the older
  global duplicate, applied 17 memory patches, and reached startup `24/24`.
- The first extended record produced exactly `rowIndex=256, levelId=256,
  serviceResult=5`; the SDK defines result 5 as `NotFound`. Its returned
  revision, row index and row size remained zero, and `rowMatch=0`. This is a
  clean service lookup refusal, not pointer aliasing or a later metadata
  mismatch.
- The physical `getRow(256)` lookup had already succeeded with revision 1,
  row index 256 and row size 396 before the keyed lookup. Rows 257..1022 were
  not reached because the unchanged fail-closed condition exits on the first
  failure.
- No character, game, save, transition, waypoint, portal, automap or room
  stream was opened. The test process was stopped. A late DLL lock made the
  first combined rollback copy fail after restoring `levels.txt`; the bounded
  retry restored only the remaining DLL without restarting D2R.
- Source and runtime `levels.txt` are restored byte-exact to
  `A46B5438164ADB1FB9540890103594EA48A79AFA2478CB6865D2E6DB5795EB04`.
  Source and runtime DLLs are restored byte-exact to 2.0.0 SHA-256
  `1874623DA1B4914BE465A430D117B19174D986CF9E326E057FFEB115AD10C508`,
  and no D2R/D2RLoader process remains.
- Fresh logs, pre-state copies, deploy receipts and rollback receipts are under
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-capacity-1023-diagnostic-2.0.1/`.

## Version 2.0.0 runtime evidence — 4 September 2026

- Installed runtime: Battle.net D2R `3.3.93847`, Build Key
  `623f7a1f73eabb08ccb2b2046e3f9164`, executable SHA-256
  `E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`.
- Loader: promoted public D2RLoader `1.2.1` (`1.2.1-beta` PE version), SHA-256
  `27A79CCD61360CC03E7C623D20A46546E5732B9997C41DC185B5EFC335B5C084`.
- The mod-local DLL matches the reproducible source build at SHA-256
  `1874623DA1B4914BE465A430D117B19174D986CF9E326E057FFEB115AD10C508`.
- The plugin reports version `2.0.0`, accepts the complete native fingerprint
  and private compatibility channel, then publishes revision 1 with
  `Classic=137`, `LoD=137`, and `RotW=147` rows.
- D2RLoader loads 38 plugins, skips the older global copy of this plugin after
  selecting the mod-local copy, applies 17 memory patches, and reaches startup
  `24/24` with all five eezstreet plugins active.
- Evidence is retained locally under
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-v2-cold-start/`.
- No Level ID above 255 was present in this run. Encoding, decoding, travel,
  Town Portal, waypoint, automap, save/reload, host/joiner, and incompatible
  peer behavior therefore remain `not run`.

## Version 2.0.0 capacity result — 4 September 2026

- The governed TSV helper generated exactly 1023 compiled records with
  contiguous IDs `0..1022`, 188 columns, CRLF, and a byte-exact round trip.
  Fixture SHA-256:
  `6D6756E03911C2BA531F007AE9E97EEC6E81C879800182964AF6BCB3E69C3FAF`.
- The fixture was copied byte-exact to the BKVince runtime. D2RLoader completed
  compilation of all 192 TXT tables, loaded 38 plugins, skipped the one global
  duplicate, applied 17 memory patches, and reached startup `24/24`.
- Extended Act Level IDs 2.0.0 accepted its native fingerprints and private
  compatibility channel. Its RotW `TableView` passed the initial service,
  revision, row-count, pointer, and row-size checks, but a physical row failed
  the subsequent `findRowById` identity round trip. The plugin rejected the
  entire RotW cache and retained the original resolver, as required by its
  fail-closed contract.
- The current log does not identify the first failing row. The SDK contract
  accepts a `uint32_t` ID and documents logical Level-ID lookup, but this run
  proves that the lookup cannot yet be treated as an identity guarantee over
  the full `0..1022` range. Removing that guard or changing the service without
  a new architecture decision would weaken the approved safety contract.
- No character, game session, save, transition, portal, waypoint, or room
  visibility path was exercised with the fixture.
- The test process was stopped and both governed and runtime `levels.txt` were
  restored byte-exact to SHA-256
  `A46B5438164ADB1FB9540890103594EA48A79AFA2478CB6865D2E6DB5795EB04`.
  Evidence is retained under
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-capacity-1023/`.

## Historical v0.1.1 / v1.0.0 evidence

The remaining evidence belongs to the original Act-resolver implementation. It
does not qualify the new v2 codec.

## Proven native contract

- Runtime under test: official D2R `3.3.93847`.
- Governed common corpus: D2R `3.2.92777` / `3.3.93847` byte-exact surfaces.
- Central resolver: `D2R.exe+0x326710`.
- ABI: `uint8 (uint8 dataContext, int32 levelId)`.
- Direct-call census: 113.
- Resolver fingerprint: 48 bytes, one match in the governed image.
- Runtime probe: `Levels` row size `0x18C`, `Act` byte `+0x0D`, Classic and
  LoD 137 rows, BKVince RotW 147 rows.
- BKVince gameplay context observed: `dataContext=3`, matching `Bank::Rotw=3`.
- Evidence log:
  `analysis-cache/extended-act-level-ids-probe/evidence/20260901-1918/ruffneckk-extended-act-level-ids-probe.log`.

## Automated gates

- [x] Policy tests pass (`1/1`) in both clean builds.
- [x] Two independent clean Release builds pass.
- [x] Final DLL hashes match byte-for-byte:
  `16805ACA4207015729516687D1A869226569A64BCCEC0BED318E453CD08E7775`.
- [x] The final DLL is 48,640 bytes and contains no JSON parser, embedded
  configuration, configuration path, or `enabled` branch.
- [x] Required D2RLoader exports and API v3 manifest pass.
- [x] PE metadata, dependencies, author, description, and version pass.
- [x] No build-name or version allowlist is present.
- [x] Mod-local cold start without a configuration file passes on 2 September
  2026 with the complete active stack: 38 plugins, 17 memory patches, and
  startup `24/24`.
- [x] Global cold start without a configuration file passes on 2 September
  2026 with the same complete stack and startup `24/24`.
- [ ] Duplicate global plus mod-local arbitration was proven on 0.1.0 but was
  not rerun for 0.1.1 after runtime control was paused.

The 0.1.1 scope logs are archived locally under
`analysis-cache/extended-act-level-ids-product/evidence/20260902-0851-v0.1.1-configless-matrix/`.
The mod-local run reached complete startup before the already known D2RLoader
TACT assertion occurred afterward; no Extended Act Level IDs refusal or failure
was logged.

## Functional resolver fixture

A temporary CRLF `levels.txt` was generated for 0.1.0 through the governed TSV API by
copying row 146 to `Id=147`, setting `Act=0`, and preserving all 188 columns.
The source and generated table both passed byte-exact round trips.

- [x] D2R compiled the fixture and the RotW cache increased from 147 to 148
  rows while Classic and LoD remained at 137.
- [x] `extended-act-level-ids resolve 147` called the hooked central resolver
  at runtime and returned `Act index 0 (Act 1), data context 3,
  source=Levels.txt`.
- [x] The proof was persisted in the plugin log at
  `2026-09-01 19:47:22.495` and archived under
  `analysis-cache/extended-act-level-ids-product/evidence/20260901-1947-functional-fixture/`.
- [x] The temporary row was removed. Runtime and governed `levels.txt` both
  returned to SHA-256
  `A46B5438164ADB1FB9540890103594EA48A79AFA2478CB6865D2E6DB5795EB04`.

The resolver and cache implementation are unchanged in 0.1.1, but the exact
fixture was not rerun after configuration removal. That current-version runtime
case remains `not run` until Vincent authorizes another launch sequence.

## Playable-area release matrix still required

The central resolver defect is proven fixed, but the temporary row was not a
fully authored playable area with a valid transition. A public release gate
therefore remains open until a real new area after ID 146 is exercised for:

- generation and travel in both directions;
- town/start behavior, portals, automap, waypoint and quest interactions;
- save/reload;
- mouse and controller;
- solo, host, and joiner authority with no desynchronization;
- coexistence with all active RuffnecKk and eezstreet plugins.

No item in this playable-area matrix is claimed as passed.
