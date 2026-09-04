# BKVince Revive Overhaul testing harness

State: `2.3.0 + Scripted AI 0.7.0 SOURCE_READY_NOT_DEPLOYED`.

This harness is deployed inside the existing BKVince runtime profile. It does
not create a separate mod or save path. BKVince remains the active mod, keeps
its existing characters and data, and loads the complete global and mod-local
D2RLoader extension stack.

No cold start or gameplay result is implied by preparing this directory.
Launch is blocked until Vincent explicitly authorizes a test.

## Deployed location

```text
<D2R>/mods/BKVince/testing/revive-overhaul
```

## Static preflight

Run from the deployed harness:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\Get-BKVinceReviveLabStatus.ps1
```

The result must report `readyForAuthorizedTest: true` and
`runtimeProcesses: 0`.

## Test configuration

`config/lab.toml` enables Revive Overhaul diagnostics, while
`config/scripted-ai-lab.toml` enables Scripted AI and its dedicated Revive
domain. The preparation script applies both configurations and installs the
companion tactical tree without launching D2R:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\Prepare-BKVinceReviveLab.ps1
```

The two candidate DLLs must already match the hashes enforced by the status
tool. Restore the exact pre-test DLLs and configurations from the laboratory
backup after testing with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\Restore-BKVinceReviveConfig.ps1
```

## Protected launch

After Vincent's explicit `GO` only:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\Launch-BKVinceReviveLab.ps1 -ConfirmGO
```

The launcher explicitly selects BKVince, keeps `-txt`, starts D2RLoader debug
mode and refuses to run if the runtime, loader, plugin, Revive TXT contract or
extension inventories have drifted.

In game, open the console with `Ctrl + Backtick`, use the installed version's
help, activate `debug_mode`, and use `Cheat Controls`, `Unit Inspector` and
`Monster Spawner` for the cases in `TEST-MATRIX.md`.

The active BKVince `Skills.txt` source contract remains:

```text
SrvStFunc = 21
SrvDoFunc = 58
CltStFunc = 24
CltDoFunc = blank
SelectProc = 3
TargetCorpse = 1
PetType = revive
```

Version 2.3.0 verifies that source contract and automatically bridges the
compiled Revive row to `blank / 58 / 39 / 36 / 2` in memory. No TXT/TSV port
and no `SrvDoFunc 49` change belong to this harness.

The `2.3.0` matrix also needs RuffnecKk Scripted AI `0.7.0`, its dedicated
TOML and `revive-companion.lua` in the configured BKVince support directory.
The protected preflight verifies both DLL versions, both DLL hashes, both lab
configuration hashes, the companion-tree hash and the complete extension
inventories before it may report ready.

Private ABI V3 lets Revive Overhaul supply a copied, bounded combat snapshot to
Scripted AI. The Lua tree chooses at most one native target action per tick;
`Handled` prevents a second native AI call, while every refusal or fallback
calls the original Revive AI exactly once. Profiles are derived from compiled
MonStats data: melee pets lock and press targets, missile users kite, and
casters rotate valid native spell slots while kiting. The target blackboard is
native-only, re-resolved by GUID before use, bounded to the owner's combat
radius and cleared on peaceful ticks, hard-leash escape and lifecycle changes.

Peaceful ticks remain entirely under Revive Overhaul/native `AiFunction03` and
retain `catch_up_distance = 8`, `follow_distance = 4`, and
`velocity_bonus = 80`. The next authorized launch must arm ProcDump hang
capture before testing town/combat transitions.
