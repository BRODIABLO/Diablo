# RuffnecKk Scripted AI

RuffnecKk Scripted AI lets configured monsters run bounded Lua behavior trees while Diablo II: Resurrected keeps ownership of targeting, pathfinding, combat modes, skills, and AI scheduling.

## Source-gate status

Version `0.4.0` closes the injectable native-adapter source gate; it is still not a gameplay release. Its shipped configuration is disabled. When explicitly enabled, it registers the private `aiscript` table in the Base and RotW banks, copies and validates each complete table revision, confines script paths to the configured root, and publishes a compiled session generation only on a locally authoritative game thread. Retained trees now map every action to a typed adapter contract, but no runtime path invokes that contract yet.

With `enabled = false`, the plugin loads without querying services, reading native D2R surfaces, registering resources, creating a Lua VM, or installing a hook. With `enabled = true`, the bridge can load and validate data, but it still installs no AI resolver hook and links no D2R gameplay-action implementation. No monster can enter Lua in this version; all native-adapter acceptance tests use deterministic doubles.

## AIScript data contract

The plugin registers header-only defaults at these virtual resources, which an active mod may replace:

```text
data/global/excel/base/d2rloader/ruffneckk-scripted-ai/aiscript.txt
data/global/excel/d2rloader/ruffneckk-scripted-ai/aiscript.txt
```

The strict TSV header is:

```text
MonStatsId	Script	FallbackAi	TargetProfile	Enabled
```

- `MonStatsId` is the physical monster row ID and must be unique among enabled rows in one bank.
- `Script` is a printable-ASCII relative `.lua` path of at most 64 bytes. Backslashes, absolute paths, dot components, parent traversal, missing files, and symlink escapes are rejected.
- `FallbackAi` must remain in the stock range `0..154`.
- `TargetProfile` is frozen to `2` for the governed resolver design.
- `Enabled` accepts only `0` or `1`; a disabled row does not require a script.

Base and RotW are copied with revision checks. One invalid row, path, source, or behavior tree rejects the complete pending transaction. The last successfully compiled same-session generation remains published.

## Behavior-tree V1 contract

Every Lua source executes once at load and must return one acyclic plain-table tree. User closures and per-think script code are not accepted. A trusted evaluator embedded in the sandbox owns the tick semantics:

- `selector` visits children in order until one succeeds, commits an action, or requests fallback.
- `sequence` requires each condition to succeed; its first accepted action terminates the complete tick.
- Conditions are `has_target`, `in_combat`, `target_distance_lte`, and `target_distance_gte`.
- Action leaves are `idle`, `wander`, `attack`, `chase`, `retreat`, and `cast`. Frames, radii, and retreat distances are bounded to `1..255`; skill IDs are bounded to `0..65535`.
- `fallback` requests the single fallback result explicitly.

The evaluator receives only copied booleans and distance plus an ephemeral `{sessionGeneration, thinkToken}` capability handle. A side-effect-free rejected action leaf may let its enclosing selector try another leaf. The first accepted action is committed and stops the whole tree; a capability that already scheduled its own fallback is equally terminal. A second action cannot reach the provider after either result. Absence of an accepted action, explicit fallback, a stale handle, Lua/capability error, instruction exhaustion, allocation exhaustion, or quarantine all produce one typed fallback decision.

The `0.4.0` source gate maps intents to typed idle, wander, attack, chase, retreat, and cast adapter methods. Authority, monster, target, mode, skill, session, token, distance, and scalar bounds are revalidated before the relevant method can run. Raw native modes and pointers never cross into Lua.

`FallbackAi` is now frozen as a **pre-callback resolver policy**: a known binding whose generation is stale, unavailable, or quarantined selects its configured stock AI record before target-category dispatch. An unbound monster or any nonzero special state delegates to the original resolver. Once the Scripted AI callback has started, it never invokes another stock AI callback out of context. An accepted action owns continuation; any post-callback failure schedules one ten-frame idle. A rejected cast that already scheduled the native internal idle terminates immediately and receives no second idle.

The shipped DLL still contains only the injectable contract and deterministic doubles. It does not call attack, cast, movement, idle, event, or resolver functions in D2R.

## Authority and lifecycle

`DataTablesLoaded` only copies rows and source bytes; it never calls Lua. `GameJoined` and `GameLeft` run on the UI thread and only publish an atomic desired session before requesting `runOnGameThread`. Lua compilation and generation replacement happen inside that authoritative callback. A remote TCP/IP client receives `Unavailable`, so it never creates a Lua VM. Late callbacks re-read the desired `sessionGeneration` and cannot resurrect a departed session.

## Supported installation scopes

The plugin supports either location with the same native contract:

```text
<D2R>/d2rloader/plugins/
<D2R>/mods/<mod>/d2rloader/plugins/
```

Its dedicated configuration is `ruffneckk-scripted-ai.toml`. An active-mod copy takes precedence over the plugin scope, followed by the global D2RLoader configuration. A present but invalid file refuses the plugin instead of silently falling back.

## Security model

- Text Lua only; binary chunks and native modules are rejected.
- No `package`, `require`, `io`, `os`, `debug`, `coroutine`, `ffi`, `string`, `utf8`, `load`, `loadfile`, `dofile`, `collectgarbage`, `pcall`, `xpcall`, or `math.random`.
- A counted allocator enforces the session limit plus per-think peak and gross allocation-growth budgets.
- Instruction hooks enforce separate load and think budgets.
- Each source runs in a fresh allowlisted chunk environment, and compiled trees are retained by the immutable session generation.
- Behavior trees are plain acyclic tables with an exact V1 kind/field allowlist and bounded nodes, depth, and fanout.
- Scripts never receive pointers, GUIDs, memory helpers, or persistent per-unit identity in V1.
- Every handle is invalidated after its `lua_pcall`; three errors or three over-`2 ms` Lua wall-time strikes quarantine the shared script for the rest of that session.
- The future gameplay runtime remains server-authoritative; this source gate proves the native continuation policy through injected doubles without touching D2R.

## Native compatibility

Runtime activation is decided by 22 exact instruction-aligned windows, not by a D2R build-name allowlist. The official runtime to qualify is D2R `3.3.93847`. D2R `3.2.92777` is covered only through the governed byte-exact native equivalence of every used surface. See `NATIVE-CONTRACT.md`.

## Credits and licenses

Authored by **RuffnecKk** as an autonomous component of the RuffnecKk D2RLoader Suite.

The embedded runtime is PUC Lua `5.4.9`, obtained from the official Lua tarball and statically linked under the MIT license. See `THIRD-PARTY-NOTICES.md`.

Native AI semantics were studied with help from the MIT-licensed [D2MOO](https://github.com/ThePhrozenKeep/D2MOO) project at commit `19019806df7f3e877fa105b05395d1e3597e2316`. D2MOO is used only as a semantic Diablo II 1.10f reference; no 32-bit address, structure, or ABI is transplanted into D2R.

The separate npz1k `lua-plugins.zip` artifact was inspected only as an external compatibility reference. RuffnecKk Scripted AI does not copy, link, require, or redistribute its DLL, scripts, offsets, configuration, or code.
