# Doll Explosion validation

Version: 0.1.0 incubation

Static qualification: **passed on 2026-09-05**. Runtime qualification: **not
run**.

## Static gates

| Gate | State |
|---|---|
| Promoted D2RLoader / PluginSDK baseline | passed before implementation |
| Governed common D2R native corpus | passed before implementation |
| PD2 Season 13 data chain | passed; formula is data-derived, not proprietary-DLL disassembly |
| Strict TOML parser and pure policy tests | passed; CTest `1/1` on both clean builds |
| Release x64 `/W4 /WX` build | passed with MSVC 19.44 |
| Two-build deterministic DLL hash | passed; both are `237310ACFB3B1495917E562444C8AA719F41DEB50C61F4D212AEF7A8795A7A3C` |
| PE version, manifest and three exports | passed; AMD64 PE32+, version 0.1.0, API 3, exact exports |
| Embedded default TOML | passed; RCDATA 1002 is byte-exact with the source TOML |
| Source contract: standalone, hybrid, no build allowlist | passed by policy test and source audit |
| Native signatures and governed RVA promotion | passed; 32 byte-exact fingerprint surfaces and 20 dedicated governed identities (19 byte sites plus one live-table relation), corpus ready, JSON valid |
| Workspace cartography | passed after regeneration and explicit add-on annotation |

The deterministic DLL is 219,136 bytes. It remains a local build artifact under
`analysis-cache/native-build/DollExplosion/Release/` and is deliberately not
copied into a package or runtime profile.

`RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE` remains false. No ZIP, release registry
entry, Suite promotion, runtime deployment or public-package claim is allowed
from static qualification alone.

## Runtime matrix still required

- Cold start on the official current D2R runtime with the full active Suite and
  all five eezstreet plugins enabled.
- Global and mod-local installations, plus both relevant plugin load orders.
- All seven default Dolls in Normal, Nightmare and Hell; Rift Doll 777 unchanged.
- Delay 0, 1, 25 and upper practical values; frame-count witness at the inner
  and outer radius boundaries.
- Fixed minimum/maximum damage witnesses and source-max-life-percent witnesses.
- Physical resistance and reduction, player count, melee/ranged/spell/DoT/
  thorns/splash deaths, and no double explosion.
- Native Revives, Find Item, Redemption, corpse consumption and a corpse removed
  while a carrier is pending.
- Solo, TCP/IP host and joiner authority, including cross-client visual timing.
- Save & Exit, game destruction, new game and plugin unload with pending carriers.
- Fresh plugin log, loader log, hashes, process state and rollback receipt.

Runtime validation must use the workspace `d2r-runtime-validation` procedure.
It may not disable installed plugins or features to manufacture a successful
cold start. A diagnostic isolation must be followed by a restored full-stack
test before any compatibility conclusion.
