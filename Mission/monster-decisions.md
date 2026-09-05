# PD2 Monster Merge — décisions d'implantation BKVince

Statut : **spécification approuvée; aucune implantation gameplay autorisée par ce document**.

Source de preuve : audit `e2b49c918b6b70ffbec4777d87ce2e46f6a080b4`.
Baseline obligatoire : `f38d73854b27376a4183a00529380f197b04100d`.
HEAD revalidé pour cette édition : `d626456c1fc2c17aca670c03706f51623707b9f7`.

`ACCEPT` signifie « cible approuvée pour un lot futur », jamais « changement déjà
testé en jeu ». `ACCEPT/no-op` signifie que la valeur voulue est déjà présente et
doit être prouvée au runtime sans réécriture. `ACCEPT/prototype HEAD` signale une
valeur déjà entrée dans le HEAD, mais qui n'a pas encore franchi la validation
fonctionnelle. `DEFER` et `REJECT` interdisent l'implantation dans le merge courant.

## Périmètre et gates absolus

- Aucun merge aveugle ou complet de `TreasureClassEx.txt`.
- Aucune copie complète d'une row PD2 : seules les cellules nommément approuvées
  peuvent changer.
- Aucun sidecar, loader, plugin ou DLL dans ce chantier. Toute future route native
  exige un propriétaire décidé, une preuve gouvernée D2R 3.2.92777 et le gate
  d'incubation des plugins avant la première modification.
- Aucun changement de `ROADMAP.html` ni de la mission courante par ce document.
- Avant chaque lot, comparer de nouveau la baseline ci-dessus au **HEAD alors
  courant**; une cellule ne peut être déclarée inchangée à partir de l'ancien audit.
- Tests bloquants futurs : solo ciblé, sauvegarde/rechargement, cold start et
  coexistence avec tous les plugins actifs et toutes les fonctionnalités du
  PluginPack activées. Le multijoueur host/join est utile, mais non bloquant pour
  BKVince.

## Revalidation baseline → HEAD

La comparaison `f38d7385…..d626456c…` invalide l'ancienne phrase « seules des
rows Rogue/Raven/mercenaire ont changé ». Ces rows étrangères au périmètre sont
toujours présentes, mais le HEAD actuel contient aussi un prototype sur quatre
rows auditées de `monstats.txt` :

| Row | cellules changées depuis l'audit | lecture décisionnelle |
|---|---|---|
| `baalclone` | `primeevil: 1 → vide` | `ACCEPT/prototype HEAD` |
| `uberizual` | `primeevil: vide → 1`; retrait de `switchai` et de `Chilling Armor`; Frost Nova niveau 44; résistances H `50/50/90/90/90/95`; `DamageRegen=1`; `Crit=5`; HP H `7177/7287`; `AC(H)=60`; A1 H `364/416` | `ACCEPT/prototype HEAD`, validation à faire |
| `uberandariel` | résistances H `50/50/90/90/90/90`; `DamageRegen=1`; `Crit=5`; HP H `7177/7287`; `AC(H)=55`; A1 H `416/458` | `ACCEPT/prototype HEAD`, validation à faire |
| `uberduriel` | résistances H `50/50/90/90/90/95`; `DamageRegen=1`; `Crit=5`; HP H `7177/7287`; `AC(H)=60`; A1 H `375/395`; A2 H `119/171`; S1 H `42/52`; ajout froid A1 `42–84`, durées `50/50/100` | `ACCEPT/prototype HEAD`, validation à faire |

Les autres cellules de la matrice auditée sont inchangées au HEAD revalidé.
Cette conclusion exclut les modifications non committées du workspace et doit
être recalculée avant toute implantation.

## Contrat exact `PrimeEvilRules`

La liste BKVince est exactement celle-ci, soit **15 membres** :

```text
andariel, duriel, mephisto, diablo, baalcrab,
ubermephisto, uberdiablo, uberbaal,
uberizual, uberandariel, uberduriel,
colossal1, colossal2, colossal3,
diabloclone
```

Conséquences : `uberizual.primeevil=1` et `baalclone.primeevil` vide. Ce swap
probable est déjà dans le prototype HEAD. La liste n'est ni la taxonomie PD2 de
149 rows, ni un synonyme de `boss`, ni une autorisation de copier les flags PD2.

Règles voulues pour ces 15 membres :

- immunité universelle à **tout effet de ralentissement**, notamment le chill
  causé par les dégâts de froid, `Slows Target by X%`, Decrepify, Holy Freeze,
  Slow Movement, Clay Golem et les slows custom;
- les dégâts de froid et les composantes qui ne ralentissent pas restent
  applicables; seule toute réduction effective de vitesse est neutralisée;
- immunité à Dim Vision, Terror, Confuse et Attract;
- dégâts totaux ×2 contre mercenaires **et** invocations/pets;
- Sanctuary reste inchangé tant que son lot est différé.

Cette immunité universelle est une décision produit BKVince qui applique la
formulation littérale du wiki PD2. Elle ne dépend pas d'une preuve que chaque
chemin runtime S13 produit effectivement ce résultat : aucun test runtime PD2
n'est requis. La validation doit démontrer le résultat dans BKVince.

Le flag `primeevil` ne produit pas ces effets à lui seul dans D2R 3.2.92777.
Pour les quatre curses d'AI, la route la plus simple est de vérifier d'abord la
protection déjà fournie à chacun des 15 par sa classe unique/superunique et son
AI non commutable. Aucun code n'est ajouté si les 15 passent. Le premier échec
interdit de déclarer la règle implantée et ouvre un lot natif ciblé; il ne doit
pas être masqué en ajoutant des flags sans consommateur prouvé.

## Deux anomalies séparées

1. **Clés `pk1` — anomalie d'audit reclassée.** Summoner et Nihlathak pointent
   bien vers `pk1` plutôt que `pk2/pk3`, mais c'est le design BKVince à clé
   universelle. `ACCEPT/no-op`; aucune correction de clé, y compris pour
   Countess ou Blood Raven.
2. **`MonHolyShock` — nom trompeur, comportement BKV intentionnel.** La row
   monster-only nommée `MonHolyShock` reproduit Fanaticism parce que BKVince a
   volontairement réaffecté les outcomes historiques du sélecteur Aura
   Enchanted (`MonHolyFire` donne Vigor et `MonHolyShock` donne Fanaticism).
   Ce n'est donc pas une row corrompue à « réparer » isolément. `DEFER` : prouver
   d'abord les huit ordinals consommés par le sélecteur 92777, puis construire
   le pool final sans doublon Fanaticism, avec Vigor, Holy Fire et Holy Shock
   réels.

## Légende des tests et rollbacks

- **T0** : round-trip byte-exact des TXT, diff limité aux cellules approuvées,
  chargement solo, save/reload, cold start et pile complète de plugins.
- **T1** : mesure solo N/NM/H, p1 et p8 lorsque la vie/dégâts en dépendent.
- **T2** : simulation et essai solo des branches non-quest, quest et TZ; p1
  bloquant, p3/p5/p8 informatifs; vérifier pool d'objets, essence et clé.
- **T3** : 1 000+ spawns uniques, distribution des auras, niveaux et exclusions.
- **T4** : Dolls N/NM/H, délais/rayons/dégâts, morts variées, Revive, cadavre,
  loot/XP/kill credit et absence de double explosion.
- **T5** : chacun des 15 Prime contre chill/froid, item slow, Decrepify, Holy
  Freeze, Slow Movement, Clay Golem et slows custom, puis contre chaque curse
  approuvée; dégâts merc/pet par attaque, missile, sort et DoT; aucun autre
  monstre ne doit être touché.
- **T6** : carte solo, trois presets, pathing, collision et hash des couches.
- **T7** : chaque Uber séparément, HP/AC/CTH/block/résistances/skills/AI/TTK.
- **R0** : aucun rollback, no-op ou décision documentaire.
- **R1** : restaurer uniquement les cellules TXT approuvées, jamais la row entière.
- **R2** : restaurer uniquement les nœuds/arêtes TC approuvés et leurs poids.
- **R3** : retirer l'override DS1; fallback SHA-256
  `1DB568BC8183EFC862468647EF5245109FE396E2178D3749591E594A25750F93`.
- **R4** : désactiver/revenir sur le seul guard natif du lot via son propriétaire;
  aucune DLL partagée ne peut être remplacée globalement comme rollback.

## Matrice de décision — toutes les lignes auditées

| # | Changement audité | Décision et cible BKVince exacte | Cellules/ressources et route | Dépendances | Tests solo | Rollback |
|---:|---|---|---|---|---|---|
| 1 | Holy Shock dans Aura Enchanted | **IMPLANTÉ** : fréquence `6/6/12`; pool monster-only `[MonConviction, MonFanaticism, MonHolyShock, MonHolyFreeze, MonHolyFire, MonMight, MonConcentration, MonVigor]`; niveau `clamp(floor(mlvl/7),1,13)`; portée ×2; Vigor conserve sa courbe BKVince ≈ `13–39 %` au lieu du scaling PD2 | `monumod.txt`, `skills.txt`, `pd2-aura-enchanted-scaling.json` — **TXT + memory patch gouverné** | ordinals et sélecteur 92777 prouvés; Lord de Seis reste Fanaticism; Uber Meph reste hors de ce lot | T0 passé, T3 en jeu restant | R1 + R4 |
| 2 | Dolls PD2 | **ACCEPT** sur les sept Dolls classiques BKV : proc 100 %, délai visé 25 frames, rayon 4, dégâts physiques fixes N `18–30`, NM `54–96`, H `318–540`; conserver l'explosion BKV des Rift Dolls et le comportement `ISREVIVE` | `monstats.txt`, `monprop.txt`, `properties.txt`, `itemstatcost.txt`, `skills.txt`, `missiles.txt`, AnimData au besoin — **TXT + native** | funcs event/skill/missile 92777; classification classique/Rift; ownership | T0, T4 | R1 + R4 |
| 3 | `primeevil` comme axe | **ACCEPT** : liste exacte de 15 ci-dessus; `uberizual=1`, `baalclone` vide; ne pas étendre aux 149 flags PD2 | `monstats.txt.primeevil` — **TXT**; consommateurs séparés — **native** | contrat stable `PrimeEvilRules` | T0, T5 | R1/R4 |
| 4 | Taxonomies Act/Apex/Rift/etc. | **DEFER** : aucun sidecar ni taxonomie chargée dans ce merge | aucun fichier — route future **TXT/native** | définitions et loader futurs | aucun tant que différé | R0 |
| 5 | Dégâts Prime merc/pet | **ACCEPT** : multiplicateur total ×2 pour mercenaires et invocations/pets, jamais ×4 | consommateur de dégâts Prime; ne pas modifier globalement `DifficultyLevels.txt` sans isolation — **native** | flags d'unité, ownership/source des DoT, preuve 92777 | T0, T5 | R4 |
| 6 | Item Slow | **ACCEPT** : les 15 ignorent entièrement `Slows Target by X%` | EventFunc19/équivalent 92777 — **native** | point d'entrée et ABI prouvés | T0, T5 | R4 |
| 7 | Decrepify | **ACCEPT** : aucune application d'état ni ralentissement sur les 15 | chemins skill 87/state 60 — **native** | deux chemins 92777 prouvés | T0, T5 | R4 |
| 8 | Holy Freeze | **ACCEPT** : les 15 ne reçoivent pas l'effet de ralentissement Holy Freeze | `skills.txt.aurafilter` si bit équivalent prouvé, sinon guard — **TXT + native** | décodage `IGNPRIME` 92777 | T0, T5 | R1/R4 |
| 9 | « Tous les slows » | **ACCEPT — règle universelle** : les 15 ne subissent aucune réduction de vitesse, quelle qu'en soit la source; cela inclut chill des dégâts de froid, item slow, Decrepify, Holy Freeze, Slow Movement, Clay Golem et slows custom. Les dégâts et effets non ralentissants restent applicables | `monstats.txt.ColdEffect` pour le chill et guards/résolveur de slow 92777 pour les autres chemins — **TXT + native** | inventaire des sources BKV, point final d'application des vitesses et ABI 92777; aucun runtime PD2 requis | T0, T5 | R1/R4 |
| 10 | Dim Vision/Terror/Confuse/Attract | **ACCEPT** : aucune des quatre curses ne doit affecter les 15. Première route : protection classe/AI existante si les 60 cas passent; fallback natif seulement en cas d'échec | AI switch/catégorie unique-superunique — **native/no-op à prouver** | matrice 15 × 4; ne pas créditer le seul flag `primeevil` | T0, T5 | R0 si no-op, sinon R4 |
| 11 | Sanctuary | **DEFER** : conserver le comportement BKV actuel; aucune règle Prime ajoutée | resolver résistance physique — **native** future | preuve 92777 et politique gameplay | T0, T5 si rouvert | R4 |
| 12 | Andariel stats | **ACCEPT** Hell : `Level=85`, `MinHP/MaxHP=1471/1471`, `AC=110`, `A1Min/A1Max=200/240`, `Crit=5` | `monstats.txt`, row `andariel` — **TXT** | aucune autre cellule Andy | T0, T1 | R1 |
| 13 | Andariel pool Meph | **ACCEPT adapté** : même famille de TC d'objets que Mephisto, mais les meilleures probabilités de Meph restent supérieures selon le ratio PD2; couvrir non-quest, quest et TZ; Andy garde `tes`; taux essence PD2 `8,6943 % / 9,7950 % / 8,6943 %` | `treasureclassex.txt`, graphe Andy/Meph/TZ — **TXT** | simulation du cap 6 et des predicates quest/TZ | T0, T2 | R2 |
| 14 | Duriel niveau/HP/dégâts | **ACCEPT/no-op** : conserver niveau H 88 et les HP/dégâts BKV actuels | `monstats.txt`, row `duriel` — **TXT** | revalidation HEAD | T0, T1 | R0 |
| 15 | Défense Duriel | **ACCEPT** : `AC(H)=120`, valeur effective auditée ≈ 2044 | `monstats.txt.AC(H)` — **TXT** | mesurer CTH avec MonLvl BKV | T0, T1 | R1 |
| 16 | Duriel pool Baal | **ACCEPT adapté** : pools/probabilités d'objets Baal pour non-quest et quest, mais `tes` remplace `fed`; essence `8,6943 % / 9,7950 %`; en TZ, adapter le même tier Baal TZ (`Act 5 Equip C/Good/Junk/Ancient Statue`) avec `tes`, jamais référencer directement le nœud qui donne `fed`; essence TZ `8,6943 %` | `treasureclassex.txt`, branches Duriel/Baal/TZ — **TXT** | simulation cap 6, ilvl Duriel 88, predicates quest/TZ | T0, T2 | R2 |
| 17 | Tombes niveau 82 | **REJECT** : conserver les sept tombes niveau 87 et Duriel's Lair 85 | `levels.txt` — **TXT/no-op** | aucune | T0 | R0 |
| 18 | Mephisto `flying` | **ACCEPT/no-op** : conserver `flying=1` | `monstats.txt` — **TXT/no-op** | validation pathing | T0 | R0 |
| 19 | Council rapproché | **ACCEPT** : `MephComp.ds1` place Bremm `(47,66)`, Wyand `(78,48)`, Maffer `(79,88)`; Mephisto reste `(40,65)`; aucune autre couche/tuile ne change | `MephComp.ds1` — **DS1** | IDs MonPreset et hash de couches | T6 | R3 |
| 20 | Council immunité foudre | **ACCEPT/no-op** : conserver l'immunité foudre de base des trois Council de Durance | `monstats.txt`/SuperUnique mods — **TXT/no-op** | revalidation des trois rows | T0, T1 | R0 |
| 21 | Council, immunité additionnelle | **ACCEPT/no-op** : ne forcer aucune seconde immunité; préserver données/mods BKV et vérifier que le cap runtime reste à deux | SuperUnique mods/résistances — **TXT + native no-op à prouver** | seeds Hell, handler 92777 si le cap échoue | T0, T1 sur plusieurs seeds | R0/R4 |
| 22 | Uber Meph Conviction | **ACCEPT PD2 exact** : niveau 20, pénalité de résistances `-50 %`, pas `-125 %` | skill monster-only + dispatcher Uber Meph — **TXT + native** | identité du skill, ordinal et dispatcher 92777 | T0, T7 | R1/R4 |
| 23 | Uber Baal Lower Resist | **ACCEPT PD2 exact** : `Baal Lowres` niveau 13, pénalité `-75 %`, durée cible environ 20–25 s | `monstats.txt.Skill6`, skill monster-only, sélection AI — **TXT + native** | état/durée/AI 92777 | T0, T7 | R1/R4 |
| 24 | Nihlathak HP | **ACCEPT** Hell : `MinHP/MaxHP=573/573`; préserver Aura, Corpse Explosion, AI, skills et TC `pk1` | `monstats.txt`, row `nihlathakboss` — **TXT** | aucune autre cellule Nihl | T0, T1 | R1 |
| 25 | Slogan Ubers `+10 % HP/-25 % def` | **REJECT comme recette** : ce résumé historique ne décrit pas le delta S13→BKVince | aucune | utiliser les valeurs boss par boss de la ligne 26 | R0 | R0 |
| 26 | Trio Uber recalibré | **ACCEPT PD2 exact avec adaptation MonLvl BKV** : voir tableau ci-dessous; ne pas modifier globalement `monlvl.txt` row 120 | `monstats.txt` des trois Ubers; skills monster-only — **TXT + native** selon les skills | recalcul contre `MonLvl120` BKV, ITD, AI et handlers | T0, T7 | R1/R4 |
| 27 | DClone stats | **DEFER** : aucun port de `uberdiablonew`; le merge courant conserve seulement `diabloclone` dans `PrimeEvilRules` | futur projet DClone — **TXT/DS1/native** | projet ROADMAP séparé | tests propres au futur projet | rollback propre au futur projet |
| 28 | DClone `PLR75` | **REJECT** : S13 porte 50, pas 75 | aucune | aucune | R0 | R0 |
| 29 | DClone `PLR50` | **DEFER** avec tout le combat DClone | future MonProp/stat handler — **TXT + native** | politique PLR et cap 92777 | futur projet | futur projet |
| 30 | DClone immune Static | **DEFER** avec tout le combat DClone; ne pas toucher `StaticFieldRework` ici | futur owner combat — **native** | coordination plugin et preuve 92777 | futur projet, pile complète | futur projet |
| 31 | Countess HP | **ACCEPT adapté, pas PD2 exact** : Hell ×1,5 sur BKV, donc `MinHP/MaxHP=90/150`; préserver défense, Crit, vitesse, AI, offense et TC `pk1` | `monstats.txt`, row Countess — **TXT** | aucune autre cellule Countess | T0, T1 | R1 |
| 32 | Summoner HP | **ACCEPT PD2** : N `1032/1376`, NM `880/1120`, H `880/1120`; préserver skills, aura, AI et TC `pk1` | `monstats.txt`, row Summoner — **TXT** | aucune autre cellule Summoner | T0, T1 | R1 |
| 33 | Clés Summoner/Nihl `pk1` | **ACCEPT/no-op** : clé universelle BKV voulue; ne pas migrer vers `pk2/pk3` | `treasureclassex.txt`/références TC — **TXT/no-op** | aucune | T0, T2 | R0 |

## Valeurs exactes du trio Uber approuvé

Les pourcentages `MonStats` doivent être recalibrés contre la row BKV
`MonLvl120`; copier les pourcentages PD2 avec une autre table de ratios ne suffit
pas. Les résultats S13 visés sont :

| Boss | H level; HP%; AC%; Crit | résistances H D/M/F/L/C/P; block | dégâts H | skills et niveaux |
|---|---|---|---|---|
| `ubermephisto` | `120; 5695/5695; 60; 5` | `40/40/60/60/60/60; 50` | A1 `375–440` | PrimeLightning 6; PrimeBolt 20; PrimePoisonNova 7; MephistoMissile 5; MephFrostNova 1; MonBlizzard 5; Conviction spéciale niveau 20 à `-50 %` |
| `uberdiablo` | `120; 6427/6427; 55; 5` | `40/40/60/60/60/60; 50` | A1 `370–380`; A2 `110–230` | DiabLight 16; DiabCold 2; DiabFire 14; DiabWall 12; DiabRun 5; PrimeFirewall 8; DiabPrison 1; Diablogeddon 10 |
| `uberbaal` | `120; 6336/6336; 56; 5` | `40/40/60/60/60/60; 55` | A1 `500–550`; A2 `330–480` | Baal Nova 18; Baal Inferno 16; Baal Tentacle 15; Baal Cold Missiles 36; Baal Teleport 1; Baal Lowres 13; Blood Mana 3 |

L'implantation BKV adapte ces cibles aux multiplicateurs de `MonLvl120` sans
modifier `monlvl.txt`. Les cellules réellement écrites sont :

| Boss | HP% BKV; AC% BKV | cellules de dégâts BKV | valeurs effectives de contrôle |
|---|---|---|---|
| `ubermephisto` | `5580/5580; 56` | A1 `487–572` | HP `569550`; AC `1270` |
| `uberdiablo` | `6297/6297; 51` | A1 `481–494`; A2 `143–299` | HP `642734`; AC `1156` |
| `uberbaal` | `6208/6208; 52` | A1 `651–714`; A2 `429–623` | HP `633650`; AC `1179` |

## Décisions additionnelles prises après l'audit

| Sujet | Décision exacte | Route, dépendances, tests et rollback |
|---|---|---|
| Mini-Ubers | **ACCEPT/prototype HEAD** : conserver exactement les cellules énumérées dans la table de revalidation pour `uberizual`, `uberandariel` et `uberduriel`; ne pas importer leurs rows complètes ni leurs TC PD2 | **TXT**, `monstats.txt`; T0/T1/T7, puis validation des skills d'Izual et de l'ajout froid de Duriel; R1 |
| Blood Raven | **ACCEPT hybride** : HP H `1329/1329` (×1,5 BKV); remplacer Quick Strike par Immolation Arrow; conserver vitesse `12/19`; escorte réduite `12–16` de zombies spéciaux et archers; conserver AC H 200, Crit 10, délais AI `15/6/0` et TC `pk1`; utiliser un clone d'archer dédié, jamais modifier `sk_archer1` global | **TXT** : `monstats`, `skills`, `missiles` et rows de summons avec nouveaux IDs BKV, jamais les IDs PD2 362/686 déjà en collision. T0/T1/T2; vérifier loot/XP des spawns initiaux et `NOXP/NOTC` des summons Nest. R1. |
| Bloodwitch | **REJECT/no-op** : laisser entièrement inchangée | aucune route; R0 |
| Essences Mephisto | conserver ses meilleurs pools; `ceh` non-quest `8,6943 %`, quest `9,7950 %`, TZ `8,6943 %` | **TXT**, T0/T2, R2 |
| Essences Diablo | conserver ses pools; `bet` non-quest `8,6943 %`, quest `9,7950 %`, TZ `8,6943 %` | **TXT**, T0/T2, R2 |
| Essences Baal | conserver ses pools; `fed` non-quest `8,6943 %`, quest `9,7950 %`, TZ `8,6943 %` | **TXT**, T0/T2, R2 |
| Keyholders | une seule clé `pk1` partout est le contrat BKVince; aucune normalisation PD2 des trois clés | **TXT/no-op**, T2, R0 |
| DClone encounter complet | tiers, arène Pandemonium Fortress D2R, phases, six fire novas, trapped souls/burning ground, squelettes non maudissables avec 60 % poison et PLR75, météores et Bone Spirits progressifs sont un **projet futur séparé**, après Monster Merge et BKVCombat | futur **TXT + DS1 + native + assets HD**; aucun fichier dans ce merge |

## Lots atomiques obligatoires

1. **Corrections indépendantes et baseline** : revalidation HEAD; consigner `pk1`
   comme design; ne pas modifier les rows d'aura réaffectées avant preuve des
   ordinals consommés par le sélecteur 92777. Aucun autre changement ne dépend
   d'une supposition de nom/ID.
2. **`MephComp.ds1`** : trois coordonnées seulement, hash de couches et fallback
   vérifiés; aucune table dans le même commit.
3. **Statistiques et économie ciblée** : Andariel, Nihlathak, Countess, Summoner,
   Duriel, Blood Raven, puis adaptation TC Andy/Duriel/TZ et poids d'essences.
   Les changements de skill/summons Blood Raven peuvent être scindés du HP.
4. **Aura Enchanted — implanté le 12 août 2026** : sélecteur 92777 prouvé et
   redirigé vers huit auras monster-only; `upick=6/6/12`, Holy Fire et Holy Shock
   réels, portée ×2, scaling `mlvl/7` cap13; Vigor conserve la courbe BKVince.
5. **Dolls** : sept classiques selon le modèle PD2; Rift Doll 777 et `ISREVIVE`
   préservés.
6. **Règles Prime Evil** : flags exacts, puis immunité universelle à tous les
   slows et chaque autre protection/multiplicateur comme sous-lot isolé. La
   preuve recherchée porte sur le résultat BKVince, pas sur le runtime PD2.
7. **Ubers/DClone** : mini-Ubers prototype à qualifier; trio Uber PD2 exact adapté
   à `MonLvl120`; DClone limité à sa présence dans la liste Prime et aux règles
   globales. Son encounter complet reste hors scope.
8. **BKVCombat seulement ensuite** : aucun propriétaire natif n'est présumé par
   les lots précédents; repasser le gate d'incubation avant toute DLL.

## Validation de sortie du document et des futurs lots

- La présente matrice couvre les 33 lignes de faisabilité de l'audit.
- Aucun `ACCEPT` n'autorise un merge complet de Treasure Classes ou de rows PD2.
- Chaque futur diff TXT doit être CRLF, byte-exact au round-trip et limité aux
  cellules approuvées; chaque graph TC doit publier une simulation reproductible.
- Chaque changement structurel doit régénérer `ai-cartographie.json`, obtenir
  `VALID`, puis passer `git diff --check`.
- Après chaque lot : checkpoint frais; validation solo, save/reload, cold start,
  pile complète active. Le multijoueur reste non bloquant pour BKVince.
- Le rollback s'exécute par lot : cellules, graphe TC, override DS1 ou guard
  natif; jamais par restauration globale d'une table ou désactivation d'un plugin.

## État d'implantation — 2026-08-12

`ACCEPT` demeure une décision de design dans la matrice; l'état technique réel
du merge est le suivant.

- **Implanté en TXT/DS1** : statistiques Andariel, Duriel, Nihlathak, Countess et
  Summoner; Blood Raven hybride et ses summons/skills/missiles dédiés;
  `MephComp.ds1`; liste Prime Evil de 15 et `ColdEffect=0`; graphes TC
  Andy/Meph/Duriel/Baal, quest et TZ; poids d'essences; trio Uber recalibré;
  `BKV Baal Lowres` niveau 13 dans le slot 6 d'Uber Baal; pool Aura Enchanted
  monster-only PD2 avec exception Vigor BKVince.
- **Non implanté, dépendance native ouverte** : explosions des sept Dolls
  classiques; la Doll Rift 777 conserve sa route distincte;
  ralentissements Prime autres que chill/froid; quatre curses
  d'AI; dégâts ×2 mercenaires/pets; Conviction spéciale d'Uber Mephisto. Aucun
  sidecar, loader, plugin ou DLL n'a été créé pour contourner ces gates.
- **Validation statique passée** : round-trip byte-exact CRLF, `verify:data`,
  références de démarrage, stats effectives, `git diff --check`, cadastre
  `VALID`, hash DS1 `768F0A06831ECF09F88FEA99D29FDCFE6BC599A0759F0D872DF9530DDFFA5279`.
- **Économie p1 passée** : essence régulière/TZ `8,6942818 %`; quest
  `9,7949614 %`. Les simulations informatives p3/p5/p8 montent respectivement à
  `23,9051/36,6133/51,7919 %` pour la branche régulière et
  `26,6281/40,3559/56,2736 %` pour la branche quest à cause du scaling NoDrop.
- **Régression TC détectée et corrigée le 10 août** : le premier runtime réellement
  probant a arrêté le chargement à `MonsterTbls.cpp:1279`, ligne TC 930, parce que
  sept sous-TC quest avaient été placées après leurs parents. Les sept rows ont
  été déplacées sans modifier leurs cellules ou probabilités; le validateur de
  démarrage interdit désormais génériquement toute référence TC vers l'avant.
- **Cold start correctif passé** : profil `BKVince -txt -offline`, build 92777,
  gameplay tables franchies jusqu'à `24/24`, `18/18` plugins actifs, `15/15`
  patches appliqués, zéro erreur/rejet/échec et aucun nouveau rapport de crash;
  SHA-256 source/runtime de `treasureclassex.txt`
  `AE8E8E8487CA34293221032E17E06168912A0482645F36F5336E0748D29F892F`.
  Les combats ciblés, la sauvegarde/recharge et les tests T1/T3/T4/T5/T6/T7
  restent `not run`; ils ne sont pas implicitement validés par le démarrage.
- **Aura Enchanted validé techniquement le 12 août** : `skills.txt` revient à
  Vanilla pour les auras Paladin, ajoute les huit rows monster-only avec portée
  ×2 et paramètres PD2 sauf `MonVigor`; le patch natif reloge huit records,
  applique `mlvl/7` et le cap13. Cold start complet : `17/17` fichiers de
  patches, `19/19` plugins actifs, `24/24`, zéro rejet/échec. La relecture
  mémoire confirme les 19 écritures et les huit IDs exacts. Le témoin T3 en jeu
  sur plusieurs packs et paliers de niveau reste à exécuter.
