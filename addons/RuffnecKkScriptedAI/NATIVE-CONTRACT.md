# RuffnecKk Scripted AI native contract

## Authority and ownership

- Plugin role: `D2RL::PluginFlags::Server | D2RL::PluginFlags::NativeHooks`.
- Supported scopes: global and mod-local; `ModScopedOnly` is forbidden.
- Owned hook: `AITHINK_GetAiTableRecord`, RVA `0x4A36C0`, 17 bytes.
- Pre-install ownership must be `Unchanged`, zero owners.
- Post-install and first-think ownership must be `Tracked`, `InlineHook`, one owner, ID `ruffneckk-scripted-ai`.
- Any missing diagnostics service, unknown mutation, second owner, mismatched owner, or changed fingerprint refuses Lua before the first scripted think.
- Version and build labels are diagnostic only and never decide activation.

The `0.7.0` DLL installs exactly this one D2RLoader-managed hook after the complete fingerprint and pre-install ownership pass. It re-attests exclusive ownership on installation and before the first scripted resolution of each session. When diagnostics are enabled, bounded one-shot lines record the first routed ordinary think and first handled Revive tactical action in each session; no per-tick logging is introduced. Teardown first clears its operational flag, bounded Revive blackboard and bridge state; D2RLoader then restores the managed hook during unload.

## Data and lifecycle bridge

- `CustomTableServiceV1` owns one 76-byte `aiscript` row schema in both Base and RotW; stock `monai.txt` and `MonStats.AI` remain untouched.
- `ResourceServiceV1` supplies one disabled row per virtual bank because D2RLoader 1.2.0-beta rejects header-only virtual custom tables. Each sentinel is filtered before binding validation, source loading, or Lua VM creation, so an absent mod override remains a valid semantic empty table.
- `copyRows` is revision-checked and the complete Base+RotW snapshot is rejected on any metadata, row, path, source, or tree failure.
- Script files are canonicalized beneath the configured active-mod or D2RLoader-scope root before their bytes are copied.
- `DataTablesLoaded` stages bytes without Lua. Only a successful `ThreadServiceV1::runOnGameThread` callback may compile and publish a session generation.
- `GameJoined`, `GameLeft`, and `sessionGeneration` invalidate late work. `Unavailable` on a remote TCP/IP client means no Lua VM is created there.
- Publication swaps one structurally immutable generation. Only bounded per-script error/slow/quarantine counters mutate on the authoritative thread; a failed replacement preserves the prior same-session generation until readers release it.
- An enabled `[domains.revive]` script is staged directly from the same canonical script root and compiled into the same immutable generation. It does not require or create an `aiscript` row.

## Optional Revive tactical ABI V3

- Export: `RuffnecKkScriptedAIQueryReviveTacticsV3` in addition to the three required D2RLoader exports. The V2 policy export remains available only so an older Revive Overhaul can fail back safely.
- ABI version: `3`; interface magic: `0x3356495645524941`; current capability bit: `CapabilityTacticalActions`.
- The query verifies the requested ABI version and caller interface size, and returns `nullptr` while the plugin or Revive domain is disabled. Otherwise it returns a process-lifetime read-only interface.
- The copied request contains `game`, Revive, current target, owner, active MonStats and signed distance/special-state/combat facts. They are opaque synchronous C ABI values and are never exposed to Lua. Scripted AI revalidates authority, thread, active session, resolver ownership, exact MonStats row, unit types, GUIDs, deaths and all distances before entering Lua.
- `Handled` means exactly one target action, or a terminal cast fallback already scheduled by D2R, was executed synchronously. Revive Overhaul must skip the original `AiFunction03` for that tick. `DelegateNative`, `Unavailable`, `Error`, an unknown value, absence or incompatibility call the original exactly once.
- Revive Overhaul owns `AiFunction03` at `0x4A3A20`. Scripted AI neither installs nor requests that hook. Scripted AI continues to own only its resolver hook at `0x4A36C0`.
- The contract is optional and private to the RuffnecKk Suite. Neither plugin loads the other, and each remains operational when the other is absent.

The V3 blackboard is capped at 2,048 Revives and keyed by session plus monster GUID/class. A target lock stores type, GUID and its original address. Every use re-resolves the GUID through `SUNIT_GetServerUnit 0x48FE80`, requires the same address without dereferencing the retained address, rejects `SUNIT_IsDead 0x34C2C0`, recomputes both distances and drops the lock outside 32 units of the owner. Target absence, hard-leash escape, game join, game leave and teardown clear the relevant state. The blackboard also holds only the last committed action and the next MonStats skill slot.

## Execution and native-adapter boundary

- The retained tree is traversed by one trusted Lua evaluator; author sources return declarative tables and cannot provide per-think closures.
- Each tick receives copied target/combat/distance values and one ephemeral handle carrying only the session generation and think token. The Revive domain additionally receives copied owner radius, profile, previous-action and preferred-skill facts.
- Typed capabilities accept or reject bounded idle, wander, attack, chase, retreat and cast intents for ordinary scripted monsters. Revive V3 uses the same adapter but its shipped tree contains only target attack/chase/retreat/cast actions; it never idles, wanders or chases the owner while a target exists.
- Authority, unit, target, owner, mode, skill, session, token, signed distances, and scalar widths are validated before the corresponding adapter method. Lua cannot provide a native mode or pointer.
- Cast resolves the requested skill only among the active monster's eight compiled skill slots at `MonStats+0x1C4`; it reads the paired signed mode at `+0x1DC` and accepts only `0..15` before calling `AITACTICS_UseSkill`.
- The first accepted intent terminates the complete tree. Side-effect-free rejected leaves may continue through a selector, but a second action is impossible. A handled Revive action returns directly to Revive Overhaul without running the original AI callback.
- `FallbackScheduled` is a distinct terminal capability result. It models the proven failed-cast path that already schedules a ten-frame idle and prevents both the next selector leaf and a second idle.
- Every `lua_pcall` path clears the userdata pointer and invalidates the C++ handle before returning.
- A valid post-callback path with no accepted action, explicit fallback, stale token, Lua/capability failure, instruction exhaustion, allocation exhaustion, or quarantine attempts exactly one ten-frame idle. A rejected explicit idle is never retried recursively.
- `FallbackAi` is resolver-only. Before an ordinary callback, a known binding with a stale, unavailable, or quarantined script selects the configured stock record; special states and unbound monsters delegate to the original resolver. The Revive overlay has no `FallbackAi`: every non-action returns control to Revive Overhaul, which invokes the original callback in its valid native context.
- Per-script error and slow-think counters are session-local operational state. Three errors or three Lua-wall strikes above `2 ms`, excluding capability time, quarantine that script until the generation is replaced.

## Fail-closed fingerprint

All 29 windows are read before the hook is installed. Only the resolver entry may become owned; the other 28 remain read-only witnesses.

| Surface | RVA | Bytes |
|---|---:|---:|
| `UNITS_GetClassId` | `0x349860` | 46 |
| `UNITS_GetUnitId` | `0x34A330` | 32 |
| `UNITS_GetUnitType` | `0x34B9D0` | 45 |
| `SUNIT_IsDead` | `0x34C2C0` | 32 |
| `SUNIT_GetServerUnit` | `0x48FE80` | 50 |
| native full-unit distance | `0x596720` | 62 |
| minimal tick context | `0x4A2ADA` | 117 |
| category dispatch | `0x4A2BD6` | 28 |
| category-2 targeting | `0x4A2C7A` | 38 |
| callback handoff | `0x4A2CED` | 19 |
| AI record resolver | `0x4A36C0` | 17 |
| Unit → active MonStats record | `0x4A3720` | 17 |
| monster skill → mode lookup | `0x33DC79` | 82 |
| compiled MonStats flags-byte witness | `0x44CF23` | 63 |
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

The `0.5.1` mod-local runtime qualification observed 38 plugins, all five eezstreet plugins, 17 memory patches, and `24/24` startup completion. With the plugin enabled and no physical table override, D2RLoader compiled the disabled sentinel as one 76-byte row in both Base and RotW; Scripted AI staged zero unique scripts and created no Lua VM. Hook installation and its immediate exclusive-ownership check passed. Static workspace audit finds no second claimant for `0x4A36C0`. First-think ownership, reverse load orders, global scope, TCP/IP authority, performance, entity routing, and gameplay remain for `RUN-AI`.
