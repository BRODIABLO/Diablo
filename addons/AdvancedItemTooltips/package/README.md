# Advanced Item Tooltips 3.2.4

## Install

Global:

- DLL: `<D2R>/d2rloader/plugins/AdvancedItemTooltips.dll`
- JSON: `<D2R>/d2rloader/config/AdvancedItemTooltips.json`

Mod-local:

- DLL: `<D2R>/mods/<mod>/d2rloader/plugins/AdvancedItemTooltips.dll`
- JSON: `<D2R>/mods/<mod>/<mod>.mpq/d2rloader/config/AdvancedItemTooltips.json`

The mod-local JSON overrides the global JSON.

## Options

- `enabled`: enables the plugin. Default: `true`.
- `showMaxSockets`: shows `Max Sockets`. Default: `true`.
- `showMaxSocketsOnSocketedItems`: keeps that line after socketing. Default: `false`.
- `showBaseDefenseRange`: shows base-defense rolls. Default: `true`.
- `showPropertyRanges`: shows property roll ranges. Default: `true`.
- `includeSocketedContributionsInRanges`: includes gems, runes, and jewels in ranges. Default: `false`.
- `propertyRangeColor`: accepts `ChronicleColor` for Chronicle's default
  teal/light blue or `BHDarkGreen` for BH's dark green from the first plugin
  iterations. Default: `ChronicleColor`.

Crafted-item creation properties from `usetype,crf` recipes remain part of the
intrinsic roll range. Later `useitem` and non-crafted `usetype` mutations are
ignored because a finished item does not retain a portable, complete recipe
history across mods.
