# Cast Triggers 0.1.0

Cast Triggers is an autonomous RuffnecKk D2RLoader Suite plugin for public
intermod use.

Cast Triggers lets a modder put Path of Exile-style spell procs directly on
Diablo II: Resurrected items:

```text
25% Chance to cast level 12 Fire Ball when casting a skill
25% Chance to cast Fire Ball at the source skill level when casting a skill
100% Chance to cast Nova at the source skill level when casting a skill
100% Chance to cast level 12 Fire Ball on Critical Strike
100% Chance to cast level 12 Nova on Crushing Blow
100% Chance to cast level 12 Frost Nova on Open Wounds
100% Chance to cast level 12 Fire Ball on Attack Attempt
```

The DLL does not use a D2R build-name or version allowlist. It logs the
observed runtime identity, then requires 27 strict native fingerprint checks
before installing any hook. A mismatched build is refused safely; an unnamed
build may load only when every required native witness matches. Official
runtime qualification targets D2R 3.3.93847; D2R 3.2.92777 is covered by the
governed byte-exact equivalence of every native surface used.

Once a mod adds the reusable
properties described below, they can be placed on any item row that
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

## How the triggered skill is aimed

Cast Triggers does not contain a list of special-case skill IDs. It captures
the authoritative server input before the player mode is finalized: either the
unit type/GUID identified by the input packet or the exact X/Y ground position
supplied by that packet. The bounded per-player record survives until the
matching successful source handler consumes it, which also covers handlers
such as War Cry that do not read a target helper. A channel retains that native
descriptor for later ticks and refreshes it when another input arrives. The
handler's own target-unit or paired first-point access remains a fallback.
Item flags, cursor state and coordinate comparisons do not guess the
descriptor.

The triggered skill is then sent through exactly one native route:

1. the original unit-target caster for a native unit input;
2. the original position caster for a native ground-position input;
3. the original self/target call when neither the input nor the channel fallback
   supplies a target.

Native `ItemTgtDo` flag-0 calls remain untouched, so native self/caster item
semantics keep their original route. Cast Triggers does not synthesize a facing
direction, rewrite the player's path, retry against arbitrary targets or move a
chosen ground point. Directional missiles therefore consume the same unit or
ground input that drove the source cast, while the triggered skill's own
native item-effect code remains responsible for self-centering behavior.
Teleport triggering Frost Nova is a useful test case, but no Teleport, Frost
Nova, Battle Orders or Taunt ID is embedded in the DLL.

## Combat trigger meanings

- **Critical Strike** means a successful native weapon-mastery or passive
  Critical roll. Deadly Strike is deliberately excluded even though D2R merges
  both outcomes into the same later damage flag.
- **Crushing Blow** and **Open Wounds** trigger only after their native chance
  callback reports that the effect was applied.
- **Cast on Attack Attempt** is a separate Cast Triggers family. It runs after
  D2R accepts a player attack input and before hit resolution, so Shift-ground
  attacks and misses remain eligible. It recognizes the native player attack
  animation families (`A1`, `A2`, throw, kick and `S1` through `S4`) plus
  sequences that transition into one of those families; no skill ID is
  embedded in the DLL.
- Native `att-skill` and `hit-skill` remain untouched and keep their original
  game behavior.

Combat-triggered skills use the accepted attack's native unit or ground target.
A thread-local execution guard prevents any item-triggered skill from feeding
a second Cast Triggers proc chain.

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

### 1. Add six rows to ItemStatCost.txt

Copy the native `item_skillonattack` row six times. Keep all of its encoding, save
and callback fields, then change only these fields:

| Purpose | `Stat` | `*ID` | String key |
|---|---|---:|---|
| Fixed-level cast-on-cast | `item_skilloncast` | `<FREE_STAT_ID_1>` | `RuffnecKkCastOnCast` |
| Source-level cast-on-cast | `item_skilloncastsamelevel` | `<FREE_STAT_ID_2>` | `RuffnecKkCastOnCastSameLevel` |
| Cast on Critical Strike | `item_skilloncritical` | `<FREE_STAT_ID_3>` | `RuffnecKkCastOnCritical` |
| Cast on Crushing Blow | `item_skilloncrushingblow` | `<FREE_STAT_ID_4>` | `RuffnecKkCastOnCrushingBlow` |
| Cast on Open Wounds | `item_skillonopenwounds` | `<FREE_STAT_ID_5>` | `RuffnecKkCastOnOpenWounds` |
| Cast on Attack Attempt | `item_skillonattackattempt` | `<FREE_STAT_ID_6>` | `RuffnecKkCastOnAttackAttempt` |

For all six rows, set `itemevent1=doactive`, `itemeventfunc1=20`, clear
`itemevent2` and `itemeventfunc2`, keep `descfunc=15`, and use the listed
string key for both `descstrpos` and `descstrneg`.

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

### 2. Add six rows to Properties.txt

Copy the native `att-skill` row six times. Keep `*Enabled=1`, `func1=11`,
`uiRangeType=7` and `*eol=0`, then use these values:

| Purpose | `code` | `*Id` | `stat1` |
|---|---|---:|---|
| Fixed-level cast-on-cast | `cast-skill` | `<FREE_PROPERTY_ID_1>` | `item_skilloncast` |
| Source-level cast-on-cast | `cast-skill-same-level` | `<FREE_PROPERTY_ID_2>` | `item_skilloncastsamelevel` |
| Cast on Critical Strike | `cast-skill-on-crit` | `<FREE_PROPERTY_ID_3>` | `item_skilloncritical` |
| Cast on Crushing Blow | `cast-skill-on-cb` | `<FREE_PROPERTY_ID_4>` | `item_skilloncrushingblow` |
| Cast on Open Wounds | `cast-skill-on-ow` | `<FREE_PROPERTY_ID_5>` | `item_skillonopenwounds` |
| Cast on Attack Attempt | `cast-skill-on-attack` | `<FREE_PROPERTY_ID_6>` | `item_skillonattackattempt` |

The property codes are consumer-owned names. You may rename them, but the
`stat1` names must continue to match the ItemStatCost rows.

Put `<FREE_STAT_ID_3>` through `<FREE_STAT_ID_6>` into their four matching
`[combat_triggers]` TOML settings. The first two cast-on-cast stat IDs do not go
in the TOML because the native event dispatcher discovers them directly.

### 3. Add the tooltip strings

Merge these entries into a localization table that D2R actually loads, normally
`data/local/lng/strings/item-modifiers.json` in the consuming mod. Do not put
them in an arbitrarily named JSON file: D2R ignores unknown string-table
filenames and displays `An Evil Force` instead. Replace the example numeric IDs
if they collide, and add translations for the locales the mod ships.

```json
{
  "id": 51000,
  "Key": "RuffnecKkCastOnCast",
  "enUS": "%d%% Chance to cast level %d %s when casting a skill"
},
{
  "id": 51001,
  "Key": "RuffnecKkCastOnCastSameLevel",
  "enUS": "%d%% Chance to cast %.*s at the source skill level when casting a skill"
},
{
  "id": 51002,
  "Key": "RuffnecKkCastOnCritical",
  "enUS": "%d%% Chance to cast level %d %s on Critical Strike"
},
{
  "id": 51003,
  "Key": "RuffnecKkCastOnCrushingBlow",
  "enUS": "%d%% Chance to cast level %d %s on Crushing Blow"
},
{
  "id": 51004,
  "Key": "RuffnecKkCastOnOpenWounds",
  "enUS": "%d%% Chance to cast level %d %s on Open Wounds"
},
{
  "id": 51005,
  "Key": "RuffnecKkCastOnAttackAttempt",
  "enUS": "%d%% Chance to cast level %d %s on Attack Attempt"
}
```

This is a one-time setup per trigger type, not per item. The `%d` and `%s`
placeholders automatically print each item's chance, skill level and skill name,
so a modder can reuse the same six keys for every Cast Triggers item.

For `descfunc=15`, the native formatter receives the chance, encoded level and
target skill name in that order. The source-level string uses `%.*s`: the
reserved level `63` becomes the maximum printed skill-name length and is
therefore consumed without being displayed. D2R skill names fit inside that
limit.

## Put a proc on an item

Use the item's ordinary property columns:

| Column | Meaning |
|---|---|
| `prop#` | `cast-skill` or `cast-skill-same-level` |
| `par#` | Target skill's numeric ID from `Skills.txt` |
| `min#` | Chance to cast, from 1 to 100 |
| `max#` | Fixed target level from 1 to 62, or `63` for source-level mode |

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
max1=63
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
max1=63
```

The item itself does not restrict the source to Frost Nova. If this modifier
should react only to Frost Nova, put Frost Nova's numeric skill ID in the TOML
include list shown below. Damage reductions or other balance costs still
belong in the consuming mod's item and skill data.

### Example: always cast Fire Ball on Critical Strike

```text
prop1=cast-skill-on-crit
par1=47
min1=100
max1=12
```

The same fixed-level encoding applies to `cast-skill-on-cb` and
`cast-skill-on-ow`. Version 0.1.0 does not apply the source-level marker to
combat triggers.

### Example: cast Fire Ball when an attack is attempted

Use the custom Cast Triggers property, not native `att-skill`:

```text
prop1=cast-skill-on-attack
par1=47
min1=100
max1=12
```

This rolls as soon as D2R accepts any player attack-family input. It therefore
supports direct targets, Shift-ground attacks and misses. Native `att-skill`
and `hit-skill` remain separate properties with unchanged game behavior.

### Why source-level mode uses max=63

`63` is a reserved positive marker; it does **not** request a level 63 skill.
Cast Triggers replaces it with the successful source spell's effective level
only during its own `doactive` dispatch. A positive marker keeps native
`descfunc=15` tooltips visible; the earlier zero marker produced by `max=64`
could make the complete stat line disappear. Fixed-level Cast Triggers
properties therefore use levels 1 through 62.

If you built an earlier 0.1.0 test item with `max=64`, change the data row to
`max=63` and create the item again. The plugin remains version 0.1.0 because
that earlier archive was a release candidate, not a public release.

## TOML configuration

The TOML controls which manually cast source spells may trigger item procs and
maps the four consumer-owned combat ItemStatCost IDs. It does not contain item
codes, target skills, chances or proc levels.

```toml
enabled = true

[on_cast]
# Empty means every otherwise eligible non-repeating source spell.
include_skill_ids = []

# Applied after the include list.
exclude_skill_ids = []

[while_channeling]
# Repeating/channelled spells use the same item properties as ordinary casts.
enabled = true

# 25 server frames = 1 second. The default is exactly 2 seconds.
interval_frames = 50

# Empty means every otherwise eligible repeating/channelled source spell.
include_skill_ids = []
exclude_skill_ids = []

[combat_triggers]
# Replace 0 with the consumer-owned ItemStatCost ID to enable a family.
attack_attempt_stat_id = 0
critical_strike_stat_id = 0
crushing_blow_stat_id = 0
open_wounds_stat_id = 0

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

Use the actual IDs from the consuming mod's `Skills.txt`. The two include and
exclude lists are independent. Neither section can make a weapon attack
eligible.

Repeating/channelled sources use the same `cast-skill` and
`cast-skill-same-level` item properties as ordinary casts. Their first
successful server tick gets an immediate native chance roll. Further rolls are
throttled by `interval_frames`; with the default `50`, a held Inferno can roll
again no more often than once every two seconds. Releasing the channel stops
the native handler and therefore stops all further rolls. No wall-clock timer
or background thread continues after the skill ends.

The four nonzero combat IDs must be distinct. They select which EventFunc20 row
belongs to each combat outcome; they do not identify an item, target skill,
chance or skill level. Ordinary cast-on-cast dispatch hides these four rows,
and a combat dispatch exposes only its matching row.

## What can trigger a proc?

| Source action | Supported? | Notes |
|---|---:|---|
| Successful manual player spell cast | Yes | Main supported case |
| Lightning or Chain Lightning | Yes | Their non-repeating sequence reaches the cast animation |
| Weapon attack as a cast-on-cast source | No | `on_cast` is intentionally spell-only |
| Accepted player attack attempt | Yes | Custom `cast-skill-on-attack`; direct target, Shift-ground and miss are eligible |
| Successful Critical Strike | Yes | Weapon-mastery or passive Critical; Deadly Strike is excluded |
| Successful Crushing Blow | Yes | Native outcome must be applied |
| Successful Open Wounds | Yes | Native outcome must be applied |
| Inferno, Arctic Blast or another repeating/channelled spell | Yes | Immediate first roll, then the configured server-frame cadence |
| A skill already triggered by an item | No | Prevents proc chains |
| Monster skill | No | Player casts only |
| Interrupted or failed cast | No | The source spell must actually cast |

The source filter remains animation-based and generic. The native `repeat` flag
selects the channel cadence. Repeating cast-mode records and repeating sequence
records whose transition remains in sequence mode are supported, so Inferno
does not require a hardcoded skill ID.

## Testing and troubleshooting

Enter `cast-triggers` in the D2RLoader console to see eligible casts,
dispatches, channel ticks, channel dispatches, throttled channel ticks, input
captures, input consumptions, expired inputs, reused channel targets,
fixed-level procs, source-level procs, native-position routes and
native-unit-target routes, combat outcomes, filtered combat stats, suppressed
combat chains and Critical-marker overflow. The command prints separate
summary, cadence/input and combat lines so D2RLoader does not clip the final
Critical counters. With `diagnostics.enabled=true`, hot hooks retain the latest
64 detailed events in a bounded in-memory trace instead of writing synchronously
during combat. Running `cast-triggers` then emits that retained trace, including
source skills, requested/effective levels, captured targets and native routes.
Older entries are overwritten when the buffer is full. The public TOML defaults
to `false`.

If the plugin loads but the item never procs, check these in order:

1. The required `ItemStatCost.txt` and `Properties.txt` rows exist in the active
   mod and their numeric IDs are unique.
2. The item is actually equipped and uses the expected `prop#` slot.
3. `par#` is the target `Skills.txt` ID, not a localization key or row name.
4. `min#` is the desired chance; use 100 for a deterministic test.
5. The manually cast source spell is not excluded by TOML.
6. The source uses the player cast animation and is not a weapon attack.
7. For a repeating/channelled source, `while_channeling.enabled` is true and
   its skill ID passes that section's include/exclude policy.
8. For a combat trigger, its ItemStatCost `*ID` exactly matches the TOML and
   the item also has the native Critical, Crushing Blow or Open Wounds chance
   needed to produce that outcome.

If a source-level tooltip is missing, make sure the item uses `max=63`, its
ItemStatCost row uses `RuffnecKkCastOnCastSameLevel`, and the localization text
uses `%d%% ... %.*s` so all three native formatter arguments are consumed.
Recreate any item that was generated with the obsolete `max=64` test marker.

If an unrelated spell is triggering the item, narrow `include_skill_ids` or
add that source ID to `exclude_skill_ids`. Item-specific source filters are not
part of version 0.1.0; the TOML policy applies to every Cast Triggers item in
the active mod.

Leave detailed diagnostics disabled during normal play.

## Release validation

Version 0.1.0 was qualified on the official D2R 3.3.93847 runtime with the
complete active RuffnecKk Suite stack and all five eezstreet plugins loaded.
D2R 3.2.92777 is covered by governed byte-exact equivalence for every native
surface used by this DLL. Other builds are not accepted by name: they load only
if the complete fail-closed native fingerprint matches.

## Saves, compatibility and removal

Items use Diablo's native stat encoding. The plugin adds no custom character
or stash payload. Removing the DLL and TOML stops the custom `doactive`
dispatch; no save migration is required, although the custom item modifiers
will remain inert while the plugin is absent.

Cast Triggers owns the central server skill-handler, the two common player-skill
input executors, unit-stat dispatcher, EventFunc15/16/20, damage
builder/copy/destructor, target observers and the two native item-skill caster
entries. Unrelated calls are forwarded unchanged.
Synthetic filtering exists only while Cast Triggers dispatches its own event,
and all item-skill execution suspends source context while the recursion guard
is active. The public DLL has no BKVince, BKVCombat or Melee Splash dependency.

## Credits

- Author: `RuffnecKk`.
- D2MOO is credited as the semantic reference for native property function 11,
  EventFunc20 and server skill-handler behavior. All runtime addresses,
  signatures and x64 ABI were independently proven against the governed native
  corpus shared by D2R 3.2.92777 and D2R 3.3.93847.
- D2RLoader and its PluginSDK provide the autonomous plugin runtime.
