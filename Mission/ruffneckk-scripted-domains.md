# RuffnecKk Scripted Domains — Lua AI first

## Statut

Chantier **actif en parallèle d’ISC12** depuis le `GO` de Vincent du
1er septembre 2026. ISC12 reste la mission courante et son Save Converter garde
sa priorité de livraison; ce chantier ouvre seulement une seconde ligne de
travail gouvernée.

La première verticale est `Scripted AI`. Aucune DLL, aucun hook runtime et
aucune table gameplay n’est encore implanté. Le lot courant est le reverse
engineering statique qui doit fermer le mécanisme D2R avant l’incubation d’un
nouveau plugin autonome de la RuffnecKk D2RLoader Suite.

## Objectif

Permettre aux auteurs de mods de décrire en Lua des politiques de comportement
sans réimplémenter le moteur D2R : le script décide **quoi** tenter et une API
native typée demande au jeu **comment** exécuter l’action. L’IA constitue la
première verticale parce qu’elle expose immédiatement les exigences les plus
dures — autorité serveur, cycle `AITHINK`, ciblage, pathfinding, coût par tick,
état par unité et fallback sans gel.

À terme, la même discipline pourra couvrir d’autres domaines — événements de
combat, actions d’objets, règles de monde ou extensions UI — mais jamais sous la
forme d’un pont mémoire universel. Chaque domaine conserve sa propre DLL, son
API Lua bornée, ses permissions, sa configuration, ses gates et son rollback.

## Décision produit et séquencement

Vincent retient un séquencement parallèle :

1. ISC12 demeure la mission courante et poursuit son Save Converter;
2. `Scripted AI` ferme en parallèle ses preuves natives et son architecture;
3. l’incubation de la DLL commence seulement après fermeture de ces preuves;
4. un noyau Lua réutilisable n’est extrait qu’après un deuxième consommateur
   réel, afin d’éviter une plateforme abstraite sans besoin mesuré.

## Faits vérifiés

### Runtime D2R 3.3

- `npm run re:d2r33 -- status` vérifie le corpus canonique commun aux builds
  `3.2.92777` et `3.3.93847`, son index et le projet Ghidra. Le runtime qui devra
  être testé est D2R `3.3.93847`; l’autre build pourra seulement être couvert
  par l’équivalence native byte-exact gouvernée de chaque surface employée.
- `AITHINK_GetAiTableRecord` est à `RVA 0x4A36C0`. Son entrée stricte de
  17 octets est unique. Il possède exactement trois callers directs :
  `0x4A276A`, `0x4A27B7` et `0x4A2BD1`.
- Lorsque `nAiSpecialState` est admissible et non nul, le résolveur indexe des
  records de `0x20` octets à partir de `RVA 0x23981F0`.
- Pour l’IA normale, il lit le WORD signé `MonStatsTxt+0x52`, exige
  `0 <= AI < 0x9B`, multiplie l’index par `0x20` et retourne un record à partir
  de `RVA 0x2396E90`. La comparaison exacte à `0x4A3791` est unique.
- La table normale occupe donc exactement
  `0x9B * 0x20 = 0x1360` octets et se termine à `0x23981F0`, soit exactement le
  début de la table des special states. Une 156e entrée ajoutée en place
  écraserait la première entrée de l’autre table.
- Le dispatch lit le type de ciblage à `record+0x00`. La séquence
  d’initialisation compare le callback principal à `record+0x10`, consulte le
  callback d’entrée à `record+0x08` et le callback de transition à
  `record+0x18`. Cela confirme un record x64 de `0x20` octets sans transposer le
  layout 32 bits de D2MOO.
- Le switch natif à `0x4A2BD6` accepte exactement les catégories `0..6`. Sa
  table de saut est fermée : `0` appelle directement le callback; `1` et `3`
  passent par `0x4A69A0` et exigent une cible; `2` appelle directement le
  sélecteur `0x595750` puis continue même si la cible est nulle; `4` n’appelle
  pas le callback principal; `5` et `6` passent par `0x4A6760`, mais `5` exige
  une cible tandis que `6` continue après le fallback automatique du wrapper.
  La catégorie **2** est donc retenue pour le premier record bridge : elle
  conserve le ciblage D2R sans action de secours pré-callback susceptible de
  doubler une décision Lua.
- `AIUTIL_SelectTargetForAiThink 0x595750` reçoit
  `(Game*, Unit*, D2AiControl*, int32_t* distance, int32_t* combat,
  uint8_t context)` et retourne une cible nullable. Son corps se termine à
  `0x595F7F`; aucun de ses appels directs ne vise `EVENT_SetEvent 0x48B720` ni
  la suppression d’événement `0x48B890`. Sa signature stricte de 32 octets est
  unique.
- Le callback principal est appelé avec l’ABI x64
  `void(Game*, Unit*, D2AiTickParam*)`. Le tick préparé par le dispatch expose
  directement `D2AiControl*` à `+0x00`, la cible à `+0x10`, sa distance à
  `+0x20`, le témoin de combat à `+0x24`, `MonStatsTxt*` à `+0x28` et
  `MonStats2Txt*` à `+0x30`. Ces champs suffisent au contexte minimal du
  bridge; aucun layout complet n’est encore revendiqué.
- `AITACTICS_IdleInNeutralMode` est identifié à `RVA 0x4A6D10`. Il normalise un
  délai nul à un frame, place l’unité en mode neutre si nécessaire, supprime
  l’événement de type `2`, puis appelle `EVENT_SetEvent 0x48B720` avec
  `Game+0x170 + délai`. La séquence interne à `0x4A6D71` est unique. C’est le
  premier candidat démontré pour le garde-fou de rescheduling.
- Les primitives terminales V1 sont fermées directement dans l’image x64 :

  | Leaf Lua | Primitive D2R | ABI utile | Contrat statique |
  |---|---|---|---|
  | `attackTarget` | `AITACTICS_ChangeModeAndTargetUnit 0x4A78E0` | `(Game*, Unit*, mode, target)` | retourne l’acceptation du changement de mode; aucun idle interne |
  | `castOnTarget` | `AITACTICS_UseSkill 0x4A7BC0` | `(Game*, Unit*, mode, skillId, target, x, y, flag)` | retourne `1` si accepté; sur échec, idle natif de `10` frames |
  | `chaseTarget` | `AITACTICS_WalkToTargetUnitWithFlags 0x4A8740` | `(Game*, Unit*, target, uint16_t flags)` | avec `flags=0`, retourne l’acceptation; aucun fallback interne |
  | `retreatFromTarget` | `D2GAME_AICORE_Escape 0x4A7DF0` | `(Game*, Unit*, target, uint8_t distance, int32_t deleteAiEvent)` | retourne l’acceptation; aucun idle interne |
  | `wander` | `AITACTICS_WalkCloseToUnit 0x4A8320` | `(Game*, Unit*, uint8_t radius)` | choisit des coordonnées proches et retourne l’acceptation; aucun idle interne |

  Les cinq signatures d’entrée sont uniques dans `.text`. Les déplacements
  `chase` et `wander` convergent vers `AITACTICS_MoveToTarget 0x4A8A10`; avec
  les flags V1 à zéro, un rejet remonte comme `0` sans action cachée.
- Le contrat de continuation statique ne consiste pas à ajouter un second
  `AITHINK` après tout appel. Une action native acceptée remet l’unité au
  pipeline de modes du jeu, qui possède alors la suite. Le wrapper C++ appelle
  `AITACTICS_IdleInNeutralMode` seulement lorsqu’une primitive retourne faux,
  lorsqu’aucune leaf ne s’engage ou lorsque Lua échoue. `UseSkill` possède déjà
  ce fallback et ne doit pas en recevoir un deuxième. Ce contrat rend le
  rescheduling obligatoire à la frontière native sans demander aux auteurs Lua
  de le programmer.
- Le `monai.txt` officiel 3.3 contient les 155 records attendus et est
  byte-identique à sa copie `base/monai.txt` : SHA-256
  `7c7c7b8866c46078356bcc6118789699351004f39edea92922425699d6aa86a8`,
  CRLF et round-trip gouverné exacts. Les tables natives résident dans la zone
  virtuelle non adossée aux octets bruts du PE; leur catégorie par ligne n’est
  donc pas extraite du fichier, et cette corrélation n’est pas nécessaire au
  choix démontré de la catégorie 2.
- Le SDK PluginSDK v4 épinglé à
  `6eb8f8b6192868214706bd6d528c5294f2f551b7` expose
  `CustomTableServiceV1`, `LifecycleServiceV1` et
  `DataTableServiceV1::TableId::MonAi`. Une table custom peut compiler après
  les tables stock, être remplacée par le mod actif, publier une révision et
  fournir des copies de rows sans exposer Fog ni le `DataTables` natif.
- Le SDK permet de **consommer** les services D2RLoader mais ne publie pas de
  registre où une DLL tierce pourrait fournir un nouveau service aux autres
  plugins. Un runtime Lua partagé entre DLL n’est donc pas retenu en V1.

### Référence D2MOO 1.10f

La référence épinglée
`D2MOO@19019806df7f3e877fa105b05395d1e3597e2316` confirme seulement la
sémantique :

- `source/D2Game/src/AI/AiThink.cpp:17051-17250` décrit le résolveur, la table
  normale, la table des special states et le fallback vers l’entrée zéro;
- `source/D2Game/src/AI/AiThink.cpp:17253-17408` décrit l’initialisation du
  callback, le ciblage et l’appel du think;
- `source/D2Game/src/AI/AiTactics.cpp:31-88,172-178,214-265,305-369,454-565`
  confirme la sémantique des wrappers de ciblage, mode, cast, idle, marche,
  fuite et errance;
- `source/D2Game/src/AI/AiUtil.cpp:671-864` confirme la sémantique du sélecteur
  de cible nullable;
- `source/D2Game/include/AI/AiGeneral.h:19-29` décrit le tick legacy;
- `source/D2Game/include/AI/AiGeneral.h:80-86` décrit le record legacy.

Aucune adresse, largeur de pointeur, structure ou ABI 32 bits n’est transposée
vers D2R. Chaque correspondance ci-dessus possède sa preuve indépendante dans
l’image x64 gouvernée.

### Plugin Lua de NpZ1k

L’archive locale `lua-plugins.zip` est indépendante de Harvest. Elle contient
une DLL binaire, Lua 5.4 et des exemples capables de résoudre des RVA, lire et
écrire la mémoire, définir des structures, appeler des pointeurs, installer des
hooks et appliquer des patches. Elle ne contient ni source native ni licence.

Cette archive prouve qu’un cycle `DLL → Lua → fonction D2R` peut être pratique
comme workbench. Elle n’est toutefois ni une dépendance, ni une base de sécurité
pour Scripted Domains : les scripts y portent directement les RVA, expected
bytes et layouts, et peuvent écrire arbitrairement dans le processus. Son rôle
reste donc **oracle d’ergonomie et prototype isolé**, jamais runtime de
production RuffnecKk.

## Hypothèses à tester

- Un hook unique du résolveur à `0x4A36C0`, limité à
  `nAiSpecialState == 0`, peut retourner un record bridge stable pour les seules
  classes présentes dans `AIScript`, puis appeler l’original pour tout le reste.
- Une table custom `aiscript` peut relier un ID physique `MonStats` à un nom de
  script, un profil de ciblage et un fallback, sans ajouter une ligne à
  `monai.txt` ni modifier les AIs stock.
- Le callback bridge peut recevoir le `D2AiTickParam` préparé par le moteur,
  exposer un handle éphémère Lua, exécuter un arbre de comportement borné et
  remettre la continuation au pipeline natif après une action acceptée, ou à
  l’idle natif après rejet, absence d’action ou erreur Lua.
- Un budget par think — allocator borné, instruction hook et limite temporelle
  diagnostique — maintiendra un coût acceptable à la fréquence réelle des
  événements AI plutôt qu’à la fréquence de rendu.

## Inconnues encore ouvertes

- Les champs et invariants de `D2AiTickParam` au-delà du sous-ensemble minimal
  directement nécessaire au bridge.
- Le cycle exact de destruction/despawn nécessaire en plus des événements de
  mort et du changement de session pour purger l’état Lua par GUID.
- La preuve runtime que chaque action acceptée reprend effectivement un think
  après son pipeline de mode, sans gel ni double décision; le statique ferme la
  politique de fallback, pas ce témoin gameplay.
- Les collisions runtime avec les hooks actifs de la RuffnecKk Suite et des
  cinq plugins eezstreet. L’audit textuel courant ne trouve aucun propriétaire
  de `0x4A36C0`, mais le diagnostic D2RLoader reste requis avant implantation.

## Approches viables

### A — Ajouter une 156e entrée native comme en 1.10f

Cette approche reproduirait le design Harvest : nouvelle ligne `LuaAI`, borne
élargie et nouvelle entrée de table. Elle est simple conceptuellement et laisse
`MonStats.AI` choisir le bridge.

Elle est **rejetée pour la verticale D2R actuelle**. Le corpus prouve que la
table normale de 155 records touche immédiatement la table des special states.
Élargir seulement `CMP ..., 0x9B` ferait lire la première special state comme
nouvelle AI; écrire une entrée à cet endroit la corromprait. Relocaliser les
deux tables et tous leurs consommateurs serait plus large, plus fragile et plus
difficile à rendre compatible qu’un hook central réversible.

### B — Intercepter le résolveur central avec fallback stock

Le hook appelle l’original lorsque le special state est non nul, lorsque la
table custom n’est pas prête ou lorsque la classe n’est pas liée. Pour une
classe opt-in, il retourne l’un des records bridge possédés par la DLL. Le jeu
continue de préparer le tick, sélectionner la cible selon la catégorie du
record et appeler le callback x64 habituel.

Cette approche conserve les 155 AIs, n’altère ni `monai.txt` ni `MonStats.AI`,
supporte un déploiement faisant strictement rien par défaut et offre un rollback
par retrait d’un hook unique. Elle devient l’**approche recommandée**, sous
réserve de fermer le lifecycle GUID, l’ownership et l’empreinte complète.

### C — Hooker chaque AI ou chaque action séparément

Cette approche pourrait modifier uniquement certains hirelings, minions ou
monstres existants et réutiliser plus de logique stock.

Elle multiplie les hooks, les ownerships et les interactions avec
ReviveOverhaul, RogueScoutMovement et les futurs plugins. Elle ne fournit pas
une surface déclarative générale et son coût de maintenance croît avec chaque
comportement. Elle reste utile pour un correctif très local, pas pour la
plateforme Scripted Domains.

## Architecture recommandée — Verticale Scripted AI

### 1. Opt-in par données

`CustomTableServiceV1` enregistre une table plugin-owned `aiscript` dans les
banks Base et RotW. La V1 envisagée contient au minimum :

- `MonStatsId` — clé physique explicite;
- `Script` — nom ASCII borné, sans chemin arbitraire;
- `TargetProfile` — catégorie bridge admise par allowlist;
- `FallbackAi` ou une politique de fallback explicitement bornée;
- `Enabled`.

Une table absente, vide, invalide ou non prête signifie **zéro unité scriptée**.
Les rows sont copiées au callback `DataTablesLoaded`, validées entièrement puis
publiées atomiquement sous une génération immuable.

### 2. Hook minimal

La DLL possède `AITHINK_GetAiTableRecord 0x4A36C0`. Son hook :

1. rejette tout pointeur ou état invalide vers l’original;
2. délègue toujours les special states non nuls;
3. consulte une map immuable `MonStatsId → binding`;
4. retourne l’original si aucune liaison n’existe;
5. retourne un record bridge statique si la liaison et son script sont valides.

Les autres plugins ne dépendent pas de cette DLL. La première implantation doit
être une nouvelle DLL autonome, hybride globale/mod-locale, versionnée et
configurée indépendamment conformément au contrat RuffnecKk Suite.

### 3. API Lua par capacités

Lua ne reçoit jamais un RVA, un pointeur natif, `read_*`, `write_*`,
`patch_*`, `call_ptr`, `ffi`, `os`, `io`, `debug` ou `package.loadlib`.
Le callback crée un handle éphémère valide pour un seul think. Les getters
retournent des copies; les mutations possibles sont une petite allowlist de
méthodes telles que :

- `m:idle(frames)`;
- `m:wander(radius)`;
- `m:attackTarget()`;
- `m:chaseTarget()`;
- `m:retreatFromTarget(distance)`;
- `m:castOnTarget(skillId)`.

Chaque méthode valide l’unité, le contexte serveur, la cible, les bornes et les
droits du script avant d’appeler un helper D2R prouvé. Le moteur conserve le
pathfinding, les modes, les collisions, l’attaque et le cast.

### 4. Arbre de comportement et garde-fous

Les nodes Lua sont des tables déclaratives parcourues par un tick unique. Les
leaves standard évitent que chaque auteur réécrive les closures critiques. La
frontière native impose :

- une seule action terminale engagée par think;
- une action acceptée délègue exclusivement la continuation au pipeline de
  modes natif;
- fallback vers l’idle natif si toutes les feuilles échouent ou si une
  primitive sans fallback interne retourne faux;
- fallback natif identique après erreur Lua, timeout ou script absent;
- aucun second idle après le fallback interne de `UseSkill`;
- aucune exception C++ à travers l’ABI D2R;
- profondeur, instructions, mémoire et volume de logs bornés;
- RNG D2R ou RNG déterministe gouverné, jamais `math.random` non synchronisé.

L’état persistant de comportement est une table Lua indexée par
`{sessionGeneration, unitType, unitGuid}`. Il est vidé sur mort prouvée,
despawn prouvé et changement de session. La V1 ne sérialise aucun état Lua dans
les sauvegardes.

### 5. Autorité et multijoueur

Le think D2GAME est traité comme serveur autoritaire : seul l’hôte décide les
actions. Les scripts et bindings portent un hash de contenu. La matrice TCP/IP
doit prouver au minimum matching host/joiner, mismatch refusé fail-closed avant
gameplay scripté, join/rejoin et absence de double exécution côté client.

## Extension future à d’autres domaines

Après une verticale AI réellement validée, un deuxième plugin peut réutiliser
le même modèle sans importer les pouvoirs mémoire du workbench NpZ1k :

- **Combat Events** — contexte copié pour hit/kill/cast et réponses natives
  bornées;
- **Item Actions** — s’appuyer d’abord sur `ItemServiceV1` et
  `ItemInteractionServiceV1` plutôt que sur des hooks bruts;
- **World Rules** — callbacks de session, quête ou événement avec autorité hôte;
- **UI Scripts** — widgets et panels via les services SDK, sans pointer ImGui ou
  renderer directement depuis Lua.

Le noyau partagé éventuel sera une bibliothèque statique versionnée, embarquée
dans chaque DLL, tant que PluginSDK ne permet pas la publication sûre d’un
service inter-plugin tiers. Une DLL Lua globale omnipotente n’est pas prévue.

## Gates observables

### RE-AI-1 — Résolveur et record

- [x] corpus 92777/93847 vérifié;
- [x] résolveur, trois callers, borne 155, stride `0x20` et deux bases prouvés;
- [x] impossibilité de l’append in-place prouvée par l’adjacence exacte;
- [x] layout minimal `D2AiTickParam` et ABI du callback fermés;
- [x] catégories `0..6` fermées et catégorie bridge `2` retenue;
- [ ] empreinte fail-closed complète du hook et de chaque témoin;
- [ ] audit statique et runtime de l’ownership de `0x4A36C0`.

### RE-AI-2 — Actions et cycle AITHINK

- [x] premier fallback `AITACTICS_IdleInNeutralMode 0x4A6D10` identifié;
- [x] helpers attaque/chase/retraite/errance/cast identifiés avec ABI et retours;
- [x] contrat statique de continuation/fallback prouvé pour chaque leaf;
- [ ] mort, despawn, changement de session et réutilisation de GUID fermés.

### INC-AI — Incubation plugin

- [ ] utiliser `d2rloader-plugin-incubation` avant le premier fichier source;
- [ ] figer le nom produit, la description anglaise courte et le TOML anglais;
- [ ] intégrer un runtime Lua avec provenance, licence et surface sandboxée;
- [ ] construire deux Release byte-identiques avec `/W4 /WX` et tests;
- [ ] vérifier métadonnées, exports, API SDK, ownership et rollback.

### RUN-AI — Qualification

- [ ] cold start default-off avec pile complète et cinq plugins eezstreet;
- [ ] un monstre opt-in, puis un hireling, sans modifier les unités non liées;
- [ ] toutes les erreurs Lua tombent sur le fallback sans gel;
- [ ] benchmark think/s, p50/p95/p99 et budget par tick sur packs denses;
- [ ] mort/despawn/reload sans état fantôme;
- [ ] TCP/IP matching/mismatch, autorité hôte et absence de double exécution;
- [ ] scripts modifiables sans rebuild DLL, avec reload seulement si le contrat
  de génération et de rollback le permet.

## Mesures de succès

- `0` hook en dehors du résolveur et des helpers strictement requis par la V1;
- `0` unité non liée entrant dans Lua;
- `0` gel après erreur, absence d’action ou script invalide;
- coût p99 par think sous le budget fixé avant qualification;
- même hash de script et mêmes décisions autoritaires en TCP/IP;
- ajout d’un second comportement par données et Lua sans rebuild de la DLL;
- aucun RVA, pointeur ou primitive mémoire exposé aux scripts de production.

## Validation et rollback

Le reverse engineering est documentaire et ne touche pas le runtime. Après
incubation, le rollback doit rester : retirer la DLL et sa configuration, ou
laisser `aiscript` vide, puis redémarrer. Les 155 AIs stock et les special states
doivent alors redevenir byte-for-byte le seul chemin exécuté. Aucun format de
sauvegarde n’est modifié par la V1.

## Frontière Git et prochain gate

Le workstream possède cette mission, le futur
`addons/RuffnecKkScriptedAI/**`, les futurs scripts dédiés
`scripts/reverse-engineering/d2r33-scripted-domains*` et, si nécessaire, un
répertoire de preuves `reverse-engineering/d2r-3.2.92777/scripted-domains/**`.
`ROADMAP.html`, `Mission/WORKSTREAMS.json`, `known-rvas.json`, `findings.md` et
le cadastre restent des registres partagés.

**Prochain gate :** fermer le lifecycle GUID (mort, despawn, changement de
session et réutilisation), l’ownership réel de `0x4A36C0`, les budgets et
l’empreinte fail-closed complète. Lancer ensuite seulement le skill
d’incubation de la nouvelle DLL. Aucun commit, push, déploiement ou test runtime
n’est autorisé implicitement par ce document.
