# RuffnecKk Scripted AI validation

## Incubation acceptance

- [x] Autonomous RuffnecKk Suite identity and hybrid scope frozen.
- [x] API v3 metadata and server/native-hook role encoded.
- [x] Dedicated English TOML, disabled by default.
- [x] Active-mod, plugin-scope, then global configuration precedence.
- [x] Invalid present configuration fails closed.
- [x] PUC Lua 5.4.9 official URL, SHA-256, static linkage, and MIT notice frozen.
- [x] Dangerous libraries, loaders, bytecode dump, and unsynchronized RNG removed.
- [x] Counted session/per-think allocator and load/think instruction budgets implemented.
- [x] Plain-table behavior-tree node/depth/fanout limits implemented.
- [x] 22 exact native windows and one hook-owner designation compiled.
- [x] Positive and negative fingerprint/ownership policy tests implemented.
- [x] No build-name/version allowlist.
- [x] Incubation source contains no `InstallInlineHook` call.
- [x] Configure and build Release x64 twice; both CTest runs pass 1/1.
- [x] Prove byte-identical 356352-byte DLLs with SHA-256 `E0E0CBD5CE5B1776E65FDD7F15B01FC8C8D38142559D0CAB59ECF4233DCCB6CC`.
- [x] Inspect PE32+ AMD64, API v3 manifest, exactly three exports, and VERSIONINFO.
- [x] Prove the embedded 1052-byte TOML is byte-identical, SHA-256 `8E860BD445B416F6FED05481B2F05F00472EECCA286CE3848DFAA38F5F609BE1`.

## BRIDGE-AI-1 acceptance

- [x] Freeze a standard-layout, trivially-copyable 76-byte `aiscript` row with five SDK columns.
- [x] Register semantically empty Base and RotW resources and one plugin-owned `CustomTableServiceV1` table.
- [x] Copy exact Ready revisions and reject stale, malformed, oversized, duplicate, or out-of-range rows atomically.
- [x] Resolve printable relative `.lua` names beneath a canonical configured root and reject traversal or symlink escape.
- [x] Deduplicate source copies across both banks and cap rows, scripts, per-source bytes, and aggregate bytes.
- [x] Compile all trees into one retained immutable session generation before publication.
- [x] Give each chunk a fresh allowlisted environment so one source cannot mutate another source's globals.
- [x] Stage data without Lua in `DataTablesLoaded`; compile only after `runOnGameThread` proves local authority.
- [x] Drive `GameJoined`, `GameLeft`, and late-callback cancellation by `sessionGeneration` without Lua on the UI thread.
- [x] Prove the remote-client `Unavailable` path creates no generation and no Lua VM.
- [x] Prove full-batch rollback preserves the prior same-session generation and that replaced VMs are reclaimed after readers release them.
- [x] Keep `InstallInlineHook`, resolver dispatch, native AI actions, deployment, and D2R startup absent.
- [x] Configure and build Release x64 twice with `/W4 /WX`; both expanded CTest suites pass 1/1.
- [x] Prove byte-identical 416256-byte DLLs with SHA-256 `55AACEB372991F022F9F2CC0BA50CA6DD80BF0753BC5949D904D63FEDFB7FB00`.
- [x] Inspect PE32+ AMD64, exactly three exports, version `0.2.0`, and only Windows/MSVC runtime imports.
- [x] Reconfirm the embedded 1052-byte TOML is byte-identical, SHA-256 `8E860BD445B416F6FED05481B2F05F00472EECCA286CE3848DFAA38F5F609BE1`.

## EXEC-AI-1 acceptance

- [x] Freeze the V1 `selector` and `sequence` semantics plus an exact declarative kind/field allowlist.
- [x] Provide copied `has_target`, `in_combat`, and target-distance conditions without exposing a pointer, GUID, or stable unit identity.
- [x] Bound idle, wander, attack, chase, retreat, and cast intents and execute them only through typed mock capabilities.
- [x] Stop the complete tree after its first accepted terminal action and reject any second action before it reaches the capability provider.
- [x] Invalidate the Lua userdata and C++ think handle on every `lua_pcall` return, including error and budget paths.
- [x] Converge no action, rejected leaves, explicit fallback, stale session/token, Lua/capability failure, instruction exhaustion, allocation exhaustion, and quarantine to one typed fallback decision.
- [x] Carry the binding's stock `FallbackAi` in every fallback decision without calling a D2R resolver or action helper.
- [x] Enforce both peak and gross per-think allocator growth plus the existing instruction budget.
- [x] Quarantine one shared script after three deterministic errors or three Lua-wall strikes above `2 ms`, excluding mocked capability time.
- [x] Keep `InstallInlineHook`, resolver dispatch, D2R action methods, deployment, and gameplay absent.
- [x] Configure and build Release x64 twice with MSVC `19.44.35228`, Windows SDK `10.0.26100`, `/W4 /WX`, and `/Brepro`; both CTest suites pass `1/1`.
- [x] Prove byte-identical 427008-byte DLLs with SHA-256 `FCB4BE478ACFD0978D85F22EE699E2B94CEDF9D58C27E036231089A9E702611E`.
- [x] Inspect PE32+ AMD64, exactly three exports, API v3 manifest, version `0.3.0`, RuffnecKk metadata, and only Windows/MSVC runtime imports.
- [x] Reconfirm the embedded 1052-byte TOML is byte-identical, SHA-256 `8E860BD445B416F6FED05481B2F05F00472EECCA286CE3848DFAA38F5F609BE1`.

## NATIVE-AI-1 acceptance

- [x] Map idle, wander, attack, chase, retreat, and cast intents to six typed injectable adapter methods.
- [x] Keep raw native modes and pointers outside Lua; validate authority, monster, target, mode, skill, session, token, signed distance, and every scalar bound before the relevant adapter call.
- [x] Prove accepted actions return exclusively to the native action pipeline without an added idle.
- [x] Freeze `FallbackScheduled` as a terminal capability result so the proven rejected-cast internal idle cannot be followed by another selector action or idle.
- [x] Prove rejected actions, explicit fallback, capability error, and invalid target or mode attempt exactly one ten-frame post-callback idle.
- [x] Prove a rejected explicit idle is not recursively attempted a second time.
- [x] Freeze `FallbackAi` as a pre-callback stock-record route for known stale, unavailable, or quarantined bindings; special states and unbound monsters delegate to the original resolver.
- [x] Prove invalid authority, unit, session, token, pointer, or signed target-distance context touches neither Lua nor an action adapter.
- [x] Keep `InstallInlineHook`, resolver dispatch, direct D2R helper calls, deployment, and gameplay absent.
- [x] Configure and build Release x64 twice with MSVC `19.44.35228`, Windows SDK `10.0.26100`, `/W4 /WX`, and `/Brepro`; both expanded CTest suites pass `1/1`.
- [x] Prove byte-identical `427520`-byte DLLs with SHA-256 `9E2A3A6EE1C549890D859C460BA58FDC1E934CCA8E226525570766D42FDCC02F`.
- [x] Inspect PE32+ AMD64, exactly three exports, API v3 manifest, version `0.4.0`, RuffnecKk metadata, and only Windows/MSVC runtime imports; reconfirm the embedded `1052`-byte TOML byte-exact with SHA-256 `8E860BD445B416F6FED05481B2F05F00472EECCA286CE3848DFAA38F5F609BE1`.

## HOOK-AI-1 acceptance

- [x] Implement all six typed actions through the governed D2R ABIs.
- [x] Require a cast skill in one of eight active MonStats slots and use its paired native mode; reject absent skills and modes outside `0..15`.
- [x] Expand the versionless fingerprint to 24 exact, unique windows, including Unit-to-MonStats and monster-skill-mode witnesses.
- [x] Install exactly one managed hook at `0x4A36C0` after vanilla ownership and full-fingerprint checks.
- [x] Re-attest one exclusive `ruffneckk-scripted-ai` inline-hook owner after installation and once per active session.
- [x] Return one process-lifetime static category-2 record only for admitted bindings in the exact active MonStats bank.
- [x] Delegate special states, unbound monsters, unavailable banks/generations, remote clients, and every refusal to the original resolver.
- [x] Disarm routing before lifecycle/resource teardown; hook restoration remains owned by D2RLoader's managed unload transaction.
- [x] Audit the workspace and find no second `0x4A36C0` claimant.
- [x] Configure and build Release x64 twice with MSVC `19.44.35228`, Windows SDK `10.0.26100`, `/W4 /WX`, and `/Brepro`; both CTest suites pass `1/1`.
- [x] Prove byte-identical `442880`-byte DLLs with SHA-256 `020B30A373CE816BC0FB14AC57837C320BA936DCBBC2C391A4CB6F02D40EC1F9`.
- [x] Inspect PE32+ AMD64, exactly three exports, API v3 manifest source, version `0.5.0`, RuffnecKk metadata, and only Windows/MSVC runtime imports; reconfirm the embedded `1052`-byte TOML byte-exact with SHA-256 `8E860BD445B416F6FED05481B2F05F00472EECCA286CE3848DFAA38F5F609BE1`.

## Explicitly outside HOOK-AI-1

- Global/mod-local deployment, cold start, gameplay, performance, and TCP/IP tests.
- Public ZIP, GitHub release, tag, commit, or push.

The plugin remains default-off and not gameplay-qualified. The next gate is `RUN-AI`: deploy it with the complete stack and prove live ownership, routing, fallbacks, performance, lifecycle, scopes, and TCP/IP authority.

## FIX-AISCRIPT-EMPTY-DEFAULT acceptance

- [x] Isolate the startup assertion to D2RLoader `1.2.0-beta` rejecting a virtual custom table whose resource contains only its header; Shared, Server, Server+Base, and one enabled exact-schema row all compile successfully.
- [x] Prove one exact-schema disabled row (`0`, empty script, `0`, `0`, `0`) compiles in both Base and RotW while producing zero bindings, zero unique scripts, and zero Lua VMs.
- [x] Ship that row only as the virtual-resource default; physical mod overrides may remain header-only and the row is filtered before binding validation or source loading.
- [x] Configure and build Release x64 twice with MSVC `19.44.35228`, Windows SDK `10.0.26100`, `/W4 /WX`, and `/Brepro`; both CTest suites pass `1/1`.
- [x] Prove byte-identical `442880`-byte DLLs with SHA-256 `335903E976A24DD1D7BC0BA344FC1A2F935D7B53E6F098AC95555BBB11EC7C78`.
- [x] Inspect PE32+ AMD64, exactly three D2RLoader exports, version `0.5.1`, RuffnecKk metadata, Windows/MSVC-only imports, and the byte-exact embedded `1052`-byte TOML with SHA-256 `8E860BD445B416F6FED05481B2F05F00472EECCA286CE3848DFAA38F5F609BE1`.
- [x] Cold-start the complete active mod-local stack default-off: `38` plugins, all five eezstreet plugins, `17` memory patches, and `24/24` startup complete.
- [x] Cold-start the same stack enabled with no physical `AIScript` override: Base and RotW each compile `1 x 76` bytes, staging reports `0 unique scripts`, no Lua VM is created, and startup reaches `24/24` without an Excel assertion.
- [x] Stop D2R and restore the shipped default-off TOML byte-exact; leave the validated `0.5.1` DLL deployed mod-locally for the next gate.

Gameplay, first-think ownership, a real monster or hireling binding, failure fallback, performance, lifecycle, global scope, and TCP/IP remain unqualified. No ZIP, release, commit, or push belongs to this fix gate.

## RUN-AI-ENTITY diagnostic — blocked binding source

- [x] Add diagnostics-only, once-per-session positive evidence for exclusive resolver ownership and the first routed scripted think; no per-tick logging is introduced and diagnostics remain disabled by default.
- [x] Build candidate `0.5.2` twice with MSVC `19.44.35228`, Windows SDK `10.0.26100`, `/W4 /WX`, and `/Brepro`; both CTest suites pass `1/1`.
- [x] Prove byte-identical `444416`-byte DLLs with SHA-256 `A07B0140FEEEFE12976524D29567A395CFC3713A8FBCFA8E39170A5C9F2694C8` and version `0.5.2` metadata.
- [x] Prepare reversible Base and RotW fixtures for Fallen `19` (`FallbackAi=6`) and hireling classes `271`, `338`, `359`, `561`, and `562` (`FallbackAi=61`) sharing one bounded retreat/chase/wander tree.
- [ ] Load the physical mod TXT overrides: **failed** on D2RLoader `1.2.0-beta`. Both banks report `Custom table failed to load` after `Refused to write a mod Excel TXT into the base D2RLoader BIN cache`; the gameplay-table fingerprint is consequently incomplete at `182/184`.
- [x] Keep the complete stack active during that diagnostic (`38` plugins, all five eezstreet plugins, `17` patches, `24/24` startup); do not treat the incomplete custom-table fingerprint as compatibility evidence.
- [x] Stop D2R, remove both temporary TXT files and the Lua witness, and restore the mod-local `0.5.1` DLL plus default-off TOML byte-exact.

No entity entered Lua. The architecture decision now required is either to wait for a loader correction, embed a test-only table (rejected as a product workflow), or let Scripted AI read bounded external table text from its support directory and register those bytes as the virtual resource before `CustomTableServiceV1` compiles them. The third option preserves editable bindings without rebuilding the DLL and is recommended, but is not implemented without Vincent's direction.

## REVIVE-DOMAIN-AI-1 acceptance

- [x] Freeze optional inter-DLL ABI V1 with explicit size, version, magic, capability bit, result enum and x64 layout assertions.
- [x] Keep Revive Overhaul as the unique `AiFunction03` hook owner and Scripted AI as the unique resolver/Lua/action owner; add no D2RLoader or PluginSDK service.
- [x] Stage the configured Revive script directly from the canonical script root without a physical or virtual `AIScript` binding and without any Diablo TXT/TSV edit.
- [x] Add copied `has_owner` and owner-distance conditions plus a typed `follow_owner` action; expose no owner pointer, GUID or stable identity to Lua.
- [x] Revalidate authority, monster, owner, session, token, signed distances, native bank and action mode before the native chase helper can receive the owner.
- [x] Prove a peaceful distant Revive schedules exactly one follow action, while combat, an existing target, a nearby owner, invalid context, explicit fallback and every non-action preserve native continuation with no synthetic idle.
- [x] Prove an action-owned native fallback is terminal and cannot be followed by the original Revive callback.
- [x] Reject unsafe domain paths and unknown domain configuration keys; ship the domain disabled by default.
- [x] Build candidate `0.6.0` twice with MSVC `19.44.35228`, Windows SDK `10.0.26100`, `/W4 /WX`, and `/Brepro`; both CTest suites pass `1/1`.
- [x] Prove byte-identical `461312`-byte DLLs with SHA-256 `27CA86A1972D887074B65398A182EED3E455D6D2D98EA0363EBA178E85370752` and version `0.6.0` metadata.
- [x] Inspect PE32+ AMD64, the three required D2RLoader exports plus `RuffnecKkScriptedAIQueryRevivePolicyV1`, API v3 manifest source and Windows/MSVC-only imports.
- [x] Record the shipped TOML as `1424` bytes / SHA-256 `80AA01495235B9F421D6D28509A1C1E047D37270E7BDE6160D9C15E9215E6855` and the sample policy as `671` bytes / SHA-256 `4CC4CC4ACC591BA2CD69E8D495ED04DF3054F86DB877E05436AAD020037DDEFC`.
- [ ] Deploy and qualify the complete active stack, both relevant load orders, provider absence, mod-local/global scopes, lifecycle, performance and TCP/IP authority on D2R `3.3.93847` after separate live-test authorization.

No live runtime, gameplay, ZIP, release, commit or push is claimed by this gate.

## REVIVE-POLICY-ONLY-0.6.1 correction

- [x] Replace private ABI V1 with ABI V2: export
  `RuffnecKkScriptedAIQueryRevivePolicyV2`, version `2`, magic
  `0x3256495645524941` and `CapabilityRequestNativeFollow`.
- [x] Remove `TryFollowOwner` from the D2 native-action adapter. The Revive
  behavior-tree leaf now terminates in a policy-only capability and cannot call
  chase, pathfinding or movement from Scripted AI.
- [x] Return `RequestNativeFollow` only for one accepted peaceful far-owner
  policy request; combat, target, nearby, invalid and fallback cases delegate.
- [x] Build Release x64 twice with MSVC `19.44.35228`, Windows SDK
  `10.0.26100`, `/W4 /WX`, and `/Brepro`; both CTest suites pass `1/1`.
- [x] Prove byte-identical `461312`-byte DLLs with SHA-256
  `D520EF88DB7BB314AF99BB7610A21E3BE68A3ADD2709CEEC7CC479547A62E6F7`
  and version `0.6.1` metadata.
- [x] Inspect PE32+ AMD64 and exactly the three required D2RLoader exports plus
  `RuffnecKkScriptedAIQueryRevivePolicyV2`; the retired V1 export is absent.
- [x] Record the unchanged shipped TOML as `1424` bytes / SHA-256
  `80AA01495235B9F421D6D28509A1C1E047D37270E7BDE6160D9C15E9215E6855`
  and the corrected sample policy as `670` bytes / SHA-256
  `F9E5008731603519A58E01A779F10B9F4D4A4A7AD7CB1B22F7D91DD9A68469DB`.
- [ ] Deploy and qualify the complete BKVince stack, obstacle recovery,
  repeated town/combat transitions, lifecycle and TCP/IP authority after a
  separate live-test authorization.

No deployment, live runtime, ZIP, release, commit or push is claimed by this
correction gate.

## REVIVE-TACTICAL-DIRECTOR-0.7.0 candidate

- [x] Add private Revive ABI V3 while retaining V2 compatibility for an older
  Revive Overhaul consumer during rolling load-order transitions.
- [x] Consume only copied tactical facts in Lua: profile, bounded distances,
  last action and preferred native skill. Native target identity remains
  inaccessible to scripts.
- [x] Add `is_melee`, `is_ranged`, `is_caster`, target-owner distance,
  last-action and preferred-skill conditions plus `cast_preferred`.
- [x] Implement one session-scoped native target blackboard with bounded size,
  GUID re-resolution, same-address and dead-unit checks, hard owner leash and
  lifecycle clearing.
- [x] Classify compiled MonStats automatically: valid cast-mode skills select
  caster, otherwise the governed `isMelee` flag selects melee, with ranged
  physical as the remaining profile.
- [x] Route handled decisions through the existing governed native adapter and
  never issue owner movement or synthetic idle from the tactical executor.
- [x] Expand the versionless fingerprint from 24 to 29 exact unique windows,
  including `GetUnitId`, `IsDead`, `GetServerUnit`, distance and the compiled
  MonStats flags witness.
- [x] Add unit tests for profile selection, ABI layout/compatibility, the melee,
  ranged and caster trees, spell rotation and all native-delegation paths.
- [x] Prove two independent Release x64 builds with MSVC `19.44.35228`, Windows
  SDK `10.0.26100`, `/W4 /WX` and `/Brepro`; both CTest suites pass `1/1`, and
  both `476160`-byte DLLs are byte-identical with SHA-256
  `F633EFACEABFB0DB1BAA96CF31D71A37F7EFA343B8566155F259F2E0239E7F39`.
- [x] Inspect PE32+ AMD64, the three D2RLoader exports plus retained ABI V2 and
  new `RuffnecKkScriptedAIQueryReviveTacticsV3`, with version `0.7.0` metadata.
- [x] Record the product Lua tree as `4484` bytes / SHA-256
  `4B339DE51F19E995212B783E4B15F975BA5A3EDA934F45ACB7CD923F7697430D`
  and the equivalent laboratory tree as `4043` bytes / SHA-256
  `72A39C2262B97A7F1BE3B6089663AA6A5A54147867B5D125F37931497020325C`.
- [x] Record the shipped default-off TOML as `1425` bytes / SHA-256
  `181056C7344751736DEB54C1F767DDEBE4538EEC3AE2621B4DBD0DF663B35D81`.
- [ ] Deploy and run the full-stack BKVince gameplay matrix only after a
  separate runtime-test GO.

No gameplay qualification, ZIP, release, commit or push is claimed here.
