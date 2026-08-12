# ProgressiveAffixesPlugin 0.2.1

Increases generated item affix counts as item levels rise.

This standalone RuffnecKk plugin targets D2R 3.2 build 92777 and works from either the global or mod-local D2RLoader plugin directory. It does not declare `ModScopedOnly`, modify an eezstreet DLL, or use `D2RPlugins.json`.

## Behavior

The shipped `ProgressiveAffixesPlugin.toml` uses player-ready, PD2-inspired defaults:

- Magic weapons and armor guarantee two affixes from item level 65, jewels and jewelry from 85, and charms from 90.
- Rare Jewels progress from the vanilla 50/50 split between three and four affixes at item level 1 to four guaranteed affixes at item level 85.
- Other Rare items progress from weighted 3-6 affixes to six guaranteed affixes at item level 85.
- Crafted items progress from weighted 1-4 random affixes to four guaranteed affixes at item level 71, in addition to their fixed recipe properties.

The plugin preserves D2R's native item seed and consumes one RNG step for each Rare or Crafted count selection. D2R remains responsible for selecting legal prefixes and suffixes, enforcing three-prefix/three-suffix limits, applying stats, serializing items, and synchronizing generated items.

## Player-ready configuration

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

The four visible gameplay sections are:

- `[magic]`: the item level where each family begins receiving one prefix and one suffix.
- `[rare_jewels]`: percentages for three or four affixes.
- `[regular_rare_items]`: percentages for three, four, five, or six affixes.
- `[crafted]`: percentages for one, two, three, or four random affixes.

Every percentage row must total exactly 100. Whole numbers and values with up to two decimals are accepted. A key such as `from_level_45` becomes active at item level 45 and remains active until the next configured level. Every Rare and Crafted progression must include `from_level_1`.

Version 0.2.1 still accepts the advanced array-table syntax shipped with 0.1.0, including custom ordered `itemtypes.txt` categories and integer weights. Simple and advanced formats cannot be mixed in one file.

The built-in fallback is disabled when the TOML is absent. A present but invalid TOML is refused before any hook is installed. Unknown item-type codes in an advanced configuration disable progressive selection and produce an explicit error instead of silently matching another category.

The `progressive-affixes` console command reports the loaded configuration, category counts, resolved item types, rejection state, and runtime selection counters.

## Compatibility and evidence

The DLL validates build 92777, native RNG and item-type ABIs, all three generator entries, and every replaced instruction range before writing code. Its owned ranges are the Magic count loads at `0x442C78` and `0x442CDC`, the Crafted count roll/clamp at `0x58A21B` and `0x58A220`, and the Rare count branch/block at `0x58BC65` and `0x58BC90..0x58BCAD`.

Version 0.2.1 preserves the Magic generator's wrapper and generation arguments across the prefix selector call. This is required because `0x442C78` runs before D2R saves those volatile argument registers.

The pinned PluginPack reference at commit `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` owns none of those ranges. BKVince's older force-max JSON patchsets must not coexist with this plugin; the BKVince integration replaces them atomically with the DLL and TOML.

The public ZIP is intentionally limited to `ProgressiveAffixesPlugin.dll` and `ProgressiveAffixesPlugin.toml`; this README remains with the project documentation.
