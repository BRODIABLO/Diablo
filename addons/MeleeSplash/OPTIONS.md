# MeleeSplash configuration

The first existing `MeleeSplash.json` wins in this order: active mod config,
the DLL's scope config, then global D2RLoader config. A malformed higher-priority
file fails closed; it never silently falls through to a lower-priority file.

## Core keys

| Key | Default | Meaning |
|---|---:|---|
| `enabled` | `false` | Installs no gameplay hooks unless explicitly enabled. |
| `activationMode` | `allEligibleMelee` | `allEligibleMelee`, `whitelist`, or `blacklist`. |
| `allowNormalAttack` | `true` | Allows native normal attack skill ID `0`. |
| `includedSkillIds` | `[]` | Only these IDs pass in `whitelist` mode. |
| `excludedSkillIds` | `[]` | Always rejected, regardless of mode or override. |
| `requireGateStat` | `false` | Requires `gateStatId` to be positive on the attacker. |
| `gateStatId` | `-1` | Host-provided stat ID; `-1` disables it. |
| `increasedRadiusStatId` | `-1` | Optional host stat providing radius percent. |
| `radiusPercentPerTile` | `20` | Each complete percentage tranche adds one tile. |
| `splashDamagePercentStatId` | `-1` | Optional host stat added to splash damage percent. |
| `baseSplashDamagePercent` | `100` | Shared packet scale before per-target Critical/Deadly and defenses. |
| `baseRadiusNormalWeapon` | `4` | Radius for a normal-tier active weapon. |
| `baseRadiusExceptionalEliteWeapon` | `5` | Radius for exceptional/elite active weapons. |
| `maximumRadiusTiles` | `0` | `0` means no configured cap; native safety bounds remain. |
| `diagnosticLogging` | `false` | Enables detailed bounded local diagnostics. |

`excludedSkillIds` always wins. In `whitelist`, only `includedSkillIds` pass.
In `blacklist`, all otherwise eligible attacks pass except exclusions. Normal
attack still obeys `allowNormalAttack` in every mode.

## Skill overrides

`skillOverrides` is an object keyed by canonical decimal skill ID. Each entry
may override:

- `enabled`;
- `baseRadiusTiles`;
- `baseSplashDamagePercent`;
- `requireGateStat`.

Example:

```json
{
  "skillOverrides": {
    "0": { "baseRadiusTiles": 4 },
    "10": {
      "enabled": true,
      "baseRadiusTiles": 6,
      "baseSplashDamagePercent": 75,
      "requireGateStat": true
    }
  }
}
```

An override cannot bypass `excludedSkillIds` or `allowNormalAttack`.

## Legacy EventFunc20 suppression

This advanced host-integration block is disabled publicly:

```json
{
  "legacyEvent20Suppression": {
    "enabled": false,
    "statId": -1,
    "layer": -1,
    "playerAttackersOnly": true
  }
}
```

When explicitly enabled, both `statId` and `layer` must be exact non-negative
IDs. Only that packed EventFunc20 token is suppressed, and only while the
plugin is operational. The generic plugin never guesses another mod's IDs.
A disabled plugin, absent DLL, invalid setup, or cold rollback passes the
legacy event through unchanged.

## Invalid values

The parser rejects unknown keys, wrong JSON types, negative IDs other than the
documented `-1` sentinel, noncanonical skill keys, impossible gate
configuration, and invalid suppression tokens. Runtime radius is clamped to
`32767` tiles even when `maximumRadiusTiles` is `0`, keeping the native signed
32-bit squared-distance calculation within its safe diagonal bound.
