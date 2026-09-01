# RuffnecKk Scripted AI native contract

## Authority and ownership

- Plugin role: `D2RL::PluginFlags::Server | D2RL::PluginFlags::NativeHooks`.
- Supported scopes: global and mod-local; `ModScopedOnly` is forbidden.
- Future owned hook: `AITHINK_GetAiTableRecord`, RVA `0x4A36C0`, 17 bytes.
- Pre-install ownership must be `Unchanged`, zero owners.
- Post-install and first-think ownership must be `Tracked`, `InlineHook`, one owner, ID `ruffneckk-scripted-ai`.
- Any missing diagnostics service, unknown mutation, second owner, mismatched owner, or changed fingerprint refuses Lua before the first scripted think.
- Version and build labels are diagnostic only and never decide activation.

The `0.2.0` bridge DLL does not install the hook. It compiles the pre-install policy and positive/negative post-install ownership policy for a later native integration gate.

## Data and lifecycle bridge

- `CustomTableServiceV1` owns one 76-byte `aiscript` row schema in both Base and RotW; stock `monai.txt` and `MonStats.AI` remain untouched.
- `ResourceServiceV1` supplies header-only defaults, so an absent mod override is a valid empty table.
- `copyRows` is revision-checked and the complete Base+RotW snapshot is rejected on any metadata, row, path, source, or tree failure.
- Script files are canonicalized beneath the configured active-mod or D2RLoader-scope root before their bytes are copied.
- `DataTablesLoaded` stages bytes without Lua. Only a successful `ThreadServiceV1::runOnGameThread` callback may compile and publish a session generation.
- `GameJoined`, `GameLeft`, and `sessionGeneration` invalidate late work. `Unavailable` on a remote TCP/IP client means no Lua VM is created there.
- Publication swaps one immutable generation. A failed replacement preserves the prior same-session generation until readers release it.

## Fail-closed fingerprint

All 22 windows are read before the future hook is installed. Only the resolver entry may become owned; the other 21 remain read-only witnesses.

| Surface | RVA | Bytes |
|---|---:|---:|
| `UNITS_GetClassId` | `0x349860` | 46 |
| `UNITS_GetUnitType` | `0x34B9D0` | 45 |
| minimal tick context | `0x4A2ADA` | 117 |
| category dispatch | `0x4A2BD6` | 28 |
| category-2 targeting | `0x4A2C7A` | 38 |
| callback handoff | `0x4A2CED` | 19 |
| AI record resolver | `0x4A36C0` | 17 |
| special-state lookup | `0x4A3767` | 32 |
| normal AI lookup | `0x4A3787` | 63 |
| native target selection | `0x595750` | 32 |
| idle entry | `0x4A6D10` | 33 |
| idle reschedule | `0x4A6D71` | 16 |
| attack entry | `0x4A78E0` | 32 |
| attack terminal | `0x4A7900` | 40 |
| cast entry | `0x4A7BC0` | 28 |
| cast success/fallback | `0x4A7C9F` | 70 |
| retreat entry | `0x4A7DF0` | 27 |
| retreat terminal | `0x4A7F1D` | 35 |
| wander entry | `0x4A8320` | 34 |
| wander terminal | `0x4A84A0` | 64 |
| chase body | `0x4A8740` | 39 |
| shared movement guard | `0x4A8A10` | 62 |

Each compiled entry includes its exact expected bytes and SHA-256 witness. The incubation test reparses the governed canonical PE, compares every RVA byte-for-byte, and requires exactly one occurrence of every window in `.text`.

## Runtime baseline

- Runtime to test: D2R `3.3.93847`.
- Covered by governed native equivalence only: D2R `3.2.92777`.
- D2RLoader: `1.2.0-beta` installed baseline.
- PluginSDK: API v3, commit `4933e2c42cb2592958cd0df3b6dc5003102252d1`.
- PluginPack integration reference: D2RL-Plugins `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`.
- D2MOO semantic reference: `19019806df7f3e877fa105b05395d1e3597e2316`.

The installed-stack snapshot observed 36 plugins, all five eezstreet plugins, and 17 memory patches while leaving the resolver and inspected helper prefixes vanilla. Runtime post-install ownership, load orders, global scope, mod-local scope, TCP/IP authority, performance, and gameplay are not claimed by this source gate.
