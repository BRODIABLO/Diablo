# PD2 / Single Player Plus — inspiration gouvernée pour BKVince

Dernière mise à jour : 8 août 2026

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

## Sources figées

Le dossier local Single Player Plus correspond byte-exactement au dépôt public
à la révision suivante :

- dépôt : <https://github.com/Lukaszpg/PD2-Single-Player-Plus-mod>;
- commit : `3debc6781f33c3c1474a995b80369a4e618cd386`;
- tree : `6f51e17e5f65abdd50b2fd33190c571fef296ccf`;
- 198 fichiers contrôlés, sans écart local;
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

L'inventaire est terminé. Le chapitre général/QoL est en cours; les autres
chapitres restent explicitement planifiés et ne sont pas déclarés complets.

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

## Prochain gate

Déployer et valider en jeu l'Acte III phase 1 : les trois auras, leurs cycles de
vie, leurs interactions avec l'équipement et l'absence de régression sur les
casts. La validation réseau hôte/joiner reste obligatoire avant de déclarer le
lot livré. Les masteries élémentaires et l'UI étendue demeurent la phase 2 et un
chantier ultérieur distinct.

## Crédits

Project Diablo 2 et ses concepts restent crédités à la Project Diablo 2 Team.
L'adaptation Single Player Plus et son dépôt restent crédités à Lukaszpg. Les
analyses, adaptations et validations BKVince sont portées par RuffnecKk, sans
effacer les crédits tiers existants.
