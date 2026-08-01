# Floating Damage

Mod-scoped D2RLoader plugin for BKVince on `D2R.exe 3.2.92777`.

Original D2RLAN/D2RHUD floating-damage renderer by d2rlan; D2RLoader 3.2
port and plugin integration by RuffnecKk.

The plugin ports the D2RLAN/D2RHUD floating-number renderer, embeds the same
twelve font presets, captures post-resistance damage through the verified
3.2 damage-info routine, and renders through one DirectX 12/ImGui host. Version
1.1.0 also exposes a bounded named-overlay registry used by compatible
RuffnecKk plugins such as CharmZone, so they do not install competing swap-chain
hooks. Floating Damage is visual-only and does not write combat state.

Runtime configuration is stored in
`d2rloader/config/floating-damage.toml`. The D2RLoader console command is:

```text
floating-damage [status|on|off|toggle|preview|reload|reset]
```

Build with Visual Studio 2022 and CMake 3.28 or newer:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```
