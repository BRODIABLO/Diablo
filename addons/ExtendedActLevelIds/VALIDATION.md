# Extended Act Level IDs 0.1.0 validation

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

- [x] Policy and strict JSON tests pass (`1/1`).
- [x] Two independent clean Release builds pass.
- [x] Final DLL hashes match byte-for-byte:
  `5A61181135A18039E3362ECDBD2A8972155C904A6B7B09AABC5611BE49E1FCD4`.
- [x] Required D2RLoader exports and API v4 manifest pass.
- [x] PE metadata, dependencies, author, description, and version pass.
- [x] No build-name or version allowlist is present.
- [x] Mod-local cold start passes with the complete active stack.
- [x] Global cold start passes with the complete active stack.
- [x] Duplicate global plus mod-local installation is arbitrated cleanly:
  the mod-local copy loads and D2RLoader skips the global duplicate.
- [x] Disabled and invalid-config paths are observed without native hook use;
  D2R still completes startup in both cases.

The exact-final matrix is archived locally under
`analysis-cache/extended-act-level-ids-product/evidence/20260901-1949-final-matrix/`.

## Functional resolver fixture

A temporary CRLF `levels.txt` was generated through the governed TSV API by
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
