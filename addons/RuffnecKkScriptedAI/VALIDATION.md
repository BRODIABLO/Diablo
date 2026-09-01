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

## Explicitly outside this gate

- Resolver hook installation, monster dispatch, and unique post-install ownership.
- Behavior-tree ticking and per-think capability handles.
- Native action methods, one-action token, fallback scheduling, and script quarantine.
- Global/mod-local deployment, cold start, gameplay, performance, and TCP/IP tests.
- Public ZIP, GitHub release, tag, commit, or push.

The plugin remains default-off and non-gameplay. The next source gate must prove behavior-tree execution and its one-action/fallback intent contract with mocked capabilities before any native hook or D2R deployment is considered.
