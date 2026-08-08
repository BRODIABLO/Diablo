# Static Field Rework 0.1.0

Adds a timed lightning-resistance debuff to Static Field while preserving the native 25% current-life damage mechanic.

This standalone RuffnecKk plugin targets D2R 3.2 build 92777 and works from either the global or mod-local D2RLoader plugin directory. It does not declare `ModScopedOnly`, modify an eezstreet DLL, or depend on a PluginPack-owned configuration block.

## Behavior

The BKVince data companion keeps `srvdofunc=20` and provides the native data consumed by the plugin:

- life damage: 25%;
- radius: `min(ln12 / 2, 14)` with `Param1=8` and `Param2=1`;
- lightning resistance: `-min(lvl, 100)`;
- duration: `125 + (5 * skill('Lightning Mastery'.blvl))` frames;
- target state and overlay: `staticfield_debuff`.

The DLL hooks the build-92777 Static Field server handler, calls its original implementation first, then invokes D2R's native curse/state applicator with the same skill and level. D2R therefore owns the authoritative target filtering, state lifetime, statlist creation, resistance mutation, and multiplayer server behavior.

## Installation

Place `StaticFieldRework.dll` in a D2RLoader `plugins` directory and keep `StaticFieldRework.toml` in the matching `config` directory.

Global installation:

```text
<D2R>/d2rloader/plugins/StaticFieldRework.dll
<D2R>/d2rloader/config/StaticFieldRework.toml
```

Mod-local installation:

```text
<D2R>/mods/<mod>/d2rloader/plugins/StaticFieldRework.dll
<D2R>/mods/<mod>/d2rloader/config/StaticFieldRework.toml
```

The built-in fallback is disabled when no configuration file is found. Set `enabled = true` in the shipped TOML to activate the hook. Set `diagnostics = true` only while testing; it logs one line per successful Static Field debuff cast. The `static-field-rework` console command reports the active state, configuration path, and successful cast count.

## Compatibility and evidence

The DLL validates D2R build 92777 and exact entry signatures before installing its only hook. The hook at RVA `0x5546B0` and the native state applicator at RVA `0x55D6B0` do not overlap the hooks or patches in the five PluginPack DLLs pinned by this workspace at commit `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`.

D2MOO was used as a semantic reference for the legacy `SKILLS_SrvDoFunc20_SrvDoStaticField` and `SKILLS_SrvDoFunc30_Curse` responsibilities. D2MOO addresses, structures, and 32-bit ABI were not reused. Credit: the D2MOO project and its contributors, pinned by this workspace at commit `19019806df7f3e877fa105b05395d1e3597e2316`.

The public ZIP is intentionally limited to `StaticFieldRework.dll` and `StaticFieldRework.toml`; this README remains with the project documentation.
