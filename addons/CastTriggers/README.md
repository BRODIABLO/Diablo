# Cast Triggers 0.1.0

> Release candidate: do not publish until the open gameplay and current Suite
> gates in [VALIDATION.md](VALIDATION.md) are closed.

Cast Triggers lets a modder put Path of Exile-style spell procs directly on
Diablo II: Resurrected items:

```text
25% Chance to cast level 12 Fire Ball when casting a skill
25% Chance to cast Fire Ball at the source skill level when casting a skill
100% Chance to cast Nova at the source skill level when casting a skill
```

Once a mod adds the two reusable
properties described below, either property can be placed on any item row that
has the usual `prop#`, `par#`, `min#` and `max#` columns.

## What "source skill level" means

The **source skill** is the spell the player manually casts. The **target
skill** is the extra spell written on the item.

For example, if the player successfully casts a level 31 Frost Nova while
wearing this modifier:

```text
100% Chance to cast Nova at the source skill level when casting a skill
```

the item casts Nova at level 31. The target does not have to be the same skill
as the source.

## Install

Copy the DLL and TOML into one D2RLoader scope. Use the global scope or the
mod-local scope, never both at once.

Global installation:

```text
<D2R>/d2rloader/plugins/d2rl-ruffneckk-cast-triggers.dll
<D2R>/d2rloader/config/ruffneckk-cast-triggers.toml
```

Mod-local installation:

```text
<D2R>/mods/<mod>/d2rloader/plugins/d2rl-ruffneckk-cast-triggers.dll
<D2R>/mods/<mod>/d2rloader/config/ruffneckk-cast-triggers.toml
```



## One-time mod setup

Numeric IDs in this guide are placeholders. Pick unused IDs in your own tables
and keep them stable after releasing items.

### 1. Add two rows to ItemStatCost.txt

Copy the native `item_skillonattack` row twice. Keep all of its encoding, save
and callback fields, then change only these fields:

| Field | Fixed-level proc | Source-level proc |
|---|---|---|
| `Stat` | `item_skilloncast` | `item_skilloncastsamelevel` |
| `*ID` | `<FREE_STAT_ID_1>` | `<FREE_STAT_ID_2>` |
| `itemevent1` | `doactive` | `doactive` |
| `itemeventfunc1` | `20` | `20` |
| `itemevent2` | empty | empty |
| `itemeventfunc2` | empty | empty |
| `descfunc` | `15` | `15` |
| `descstrpos` | `RuffnecKkCastOnCast` | `RuffnecKkCastOnCastSameLevel` |
| `descstrneg` | `RuffnecKkCastOnCast` | `RuffnecKkCastOnCastSameLevel` |

After copying, these inherited fields should still match the native row:

```text
Signed=1
Send Bits=7
Send Param Bits=16
fCallback=1
Encode=2
Add=190
Multiply=256
1.09-Save Bits=21
1.09-Save Add=0
Save Bits=7
Save Add=0
Save Param Bits=16
damagerelated=1
descpriority=158
advdisplay=2
*eol=0
```

### 2. Add two rows to Properties.txt

Copy the native `att-skill` row twice and change these fields:

| Field | Fixed-level property | Source-level property |
|---|---|---|
| `code` | `cast-skill` | `cast-skill-same-level` |
| `*Id` | `<FREE_PROPERTY_ID_1>` | `<FREE_PROPERTY_ID_2>` |
| `*Enabled` | `1` | `1` |
| `func1` | `11` | `11` |
| `stat1` | `item_skilloncast` | `item_skilloncastsamelevel` |
| `uiRangeType` | `7` | `7` |
| `*eol` | `0` | `0` |

### 3. Add the tooltip strings

Add these entries to an appropriate localization JSON in the consuming mod.
Replace the example numeric IDs if they collide, and add translations for the
locales the mod ships.

```json
{
  "id": 51000,
  "Key": "RuffnecKkCastOnCast",
  "enUS": "%0%% Chance to cast level %1 %2 when casting a skill"
},
{
  "id": 51001,
  "Key": "RuffnecKkCastOnCastSameLevel",
  "enUS": "%0%% Chance to cast %2 at the source skill level when casting a skill"
}
```

For `descfunc=15`, `%0` is the chance, `%1` is the encoded level and `%2` is
the target skill name. The source-level string deliberately omits `%1` because
its stored value is a marker, not the displayed skill level.

## Put a proc on an item

Use the item's ordinary property columns:

| Column | Meaning |
|---|---|
| `prop#` | `cast-skill` or `cast-skill-same-level` |
| `par#` | Target skill's numeric ID from `Skills.txt` |
| `min#` | Chance to cast, from 1 to 100 |
| `max#` | Fixed target level, or `64` for source-level mode |

`100` means the proc runs on every eligible cast. The native item-stat system
still performs the chance roll and uses its normal target-selection rules.

### Example: 25% fixed-level Fire Ball

Fire Ball is skill ID 47 in the vanilla table:

```text
prop1=cast-skill
par1=47
min1=25
max1=12
```

Result:

```text
25% Chance to cast level 12 Fire Ball when casting a skill
```

### Example: 25% Fire Ball matching the cast spell

```text
prop1=cast-skill-same-level
par1=47
min1=25
max1=64
```

If the player casts an eligible level 20 spell, the proc casts level 20 Fire
Ball. If the source spell is effectively level 31 after bonuses, the proc uses
level 31.

### Example: always cast Nova alongside Frost Nova

Nova is skill ID 48:

```text
prop1=cast-skill-same-level
par1=48
min1=100
max1=64
```

The item itself does not restrict the source to Frost Nova. If this modifier
should react only to Frost Nova, put Frost Nova's numeric skill ID in the TOML
include list shown below. Damage reductions or other balance costs still
belong in the consuming mod's item and skill data.

### Why source-level mode uses max=64

`64` is an encoding marker; it does **not** request a level 64 skill. Native
property function 11 stores the level in six bits, so 64 becomes internal
level zero. Cast Triggers replaces that zero with the successful source
spell's effective level only during its own `doactive` dispatch. Values 1
through 63 remain ordinary fixed levels.

## TOML configuration

The TOML controls which manually cast source spells may trigger item procs. It
does not contain item codes, target skills, chances or proc levels.

```toml
enabled = true

[on_cast]
# Empty means every otherwise eligible source spell.
include_skill_ids = []

# Applied after the include list.
exclude_skill_ids = []

[diagnostics]
# Enable detailed per-cast logging only while testing.
enabled = false
```

Common policies:

```toml
# Allow every eligible spell except one custom skill.
[on_cast]
include_skill_ids = []
exclude_skill_ids = [321]
```

```toml
# Allow only Frost Nova as the source spell.
[on_cast]
include_skill_ids = [44]
exclude_skill_ids = []
```

Use the actual IDs from the consuming mod's `Skills.txt`. An include entry
cannot make an attack, repeating skill or channelled skill eligible.

## What can trigger a proc?

| Source action | Supported? | Notes |
|---|---:|---|
| Successful manual player spell cast | Yes | Main supported case |
| Lightning or Chain Lightning | Yes | Their non-repeating sequence reaches the cast animation |
| Weapon attack | No | `on cast` is intentionally spell-only |
| Inferno, Arctic Blast or another repeating/channelled skill | No | Prevents a proc every channel tick |
| A skill already triggered by an item | No | Prevents proc chains |
| Monster skill | No | Player casts only |
| Interrupted or failed cast | No | The source spell must actually cast |

The source filter is intentionally conservative. A future `cast while
channeling` trigger would need its own timer, balance rules and data contract;
this property does not simulate it.

## Testing and troubleshooting

Enter `cast-triggers` in the D2RLoader console to see eligible casts,
dispatches, fixed-level procs and source-level procs. With
`diagnostics.enabled=true`, the plugin log also records the source skill and
the requested/effective level for each target or position proc.

If the plugin loads but the item never procs, check these in order:

1. The two `ItemStatCost.txt` and `Properties.txt` rows exist in the active
   mod and their numeric IDs are unique.
2. The item is actually equipped and uses the expected `prop#` slot.
3. `par#` is the target `Skills.txt` ID, not a localization key or row name.
4. `min#` is the desired chance; use 100 for a deterministic test.
5. The manually cast source spell is not excluded by TOML.
6. The source is a supported non-repeating player spell, not an attack or a
   channelled skill.

If a source-level tooltip displays 0 or 64, make sure its ItemStatCost row uses
`RuffnecKkCastOnCastSameLevel` and that the localization text omits `%1`.

If an unrelated spell is triggering the item, narrow `include_skill_ids` or
add that source ID to `exclude_skill_ids`. Item-specific source filters are not
part of version 0.1.0; the TOML policy applies to every Cast Triggers item in
the active mod.

Leave detailed diagnostics disabled during normal play.

## Saves, compatibility and removal

Items use Diablo's native stat encoding. The plugin adds no custom character
or stash payload. Removing the DLL and TOML stops the custom `doactive`
dispatch; no save migration is required, although the custom item modifiers
will remain inert while the plugin is absent.

Cast Triggers owns the central server skill-handler entry and the two native
item-skill caster entries. It does not hook EventFunc20, which remains owned by
Melee Splash in the RuffnecKk Suite. Unrelated calls are forwarded unchanged,
and same-level context is suspended while the triggered skill runs to prevent
proc chains.

## Credits

- Author: `RuffnecKk`.
- D2MOO is credited as the semantic reference for native property function 11,
  EventFunc20 and server skill-handler behavior. All runtime addresses,
  signatures and x64 ABI were independently proven against the governed D2R
  3.3 target corpus.
- D2RLoader and its PluginSDK provide the autonomous plugin runtime.
