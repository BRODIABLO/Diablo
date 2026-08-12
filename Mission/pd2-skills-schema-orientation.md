# PD2 Skills Schema and Engine Orientation

Phase 0 analytique. Le schéma cible reste D2R 3.2 / BKVince; aucune table gameplay, ligne, colonne, ordinal ou valeur de dégâts n’est modifiée.

- Orientation hash : `019FA5F148559A4B197166075F064110127FEE9C66228D0A12A1C9DE63542E06`
- Contrat gelé : `3133A16AD42315181599DBB5DE29C4C7DAEBC5DFB7F1110639C2CEBFEA13EC6B`
- Headers : Vanilla 322, BKVince 322, PD2 256, union canonique 330.
- PD2-only utilisés : `delay`, `checkfunc`, `nocostinstate`, `general`.
- PD2-only entièrement vides : `auratgtevent`, `auratgteventfunc`, `passiveevent`, `passiveeventfunc`.

## Contrats mécaniques

### Cooldowns

Cadence d’exécution et délais de relance du skill.

Politique : `NO_AUTOMATIC_DELAY_TRANSLATION` — preuve : `NATIVE_UNPROVEN`.

### Fonctions moteur

Dispatch natif serveur/client; les nombres sont des sélecteurs propres à chaque moteur.

Politique : `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` — preuve : `NATIVE_UNPROVEN`.

### Courbes de dégâts

Calcul des dégâts physiques et élémentaires, leurs cinq paliers et leurs synergies.

Politique : `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` — preuve : `EXACT_FORMULA`.

### Mana

Calcul du coût réel en mana et de son évolution avec le niveau.

Politique : `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` — preuve : `EXACT_FORMULA`.

### Projectiles

Création serveur/client des missiles et résolution liée dans missiles.txt.

Politique : `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` — preuve : `NATIVE_UNPROVEN`.

### Calc et Param

Entrées génériques dont le sens dépend du callback et de la description documentaire de la ligne.

Politique : `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` — preuve : `NATIVE_UNPROVEN`.

### Auras et passifs

États, événements et statistiques appliqués par les fonctions d’aura ou passives.

Politique : `NO_PD2_HEADER_IMPORT_WITHOUT_NATIVE_PROOF` — preuve : `NATIVE_UNPROVEN`.

### Summons

Création, limite et package de skills du pet, avec dépendances pettype.txt et monstats.txt.

Politique : `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` — preuve : `NATIVE_UNPROVEN`.

### Skills déclenchés par objet

Exécution serveur/client lorsque le skill provient d’un proc, d’une charge ou d’un objet.

Politique : `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` — preuve : `NATIVE_UNPROVEN`.

### Interface et contrôles

Assignation des boutons, maintien du curseur et présentation du skill dans l’interface.

Politique : `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` — preuve : `EXACT_TABLE`.

## Bundles atomiques

| Bundle | Libellé | Protégé | Tables liées |
|---|---|:---:|---|
| `PHYSICAL_DAMAGE_CURVE` | Courbe de dégâts physiques | non |  |
| `ELEMENTAL_DAMAGE_CURVE` | Courbe de dégâts élémentaires | non |  |
| `MANA_CURVE` | Coût en mana réel | non |  |
| `DAMAGE_SYNERGIES` | Synergies de dégâts | non | skills.txt |
| `LENGTH_SYNERGIES` | Synergies de durée élémentaire | non | skills.txt |
| `AURA_RADIUS` | Rayon d’aura | non |  |
| `AURA_DURATION` | Durée d’aura ou de buff | non |  |
| `PROJECTILE_ARCHITECTURE` | Architecture des projectiles | oui | missiles.txt |
| `PROJECTILE_PHYSICS` | Physique des projectiles | oui | missiles.txt:Vel, missiles.txt:MaxVel, missiles.txt:Range, missiles.txt:Size, missiles.txt:CollideKill, missiles.txt:NextHit |
| `ITEM_TRIGGER_EXECUTION` | Exécution déclenchée par un objet | oui |  |
| `NATIVE_EXECUTION` | Exécution native serveur/client | oui |  |
| `SUMMON_PACKAGE` | Package d’invocation | oui | pettype.txt, monstats.txt |
| `PASSIVE_PACKAGE` | Package passif | oui | states.txt, itemstatcost.txt |
| `UI_ASSIGNMENT` | Assignation et contrôles d’interface | non | skilldesc.txt |
| `COOLDOWN_MODEL` | Modèle de cooldown et cadence | oui |  |

## Politiques globales

- `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` — **PENDING** — Le schéma cible conserve toutes les colonnes Vanilla D2R 3.2 et BKVince, sans suppression ni réordonnancement.
- `IGNORE_ALL_EMPTY_PD2_ONLY_COLUMNS` — **PENDING** — Une colonne propre à PD2 entièrement semanticBlank reste une preuve de schéma et ne crée aucune décision par skill.
- `NO_PD2_HEADER_IMPORT_WITHOUT_NATIVE_PROOF` — **PENDING** — Aucune colonne PD2 absente de D2R/BKVince ne peut être ajoutée sans preuve que le build D2R 3.2 la compile et la consomme.
- `NO_AUTOMATIC_DELAY_TRANSLATION` — **PENDING** — PD2 delay, D2R localdelay, globaldelay et perdelay restent des modèles distincts tant qu’une équivalence n’est pas prouvée.
- `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` — **PENDING** — Absent, null et chaîne vide restent distincts dans la preuve brute mais ne demandent aucune décision gameplay lorsqu’ils sont tous semanticBlank.
- `ITEM_ECONOMY_FIELDS_PRESERVE_BKVINCE` — **PENDING** — cost add et cost mult conservent BKVince par défaut et ne deviennent pas des décisions de balance du skill.
- `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` — **PENDING** — Les numéros de fonctions serveur/client sont propres au moteur; une divergence reste protégée ou différée sans preuve D2R 3.2.
- `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` — **PENDING** — Une décision de comportement gouverne ses cellules constitutives; les cellules individuelles restent accessibles uniquement en mode expert.

## Témoin Fire Bolt

Le modèle actuel expose 82 champs modifiés et 83 décisions. Phase 0 les ramène à 6 décisions de comportement, sans perte de preuve.

| Réduction | Champs | Justification |
|---|---:|---|
| semanticBlank | 61 | RAW_ONLY_ALL_SEMANTIC_BLANK |
| preserveD2rColumnAbsentFromPd2 | 2 | PD2_HEADER_ABSENT_PRESERVE_D2R_BKVINCE |
| vanillaHistoricalOnly | 1 | VANILLA_ONLY_HISTORICAL_DIFFERENCE |
| technicalOrDocumentary | 1 | AUTO_RESOLVED_ITEM_ECONOMY |
| bundled | 17 | PLAYER_BEHAVIOR_DIFFERS |

Bundles finaux :

- `DAMAGE_SYNERGIES` : `skills.txt:edmgsympercalc`, `skills.txt:param8`
- `ELEMENTAL_DAMAGE_CURVE` : `skills.txt:emaxlev1`, `skills.txt:emaxlev2`, `skills.txt:emaxlev3`, `skills.txt:emaxlev4`, `skills.txt:emaxlev5`, `skills.txt:eminlev1`, `skills.txt:eminlev2`, `skills.txt:eminlev3`, `skills.txt:eminlev4`, `skills.txt:eminlev5`
- `ITEM_TRIGGER_EXECUTION` : `skills.txt:itemclteffect`, `skills.txt:itemeffect`
- `MANA_CURVE` : `skills.txt:lvlmana`, `skills.txt:mana`, `skills.txt:manashift`
- `NATIVE_EXECUTION` : `missiles.txt:firebolt:psrvhitfunc`, `missiles.txt:firebolt:shitpar1`
- `PROJECTILE_PHYSICS` : `missiles.txt:firebolt:range`

## Matrice exhaustive des headers Skills.txt

| Header canonique | Vanilla | BKV | PD2 | Non vides V/B/P | Joueur V/B/P | Technique V/B/P | Classification | Portée | Politique |
|---|:---:|:---:|:---:|---:|---:|---:|---|---|---|
| `skill` | ✓ | ✓ | ✓ | 429/451/603 | 241/241/224 | 188/210/379 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `id` | ✓ | ✓ | ✓ | 429/451/603 | 241/241/224 | 188/210/379 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `charclass` | ✓ | ✓ | ✓ | 240/240/231 | 240/240/224 | 0/0/7 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `skilldesc` | ✓ | ✓ | ✓ | 283/297/384 | 241/241/224 | 42/56/160 | `UI_ONLY` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `srvstfunc` | ✓ | ✓ | ✓ | 133/134/172 | 77/77/75 | 56/57/97 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `srvdofunc` | ✓ | ✓ | ✓ | 329/346/488 | 188/194/181 | 141/152/307 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `srvstopfunc` | ✓ | ✓ | — | 2/2/0 | 1/1/0 | 1/1/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `prgstack` | ✓ | ✓ | ✓ | 3/3/6 | 3/3/5 | 0/0/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `srvprgfunc1` | ✓ | ✓ | ✓ | 2/2/4 | 2/2/3 | 0/0/1 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `srvprgfunc2` | ✓ | ✓ | ✓ | 4/4/4 | 4/4/4 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `srvprgfunc3` | ✓ | ✓ | ✓ | 4/4/6 | 4/4/5 | 0/0/1 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `prgcalc1` | ✓ | ✓ | ✓ | 5/5/11 | 5/5/5 | 0/0/6 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `prgcalc2` | ✓ | ✓ | ✓ | 3/3/2 | 3/3/2 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `prgcalc3` | ✓ | ✓ | ✓ | 4/4/6 | 4/4/5 | 0/0/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `prgdam` | ✓ | ✓ | ✓ | 5/5/7 | 5/5/6 | 0/0/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `srvmissile` | ✓ | ✓ | ✓ | 51/47/79 | 23/18/17 | 28/29/62 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `decquant` | ✓ | ✓ | ✓ | 12/4/0 | 12/4/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `lob` | ✓ | ✓ | ✓ | 4/4/2 | 2/2/0 | 2/2/2 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `srvmissilea` | ✓ | ✓ | ✓ | 119/127/235 | 60/67/72 | 59/60/163 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `srvmissileb` | ✓ | ✓ | ✓ | 32/39/92 | 24/30/33 | 8/9/59 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `srvmissilec` | ✓ | ✓ | ✓ | 25/26/61 | 21/21/22 | 4/5/39 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `useservermissilesonremoteclients` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `srvoverlay` | ✓ | ✓ | ✓ | 15/15/11 | 8/8/5 | 7/7/6 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurafilter` | ✓ | ✓ | ✓ | 64/67/112 | 49/50/48 | 15/17/64 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurastate` | ✓ | ✓ | ✓ | 105/107/134 | 76/76/71 | 29/31/63 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `auratargetstate` | ✓ | ✓ | ✓ | 65/72/104 | 49/52/47 | 16/20/57 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `auralencalc` | ✓ | ✓ | ✓ | 81/92/126 | 59/61/58 | 22/31/68 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurarangecalc` | ✓ | ✓ | ✓ | 100/112/181 | 70/71/78 | 30/41/103 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurastat1` | ✓ | ✓ | ✓ | 105/111/154 | 80/82/87 | 25/29/67 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurastatcalc1` | ✓ | ✓ | ✓ | 100/106/147 | 75/77/80 | 25/29/67 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurastat2` | ✓ | ✓ | ✓ | 68/73/107 | 56/58/64 | 12/15/43 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurastatcalc2` | ✓ | ✓ | ✓ | 70/75/108 | 58/60/65 | 12/15/43 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurastat3` | ✓ | ✓ | ✓ | 38/41/63 | 30/30/33 | 8/11/30 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurastatcalc3` | ✓ | ✓ | ✓ | 39/42/63 | 31/31/33 | 8/11/30 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurastat4` | ✓ | ✓ | ✓ | 16/17/30 | 15/15/19 | 1/2/11 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurastatcalc4` | ✓ | ✓ | ✓ | 17/18/30 | 16/16/19 | 1/2/11 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurastat5` | ✓ | ✓ | ✓ | 3/3/14 | 3/3/8 | 0/0/6 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurastatcalc5` | ✓ | ✓ | ✓ | 3/3/14 | 3/3/8 | 0/0/6 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurastat6` | ✓ | ✓ | ✓ | 3/3/8 | 3/3/6 | 0/0/2 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aurastatcalc6` | ✓ | ✓ | ✓ | 3/3/8 | 3/3/6 | 0/0/2 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `auraevent1` | ✓ | ✓ | ✓ | 18/18/17 | 17/17/11 | 1/1/6 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `auraeventfunc1` | ✓ | ✓ | ✓ | 18/18/17 | 17/17/11 | 1/1/6 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `auraevent2` | ✓ | ✓ | ✓ | 7/7/5 | 7/7/3 | 0/0/2 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `auraeventfunc2` | ✓ | ✓ | ✓ | 7/7/5 | 7/7/3 | 0/0/2 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `auraevent3` | ✓ | ✓ | ✓ | 5/5/1 | 5/5/1 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `auraeventfunc3` | ✓ | ✓ | ✓ | 5/5/1 | 5/5/1 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `auraevent4` | ✓ | ✓ | — | 1/1/0 | 1/1/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `auraeventfunc4` | ✓ | ✓ | — | 1/1/0 | 1/1/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivestate` | ✓ | ✓ | ✓ | 34/34/46 | 33/33/35 | 1/1/11 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `passiveitype` | ✓ | ✓ | ✓ | 12/12/9 | 12/12/8 | 0/0/1 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `passivereqweaponcount` | ✓ | ✓ | — | 4/4/0 | 4/4/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivestat1` | ✓ | ✓ | ✓ | 72/71/101 | 69/69/74 | 3/2/27 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `passivecalc1` | ✓ | ✓ | ✓ | 72/71/101 | 69/69/74 | 3/2/27 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `passivestat2` | ✓ | ✓ | ✓ | 45/48/57 | 42/47/42 | 3/1/15 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `passivecalc2` | ✓ | ✓ | ✓ | 45/48/57 | 42/47/42 | 3/1/15 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `passivestat3` | ✓ | ✓ | ✓ | 36/38/29 | 36/38/26 | 0/0/3 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `passivecalc3` | ✓ | ✓ | ✓ | 36/38/29 | 36/38/26 | 0/0/3 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `passivestat4` | ✓ | ✓ | ✓ | 21/22/12 | 21/22/12 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `passivecalc4` | ✓ | ✓ | ✓ | 21/22/12 | 21/22/12 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `passivestat5` | ✓ | ✓ | ✓ | 15/16/3 | 15/16/3 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `passivecalc5` | ✓ | ✓ | ✓ | 15/16/3 | 15/16/3 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `passivestat6` | ✓ | ✓ | — | 12/13/0 | 12/13/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivecalc6` | ✓ | ✓ | — | 12/13/0 | 12/13/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivestat7` | ✓ | ✓ | — | 11/12/0 | 11/12/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivecalc7` | ✓ | ✓ | — | 11/12/0 | 11/12/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivestat8` | ✓ | ✓ | — | 6/6/0 | 6/6/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivecalc8` | ✓ | ✓ | — | 6/6/0 | 6/6/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivestat9` | ✓ | ✓ | — | 6/6/0 | 6/6/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivecalc9` | ✓ | ✓ | — | 6/6/0 | 6/6/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivestat10` | ✓ | ✓ | — | 2/2/0 | 2/2/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivecalc10` | ✓ | ✓ | — | 2/2/0 | 2/2/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivestat11` | ✓ | ✓ | — | 1/1/0 | 1/1/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivecalc11` | ✓ | ✓ | — | 1/1/0 | 1/1/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivestat12` | ✓ | ✓ | — | 2/2/0 | 2/2/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivecalc12` | ✓ | ✓ | — | 2/2/0 | 2/2/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivestat13` | ✓ | ✓ | — | 1/1/0 | 1/1/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivecalc13` | ✓ | ✓ | — | 1/1/0 | 1/1/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivestat14` | ✓ | ✓ | — | 1/1/0 | 1/1/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passivecalc14` | ✓ | ✓ | — | 1/1/0 | 1/1/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `summon` | ✓ | ✓ | ✓ | 35/35/45 | 32/32/32 | 3/3/13 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `pettype` | ✓ | ✓ | ✓ | 40/48/45 | 39/39/34 | 1/9/11 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `petmax` | ✓ | ✓ | ✓ | 33/33/40 | 33/33/32 | 0/0/8 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `requirespettype` | ✓ | ✓ | — | 4/12/0 | 4/4/0 | 0/8/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `summode` | ✓ | ✓ | ✓ | 35/35/45 | 33/33/34 | 2/2/11 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `sumskill1` | ✓ | ✓ | ✓ | 21/22/23 | 21/22/20 | 0/0/3 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `sumsk1calc` | ✓ | ✓ | ✓ | 21/22/23 | 21/22/20 | 0/0/3 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `sumskill2` | ✓ | ✓ | ✓ | 12/12/17 | 12/12/14 | 0/0/3 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `sumsk2calc` | ✓ | ✓ | ✓ | 12/12/17 | 12/12/14 | 0/0/3 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `sumskill3` | ✓ | ✓ | ✓ | 9/9/17 | 9/9/14 | 0/0/3 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `sumsk3calc` | ✓ | ✓ | ✓ | 9/9/17 | 9/9/14 | 0/0/3 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `sumskill4` | ✓ | ✓ | ✓ | 6/6/7 | 6/6/7 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `sumsk4calc` | ✓ | ✓ | ✓ | 6/6/7 | 6/6/7 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `sumskill5` | ✓ | ✓ | ✓ | 1/1/3 | 1/1/3 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `sumsk5calc` | ✓ | ✓ | ✓ | 1/1/3 | 1/1/3 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `sumumod` | ✓ | ✓ | ✓ | 6/6/1 | 5/5/0 | 1/1/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `sumoverlay` | ✓ | ✓ | ✓ | 3/11/2 | 2/2/0 | 1/9/2 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `stsuccessonly` | ✓ | ✓ | ✓ | 25/26/34 | 19/20/22 | 6/6/12 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `stsound` | ✓ | ✓ | ✓ | 190/203/300 | 136/139/120 | 54/64/180 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `stsoundclass` | ✓ | ✓ | ✓ | 22/27/33 | 18/23/18 | 4/4/15 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `stsounddelay` | ✓ | ✓ | ✓ | 12/15/16 | 10/13/8 | 2/2/8 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `weaponsnd` | ✓ | ✓ | ✓ | 1/1/4 | 1/1/4 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `dosound` | ✓ | ✓ | ✓ | 15/26/22 | 10/12/10 | 5/14/12 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `dosound a` | ✓ | ✓ | ✓ | 3/3/5 | 2/2/2 | 1/1/3 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `dosound b` | ✓ | ✓ | ✓ | 2/2/3 | 1/1/1 | 1/1/2 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `tgtoverlay` | ✓ | ✓ | ✓ | 7/7/5 | 4/4/4 | 3/3/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `tgtsound` | ✓ | ✓ | ✓ | 7/9/5 | 6/7/4 | 1/2/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `prgoverlay` | ✓ | ✓ | ✓ | 9/9/9 | 8/8/6 | 1/1/3 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `prgsound` | ✓ | ✓ | ✓ | 8/7/8 | 5/5/4 | 3/2/4 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `castoverlay` | ✓ | ✓ | ✓ | 84/88/136 | 59/62/42 | 25/26/94 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `cltoverlaya` | ✓ | ✓ | ✓ | 5/5/7 | 5/5/4 | 0/0/3 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `cltoverlayb` | ✓ | ✓ | ✓ | 2/2/3 | 2/2/2 | 0/0/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `cltstfunc` | ✓ | ✓ | ✓ | 123/126/195 | 65/66/80 | 58/60/115 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `cltdofunc` | ✓ | ✓ | ✓ | 157/167/311 | 80/88/106 | 77/79/205 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `cltstopfunc` | ✓ | ✓ | — | 2/2/0 | 1/1/0 | 1/1/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `cltprgfunc1` | ✓ | ✓ | ✓ | 12/12/19 | 7/7/7 | 5/5/12 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `cltprgfunc2` | ✓ | ✓ | ✓ | 6/6/7 | 6/6/6 | 0/0/1 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `cltprgfunc3` | ✓ | ✓ | ✓ | 6/6/7 | 6/6/6 | 0/0/1 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `cltmissile` | ✓ | ✓ | ✓ | 53/49/76 | 23/18/16 | 30/31/60 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `cltmissilea` | ✓ | ✓ | ✓ | 140/150/273 | 75/83/87 | 65/67/186 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `cltmissileb` | ✓ | ✓ | ✓ | 47/56/100 | 27/36/31 | 20/20/69 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `cltmissilec` | ✓ | ✓ | ✓ | 35/39/80 | 29/31/32 | 6/8/48 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `cltmissiled` | ✓ | ✓ | ✓ | 1/1/1 | 0/0/0 | 1/1/1 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `cltcalc1` | ✓ | ✓ | ✓ | 36/38/32 | 18/19/8 | 18/19/24 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*cltcalc1 desc` | ✓ | ✓ | ✓ | 36/38/32 | 18/19/8 | 18/19/24 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `cltcalc2` | ✓ | ✓ | ✓ | 7/7/18 | 4/4/7 | 3/3/11 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*cltcalc2 desc` | ✓ | ✓ | ✓ | 7/7/18 | 4/4/7 | 3/3/11 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `cltcalc3` | ✓ | ✓ | ✓ | 4/4/15 | 2/2/7 | 2/2/8 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*cltcalc3 desc` | ✓ | ✓ | ✓ | 4/4/15 | 2/2/7 | 2/2/8 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `warp` | ✓ | ✓ | ✓ | 3/3/7 | 3/3/5 | 0/0/2 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `immediate` | ✓ | ✓ | ✓ | 11/13/23 | 10/10/10 | 1/3/13 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `enhanceable` | ✓ | ✓ | ✓ | 405/409/583 | 241/241/224 | 164/168/359 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `attackrank` | ✓ | ✓ | ✓ | 424/437/603 | 241/241/224 | 183/196/379 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `noammo` | ✓ | ✓ | ✓ | 1/1/20 | 1/1/13 | 0/0/7 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `range` | ✓ | ✓ | ✓ | 424/437/603 | 241/241/224 | 183/196/379 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `weapsel` | ✓ | ✓ | ✓ | 16/16/23 | 12/12/15 | 4/4/8 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `requiresweapon` | ✓ | ✓ | — | 8/8/0 | 7/7/0 | 1/1/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `itypea1` | ✓ | ✓ | ✓ | 67/67/116 | 56/55/72 | 11/12/44 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `itypea2` | ✓ | ✓ | ✓ | 2/2/4 | 2/2/4 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `itypea3` | ✓ | ✓ | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `etypea1` | ✓ | ✓ | ✓ | 6/6/8 | 4/4/0 | 2/2/8 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `etypea2` | ✓ | ✓ | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `itypeb1` | ✓ | ✓ | ✓ | 5/5/7 | 5/5/6 | 0/0/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `itypeb2` | ✓ | ✓ | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `itypeb3` | ✓ | ✓ | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `etypeb1` | ✓ | ✓ | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `etypeb2` | ✓ | ✓ | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `anim` | ✓ | ✓ | ✓ | 386/397/577 | 218/218/221 | 168/179/356 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `seqtrans` | ✓ | ✓ | ✓ | 386/397/577 | 218/218/221 | 168/179/356 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `monanim` | ✓ | ✓ | ✓ | 422/435/603 | 241/241/224 | 181/194/379 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `seqnum` | ✓ | ✓ | ✓ | 45/46/59 | 29/29/22 | 16/17/37 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `seqinput` | ✓ | ✓ | ✓ | 5/5/6 | 5/5/3 | 0/0/3 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `durability` | ✓ | ✓ | ✓ | 24/25/26 | 14/14/11 | 10/11/15 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `useattackrate` | ✓ | ✓ | ✓ | 166/164/234 | 111/108/116 | 55/56/118 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `lineofsight` | ✓ | ✓ | ✓ | 64/65/99 | 59/59/54 | 5/6/45 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `targetableonly` | ✓ | ✓ | ✓ | 58/58/68 | 41/41/34 | 17/17/34 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `searchenemyxy` | ✓ | ✓ | ✓ | 54/53/67 | 36/35/31 | 18/18/36 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `searchenemynear` | ✓ | ✓ | ✓ | 21/21/24 | 16/16/18 | 5/5/6 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `searchopenxy` | ✓ | ✓ | ✓ | 15/15/25 | 8/8/7 | 7/7/18 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `selectproc` | ✓ | ✓ | ✓ | 10/10/10 | 9/9/9 | 1/1/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `targetcorpse` | ✓ | ✓ | ✓ | 20/20/26 | 9/9/9 | 11/11/17 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `targetpet` | ✓ | ✓ | ✓ | 7/7/4 | 5/5/3 | 2/2/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `targetally` | ✓ | ✓ | ✓ | 4/4/3 | 3/3/3 | 1/1/0 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `targetitem` | ✓ | ✓ | ✓ | 3/4/3 | 2/2/2 | 1/2/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `attacknomana` | ✓ | ✓ | ✓ | 47/47/59 | 38/38/32 | 9/9/27 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `tgtplacecheck` | ✓ | ✓ | ✓ | 2/2/2 | 0/0/0 | 2/2/2 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `keepcursorstateonkill` | ✓ | ✓ | — | 109/111/0 | 93/93/0 | 16/18/0 | `UI_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `continuecastunselected` | ✓ | ✓ | — | 3/2/0 | 3/2/0 | 0/0/0 | `UI_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `clearselectedonhold` | ✓ | ✓ | — | 2/2/0 | 2/2/0 | 0/0/0 | `UI_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `itemeffect` | ✓ | ✓ | ✓ | 97/98/195 | 78/77/80 | 19/21/115 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `itemclteffect` | ✓ | ✓ | ✓ | 5/7/29 | 5/5/17 | 0/2/12 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `NATIVE_CALLBACKS_PRESERVE_OR_DEFER` |
| `itemtgtdo` | ✓ | ✓ | ✓ | 1/1/6 | 1/1/1 | 0/0/5 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `itemtarget` | ✓ | ✓ | ✓ | 8/8/24 | 7/7/15 | 1/1/9 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `itemuserestrict` | ✓ | ✓ | — | 2/2/0 | 0/0/0 | 2/2/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `itemcheckstart` | ✓ | ✓ | ✓ | 2/2/8 | 2/2/4 | 0/0/4 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `itemcltcheckstart` | ✓ | ✓ | ✓ | 3/3/1 | 3/3/1 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `itemcastsound` | ✓ | ✓ | ✓ | 64/66/129 | 58/58/54 | 6/8/75 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `itemcastoverlay` | ✓ | ✓ | ✓ | 18/18/31 | 16/16/14 | 2/2/17 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `skpoints` | ✓ | ✓ | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `reqlevel` | ✓ | ✓ | ✓ | 424/437/603 | 241/241/224 | 183/196/379 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `maxlvl` | ✓ | ✓ | ✓ | 260/263/397 | 241/241/224 | 19/22/173 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `reqstr` | ✓ | ✓ | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `reqdex` | ✓ | ✓ | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `reqint` | ✓ | ✓ | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `reqvit` | ✓ | ✓ | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `reqskill1` | ✓ | ✓ | ✓ | 174/177/244 | 170/170/162 | 4/7/82 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `reqskill2` | ✓ | ✓ | ✓ | 29/30/27 | 28/28/18 | 1/2/9 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `reqskill3` | ✓ | ✓ | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `restrict` | ✓ | ✓ | ✓ | 33/41/59 | 23/30/31 | 10/11/28 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `state1` | ✓ | ✓ | ✓ | 9/9/9 | 7/7/7 | 2/2/2 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `state2` | ✓ | ✓ | ✓ | 2/6/1 | 2/6/1 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `state3` | ✓ | ✓ | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `localdelay` | ✓ | ✓ | — | 27/28/0 | 25/18/0 | 2/10/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `globaldelay` | ✓ | ✓ | — | 2/2/0 | 2/2/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `leftskill` | ✓ | ✓ | ✓ | 427/439/286 | 240/240/159 | 187/199/127 | `UI_ONLY` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `rightskill` | ✓ | ✓ | — | 428/441/0 | 241/241/0 | 187/200/0 | `UI_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `repeat` | ✓ | ✓ | ✓ | 3/3/3 | 3/3/3 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `alwayshit` | ✓ | ✓ | — | 1/11/0 | 1/11/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `usemanaondo` | ✓ | ✓ | ✓ | 3/3/2 | 3/3/2 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `startmana` | ✓ | ✓ | ✓ | 4/4/3 | 4/4/3 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `minmana` | ✓ | ✓ | ✓ | 262/276/421 | 240/240/224 | 22/36/197 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `manashift` | ✓ | ✓ | ✓ | 262/276/421 | 240/240/224 | 22/36/197 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `mana` | ✓ | ✓ | ✓ | 262/276/421 | 240/240/224 | 22/36/197 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `lvlmana` | ✓ | ✓ | ✓ | 262/276/421 | 240/240/224 | 22/36/197 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `interrupt` | ✓ | ✓ | ✓ | 387/398/559 | 225/224/208 | 162/174/351 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `intown` | ✓ | ✓ | ✓ | 79/84/113 | 62/65/59 | 17/19/54 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aura` | ✓ | ✓ | ✓ | 34/35/50 | 23/23/23 | 11/12/27 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `periodic` | ✓ | ✓ | ✓ | 5/5/5 | 4/4/2 | 1/1/3 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `perdelay` | ✓ | ✓ | ✓ | 33/34/48 | 24/24/22 | 9/10/26 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `periodicclearaura` | ✓ | ✓ | — | 1/1/0 | 1/1/0 | 0/0/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `finishing` | ✓ | ✓ | ✓ | 6/6/11 | 4/4/4 | 2/2/7 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `prgchargestocast` | ✓ | ✓ | — | 6/6/0 | 4/4/0 | 2/2/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `prgchargesconsumed` | ✓ | ✓ | — | 6/6/0 | 4/4/0 | 2/2/0 | `D2R_NATIVE_PRESERVE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `passive` | ✓ | ✓ | ✓ | 30/30/29 | 30/30/27 | 0/0/2 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `progressive` | ✓ | ✓ | ✓ | 9/9/7 | 9/9/6 | 0/0/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `scroll` | ✓ | ✓ | ✓ | 4/4/6 | 0/0/0 | 4/4/6 | `UI_ONLY` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `calc1` | ✓ | ✓ | ✓ | 190/202/223 | 125/127/101 | 65/75/122 | `RAW_TECHNICAL_OVERRIDE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*calc1 desc` | ✓ | ✓ | ✓ | 192/196/216 | 126/128/97 | 66/68/119 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `calc2` | ✓ | ✓ | ✓ | 106/119/116 | 72/76/56 | 34/43/60 | `RAW_TECHNICAL_OVERRIDE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*calc2 desc` | ✓ | ✓ | ✓ | 106/112/118 | 72/77/58 | 34/35/60 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `calc3` | ✓ | ✓ | ✓ | 52/59/52 | 33/40/24 | 19/19/28 | `RAW_TECHNICAL_OVERRIDE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*calc3 desc` | ✓ | ✓ | ✓ | 52/57/55 | 33/38/26 | 19/19/29 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `calc4` | ✓ | ✓ | ✓ | 64/66/31 | 40/44/17 | 24/22/14 | `RAW_TECHNICAL_OVERRIDE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*calc4 desc` | ✓ | ✓ | ✓ | 65/67/32 | 41/45/17 | 24/22/15 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `calc5` | ✓ | ✓ | — | 10/8/0 | 7/5/0 | 3/3/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `*calc5 desc` | ✓ | ✓ | — | 10/8/0 | 7/5/0 | 3/3/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `calc6` | ✓ | ✓ | — | 7/7/0 | 4/4/0 | 3/3/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `*calc6 desc` | ✓ | ✓ | — | 7/7/0 | 4/4/0 | 3/3/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `calc7` | ✓ | ✓ | — | 5/5/0 | 3/3/0 | 2/2/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `*calc7desc` | ✓ | ✓ | — | 7/7/0 | 5/5/0 | 2/2/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `calc8` | ✓ | ✓ | — | 1/1/0 | 1/1/0 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `*calc8desc` | ✓ | ✓ | — | 3/3/0 | 3/3/0 | 0/0/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `calc9` | ✓ | ✓ | — | 1/1/0 | 1/1/0 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `*calc9desc` | ✓ | ✓ | — | 3/3/0 | 3/3/0 | 0/0/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `calc10` | ✓ | ✓ | — | 1/1/0 | 1/1/0 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `*calc10desc` | ✓ | ✓ | — | 1/1/0 | 1/1/0 | 0/0/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `param1` | ✓ | ✓ | ✓ | 278/284/443 | 203/205/196 | 75/79/247 | `RAW_TECHNICAL_OVERRIDE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*param1 description` | ✓ | ✓ | ✓ | 278/284/453 | 203/205/197 | 75/79/256 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `param2` | ✓ | ✓ | ✓ | 242/250/399 | 183/187/188 | 59/63/211 | `RAW_TECHNICAL_OVERRIDE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*param2 description` | ✓ | ✓ | ✓ | 243/251/412 | 184/188/189 | 59/63/223 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `param3` | ✓ | ✓ | ✓ | 194/203/332 | 151/156/150 | 43/47/182 | `RAW_TECHNICAL_OVERRIDE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*param3 description` | ✓ | ✓ | ✓ | 194/204/342 | 151/157/150 | 43/47/192 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `param4` | ✓ | ✓ | ✓ | 174/179/297 | 134/136/135 | 40/43/162 | `RAW_TECHNICAL_OVERRIDE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*param4 description` | ✓ | ✓ | ✓ | 174/180/314 | 134/136/136 | 40/44/178 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `param5` | ✓ | ✓ | ✓ | 112/116/197 | 91/93/103 | 21/23/94 | `RAW_TECHNICAL_OVERRIDE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*param5 description` | ✓ | ✓ | ✓ | 112/116/207 | 91/93/103 | 21/23/104 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `param6` | ✓ | ✓ | ✓ | 91/94/175 | 76/77/97 | 15/17/78 | `RAW_TECHNICAL_OVERRIDE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*param6 description` | ✓ | ✓ | ✓ | 91/94/189 | 76/77/98 | 15/17/91 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `param7` | ✓ | ✓ | ✓ | 70/69/113 | 58/59/73 | 12/10/40 | `RAW_TECHNICAL_OVERRIDE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*param7 description` | ✓ | ✓ | ✓ | 70/68/114 | 58/58/73 | 12/10/41 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `param8` | ✓ | ✓ | ✓ | 169/168/289 | 148/148/153 | 21/20/136 | `RAW_TECHNICAL_OVERRIDE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `*param8 description` | ✓ | ✓ | ✓ | 169/168/292 | 148/148/153 | 21/20/139 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `param9` | ✓ | ✓ | — | 20/20/0 | 19/19/0 | 1/1/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `*param9 description` | ✓ | ✓ | — | 22/22/0 | 21/21/0 | 1/1/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `param10` | ✓ | ✓ | — | 12/12/0 | 11/11/0 | 1/1/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `*param10 description2` | ✓ | ✓ | — | 13/13/0 | 12/12/0 | 1/1/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `param11` | ✓ | ✓ | — | 12/12/0 | 12/12/0 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `*param11 description` | ✓ | ✓ | — | 12/12/0 | 12/12/0 | 0/0/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `param12` | ✓ | ✓ | — | 14/14/0 | 13/13/0 | 1/1/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `*param12 description` | ✓ | ✓ | — | 13/13/0 | 12/12/0 | 1/1/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `param13` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `*param13description` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `param14` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `*param14description` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `param15` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `*param15description` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `param16` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `*param16description` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `param17` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `*param17description` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `param18` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `*param18description` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `param19` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `*param19description` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `param20` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `RAW_TECHNICAL_OVERRIDE` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `*param20description` | ✓ | ✓ | — | 0/0/0 | 0/0/0 | 0/0/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `SEMANTIC_BLANKS_REQUIRE_NO_DECISION` |
| `ingame` | ✓ | ✓ | ✓ | 425/438/603 | 241/241/224 | 184/197/379 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `tohit` | ✓ | ✓ | ✓ | 72/76/111 | 54/56/66 | 18/20/45 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `levtohit` | ✓ | ✓ | ✓ | 72/76/111 | 54/56/66 | 18/20/45 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `tohitcalc` | ✓ | ✓ | ✓ | 7/6/3 | 5/4/2 | 2/2/1 | `RAW_TECHNICAL_OVERRIDE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `resultflags` | ✓ | ✓ | ✓ | 22/20/40 | 14/13/15 | 8/7/25 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `hitflags` | ✓ | ✓ | ✓ | 4/5/12 | 4/4/5 | 0/1/7 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `hitclass` | ✓ | ✓ | ✓ | 8/8/10 | 6/5/7 | 2/3/3 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `kick` | ✓ | ✓ | ✓ | 4/4/4 | 3/3/3 | 1/1/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `hitshift` | ✓ | ✓ | ✓ | 414/427/596 | 241/241/223 | 173/186/373 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `srcdam` | ✓ | ✓ | ✓ | 74/75/104 | 53/53/52 | 21/22/52 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `mindam` | ✓ | ✓ | ✓ | 34/34/66 | 21/21/21 | 13/13/45 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `minlevdam1` | ✓ | ✓ | ✓ | 33/33/56 | 21/21/21 | 12/12/35 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `minlevdam2` | ✓ | ✓ | ✓ | 33/33/56 | 21/21/21 | 12/12/35 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `minlevdam3` | ✓ | ✓ | ✓ | 33/33/56 | 21/21/21 | 12/12/35 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `minlevdam4` | ✓ | ✓ | ✓ | 33/33/56 | 21/21/21 | 12/12/35 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `minlevdam5` | ✓ | ✓ | ✓ | 33/33/56 | 21/21/21 | 12/12/35 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `maxdam` | ✓ | ✓ | ✓ | 33/33/66 | 20/20/21 | 13/13/45 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `maxlevdam1` | ✓ | ✓ | ✓ | 32/32/56 | 20/20/21 | 12/12/35 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `maxlevdam2` | ✓ | ✓ | ✓ | 32/32/56 | 20/20/21 | 12/12/35 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `maxlevdam3` | ✓ | ✓ | ✓ | 32/32/56 | 20/20/21 | 12/12/35 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `maxlevdam4` | ✓ | ✓ | ✓ | 32/32/56 | 20/20/21 | 12/12/35 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `maxlevdam5` | ✓ | ✓ | ✓ | 32/32/56 | 20/20/21 | 12/12/35 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `dmgsympercalc` | ✓ | ✓ | ✓ | 17/19/30 | 15/17/20 | 2/2/10 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `etype` | ✓ | ✓ | ✓ | 128/150/264 | 96/117/129 | 32/33/135 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `emin` | ✓ | ✓ | ✓ | 120/121/238 | 90/91/106 | 30/30/132 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `eminlev1` | ✓ | ✓ | ✓ | 118/120/208 | 89/90/105 | 29/30/103 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `eminlev2` | ✓ | ✓ | ✓ | 118/120/208 | 89/90/105 | 29/30/103 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `eminlev3` | ✓ | ✓ | ✓ | 118/120/208 | 89/90/105 | 29/30/103 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `eminlev4` | ✓ | ✓ | ✓ | 118/120/208 | 89/90/105 | 29/30/103 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `eminlev5` | ✓ | ✓ | ✓ | 118/120/207 | 89/90/103 | 29/30/104 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `emax` | ✓ | ✓ | ✓ | 112/113/229 | 84/85/100 | 28/28/129 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `emaxlev1` | ✓ | ✓ | ✓ | 112/113/201 | 84/85/101 | 28/28/100 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `emaxlev2` | ✓ | ✓ | ✓ | 112/112/199 | 84/85/99 | 28/27/100 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `emaxlev3` | ✓ | ✓ | ✓ | 112/112/199 | 84/85/99 | 28/27/100 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `emaxlev4` | ✓ | ✓ | ✓ | 112/112/199 | 84/85/99 | 28/27/100 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `emaxlev5` | ✓ | ✓ | ✓ | 112/112/200 | 84/85/99 | 28/27/101 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `edmgsympercalc` | ✓ | ✓ | ✓ | 83/91/145 | 79/87/92 | 4/4/53 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `elen` | ✓ | ✓ | ✓ | 35/36/74 | 25/26/35 | 10/10/39 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `elevlen1` | ✓ | ✓ | ✓ | 26/26/40 | 17/17/20 | 9/9/20 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `elevlen2` | ✓ | ✓ | ✓ | 28/28/41 | 19/19/20 | 9/9/21 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `elevlen3` | ✓ | ✓ | ✓ | 28/28/41 | 19/19/20 | 9/9/21 | `D2R_NATIVE_PRESERVE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `elensympercalc` | ✓ | ✓ | ✓ | 5/6/5 | 5/6/4 | 0/0/1 | `RAW_TECHNICAL_OVERRIDE` | `BEHAVIOR_BUNDLE` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aitype` | ✓ | ✓ | ✓ | 59/49/42 | 45/35/31 | 14/14/11 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `aibonus` | ✓ | ✓ | ✓ | 4/4/3 | 2/2/2 | 2/2/1 | `D2R_NATIVE_PRESERVE` | `EXPERT_OVERRIDE_ONLY` | `RAW_FIELDS_INHERIT_FROM_BEHAVIOR_BUNDLE` |
| `cost mult` | ✓ | ✓ | ✓ | 267/269/399 | 241/241/223 | 26/28/176 | `ITEM_ECONOMY_ONLY` | `NO_SKILL_DECISION` | `ITEM_ECONOMY_FIELDS_PRESERVE_BKVINCE` |
| `cost add` | ✓ | ✓ | ✓ | 427/431/603 | 241/241/224 | 186/190/379 | `ITEM_ECONOMY_ONLY` | `NO_SKILL_DECISION` | `ITEM_ECONOMY_FIELDS_PRESERVE_BKVINCE` |
| `*eol` | ✓ | ✓ | — | 429/451/0 | 241/241/0 | 188/210/0 | `DOCUMENTARY_ONLY` | `NO_SKILL_DECISION` | `PRESERVE_ALL_D2R_BKVINCE_COLUMNS` |
| `auratgtevent` | — | — | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `PD2_SCHEMA_UNUSED` | `NO_SKILL_DECISION` | `IGNORE_ALL_EMPTY_PD2_ONLY_COLUMNS` |
| `auratgteventfunc` | — | — | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `PD2_SCHEMA_UNUSED` | `NO_SKILL_DECISION` | `IGNORE_ALL_EMPTY_PD2_ONLY_COLUMNS` |
| `passiveevent` | — | — | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `PD2_SCHEMA_UNUSED` | `NO_SKILL_DECISION` | `IGNORE_ALL_EMPTY_PD2_ONLY_COLUMNS` |
| `passiveeventfunc` | — | — | ✓ | 0/0/0 | 0/0/0 | 0/0/0 | `PD2_SCHEMA_UNUSED` | `NO_SKILL_DECISION` | `IGNORE_ALL_EMPTY_PD2_ONLY_COLUMNS` |
| `delay` | — | — | ✓ | 0/0/48 | 0/0/32 | 0/0/16 | `NATIVE_EXTENSION_REQUIRED` | `GLOBAL_POLICY` | `NO_AUTOMATIC_DELAY_TRANSLATION` |
| `checkfunc` | — | — | ✓ | 0/0/38 | 0/0/12 | 0/0/26 | `NATIVE_EXTENSION_REQUIRED` | `GLOBAL_POLICY` | `NO_PD2_HEADER_IMPORT_WITHOUT_NATIVE_PROOF` |
| `nocostinstate` | — | — | ✓ | 0/0/3 | 0/0/2 | 0/0/1 | `NATIVE_EXTENSION_REQUIRED` | `GLOBAL_POLICY` | `NO_PD2_HEADER_IMPORT_WITHOUT_NATIVE_PROOF` |
| `general` | — | — | ✓ | 0/0/13 | 0/0/0 | 0/0/13 | `NATIVE_EXTENSION_REQUIRED` | `GLOBAL_POLICY` | `NO_PD2_HEADER_IMPORT_WITHOUT_NATIVE_PROOF` |

## Questions natives non résolues

- `PD2_DELAY_CONSUMER_AND_UNITS` — Quel consumer PD2 interprète delay, avec quelles unités et quelles interactions, et quelle intention peut être reproduite sans nouveau header D2R ?
- `PD2_CHECKFUNC_D2R_SUPPORT` — D2R 3.2 compile-t-il ou consomme-t-il un équivalent de checkfunc absent de son schéma suivi ?
- `CROSS_ENGINE_CALLBACK_NUMBERS` — Pour chaque numéro divergent, quel comportement D2R 3.2 est prouvé au lieu d’être déduit du numéro PD2 ?
- `PD2_ONLY_USED_HEADERS` — Ces intentions PD2 peuvent-elles être traduites vers des mécanismes D2R existants avant toute extension native ?
- `MISSILE_NATIVE_CALLBACKS` — Les fonctions et paramètres de missile PD2 sélectionnés ont-ils un équivalent D2R 3.2 prouvé côté serveur et client ?

## Sources hashées

- Skills Vanilla D2R 3.2 : `EFAF7AC4BA0493109C698EF32ACF4A2B3A577E13500D0B50258C80B600986F51`
- Skills BKVince HEAD : `08497CC0BD8B2B5CBD895F7477AD0CBF272571FB67B78061EDFABC31C48B8B77`
- Skills PD2 / Single Player+ : `AEEFC3F2C0C80811D62FC1A17C3B031DE2164E5606BF9779F34024B35BC87B8B`
- skillsSchema : `944A40AA17BF44C8D5B262482925FAD26CF3B20FBE0B941D779D6E60F36DE742`
- analyticalAudit : `BE9385A532CBD3DF80D94E83F04293FB9238DFD10101C5A9F442DB8DAC07D565`
- nativeFindings : `0A8854BF1151CE4CE2FEA4F77EACE73C96C968C4D4132E05E4BCE29AD5D7E620`
- knownRvas : `4AB3FF680C128F5E7D12520CD0ACAA97800AA1DE491141B65593BE9AF3941A5B`
