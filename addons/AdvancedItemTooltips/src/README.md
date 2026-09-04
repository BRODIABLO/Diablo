# Advanced Item Tooltips

Hybrid D2RLoader plugin for D2R 3.2.92777. It can be installed either in the
global `d2rloader/plugins` directory or in a mod-local plugin directory. This
public release is autonomous: it does not depend on BKVince, Transmogrify, or
any eezstreet PluginPack DLL.

Version 3.4.0 makes the hold-to-display key configurable while preserving the
validated intrinsic item-range pipeline and the existing always-visible
behavior by default. Property and `Base Defense` ranges can now appear only
while the configured key is held; `Shift` remains the default and `Max Sockets`
remains independently configurable.

The plugin caches exact, unchanged tooltip transformations per UI thread.
Repeated hover frames reuse the validated result while any native tooltip text,
affix identity, defense, socket count, or socket filler change invalidates the
entry automatically. Later `useitem` and non-crafted `usetype` mutations are
not reconstructed or combined on a cache miss.

`rangeDisplayMode` accepts `Always` (the default) or `HoldHotkey`. In
`HoldHotkey`, both property ranges and the `Base Defense` line are hidden until
the key selected by `holdToDisplayHotkey` is held, then disappear again when it
is released. `Shift` is the default key. Legacy `HoldShift` configuration is
accepted as an alias for `HoldHotkey`. The pressed state is part of the tooltip
cache key, so the visible and hidden variants cannot contaminate each other.
`Max Sockets` is not a roll range and is unaffected by this mode.

It also derives unique-item, set-item, property, and stat identities from the
same compiled row order used by D2R. Modder comment columns such as `*ID` and
`*Tooltip` may be blank, stale, or duplicated without hiding newly added
records. Properties using several `func`/`stat` slots are decoded component by
component, and conditional set-item bonus groups are evaluated as alternative
equipped states before whole-tooltip consensus chooses a range.

## Installation

Install the DLL in either supported D2RLoader scope:

- global: `<D2R>/d2rloader/plugins/AdvancedItemTooltips.dll`;
- mod-local: `<D2R>/mods/<mod>/d2rloader/plugins/AdvancedItemTooltips.dll`.

The JSON configuration does **not** go beside the DLL. Install it in the
matching D2RLoader `config` directory:

- global or pure vanilla:
  `<D2R>/d2rloader/config/AdvancedItemTooltips.json`;
- mod-local:
  `<D2R>/mods/<mod>/<mod>.mpq/d2rloader/config/AdvancedItemTooltips.json`.

For example, BKVince uses
`<D2R>/mods/BKVince/BKVince.mpq/d2rloader/config/AdvancedItemTooltips.json`.
The active mod's JSON takes priority over the global JSON. If neither file
exists, the plugin uses its built-in public defaults. Restart D2RLoader after
changing the file.

The plugin formats a vanilla-white `Max Sockets: N` line for every item whose
native `ITEMS_GetMaxSockets` capacity is non-zero. The public default hides
that line after the item receives sockets; `showMaxSocketsOnSocketedItems=true`
restores the BKVince behavior. This switch is isolated from the range pipeline.
Its autonomous final-tooltip pipeline inserts the line below the full weapon
damage block or below an armor's defense line.

## Languages

The plugin follows D2R's active language automatically. Native property,
damage, defense, requirement, skill-tab, and runeword text is matched through
the localization keys referenced by the active mod's `itemstatcost.txt`, not
through English tooltip text. The two labels created by the plugin itself,
`Max Sockets` and `Base Defense`, are built in for all thirteen D2R locales:
English, Traditional Chinese, German, European Spanish, French, Italian,
Korean, Polish, Latin American Spanish, Japanese, Brazilian Portuguese,
Russian, and Simplified Chinese. No language option is required in the JSON.

Mods may provide their own translations for native stat keys and the plugin
will use the strings resolved by D2R at runtime. The numeric range color is
language-neutral and selected by `propertyRangeColor`; its connector is read
from D2R's active localization rather than hardcoded as English `to`.

For identified items, the plugin reads the loose TXT tables from the currently
loaded mod and appends exact variable roll ranges. `ChronicleColor` uses D2R's
`U` palette entry for Chronicle's default teal/light blue. `BHDarkGreen` uses
BH's `:` dark-green entry from the first plugin iterations. The calculation
starts from the affix identifiers stored on the item, combines all intrinsic
sources that render as the same stat, and includes fixed `cubemain.txt`
properties from the `usetype,crf` recipe that created a crafted item. For
example, a fixed crafted `5-10% Faster Cast Rate` property and a `+10% Faster
Cast Rate` suffix are displayed as `[15 - 20]`.

Later `useitem` recipes and non-crafted `usetype` recipes are deliberately
ignored. A finished item does not retain a portable, complete record of which
mod-specific mutations ran, how many times they ran, or which roll each
application produced. If such a mutation moves a visible value outside its
intrinsic range, that line remains unannotated rather than receiving a guessed
aggregate. This policy also removes recipe-history combinations from the
mouse-hover path.

Pure vanilla has no loose mod directory for the public D2RLoader SDK to expose.
In that context, the DLL automatically loads its embedded D2R 3.2.92777 vanilla
catalog; no fake mod, `-txt` launch flag, or copied TXT files are required.
Active packages are resolved table by table: a loose TXT supplied by the package
takes priority, while a table that is absent in both TXT and BIN form falls back
to embedded vanilla. Cosmetic online packages therefore keep vanilla ranges,
and partial TXT mods inherit only the vanilla tables they did not replace. A BIN
without its matching TXT is treated as an unreadable gameplay override: range
annotations fail closed with the exact binary path while native `Max Sockets`
remains available.

`includeSocketedContributionsInRanges=false` is the public default. Affixes,
automagic, superior properties, crafts, uniques, sets, and intrinsic runeword
properties remain modeled, while direct gem, rune, and jewel contributions are
excluded. Set the option to `true` to resolve socket fillers individually and
merge their contributions with the parent item before tooltip validation.
Loose gems and runes use the active mod's `gems.txt` weapon, armor, or shield
columns according to the socketed parent. Fixed filler bonuses shift an
overlapping variable range without receiving a fake range of their own, and
multiple fillers accumulate.

Runewords are resolved from the concrete item's native compiled RunesTxt
record, then matched to the active mod's `runes.txt` localization key. All
active runeword rows are loaded. Their variable scalar properties are combined
with rune bonuses only when `includeSocketedContributionsInRanges=true`.
Multiple active rows may intentionally reuse one localization key; the plugin
keeps every variant as a candidate and lets the complete rendered tooltip select
the compatible range instead of rejecting the mod's entire catalog.
For example, Call to Arms combines its `200-240%` runeword Enhanced Damage
with Ohm's fixed `+50%`, so the final line is annotated `[250 - 290]`.

Elemental and physical `Adds X-Y Damage` lines are matched as two independent
visible rolls. When both components are reconstructed exactly, each receives
its own range (for example `[11 - 25] to [31 - 50]`). Chance/skill-level event
properties, charged-skill charges/level, poison duration arithmetic, and other
time/level formulas remain omitted when they cannot be reconstructed exactly.
Fixed-only properties remain unchanged. If a runeword identity cannot be
resolved unambiguously, the plugin omits its ranges and never interprets the
repurposed runeword id as an affix.

Crafted creation-recipe variants (`usetype,crf`) are filtered against every
modeled stat in the complete tooltip before any individual range is rendered.
This lets an identifying line such as stacked Faster Cast Rate select the
Caster creation recipe and apply its fixed Mana bonus to the separate Mana
range. It does not re-enable later `useitem` or non-crafted `usetype`
mutations. If multiple remaining creation recipes disagree, or if a property
function is not modeled safely, the range is omitted. Base defense is
reconstructed exactly across plain, superior, ethereal, unique, set, rare,
crafted, socketed, and runeword armor, including stacked Enhanced Defense and
exact flat `+Defense`. Ambiguous reconstructions still fail closed. Eligible
armor receives a white `Base Defense: N [min - max]` line immediately below
its Defense line.

The range suffix restores the color active on the original modifier line. It
recognizes D2R's private marker and both native `ÿcX` encodings, then follows
the inherited color state through the bottom-to-top tooltip buffer. It does not
replace the item's font or recolor existing modifiers.

The TXT reader mirrors the compiled affix index space: the named `Expansion`
separator in `magicprefix.txt` and `magicsuffix.txt` is not counted as a runtime
record. This keeps every post-separator prefix and suffix aligned with the ids
stored on the item.

Rare and crafted name-word ids are kept separate from property affixes.
`rarePrefix` and `rareSuffix` select generated names such as `Stone Razor`;
only the three `magicPrefix` and three `magicSuffix` slots contribute stats.
Treating name ids as property ids makes ranges depend incorrectly on the random
rare name and is therefore forbidden.

For `uniqueitems.txt` and `setitems.txt`, runtime `fileIndex` follows compiled
physical row order. The named `Expansion` delimiter is skipped, while every
other row—including blank section records—consumes an index. The plugin never
trusts the optional `*ID` comment column, so new unique and set records remain
aligned even when a mod leaves that column empty or reuses an old value.

Set `add func` modes `0`, `1`, and `2` are modeled from `aprop1a` through
`aprop5b`: unconditional groups, companion-piece subsets, and cumulative
piece-count prefixes respectively. Unknown modes are fail-closed and keep only
the set item's intrinsic properties. Property functions that cannot be decoded
exactly are likewise omitted and summarized once in the startup log rather
than fabricating a range.

D2R stores final-tooltip lines in bottom-to-top display order. The plugin
therefore inserts the socket line immediately before the lowest displayed
`Damage:` line or the `Defense:` line in the internal buffer.

Advanced Item Tooltips rejects unsupported D2R builds. It redirects all seven
verified direct call-sites of the native final-tooltip builder through one
private near relay, calls the live D2R builder first, then applies its own
idempotent transformation. It never imports or resolves Transmogrify,
ExtendedItemStats, `plugin-items.dll`, or any other PluginPack DLL. This also
avoids competing for the builder's strict prologue when another plugin owns it.

The strict `AdvancedItemTooltips.json` keys are `enabled`, `showMaxSockets`,
`showMaxSocketsOnSocketedItems`, `showBaseDefenseRange`, `showPropertyRanges`,
`includeSocketedContributionsInRanges`, `_rangeDisplayModeHelp`,
`rangeDisplayMode`, `_holdToDisplayHotkeyHelp`, `holdToDisplayHotkey`,
`_propertyRangeColorHelp`, and `propertyRangeColor`. Help
keys are optional, must be strings when present, and are ignored at runtime.
`rangeDisplayMode` accepts `Always` or `HoldHotkey`; legacy `HoldShift` is also
accepted. `holdToDisplayHotkey` accepts Shift, left/right Shift, Ctrl or Alt,
A-Z, 0-9, F1-F12, Mouse4, or Mouse5, and defaults to `Shift`;
`propertyRangeColor` accepts exactly `ChronicleColor` or `BHDarkGreen`. The
mod-local file takes priority over the global file. Missing configuration uses
built-in defaults; a present but malformed configuration refuses the plugin
before hooks are installed.
