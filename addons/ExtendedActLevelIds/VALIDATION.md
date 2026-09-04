# Extended Act Level IDs validation

Version 2.1.1 retains the qualified two-pass cache and server Town Portal
sidecar, removes both false 0x60 equality checks, and adds the approved bounded
client GUID/session sidecar plus the exact portal-label relay at `0xC188E`.
Its build, policy, signature, ABI, metadata, and reproducibility gates pass.
No 2.1.1 runtime has been authorized or executed; the failed 2.1.0 artifact
must not be redeployed. Town Portal return travel, waypoints, automap UX,
persistence, multiplayer, and release gates remain open.

## Version 2.1.1 client Town Portal sidecar implementation — 4 September 2026

- Vincent authorized `GO implantation sidecar client Town Portal 0x51/0x60 +
  UI 0xC188E local/offline`. This gate changed only plugin source, policy
  tests, mission, and documentation. It did not deploy a DLL, launch D2R,
  alter a table or save, package an archive, commit, or push.
- Packet 0x60 byte `+2` remains the portal destination low byte. Argument R8B
  and packet byte `+0x0B` remain the owner player's current-room low byte and
  are passed through unchanged. Version 2.1.1 removes the incorrect send-side
  comparison with the endpoint destination and the incorrect receive-side
  comparison `packet[11] == packet[2]`.
- A separate immutable client map stores only `{session generation, portal
  GUID, full destination Level ID, native low byte}`. Copy-on-write publication
  gives lock-free reads, the hard bound is 1024 entries, every class-59 0x51
  spawn evicts its GUID before stock processing, and session reset clears the
  complete map. No client `Unit*` is retained.
- Marked 0x51 and 0x60 packets decode and validate a known extended Level ID,
  restore only X in their fixed-size packet copy, and call the stock handler
  before publication. The 0x60 path then resolves the live type-2 object via
  `CLIENT_GetUnitByIdAndType 0x9A5D0` and requires class 59, GUID, and native
  destination-low agreement before refreshing the client descriptor.
  Allocation failure, overflow, malformed marked data, or live mismatch
  poisons portal handling for the session.
- The shared `DATATBLS_GetLevelsTxtRecord 0x32C4A0` entry remains byte-exact.
  A 17-byte near relay at the exact client callsite `0xC188E` executes
  `mov r8,rsi`, preserving the live `Unit*`, then dispatches to the
  portal-aware resolver. A valid descriptor and data context use the full ID;
  a poisoned/mismatched descriptor or class-59 low byte zero without a
  descriptor returns null to the stock string fallback. Vanilla portals and
  unrelated objects use the original low-ID lookup.
- The strict 33-byte client-unit resolver fingerprint, strict 27-byte UI
  context witness, and exact five-byte UI call are each unique in the governed
  common 92777/93847 image. Their governed RVAs are `0x9A5D0`, `0xC187F`, and
  `0xC188E`; runtime load refuses any mismatch.
- Policy tests cover full-ID recovery at 256 and 1022, vanilla fallback,
  missing-low-zero refusal, low-byte mismatch, stale session rejection, GUID
  reuse/eviction, a full 1024-entry map, overflow refusal, session pruning, and
  unrelated-object passthrough. Source-contract tests also forbid both former
  0x60 equality checks and require the UI relay/fingerprints.
- Two independent MSVC 19.44 / Windows SDK 10.0.26100.0 Release builds pass
  `CTest 1/1` without compiler warnings and are byte-identical: 88,576 bytes,
  SHA-256
  `DC1DEFC82D8B62F3F33859D2D57B621F8802CFEFAE14644D5B5884179E5887F6`.
  PE metadata reports `RuffnecKk / 2.1.1`; the DLL exports exactly the three
  D2RLoader entry points and imports only the expected Windows/MSVC runtime.
- The two local candidates are under
  `analysis-cache/extended-act-level-ids-v2/town-portal-client-final-a/Release/`
  and
  `analysis-cache/extended-act-level-ids-v2/town-portal-client-final-b/Release/`.
  Neither candidate is deployed, packaged, committed, or pushed.
- Verdict: **STATIC PASS**. The next separate gate is a controlled 2.1.1
  local/offline runtime with the complete active stack and disposable
  character: create in Level 256, traverse to Harrogath, inspect the portal
  label without assertion, return to Level 256, clean up the pair, and capture
  the new client publication/full-lookup/refusal counters. Save/reload and all
  network roles remain later gates.

## Version 2.1.0 Town Portal local/offline implementation — 4 September 2026

- Vincent authorized `GO implantation Town Portal 1023 local/offline` after the
  read-only portal census selected a process-local GUID-pair/session sidecar.
- The implementation scopes only dynamic object class `59`. A copy-on-write
  endpoint map records the session generation, `Game*`, GUID, reciprocal GUID,
  full destination Level ID, and native low byte for both portal halves. Every
  use revalidates the current session, game, class, low byte, and reciprocal
  pair before recovering a full ID.
- `D2GAME_CreateLinkPortal 0x435DD0` is hooked with its verified five-argument
  x64 ABI. `DUNGEON_GetLevelIdFromRoom 0x2EFC10` returns the low byte only when
  called from the exact guarded return address `0x43605F` under that TLS
  creation scope. This satisfies the stock byte store while retaining the full
  source ID in the sidecar; no assertion is NOPed or globally suppressed. An
  extended creation additionally requires the `LocalPlayerReady` owner and a
  healthy session contract. Any precondition or sidecar-publication failure is
  refused before an unscoped native call can reach the stock byte guard and
  poisons further class-59 portal traffic for that session.
- `SUNIT_GetPortalOwner 0x490070` scopes the full destination through the
  existing act resolver and accepts the result only when the live owner GUID is
  the stored reciprocal endpoint. `OBJECTS_OperateFunction15_Portal 0x58F680`
  then exposes the full ID to its two record lookups only after owner
  validation and only for the `LocalPlayerReady` unit.
- The first compiled design hooked shared record entries `0x32C4A0` and
  `0x32C200`. A coexistence audit found that MapSense fingerprints and calls
  `0x32C200` directly, so that design was replaced before delivery. Version
  2.1.0 leaves both shared entries untouched and uses two near relays for only
  the unique portal-operation calls `0x58F819` and `0x58F8EE`.
- Packet `0x51` and `0x60` remain 14 and 12 bytes. Their server hooks encode
  high Level-ID bits in the marked X word only for the local client; their
  client hooks validate a known extended record and restore X in a stack copy
  before calling the stock handler. Remote operations and remote portal
  packets fail closed. TCP host/joiner and Battle.net portal support are not
  claimed.
- The plugin remains configuration-free and hybrid global/mod-local. It adds
  no save hook, D2S/D2I field, waypoint bit, portal-flag field, packet byte, or
  dependency on another plugin. The session sidecar is cleared on generation
  change and game exit.
- All thirteen hook signatures, both redirected callsites, and the additional
  ABI/layout witnesses have one exact match each in the governed common
  92777/93847 corpus. The eezstreet PluginPack pin remains clean and no other
  RuffnecKk add-on owns the portal seams. The shared MapSense lookup entries
  remain byte-exact.
- Two independent MSVC 19.44 / Windows SDK 10.0.26100.0 Release builds pass
  `CTest 1/1` without compiler warnings and are byte-identical: 78,336 bytes,
  SHA-256
  `1803A73E0894C2A8916DD5BD32793525E4F795B13652CFC488786247AB6045B6`.
  PE metadata reports `RuffnecKk / 2.1.0`; exports remain exactly
  `D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin`, and
  `D2RLoaderUnloadPlugin`, with the expected Windows/MSVC dependencies.
- The two local candidates are under
  `analysis-cache/extended-act-level-ids-v2/town-portal-build-a/Release/` and
  `analysis-cache/extended-act-level-ids-v2/town-portal-build-b/Release/`.
  Neither was deployed, packaged, copied into BKVince, committed, or pushed.
- Next gate: deploy the exact 2.1.0 candidate and the proven Level 256 fixture
  under the complete active stack, then validate creation, Level 256 →
  Harrogath, Harrogath → Level 256, cleanup, and fresh portal counters. Save
  reload and all network roles remain separate later gates.

## Version 2.1.0 Town Portal local/offline runtime — 4 September 2026

- Vincent authorized the exact runtime sequence after correcting `123` to the
  1023-record fixture. The tested mod-local DLL is the reproducible 78,336-byte
  candidate SHA-256
  `1803A73E0894C2A8916DD5BD32793525E4F795B13652CFC488786247AB6045B6`.
- Battle.net D2R `3.3.93847` and promoted public D2RLoader `1.2.1` retained the
  complete active stack. D2RLoader compiled 192 TXT tables, loaded 38 plugins
  including all five eezstreet DLLs, applied 17 memory patches, and reached
  startup `24/24`. Extended Act Level IDs accepted its native fingerprints and
  published `Classic=137`, `LoD=137`, and `RotW=1023`.
- Portal creation in Level 256 and the first traversal to Harrogath passed.
  Before Vincent attempted the return traversal, D2R reported
  `BC_ASSERT: eLevelId > 0 && eLevelId < DataTablesGetNumLevels(ver)` at
  `D2Common/src/DataTbls/LvlTbls.cpp:284`. Return traversal, pair cleanup, and
  final status counters were not run.
- The native stack returns from `DATATBLS_GetLevelsTxtRecord 0x32C4A0` at
  `0xC1893`. Its exact client callsite `0xC188E` reads the portal's byte-sized
  `ObjectData.InteractType` and zero-extends it as the Levels ID. A destination
  of 256 therefore reaches the stock record lookup as zero.
- Immediately before the assertion, the plugin logs
  `extended Town Portal state packet refused outside the local codec
  contract`. Native caller witness `0x5388CA` passes the owner player's current
  room Level ID low byte in R8B, while packet byte `+2` comes independently
  from the portal destination `InteractType`. Version 2.1.0 incorrectly
  requires this argument to equal the endpoint destination low byte on send
  and requires packet bytes `+0x0B` and `+2` to be equal on receive.
- Both marked packet handlers decode a full ID and restore X only in a native
  packet copy. Neither publishes the decoded client portal identity. The
  client UI callsite `0xC188E` consequently remains outside the server sidecar
  scopes and consumes the truncated byte. A global hook of shared lookup
  `0x32C4A0` remains prohibited because MapSense and 43 other direct callsites
  must retain the stock entry.
- The user selected Exit rather than Continue. The game was stopped, all nine
  QtyTester files were restored exactly to their pre-gate hashes, and the
  source/runtime tables and mod-local DLL were restored byte-for-byte. The
  global duplicate remained untouched and no Diablo process remains.
- Fresh logs, deploy/rollback receipts, before/failure save snapshots, hashes,
  and the result summary are retained under
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-town-portal-1023-local-offline-2.1.0/`.
  `historical-crash-not-current/` contains only two older relabelled reports;
  the handled current assertion produced no fresh crash report.
- Verdict: **FAIL**. Do not redeploy 2.1.0. The next permitted gate is a
  read-only census for a client portal sidecar populated by packet `0x51/0x60`
  and for every portal consumer, starting with UI callsite `0xC188E`.

## Read-only client Town Portal sidecar census — 4 September 2026

- Verdict: **STATIC PASS**. The governed 92777/93847 corpus proves that
  `0xC188E` is the only client `DATATBLS_GetLevelsTxtRecord` call fed directly
  by `ObjectData.InteractType`. The other two client readers among the 28
  direct `UNITS_GetObjectInteractType` xrefs serve object-lock and shrine
  behavior, not Levels.
- The UI helper at `0xC17E0` receives `Unit*` in RDX and retains it in RSI at
  `0xC188E`. Its 27-byte context at `0xC187F` has one corpus match. A near
  relay can therefore pass `R8=RSI` to a portal-aware resolver and leave the
  shared `0x32C4A0` entry and its other 43 callsites byte-exact. Returning null
  is a proven fail-closed path because the stock helper already falls back to
  string ID `0x150D`.
- Handler `0x51` carries GUID `+2`, class `+6`, X `+8`, and destination low
  byte `+0x0D`. Handler `0x60` carries GUID `+3`, destination low byte `+2`,
  owner coordinates `+7/+9`, and owner-room low byte `+0x0B`. The latter is
  semantically independent and must be passed through unchanged.
- The selected design adds a distinct immutable client map keyed by the
  current session generation and portal GUID, storing full destination ID and
  its native low byte. Every class-59 `0x51` first evicts the same GUID;
  marked `0x51/0x60` packets delegate their X-restored copy to stock before
  publishing or refreshing. `0x60` publication additionally re-resolves the
  live object through `CLIENT_GetUnitByIdAndType 0x9A5D0` and requires unit
  type 2, class 59, GUID, and low-byte agreement. No `Unit*` is retained.
- The client map must be copy-on-write, lock-free for reads, capped at 1024
  entries, and cleared on `GameJoined` and `GameLeft`. Allocation failure,
  overflow, malformed marked data, or a live mismatch poisons portal handling
  for the session. At the UI callsite, a matching entry uses the full ID after
  context validation; a poisoned/mismatched entry or `low=0` without an entry
  returns null; untouched vanilla and unrelated objects use the original low
  ID lookup.
- The write census finds no extra client hook requirement. The generic portal
  initializer write at `0x1CB5D3` belongs to object construction and precedes
  the explicit packet write at `0x99582`; the state write is already owned at
  `0x1CB1E9`. GUID reuse is covered by `0x51` eviction and session turnover,
  so a global client-unit destruction hook is unnecessary.
- No plugin source, binary, runtime profile, table, save, process, package,
  ROADMAP entry, commit, or push changed in this gate. Version 2.1.0 remains a
  failed artifact that must not be redeployed.
- Next gate: implement the client map, remove both false `0x60` equality
  checks, add the exact `0xC188E` relay and policy tests, then produce two
  clean reproducible builds. Runtime remains a separate authorization.

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
  `analysis-cache/extended-act-level-ids-v2/build-two-pass-b/Release/`. The
  byte-identical build-a DLL was deployed only for the runtime gate below;
  neither candidate has been packaged or promoted.
- The next gate required a complete-stack cold start with the exact 1023-record
  fixture and RotW publication of all 1023 rows. It is closed below.

## Version 2.0.2 exact 1023-record runtime — 4 September 2026

- Vincent authorized the bounded sequence with
  `GO runtime 1023 cache deux passes`.
- The exact 2.0.2 DLL
  `FDAF884B69AB879D8F3946E2CF036FF25AF3C03C7CBE479803761A19B13A6EC1`
  and exact fixture
  `6D6756E03911C2BA531F007AE9E97EEC6E81C879800182964AF6BCB3E69C3FAF`
  were deployed temporarily to the BKVince mod-local profile.
- The tested runtime was Battle.net D2R `3.3.93847`, Build Key
  `623f7a1f73eabb08ccb2b2046e3f9164`, under the promoted public D2RLoader
  `1.2.1` baseline with `-mod BKVince -txt`.
- The complete installed stack compiled 192 TXT tables, loaded 38 plugins,
  retained all five eezstreet DLLs, skipped the older global duplicate, applied
  17 memory patches, and reached startup `24/24` with no fresh error.
- Extended Act Level IDs 2.0.2 accepted its native fingerprints and private
  compatibility channel, then published `Levels revision 1` with
  `Classic=137`, `LoD=137`, and `RotW=1023`.
- This directly proves that physical `getRow` covers every extended record and
  that the retained keyed identity pass remains valid for `0..255`. It does not
  prove playable-area, travel, save, or network behavior.
- No character or save was opened. The single test process was stopped; source
  and runtime were restored byte-exact to the normal table
  `A46B5438…795EB04` and mod-local 2.0.0 DLL `1874623D…10C508`. The global
  1.0.0 duplicate remained untouched and no D2R/D2RLoader process remains.
- Fresh logs, exact inputs, pre-state copies, deployment receipts, rollback
  receipts, and the result summary are under
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-capacity-1023-two-pass-2.0.2/`.
- The next gate is a disposable-character test in a real playable Level ID
  above 255, covering resolver output, room streaming, return travel, Town
  Portal, waypoint, and automap. Save/reload and host/joiner remain separate.

## Version 2.0.2 same-act Level 256 gameplay gate — 4 September 2026

- Vincent authorized this bounded gate with `go next gate` and reported after
  entering `To Rift Level 7` from Harrogath that the character appeared
  normally inside the level.
- The governed `levels.txt` fixture contains numeric IDs `0..1022` plus the
  historical non-record `Expansion` separator. It preserves CRLF and passes a
  byte-exact TSV round-trip. Its SHA-256 is
  `5C9B604E0D9A595D8DDB24699D91AEF0C564E7956BE7DBAA7CC5B8B92928B2DF`.
  The only change from the preceding fixture is `Id=256 / Act: 0 → 4`.
- Harrogath is `Id=109 / Act=4 / Vis0=256`; the target is
  `Id=256 / Act=4 / Vis0=109 / DrlgType=2`. The matching preset remains
  `LevelId=256 / Def=1095 / File1=Expansion/baalLair/wthrone.ds1` in the
  `lvlprest.txt` fixture, SHA-256
  `4EF8B404DB5E54C0C676E7C57D290CF142ED5C8B158D9A11056FB7975869EC39`.
- The exact candidate DLL was 2.0.2, 61,440 bytes, SHA-256
  `FDAF884B69AB879D8F3946E2CF036FF25AF3C03C7CBE479803761A19B13A6EC1`.
  The complete stack compiled 192 TXT tables, loaded 38 plugins including all
  five eezstreet DLLs, applied 17 patches, reached `24/24`, and published
  `Classic=137 / LoD=137 / RotW=1023`.
- After entry, the fresh MapSense log records `current-level=256`, `act=4`,
  `room-witness=35`, and `external labels: PASS`. The level rendered normally
  and no new D2RLoader crash report was created. This closes local same-act
  ingress and provides a runtime room-graph witness above 255; it does not by
  itself close explicit codec counters, out-of-sight handling, return travel,
  Town Portal, waypoint, visual automap, persistence, or multiplayer.
- The controlled session changed the normal `QtyTester.d2s` and
  `QtyTester.d2rl` session/location state. No persistence-format claim is made.
  After evidence capture, the only process was stopped and all nine QtyTester
  files were restored byte-exact to their pre-gate SHA-256 values.
- Source and runtime `levels.txt`, `lvlprest.txt`, and the mod-local DLL were
  restored byte-exact to their normal hashes; no D2R/D2RLoader process remains.
  Evidence is under
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-gameplay-level-256-same-act-2.0.2/`.
- The recommended next gate is return travel from Level 256 to Harrogath,
  followed by Town Portal in both directions. Waypoint, save/reload,
  controller, and host/joiner remain separate gates.

## Version 2.0.2 Level 256 return and Town Portal gate — 4 September 2026

- Vincent authorized this bounded run with `Go` after the preceding gate
  recommended Level 256 → Harrogath followed by Town Portal in both directions.
- The exact 2.0.2 candidate and same-act fixtures were deployed only to the
  BKVince mod-local profile after capturing the normal tables, DLL, logs, and
  all nine `QtyTester` files.
- The complete stack compiled 192 TXT tables, loaded 38 plugins including all
  five eezstreet DLLs, applied 17 memory patches, reached startup `24/24`, and
  published `Classic=137 / LoD=137 / RotW=1023`.
- Fresh MapSense observations close physical travel: `109 → 256 → 109 → 256 →
  109 → 256`, all in Act 4. Every Level 256 observation reports
  `room-witness=35` and `external labels: PASS`.
- Casting Town Portal from the final Level 256 visit produced
  `BC_ASSERT: eLevelIdLocal <= 255` at
  `D2Game\src\Skills\Skills.cpp:4120`. The captured stack returns through
  `0x436075`, `0x432F27`, `0x46FD81`, `0x581965`, `0x4F52CB`, `0x4C144C`, and
  `0x4F30BD`. The assertion was not continued, so portal entry and return are
  `NOT RUN` rather than failed gameplay claims.
- Governed native analysis identifies `D2GAME_CreateLinkPortal 0x435DD0` and
  its unique guard/store witness at `0x436061`. It obtains the full source-room
  Level ID, compares it to `0xFF`, then truncates through `movzx edx,dil`
  before `UNITS_SetObjectInteractType 0x34E9D0`. The setter writes one byte to
  `ObjectData+0x08`; removing only the assertion would encode Level 256 as
  zero.
- The same byte is consumed by `SUNIT_GetPortalOwner 0x490070`, emitted at
  packet-`0x60` offset `+2` by the unique body at `0x47F650`, and read back by
  the client consumer at `0x1CB1C0`. Current v2 hooks cover only room packets
  `0x07/0x08`, so this is a distinct portal-state contract.
- This result does not implicate the D2S/D2I format. The plugin still installs
  no save hook and changes no serialized layout. The run changed normal
  location/session files only; all nine installed `QtyTester` files were
  restored and verified `9/9` against the pre-gate hashes.
- The process was stopped without continuing the assertion. The first rollback
  restored both tables and encountered a transient DLL lock; a bounded retry
  restored only the DLL. Source/runtime now match the normal `levels.txt`,
  `lvlprest.txt`, and 2.0.0 DLL hashes, and no process remains.
- Evidence, screenshot, fresh logs, pre-state copies, save copies, sync
  receipts, rollback receipts, native analysis, and the result summary are
  under
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-gameplay-level-256-return-portal-2.0.2/`.
- Verdict: physical return **PASS**; Town Portal support above 255 **BLOCKED**;
  no package or release is permitted.
- The next safe gate is a read-only native census of all portal creation,
  destruction, interaction, lifecycle/GUID, initial packet `0x51`, state
  packet `0x60`, host/joiner, and incompatible/Battle.net paths before choosing
  a session-sidecar contract. No new runtime attempt should use Town Portal
  above 255 with 2.0.2.

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
- [x] Local-game same-act entrance to Level ID 256 renders normally and
  produces a Level 256 / Act 4 room-graph witness.
- [x] Physical same-act return travel Level 256 → Harrogath and repeated
  bidirectional transitions.
- [ ] Explicit room-codec counters and out-of-sight handling.
- [ ] Private TCP host/joiner handshake, parity, disconnect, and reconnect.
- [ ] Incompatible-peer and Battle.net fail-closed observations.
- [ ] Town Portal (known native byte blocker), waypoint, automap, save/reload,
  mouse, and controller.
- [x] Duplicate arbitration selects the mod-local 2.0.0 instance and skips the
  older global 1.0.0 instance by plugin identity.
- [x] The exact 1023-record capacity fixture was executed and its failed result
  was preserved without converting it into a compatibility claim.
- [x] Version 2.0.2 publishes the exact 1023-record RotW cache under the full
  active stack; the remaining Level ID 256+ matrix stays explicitly open.

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

The later Level 256 fixture is a playable area with a same-act transition to
Harrogath. The matrix now records:

- [x] generation, rendering, room-graph witness, and repeated physical travel
  in both directions;
- [x] coexistence during those runs with the complete active RuffnecKk stack
  and all five eezstreet plugins;
- [ ] Town Portal: 2.1.0 passes creation and the first Level 256 → Harrogath
  traversal, then fails before return because its `0x60` invariant is wrong
  and no client GUID/session sidecar serves the UI lookup at `0xC188E`;
- [ ] waypoint, visual automap, and quest interactions;
- [ ] save/reload;
- [ ] mouse and controller coverage;
- [ ] solo-network, host, and joiner authority with no desynchronization;
- [ ] explicit room-visibility codec counters, out-of-sight behavior, and
  incompatible/Battle.net fail-closed cases.

Packaging and release remain blocked until every required row is closed. A
corrected implementation, preceded by the read-only client-side census, must
precede any further Town Portal runtime attempt above Level ID 255.
