# Burn Damage Fix

Burn Damage Fix restores generic Burn damage production and makes applied Burn
respect the game's live Fire Resistance rules while keeping a moving flame on
the affected target.

The plugin has no Diablo II: Resurrected build or version allowlist. It records
the observed build name for diagnostics only, then validates every native RVA,
signature, layout, ABI witness and hook surface it uses. A matching unnamed or
future build may load; any fingerprint difference refuses safely before the
first hook is installed.

The plugin corrects two native defects and one related visual issue:

- generic Burn producers use `burningmin`, `burningmax` and
  `passive_fire_mastery` instead of adding the numeric stat ID `316` as flat
  damage;
- Burn is resolved through the current Fire Resistance path before the
  resulting negative life regeneration is stored;
- the target-bound `fire_hit` effect is replayed while the real native
  `burning` state remains active, and the stationary native ground flame is
  suppressed process-locally.

Resistance, maximum resistance, difficulty, immunity and attacker Fire pierce
remain native. Magic Damage Reduction and Fire Absorb are intentionally
excluded. Burn duration, tick scheduling, saves, packets, kill attribution and
experience are not changed.

The overlay replacement never creates or extends Burn. It only observes the
native `burning` state, replays `fire_hit` on the affected unit, and suppresses
the original stationary overlay when that row still contains vanilla ID `224`.
A pre-suppressed row is accepted and any custom overlay ID is preserved.

## Installation

Remove the historical `BurnFireResistance.dll` first. Never load it together
with `BurnDamageFix.dll`.

Install `BurnDamageFix.dll` in either:

- `<D2R>/d2rloader/plugins/`; or
- `<D2R>/mods/<mod>/d2rloader/plugins/`.

Place `burn-damage-fix.toml` in the matching `d2rloader/config/` directory.
The embedded defaults are safe if the file is absent. Mod-local configuration
takes priority over scope-local and global configuration.

The public configuration intentionally exposes only the master switch and
diagnostic counters. Generic Burn normalization, Fire Resistance, the moving
`fire_hit` replay at ten frames and stationary-flame suppression are the fixed
plugin behavior rather than optional gameplay variants.

The console command `burn-damage-fix` reports the observed build,
configuration, suppression state and optional diagnostic counters.

## Adding Burn Damage to a Skill

Burn Damage Fix does not automatically add Burn damage to skills. It fixes the
engine so that Burn created by your mod works correctly, respects Fire
Resistance and displays a moving fire effect.

This tutorial uses Fire Ball as an example, but the same principle applies to
other skills.

### What we are building

A skill can only use one elemental type in its normal damage fields. Changing
Fire Ball from `fire` to `burn` would replace its Fire damage.

To keep the original damage and add Burn, the impact must produce two separate
server damage effects:

```text
Fire Ball impact
|-- Fire damage missile -- deals the original instant Fire damage
`-- Burn damage missile -- applies the additional Burn DoT
```

Both damage missiles are invisible and processed only by the server. The player
still sees the normal Fire Ball and its normal explosion.

### Files used

The gameplay change uses:

```text
data/global/excel/missiles.txt
```

The optional tooltip change also uses:

```text
data/global/excel/skilldesc.txt
data/local/lng/strings/skills.json
```

You normally do not need to change the skill's original elemental damage in
`skills.txt`.

### Step 1 -- Create the Burn damage missile

Open `missiles.txt`.

Duplicate a simple server explosion missile such as `explodingarrowexp2`. Add
the duplicate as a new row at the end of the table, then give it a unique name
and the next unused `*ID`.

For Fire Ball, use a name such as:

```text
fireball_burn_hit
```

Configure these fields:

| Field | Example | Purpose |
|---|---:|---|
| `Missile` | `fireball_burn_hit` | Unique missile name |
| `pSrvDoFunc` | `1` | Processes the missile on the server |
| `pSrvHitFunc` | `1` | Deals radial elemental damage |
| `sHitPar1` | `4` | Damage radius |
| `Range` | `1` | Applies the effect immediately |
| `CelFile` | `null` | Keeps the missile invisible |
| `CollideType` | `3` | Uses normal enemy collision |
| `CollideKill` | `1` | Removes the missile after use |
| `AlwaysExplode` | `1` | Processes the hit when it expires |
| `ResultFlags` | `5` | Standard elemental hit result |
| `HitFlags` | `2` | Standard elemental damage hit |
| `MissileSkill` | blank | Uses its own Burn values |
| `Skill` | blank | Does not copy the original skill damage |
| `EType` | `burn` | Produces Burn damage |
| `EMin` | see below | Minimum Burn damage rate |
| `EMax` | see below | Maximum Burn damage rate |
| `ELen` | `500` | Duration: 500 frames = 20 seconds |
| `HitShift` | see below | Damage scaling |

Leave client-only fields such as `CltHitSubMissile` blank. The hidden missile
should not create its own visual effect. Burn Damage Fix provides the fire
effect on the affected target.

### Step 2 -- Choose the Burn damage and duration

D2R runs at 25 frames per second:

```text
ELen = seconds x 25

2 seconds  = 50 frames
5 seconds  = 125 frames
20 seconds = 500 frames
```

For Burn, `EMin` and `EMax` describe a damage rate. They are not the final total
displayed in the tooltip.

The total is calculated as follows:

```text
Total Burn Damage =
    EMin or EMax x 2^HitShift x ELen / 256
```

For example, use these values to deal approximately 35--53 Burn Damage over
20 seconds:

```text
HitShift = 0
EMin     = 18
EMax     = 27
ELen     = 500
```

The resulting totals are:

```text
Minimum: 18 x 1 x 500 / 256 = 35.15
Maximum: 27 x 1 x 500 / 256 = 52.73
```

The `HitShift` multipliers are:

| `HitShift` | Multiplier |
|---:|---:|
| 8 | 256 |
| 7 | 128 |
| 6 | 64 |
| 5 | 32 |
| 4 | 16 |
| 3 | 8 |
| 2 | 4 |
| 1 | 2 |
| 0 | 1 |

Use lower `HitShift` values when you need precise control over long-duration
Burn damage.

Do not use `HitShift=8` simply because you want the listed values unchanged.
For example:

```text
EMin     = 36
HitShift = 8
ELen     = 500
```

This would produce approximately 18,000 total Burn damage, not 36.

### Step 3 -- Move the original Fire damage into a hidden missile

Fire Ball normally deals its radial Fire damage directly through its server hit
function. That function cannot also create the additional Burn effect.

Duplicate `explodingarrowexp2` again and create another hidden server missile:

```text
fireball_fire_hit
```

Give it its own next unused `*ID`, then configure it to use Fire Ball's original
damage:

```text
Missile          = fireball_fire_hit
pSrvDoFunc       = 1
pSrvHitFunc      = 1
sHitPar1         = 4
Range            = 1
CelFile          = null
CollideType      = 3
CollideKill      = 1
AlwaysExplode    = 1
MissileSkill     = 1
Skill            = Fire Ball
EType            = fire
HitShift         = 7
ResultFlags      = 5
HitFlags         = 2
```

`MissileSkill=1` and `Skill=Fire Ball` make this hidden missile use Fire Ball's
existing Fire damage, skill level and scaling. Use the original skill's
`HitShift` value. Fire Ball currently uses `7`.

### Step 4 -- Apply both effects when Fire Ball hits

Find the original `fireball` row in `missiles.txt`.

Change its server hit behavior:

```text
pSrvHitFunc = 4
```

The original radius is now controlled by the two hidden damage missiles, so
clear:

```text
sHitPar1 = blank
```

Connect both server damage effects:

```text
HitSubMissile1 = fireball_fire_hit
HitSubMissile2 = fireball_burn_hit
```

Do not use `CltHitSubMissile` for gameplay damage. Fields beginning with `Clt`
are client-side visual effects.

Keep Fire Ball's existing visual fields unchanged:

```text
pCltHitFunc
CltHitSubMissile1
HitSound
ExplosionMissile
```

The completed impact now behaves like this:

```text
Visible Fire Ball hits
|-- fireball_fire_hit deals the original Fire damage
|-- fireball_burn_hit applies the additional Burn effect
`-- the original Fire Ball explosion remains visible
```

### Step 5 -- Display Burn damage in the tooltip

This step is optional. Burn Damage Fix does not modify skill descriptions
automatically. The tooltip must read the values from the hidden Burn missile.

#### Add the Burn Damage text

Open:

```text
data/local/lng/strings/skills.json
```

Duplicate an existing skill string entry. Give it a unique unused ID and use
this key:

```json
{
  "id": 99999,
  "Key": "StrSkillBurnDamage",
  "enUS": "Burn Damage: %d-%d"
}
```

Replace `99999` with an unused ID. Preserve every language field from the
duplicated entry and keep the JSON structure valid. English can temporarily be
used for languages that have not been translated yet.

#### Add the damage and duration lines

Open `skilldesc.txt` and find the skill's `skilldesc` row. Fire Ball currently
has tooltip slots 4, 5 and 6 available. Configure slots 4 and 5:

| Field | Value |
|---|---|
| `descline4` | `75` |
| `desctexta4` | `StrSkillBurnDamage` |
| `desccalca4` | `miss('fireball_burn_hit'.edns)*miss('fireball_burn_hit'.edln)/256` |
| `desccalcb4` | `miss('fireball_burn_hit'.edxs)*miss('fireball_burn_hit'.edln)/256` |
| `descline5` | `36` |
| `desctexta5` | `StrSkillPoisonLengthSingular` |
| `desctextb5` | `StrSkill63` |
| `desccalca5` | `miss('fireball_burn_hit'.edln)` |
| `desccalcb5` | `25` |

The first line calculates the total Burn damage over the entire duration. The
second line converts the duration from frames to seconds and automatically
selects the singular or plural text.

Depending on the skill level and the values chosen above, the tooltip will show
the original Fire damage followed by the additional Burn damage and duration:

```text
Fire Damage: [original Fire Ball damage]
Burn Damage: 35-53
over 20 seconds
Mana Cost: [current Fire Ball mana cost]
```

These are raw values before the target's defenses are applied. Actual Burn
damage depends on Fire Resistance, Fire Immunity and applicable Fire pierce.

### Applying the same method to other skills

The general principle is always:

```text
keep the original damage
+ create a separate hidden Burn missile
+ create it from the real server damage event
+ optionally display its values through skilldesc.txt
```

For projectile and impact skills, the real damage event is usually found in the
skill's server missile and its `pSrvHitFunc`.

If the server missile already creates `HitSubMissile` entries, place the hidden
Burn missile in an unused server `HitSubMissile` slot.

If the skill deals damage directly and does not create server hit missiles,
move its original damage into a hidden damage missile first, as shown for Fire
Ball.

Persistent fields, novas, melee attacks, auras and channelled skills may use
different server functions. Always attach the additional Burn effect to the
missile or event that actually deals gameplay damage, not to a client-side
visual effect.

Rapidly repeating skills may reapply and refresh the Burn duration on every
hit.

## Compatibility contract

Burn Damage Fix owns only the internal generic-production seam `0x44CB32` and
the Burn-application entry `0x451380`, plus the player and monster event
dispatchers used for periodic overlay replay. It deliberately does not own the
shared resolver at `0x4523E0`; it calls the live address so Monster Display and
Resistance Floor remain in the chain regardless of load order.

The D2RLoader API version check is retained because it validates the loader ABI,
not the Diablo build. Removing that check could interpret an incompatible
plugin context layout and crash before the native fingerprint can be evaluated.

The fixed mechanics shipped in Version 1.0.0 were gameplay-qualified on the
official D2R 3.3.93847 runtime. D2R 3.2.92777 is covered by governed
byte-identical native surfaces; this evidence coverage does not restrict
loading to either named version.

Static ownership checks found no overlap with Bind And Summon, Melee Splash,
the five eezstreet PluginPack DLLs, or the active RuffnecKk Suite. See
`NATIVE-HOOKS.md` and `Mission/burn-damage-fix.md` for the governed evidence and
the runtime matrix.

The independent `ruffneckk-thorns-burn-kill-credit.json` memory patch remains
responsible for kill attribution and experience credit.

## Release candidate 1.0.0

- DLL size: 189,440 bytes
- DLL SHA-256:
  `F2E811E5EC2823616A3418604E657E7ABEA1D401937802342A8710972040703E`
- TOML SHA-256:
  `287802A7356272E47928765E0E88001AB8BCB623B55A0891C9DF40B3219A40A5`
- ZIP SHA-256:
  `25BB4FE4426DAB101F7AF664C99D85796405919717F25901E3E67BD3218C296E`
- ZIP contents: `BurnDamageFix.dll`, `burn-damage-fix.toml`

The ZIP stays beside this README and does not contain the README. Runtime and
gameplay evidence and any remaining release gates are tracked in the mission.

## Credits

Authored by RuffnecKk. D2MOO provided the pinned semantic reference used to
identify Burn stat production, native random-range behavior and Burn state
application. No D2MOO address, structure or 32-bit ABI is reused.
