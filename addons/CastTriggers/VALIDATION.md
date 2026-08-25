# Cast Triggers validation

Status as of **24 August 2026**: **release candidate, not publishable yet**.
Static, build, data-compilation and both installation-scope gates are proven.
Gameplay is blocked by an independent D2Prism presentation failure, and the
current complete Suite gate is blocked by a newly deployed Resistance Floor
failure that occurs before Cast Triggers loads.

## Static gates

| Gate | Status | Current proof |
|---|---|---|
| Policy and strict TOML tests | passed | Release CTest `cast-triggers-policy`, 1/1 passed |
| Release x64 build | passed | MSVC Release against PluginSDK `4933e2c42cb2592958cd0df3b6dc5003102252d1` |
| Metadata and scope | passed | `0.1.0`, `RuffnecKk`, `Server + NativeHooks`, no `ModScopedOnly` |
| D2R 3.3.93847 signatures | passed | Three owned hook entries and four direct helpers matched before hook installation |
| Native source filtering | passed | Unit tests cover manual-player, cast animation, sequence-to-cast and repeat exclusions |
| Intermod table integrity | passed | Vanilla 3.3 source round-trips byte-exact; generated fixture remains CRLF-only |
| Exact hook-range scan | passed | No other governed addon source owns `0x43ACB0`, `0x5896E0` or `0x589820`; EventFunc20 remains owned by Melee Splash |

## Disposable intermod fixture

The separate `CastTriggersTest` fixture does not read or modify BKVince data.
It generated free stat IDs `368/369`, free property IDs `284/285`, two cube
recipes and independent localization. D2R compiled **188 tables** from TXT:
185 game tables and the three fixture overlays.
The pre-existing Blizzard ItemStatCost duplicate ID `213` is preserved
unchanged; neither custom ID collides with it or any other row.

| Case | Status | Current proof or blocker |
|---|---|---|
| Fixed and source-level properties compile | passed | ItemStatCost, Properties and CubeMain compile without table error |
| Mod-local diagnostics override global config | passed | Plugin log resolves `mods/CastTriggersTest/d2rloader/config/ruffneckk-cast-triggers.toml` |
| 25% fixed level 12 Fire Ball | blocked | No visible gameplay frame because D2Prism reports `Present failed` |
| 100% one proc per eligible cast | blocked | Same presentation blocker |
| Source effective level, including bonuses | blocked | Same presentation blocker; diagnostic requested/effective-level proof not produced |
| Frost Nova triggers Nova | blocked | Same presentation blocker |
| Attack exclusion | not run | Requires gameplay input |
| Inferno and Arctic Blast exclusion | not run | Requires gameplay input |
| Lightning and Chain Lightning single dispatch | not run | Requires gameplay input |
| Triggered-skill chain exclusion | not run | Requires gameplay input |
| Include/exclude runtime behavior | not run | Parsing and policy pass; gameplay behavior remains open |

The same-level `max=64` encoding is supported by D2MOO semantics and accepted
by the current D2R TXT compiler. It must not be declared runtime-proven until a
fresh diagnostic line shows `requested-level=0` and the source effective level
as `effective-level`.

## Runtime matrix

| Gate | Status | Current proof |
|---|---|---|
| Build identity | passed | D2R `3.3.93847`, Build Key `623f7a1f73eabb08ccb2b2046e3f9164` |
| Global scope and hooks | passed | 17:30 cold start loaded final DLL hash `0C9B…695C9` as `[global]`; startup completed |
| Mod-local scope and hooks | passed | 17:32 cold start loaded final DLL hash `0C9B…695C9` as `[mod]`; 25 plugins, five eezstreet plugins, startup completed |
| No duplicate scope | passed | Global DLL was absent during the mod-local run |
| Current complete Suite coexistence | blocked | At 17:30, the refreshed `Resistance Floor [mod]` still failed before Cast Triggers loaded: safety-check size did not match its jmp-rel32 patch at `0x4524C4` |
| Earlier complete-stack observation | passed, superseded | At 17:11, before Resistance Floor appeared in the runtime, 31 plugins and 18 patches loaded with Cast Triggers `[global]` and startup completed |
| Five eezstreet plugins | passed | Items, Levels, Misc, Quests and Skills loaded in global and mod-local-scope cold starts |
| Gameplay counters | blocked | No source-cast or proc counters can be collected through the transparent frontend |
| Multiplayer host/client | not run | Gameplay gate must pass first |

The Resistance Floor failure is not attributed to Cast Triggers: it occurs
earlier in load order, and Cast Triggers subsequently loads successfully. It
still blocks the mandatory *current* all-components-active Suite claim.

## Packaging and rollback

| Gate | Status | Requirement |
|---|---|---|
| ZIP payload | passed | `CastTriggers-0.1.0-rc.zip`, exactly the DLL and TOML |
| Human-review documents | passed | README, intermod guide and this validation file remain outside the ZIP |
| Reproducible DLL | passed | Two consecutive Release builds produced the same SHA-256 |
| Rollback | passed by design | Remove DLL/TOML; no proprietary save payload or migration exists |

Final release-candidate artifacts:

| File | Bytes | SHA-256 |
|---|---:|---|
| `d2rl-ruffneckk-cast-triggers.dll` | 174592 | `0C9B3FA38B8AABC330A66B67AA39B8F47E9B3D852A619F54EB7F3E1C112695C9` |
| `ruffneckk-cast-triggers.toml` | 621 | `CB0C4EE88346EE08CC9366A6130C8D1B5BE7CDEBC656163584CD1722C6FC05F5` |
| `CastTriggers-0.1.0-rc.zip` | 82833 | `3D10C3F2076D58299951A87145E362C01F3DCA7884884869007FA664390EEF80` |

## Gates required before publication

1. Repair and requalify the independently developed Resistance Floor plugin,
   then rerun the complete active Suite matrix without disabling anything.
2. Resolve the D2Prism presentation failure and execute every gameplay case.
3. Capture diagnostics proving fixed level, source effective level and proc
   chain exclusion.
4. Run solo host and client coverage because the hook is server-authoritative.
5. Review the README beside the release-candidate ZIP before publication.
