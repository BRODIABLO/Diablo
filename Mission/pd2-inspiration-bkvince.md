# PD2 / Single Player Plus — inspiration gouvernée pour BKVince

Dernière mise à jour : 10 août 2026

> **Décision produit du 10 août 2026 — rollback mercenaires.** Vincent a
> retiré les prototypes mercenaires postérieurs au jalon pré-TDE
> `e67e66d277230dcdd577e4f608ef293448cd07ea`. Les refontes Rogue Scout,
> Desert Mercenary et Eastern Sorceror ne sont plus actives dans BKVince.
> `RogueScoutMovement` est explicitement conservé comme plugin autonome; il
> reste indépendant des tables mercenaires retirées. Les autres sections
> mercenaires ci-dessous restent des preuves historiques et ne décrivent plus
> le runtime courant.

## But

Produire un rapport détaillé, reproductible et orienté vers la décision sur les
différences entre Project Diablo 2, son adaptation Single Player Plus et
BKVince. Le rapport doit séparer le cœur PD2 de la surcouche Single Player Plus,
puis sélectionner les concepts qui méritent une adaptation et un rééquilibrage
pour BKVince. La parité intégrale n'est pas recherchée.

Le périmètre couvre d'abord les tables TXT : changements généraux, qualité de
vie, équilibrage, mercenaires, skills généraux et propres aux classes,
monstres/boss, bases et concepts d'items. Le PvP, le ladder, le commerce et les
services purement en ligne sont exclus. Les différences hardcodées — notamment
AI, pointmods/staffmods et règles de génération — forment un chapitre ultérieur
et exigent des preuves natives distinctes.

## Décisions confirmées

- auditer PD2 core et Single Player Plus comme deux couches distinctes;
- terminer l'audit gouverné avant d'ouvrir des lots gameplay stables;
- adapter sélectivement les idées retenues à l'identité de BKVince;
- préserver autant que possible les personnages et coffres existants;
- arrêter le chantier pour une décision explicite avant tout reset inévitable;
- valider chaque futur lot en solo, puis obligatoirement en hôte/joiner avant sa
  livraison complète;
- ne copier en masse ni tables, ni valeurs, ni assets.

## Lot 2 — décisions mercenaires confirmées

- Acte I : conserver l'idée d'une Rogue physique avec des auras et 66 % de
  pierce intrinsèque, mais reporter son nouveau skill et son équilibrage à un
  lot ultérieur;
- Acte II : ne rien changer;
- Acte III : procéder en deux phases. La phase 1 ajoute seulement Cleansing,
  Prayer et Holy Shock aux trois variantes; les masteries élémentaires restent
  réservées à la phase 2;
- Acte IV : conserver les Ascendants comme idée de chantier hardcodé futur,
  sans copier leurs IDs, skills ou tables PD2;
- Acte V : étudier plus tard une variante à nouvel ID avec Battle Orders,
  Battle Cry et un Whirlwind propre au mercenaire;
- reporter l'interface générique à cinq ou six compétences. Le panneau actuel
  reste limité à trois widgets visibles et son extension exige d'abord un test
  de binding `Skill3–Skill5`.

### Acte III phase 1 — implantation statique

Le 8 août 2026, les 18 lignes `Eastern Sorceror`, `Version=100`, de
`hireling.txt` ont reçu une quatrième compétence sans modifier leurs trois
sorts existants, leurs IDs, leurs classes ou leurs paliers de recrutement :

| Variante | Skill4 | Niveau 15 | Niveau 49 | Niveau 79 |
|---|---|---:|---:|---:|
| Fire | Cleansing | `4 + 13/32` | `17 + 16/32` | `30`, plafonné |
| Cold | Prayer | `3 + 8/32` | `12 + 8/32` | `18`, plafonné |
| Lightning | Holy Shock | `6 + 8/32` | `15 + 8/32` | `21`, plafonné |

Les six cellules possédées par ligne sont `Skill4`, `Mode4`, `Chance4`,
`ChancePerLvl4`, `Level4` et `LvlPerLvl4`, soit exactement 108 cellules. Les
trois auras utilisent `Mode4=1`, `Chance4=10` et `ChancePerLvl4=0`, le pattern
natif D2R 3.2 des auras de mercenaires. Le palier 79 porte
`LvlPerLvl4=0` afin de préserver l'identité du mercenaire Acte II Prayer et de
contenir les auras BKVince déjà accélérées ou agrandies.

Ces deux choix sont des adaptations BKVince explicites, pas une copie littérale
de PD2 : la source PD2 S13 emploie `Chance=0` et laisse encore progresser Prayer
et Holy Shock après le palier 79. BKVince retient `Chance=10`, conformément au
précédent natif D2R 3.2 des auras de mercenaires, puis plafonne les courbes. Le
risque résiduel est une sélection AI périodique de l'aura; l'activité des sorts,
le recast et l'idle doivent donc être observés en jeu.

Le fichier final reste en CRLF, conserve ses 77 colonnes, ses 126 lignes et son
saut de ligne final, puis passe un round-trip byte-exact. Son SHA-256 source est
`1FA4EC8899F772F87C6129C69518C15D701C9AB59D0002C43214FB08CB1CE27A`.
Les validateurs de références de démarrage, Mercenary Command et Rogue Acte I
sont verts après la modification. Ce jalon est donc **implanté statiquement**,
mais pas encore déclaré validé en jeu. `npm run verify:data` est également vert
sur la table finale.

Le déploiement allowlist du 8 août 2026 a copié uniquement `hireling.txt` vers
le profil `C:\Games\Diablo II Resurrected\mods\BKVince`, après sauvegarde de la
version runtime précédente. Le rapport
`analysis-cache/runtime-sync/20260808-152233670-apply.json` prouve l'égalité du
SHA-256 source/runtime. Le cold start frais accepte le build 92777, charge
`18/18` patchsets et `14/14` plugins sans désactivation, rejet ni échec, puis
atteint `24/24` étapes. Aucun nouvel assert n'apparaît dans les logs frais.
Cela valide le déploiement et le chargement de la table, pas encore le
comportement des auras en combat.

Gates runtime encore ouverts : activation et maintien de chaque aura après
embauche, chargement, portail, waypoint, mort et résurrection; continuité des
casts; interaction avec une aura identique fournie par l'équipement; solo,
hôte et joiner. Cleansing doit aussi être observé séparément : sa composante de
soin dépend de Prayer et devrait rester nulle sur la variante Fire qui ne
possède pas cette compétence.

## Lot Skills — Warmth Attack Rating

Le 8 août 2026, le second effet passif de Warmth a été adapté depuis PD2 dans
BKVince sans copier les différences d'interface ou de coût de la source. La
ligne `Warmth` de `skills.txt` conserve sa régénération de mana existante et
reçoit exactement quatre cellules gameplay :

- `passivestat2=item_tohit_percent`;
- `passivecalc2=toht`;
- `ToHit=20`;
- `LevToHit=10`.

La courbe calculée est donc `20 + (niveau - 1) × 10`, soit `20 %`, `110 %`,
`210 %` et `410 %` d'Attack Rating aux niveaux 1, 10, 20 et 40. La ligne
`warmth` de `skilldesc.txt` conserve sa première ligne D2R de régénération de
mana et ajoute une seconde ligne adaptée aux conventions BKVince :
`descline2=74`, `desctexta2=StrSkill22`, `desccalca2=toht`.

`item_tohit_percent` existe déjà à l'ordinal 119 dans BKVince et dans la
référence vanilla D2R 3.2; `itemstatcost.txt` n'a donc pas été modifié. Le coût
de Warmth, `leftskill`, `rightskill`, les lignes, les headers et les sauvegardes
restent inchangés. Les deux tables finales conservent CRLF et passent un
round-trip byte-exact. Leurs SHA-256 sont :

- `skills.txt` :
  `83374AE6758ED48A36161DF5EE1BE81DD10643D88B83A68EF2E9601F8CAEEDA0`;
- `skilldesc.txt` :
  `2B632E4BD85E865443C328673A32AA0AF2C9F3D919493CA0C33F676F413D1373`.

`npm run verify:data`, le catalogue PD2 et les 13 tests du rapport Skills ont
été verts après la régénération gouvernée du rapport. Une assertion ciblée
ferme également la formule BKVince au niveau 20, le stat passif, le calc et les
six cellules de tooltip mana/Attack Rating. Un changement concurrent hors
périmètre dans `missiles.txt` a ensuite rendu rouge le seul gate global de
fraîcheur du rapport par changement de hash; il n'a pas été absorbé ni modifié
par ce lot, et le test Warmth isolé reste vert.

Le lot est **implanté statiquement**, mais pas encore validé en jeu. Gates
runtime ouverts : affichage du tooltip; Attack Rating réel aux niveaux 1, 10,
20 et 40; cumul avec Enchant; équipement/déséquipement; respec et
sauvegarde/rechargement; solo puis hôte/joiner avec des tables identiques.

## Lot Skills — Enchant de groupe

Le 8 août 2026, BKVince a repris uniquement l'architecture de lancement de
groupe d'Enchant depuis PD2. La ligne unique `Enchant` de `skills.txt` reçoit
exactement trois cellules :

- `aurafilter=65539`, soit les bits documentés `Find Players`,
  `Find Monsters` et `Find Allies`;
- `aurarangecalc=15`;
- `auratargetstate=enchant`.

Le ciblage individuel BKVince (`targetally=1`, `targetpet=1`) reste présent.
Les formules d'équilibrage BKVince sont conservées octet pour octet :
`auralencalc=ln12`, `aurastatcalc1=enma`, `aurastatcalc2=exma`,
`aurastatcalc3=toht` et
`edmgsympercalc=(skill('Warmth'.blvl))*par8`. La durée pratiquement permanente,
la courbe d'Attack Rating, les dégâts et la synergie Warmth ne sont donc pas
remplacés par leurs valeurs PD2.

La table finale conserve ses 322 colonnes, ses 449 lignes, ses fins de ligne
CRLF et son saut de ligne final, puis passe un round-trip byte-exact. Son
SHA-256 source est
`90FE55CB5B19C5B6275F5655411F42DBE6AF524E22629094F7C0B70E498C22B9`.

`npm run verify:data` et les cinq tests du catalogue PD2 sont verts. Les 11
tests sémantiques du rapport Skills sont également verts; ses deux gates de
fraîcheur restent ouverts parce que le hash épinglé précède ce lot et que sa
régénération absorberait aussi un changement concurrent hors périmètre dans
`missiles.txt`. Le rapport exhaustif n'a donc pas été réécrit dans ce lot.

Ce jalon est **implanté statiquement**, mais pas encore validé en jeu. Gates
runtime ouverts : un cast doit enchanter le joueur, les membres du groupe, les
mercenaires et les summons alliés dans le rayon, sans affecter les ennemis ni
les unités hors rayon; vérifier aussi le recast, les transitions de zone, la
persistance après sauvegarde/rechargement et le comportement solo, hôte et
joiner avec des tables identiques.

## Lot Skills — Nova, Lightning et Chain Lightning

Le 8 août 2026, le prototype Sorcière demandé par Vincent a repris seulement
les courbes PD2 explicitement retenues, sans copier les dégâts, les synergies ou
le rayon de recherche de la source :

- `Nova` : `mana=16` et `manashift=7`, soit 8 mana au niveau 1 et 17,5 au
  niveau 20; la courbe de dégâts et la synergie BKVince avec Static Field
  restent inchangées;
- `Lightning` : `anim=SC` remplace `SQ`; `mana=16` et `manashift=7` restent
  inchangés, soit 17,5 mana au niveau 20, afin d'isoler l'effet réel de
  l'animation;
- `Chain Lightning` : `mana=8`, `manashift=7`, `Param5=4` et
  `calc1="min(ln34 / 4, 14)"`. La progression donne 6, 11 et 14 rebonds aux
  niveaux 1, 20 et 40. `Param1=25`, `Param8=9` et la synergie additionnelle
  avec Nova restent les valeurs BKVince;
- `chain lightning[desccalca3]` reçoit la même formule plafonnée dans
  `skilldesc.txt` afin que le nombre de rebonds affiché corresponde au calcul
  gameplay.

Le lot modifie exactement sept cellules de `skills.txt` et une cellule de
`skilldesc.txt`. Les tables conservent respectivement 449 lignes × 322 colonnes
et 269 lignes × 120 colonnes, leurs CRLF, leur saut de ligne final et leur
round-trip byte-exact. Leurs SHA-256 sont :

- `skills.txt` :
  `4BAA984DAFF79AE5783F82DF500994B23C511DEC72A58D0736C67161CEA98683`;
- `skilldesc.txt` :
  `842162C5E5160AD16F9FB87A358BD28C93FCBD9ABD51CBD14569AA01E59D795E`.

Le rapport exhaustif a été régénéré sur les tables finales et ses 15 tests sont
verts. L'assertion dédiée ferme les coûts de mana au niveau 20, l'animation
SC, la courbe et le plafond des rebonds, le rayon BKVince, les dégâts nus de
Nova, les synergies conservées et la formule de tooltip. Ce jalon est donc
**implanté statiquement**, sans modification de structure ni impact attendu sur
les sauvegardes, mais pas encore validé en jeu.

Gates runtime ouverts : mesurer les frames de cast, les breakpoints FCR, le DPS
réel et la consommation de mana de Lightning avant toute réduction de son coût;
vérifier Nova et Chain Lightning aux niveaux 1, 20 et 40, le plafond de 14
rebonds, le rayon de 25, le tooltip, les respecs et sauvegarde/rechargement;
terminer en solo puis hôte/joiner avec des tables identiques.

## Lot Skills — Inferno shredder

Le 8 août 2026, Vincent a autorisé le prototype data-only recommandé pour
transformer l'Inferno partagé de BKVince en canalisation courte qui réduit la
résistance au feu. La ligne `Inferno` conserve son ordinal runtime `41`, son
niveau requis `6`, sa courbe de mana BKVince et ses contrôles D2R modernes :
`KeepCursorStateOnKill=1`, `ContinueCastUnselected=1`,
`ClearSelectedOnHold=1`, `seqinput=10` et `rightskill=1`.

Le pipeline fonctionnel reprend celui de PD2 :

- `srvdofunc=182`, `cltdofunc=111`;
- `srvmissilea=infernodebuff`, `cltmissilea=infernodebuff` et
  `cltmissileb=infernodebuff2`;
- `auratargetstate=inferno_debuff`, `auralencalc=10`,
  `aurastat1=fireresist` et `aurastatcalc1="-min(lvl,100)"`;
- `Param1=22`, `Param2=2` et `aurarangecalc=ln12/4-2`, soit exactement
  `3`, `8`, `13` et `23` grid sub-tiles aux niveaux 1, 10, 20 et 40;
- la réduction vaut exactement `-1 %`, `-10 %`, `-20 %` et `-40 %` aux mêmes
  niveaux, plafonnée à `-100 %`, et le state de 10 frames doit être rafraîchi
  par les impacts continus du missile;
- les paliers de dégâts, `HitShift=5`, le plafond de portée missile et le nombre
  de missiles reprennent PD2; la synergie devient Fire Wall + Blaze à 20 % par
  hard point. La mana, le coût marchand et le niveau requis restent BKVince.

Trois missiles ont été ajoutés uniquement en fin de `missiles.txt` aux ordinals
`756..758` : `infernodebuff`, `infernodebuff2` et `infernotrail`. Le state
`inferno_debuff` est ajouté à l'ordinal `245` et son overlay visuel à l'ordinal
`342`; aucun ordinal existant n'a été déplacé. Le tooltip `inferno` conserve
ses lignes mana, portée et dégâts et reçoit une quatrième ligne explicite de
réduction de résistance au feu. Les mercenaires Acte III qui consomment la
même ligne `Inferno` reçoivent donc également ce prototype partagé.

La migration gouvernée et réversible est portée par
`scripts/migrate-bkvince/apply-pd2-inferno-shredder.js`; son mode `--check`
est idempotent et son mode `--revert` refuse de retirer une ligne ou cellule
devenue divergente. Les cinq tables finales conservent leurs headers, CRLF,
saut de ligne final et round-trip byte-exact. Leurs SHA-256 déployés sont :

- `skills.txt` :
  `4BAA984DAFF79AE5783F82DF500994B23C511DEC72A58D0736C67161CEA98683`;
- `missiles.txt` :
  `D36CAF2988E3785C5C19D69C489A7FC6BDDDA8BBF0DECCF449DF1D3D382265CB`;
- `states.txt` :
  `A67B9B7B29DBC70AC53B8D68E2B0BFAA0C700F5BB30478EA88A50EA9CF1D2FF3`;
- `overlay.txt` :
  `3C0B9CC9BFBF47A459BAF8C4295DC84E8F88F693955700F4C4F73BC3D4D85188`;
- `skilldesc.txt` :
  `842162C5E5160AD16F9FB87A358BD28C93FCBD9ABD51CBD14569AA01E59D795E`.

Les preuves statiques sont vertes : migration ciblée, références de démarrage,
`npm run verify:data`, cadastre `VALID` et rapport exhaustif `15/15`, dont une
assertion Inferno couvrant fonctions, missiles, state, contrôles modernes,
mana conservée et vecteurs portée/shred/dégâts aux niveaux 1/10/20/40.

Le déploiement mod-local a copié uniquement les cinq tables autorisées vers le
profil BKVince et prouvé l'égalité de leurs hashes avant et après le lancement.
Le cold start frais du 8 août 2026 à 17:10 EDT monte explicitement
`mod="BKVince"` sur D2R `3.2.92777`, applique `18/18` patchsets, active
`15/15` plugins avec zéro désactivation, rejet ou échec, puis atteint `24/24`.
Le log ne contient aucune erreur, assertion ou entrée critique; son SHA-256 est
`9BCDA77F247289B398D46127BBCB683385ECF18EFE53350D9FF5B1832974E52C`.
La session a été arrêtée après la collecte.

Le jalon est donc **implanté statiquement et chargé par le runtime**, mais pas
encore validé fonctionnellement en jeu. Gates ouverts : vérifier le maintien de
la canalisation après perte ou mort de la sélection; mesurer les portées et le
shred réel aux niveaux 1/10/20/40; prouver le rafraîchissement continu et la
disparition du debuff 10 frames après l'arrêt; contrôler dégâts, tooltip,
overlay, consommation de mana, interaction avec Burn Fire Resistance et le
comportement des mercenaires Acte III; terminer en solo puis hôte/joiner avec
des tables identiques.

## Lot Skills — Blizzard PD2-feel

Le 8 août 2026, Vincent a demandé d’implanter dans BKVince la variante PD2 de
Blizzard afin de tester son rythme plus actif et sa couverture plus permissive.
Le lot retient l’équilibrage PD2 complet plutôt que de conserver les dégâts
BKVince :

- `localdelay=23`, `mana=26` et `manashift=7`, soit 13, 17,5, 22,5 et
  32,5 mana aux niveaux 1, 10, 20 et 40;
- `Param1=8` pour le rayon et conservation de `Param3=2` pour l’intervalle de
  spawn déjà commun à PD2 et BKVince;
- `Param8=12` et synergies limitées à Ice Bolt + Ice Blast via
  `EDmgSymPerCalc=(skill('Ice Bolt'.blvl)+skill('Ice Blast'.blvl))*par8`;
- paliers PD2 donnant exactement `17–24`, `64–89`, `132–181` et `376–515`
  dégâts froids nus aux niveaux 1, 10, 20 et 40;
- `blizzardcenter[Range]=50`, soit deux secondes à 25 FPS;
- `blizzard1..4[Size]=3` au lieu de `2`, adaptation D2R 3.2 exacte du
  changement de hitbox de 50 % annoncé par PD2;
- tooltip D2R conservé, mais cooldown affiché à 23 frames et ligne de synergie
  Glacial Spike retirée.

Le callback PD2 `pSrvHitFunc=62` et son paramètre `sHitPar1=4` ne sont pas
copiés : ils appartiennent au moteur PD2 et ne sont pas documentés comme ABI
portable dans D2R 3.2. Les fonctions natives D2R de Blizzard, ses missiles, son
ordinal runtime 59, `HitShift=8`, sa durée de froid et ses contrôles modernes
restent inchangés. Le lot ne déplace aucune ligne et n’affecte pas le format des
sauvegardes.

La migration réversible
`scripts/migrate-bkvince/apply-pd2-blizzard-prototype.js` possède uniquement la
ligne `Blizzard`, `blizzardcenter`, `blizzard1..4` et cinq cellules du tooltip.
Ses modes `--apply`, `--check` et `--revert` refusent toute cellule divergente;
le cycle revert/apply puis le check idempotent sont verts. Les trois tables
conservent leurs headers, leurs CRLF, leur saut de ligne final et leur
round-trip byte-exact :

- `skills.txt` : 449 lignes × 322 colonnes;
- `missiles.txt` : 759 lignes × 172 colonnes;
- `skilldesc.txt` : 269 lignes × 120 colonnes.

La migration ciblée, `npm run verify:data`, les références de démarrage,
`git diff --check` et le cadastre sont verts. Le rapport exhaustif reste
volontairement non régénéré : son gate de fraîcheur détecte simultanément les
travaux concurrents Static Field, Shiver Armor, Firewall et `states.txt`;
le régénérer ici les absorberait hors du lot Blizzard. Les hashes de fichiers
complets au moment du contrôle sont donc des snapshots composites, pas des
empreintes attribuables au seul lot :

- `skills.txt` :
  `588D455E0F1386F3791E0D5A489206BD395CB65CB23CF48820D5BFDFD68267E5`;
- `missiles.txt` :
  `A1C7787689C647CE4A0F6CB7E15B3CB35FF7E0E4CFC0AC634749B7773E6E0DAF`;
- `skilldesc.txt` :
  `E998CDB316EEFAB36F542AAD646C50BE84A199C36DEE430F27AEB553D65339F4`.

Le jalon est **implanté statiquement, mais pas encore déployé ni validé en
jeu**. Le déploiement est retenu tant que les trois fichiers sources portent
des cellules concurrentes absentes du runtime : une copie entière emporterait
Shiver Armor et Firewall sans autorisation dans ce lot. Gates ouverts : égalité
source/runtime hors Blizzard ou arbitrage explicite; hashes de déploiement;
cold start complet; tooltip, cadence, durée, couverture et dégâts aux niveaux
1/10/20/40; mobilité des cibles, empilement de plusieurs casts, consommation de
mana et comparaison de ressenti avec la variante BKVince; solo puis hôte/joiner
avec des tables identiques.

## Sources figées

Le catalogue rattache le dossier local Single Player Plus à la révision
publique suivante; le manifeste local prouve exactement les octets audités,
mais le dossier ne contient pas de métadonnées `.git` permettant de reconstruire
à lui seul une preuve cryptographique d'égalité avec le tree distant :

- dépôt : <https://github.com/Lukaszpg/PD2-Single-Player-Plus-mod>;
- commit : `3debc6781f33c3c1474a995b80369a4e618cd386`;
- tree : `6f51e17e5f65abdd50b2fd33190c571fef296ccf`;
- 198 fichiers inventoriés et contrôlés dans le snapshot local;
- 93 tables TXT;
- manifeste SHA-256 des tables :
  `AED5AC542E7B879FBF6BEB49F7F76A8ED40F5725DC830E82536CBA2A1C44A2B8`.

Le manifeste interne indique encore `12.0.0a`, alors que le commit est annoncé
comme Version 13.0.2. La révision Git, et non cette chaîne obsolète, est donc la
preuve de version.

Les pages wiki utilisées sont épinglées par identifiant et horodatage de
révision dans
[`pd2-inspiration-bkvince.catalog.json`](pd2-inspiration-bkvince.catalog.json).

## Faits vérifiés — baseline TXT

- PD2/SP+ contient 93 tables et BKVince 50;
- 49 noms de tables sont communs;
- `levelgroups.txt` est la seule table propre à BKVince;
- 44 tables existent seulement dans la source PD2/SP+;
- 9 tables communes ont des headers normalisés identiques;
- 40 tables communes ont une différence de schéma;
- les 93 tables PD2/SP+ sont en LF et passent un round-trip byte-exact avec le
  parseur gouverné du workspace;
- les tables BKVince restent en CRLF et passent aussi le round-trip byte-exact;
- la taille des domaines diverge fortement : par exemple `skills.txt` contient
  603 lignes et 256 colonnes côté PD2/SP+, contre 449 lignes et 322 colonnes
  côté BKVince; `monstats.txt` contient 1 242 × 255 contre 799 × 273;
  `hireling.txt` 156 × 73 contre 126 × 77; `cubemain.txt` 3 019 × 105 contre
  2 321 × 106.

Ces nombres décrivent une incompatibilité structurelle réelle : une fusion par
remplacement de fichier détruirait des colonnes et systèmes propres à BKVince.

## Hypothèses à tester

- une partie des différences visibles dans les tables PD2 est probablement
  héritée du cœur PD2 plutôt que créée par Single Player Plus;
- certaines règles apparemment data-only ont une seconde moitié native dans
  D2R 3.2, en particulier no-drop, rare affix floor, AI et pointmods;
- plusieurs concepts PD2 sont déjà couverts, parfois plus largement, par les
  systèmes BKVince de stockage, stacking, charm inventory, corruptions, rifts et
  équipement étendu des mercenaires;
- les écarts de schéma incluent à la fois des ajouts PD2 et des colonnes D2R 3.2
  absentes du port historique; ils doivent être comparés par identité de ligne et
  sémantique de header, jamais par position brute.

## Inconnues ouvertes

- la provenance exacte, PD2 core ou Single Player Plus, de chaque ligne modifiée;
- l'autorité serveur/client des candidats qui modifient drop, AI ou génération;
- la compatibilité des nouveaux types d'items et mercenaires avec les anciennes
  sauvegardes BKVince;
- le périmètre natif exact des pointmods/staffmods dans le build 92777;
- la meilleure valeur cible de chaque mécanisme après adaptation à l'économie et
  à l'endgame BKVince.

## Recommandation démontrée

La seule stratégie sûre est une migration conceptuelle et atomique : identifier
le comportement joueur, établir la provenance et la preuve TXT/native, mesurer
le chevauchement BKVince, puis ouvrir un lot indépendant avec rollback et matrice
de validation. Une fusion brute des 49 tables communes est rejetée dès la
baseline parce que 40 schémas divergent et que BKVince porte des systèmes absents
de la source.

Le catalogue gouverné contient déjà les premiers candidats : stockage et
stacking à conserver côté BKVince; zones endgame, bases d'items, crafting,
mercenary kits, monstres et boss à adapter; no-drop, pointmods, affix floor, AI,
Act IV mercenary, dolls et copie d'état Cube à placer dans le backlog de preuves
natives.

## Architecture du rapport

1. inventaire exhaustif des 93 tables et comparaison de structure;
2. changements généraux, qualité de vie et équilibrage;
3. mercenaires : équipement, progression, skills et AI;
4. skills généraux et changements propres aux classes;
5. monstres et boss;
6. items : bases, affixes, crafting et concepts généraux;
7. surcouche propre à Single Player Plus;
8. différences hardcodées et routage memory patch/plugin/hybride;
9. backlog de lots BKVince classés par valeur, risque et dépendances.

L'inventaire est terminé. Le chapitre général/QoL est en cours. Le chapitre
**Monsters, Bosses et Prime Evil** est fermé au niveau audit dans
[`pd2-monsters-bosses-vs-bkvince-audit.md`](pd2-monsters-bosses-vs-bkvince-audit.md) :
core S13, SP+, BKVince, cartes et preuves natives y restent séparés, sans
autoriser de merge gameplay. Les autres chapitres restent explicitement
planifiés et ne sont pas déclarés complets.

## Outillage reproductible

- `npm run audit:pd2-bkvince` recalcule la matrice complète et les fingerprints;
- `npm run validate:pd2-catalog` vérifie le schéma, les références et la source
  locale figée;
- `npm run test:pd2-catalog` couvre les règles métier du validateur;
- le catalogue est validé par
  [`pd2-inspiration-bkvince.schema.json`](pd2-inspiration-bkvince.schema.json).

Le dossier PD2/SP+ et les mods de référence restent read-only. La baseline
initiale ne modifiait aucune table gameplay BKVince; l'implantation atomique
Acte III phase 1 modifie désormais seulement `hireling.txt`.

## Shiver Armor — Faster Block Rate PD2, durée et dégâts BKVince

Le 8 août 2026, Vincent a autorisé l'implantation du prototype Shiver Armor.
L'effet actif ajoute `item_fasterblockrate` par `aurastat2` avec la formule
`10 + (blvl*2)` : 12 % au niveau 1, 30 % au niveau 10 et 50 % à partir du
niveau 20. Le calcul emploie `blvl`; les bonus de niveaux ne dépassent donc pas
le plafond de 50 %. La description utilise le slot 5 avec `descline5=2`,
`desctexta5=StrFBR`, `desctextb5=StrSkill23` et la même formule dans
`desccalca5`.

La preuve gouvernée confirme que `item_fasterblockrate` existe déjà dans
`ItemStatCost.txt` à l'ordinal 102, avec `*ID=102`, `Save Bits=7` et
`Save Add=20`. Cette table n'a pas été modifiée; son SHA-256 reste
`75E032F94F89C2304ACA6A045628F0DD0F30D5EFE21B22511A4C4925894B19BB`.
Les 23 cellules BKVince qui déterminent la durée et les dégâts de Shiver Armor
ont été contrôlées avant et après l'écriture et sont byte-identiques. Les dégâts
calculés restent respectivement 6–8, 35–41,5, 79–90,5 et 209–230,5 aux niveaux
1, 10, 20 et 40.

Le différentiel est limité exactement à six cellules : deux dans `skills.txt`
et quatre dans `skilldesc.txt`. Leur remise en mémoire à l'état antérieur
reconstruit exactement les SHA-256 préimplantation
`E39BF427270743E6AEC6FEB8E484305B4CA6E6E9F933686C00AFB5BAA83DC42E` et
`05EE4F9E72C4AC8C12240A59E51BDDE0A4DD3D44B6E70C129953B5761399DF19`.
Les SHA-256 gouvernés après implantation sont respectivement
`588D455E0F1386F3791E0D5A489206BD395CB65CB23CF48820D5BFDFD68267E5` et
`E998CDB316EEFAB36F542AAD646C50BE84A199C36DEE430F27AEB553D65339F4`.
Les tables conservent leurs CRLF, leur saut de ligne final, leur forme et leur
round-trip byte-exact.

La validation statique passe : les 16 tests du rapport PD2, `verify:data`, le
contrôle ItemStatCost et le cadastre sont `VALID`. Le rapport exhaustif a été
régénéré et ne contient aucune ambiguïté de correspondance. Le déploiement
runtime a synchronisé uniquement les deux tables autorisées; le rapport
`analysis-cache/runtime-sync/20260808-214054073-apply.json` a le SHA-256
`35E1F5CC2E2C29553CD4E949E2CDA1F9F2162FD3A076705FD3AA07DA1FB082BF`
et son rollback se trouve sous
`analysis-cache/runtime-sync-backups/20260808-214054073/`.

Le cold start technique a chargé les tables avec des hashes source/runtime
identiques. Les huit plugins ayant écrit des logs frais ne signalent aucune
erreur fatale et le PluginPack a validé ses 36 opérations différées. Le log jeu
ne cite ni `skills.txt`, ni `skilldesc.txt`, ni `ItemStatCost`, ni Shiver Armor,
et ne contient aucun motif fatal lié à ce lot. Le processus a ensuite été fermé
proprement. Cette preuve établit le chargement technique avec la configuration
installée, pas encore la matrice complète de compatibilité : plusieurs fonctions
du PluginPack étaient déjà désactivées dans cette configuration.

Les gates fonctionnels restent ouverts : rendu de `StrFBR` dans l'infobulle,
mesure réelle des frames de blocage aux niveaux 1/10/20, vérification du plafond
avec bonus de niveaux, sauvegarde/respec/rechargement et témoin hôte/joiner. Le
risque de migration de sauvegarde est néanmoins faible puisque la statistique
existait déjà et qu'aucun schéma persistant n'a changé.

## Chilling Armor — block PD2 isolé, riposte BKVince conservée

Le 8 août 2026, Vincent a autorisé uniquement le premier lot Chilling Armor :
le block PD2, sans reprendre la nouvelle riposte de mêlée. L'effet actif ajoute
`toblock` par `aurastat2` avec `aurastatcalc2=5 + blvl`, soit 6 % au niveau 1,
15 % au niveau 10 et 25 % à partir du niveau 20. L'infobulle emploie le slot 4
avec `descline4=2`, `desctexta4=StrSkill110`, `desctextb4=StrSkill23` et
`desccalca4=5+blvl`; les deux clés de localisation existaient déjà dans
BKVince.

La preuve source et runtime confirme que `toblock` existait avant ce lot dans
`ItemStatCost.txt`, à l'ordinal 20 et avec `*ID=20`. Cette table n'a pas été
modifiée et conserve le SHA-256
`75E032F94F89C2304ACA6A045628F0DD0F30D5EFE21B22511A4C4925894B19BB`.
La ligne PD2 ItemStatCost, dont les paramètres de sauvegarde diffèrent, n'a donc
pas été copiée.

Le différentiel est limité exactement à six cellules : deux dans `skills.txt`
et quatre dans `skilldesc.txt`. La remise en mémoire de ces seules cellules à
vide reconstruit exactement les SHA-256 préimplantation
`588D455E0F1386F3791E0D5A489206BD395CB65CB23CF48820D5BFDFD68267E5` et
`E998CDB316EEFAB36F542AAD646C50BE84A199C36DEE430F27AEB553D65339F4`.
Après implantation, les hashes gouvernés sont respectivement
`075EDF1867079FF307FB3066C84E05EDECDBE49C8E611896630E74016DA85ADB` et
`40DDD3E0A297625F9A082C1D0AD670F5768687ED37672401D6C663E6E2B55D59`.
Les CRLF, le saut de ligne final, les dimensions et le round-trip byte-exact
sont préservés.

La durée, l'armure et les dégâts BKVince sont inchangés. Les dégâts calculés
restent 8–10, 46–52,5, 100–111,5 et 250–271,5 aux niveaux 1, 10, 20 et 40.
La riposte existante reste strictement `auraevent1=hitbymissile` avec
`auraeventfunc1=1`; `auraevent2`, `auraeventfunc2`, `LineOfSight`,
`SearchEnemyNear` et `ItemTarget` restent vides. La branche PD2 de riposte
directe en mêlée n'est donc ni partiellement ni implicitement activée.

La validation statique passe : les 17 tests du rapport exhaustif, `verify:data`,
le contrôle ItemStatCost et le cadastre sont `VALID`; le rapport régénéré couvre
603 lignes PD2 et 449 lignes BKVince sans ambiguïté, avec 5 034 différences de
cellules `Skills.txt` sur les 349 paires nominales.

Le déploiement runtime a synchronisé uniquement `skills.txt` et `skilldesc.txt`.
Le rapport `analysis-cache/runtime-sync/20260808-224505557-apply.json` a le
SHA-256
`F10E00710DF7B0ED2859626A96893DE53A058DFC7B7A79D3D96BD2A20AFE7738`;
le rollback récupérable se trouve sous
`analysis-cache/runtime-sync-backups/20260808-224505557/`. Les hashes
source/runtime des deux tables et d'ItemStatCost sont identiques après le cold
start.

Le cold start technique a terminé avec 16 plugins actifs sur 16, 18 patches
mémoire appliqués sur 18 et 36 opérations PluginPack différées validées sur 36.
Les logs frais ne contiennent aucune sévérité `ERROR`, `CRITICAL` ou `FATAL`,
aucun plugin rejeté et aucun échec non nul. Les 1 280 lignes fraîches du log jeu
ne citent ni `skills.txt`, ni `skilldesc.txt`, ni `ItemStatCost`, ni Chilling
Armor et ne contiennent aucun assert, exception, crash ou motif fatal. Les 65
avertissements de matériaux HD observés appartiennent au bruit asset déjà connu
et ne concernent pas ce lot. L'unique processus lancé pour le test a ensuite été
fermé.

Cette preuve ferme l'implantation et le chargement technique, mais pas encore la
validation fonctionnelle. Restent `not run` : rendu de l'infobulle, évolution du
blocage avec et sans bouclier aux niveaux 1/10/20, interaction dextérité/niveau
et plafond de 75 %, bonus de niveaux, transitions entre les trois armures,
sauvegarde/respec/rechargement et témoin hôte/joiner. La compatibilité complète
du PluginPack n'est pas revendiquée tant que toutes ses fonctions configurables
ne sont pas actives. Le risque de migration de sauvegarde reste faible puisque
la stat existait déjà et qu'aucun schéma persistant n'a changé.

## Static Field — package PD2 hybride D2R 3.2

Le 8 août 2026, Vincent a retenu explicitement une **DLL autonome**. Le portage
conserve donc `srvdofunc=20` dans `skills.txt` et ne copie pas le
`srvdofunc=160` propre à PD2. Le reverse engineering gouverné du build
`3.2.92777` prouve que le handler Static Field D2R commence à `0x5546B0` et ne
consomme que les calculs de vie et de rayon; le handler natif d'état/malédiction
à `0x55D6B0` consomme en revanche le target state, la durée, le filtre, le rayon
et jusqu'à six paires aura stat/calc. Le plugin appelle donc le Static Field
natif en premier, puis cet applicateur D2R avec le même skill et niveau. Les
25 % de vie, le filtrage, les statlists, la durée et l'autorité multijoueur
restent dans le moteur.

La migration réversible
`scripts/migrate-bkvince/apply-static-field-rework.js` applique et vérifie :

- dégâts de vie : `calc1=par4`, `Param4=25`;
- rayon : `"min(ln12 / 2, 14)"`, `Param1=8`, `Param2=1`, soit
  `4/8/13/14` aux niveaux `1/10/20/40`;
- résistance foudre : `lightresist` avec `"-min(lvl, 100)"`, soit
  `-1/-10/-20/-40` aux mêmes niveaux;
- durée : `125 + (5 * skill('Lightning Mastery'.blvl))`, soit 225 frames
  avec Lightning Mastery niveau 20;
- état et feedback visuel dédiés : `staticfield_debuff` dans `states.txt` et
  `overlay.txt`, sans collision avec Lower Resist.

`StaticFieldRework 0.1.0` est une DLL RuffnecKk hybride sans
`ModScopedOnly`, munie de son TOML indépendant et de signatures strictes. Le
seul hook est `0x5546B0`; le handler `0x55D6B0` est validé puis appelé sans être
patché. L'audit du PluginPack eezstreet épinglé à
`dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` ne trouve aucun chevauchement dans
les cinq DLL. D2MOO est crédité dans le README comme référence sémantique, sans
transposition d'adresse, de structure ou d'ABI 32 bits.

La compilation MSVC Release, le test de politique, l'architecture PE32+ x64,
les ressources de version et les trois exports D2RLoader sont validés. Le
SHA-256 de la DLL est
`9BCF9C96D21A21AD54BA66C8795B2900FC569348D7F30793359D1C6020DE8883`;
celui du TOML est
`734271EB84151A443DCC81CBE0CB915F76F5EA82FEA3CEC1F5703581DAEFC708`.
L'archive stricte contient uniquement ces deux fichiers et porte le hash
`D9E5849EE5BBA1ED900A6C4A055E67752CF3F87E9285B6D1371AAAEEBB7FC2FE`;
le README et les crédits restent hors ZIP.

Les validations TSV sont vertes : CRLF et round-trip byte-exact, migration
`--check`, références de démarrage et cadastre `VALID`. Les hashes finaux sont
`FF8EEC13D83183261484CFBB49E66EB79017F902BD2B5B1EA6F06A5BE3E325C1`
pour `skills.txt`,
`DA580CA0FDB0713FC62663602BB7BF81EB4D2AB0D422031901D676A2876E0940`
pour `states.txt` et
`918B3CADEC29424ED86706DCC91B5286BD063040494A6BDCE2168860DC9CAE6F`
pour `overlay.txt`.

La matrice runtime du 8 août 2026 est la suivante :

| Domaine | Cas | Statut | Preuve |
|---|---|---|---|
| Déploiement | 5 hashes source/runtime | passed | égalité avant/après cold start |
| Mod-local | DLL + TOML BKVince | passed | hook accepté, config mod-local résolue |
| PluginPack complet | 5 DLL et fonctionnalités actives | passed | `16/16`, rejet `0`, échec `0` |
| Memory patches | pile complète | passed | `18/18`, disabled `0`, failed `0` |
| Startup | D2R 3.2.92777 | passed | `24/24`, processus répondant |
| Doublon hybride | mod + global | passed | mod actif, global neutralisé; rejet `0`, échec `0` |
| Portée globale seule | DLL + TOML globaux | passed | `16/16`, `18/18`, `24/24` |
| Gameplay solo | cast, rayon, durée, shred, overlay | not run | témoin en jeu encore requis |
| Réseau | hôte/joiner | not run | témoin synchronisé encore requis |

Après les tests de portée, les deux copies globales temporaires ont été
supprimées, la DLL et le TOML mod-locaux ont été restaurés avec leurs hashes
sources, et aucun processus D2R/D2RLoader n'est resté ouvert. Le lot est donc
**implanté, compilé et qualifié au chargement dans les deux portées**, mais la
validation fonctionnelle ne sera fermée qu'après un cast observé en solo puis
un témoin hôte/joiner.

## Audit Mechanics 2.0 — clôture documentaire

Le 8 août 2026, l'Option A demandée par Vincent est fermée dans
[`pd2-game-mechanics-vs-bkvince-audit.md`](pd2-game-mechanics-vs-bkvince-audit.md).
La page officielle `Game Mechanics` est épinglée à la révision `23934`,
horodatée `2026-07-18T16:30:25Z`, encore courante au moment de l'audit. Les
treize sections de premier niveau possèdent chacune une couche source, une
preuve BKVince, une route et une disposition exacte : quatre
`baseline_only`, deux `adapt`, un `reject` et six `needs_re`.

Les décisions structurantes sont :

- conserver arrondis, leech de base, distance et mouvement comme contrats
  moteur, sans nouveau port;
- rejeter la règle dual-wield globale `IAS - WSM`, tout en conservant la
  taxonomie local/global comme contrainte à prouver;
- isoler comme futurs candidats le cap élémentaire 90 et la baseline NoDrop
  PD2 core, sans les confondre avec les breakers ni avec l'accélération SP+;
- maintenir melee splash, Critical/Deadly, Crushing Blow, Open Wounds, curse
  effectiveness et réduction de résistances dans le backlog 92777 tant que
  leurs handlers, ABI, ordre et propriétaires ne sont pas gouvernés;
- ne copier aucun ID PD2 : les collisions et les extensions hors de la plage
  BKVince `0..390` sont inventoriées dans l'audit.

Le catalogue gouverné référence désormais `Game Mechanics`, marque le chapitre
`mechanics-foundation` complet et corrige `players-five-nodrop` : la baseline
p5-like appartient au cœur PD2 et sa route est le retune data-only des poids de
Treasure Class, pas `/players5` ni un handler natif. L'accélération Single
Player Plus reste une surcouche économique distincte.

Ce lot n'a modifié aucune table BKVince, DLL, configuration, sauvegarde ou
installation runtime. Le melee déjà en cours n'a reçu aucune nouvelle
implantation et garde son propriétaire.

## Premier merge General/QoL — conservation BKVince et cap élémentaire 90

Le 8 août 2026, Vincent a autorisé le premier lot de merge gouverné après
Mechanics 2.0. Les deux décisions QoL déjà vérifiées sont fermées sans mutation
gameplay : `storage-layout` reste `keep_bkvince` parce que le stockage et
l'identité charm-inventory de BKVince dépassent la couverture PD2, et
`stackable-materials` reste `keep_bkvince` afin de préserver les codes,
compteurs, recettes et sauvegardes existants. Le catalogue consigne explicitement
ces deux clôtures; le chapitre General/QoL reste néanmoins `in_progress` pour
les candidats encore non traités.

Le seul changement gameplay de ce lot active l'option existante de
`plugin-items` dans `D2RPlugins.json` :

- `items.elementalResistCap.enabled=true`;
- `items.elementalResistCap.max=90`;
- `items.physResistCap` reste désactivé à `50`;
- `items.absorbCap` reste désactivé à `40`.

Aucune table TXT, DLL, sauvegarde ou statistique persistante n'a changé. Le
resolver d'immunité, Sunders, breakers, pierce et le patch permissif
`0x44F8F1` restent hors de ce lot. Le propriétaire natif demeure
`plugin-items`; son manifeste gouverne l'octet attendu `0x5F` au RVA
`0x4524DE`, et sa transaction échoue fermée si le site ne correspond pas.
Le SHA-256 de la configuration source et runtime est
`7F3CE0442BF8DF3A4D308D1F8E1D3DBF9E7085021A6BB696B4BAA6C6E85F8C86`.
Le runtime précédent est récupérable sous
`analysis-cache/runtime-sync-backups/20260809-001257287/`.

Les validations statiques sont vertes : catalogue gouverné, cinq tests métier,
références de démarrage BKVince et `git diff --check` ciblé. Le rapport de
synchronisation final
`analysis-cache/runtime-sync/20260809-001648988-apply.json` porte le SHA-256
`24D0C4B2C5D1CFC2B5D864D9639BE0B8B2106343EAE21352BB9C78DB8B057A29`.

La première tentative de cold start a chargé les cinq DLL PluginPack et commis
les `30/30` opérations `plugin-items`, puis a reproduit à l'étape graphique une
signature de crash préexistante, déjà observée le 8 août avant ce lot :
`dxgi.dll + 0x38B1C1` appelée depuis `plugin-items.dll + 0x8436`. Cette preuve
négative est conservée au lieu d'être masquée. La seconde tentative, lancée
avec les arguments BKVince gouvernés par défaut, atteint le frontend `24/24`,
avec build `92777`, `16/16` plugins actifs, `18/18` memory patches, aucun plugin
désactivé, rejeté ou échoué et `30/30` opérations `plugin-items` committées. Les
logs figés sont sous
`analysis-cache/pd2-elemental-cap-90/20260809-001648988/`; aucun processus
D2R/D2RLoader ne reste ouvert après la matrice.

| Domaine | Cas | Statut | Preuve |
|---|---|---|---|
| QoL | stockage et stacking BKVince | passed | décisions `keep_bkvince` vérifiées dans le catalogue |
| Déploiement | hash source/runtime | passed | SHA-256 identiques avant le second cold start |
| Chargement | build et profil mod-local | passed | `92777`, BKVince, frontend `24/24` |
| Pile installée | plugins et memory patches | passed | `16/16`, `18/18`, rejet `0`, échec `0` |
| Transaction | cap élémentaire | passed | `plugin-items` commet `30/30`; un mismatch aurait annulé la transaction |
| PluginPack toutes options | toutes fonctions configurables actives | not run | plusieurs fonctions restent volontairement désactivées dans la configuration livrée |
| Gameplay solo | base 75, bornes sous/à/au-dessus de 90, feu/froid/foudre/poison | not run | témoin équipé et observation en jeu requis |
| Non-régression | physique 50, absorb 40, immunités, Sunders, breakers et pierce | not run | matrice gameplay requise |
| Réseau | hôte/joiner | not run | témoin synchronisé requis |

Le rollback à froid consiste à remettre `elementalResistCap.enabled=false` et
`max=95`, resynchroniser ce seul JSON et refaire un cold start. Aucun ID ne doit
être réservé et aucune migration de sauvegarde n'est nécessaire. La baseline
NoDrop PD2 reste un futur lot économique distinct, sans modification dans ce
merge.

## Prochain gate

Valider en jeu le cap élémentaire avec une base à 75 et des maximums effectifs
sous, à et au-dessus de 90 pour feu, froid, foudre et poison; confirmer que le
physique reste à 50 et l'absorb à 40, puis couvrir sauvegarde/rechargement et
hôte/joiner. La compatibilité formelle « toutes fonctions PluginPack actives »
reste séparément ouverte. Ensuite reprendre General/QoL sur les prochains
candidats data-only; la baseline NoDrop PD2 exige d'abord l'inventaire complet
des Treasure Classes et une simulation économique approuvée.

## Crédits

Project Diablo 2 et ses concepts restent crédités à la Project Diablo 2 Team.
L'adaptation Single Player Plus et son dépôt restent crédités à Lukaszpg. Les
analyses, adaptations et validations BKVince sont portées par RuffnecKk, sans
effacer les crédits tiers existants.
