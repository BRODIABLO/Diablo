# RuffnecKk Scripted AI

RuffnecKk Scripted AI lets configured monsters run bounded Lua behavior trees while Diablo II: Resurrected keeps ownership of targeting, pathfinding, combat modes, skills, and AI scheduling.

## Incubation status

Version `0.1.0` is an incubation build, not a gameplay release. Its shipped configuration is disabled. It implements and tests the product metadata, configuration precedence, hard sandbox limits, Lua VM restrictions, native fingerprint, and hook-ownership policy. It intentionally does not install the future AI resolver hook or bind monsters to scripts yet.

With `enabled = false`, the plugin loads without reading native D2R surfaces, creating a Lua VM, or installing a hook. Setting it to `true` runs the fail-closed incubation preflight and then refuses activation because the gameplay bridge has not passed its next gate.

## Planned installation scopes

The final plugin will support either location with the same native contract:

```text
<D2R>/d2rloader/plugins/
<D2R>/mods/<mod>/d2rloader/plugins/
```

Its dedicated configuration is `ruffneckk-scripted-ai.toml`. An active-mod copy takes precedence over the plugin scope, followed by the global D2RLoader configuration. A present but invalid file refuses the plugin instead of silently falling back.

## Security model

- Text Lua only; binary chunks and native modules are rejected.
- No `package`, `require`, `io`, `os`, `debug`, `coroutine`, `ffi`, `string`, `utf8`, `load`, `loadfile`, `dofile`, `collectgarbage`, `pcall`, `xpcall`, or `math.random`.
- A counted allocator enforces the session and per-think heap budgets.
- Instruction hooks enforce separate load and think budgets.
- Behavior trees are plain acyclic tables with bounded nodes, depth, and fanout.
- Scripts never receive pointers, GUIDs, memory helpers, or persistent per-unit identity in V1.
- The future runtime remains server-authoritative and falls back to stock AI on every rejected path.

## Native compatibility

Runtime activation is decided by 22 exact instruction-aligned windows, not by a D2R build-name allowlist. The official runtime to qualify is D2R `3.3.93847`. D2R `3.2.92777` is covered only through the governed byte-exact native equivalence of every used surface. See `NATIVE-CONTRACT.md`.

## Credits and licenses

Authored by **RuffnecKk** as an autonomous component of the RuffnecKk D2RLoader Suite.

The embedded runtime is PUC Lua `5.4.9`, obtained from the official Lua tarball and statically linked under the MIT license. See `THIRD-PARTY-NOTICES.md`.

Native AI semantics were studied with help from the MIT-licensed [D2MOO](https://github.com/ThePhrozenKeep/D2MOO) project at commit `19019806df7f3e877fa105b05395d1e3597e2316`. D2MOO is used only as a semantic Diablo II 1.10f reference; no 32-bit address, structure, or ABI is transplanted into D2R.

The separate npz1k `lua-plugins.zip` artifact was inspected only as an external compatibility reference. RuffnecKk Scripted AI does not copy, link, require, or redistribute its DLL, scripts, offsets, configuration, or code.
