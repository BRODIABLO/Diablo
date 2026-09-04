# Extended Act Level IDs validation

Version 2.0.0 is implemented and builds locally, but it has not been deployed
or exercised in D2R. No v2 runtime or release claim is closed yet.

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
- [ ] Official D2R 3.3.93847 cold start with the complete active stack.
- [ ] Local-game Level ID 256+ encode/decode and room streaming.
- [ ] Private TCP host/joiner handshake, parity, disconnect, and reconnect.
- [ ] Incompatible-peer and Battle.net fail-closed observations.
- [ ] Travel, Town Portal, waypoint, automap, save/reload, mouse, and controller.
- [ ] Duplicate global plus mod-local arbitration.

The two current builds are local under
`analysis-cache/extended-act-level-ids-v2/build-repro-a/Release/` and
`analysis-cache/extended-act-level-ids-v2/build-repro-b/Release/`; neither is a
release artifact.

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
