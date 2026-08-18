# PD2 Affixes — prévisualisation gouvernée du round 1

## Portée

- État : `PREVIEW_ONLY_APPLICATION_FORBIDDEN`
- Commande exécutée : `npm run preview:bkvince-pd2-affix-decisions -- "<checkpoint-round1>" --output="Mission/pd2-affixes-decisions-v3-round1-preview.json"`
- `reviewId` : `pd2-affixes-review-v3`
- `comparisonHash` conservé : `564C42A58959B99ED6C8BE8923B8591FDDC9645299E7CF3C034402CBC711E0D3`
- Baseline BKVince : `756df5f53109729f16643b36aa459fead4cdbf94`
- SHA-256 du checkpoint de décisions : `4B552134A044A9BD49E2094F3DE5B90CB9F22624AFE8E71194FA1BBF420CCB33`
- SHA-256 du preview : `09750DE511F4588BEE251A6FE310E9D803904CE8E7ADC6AF32CDFB1A73C40715`

Aucune décision n'a été corrigée, aucune application n'a été tentée et aucun fichier gameplay n'a été modifié.

## Verdict

| Mesure | Résultat |
|---|---:|
| `ready` | **false** |
| `changedCells` proposées | 1 013 |
| `appendedRows` proposées | 6 |
| `rejectedRows` | 1 |
| `auditedOccurrences` | 655 |
| `incomplete` | 544 |
| `conflicts` | 291 |

Les cellules proposées se répartissent entre `magicprefix.txt` (495) et `magicsuffix.txt` (518).

Les six ajouts proposés, non appliqués, sont :

- `magicprefix.txt` : Crimson (source 796), Snowflake (813), Ember (812);
- `magicsuffix.txt` : of Hypothermia (914), of Ashes (915), of Maelstrom (916).

La ligne `magicsuffix.txt:913`, **of Decay**, est rejetée conformément à `EXCLUDE_PD2_AFFIX`.

## Dépendances bloquantes et conflits

Les hashes gouvernés des sources et de la baseline ont été acceptés avant l'audit. Les dépendances compatibles existantes ne bloquent pas le preview. Les 291 conflits bloquants sont :

| Type | Nombre | Détail |
|---|---:|---|
| ItemType incompatible | 109 | `helm` 59, `knif` 20, `club` 13, `amul` 5, `ring` 4, `miss` 4, `jewl` 2, `thro` 2 |
| Limite ItemStatCost / sérialisation | 181 | 170 charges à valeur négative, 6 paramètres de classe hors plage, 1 valeur `howl`, 4 valeurs de poison hors plage |
| Skill paramétré incompatible | 1 | ID numérique 444 non stable : `Iron Maiden Proc` contre `unused_bkv_merc_skill_444` |

Répartition des conflits par catégorie : `PD2_DELETED` 167, `PD2_MODIFIED` 123 et `PD2_NEW_PORTABLE` 1.

## Localisations

Le plan contient 348 clés : 344 existent déjà de façon compatible dans les localisations moderne et legacy; 4 ajouts sont proposés dans les deux catalogues, sans conflit :

| ID proposé | Clé |
|---:|---|
| 61490 | Cardinal |
| 61491 | of Hypothermia |
| 61492 | of Ashes |
| 61493 | of Maelstrom |

## Limites de sérialisation

| Table | Lignes compilées actuelles | Ajouts proposés | Projection | Limite 11 bits |
|---|---:|---:|---:|---|
| `magicprefix.txt` | 742 | 3 | 745 | respectée |
| `magicsuffix.txt` | 794 | 3 | 797 | respectée |
| `automagic.txt` | 45 | 0 | 45 | respectée |

Le total unifié projeté est de 1 587, donc la limite `uint16` est respectée. Cette capacité structurelle n'annule pas les 181 conflits de valeurs et paramètres ItemStatCost détaillés plus haut.

## Décisions incomplètes

Toutes les 544 décisions incomplètes ont la raison `line decision missing`. Elles concernent 259 occurrences de préfixe et 285 occurrences de suffixe.

| Catégorie | Occurrences incomplètes | Familles touchées |
|---|---:|---:|
| `PD2_DELETED` | 0 | 0 |
| `PD2_MODIFIED` | 0 | 0 |
| `PD2_NEW_PORTABLE` | 193 | 104 |
| `PD2_NEW_REVIEW` | 351 | 155 |

Distribution exacte de la taille des familles incomplètes :

- `PD2_NEW_PORTABLE` : 55 familles de 1 occurrence, 21 de 2, 22 de 3, 2 de 4, 2 de 5 et 2 de 6;
- `PD2_NEW_REVIEW` : 30 familles de 1 occurrence, 112 de 2, 6 de 3, 3 de 4, 1 de 6, 1 de 8, 1 de 26 et 1 de 27.

<details>
<summary>Familles PD2_NEW_PORTABLE avec au moins 3 décisions incomplètes</summary>

| Famille | Incomplètes |
|---|---:|
| Enhanced Damage · Armor, Any Shield | 6 |
| Enhanced Damage · Missile | 6 |
| Crushing Blow · Weapon, Missile | 5 |
| Deadly Strike · Weapon, Missile | 5 |
| Piercing Attack · Missile | 4 |
| to Energy · Any Shield | 4 |
| Crushing Blow · Helm | 3 |
| Deadly Strike · Helm | 3 |
| Life after each Kill · Ring, Amulet | 3 |
| Life Replenishment · Orb, Wand | 3 |
| Life Replenishment · Staff | 3 |
| Mana Regeneration · Orb, Wand | 3 |
| Mana Regeneration · Staff | 3 |
| Physical Damage Reduction · Armor, Any Shield | 3 |
| to Attack Rating against Demons + Damage to Demons · Missile | 3 |
| to Attack Rating against Undead + Damage to Undead · Missile | 3 |
| to Cold Skill Damage · Orb | 3 |
| to Cold Skill Damage · Staff | 3 |
| to Enemy Cold Resistance · Staff | 3 |
| to Enemy Fire Resistance · Staff | 3 |
| to Enemy Lightning Resistance · Staff, Amazon Spear | 3 |
| to Enemy Poison Resistance · Javelin, Wand, Assassin Item | 3 |
| to Fire Skill Damage · Orb, Wand | 3 |
| to Fire Skill Damage · Staff | 3 |
| to Lightning Skill Damage · Orb | 3 |
| to Lightning Skill Damage · Staff | 3 |
| to Mana after each Kill · Ring, Amulet | 3 |
| to Poison Skill Damage · Wand | 3 |

</details>

<details>
<summary>Familles PD2_NEW_REVIEW avec au moins 3 décisions incomplètes</summary>

| Famille | Incomplètes |
|---|---:|
| Chance to Cast on Casting · Staff | 27 |
| Chance to Cast on Striking · Weapon | 26 |
| Open Wounds + Deep Wounds · Weapon, Missile | 8 |
| Open Wounds + Deep Wounds · Jewel | 6 |
| Map Monster Life + Map Monster Density + Player Experience Gained · Map T1, Map T2 | 4 |
| Map Monster Life + Map Monster Density + Player Experience Gained · Map T2, Map T3 | 4 |
| Map Monster Life + Map Monster Density + Player Experience Gained · Map T3, Map T4 | 4 |
| Magic Skill Damage · Wand | 3 |
| Map Area Level + Player Experience Gained + Player Magic Find and Gold Find · All Items | 3 |
| Map Monsters Gain Magic Damage + Map Monster Density + Player Magic Find and Gold Find · All Items | 3 |
| to Enemy Cold Resistance · Missile Weapon, Scepter, Crystal Swords +3 More | 3 |
| to Enemy Fire Resistance · Missile Weapon, Scepter, Crystal Swords +3 More | 3 |
| to Enemy Lightning Resistance · Javelin, Scepter, Crystal Swords +2 More | 3 |

</details>
