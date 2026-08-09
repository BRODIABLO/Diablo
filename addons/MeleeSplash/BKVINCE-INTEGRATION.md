# BKVince integration

BKVince consumes the public `MeleeSplash.dll` through a separate, default-off
host profile. The public DLL contains no BKVince name, path, skill ID, or stat
ID.

## Reserved namespace

The two new stats are append-only BKVince reservations. Their runtime IDs must
never be reassigned after publication.

| Purpose | Stat | Stat ID | Property | Property ID | String key | String ID |
|---|---|---:|---|---:|---|---:|
| Existing distribution gate | `item_splashonhit` | 384 | `splash` | 302 | `splash3` | 28301 |
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

It remains `enabled=false` after packaging and selects gate stat 384, radius
stat 391, and damage stat 392. The public configuration keeps all three IDs at
`-1` and does not require a gate.

## Reversible legacy neutralization

The historical player splash is carried by EventFunc20 token stat 384/layer
430, which invokes skill 430 and missile 743. BKVince opts into suppressing
only that exact token, only for player attackers, and only after the new plugin
is fully operational.

No integration edit is made to:

- `item_splashonhit` or property `splash`;
- skill 430 `Splash` or skill 432 `Summon Splash`;
- missile 743 `proc_splashdamage`;
- state 242 `splashdamage`;
- `Titan's Echo` or any other item row;
- the distinct, currently unassigned stat 379 `hit_skill_splash`.

This preserves the legacy non-player `Summon Splash` path and makes rollback
atomic. With the global plugin switch disabled, an invalid configuration, a
signature or ownership failure, or the DLL removed, EventFunc20 remains
pass-through. A cold restart then restores the historical player skill/missile
path without a TXT rollback. The two reserved, unassigned stats may remain in
the tables permanently.

## Governed deployment

Repository files deploy to these mod-local runtime paths:

```text
data-BKVince/d2rloader/plugins/MeleeSplash.dll
  -> <D2R>/mods/BKVince/d2rloader/plugins/MeleeSplash.dll

data-BKVince/d2rloader/config/MeleeSplash.json
  -> <D2R>/mods/BKVince/d2rloader/config/MeleeSplash.json

data-BKVince/BKVince.mpq/data/global/excel/{itemstatcost,properties}.txt
  -> <D2R>/mods/BKVince/BKVince.mpq/data/global/excel/

data-BKVince/BKVince.mpq/data/local/lng/strings/item-modifiers.json
  -> <D2R>/mods/BKVince/BKVince.mpq/data/local/lng/strings/
```

Do not deploy `skills.txt`, `missiles.txt`, `states.txt`, or
`uniqueitems.txt` for this integration. They are deliberately outside its
owned delta.

Run the targeted byte-safety check before deployment:

```text
npm run test:bkvince-melee-splash-stats
```

The check requires CRLF plus final EOL for both TSVs, exact row widths and
append positions, unique ownership of the two stat IDs, two property IDs, and
two string IDs, preserved UTF-8 BOM plus LF for the localization file, and the
exact default-off BKVince profile.
Runtime synchronization and smoke testing remain separate governed steps.
