# RuffnecKk Scripted AI

RuffnecKk Scripted AI lets configured monsters run bounded Lua behavior trees while Diablo II: Resurrected keeps ownership of targeting, pathfinding, combat modes, skills, and AI scheduling.

## Source-gate status

Version `0.7.0` adds a dedicated Revive tactical director while retaining the managed resolver-hook design and semantic-empty virtual-table default. Its shipped configuration is disabled. When explicitly enabled, it loads the Revive behavior tree directly from its support directory without an `AIScript` row or any Diablo TXT edit. Ordinary per-monster scripts still use the private `aiscript` table in the Base and RotW banks.

With `enabled = false`, the plugin loads without querying services, reading native D2R surfaces, registering resources, creating a Lua VM, or installing a hook. With `enabled = true`, all 29 native windows and resolver ownership must pass before the hook is installed. Special states, unbound units, unavailable generations, remote clients, and any failed attestation delegate to the original resolver. The `0.7.0` Revive cooperation is statically qualified but not yet gameplay-qualified.

## Revive companion domain

Revive Overhaul `2.3.0` discovers Scripted AI through optional tactical ABI V3. Neither DLL loads the other, and Revive Overhaul keeps its native behavior when Scripted AI is missing, disabled, incompatible, unavailable, or delegates. Revive Overhaul remains the sole owner of its `AiFunction03` hook. Scripted AI may execute exactly one validated enemy-target action and return `Handled`; Revive Overhaul then skips the original callback for that tick. Every other result invokes the original callback exactly once.

To enable the shipped conservative policy, enable both the Scripted AI master switch and its Revive domain:

```toml
enabled = true

[domains.revive]
enabled = true
script = "revive-companion.lua"
```

Copy `revive-companion.lua` to:

```text
<active support directory>/scripts/ruffneckk-scripted-ai/revive-companion.lua
```

The sample derives its profile from the compiled monster record. A cast/skill-mode MonStats slot selects `caster`; otherwise `isMelee` selects `melee`, and the remainder selects physical `ranged`. Melee Revives aggressively chase and attack a locked target, ranged Revives alternate attack and retreat inside a firing band, and casters rotate valid native MonStats spell slots while kiting. All three profiles require the Revive and target to remain inside the owner's bounded combat radius and none wanders while a target exists.

The lock stores only `{type, GUID, original address}` in a bounded native session blackboard. It re-resolves the unit through `SUNIT_GetServerUnit`, requires the same address, rejects dead units, and clears on target loss, hard leash, game join or game leave. Lua sees only copied profile, distance, previous-action and preferred-skill facts. Peaceful following, obstacle recovery, catch-up velocity and transition behavior remain Revive Overhaul's native path; Scripted AI never chases the owner. No `Skills.txt`, `MonStats.txt`, `monai.txt`, custom TSV, or `AIScript` override is required.

## AIScript data contract

The plugin registers one disabled compiler sentinel at each virtual resource below, which an active mod may replace. D2RLoader 1.2.0-beta rejects a virtual custom table containing only its header; the sentinel compiles as one row but is discarded before binding validation, script loading, or Lua VM creation.

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

The embedded default row is `0, empty Script, 0, 0, 0`. It is not a monster binding. On the installed D2RLoader `1.2.0-beta`, a physical mod TXT override is currently refused when the custom-table compiler attempts to write the base D2RLoader BIN cache. Therefore the semantic-empty embedded resource is qualified, but the external binding-source location remains unresolved; do not claim a disk-backed binding workflow from this build.

Base and RotW are copied with revision checks. One invalid row, path, source, or behavior tree rejects the complete pending transaction. The last successfully compiled same-session generation remains published.

## Behavior-tree V1 contract

Every Lua source executes once at load and must return one acyclic plain-table tree. User closures and per-think script code are not accepted. A trusted evaluator embedded in the sandbox owns the tick semantics:

- `selector` visits children in order until one succeeds, commits an action, or requests fallback.
- `sequence` requires each condition to succeed; its first accepted action terminates the complete tick.
- Conditions are `has_target`, `in_combat`, target/owner distance comparisons, `is_melee`, `is_ranged`, `is_caster`, previous attack/retreat/cast checks, and `has_preferred_skill`.
- Action leaves are `idle`, `wander`, `attack`, `chase`, `retreat`, `cast`, `cast_preferred`, and the retained V2 compatibility leaf `follow_owner`. Frames, radii, and retreat distances are bounded to `1..255`; explicit skill IDs are bounded to `0..65535`. `cast_preferred` can use only the native skill selected from the current monster's compiled slots.
- `fallback` requests the single fallback result explicitly.

The evaluator receives only copied booleans, classifications, distances and bounded skill facts plus an ephemeral `{sessionGeneration, thinkToken}` capability handle. A side-effect-free rejected action leaf may let its enclosing selector try another leaf. The first accepted action is committed and stops the whole tree; a capability that already scheduled its own fallback is equally terminal. A second action cannot reach the provider after either result. Absence of an accepted action, explicit fallback, a stale handle, Lua/capability error, instruction exhaustion, allocation exhaustion, or quarantine all produce one typed fallback decision.

The `0.7.0` source maps ordinary monster intents to governed D2R idle, wander, attack, chase, retreat, and cast helpers. The Revive V3 path admits only attack, chase, retreat or a selected native cast while an engine target exists. Authority, monster, target, owner, death state, mode, skill, session, token, distance, identity and scalar bounds are revalidated before the relevant helper can run. A cast is accepted only when its skill is declared in one of the monster's eight compiled `MonStats` slots; the matching per-monster mode is used. Raw native modes, pointers and GUIDs never cross into Lua.

`FallbackAi` is now frozen as a **pre-callback resolver policy**: a known binding whose generation is stale, unavailable, or quarantined selects its configured stock AI record before target-category dispatch. An unbound monster or any nonzero special state delegates to the original resolver. Once the Scripted AI callback has started, it never invokes another stock AI callback out of context. An accepted action owns continuation; any post-callback failure schedules one ten-frame idle. A rejected cast that already scheduled the native internal idle terminates immediately and receives no second idle.

The deployed `0.5.1` DLL passed complete-stack default-off and semantic-empty startup. Candidate `0.7.0` passes its expanded unit suite, including all three Revive profiles, cast-slot selection, tactical fallback with zero synthetic idle, ABI V3 validation and the complete native fingerprint. No `0.7.0` gameplay result is claimed before a separate live-test authorization.

## Authority and lifecycle

`DataTablesLoaded` only copies rows and source bytes; it never calls Lua. `GameJoined` and `GameLeft` run on the UI thread and only publish an atomic desired session before requesting `runOnGameThread`. Lua compilation and generation replacement happen inside that authoritative callback. A remote TCP/IP client receives `Unavailable`, so it never creates a Lua VM. Late callbacks re-read the desired `sessionGeneration` and cannot resurrect a departed session.

## Supported installation scopes

The plugin supports either location with the same native contract:

```text
<D2R>/d2rloader/plugins/
<D2R>/mods/<mod>/d2rloader/plugins/
```

Its dedicated configuration is `ruffneckk-scripted-ai.toml`. An active-mod copy takes precedence over the plugin scope, followed by the global D2RLoader configuration. A present but invalid file refuses the plugin instead of silently falling back.

The Revive provider export is an inter-plugin contract, not an installation dependency. Loading only Scripted AI or only Revive Overhaul remains supported.

## Security model

- Text Lua only; binary chunks and native modules are rejected.
- No `package`, `require`, `io`, `os`, `debug`, `coroutine`, `ffi`, `string`, `utf8`, `load`, `loadfile`, `dofile`, `collectgarbage`, `pcall`, `xpcall`, or `math.random`.
- A counted allocator enforces the session limit plus per-think peak and gross allocation-growth budgets.
- Instruction hooks enforce separate load and think budgets.
- Each source runs in a fresh allowlisted chunk environment, and compiled trees are retained by the immutable session generation.
- Behavior trees are plain acyclic tables with an exact V1 kind/field allowlist and bounded nodes, depth, and fanout.
- Scripts never receive pointers, GUIDs, memory helpers, or persistent per-unit identity. The Revive blackboard remains private to bounded native code and is discarded with the game session.
- Every handle is invalidated after its `lua_pcall`; three errors or three over-`2 ms` Lua wall-time strikes quarantine the shared script for the rest of that session.
- The gameplay runtime is server-authoritative; this source gate proves native routing and continuation statically without claiming a D2R run.

## Native compatibility

Runtime activation is decided by 29 exact instruction-aligned windows, not by a D2R build-name allowlist. The official runtime to qualify is D2R `3.3.93847`. D2R `3.2.92777` is covered only through the governed byte-exact native equivalence of every used surface. See `NATIVE-CONTRACT.md`.

## Credits and licenses

Authored by **RuffnecKk** as an autonomous component of the RuffnecKk D2RLoader Suite.

The embedded runtime is PUC Lua `5.4.9`, obtained from the official Lua tarball and statically linked under the MIT license. See `THIRD-PARTY-NOTICES.md`.

Native AI semantics were studied with help from the MIT-licensed [D2MOO](https://github.com/ThePhrozenKeep/D2MOO) project at commit `19019806df7f3e877fa105b05395d1e3597e2316`. D2MOO is used only as a semantic Diablo II 1.10f reference; no 32-bit address, structure, or ABI is transplanted into D2R.

The separate npz1k `lua-plugins.zip` artifact was inspected only as an external compatibility reference. RuffnecKk Scripted AI does not copy, link, require, or redistribute its DLL, scripts, offsets, configuration, or code.
