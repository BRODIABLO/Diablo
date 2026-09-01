# RuffnecKk Scripted Domains — Lua AI first

## Statut

Chantier **actif en parallèle d’ISC12** depuis le `GO` de Vincent du
1er septembre 2026. ISC12 reste la mission courante et son Save Converter garde
sa priorité de livraison; ce chantier ouvre seulement une seconde ligne de
travail gouvernée.

La première verticale est `Scripted AI`. Aucun hook runtime ni chemin gameplay
n’est encore implanté. Les gates de conception natifs, d’incubation et de
transaction données/lifecycle sont fermés. RuffnecKk Scripted AI `0.2.0`
enregistre maintenant sa table privée `aiscript`, confine et copie les sources,
puis publie une génération Lua seulement après attestation du thread hôte. Le
tick de l’arbre et toute capacité d’action restent volontairement absents
jusqu’au gate `EXEC-AI-1`.

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

## Décision d’incubation — 1er septembre 2026

- Identité produit figée au jalon d’incubation `0.1.0` : **RuffnecKk Scripted
  AI**, plugin ID
  `ruffneckk-scripted-ai`, DLL `d2rl-ruffneckk-scripted-ai.dll` et configuration
  indépendante `ruffneckk-scripted-ai.toml`.
- Description publique : `Lets configured monsters run bounded Lua behavior
  trees.` Auteur : exactement `RuffnecKk`.
- La DLL est un composant autonome permanent de la RuffnecKk D2RLoader Suite,
  installable globalement ou dans un mod sans `ModScopedOnly`. Elle ne dépend
  d’aucune DLL tierce, ne modifie ni ne redistribue les plugins eezstreet et ne
  prévoit aucun merge dans leur PluginPack.
- Le rôle API v3 est `Server | NativeHooks`. Une installation distante cliente
  n’instancie aucune VM gameplay; l’hôte local demeure le serveur autoritaire.
- La configuration est recherchée dans le support du mod actif avant la portée
  de la DLL puis le dossier global. Elle est strictement refusée si elle existe
  mais est invalide. `enabled = false` est la valeur livrée : aucun hook ni VM
  ne sont créés par défaut.
- Le runtime retenu est **PUC Lua 5.4.9**, lié statiquement depuis le tarball
  officiel `https://www.lua.org/ftp/lua-5.4.9.tar.gz`, SHA-256
  `2335b6c582a52654f94612bf10d2f4672805d05329aa6568b1d8cd9e5c6fb8e6`,
  sous licence MIT. La branche 5.4 est choisie à sa dernière maintenance plutôt
  que la jeune 5.5 : ses hooks d’instructions, son allocateur hôte et son API C
  couvrent entièrement le besoin sans migration de langage supplémentaire.
- `lua-plugins.zip` de npz1k, SHA-256
  `6099782C2A6541697C5FF3F7BD3BAB5635B64189E137427EE49C9C7680EF4B35`, est une
  référence binaire externe distincte. Aucun code, pointeur, DLL, script ou
  configuration de cette archive n’est copié, lié ou redistribué.
- Cette incubation matérialise le chargement default-off, l’empreinte native,
  le parseur TOML, la sandbox et leurs tests. Elle n’autorise encore ni binding
  `aiscript`, ni remplacement AI, ni déploiement, ni qualification gameplay.

## Résultat d’incubation — 1er septembre 2026

- Deux compilations Release x64 indépendantes avec MSVC `19.44.35228`,
  `/W4 /WX` pour les sources RuffnecKk et le Windows SDK `10.0.26100` produisent
  une DLL byte-identique de `356352` octets, SHA-256
  `E0E0CBD5CE5B1776E65FDD7F15B01FC8C8D38142559D0CAB59ECF4233DCCB6CC`.
- Les deux arbres de build passent chacun `CTest 1/1`. La suite couvre le TOML
  strict et sa précédence, les 22 fenêtres PE exactes et uniques, les refus
  d’empreinte et d’ownership, la sandbox, les limites arbre/heap/instructions,
  le rejet du bytecode et la politique source sans hook ni allowlist de build.
- La DLL est PE32+ AMD64 et exporte exactement `D2RLoaderGetPluginInfo`,
  `D2RLoaderLoadPlugin` et `D2RLoaderUnloadPlugin`. Son manifeste encode l’API
  v3, son `VERSIONINFO` porte les métadonnées RuffnecKk `0.1.0` arrêtées et le
  TOML embarqué est byte-exact au fichier de `1052` octets, SHA-256
  `8E860BD445B416F6FED05481B2F05F00472EECCA286CE3848DFAA38F5F609BE1`.
- PUC Lua `5.4.9` est lié statiquement. La VM n’ouvre que les bibliothèques
  bornées retenues, retire notamment les loaders, modules natifs, I/O, OS,
  debug, coroutine, chaînes, UTF-8, RNG et les mécanismes permettant de capturer
  une erreur de budget. Les scripts sont texte seulement et l’arbre déclaratif
  refuse métatables, cycles, partage de nodes et dépassements de plafonds.
- Dans ce jalon `0.1.0`, `enabled = false` ne vérifie aucune surface native,
  n’instancie aucune VM et n’installe aucun hook. `enabled = true` effectue le
  preflight service/ownership/empreinte et sandbox, puis refuse proprement l’activation
  tant que le bridge n’existe pas. Il n’y a donc encore aucun chemin gameplay.
- Aucun déploiement, lancement D2R, test gameplay, ZIP, tag, commit ou push ne
  fait partie de cette fermeture. L’attestation post-installation d’ownership
  et les mesures de performance restent des gates runtime futurs.

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
- La baseline PluginSDK API v3 de la Suite, épinglée à
  `4933e2c42cb2592958cd0df3b6dc5003102252d1`, expose
  `CustomTableServiceV1`, `LifecycleServiceV1` et
  `DataTableServiceV1::TableId::MonAi`. Une table custom peut compiler après
  les tables stock, être remplacée par le mod actif, publier une révision et
  fournir des copies de rows sans exposer Fog ni le `DataTables` natif.
- Le SDK permet de **consommer** les services D2RLoader mais ne publie pas de
  registre où une DLL tierce pourrait fournir un nouveau service aux autres
  plugins. Un runtime Lua partagé entre DLL n’est donc pas retenu en V1.
- Cette même baseline fournit le contrat d’autorité requis sans canal réseau
  custom : `PluginFlags::Server` désigne les règles gameplay et les sauvegardes,
  l’hôte remplit le rôle serveur, et `ThreadServiceV1::runOnGameThread` est la
  seule file de travail gameplay autoritaire. Elle retourne `Unavailable` sur
  un client TCP/IP distant. Scripted AI sera donc `Server | NativeHooks`, jamais
  `Shared`; un client distant ne possède ni VM Lua autoritaire ni décision AI.
- `LifecycleServiceV1` publie `GameJoined`, `GameLeft` et un
  `sessionGeneration` qui invalide les handles de l’ancienne partie. Les
  callbacks gameplay étant UI-thread, ils ne touchent jamais Lua : ils publient
  seulement une génération atomique et demandent une activation unique via
  `runOnGameThread`. Cette demande retourne `Unavailable` sur un client distant;
  sur l’hôte, son callback capture le thread autoritaire et détruit/recrée le
  contexte de session. Tout think reçu avant cette attestation délègue à l’AI
  stock sans entrer dans Lua.
- `DiagnosticsServiceV1::queryHookStatus` distingue une plage vanilla, un hook
  D2RLoader suivi et une mutation inconnue, avec nombre et identifiant des
  propriétaires. Le hook du résolveur doit être `Unchanged/ownerCount=0` avant
  installation, puis `Tracked/InlineHook/ownerCount=1` et possédé par le propre
  `PluginInfo.id`; le même contrôle est répété au premier think de chaque
  session avant d’autoriser Lua.
- L’audit source/patch complet ne trouve aucun autre propriétaire de
  `0x4A36C0`. Un snapshot read-only du runtime officiel `3.3.93847` démarré le
  1er septembre 2026, après chargement de 36 plugins, des cinq eezstreet et de
  17 memory patches, retrouve les préfixes vanilla exacts aux douze entrées
  résolveur/dispatch/catégorie/handoff/sélecteur/helpers inspectées. Cela ferme
  l’ownership de la pile installée observée; le futur cold start devra encore
  attester que le propriétaire suivi après installation est exclusivement la
  nouvelle DLL.

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
- Les plafonds V1 ci-dessous sont des limites de sécurité arrêtées avant
  incubation. Leur performance reste à mesurer sur le runtime officiel; un
  benchmark insuffisant bloque la release au lieu d’autoriser silencieusement
  davantage de travail par think.

## Inconnues encore ouvertes

- Les champs et invariants de `D2AiTickParam` au-delà du sous-ensemble minimal
  directement nécessaire au bridge.
- La preuve runtime que chaque action acceptée reprend effectivement un think
  après son pipeline de mode, sans gel ni double décision; le statique ferme la
  politique de fallback, pas ce témoin gameplay.
- Le coût p50/p95/p99 réel du bridge et le plafond de densité acceptable sur
  `100/250/500` unités opt-in.
- Le comportement observé solo, hôte et client distant sous le rôle `Server`,
  notamment l’absence de double exécution et le rejoin.

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

La V1 ne possède **aucun état persistant par unité** et n’expose ni GUID,
pointeur, identité stable, `tostring` d’un userdata ni clé équivalente. Un
handle est un userdata éphémère portant `{sessionGeneration, thinkToken}`; il
est invalidé dans tous les chemins de sortie du `lua_pcall` et toute méthode
appelée ensuite refuse sans toucher D2R. Une closure peut conserver un état
global au script, mais seulement dans la VM bornée de la session et sans moyen
supporté de le rattacher à l’identité d’un monstre.

Cette réduction de surface ferme mort, despawn et réutilisation de GUID sans
ajouter de hook : aucune donnée par unité ne survit au think. `GameLeft` invalide
immédiatement les handles par génération; l’ancienne VM est réclamée sur le
prochain game-thread autoritaire ou au teardown gouverné. Un futur besoin réel
de blackboard par unité rouvrira un gate séparé et devra alors prouver mort,
despawn, réutilisation et purge. La V1 ne sérialise aucun état Lua dans les
sauvegardes.

### 5. Autorité et multijoueur

Le think D2GAME est traité comme serveur autoritaire : seul l’hôte décide les
actions. La DLL déclare `PluginFlags::Server | PluginFlags::NativeHooks`; elle
ne crée aucun canal réseau et n’exige pas que le client possède les mêmes
scripts, puisque le client ne décide rien. Les scripts, bindings et budgets de
l’hôte portent néanmoins un hash de contenu journalisé et attaché aux preuves
de session. Lua n’est activé que lorsque le callback
`runOnGameThread` de la génération courante a effectivement tourné et que le
think arrive sur ce même thread; `Unavailable`, génération différente ou thread
différent signifie fallback stock. La matrice TCP/IP doit prouver hôte
avec/sans bindings, client avec DLL absente ou configuration différente,
join/rejoin et surtout zéro invocation Lua autoritaire côté client distant.

### 6. Budgets V1 arrêtés

Tous les plafonds sont fail-closed et non contournables par le TOML public :

| Ressource | Plafond V1 | Réaction |
|---|---:|---|
| source Lua texte seulement | `256 KiB` par script | binding rejeté avant publication |
| arbre déclaratif | `256` nodes, profondeur `32`, `32` enfants/composite | script rejeté au chargement |
| heap Lua de session | `16 MiB` via allocator comptable | erreur Lua, fallback natif |
| croissance heap par think | `64 KiB` | allocation refusée, fallback natif |
| instructions par think | `25 000`, hook de contrôle chaque `500` | abort Lua, fallback natif |
| action terminale | `1` par think | seconde action refusée |
| mur diagnostique Lua | strike au-delà de `2 ms` hors helper natif | quarantine après `3` strikes/session |
| erreurs de script | `3` par session | binding désactivé, retour à l’AI stock |
| logs détaillés | `1` par script toutes les `5 s` | agrégation des répétitions |

Le chargement possède un budget distinct de `250 000` instructions et ne
publie une génération que si tous les scripts et rows sont valides. Le runtime
retire `io`, `os`, `debug`, chargement binaire, modules natifs et toute fonction
mémoire. La mesure de release exige p99 Lua/bridge hors helper natif
`<= 50 us/think` et une enveloppe agrégée `<= 2 ms` par update serveur de
`40 ms` dans le scénario dense retenu. Ces deux valeurs sont des critères
d’acceptation, pas des résultats déjà observés.

### 7. Empreinte native V1 complète

L’empreinte arrêtée contient 22 fenêtres instruction-aligned, exactes et
uniques dans `.text`; toute différence désactive entièrement Scripted AI avant
le premier appel Lua :

| Contrat | Fenêtres RVA / taille |
|---|---|
| identité du monstre | `UNITS_GetClassId 0x349860/46`, `UNITS_GetUnitType 0x34B9D0/45` |
| tick minimal | `0x4A2ADA/117`, SHA-256 `85EE58C5…00F5FA9` |
| dispatch bridge | `0x4A2BD6/28`, `0x4A2C7A/38`, `0x4A2CED/19` |
| résolveur hooké | `0x4A36C0/17`, special lookup `0x4A3767/32`, normal lookup `0x4A3787/63` |
| ciblage | `AIUTIL_SelectTargetForAiThink 0x595750/32` |
| idle | entrée `0x4A6D10/33`, reschedule `0x4A6D71/16` |
| attaque | entrée `0x4A78E0/32`, pipeline/retour `0x4A7900/40` |
| cast | entrée `0x4A7BC0/28`, succès/fallback `0x4A7C9F/70` |
| retraite | entrée `0x4A7DF0/27`, appel terminal `0x4A7F1D/35` |
| errance | entrée `0x4A8320/34`, appel terminal `0x4A84A0/64` |
| chase | fonction complète `0x4A8740/39` |
| mouvement commun | garde d’entrée `0x4A8A10/62` |

Les tailles `32/28/34/39/62` corrigent respectivement les anciennes fenêtres
attaque/cast/errance/chase/mouvement qui étaient trop courtes; certaines
s’arrêtaient au milieu d’une instruction et les anciennes longueurs chase et
mouvement avaient respectivement `2` et `5` matches. Les nouvelles fenêtres
sont toutes uniques. Seule la plage `0x4A36C0/17` est possédée par le hook; les
21 autres restent des témoins read-only vérifiés avant son installation.

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
- [x] empreinte fail-closed complète de 22 fenêtres instruction-aligned;
- [x] audit statique et snapshot runtime de l’ownership de `0x4A36C0`;
- [ ] attestation post-installation du propriétaire D2RLoader unique (INC/RUN).

### RE-AI-2 — Actions et cycle AITHINK

- [x] premier fallback `AITACTICS_IdleInNeutralMode 0x4A6D10` identifié;
- [x] helpers attaque/chase/retraite/errance/cast identifiés avec ABI et retours;
- [x] contrat statique de continuation/fallback prouvé pour chaque leaf;
- [x] V1 sans état/identité par unité; génération de session et handles
  éphémères ferment mort, despawn et réutilisation sans hook supplémentaire.

### RE-AI-3 — Sandbox, budgets et autorité

- [x] rôle `Server | NativeHooks`, zéro décision Lua sur client TCP/IP distant;
- [x] VM session-scoped, génération atomique et aucune mutation Lua UI-thread;
- [x] plafonds source/arbre/heap/instructions/actions/temps/logs arrêtés;
- [x] aucune primitive mémoire, FFI, module natif ou bytecode externe;
- [ ] mesures p50/p95/p99 et densité réelle (RUN-AI).

### INC-AI — Incubation plugin

- [x] utiliser `d2rloader-plugin-incubation` avant le premier fichier source;
- [x] figer le nom produit, la description anglaise courte et le TOML anglais;
- [x] intégrer un runtime Lua avec provenance, licence et surface sandboxée;
- [x] construire deux Release byte-identiques avec `/W4 /WX` et tests;
- [x] vérifier métadonnées, exports, API SDK, ownership et rollback statiques.

### BRIDGE-AI-1 — Transaction données et lifecycle, sans hook

- [x] enregistrer le schéma plugin-owned `aiscript` via
  `CustomTableServiceV1`, dans les banks Base et RotW, sans toucher
  `monai.txt` ni `MonStats.AI`;
- [x] copier et valider strictement chaque row, résoudre un script uniquement
  sous la racine configurée et compiler tout le lot dans une génération
  immuable avant publication;
- [x] rejeter atomiquement le lot complet si une row, un chemin, une source ou
  un arbre est invalide; conserver la génération précédente si elle existe;
- [x] relier `GameJoined`, `GameLeft`, `sessionGeneration` et
  `runOnGameThread` sans appeler Lua depuis le thread UI ni créer de VM sur un
  client TCP/IP distant;
- [x] prouver par tests les transitions de génération, l’annulation tardive,
  le rollback de publication, les configurations hôte/client asymétriques et
  la destruction de toute VM périmée;
- [x] conserver l’appel au résolveur, l’installation du hook, les actions
  natives et tout déploiement hors de ce gate.

Gate fermé le 1er septembre 2026 par RuffnecKk Scripted AI `0.2.0` : deux
builds Release x64 indépendants produisent la même DLL de `416256` octets,
SHA-256 `55AACEB372991F022F9F2CC0BA50CA6DD80BF0753BC5949D904D63FEDFB7FB00`,
et les deux suites CTest passent `1/1`. Le schéma `aiscript` mesure `76` octets,
les ressources Base/RotW par défaut sont header-only, les copies sont liées à
leur révision et la compilation Lua n’arrive qu’après attestation locale par
`runOnGameThread`. Les tests couvrent lot invalide, rollback, remplacement et
réclamation de VM, annulation tardive et client distant sans VM. La DLL contient
toujours exactement trois exports et aucun appel `InstallInlineHook`; aucun
déploiement ni démarrage D2R n’a eu lieu.

### EXEC-AI-1 — Évaluateur Lua et intentions simulées, sans hook

- [ ] figer la sémantique V1 des nodes `selector`, `sequence` et des leaves
  déclaratives ainsi que le résultat explicite `action` ou `fallback`;
- [ ] exécuter l’arbre retenu par la génération via un handle de think
  éphémère simulé, avec exactement une intention terminale au maximum;
- [ ] faire converger absence d’action, leaf refusée, erreur Lua, budget
  d’instructions ou d’allocation et handle périmé vers une unique intention de
  fallback, sans appeler D2R;
- [ ] prouver les budgets par think, l’invalidation de handle et la quarantaine
  bornée par tests déterministes avec capacités natives mockées;
- [ ] conserver le résolveur, son hook, les helpers natifs, le déploiement et
  tout gameplay hors de ce gate.

### RUN-AI — Qualification

- [ ] cold start default-off avec pile complète et cinq plugins eezstreet;
- [ ] un monstre opt-in, puis un hireling, sans modifier les unités non liées;
- [ ] toutes les erreurs Lua tombent sur le fallback sans gel;
- [ ] benchmark think/s, p50/p95/p99 et budget par tick sur packs denses;
- [ ] mort/despawn/reload sans handle ni VM d’ancienne génération réutilisable;
- [ ] TCP/IP hôte/client, configs asymétriques et absence de Lua autoritaire
  côté client distant;
- [ ] scripts modifiables sans rebuild DLL, avec reload seulement si le contrat
  de génération et de rollback le permet.

## Mesures de succès

- `0` hook en dehors du résolveur et des helpers strictement requis par la V1;
- `0` unité non liée entrant dans Lua;
- `0` gel après erreur, absence d’action ou script invalide;
- coût p99 par think sous le budget fixé avant qualification;
- hash hôte journalisé et `0` décision Lua autoritaire sur le client distant;
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

**Prochain gate : `EXEC-AI-1`.** Implanter et tester l’évaluateur de l’arbre
retenu avec un handle de think éphémère et des capacités entièrement mockées.
Chaque chemin doit produire au plus une intention terminale ou une intention de
fallback, sans appeler D2R. Le résolveur `0x4A36C0`, son hook, les helpers natifs,
le gameplay et le déploiement restent absents jusqu’au gate d’intégration natif.
Aucun commit, push, déploiement ou test runtime n’est autorisé implicitement par
ce document.
