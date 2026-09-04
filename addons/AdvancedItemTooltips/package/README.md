# Advanced Item Tooltips 3.4.0

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
- `rangeDisplayMode`: accepts `Always` or `HoldHotkey`. `HoldHotkey` shows
  property ranges and `Base Defense` only while the configured key is held.
  Legacy `HoldShift` is also accepted. `Max Sockets` remains controlled
  separately. Default: `Always`.
- `holdToDisplayHotkey`: selects the key used by `HoldHotkey`. Accepts Shift,
  left/right Shift, Ctrl or Alt, A-Z, 0-9, F1-F12, Mouse4, and Mouse5.
  Default: `Shift`.
- `propertyRangeColor`: accepts `ChronicleColor` for Chronicle's default
  teal/light blue or `BHDarkGreen` for BH's dark green from the first plugin
  iterations. Default: `ChronicleColor`.

Crafted-item creation properties from `usetype,crf` recipes remain part of the
intrinsic roll range. Later `useitem` and non-crafted `usetype` mutations are
ignored because a finished item does not retain a portable, complete recipe
history across mods.
