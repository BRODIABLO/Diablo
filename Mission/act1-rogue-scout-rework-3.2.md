# Rework des Rogue Scouts de l’Acte I — BKVince 3.2

Dernière mise à jour : 1 août 2026

## Statut

Chantier actif en parallèle de la mission courante BaseMod. Vincent a confirmé
le 1 août 2026 l’adaptation originale des idées de TDE 3.1d pour les Rogue
Scouts de BKVince, sans branche Holy dans ce premier lot.

## Contrat joueur confirmé

- Les Rogues doivent utiliser la course pour leurs déplacements significatifs.
- Lorsqu’elles sont frappées en mêlée, elles lancent `Terror` selon la courbe
  TDE : 25 % au niveau 5 en Normal, 33 % au niveau 10 en Nightmare et 50 % au
  niveau 15 en Hell.
- Les deux factions conservent `Inner Sight` et reçoivent `Bow Mastery` selon
  la courbe TDE.
- La Rogue Fire progresse de `Fire Arrow` vers `Exploding Arrow`, puis
  `Immolation Arrow`.
- La Rogue Cold progresse de `Cold Arrow` vers `Ice Arrow`, puis
  `Freezing Arrow`.
- Les compétences avancées se débloquent aux paliers BKVince 36 et 67 au lieu
  d’être toutes actives dès le niveau 3 comme dans TDE.
- Chaque Rogue invoque au maximum un familier distinct : `Fire Raven` ou
  `Cold Raven`. Le familier réutilise le modèle HD Raven existant, suit son
  propriétaire, fournit un rayon lumineux et ne doit pas entrer en collision
  avec les Ravens d’un joueur Druide.
- La faction Holy reste hors du premier lot et sera évaluée séparément. Aucun
  nouvel effet HD original n’est requis ici.

## Faits vérifiés

- BKVince utilise déjà `Velocity=11`, `Run=14` pour `roguehire`, contre
  `Velocity=11`, `Run=11` dans TDE et `7/11` dans vanilla 3.2. Le besoin restant
  concerne donc le choix du mode de déplacement, pas une simple hausse de
  vitesse.
- Le patch `hireling-ai-tuning.json` resserre déjà les distances de suivi et de
  rattrapage, augmente l’activité ranged et renforce le recul.
- Les lignes Expansion de `hireling.txt` possèdent déjà les paliers 3, 36 et 67
  nécessaires à la progression retenue.
- TDE met les six compétences à disposition dès le niveau 3. BKVince utilise
  une progression réelle : une compétence verrouillée porte `Chance=0`,
  `Level=0` et `LvlPerLvl=0`, donc elle ne peut ni être choisie ni fournir une
  synergie avant son palier.
- Le proc TDE repose sur un stat événementiel `damagedinmelee`, distinct du
  `gethit-skill` vanilla qui accepte aussi les missiles.
- Le Raven TDE est un vrai summon limité à un, et non un projectile : pet type
  distinct, warp vers le propriétaire, attaque physique et élémentaire, lumière
  blanche de rayon 7.
- Le workbench D2R 3.2.92777 est vérifié. Les six patches IA ranged existants
  sont gouvernés, mais aucun site ne prouve encore une course forcée permanente.

## Architecture retenue

1. Appliquer une migration TSV reproductible et idempotente; refuser toute clé
   absente, dupliquée ou déjà occupée par un contenu différent.
2. Ajouter les définitions propres à BKVince pour `Bow Mastery`, `Fire Raven`,
   `Cold Raven` et leur pet type sans copier les identifiants numériques TDE.
3. Ajouter un proc mêlée-only de `Terror` sans modifier la compétence joueur.
4. Réaffecter les six slots de compétence des Rogues et conserver une vraie
   progression 3/36/67.
5. Équilibrer les dégâts sur la puissance BKVince actuelle aux niveaux 3, 36,
   67 et 90 avec la formule gouvernée de `hireling.txt`, le `HitShift` propre à
   chaque compétence et uniquement les synergies réellement apprises; la
   maîtrise fait partie du budget total de DPS.
6. Déployer uniquement les tables allowlistées, comparer leurs hashes et
   valider cold start puis gameplay.
7. Observer la marche/course réelle. Ne rechercher ou implanter un complément
   natif qu’en présence d’un écart reproductible.

## Matrice de validation

| Domaine | Cas | Attendu | Statut |
|---|---|---|---|
| TSV | Round-trip et CRLF | byte-exact avant/après migration | PASS statique |
| Données | Références et IDs | aucune collision ou référence absente | PASS statique |
| Progression | Niveaux 3/36/67 | attaques avancées niveau/croissance 0, puis actives | PASS statique |
| Terror | Normal/Nightmare/Hell | 25/33/50 %, mêlée seulement | PASS statique; combat runtime requis |
| Raven | Fire/Cold | un familier correct, dégâts et lumière cohérents | PASS statique; combat runtime requis |
| Cycle de vie | portail/waypoint/mort/résurrection | aucun doublon ni familier orphelin | not run |
| Coexistence | Druide Raven | pools indépendants | not run |
| Déplacement | suivi/recul/rattrapage | course sur les déplacements significatifs | suivi en ville observé; combat requis |
| Runtime | cold start BKVince | table chargée et frontend atteint | PARTIAL : 24/24; assertions item post-frontend à isoler |
| Runtime | roster et Rogue existante | lignes version 100 visibles; niveau 98 et icônes cohérentes | PASS |
| Réseau | solo/hôte/joiner | propriété et comportement synchronisés | not run |

## Implantation data-first

- `Terror` est injecté par `monprop.txt` avec une propriété et un stat
  `damagedinmelee` propres à BKVince : 25/5, 33/10 et 50/15 selon la difficulté.
- Les paliers 3/36/67 sont conservés. Fire reçoit successivement `Fire Arrow`,
  `Exploding Arrow`, `Immolation Arrow`; Cold reçoit `Cold Arrow`, `Ice Arrow`,
  `Freezing Arrow`.
- `BKV Bow Mastery` reprend exactement la courbe TDE : à slvl 1/12/22/50,
  +28/116/196/420 % d’AR et +28/83/133/273 % de dégâts physiques.
- `BKV Fire Raven` et `BKV Cold Raven` utilisent deux monstres distincts et le
  pet type `bkvrogueraven`, plafonné à un familier, sans réutiliser le pool du
  Raven Druide.

## Budget de dégâts statique corrigé

Les valeurs ci-dessous sont des dégâts élémentaires par impact calculés depuis
les formules et synergies de `skills.txt`. Le calcul choisit le dernier palier
de recrutement inférieur ou égal au niveau courant, puis applique exactement :

`slvl = Level# + floor(LvlPerLvl# × (niveau courant − niveau du palier) / 32)`.

Il applique également le facteur `2^(HitShift−8)`; `Cold Arrow` utilise
`HitShift=7` et inflige donc la moitié des valeurs brutes de ses colonnes. Les
estimations excluent le physique de l’arc, Bow Mastery, l’équipement, les
résistances, la cadence réelle de l’IA et le sol brûlant d’Immolation Arrow.

| Niveau | Fire — attaques apprises | Cold — attaques apprises |
|---:|---|---|
| 3 | `Fire Arrow` 1–4 | `Cold Arrow` 3–4 |
| 36 | `Fire Arrow` 50–55; `Exploding` 66–88 | `Cold Arrow` 50–52; `Ice` 82–90 |
| 67 | `Fire Arrow` 183–205; `Exploding` 233–279; `Immolation` 57–62 | `Cold Arrow` 72–74; `Ice` 132–143; `Freezing` 163–198 |
| 90 | `Fire Arrow` 479–546; `Exploding` 380–444; `Immolation` 93–99 | `Cold Arrow` 149–159; `Ice` 265–287; `Freezing` 242–283 |

La branche Fire conserve ses niveaux 22/12/7 au palier 67. La branche Cold
utilise 15/9/7 au même palier : cela absorbe l’ajout de la synergie Ice au lieu
de doubler artificiellement sa puissance. À niveau 90, ses trois attaques sont
environ 8 à 18 % au-dessus de leurs équivalents actifs précédents, tandis que
Fire gagne surtout la variété, l’AoE et le sol brûlant. Bow Mastery atteint
alors slvl 29, soit +252 % d’AR et +168 % de dégâts physiques. Le Raven atteint
slvl 15 et demeure volontairement un compagnon d’appoint.

La migration recalcule et vérifie automatiquement ces huit snapshots. La
première estimation du 1 août extrapolait à tort la croissance depuis le niveau
zéro et omettait `HitShift`; les valeurs 1019–1394 et 525–754 sont retirées.

Le checkpoint conserve les réservations Readable Items `389/308` avant les IDs
Act I `390/309`, afin de préserver l’indexation physique de `itemstatcost.txt` et
`properties.txt`. Ces deux définitions restent inertes sans le plugin et la
recette Readable Items. Le validateur Storage Bag accepte donc des lignes tierces
après son bloc gouverné, tout en exigeant que ce bloc demeure complet, contigu et
byte-exact.

## Validation runtime

### 1 août 2026 — antérieure au correctif de dégâts

- Les neuf tables déployées (`hireling`, `itemstatcost`, `monprop`, `monstats`,
  `monstats2`, `pettype`, `properties`, `skills`, `states`) ont un SHA-256
  identique entre le dépôt et `mods/BKVince`.
- Le cold start atteint la sélection des personnages puis une partie BKVince;
  les logs frais D2RLoader ne contiennent aucune nouvelle erreur, exception ou
  référence invalide.
- Le roster de Kashya affiche les lignes Expansion version 100 au niveau 98 et
  leurs profils de compétences alternés.
- La Rogue existante Amplisa charge au niveau 98, accompagne le joueur et son
  panneau présente le profil de compétences attendu sans crash.
- Une tentative runtime temporaire avec un prix de recrutement nul a été
  refusée par le jeu; cette donnée de diagnostic n’a jamais été écrite dans le
  dépôt et le runtime officiel a été restauré puis revalidé par hash.
- Aucun remplacement de mercenaire ni aucune modification volontaire de
  sauvegarde n’a été effectué. Le summon effectif des deux Ravens, `Terror`, la
  mort/résurrection et la coexistence Druide restent donc des gates de combat.

### 2 août 2026 — progression et dégâts corrigés

- `hireling.txt` source et runtime portent tous deux le SHA-256
  `93B0196A…E4C1`; le rapport gouverné est
  `analysis-cache/runtime-sync/20260802-102852977-apply.json`.
- Le cold start charge `19/19` memory patches, `8/8` plugins, zéro rejet et
  atteint les étapes `24/24`; le frontend et la liste des personnages sont
  visibles et fonctionnels.
- Quatre assertions fraîches surviennent ensuite dans
  `D2Common/src/Items/Items.cpp` (`ptStats`, `ptItemStats` et classe d’objet),
  depuis le caller `D2RLoader.exe+0x7600A`. Elles ne désignent ni
  `hireling.txt` ni une compétence, mais leur causalité n’est pas encore
  isolée; le cold start reste donc `PARTIAL`, pas un PASS propre.
- Un second cold start, rapport
  `analysis-cache/runtime-sync/20260802-103307722-apply.json`, reproduit les
  quatre mêmes assertions après sélection de `QtyTester` au lieu de la
  sauvegarde invalide `BerserkBarb`; l’hypothèse d’un incident propre au
  personnage sélectionné est donc écartée.
- Aucun personnage n’a été ouvert et aucune sauvegarde n’a été modifiée durant
  cette validation. Les dégâts réels, les trois paliers et la branche Cold
  corrigée restent à observer en combat.

## Gate immédiat

Isoler les quatre assertions item post-frontend, puis valider en combat les
deux factions, les trois paliers, le proc `Terror`, le cycle de vie des Ravens
et leur coexistence avec un Druide. La course littérale demeure un gate
d’observation séparé; aucune nouvelle DLL n’est autorisée implicitement par
cette mission.
