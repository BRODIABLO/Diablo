# Advanced Item Tooltips

Hybrid D2RLoader plugin for D2R 3.2.92777. It can be installed either in the
global `d2rloader/plugins` directory or in a mod-local plugin directory.

The plugin formats a vanilla-white `Max Sockets: N` line for every item whose
native `ITEMS_GetMaxSockets` capacity is non-zero. Items already displayed as
`Socketed (N)` keep the `Max Sockets` line; socketing must not alter the rest of
the tooltip enhancement pipeline.
Its autonomous final-tooltip pipeline inserts the line below the full weapon
damage block or below an armor's defense line.

For identified items, the plugin also reads the loose TXT tables from the
currently loaded mod and appends exact variable roll ranges using
SlashDiablo's dark green (`:`), distinct from set-item green. The
calculation starts from the affix identifiers stored on the item, combines all
sources that render as the same stat, and includes fixed `cubemain.txt` crafted
properties. For example, a fixed crafted `5-10% Faster Cast Rate` property and
a `+10% Faster Cast Rate` suffix are displayed as `[15 - 20]`.

Socket fillers are resolved individually. Magic, rare, and unique jewel affixes
are merged with the parent item before tooltip validation. Loose gems and runes
use the active mod's `gems.txt` weapon, armor, or shield columns according to
the socketed parent. Fixed filler bonuses shift an overlapping variable range
without receiving a fake range of their own, and multiple fillers accumulate.

Runewords are resolved from the concrete item's native compiled RunesTxt
record, then matched to the active mod's `runes.txt` localization key. All
active runeword rows are loaded. Their variable scalar properties are combined
with the fixed weapon, armor, or shield bonuses of every rune from `gems.txt`.
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

Craft recipe variants are filtered against every modeled stat in the complete
tooltip before any individual range is rendered. This lets an identifying line
such as stacked Faster Cast Rate select the Caster recipe and apply its fixed
Mana bonus to the separate Mana range. If multiple remaining recipes disagree,
or if a property function is not modeled safely, the range is omitted. This
fail-closed policy also applies to base defense when Enhanced Defense prevents
an exact reconstruction. Plain armor can receive a white
`Base Defense: N [min - max]` line immediately below its Defense line.

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

For `uniqueitems.txt` and `setitems.txt`, only rows carrying a valid `*ID` are
indexed. Blank section labels never replace the real record 0, including The
Gnasher and Civerb's Ward.

D2R stores final-tooltip lines in bottom-to-top display order. The plugin
therefore inserts the socket line immediately before the lowest displayed
`Damage:` line or the `Defense:` line in the internal buffer.

Advanced Item Tooltips rejects unsupported D2R builds. It redirects all seven
verified direct call-sites of the native final-tooltip builder through one
private near relay, calls the live D2R builder first, then applies its own
idempotent transformation. It never imports or resolves Transmogrify,
ExtendedItemStats, `plugin-items.dll`, or any other PluginPack DLL. This also
avoids competing for the builder's strict prologue when another plugin owns it.
