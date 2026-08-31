# BKVince Revive Overhaul testing harness

State: `2.1.1 PREPARED_NOT_RUN`.

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

`config/lab.toml` enables Revive Overhaul diagnostics. The runtime is prepared
with this file before testing. Reapply it without launching D2R with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\Prepare-BKVinceReviveLab.ps1
```

Restore the normal configuration after testing with:

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

The active BKVince `Skills.txt` contract must remain:

```text
SrvStFunc = 21
SrvDoFunc = 58
CltStFunc = 24
CltDoFunc = blank
SelectProc = 3
TargetCorpse = 1
PetType = revive
```

No TXT/TSV port and no `SrvDoFunc 49` change belong to this harness.
