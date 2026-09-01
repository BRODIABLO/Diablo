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

## Explicitly outside this gate

- `aiscript` custom-table registration and compilation.
- Monster-to-script binding.
- Resolver hook installation and unique post-install ownership.
- Lifecycle listeners and authoritative game-thread VM generation.
- Native action methods, one-action token, fallback scheduling, and script quarantine.
- Global/mod-local deployment, cold start, gameplay, performance, and TCP/IP tests.
- Public ZIP, GitHub release, tag, commit, or push.

The plugin must remain default-off and non-gameplay until the next source gate closes the complete bridge transaction and its unit tests.
