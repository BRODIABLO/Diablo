# PD2 affix round 1 — corrected remediation

> Analysis only. No gameplay table is modified by this checkpoint.

## Result

- appended rows: **7**
- incomplete decisions: **296**
- conflicts: **5**
- approved map exclusions: **248** (plus one pre-existing exclusion = 249 total rejected rows)
- preserved conservative PD2_DELETED decisions: **83 occurrences**

The remaining conflicts are exactly four poison fields and one skill identity dependency for Iron Maiden Proc.

## Auditor corrections

- Function 19 decodes negative charge formulas instead of comparing raw negative values to item_charged_skill.
- Class selectors 0..6 are accepted in the unsigned 3-bit parameter domain.
- howl accepts the governed 0..128 scale; Wailing remains 128.
- ItemType target semantics are accepted only for helm, knif, club, amul, ring, miss, jewl and thro when their concrete coverage hashes match.
- Cardinal is now the seventh appended row; BKVince jewl coverage includes Colossal Jewel.

## Iron Maiden Proc

No active BKVince reference consumes ID 444. The only skill-domain occurrence is the reserved skills.txt row itself; every other literal 444 is an unrelated local value or row ID. The proposal is therefore separate and unapplied: replace reserved skills row 444 with the exact PD2 row and append the missing iron maiden proc skilldesc row. The required state, missiles, sound and most localization keys already exist, but `CurseMastery`, `StrIncDmgRet` and `StrIncRadiusplev` are missing. Their authoritative localization text must be imported or authored before this dependency patch can be applied.

Important semantic differences from BKVince Iron Maiden 76 include radius (par1 / Param1=17 instead of ln12) and duration (150 + 30/level instead of 300 + 60/level). No automatic 444→76 mapping is allowed.

| Field | PD2 skill 444 | BKVince skill 76 |
|---|---|---|
| skill | Iron Maiden Proc | Iron Maiden |
| id | 444 | 76 |
| charclass |  | nec |
| skilldesc | iron maiden proc | iron maiden |
| aurarangecalc | par1  | ln12 |
| reqskill1 |  | Amplify Damage |
| leftskill |  | 1 |
| manashift | 7 | 8 |
| mana | 10 | 5 |
| lvlmana | 1 | 0 |
| calc1 desc | % damage to return | Damage % Returned VS Monster |
| calc2 | (ln56) / 4 | ln56/4 |
| calc2 desc | % damage to return vs. players | % Damage To Return VS Player |
| calc3 | (ln56) / 4 | ln56/4 |
| calc3 desc | % damage to return other | % Damage To Return VS Hireling |
| calc4 |  | 0 |
| calc4 desc |  | Flat Damage Added |
| param1 | 17 | 7 |
| param1 description | radius | Radius baseline |
| param2 description | radius per level | Radius per level |
| param3 | 150 | 300 |
| param3 description | duration | Duration baseline |
| param4 | 30 | 60 |
| param4 description | additional duration/level | Duration per level |
| param5 description | % damage returned to accursed | Damage % returned baseline |
| param6 description | % additional returned/level | Damage % returned per level |
| param8 | 6 |  |
| param8 description | damage synergy |  |
| cost add | 50000 | 8000 |
| rightskill |  | 1 |
| eol |  | 0 |

The complete source rows, all 330 aligned fields, exact reference audit and proposed projected rows are stored in pd2-affixes-round1-conflict-matrix.json.

## Poison options

Formula: total damage = encoded damage × duration frames / 256; DPS = encoded damage × 25 / 256. ItemStatCost remains unchanged.

### Toxic

| Option | Values | Duration | Total damage | DPS |
|---|---:|---:|---:|---:|
| KEEP_BKVINCE | 308/125f | 5s | 150.390625 | 30.078125 |
| CUSTOM_CAPPED | 1023/50f | 2s | 199.8046875 | 99.90234375 |
| CUSTOM_TOTAL_DAMAGE_EQUIVALENT | 1000/58f | 2.32s | 226.5625 | 97.65625 |
| ITEMSTATCOST_CHANGE_DEFERRED | 1160/50f | 2s | 226.5625 | 113.28125 |

### Pestilent

| Option | Values | Duration | Total damage | DPS |
|---|---:|---:|---:|---:|
| KEEP_BKVINCE | 470/150f | 6s | 275.390625 | 45.8984375 |
| CUSTOM_CAPPED | 1023/50f | 2s | 199.8046875 | 99.90234375 |
| CUSTOM_TOTAL_DAMAGE_EQUIVALENT | 985/100f | 4s | 384.765625 | 96.19140625 |
| ITEMSTATCOST_CHANGE_DEFERRED | 1970/50f | 2s | 384.765625 | 192.3828125 |

No poison option is selected by this report.
