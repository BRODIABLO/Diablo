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
- [x] Register header-only Base and RotW resources and one plugin-owned `CustomTableServiceV1` table.
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

## Explicitly outside this gate

- Resolver hook installation, monster dispatch, and unique post-install ownership.
- D2R-backed action adapter implementation and native fallback scheduling.
- Global/mod-local deployment, cold start, gameplay, performance, and TCP/IP tests.
- Public ZIP, GitHub release, tag, commit, or push.

The plugin remains default-off and non-gameplay. The next gate may install the managed resolver hook and static category-2 record only after preserving the proven pre/post-callback routing, unique ownership, and complete fail-closed fingerprint; D2R deployment remains a later, separately authorized validation gate.
