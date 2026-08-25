# Cast Triggers intermod data contract

Cast Triggers intentionally does not identify items in TOML. A consuming mod
adds ordinary native D2R properties to any item table that already supports
`prop#`, `par#`, `min#` and `max#` columns.

Numeric IDs below are placeholders. Choose unused IDs in your own tables and
never renumber them after release.

## 1. Add two ItemStatCost rows

Copy `item_skillonattack` twice, then change the following fields. Keep the
native encoding, save and callback fields copied from the source row.

| Field | Fixed-level row | Source-level row |
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

The copied fields should remain equivalent to the native row:

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

## 2. Add two Properties rows

Copy `att-skill` twice and change these fields:

| Field | Fixed-level property | Source-level property |
|---|---|---|
| `code` | `cast-skill` | `cast-skill-same-level` |
| `*Id` | `<FREE_PROPERTY_ID_1>` | `<FREE_PROPERTY_ID_2>` |
| `*Enabled` | `1` | `1` |
| `func1` | `11` | `11` |
| `stat1` | `item_skilloncast` | `item_skilloncastsamelevel` |
| `uiRangeType` | `7` | `7` |
| `*eol` | `0` | `0` |

The source-level name is deliberately explicit. There is no abbreviated
`cast-skill-src`; `source` would mean the skill the player manually cast.

## 3. Add localization

`descfunc=15` supplies three indexed values: chance `%0`, encoded level `%1`
and skill name `%2`. The source-level string deliberately omits `%1`.

Add keys like these to the consuming mod's localization JSON, using unused
numeric IDs and translating every locale the mod ships:

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

The example localization IDs are not reserved by the plugin. Replace them if
they collide with the consuming mod.

## 4. Put the property on an item

The native item columns keep their usual meaning:

- `par#`: target skill ID from `Skills.txt`;
- `min#`: chance from 1 to 100;
- `max#`: fixed target-skill level, or the reserved marker below.

Fixed level 12 Fire Ball at 25% chance (`Fire Ball` skill ID 47):

```text
prop1=cast-skill
par1=47
min1=25
max1=12
```

Fire Ball at the successful source skill's effective level:

```text
prop1=cast-skill-same-level
par1=47
min1=25
max1=64
```

`max=64` is an encoding marker, not target level 64. Native property function
11 masks the level to six bits, so 64 becomes internal level zero. Cast
Triggers replaces that zero only inside its own `doactive` dispatch. Values
from 1 through 63 remain ordinary fixed levels.

For an always-on double-Nova item, use `min=100`, `par=48` (`Nova`) and either
a fixed `max` or `max=64` with `cast-skill-same-level`. Any intended damage
penalty still belongs in the consuming mod's item/skill balance data.

## Stable data rules

- The consuming mod owns and freezes all numeric IDs.
- Do not reuse either stat for a different event or encoding later.
- Keep `doactive` limited to EventFunc20 for this contract.
- Keep the property name, stat name and localization key stable after items
  have shipped.
- Do not put item codes or item mappings in the plugin TOML.
- Test new custom source skills. If one should not proc, add its numeric ID to
  `on_cast.exclude_skill_ids`.
