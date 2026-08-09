# BKVince integration

BKVince consumes the public `MeleeSplash.dll` through a separate active host
profile. The public DLL contains no BKVince name, path, skill ID, or stat ID.
The public package remains default-off; only the governed BKVince integration
enables it.

## Reserved namespace

The two new stats are append-only BKVince reservations. Their runtime IDs must
never be reassigned after publication.

| Purpose | Stat | Stat ID | Property | Property ID | String key | String ID |
|---|---|---:|---|---:|---|---:|
| Retired legacy ABI tombstone | `item_splashonhit` | 384 | `splash` | 302 | `splash3` | 28301 |
| Increased radius percent | `inc_splash_radius` | 391 | `splash-radius%` | 310 | `ModIncSplashRadius` | 65028 |
| Increased splash damage percent | `item_melee_splash_damage_percent` | 392 | `splash-dmg%` | 311 | `ModMeleeSplashDamagePercent` | 65029 |

Stats 391 and 392 use 16 send bits and 10 item-save bits with a zero save
offset. They are non-damage-related aggregate unit stats so every equipped item
contribution can be read by the host-configured plugin. No BKVince item is
assigned either new property in 0.1.0; itemization remains a later balancing
lot.

The governed profile is:

```text
data-BKVince/d2rloader/config/MeleeSplash.json
```

It is `enabled=true`, applies to all eligible player melee attacks without a
gate stat, and selects radius stat 391 plus damage stat 392. The public
configuration remains default-off with all host stat IDs at `-1`.

## Retired legacy splash

The historical skill/missile splash was found unsafe during its default-off
rollback witness: skill item-effect execution asserted before a visual legacy
missile could be qualified. Vincent therefore retired that system instead of
preserving it as a fallback.

The migration keeps numeric IDs stable but makes the old graph unreachable:

- stat 384 and property 302 remain decode-safe tombstones with no event or
  property function;
- skills 430/432 and missile 743 retain only their names and numeric IDs;
- every `Summon Splash` assignment is removed from skills and monsters;
- `Titan's Echo` remains at unique ID 473 but is non-spawnable and has no
  splash property; its treasure-class reference is removed;
- state 242 remains an inert name/ID tombstone;
- stat 379 `hit_skill_splash` remains distinct and untouched.

Disabling or removing `MeleeSplash.dll` now means **no melee splash**, not a
return to the old missile. This is the safe rollback. Existing saves may still
contain stat 384, but the retired row can only decode it; it cannot dispatch
EventFunc20. The migration is idempotent and never shifts a row or reuses an
ID.

## Governed deployment

Repository files deploy to these mod-local runtime paths:

```text
data-BKVince/d2rloader/plugins/MeleeSplash.dll
  -> <D2R>/mods/BKVince/d2rloader/plugins/MeleeSplash.dll

data-BKVince/d2rloader/config/MeleeSplash.json
  -> <D2R>/mods/BKVince/d2rloader/config/MeleeSplash.json

data-BKVince/BKVince.mpq/data/global/excel/{itemstatcost,properties,skills,missiles,monstats,uniqueitems,treasureclassex}.txt
  -> <D2R>/mods/BKVince/BKVince.mpq/data/global/excel/

data-BKVince/BKVince.mpq/data/local/lng/strings/item-modifiers.json
  -> <D2R>/mods/BKVince/BKVince.mpq/data/local/lng/strings/
```

`states.txt` is intentionally unchanged because state 242 was already an inert
name/ID row.

Run the targeted byte-safety check before deployment:

```text
npm run test:bkvince-melee-splash-stats
```

The check requires CRLF plus final EOL for both TSVs, exact row widths and
append positions, unique ownership of the two stat IDs, two property IDs, and
two string IDs, preserved UTF-8 BOM plus LF for the localization file, the
exact active BKVince profile, and the complete legacy tombstone/reference
retirement.
Runtime synchronization and smoke testing remain separate governed steps.
