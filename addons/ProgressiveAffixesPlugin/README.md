# ProgressiveAffixesPlugin 0.1.0

Increases generated item affix counts as item levels rise.

This standalone RuffnecKk plugin targets D2R 3.2 build 92777 and works from either the global or mod-local D2RLoader plugin directory. It does not declare `ModScopedOnly`, modify an eezstreet DLL, or use `D2RPlugins.json`.

## Behavior

The shipped `ProgressiveAffixesPlugin.toml` reproduces the Project Diablo 2 progression shown in the item-generation documentation:

- Magic weapons and armor guarantee two affixes from item level 65, jewels and jewelry from 85, and charms from 90.
- Rare jewels always receive four affixes.
- Other Rare items progress from weighted 3–6 affixes to six guaranteed affixes at item level 85.
- Crafted items progress from weighted 1–4 random affixes to four guaranteed affixes at item level 71, in addition to their fixed recipe properties.

Categories are matched in TOML declaration order through D2R's native `itemtypes.txt` inheritance. Each configured quality ends with a wildcard fallback. Integer weights provide exact rational probabilities without floating-point rounding.

The plugin preserves D2R's native item seed and consumes one RNG step for each Rare or Crafted count selection. D2R remains responsible for selecting legal prefixes and suffixes, enforcing three-prefix/three-suffix limits, applying stats, serializing items, and synchronizing generated items.

## Configuration

Place `ProgressiveAffixesPlugin.toml` in the matching D2RLoader `config` directory. The active mod configuration is checked before the installation-scope and global configuration directories.

Global installation:

```text
<D2R>/d2rloader/plugins/ProgressiveAffixesPlugin.dll
<D2R>/d2rloader/config/ProgressiveAffixesPlugin.toml
```

Mod-local installation:

```text
<D2R>/mods/<mod>/d2rloader/plugins/ProgressiveAffixesPlugin.dll
<D2R>/mods/<mod>/d2rloader/config/ProgressiveAffixesPlugin.toml
```

The built-in fallback is disabled when the TOML is absent. A present but invalid TOML is refused before any hook is installed. Unknown item-type codes in the active compiled table disable progressive selection and produce an explicit error instead of silently matching another category.

Supported native count ranges are 1–2 for Magic, 3–6 for Rare, and 1–4 for Crafted items. Item-level thresholds must be strictly increasing. Rare and Crafted categories must start at item level 1; the wildcard fallback must be declared last.

The `progressive-affixes` console command reports the loaded configuration, category counts, resolved item types, rejection state, and runtime selection counters.

## Compatibility and evidence

The DLL validates build 92777, native RNG and item-type ABIs, all three generator entries, and every replaced instruction range before writing code. Its owned ranges are the Magic count loads at `0x442C78` and `0x442CDC`, the Crafted count roll/clamp at `0x58A21B` and `0x58A220`, and the Rare count branch/block at `0x58BC65` and `0x58BC90..0x58BCAD`.

The pinned PluginPack reference at commit `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` owns none of those ranges. BKVince's older force-max JSON patchsets must not coexist with this plugin; the BKVince integration replaces them atomically with the DLL and TOML.

The public ZIP is intentionally limited to `ProgressiveAffixesPlugin.dll` and `ProgressiveAffixesPlugin.toml`; this README remains with the project documentation.
