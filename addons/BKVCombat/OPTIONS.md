# BKVCombat configuration

`BKVCombat.json` is strict JSON. Unknown keys, duplicate logical identities,
wrong types, invalid schema versions, missing MajorBoss entries, and active
`monstats.txt` key/ID mismatches fail closed before any hook is installed.

## General fields

| Field | Type | Default | Meaning |
|---|---|---:|---|
| `schemaVersion` | integer | `2` | Configuration schema; only version 2 is accepted. |
| `enabled` | boolean | `false` | Master switch. `false` installs no hooks. |
| `diagnosticLogging` | boolean | `false` | Logs one bounded activation summary; it does not enable per-hit spam. |

## Independent policy toggles

The `policies` object contains six booleans:

| Field | Effect when enabled |
|---|---|
| `criticalStrike` | Caps weapon/passive Critical at 75%; Critical remains first and 2.0x. |
| `deadlyStrike` | Caps Deadly at 75% and changes only its success multiplier to 1.5x. |
| `crushingBlow` | Uses the configured target classes, fractions, live player-count scaling, and optional CBE stat. |
| `lifeSteal` | Validates and retains the native 1/1, 1/2, 1/3 difficulty and Drain baseline; installs no leech hook. |
| `manaSteal` | Validates and retains the native 1/1, 1/2, 1/3 difficulty and Drain baseline; installs no leech hook. |
| `openWounds` | Applies the governed five-second, three-stack player-to-monster policy. |

Toggles select policies independently, but native installation is atomic. A
configuration is never allowed to leave half of its requested native seams
active.

## Configurable stats

The `stats` object contains:

| Field | Type | Default | Meaning |
|---|---|---:|---|
| `crushingBlowEfficiencyStatId` | integer | `-1` | Global plus active-weapon CBE stat. `-1` disables CBE; valid configured IDs are `0..65535`. |

The public configuration does not reserve a universal ID. BKVince uses the
collision-safe stat `393` (`item_crushingblow_efficiency`), property `312`
(`crush-efficiency`), and string `65030`. When Crushing Blow is enabled with a
configured stat ID, the active `itemstatcost.txt` row must match and must carry
`damagerelated=1`, or activation fails before any native write. Negative CBE is
treated as zero; no unproved upper cap is applied.

## MajorBoss registry

`classifications.majorBosses` contains exactly ten objects:

```json
{
  "monstats": "ubermephisto",
  "expectedId": 704
}
```

`monstats` is the non-localized `Id` key in the active `monstats.txt`.
`expectedId` is its BKVince `*hcIdx`, not a PD2 ID. Both values must match the
active table. This registry affects only the Crushing Blow fraction; it grants
no Prime Evil buff by itself.

The current BKVince Herald/Ascendant implementation uses the runtime
Herald/ghostly marker, which is classified as Elite. It is not a name list in
the configuration.

## Safe fallback

- Missing configuration: plugin loads disabled and installs no hooks.
- Present but invalid configuration: load fails before mutation.
- Enabled unsupported policy: activation plan is rejected before mutation.
- Signature/build/owner mismatch: activation fails closed.
- Late partial write: wrappers stay pass-through and the DLL remains loaded;
  remove or disable it and cold-start to restore original bytes.
