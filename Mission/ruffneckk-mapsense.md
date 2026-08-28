# RuffnecKk MapSense — D2R 3.2.92777 et 3.3.93847

Dernière mise à jour : 28 août 2026

## Décision et état

Le 21 août 2026, Vincent a autorisé par `GO` un véritable spike de menu
Dear ImGui **in-process et in-frame** dans `RuffnecKkMapSense.dll`. Cette voie
dessine le panneau dans la frame DirectX 12 déjà présentée par D2R et interdit
explicitement tout second `HWND`, fenêtre transparente plein client, contexte
OpenGL/WGL, thread de swap séparé ou `MapSense.exe` obligatoire. L'automap
native reste propriétaire de la carte; Dear ImGui sert uniquement à
l'interface de configuration. Le configurateur externe devient un plan de
secours, pas l'architecture retenue.

Le 21 août, Vincent a retenu le curseur D2R comme curseur unique : Dear ImGui ne
doit plus afficher, masquer ou transformer un second curseur Windows. Le contrat
fonctionnel exigé reste l'absence totale de propagation des clics, de la
molette et des drags vers le déplacement, les attaques, les casts ou les
interactions D2R lorsque le menu développé les possède. En mode réduit, seule
la petite surface du lanceur possède l'entrée; le reste revient au jeu.

La matrice graphique obligatoire est limitée à MapSense seul, Floating Damage
seul et les deux ensemble. Les deux DLL demeurent autonomes : aucune ne doit
être requise pour charger ou fonctionner seule. Lorsqu'elles coexistent, un
seul propriétaire du renderer/hook DirectX 12 doit être désigné par un contrat
optionnel, versionné, indépendant de l'ordre de chargement et fail-safe face à
une ABI absente ou incompatible. ReShade n'est pas une dépendance ni un gate
de compatibilité du produit public. L'ancien plugin d'interface retiré de la
pile n'appartient plus à cette architecture ni à sa matrice.

Vincent a désigné cette mission comme priorité active le 19 août 2026, puis a
retenu définitivement le nom public `RuffnecKk MapSense` le même jour.

Le 24 août 2026, Vincent a validé en jeu le correctif de classement 0.7.1 :
les Champions utilisent maintenant leur style bleu tandis que les Uniques
conservent leur style orange. Il a ensuite donné `GO` au prochain incrément en
plaçant initialement la navigation Direct avant les missiles. Vincent a corrigé
immédiatement cette priorité : les **immunités passent d'abord**, puis la
navigation Direct. La navigation doit ensuite distinguer trois familles
activables indépendamment :
waypoints bleus, prochaine sortie de progression verte et objectifs de quête
rouges; chacune conserve une couleur configurable. La ligne Direct indique la
direction à vol d'oiseau et ne prétend pas être un chemin praticable. La
navigation GPS suivant les corridors demeure un lot séparé après preuve des
salles et collisions D2R 3.3.

Le même `GO` impose deux corrections de cadrage : la couleur par défaut de
l'immunité Physical deviendra beige, et la portée 2 500 doit être prouvée en
jeu. Vincent observe actuellement qu'augmenter la valeur au-delà d'environ
1 600 ne semble plus ajouter de monstres; la borne TOML seule ne constitue donc
pas une validation fonctionnelle.

Le séquencement actif devient **preuves natives → MVP automap et marqueurs →
immunités → navigation Direct et destinations → salles/collisions et GPS →
missiles**.

Les prototypes natifs 0.3.0 et 0.3.1 restent des preuves négatives. Le premier
avait une géométrie cassée et des contrôles intermittents; le second prouvait
les quatre actions Reveal, mais ne constituait pas le configurateur extensible
attendu. Aucun des deux n'est l'interface produit retenue.

Décision produit confirmée puis implantée le **24 août 2026** : MapSense
demeure une DLL autonome hybride de la RuffnecKk D2RLoader Suite avec son TOML
indépendant. Toutes les catégories de monstres restent toujours visibles, mais
chaque rang dispose maintenant d'une forme indépendante parmi `x`,
`player_cross` et `dot`, en plus de sa couleur, de son alpha et de sa taille.
`player_cross` est la nouvelle forme par défaut. Les anciennes configurations
exprimaient la portée en unités client/dimétriques de 500 à 2 500; le schéma 7
les remplace par 30 à 220 vrais subtiles monde, avec 60 par défaut.

État actuel : **MapSense 0.11.2 est déployé byte-identique dans le runtime
BKVince; son cold start pile complète est PASS et son témoin gameplay ciblé est
EN COURS**. La
politique verte explicite comprend 93 niveaux sources et 94 routes : 82 sorties
statiques exactes par RoomTile/ouverture extérieure et 12 transitions de
portail dynamiques exactes. Le candidat 0.11.2 corrige l'unité du rayon, exige
une ouverture bilatérale pour Tamoe→Monastery et ajoute les presets exacts
Summoner/Tome avant la création du portail Arcane. Le rayon réel,
Tamoe→Monastery et Arcane constituent le témoin immédiat; les lignes rouges de
quête constituent le lot suivant. Il ne crée aucun
second `HWND`, processus, canvas plein écran, backend OpenGL/WGL ni thread de
rendu. `Map & Reveal` conserve `Additions Opacity`, `Reveal Level`, `Reveal
Act`, `Toggle Reveal All` et `Reveal All Off`; `Monsters` expose la portée,
l'épaisseur et, pour chaque rang toujours actif, forme, couleur, alpha et
taille. `Immunities` ajoute activation, style `colored_i` ou `split_halo`, taille
des indicateurs, épaisseur du halo et six couleurs. Le sélecteur de couleur a
été nettoyé et les tooltips s'affichent au-dessus du curseur. Position et
réglages sont persistés dans le TOML indépendant au schéma 7 après la fin d'une
interaction, hors thread `Present`.

Le candidat 0.11.2 conserve les trois familles Direct réellement reliées :
waypoint bleu, progression principale verte Acts I-V et sorties personnalisées
mauves. Activation, couleurs et épaisseur sont dans
le menu MapSense; seule la liste mauve `level_id`/`level_name` est une édition
TOML manuelle. Le contrôle Quests demeure dans le schéma pour compatibilité
future, mais il est désactivé et caché du panneau tant qu'aucun adaptateur de
quête réel ne publie une destination. Le build Release `/W4 /WX`, CTest `1/1`
et les quatre exports passent. La DLL de 766 464 octets porte PE 0.11.2 et
SHA-256
`8B7CF8140C895BDBA3017741E64CDD6932C861378A0B894FB39384C86DEA2A4E`.
Le 28 août 2026, la DLL source et le runtime mod-local concordent à
`8B7CF8140C895BDBA3017741E64CDD6932C861378A0B894FB39384C86DEA2A4E`.
Le cold start officiel 3.3.93847 atteint `24/24`, charge 36 plugins dont
MapSense 0.11.2 et applique 18 patches. Le seul échec est Revive Overhaul,
incident préexistant distinct. Le gameplay rayon/Tamoe/Arcane reste **EN COURS**.

Le runtime 0.10.4 déjà testé conserve le squelette Dear ImGui DirectX 12, les
marqueurs, les immunités resserrées, les invalidations immédiates Tab/Escape et
la navigation acte I. Vincent confirme le mécanisme mauve, Underground Passage,
Barracks, les Jails et les Catacombs, ainsi que la disparition immédiate des
icônes à la fermeture de l'automap et à Pause. La ligne verte acte I est jugée
suffisamment fonctionnelle pour généraliser le principe; le décalage bleu du
waypoint n'est plus bloquant pour ce chantier.

Le collecteur 0.8.1 observe le passage natif par unité que D2R exécute lorsque
son automap est visible. Avant toute insertion dans le cache, il exige un
`UnitMonster` vivant, rejette les flags natifs mercenaire et acteur asynchrone,
résout le `classId` et le contexte propres à l'unité, puis exige un MonStats
`KILLABLE` qui n'est ni `NPC`, ni `INTERACT`, ni `INTOWN`. Le getter natif
d'alignement doit ensuite retourner `Evil (0)`. Une signature, métadonnée ou
lecture absente échoue fermée au lieu d'interpréter un zéro par défaut comme un
hostile. Le collecteur conserve ainsi tous les hostiles observés dans le rayon
30–220 vrais subtiles monde, sans top-N ni éviction par priorité. Le double buffer préalloue 16
chunks de 4 096 observations par buffer, peut ajouter des chunks dynamiques
réutilisables et compte un échec d'allocation comme faute de stockage. Le cache
dynamique par `unitId` expire après 250 ms; aucun pointeur D2R vivant ne traverse
vers le renderer. Pour un hostile déjà admis par le rayon, la projection et le
clip natif, il lit conditionnellement les six résistances lorsque les immunités
sont activées et transmet seulement un masque de six bits.

Les couleurs et tailles par défaut restent normal blanc 18, minion jaune 18,
champion bleu 20, unique orange 22 et super unique/boss rouge 24, avec une
épaisseur globale de 2; leur forme par défaut devient `player_cross`. Le
classement 0.7.1, désormais validé en jeu, applique explicitement la priorité `SuperUnique > Champion >
Unique > Minion > Normal` afin qu'un flag partagé ne transforme plus un Champion
en Unique. Le filtre composite 0.8.1 est implanté et testé statiquement. Le 24
août 2026, Vincent confirme en partie que les PNJ/figurants n'ont plus de
marqueur et que les vrais monstres restent visibles : ces deux cases sont
**PASS**. Mercenaires, invocations et monstres convertis restent `NOT RUN`. Le menu
reste anglais pour ce lot. La locale `Auto` et les thèmes de couleurs sont
différés; les thèmes pourront plus tard devenir des presets persistants, sans
que ce design futur soit encore fermé.

MapSense est toujours propriétaire prioritaire du renderer lorsqu'il coexiste
avec Floating Damage. Les deux DLL pinent le même commit Dear ImGui et partagent
un contrat ABI v2 byte-identique : Floating Damage attache ses fonts et son
rendu comme client du contexte MapSense. Seul, Floating Damage garde son hôte
autonome; si MapSense s'arrête, il ne le reprend qu'après retrait complet des
hooks MapSense. Un transfert refusé fait échouer le second propriétaire plutôt
que d'installer deux hooks concurrents. L'ordre de chargement ne change pas
cette règle.

La DLL Release 0.8.1 candidate mesure 680 960 octets et vaut SHA-256
`8BA1161F97713FA78446F5C61560404CB86AB9BAB91004027ECAD09CB5DBF519`.
Le build Release `/W4 /WX`, CTest `1/1`, les quatre exports et la version PE
0.8.1 passent. Le candidat a été déployé byte-identique dans le profil BKVince
normal. Le cold start pile complète sans capture Windows réussit avec MapSense
0.8.1 mod-local, Floating Damage 1.4.1 global, 31 plugins, 18 patches et `24/24`
au startup. Vincent confirme ensuite en partie l'exclusion des PNJ/figurants et
la conservation des vrais hostiles. Le rendu des immunités, les mercenaires,
invocations et monstres convertis demeurent **NOT RUN**.

Jalon historique défectueux : 0.7.0, 664 576 octets,
`2951E284BE9306D096BAFF52817CD098AD1053D68DB252B12E1E5460AE403A02`,
avait été déployé byte-identique puis réellement lancé. Vincent y a observé des
Champions rendus comme des Uniques. Cette preuve gameplay négative motive la
priorité de classement corrigée en 0.7.1; aucun succès visuel 0.7.0 n'est
transféré au nouveau candidat.

Historique conservé : le candidat 0.6.0 de 646 656 octets,
`DC85526DE2B3D2CEBC1BC84BD01512051C54CB6BE99B8A165FC14452BD185D10`,
passait déjà `/W4 /WX`, CTest `1/1`, quatre exports et PE 0.6.0; son fichier
avait été déployé byte-identique, mais son gameplay était resté **NOT RUN**. Il
utilisait le schéma 3, un rayon 60–600 par défaut 300 et une croix commune; il
est maintenant supersédé par les jalons 0.7.0, 0.7.1, 0.8.0 puis 0.8.1. Floating Damage 1.4.1 vaut
`6AE91CBC60F03F50708D31E5B10349E9F3BEB788740426BA67457DE6DF2B386E`
et ses quatre tests ciblés passent. Le cold start 0.8.1 prouve maintenant son
chargement global avec MapSense mod-local, mais pas encore leur coexistence
visuelle en gameplay.

La baseline 0.4.1 précédente mesurait 577 536 octets et valait
`A896074FFA7143CE317905B76794EB0FC1927673292789B7CA1CCF934983E698`;
elle demeure le rollback runtime qualifié. Le test 0.4.0 avait prouvé les
boutons Reveal et l'isolation des clics par rapport au personnage, mais échoué
sur le lanceur visible hors partie et le double curseur D2R/Windows. La 0.4.1 a
corrigé ces deux causes sans modifier Reveal ni l'ABI renderer. Son cold start
frais avait chargé 29 plugins, appliqué 17 memory patches, atteint `24/24` puis
`D2R startup complete`; MapSense avait initialisé l'hôte in-frame et Floating
Damage 1.4.1 produit son témoin de dégâts/projection. Vincent avait confirmé le
lanceur seulement en partie, le curseur D2R unique, l'absence de mouvement
derrière le panneau, les actions Reveal, les performances et Floating Damage,
et accepté 0.4.1 comme **bon squelette de départ**. Ces preuves historiques
ne sont pas réattribuées à 0.9.9. Les candidats Navigation Direct 0.9.7 et
0.9.8 sont construits, déployés et rejetés par leurs témoins gameplay sur les
ancres exactes. Le correctif 0.9.9 est construit hors runtime, sans déploiement
ni exécution du jeu; GPS et labels restent absents, et les immunités ne sont
toujours pas validées visuellement.

Le produit est définitivement une DLL autonome `RuffnecKkMapSense.dll`,
membre de la RuffnecKk D2RLoader Suite, attribuée exactement à `RuffnecKk`.
Elle demeure hybride globale/mod-locale, sans `ModScopedOnly`, avec sa propre
version, ses métadonnées et son TOML indépendant
`ruffneckk-mapsense.toml`. Le TOML et ses commentaires sont entièrement en
anglais; une configuration présente mais invalide est refusée avant tout hook.
Aucun merge dans une DLL d'eezstreet ou dans `D2RPlugins.json` n'est planifié.

Description anglaise de travail :

> Reveals maps, marks monsters, and draws direct navigation lines.

## Périmètre produit

Le plugin doit permettre de configurer :

- la révélation et l'affichage de la carte native;
- les monstres `normal`, `minion`, `champion`, `unique`, `super unique` et
  `boss`, avec couleurs, tailles et formes distinctes;
- les immunités physical, fire, cold, lightning, poison et magic, affichées
  comme segments ou contours colorés autour du symbole du monstre;
- les super chests, shrines, weapon racks et armor racks;
- les missiles physical, fire, cold, lightning, poison, magic, mixed et
  unknown, avec couleur et taille par catégorie;
- les noms de destination, lignes de direction et chemins calculés vers les
  sorties de niveau, objectifs de quête et boss;
- les hotkeys avec modificateurs Ctrl, Alt, Shift et Win;
- l'anglais, l'allemand, l'espagnol, le français, l'italien, le coréen, le
  polonais et le chinois;
- un cache de géométrie optionnel correctement lié au build, au mod et aux
  données actives;
- un menu graphique habillé dans le style de D2R pour les options, couleurs,
  tailles, opacités, thèmes et aperçu en direct; le TOML demeure la persistance
  dédiée, pas l'interface normale imposée au joueur.

MapSense réserve ses marqueurs additionnels aux monstres hostiles observés dans
son rayon. Il ne redessine pas les joueurs, corpses, waypoints, portails ou
autres symboles déjà fournis par l'automap native. Une autre information ne
sera envisagée que si elle apporte une destination ou un état réellement absent
du rendu D2R et après preuve de non-duplication.

Les noms sont affichés pour tous les Super Uniques à nom fixe, les boss de
quête comme la Countess et le Summoner, les boss d'acte et les Mini-Ubers. Les
uniques aléatoires restent visuellement plus grands mais ne reçoivent pas de
nom par défaut afin de limiter l'encombrement.

Les noms et classifications doivent venir du runtime et des tables compilées
du mod réellement chargé. Aucun identifiant, nom ou catalogue propre à BKVince
ne peut devenir une vérité codée en dur dans la DLL.

## Exclusions décidées

- Aucun item log, ground-item alert, son d'objet ou TTS; ces fonctions
  appartiendront à un autre projet.
- Aucun level/location de groupe, XP meter, HP de mercenaire, timer de session
  ou Buff Bar. Ces enrichissements des widgets natifs appartiendront à une DLL
  RuffnecKk Suite autonome distincte, dont le nom public reste à choisir.
- Aucun mapgen externe, processus compagnon, installation LoD 1.13c ou MPQ
  classique.
- Aucun travail d'anti-détection et aucun support Battle.net. La cible produit
  est solo offline et TCP/LAN.
- Aucune mutation directe du gameplay ou du réseau et aucun format de
  sauvegarde propriétaire. La révélation utilise l'état automap natif; D2R peut
  donc réécrire ses sauvegardes et sidecars `.ma*` habituels.

## Gate d'approbation visuelle

Vincent conserve l'approbation finale du langage visuel MapSense. Le candidat
0.8.1 rend chaque catégorie toujours visible et configurable séparément avec
`x`, `player_cross` ou `dot`; `player_cross` est le nouveau défaut, avec la
palette et les tailles historiques blanc 18, jaune 18, bleu 20, orange 22 et
rouge 24. La portée technique par défaut devient 1 000 dans la plage 500–2 500.
Ces valeurs sont implantées et le classement Champion bleu contre Unique orange
est validé. Les immunités proposent maintenant de petits `i` colorés ou un halo
segmenté, de une à six résistances; leur rendu, leurs tailles et leurs couleurs
restent `NOT RUN` et soumis à l'approbation de Vincent. Les lignes Direct sont
maintenant un candidat Release 0.9.9 hors runtime, mais épaisseur, couleurs et alignement
restent soumis au gate visuel runtime. Entrées de niveaux, GPS, objets, polices
et priorités de labels restent soumis à leurs gates futurs.

Avant toute valeur visuelle de production :

1. produire des maquettes statiques sur une capture D2R couvrant densité faible
   et forte, un boss nommé, une et deux immunités, une sortie et une route;
2. faire approuver explicitement par Vincent formes, couleurs, tailles et
   hiérarchie;
3. reproduire uniquement la variante approuvée dans une scène témoin en jeu;
4. obtenir une seconde approbation en jeu avant d'en faire les valeurs par
   défaut.

Les noms proviennent du runtime et de la localisation du mod chargé; aucun nom
de boss ou de niveau propre à BKVince n'est codé en dur.

## Projets connexes confirmés

- Une future DLL autonome d'enrichissement des widgets natifs D2R regroupera
  level et location dans les portraits du groupe, XP meter, HP du mercenaire,
  timer de session et Buff Bar. Elle n'appartiendra pas à MapSense et ne
  modifiera ni réseau ni sauvegardes.
- Le tooltip complet des objets au sol reste un projet futur séparé. Il devra
  prouver le trajet label au sol → unité objet → tooltip sans capturer le clic
  de ramassage; il ne dépend d'aucun ancien plugin retiré de la pile.
- Un futur plugin autonome Item Alerts/Filter possédera les règles de drop,
  marqueurs optionnels, journal, sons et TTS. Son éventuelle coopération avec
  MapSense restera optionnelle, versionnée et fail-safe; l'exemple
  `itemfilter.yml` fourni par Vincent constitue une référence fonctionnelle,
  pas encore un format autoritaire adopté.

## Architecture retenue

1. Reveal Level résout le DRLG client courant et appelle le callback automap
   natif derrière un unique hook `DRLG_InitLevel` vérifié par signatures.
2. Reveal Act et Reveal All utilisent les exports publics nommés `Version`,
   `IsInGame` et `ExecuteConsoleCommand` de D2RCore 1.1.0-beta. Un retour `true`
   signifie seulement que la commande `revealmap` a été acceptée.
3. Reveal All est progressif : l'acte courant est demandé immédiatement, puis
   un événement SDK `ActChanged` soumet une seule requête pour chaque nouvel
   acte chargé. Aucun parcours numérique des niveaux ni exclusion BKVince
   codée en dur n'est conservé.
4. Le collecteur live hooke le passage automap natif par unité à `0xD76E0`,
   appelle toujours l'original en premier et exactement une fois, puis soumet
   chaque `UnitMonster` vivante observée côté client au filtre composite. Il
   rejette `ISMERC` et `ISASYNC`, exige un MonStats `KILLABLE` qui n'est ni
   `NPC`, ni `INTERACT`, ni `INTOWN`, puis exige `Evil (0)` du getter natif
   d'alignement. La résolution du contexte est propre à chaque unité et toute
   anomalie échoue fermée. Le rayon est configurable de 500 à 2 500 subtiles,
   1 000 par défaut. Le double buffer préalloue 16 × 4 096 observations par
   buffer, étend sa capacité par chunks dynamiques réutilisables et compte tout
   OOM comme faute de stockage. Le cache dynamique par `unitId`, sans top-N ni
   éviction par priorité, expire après 250 ms. Aucun pointeur D2R vivant n'est
   conservé par le renderer.
5. MapSense crée un seul contexte Dear ImGui dans le `Present` DirectX 12 de
   D2R, sans seconde fenêtre. Le subclass du véritable HWND D2R est installé et
   retiré sur son thread propriétaire. Il consomme souris, molette, drags et
   raw mouse uniquement dans la surface du panneau; clavier et reste du client
   demeurent au jeu. D2R reste l'unique propriétaire du curseur; le backend
   ImGui est explicitement empêché de modifier le curseur Win32.
6. MapSense garde la priorité graphique. Floating Damage est un client ABI v2
   optionnel lorsqu'ils coexistent, autonome lorsqu'il est seul, et reprend ses
   hooks seulement après le callback `hostStopped`. Tout échec de handoff est
   fail-closed afin qu'il n'existe jamais deux propriétaires D3D12.
7. Les candidats WARP D3D11, OpenGL/WGL et fenêtre transparente externe sont
   rejetés. Aucun backend graphique ou EXE compagnon ne sera réintroduit sans
   une nouvelle décision fondée sur des mesures.
8. Le panneau 0.8.1 reste extensible par accordéons à la manière de PrimeMH.
   Le TOML schéma 5 est la persistance, pas l'interface joueur normale. Tous les
   rangs sont toujours visibles; chacun configure `x`, `player_cross` ou `dot`
   ainsi que couleur, alpha et taille, tandis que portée et épaisseur restent
   globales. `player_cross` est le défaut. Le sélecteur de couleur est nettoyé
   et les tooltips apparaissent au-dessus du curseur. `Additions Opacity`
   contrôle les ajouts MapSense et ne remplace jamais l'opacité de l'automap
   déjà fournie par D2R. Locale `Auto` et thèmes sont différés; de futurs thèmes
   pourront être des presets persistants après décision dédiée.
9. Le modèle complet `Act/Level/Room`, les collisions, l'A* et le cache ne
   commenceront qu'après preuve que les salles non visitées sont lisibles sans
   effet de bord.
10. L'A* futur utilisera des coûts orthogonaux/diagonaux cohérents, interdira le
   corner-cutting et ne mettra jamais en cache les unités dynamiques.

## Faits vérifiés au démarrage du lot 0

- Le runtime installé est bien **D2R 3.3.93847** : `.build.info` porte la
  Build Key `623f7a1f73eabb08ccb2b2046e3f9164` et son SHA-256 vaut
  `2EBCAD0521DBF038D5A7FE5395E96B4BEF6D4F0774F7B1F840E03C3DE9CB067A`.
  `D2R.exe` version 3.3.93847 vaut
  `E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`.
- Le corpus gouverné vaut
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`.
  Ce hash porte sur l'image canonique déchiffrée et n'est pas comparable au hash
  du PE retail protégé. L'identité binaire utile entre 3.2.92777 et 3.3.93847
  étant déjà établie, les RVA, prologues, hooks, ABI, index et preuves du corpus
  sont directement réutilisables.
- `re:d2r33` nomme la cible courante 3.3.93847 tout en réutilisant le workbench
  binaire commun et son self-test vérifié. Aucun workbench duplicatif 93847
  n'est nécessaire.
- Le trajet natif du premier marqueur est gouverné : `AUTOMAP_RenderUnit`
  `0xD76E0` reçoit `Unit*` et le contexte automap, appelle les getters X/Y
  `0x34AF60/0x34AFB0`, la projection `0xD4910`, puis le dessin natif
  `0xD6DB0`. Les dimensions UI viennent de `0x7F510/0x7F4A0`; le clip exact
  vient du contexte `+0x18/+0x1C/+0x20/+0x24`.
- Le témoin de rang `0x51F280` prouve `Unit+0x10 → MonsterData` et les flags à
  `MonsterData+0x1A`, avec le masque haut rang natif `0x0E`. Le candidat rejette
  aussi le bit minion `0x10`, corroboré historiquement mais encore soumis au
  témoin gameplay. `UNITS_GetUnitId 0x34A330` et `UNITS_GetUnitMode 0x34AB60`
  ont des signatures strictes uniques dans le corpus.
- Le filtre 0.8.1 emploie `STATLIST_GetUnitAlignment` `0x2F4190`,
  `UNITS_GetClassId` `0x349860`, le contexte propre à l'unité via `0x34A0E0` et
  le getter MonStats `0x976E0`, tous protégés par signatures strictes. Les flags
  `ISMERC`/`ISASYNC`, puis `KILLABLE` sans `NPC`/`INTERACT`/`INTOWN`, précèdent
  l'exigence finale `Evil (0)`. Cette preuve est statique et ne ferme pas le
  gate gameplay PNJ/figurant/mercenaire/invocation/monstre converti.
- Le même getter gouverné lit maintenant les six résistances actuelles seulement
  après admission hostile, rayon, projection et clip : Physical 36, Magic 37,
  Fire 39, Lightning 41, Cold 43 et Poison 45. Le seuil d'immunité est `>=100`;
  le hook copie un masque de six bits et ne conserve aucun pointeur D2R.
- L'égalité ou le facteur d'échelle entre les coordonnées automap natives et
  les pixels ImGui n'est pas déduit sans contrôle : le renderer exige des
  dimensions positives, des rapports X/Y cohérents à 1 % et un point dans la
  frame; sinon il ne dessine rien. Centre, coin gauche/droit, zoom, 16:9 et
  ultrawide restent des preuves runtime ouvertes.
- La référence `eezstreet/D2RL-Plugins` est vérifiée, propre et épinglée au
  commit `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`, cible PluginPack 2.0.1,
  licence MIT déclarée.
- Les requêtes gouvernées `known automap`, `known reveal`, `known collision`,
  `known roster` et `known portal` ne retournent aucune preuve MapSense.
- `known room` connaît déjà `DUNGEON_GetRoomListAndCount`,
  `DUNGEON_GetFirstUnitInRoom`, `UNITS_GetNextUnitInRoom`,
  `DUNGEON_DoesRoomIntersectSearchSquare` et l'énumérateur de zone; leur
  applicabilité au runtime courant reste conditionnée par l'identité binaire.
- L'audit de portée confirme qu'aucun clamp caché ne réduit la valeur 2 500 :
  le setter accepte bien 500–2 500. Le hook est toutefois alimenté par la liste
  native des salles proches/visibles du joueur et reproduit ensuite le clip de
  l'automap; ce contrat peut expliquer le plateau visuel observé vers 1 600.
  Le statut 0.8.1 expose donc rayon actif, rejets rayon/projection/clip, rejets
  ennemi par étape et fautes de métadonnées, ainsi que les
  distances maximales observée, admise et publiée pour fermer le gate par un
  A/B réel 1 600/2 500 plutôt que par déduction.
- `known missile` ne fournit encore que des coutures de dégâts; aucune source
  gouvernée d'énumération, propriétaire, trajectoire ou élément de missile
  n'est prouvée pour cet affichage.
- La baseline installée est D2RLoader/D2RCore `1.1.0-beta`, SHA-256 respectifs
  `A926DAA85DE85EADCF98FDB5FB30143CC32D6B4913EB21A6416EBDAA78945128`
  et `013B047612BFF0EB564891037508FD43D03AFDFB20BFEB9B5BC683B36559FFC6`.
  Le hash final pré-renommage a reproduit un cold start stable dans le profil
  normal BKVince : 28 plugins, 17 patches et `24/24`.
- La baseline publique du nouveau plugin est PluginSDK API v3, tag `v3`, commit
  `4933e2c42cb2592958cd0df3b6dc5003102252d1`. Elle expose Lifecycle, Input,
  DataTable, Thread, Localization et Diagnostics. MapSense utilisera le rôle
  `Client`, puis `NativeHooks` seulement lorsqu'un hook réellement nécessaire
  est prouvé; il ne déclarera jamais `ModScopedOnly`.
- DataTable v1 permet de lire les tables compilées `MonStats`, `SuperUniques`,
  `Missiles`, `Levels`, `LevelDefs`, `LvlPrest`, `LvlWarp`, `LvlMaze`,
  `Objects`, `ObjGroup` et `Shrines` du mod actif. Il ne fournit ni
  `AutoMap.txt`, ni renderer, automap, rooms/collisions, roster ou énumération
  des unités live. Chaque layout brut et `rowSize` reste à prouver sous 93847.
- D2RCore expose les exports publics nommés `Version`, `IsInGame` et
  `ExecuteConsoleCommand`. Le plugin les résout par nom, sans import ni ordinal,
  et exige exactement `1.1.0-beta`. La commande interne `revealmap` sert de
  backend remplaçable pour Act et All; son retour `true` prouve l'acceptation,
  pas l'effet. Elle peut rester sans effet lorsque le gate debug de D2RCore,
  notamment `.DISABLE_DEBUG`, est actif.
- InputServiceV1 gère une seule touche modificatrice parmi Shift, Ctrl ou Alt,
  s'intègre aux Controls et se bloque pendant la saisie. Il ne gère ni Win ni
  plusieurs modificateurs; satisfaire ces deux cas exigerait une couture input
  supplémentaire à prouver et posséder explicitement.
- Le `reveal-map.toml` non suivi est orphelin dans la portée active. Son
  consommateur historique est `RevealMap.dll` 1.3.0, auteur `RuffnecKk`, trouvé
  seulement dans une sauvegarde non active, SHA-256
  `8E81E0DDE5E2C3B9F2170BE0832E64B875DF0F255E26A8EA4FB4EB22EF582A5C`.
  Cette DLL refuse explicitement tout build différent de 92777. D2RCore expose
  séparément une commande intégrée `revealmap`; aucune preuve ne montre qu'elle
  lise ce TOML ou fournisse une API réutilisable à un plugin tiers.
- La source historique complète de RevealMap 1.3.0 subsiste localement sous
  `analysis-cache/abandoned-reveal-map-20260723-160905/RevealMap-src/`. Elle a
  fourni les ancres sémantiques du backend Reveal Zone; la candidate courante
  n'utilise les RVA du corpus gouverné commun qu'avec vérification stricte des
  signatures et ABI sous 3.3.93847. Un premier parcours direct de tous les IDs
  de niveau d'un acte a déclenché l'assertion `ptWarp` sur les niveaux
  volontairement déconnectés de BKVince; cette voie est abandonnée. Le binaire
  final ne contient aucune plage d'acte ni liste d'exclusions BKVince codée en
  dur.
- D2MOO au commit `19019806df7f3e877fa105b05395d1e3597e2316` reste une
  référence sémantique pour les chemins statiques d'Unit et les conversions de
  coordonnées. D2RMH au commit
  `32d55b8ab9a3e9b380103e73e3c8d328cd4f3ad4` documente l'intersection des
  ouvertures de collision sur les deux côtés d'une transition extérieure.
  MapSense n'en transpose aucune adresse, structure ou ABI 32 bits et ne copie
  aucun code; toutes les RVA et layouts viennent du corpus D2R gouverné commun.
- Floating Damage actif est en version 1.3.5, SHA-256
  `2B3BFCF5DF161C911A45CE653C932CE46E1610E846CDAF6CB8E7EB13E208F8B1`,
  avec l'export `RuffnecKkFloatingDamageGetOverlayApi`. Son
  `ExternalOverlayApiV1` enregistre un overlay nommé et expose seulement des
  rectangles vides/remplis; il ne couvre pas encore lignes, polylignes,
  cercles ou textes UTF-8. Un ancien log prouve l'inscription différée de
  CharmZone, mais Floating Damage s'y annonce encore ciblé 3.2.92777 malgré le
  processus 93847. Depuis la décision du 20 août, cette API n'est plus une
  dépendance planifiée de MapSense; elle ne constitue qu'une possibilité
  d'interopérabilité future et facultative.
- ReShade `dxgi.dll` 6.8.0.2158 est présent dans la pile et devra être inclus
  dans la matrice de coexistence graphique.
- Le dépôt public PrimeMH au commit
  `92b6a97d8e56346f8b63a88bb647c1af044d2c8b` utilise une seule fenêtre externe
  transparente et topmost, rendue en OpenGL matériel avec Notan et un panneau
  `egui`. Il plafonne son rendu (`fps_limit=60` par défaut), active le
  mouse-passthrough, puis retire `WS_EX_TRANSPARENT | WS_EX_LAYERED` lorsque la
  souris entre dans le rectangle réellement utilisé par le menu et les remet à
  la sortie. Il ne transmet pas ces événements à D2R et n'implante aucun maintien
  du curseur natif D2R. Son README indique explicitement que le projet n'est pas
  licencié pour redistribution : seules les idées d'architecture peuvent être
  réévaluées clean-room; aucun code, asset, police ou binaire n'est copiable.

## Preuves runtime du socle Reveal 0.1.0

- DLL MapSense source/runtime : 132 608 octets, SHA-256
  `32C79878042F8B693D23445741020DBA874353AA19D5A7468B31C4A86EEC4D67`.
- CTest MapSense : `1/1`; cold start du hash exact : 28 plugins, 17 patches,
  `24/24`, processus stable puis fermé au frontend sans entrer dans une
  sauvegarde.
- Les trois entrées globales `ruffneckk-mapsense/reveal-zone`,
  `ruffneckk-mapsense/reveal-act` et `ruffneckk-mapsense/toggle-reveal-all`
  conservent Ctrl+F9, Ctrl+F10 et Ctrl+F11. Chacune a invoqué MapSense au
  frontend et produit le refus fail-safe attendu hors partie.
- La commande enregistrée est `mapsense [status|zone|act|all|off]`; aucun alias
  `map-overlay` n'est conservé.
- DLL de gameplay pré-renommage : 132 608 octets, SHA-256
  `7F53E4DAC6B1CB081EE4DCB5518E4922F0DA7510CB1F6A0DC3C8E0AACA1B58A8`.
- DLL pré-renommage finale après correction d'un commentaire TOML embarqué :
  132 608 octets, SHA-256
  `41A049E8B66A674E6D6F16D8E56045471721800B00C37A8D99887E1EBFE42D1F`.
- Profil : BKVince normal mod-local; la validation finale n'a ni créé ni utilisé
  de profil isolé. Un ancien reliquat de sauvegarde
  `mods/RuffnecKkRuntime93847` subsiste hors du runtime et n'a pas été touché.
- Reveal Zone : passé dans deux sessions.
- Reveal Act : requête acceptée, automap visible, aucun `ptWarp` frais.
- Reveal All : armement/désarmement passé.
- Transition Acte I → Acte II : exactement une requête automatique acceptée et
  automap Acte II visible.
- Cycle GameLeft/GameJoined : reset confirmé; la partie suivante ne conserve
  pas l'état armé.
- D2R a réécrit la sauvegarde et les sidecars automap actifs pendant le jeu et
  Save & Exit. Aucun format propriétaire, contrôle byte-exact ni causalité
  exclusive au plugin n'est revendiqué.
- Le premier cold start MapSense a chargé les 28 plugins et 17 patches, puis a
  rencontré l'assertion renderer préexistante
  `GetLoadingBundleCmdQueue()->Flush().Succeeded()` après l'ouverture d'une
  session de capture Windows et avant `24/24`. Le cold start répété sans capture
  a atteint `24/24` et est resté stable; le premier passage est conservé comme
  incident environnemental et ne constitue pas une preuve MapSense.
- Actes III–V, autres difficultés/seeds, portée globale, autres mods et TCP/LAN :
  `NOT RUN`.

## Hypothèses à tester

- Les surfaces natives du marqueur et le filtre d'alignement sont identifiés
  statiquement sous 3.3.93847, mais le témoin gameplay doit encore confirmer les
  modes mort/vivant, les cinq rangs, les trois formes et la projection sans faux
  positif ni désalignement. Le témoin BKVince du 24 août 2026 prouve que
  `Evil (0)` exclut les PNJ et figurants observés tout en conservant les vrais
  monstres hostiles visibles. Mercenaires, invocations et monstres convertis
  restent `NOT RUN`; leur exclusion ne doit pas être déduite de cette preuve.
- D2R pourrait matérialiser l'ensemble des salles/collisions d'un niveau dans
  le processus offline; aucune preuve actuelle ne garantit les zones non
  visitées ni le comportement d'un client TCP/LAN.
- Le cycle client DRLG/rooms/callback est prouvé pour Reveal Zone sur la matrice
  ciblée. Le trajet interne privé de `revealmap` demeure non contractuel; son
  effet n'est observé que dans les actes I et II du test courant.
- L'automap natif pourrait suffire au MVP sans cache. Le cache ne sera ajouté
  que si une mesure de temps ou de coût démontre sa valeur.

## Séquencement actif

### Lot 0 — preuves et ownership

1. **Fermé :** version et Build Key du runtime 3.3 ainsi que son identité
   binaire utile avec le corpus gouverné commun.
2. **Fermé :** le RevealMap historique, sa source et sa couture 92777 sont
   retrouvés; le TOML actif est orphelin et D2RCore possède un mécanisme
   distinct désormais utilisé comme backend Act/All remplaçable.
3. **Fermé pour le socle Reveal ciblé :** baseline D2RLoader/D2RCore,
   PluginSDK v3, hook Zone, bridge Act/All, build, CTest et cold start pile
   complète. La matrice cinq actes/difficultés/seeds reste ouverte.
4. **Partiel :** transitions et cycle de session sont prouvés pour Acte I →
   Acte II. Le passage automap par unité, les coordonnées, le mode et les flags
   de rang nécessaires au premier monstre normal sont maintenant prouvés et
   implantés. `Act/Level/Room`, collisions, presets, roster, objets et missiles
   restent à identifier pour la navigation et les catégories suivantes.
5. **Panneaux natifs 0.3.x rejetés; hôte in-frame 0.4.1 construit :** un seul
   contexte Dear ImGui vit dans la frame D2R, sans HWND ou renderer externe.
   Le lanceur et la section Reveal minimale sont implantés; build, CTest,
   exports, version, contrat ABI v2 et coexistence statique avec Floating
   Damage sont verts. La 0.4.0 a passé Reveal et l'isolation des clics, puis la
   0.4.1 a relié la visibilité à `LocalPlayerReady/GameLeft` et rendu à D2R la
   propriété exclusive du curseur. Le hash candidat est déployé dans BKVince
   avec les TOML préservés; cold start, visibilité frontend/in-game, curseur,
   clics, Reveal, coexistence et performance passent. Le cycle explicite
   Save & Exit → nouvelle partie et la manette restent `NOT RUN`. La 0.6.0 a
   ensuite réutilisé ce propriétaire graphique pour le premier candidat
   multi-monstres au schéma 3, sans modifier l'ABI v2 de Floating Damage; cette
    preuve statique historique est supersédée par le candidat 0.8.1 au schéma 5.

### Lot 1 — MVP automap et marqueurs

**Reveal et panneau in-frame qualifiés historiquement; candidat correctif 0.7.1
déployé et cold-starté :** Reveal Level (identifiant historique `reveal-zone` conservé),
Reveal Act, Reveal All progressif, Off, TOML, actions configurables et panneau
Dear ImGui dans la frame D2R sont implantés. Seule la transition Acte I → Acte
II d'une difficulté demeure qualifiée pour la matrice multi-actes; la baseline
0.4.1 passe historiquement son gate menu/curseur, isolation d'entrée, Reveal,
coexistence et performance. Le candidat 0.7.1 conserve tous les hostiles
observés sans top-N dans un rayon 500–2 500, défaut 1 000. Le schéma 4 maintient
chaque rang visible et lui donne `x`, `player_cross` ou `dot`, couleur, alpha et
taille; `player_cross` est le défaut. `STATLIST_GetUnitStat 0x2F5020`, signature
stricte et stat 172 `Evil (0)` filtrent avant le cache en échouant fermé. Le
sélecteur de couleur est nettoyé et les tooltips apparaissent au-dessus du
curseur; locale `Auto` et thèmes sont différés, les thèmes pouvant plus tard
devenir des presets persistants. La classification applique désormais
`SuperUnique > Champion > Unique > Minion > Normal`. La DLL 664 576 octets
`1EA277B5…F48CB0`, `/W4 /WX`, CTest `1/1`, quatre exports et PE 0.7.1 passent.
Le binaire est déployé byte-identique dans le profil BKVince normal. Le cold
start pile complète réussit avec MapSense 0.7.1 mod-local, Floating Damage 1.4.1
global, 29 plugins, 17 patches et `24/24`. La 0.7.0 avait été lancée et Vincent
y avait observé les Champions rendus comme des Uniques; elle reste un jalon
historique défectueux. La comparaison visuelle Champion bleu contre Unique
orange en 0.7.1 est **PASS**, confirmée par Vincent le 24 août 2026.
L'ancien filtre fondé sur le seul stat d'alignement est supersédé par le filtre
composite 0.8.1. L'exclusion des PNJ/figurants et la conservation des vrais
hostiles sont **PASS**, confirmées par Vincent le 24 août 2026; mercenaires,
invocations et monstres convertis restent `NOT RUN`. Noms,
objets, localisation et autres matrices restent ouverts; les immunités ont été
portées en 0.8.0 puis conservées dans le hotfix 0.8.1.

### Lot 2 — immunités

Affichage configurable par petits `i` colorés ou halo segmenté. La couleur
Physical par défaut devient beige. Une, deux ou trois immunités produisent
respectivement un, deux ou trois indicateurs/segments; la représentation reste
générique jusqu'aux six résistances sans recolorer le marqueur de rang.

**Candidat 0.8.1 construit, déployé et cold-starté; validation gameplay NOT
RUN :** le schéma 5 ajoute `colored_i`, `split_halo`, taille des indicateurs,
épaisseur du halo et six couleurs. Les `i` occupent une rangée jusqu'à trois
immunités puis deux rangées jusqu'à six; le halo utilise un anneau complet ou
des segments égaux. Physical vaut par défaut `#D8C39AFF`. Les schémas 1–4 sont
migrés : l'état enabled est conservé, l'ancien gris par défaut devient beige et
une couleur Physical personnalisée reste intacte. Le collecteur effectue les
six lectures seulement pour un hostile déjà admis et seulement si l'affichage
des immunités est actif; le renderer ne reçoit qu'un masque copié.

La DLL Release x64 de 680 960 octets porte PE 0.8.1 et SHA-256
`8BA1161F97713FA78446F5C61560404CB86AB9BAB91004027ECAD09CB5DBF519`;
`/W4 /WX`, CTest `1/1` et quatre exports passent. Elle est déployée
byte-identique dans BKVince normal. Le cold start sans capture Windows charge
MapSense 0.8.1, Floating Damage 1.4.1, 31 plugins, 18 patches et atteint
`24/24`. Le hotfix ajoute le filtre composite ennemi uniquement avant les
lectures de résistances. Le test gameplay ferme l'exclusion PNJ/figurants et la
conservation des hostiles, mais pas le rendu des immunités. Restent les preuves
à une, deux, trois et six immunités, les changements dynamiques et la performance dans un pack dense.
Les six lectures par hostile et par passage
automap constituent un risque mesurable; un échantillonnage 100–250 ms ne sera
ajouté que si une régression est observée.

### Lot 3 — navigation Direct et destinations

Décision produit confirmée le **24 août 2026** : ce lot devient le chantier
MapSense actif sans attendre la fermeture visuelle du lot Immunities. Quatre
familles de destination sont activables indépendamment : waypoint bleu du
niveau courant, prochaine sortie de progression principale verte, niveaux
personnalisés mauves et objectif de quête rouge. Chaque couleur, l'épaisseur et
chaque activation appartiennent au menu normal MapSense. Seule la liste des
niveaux personnalisés mauves est destinée à être éditée manuellement dans le
TOML par `level_id` ou `level_name`; le menu affiche son nombre d'entrées et
permet de l'activer ou la désactiver.

La progression verte repose sur une politique explicite qui ignore les niveaux
optionnels tels que The Cave; elle ne doit pas être déduite naïvement de l'ordre
des IDs ni des seuls champs `Vis`. Les quêtes utilisent des résolveurs dédiés,
notamment Summoner/Horazon's Journal, Lam Esen et les objectifs des égouts de
l'acte III. Le collecteur doit produire des coordonnées monde prouvées depuis
les données runtime de la partie et le renderer ne conserve aucun pointeur D2R.
La ligne Direct est une indication à vol d'oiseau; elle peut traverser les murs
et ne doit jamais être présentée comme un trajet praticable.

Base 0.9.5 : le résolveur s'exécute uniquement depuis les événements
gameplay/UI de D2RLoader. Il part du joueur local, atteint son `ActiveRoom`, puis
le `DrlgRoom` et le `Level` courant par les accesseurs natifs gouvernés
`UNITS_GetRoom`, `ACTIVEROOM_GetDrlgRoom` et `DRLGROOM_GetLevelId`. Le waypoint provient directement de
`DRLGROOM_FindWaypointRoomAndCoordinates` `0x3DAD90`, ABI `(uint8 context,
Level*, int32* tileX, int32* tileY) -> DrlgRoom*`. D2R choisit la room admissible,
l'initialise, valide le preset objet et le bit waypoint natif, puis retourne des
coordonnées en game tiles. MapSense exige un retour non nul avec deux sorties
valides et les convertit en subtiles par `tile * 5`; aucun premier objet ni
centre de room ne sert de fallback.

Chaque sortie type 5 conserve son point source exact `roomTile * 5 +
presetRelative`, mais son véritable niveau cible vient maintenant de
`DRLGWARP_ResolveRoomTileLink` `0x3DA9A0`, ABI `(uint8 context, DrlgRoom*, int32
sourcePresetId, int32* reciprocalPresetId, LvlWarpTxt** reciprocalWarp) ->
ActiveRoom*`. Le helper initialise la source si nécessaire, valide le RoomTile
réciproque et retourne l'ActiveRoom destination; MapSense résout ensuite son
`DrlgRoom` et son `LevelId` par les accesseurs natifs. Tous les scalaires des presets
d'une room sont copiés avant le premier appel natif : aucun pointeur PresetUnit
n'est conservé pendant une initialisation DRLG. Retour nul, signature absente,
pointeur incohérent, cycle ou dépassement échoue fermé sans ligne devinée. Le
prédicat natif `DUNGEON_IsRoomInTown` `0x2F0750` vide les destinations en ville
avant tout appel waypoint/sortie. Pour les sorties extérieures, le corpus prouve
le vecteur natif de rooms voisines à `DrlgRoom+0x10` et son compte 64 bits à
`+0x18`; l'ancien lecteur 0.9.4 utilisait par erreur les drapeaux à `+0x50`
comme un compte 16 bits, ce qui pouvait supprimer totalement la liaison vers
Stony Field. Le candidat 0.9.5 validait l'ensemble des témoins vectoriels,
géométriques et RoomTile avant toute lecture.

Correction 0.9.7 : les positions D2R de preset waypoint et de sortie restent des
**subtiles**, tandis que `AUTOMAP_ProjectClientCoordinatesToScreen 0xD4910`
attend les coordonnées client isométriques. MapSense applique désormais la
conversion exacte `clientX = 16 * (subtileX - subtileY)` et
`clientY = 8 * (subtileX + subtileY)` avec contrôles de débordement avant cette
frontière; la position du joueur issue des getters client reste inchangée. Pour
les transitions extérieures directes comme Cold Plains → Stony Field, le
résolveur lit en plus les huit slots dynamiques `Vis`/`Warp` par
`DRLGWARP_GetVisLevelId 0x360880` et `DRLGWARP_GetWarpValue 0x3DAAD0`, exige la
réciprocité, `Warp == -1` des deux côtés et les bits de room correspondants,
avant de calculer un point géométrique sur la frontière de la room source. Le
témoin gameplay du 26 août 2026 valide l'apparition des lignes mais invalide
ces deux ancres : la position finale de l'objet waypoint actif diffère de son
preset de génération, et la frontière rectangulaire ne désigne pas l'ouverture
traversable normale.

Correction 0.9.8 — waypoint exact : le preset natif reste le sélecteur d'identité
et fournit le `classId` waypoint validé par `ObjectsTxt.SubClass & 0x40`.
MapSense exige ensuite l'`ActiveRoom*` prouvée à `DrlgRoom+0x58`, parcourt sa
liste bornée par `DUNGEON_GetFirstUnitInRoom 0x2EFD90` (`ActiveRoom+0xA8`) et
`UNITS_GetNextUnitInRoom 0x34B4A0` (`Unit+0x160`), puis exige type objet,
`classId` et room identiques. Le bleu conserve enfin sans conversion le couple
`UNITS_GetClientCoordX/Y 0x34AF60/0x34AFB0` de cette Unit, soit exactement le
couple que `AUTOMAP_RenderUnit` transmet au projecteur natif. Une room ou une
Unit absente ne réactive jamais le preset approximatif : le résultat reste
partiel et retryable.

Correction 0.9.8 — ouverture extérieure exacte : Vis/Warp et les rooms voisines
servent seulement à sélectionner une paire de niveaux. Pour une room source
déjà active, `DUNGEON_GetCollisionGridFromRoom 0x2EFB30` retourne la grille à
`ActiveRoom+0x38`; les témoins uniques `0x36697B/0x3669E6` prouvent origine
`+0/+4`, largeur/hauteur `+8/+0x0C` et cellules `uint16* +0x20`. Sur la bordure
cardinale faisant face à la room cible, le résolveur exige que la cellule de
bord et sa voisine intérieure n'aient pas le bit mur `1`, rejette les runs de
moins de trois subtiles et choisit le centre du plus large run exact. Aucune
collision active signifie aucune ligne et un retry; après `Reveal Zone` ou
`Reveal Act`, une résolution fraîche est demandée sur le thread UI. Il ne crée
ni Level ni Room, n'appelle aucune Add/RemoveRoomData, ne reconstruit aucun warp
et ne conserve aucun pointeur natif.

Correction 0.9.9 — pipeline de référence : le témoin gameplay 0.9.8 invalide
l'hypothèse « Unit active plus exacte que le preset » : l'Unit et le preset
produisent le même couple client `(-8720,79784)` pour le waypoint observé.
MapSense revient donc au contrat réellement utilisé par PrimeMH et d2mapapi :
la position monde du waypoint est exactement `roomTile * 5 + presetRelative`,
sans substitution par une Unit voisine ni centre de tuile. PrimeMH au commit
`92b6a97d8e56346f8b63a88bb647c1af044d2c8b` reste un témoin comportemental;
aucun code de ce dépôt sans licence n'est copié.

La disparition du vert est, elle, expliquée par les milliers de lectures
`collision-pending` : Navigation parcourait le Level du graphe joueur tandis
que Reveal possédait déjà le DRLG client complet. La 0.9.9 centralise cette
identité dans `ClientLevelView`, résout le niveau courant et les niveaux Vis
par `DRLG_GetLevel 0x3267C0`, initialise leurs listes de rooms par
`DRLG_InitLevel 0x3271C0`, puis matérialise chaque room nécessaire avec
`DRLGROOM_CreateActiveRoom 0x3289A0`. Ce dernier helper est déjà propriétaire
du gate natif de Reveal et retourne l'`ActiveRoom*` stockée à `DrlgRoom+0x58`.
Le scan de collision reprend ensuite le milieu exact des runs de bordure
traversables comme d2mapapi; aucun centre rectangulaire ne redevient fallback.
La 0.9.9 est construite hors runtime; ce résultat reste à confirmer en jeu.

Correction 0.10.0 — sorties intérieures et presets exacts : les témoins live
0.9.9 ont montré `raw=1 native=0 exact=0` à Tamoe Highland et Barracks, puis à
Jail Level 1 uniquement le lien de retour vers Barracks. La ligne mauve Pit
Level 1 était absente; les lignes vertes Barracks→Jail 1 et Jail 1→Jail 2
étaient absentes, et l'ouverture extérieure Tamoe→Monastery Gate pouvait encore
supplanter le preset d'entrée exact. Le désassembleur gouverné explique ce
comportement : `DRLGWARP_ResolveRoomTileLink 0x3DA9A0` suit bien
`RoomTile+0x00` vers le `DrlgRoom` destination à `0x3DA9FB`, mais refuse ensuite
le résultat tant que la destination ne possède pas encore son RoomTile
réciproque. D2MOO au commit épinglé confirme cette séparation sémantique entre
destination source et validation réciproque; aucune adresse ni ABI 32 bits n'est
transposée.

MapSense 0.10.0 retire ce gate artificiel du collecteur. Il initialise d'abord
la cible de progression explicite et chaque cible personnalisée configurée qui
apparaît comme voisine directe dans les huit slots `Vis` du même DRLG client,
par les helpers déjà gouvernés `DRLG_GetLevel` et
`DRLG_InitLevel`. Il matérialise ensuite chaque room du niveau courant par
`DRLGROOM_CreateActiveRoom`, parcourt sa chaîne `DrlgRoom+0x78`, lit
`LvlWarp+0x2C` pour l'identité source et `RoomTile+0x00` pour le vrai
`DrlgRoom` destination, puis associe cette identité au `PresetUnit` type 5. Le
point publié reste exactement `roomTile * 5 + presetRelative`; une preuve
RoomTile exacte remplace donc l'ouverture collision de même cible. L'empreinte
fail-closed ajoute le témoin de 15 octets `0x3DA9FB`, couvrant
`RoomTile+0x00`, `DrlgRoom destination+0x78` et le pointeur réciproque sans
appeler le helper qui l'exige.

La matrice hors jeu 0.10.0 nomme désormais toute la progression acte I, en
particulier Stony Field→Underground Passage 1, Underground Passage 1→Dark Wood,
Barracks→Jail 1, Jail 1→Jail 2, Jail 2→Jail 3, Jail 3→Inner Cloister,
Cathedral→Catacombs 1 et Catacombs 1→2→3→4. Pit Level 1 et Underground Passage
Level 2 sont aussi couverts comme destinations personnalisées mauves. Release
`/W4 /WX`, PE 0.10.0, quatre exports et CTest `1/1` passent; la DLL de 760 320
octets vaut SHA-256
`CE06F62D82A72FDE8BFC22001D85828C3BE85E5A90F22B69DD44CBB6B619D405`.
Après le GO de Vincent du 27 août, le même hash est déployé dans le profil
BKVince mod-local. Le cold start frais sur le runtime officiel courant
D2R 3.3.93847 (Build Key `623f7a1f73eabb08ccb2b2046e3f9164`, D2R.exe
`E1F5436E…CC8936`) charge MapSense 0.10.0, les 37 plugins et 18 patches de la
pile complète, atteint `24/24`, installe les hooks D3D12, capture la command
queue et initialise l'hôte ImGui. Ce gate est **PASS technique**. Le témoin
gameplay est **EN COURS**. Vincent confirme le mauve Tamoe Highland→Pit Level 1
**PASS**, mais rejette encore le vert Tamoe Highland→Monastery Gate en
**FAIL précision**. Vincent confirme ensuite Underground Passage, Barracks, les
Jails et les Catacombs **PASS gameplay**. Le mécanisme mauve est validé par Pit
Level 1; Underground Passage Level 2 n'était pas dans la liste mauve active et
n'est donc pas revendiqué comme témoin visuel distinct.

Le renderer reçoit soit `{destinationId, subtileX, subtileY, kind}` pour les
sorties, soit le même témoin monde accompagné du couple client exact pour une
Unit waypoint. La
projection est effectuée pendant le passage automap natif du joueur local dans
l'unique hook automap déjà possédé par MapSense; `Present` ne lit aucun pointeur
D2R et ne consomme que des snapshots immuables expirant après 250 ms. La
politique verte acte I est explicite et exclut Den of Evil, Cave, Crypt,
Mausoleum, Pit et Tower. Les noms mauves utilisent un catalogue statique dérivé
de la colonne `*StringName` du `Levels.txt` vanilla 3.3 read-only; un nom
ambigu échoue fermé et doit être remplacé par son `level_id`.

Un `LevelChanged` invalide les anciennes lignes immédiatement. Si le Level ou
son preset exact est encore transitoire, le résolveur retente au plus huit fois sur de
futures mises à jour du thread UI; le dernier événement remplace la cible
pending et aucune résolution n'est effectuée dans `Present`.

`Tab` appartient exclusivement à l'automap native. Les messages Win32 key-down,
key-up, system-key et character sont rejetés avant le backend ImGui; le backend
et la callback d'action répètent ce garde. Même si la touche est assignée à
`Toggle MapSense Settings`, la callback retourne `Ignored` avant la queue, quel
que soit le modificateur; le panneau ne change pas et D2R reçoit la touche. La
commande console `mapsense menu` et toute autre touche configurée restent
disponibles.

### Lot 4 — salles, collisions, GPS et cache

Graphe des salles/collisions, copie immuable, A*, cache atomique et invalidation.
La clé inclura au minimum build, version du backend, seed, difficulté, niveau,
mod et empreinte des données pertinentes. La première version GPS reste limitée
au niveau courant et ne doit jamais se rabattre silencieusement sur Direct.

### Lot 5 — missiles

Énumération et classification runtime, ownership hostile, trajectoire et
catégories élémentaires. Le terme `incoming` ne sera utilisé qu'après preuve de
la trajectoire vers le joueur.

### Extension 0.11.0 — progression verte Acts I-V

Le 27 août 2026, Vincent autorise la généralisation du mécanisme vert acte I à
tout le jeu sans exiger un témoin manuel dans chaque zone. La politique devient
une table explicite de **93 niveaux sources / 94 routes**, couverte
exhaustivement par le test C++ : 82 transitions statiques et 12 transitions
dynamiques. Les extérieurs suivent la route de campagne et ignorent leurs
entrées optionnelles; lorsqu'un joueur entre volontairement dans un donjon à
plusieurs étages, la ligne verte continue vers l'étage suivant. Spider Forest
préfère le raccourci direct Flayer Jungle et retombe sur Great Marsh seulement
si ce raccourci n'existe pas dans le seed courant.

Les portails de progression ne reçoivent aucune coordonnée codée en dur. Le
résolveur matérialise la room, parcourt de façon bornée sa chaîne native
d'Units, exige `UnitObject`, le contexte et l'`ActiveRoom` attendus, puis mappe
le couple **niveau courant + classe objet** : Palace Cellar 3→Arcane Sanctuary
`298`, Arcane Sanctuary→Canyon `60`, chacun des sept tombeaux→Duriel's Lair
`100` uniquement lorsque le portail du vrai tombeau existe, Durance 3→Acte IV
`342`, Chaos Sanctuary→Harrogath `566` et Throne→Worldstone Chamber `563`.
Les coordonnées client/dimétriques de l'Unit sont publiées inchangées et leur
inverse subtile exacte sert de témoin; une preuve runtime-object surclasse un
RoomTile, qui surclasse une ouverture collision.

Cette extension réemploie uniquement les surfaces natives déjà gouvernées et
fingerprintées : `DUNGEON_GetFirstUnitInRoom 0x2EFD90`,
`UNITS_GetNextUnitInRoom 0x34B4A0`, type, classe, contexte, room et coordonnées
client. Elle n'ajoute ni hook ni RVA. D2MOO au commit
`19019806df7f3e877fa105b05395d1e3597e2316` sert de preuve sémantique : portail
Palace/Arcane dans `ObjMode.cpp:3066-3074`, création Arcane→Canyon dans
`A2Q4.cpp:394-400`, portail de Duriel dans `A2Q6.cpp:1333-1339`, Hell Gate vers
Pandemonium Fortress dans `ObjMode.cpp:3089-3117`, portail 566 vers Harrogath
dans `A4Q2.cpp:259-263,1299-1305`, et portail de Baal vers la Chambre dans
`A5Q6.cpp:764-783`. Aucune adresse ni ABI D2MOO 32 bits n'est transposée.

Un niveau à portail dynamique absent est considéré complet plutôt que
`PartialRetryable`; il n'initialise pas artificiellement le Level cible. Tant
que l'automap native rend ce niveau et qu'aucune ligne verte n'existe, une
demande UI bornée est armée au plus une fois par seconde. Le polling cesse dès
que le portail exact est publié. Aucune traversée DRLG ni lecture d'Unit ne se
fait dans `Present`.

Le build Release x64 0.11.0 passe `/W4 /WX`, CTest `1/1`, les quatre exports et
PE 0.11.0. La DLL de 763 904 octets vaut
`72858DDEC90334742E8DACDCB3707F86CE11C67B70FD5A5E846008709E32B706`.
Le `Levels.txt` BKVince a été relu avec le helper TSV officiel : 148 lignes,
CRLF et round-trip byte-exact. Aucun fichier de données ni TOML runtime n'est
modifié. Le candidat 0.11.0 reste **source/build PASS, gameplay NOT RUN** et
n'est pas encore déployé.

La bonne tombe depuis Canyon of the Magi n'est pas une sortie verte statique :
elle dépend du symbole choisi par la quête. Elle appartient donc au prochain
lot rouge, avec les autres objectifs de quête, plutôt qu'à une approximation
verte ou à sept lignes concurrentes.

### Correctif 0.11.1 — flood diagnostic de projection

Le 28 août 2026, Vincent observe une chute à 60 FPS lorsque l'automap est
ouverte dans Frigid Highlands et Halls of Pain, tandis qu'Arreat Plateau ne
reproduit pas le défaut. La configuration active porte pourtant
`[diagnostics] enabled = false`. Les logs et le code prouvent la cause : le
callback de projection était fourni inconditionnellement au hot path automap et
son cache direct de 16 cases utilisait `destinationId % 16`. Deux lignes dont
les identifiants partageaient une case s'écrasaient mutuellement à chaque frame
et contournaient la déduplication.

Dans le seed observé, Frigid Highlands faisait alterner le waypoint
`18214375364822279195` et la progression `17344995562435206827` dans la case
11, pour 10 456 écritures; Halls of Pain faisait alterner
`4192527222844799637` et `12336832201234066901` dans la case 5, pour 3 030
écritures. Arreat Plateau et les six autres niveaux Acte V visités avaient des
cases distinctes. Le flood mesuré ajoutait environ 27 012 octets de log par
seconde et coïncidait avec 1,844 seconde CPU du processus sur une fenêtre de
2,006 secondes. Trente-quatre zones sont structurellement exposées à au moins
deux lignes avec la configuration actuelle, mais la collision exacte dépend du
seed; une tournée manuelle ne constitue donc pas une validation fiable.

MapSense 0.11.1 ne fournit désormais aucun callback de projection lorsque les
diagnostics sont désactivés et retire aussi le témoin diagnostic forcé du
résolveur et des actions Controls. Lorsque les diagnostics sont explicitement
activés, une table bornée de 256 clés exactes par niveau déduplique le couple
type de ligne + destination sans modulo collisionnel; une saturation supprime
le log supplémentaire au lieu de produire un flood. Le test C++ reproduit les
deux IDs réels de Frigid Highlands, vérifie leur déduplication indépendante, le
reset de niveau et la borne de 256 destinations.

Le build Release `/W4 /WX`, CTest `1/1`, les quatre exports et PE 0.11.1 sont
**PASS**. La DLL de 763 904 octets vaut
`0DE926350D90CCE7668E28652A59BE2505B897FB5C6B9A4DDED24ABEEBCCA429`;
le build et le runtime BKVince mod-local sont byte-identiques. Le cold start
D2R 3.3.93847 / build key `623f7a1f73eabb08ccb2b2046e3f9164`
atteint `24/24`, charge MapSense 0.11.1, installe les hooks D3D12, capture la
command queue et initialise ImGui avec les cinq DLL eezstreet et toute la pile
active : 36 plugins chargés, le même échec Revive Overhaul préexistant et 18
patches. Vincent confirme ensuite le 28 août 2026 que Frigid Highlands et Halls
of Pain fonctionnent toutes deux correctement avec l'automap ouverte. Les logs
frais post-test contiennent toujours `0` diagnostic de projection et `0`
diagnostic du résolveur; les messages Tab ponctuels de l'hôte UI ne sont pas le
flood par frame corrigé. Le gate performance 0.11.1 est donc **PASS gameplay**.

## Prochain gate

Gate actif : **lignes rouges de quête**. Le graphe vert Acts I-V est fermé par
la matrice exhaustive et le correctif performance 0.11.1 est confirmé dans
Frigid Highlands et Halls of Pain avec l'automap ouverte. Le lot rouge doit
commencer par inventorier les objectifs réellement déterministes par état de
quête — notamment le vrai tombeau depuis Canyon — puis publier leurs positions
exactes sans réutiliser une sortie verte approximative. Une preuve native et
un test automatisé par famille d'objectif remplacent une tournée manuelle de
chaque zone; quelques témoins gameplay représentatifs resteront nécessaires
avant une revendication runtime globale.

Le gate ciblé du filtre ennemi est **PASS** :
Vincent a confirmé sous Tab que les PNJ/figurants n'ont plus de marqueur et que
les vrais monstres hostiles restent visibles. Les témoins Navigation Direct
0.9.3 et 0.9.4 sont **FAIL** : en Cold Plains la ligne bleue visait le vide et
la ligne verte Stony Field était absente; en 0.9.4, Tab interagissait encore
avec le menu. Le candidat 0.9.7 corrige le contrat subtile/client et ajoute le
résolveur extérieur Vis/Warp non mutatif. Son build, son déploiement byte-exact
et son cold start 3.3.93847 `24/24` restent **PASS**, mais le témoin gameplay
joint par Vincent est **FAIL précision** : le bleu manque l'objet waypoint et
le vert manque l'entrée normale.

Le témoin 0.9.8 est également **FAIL** : le bleu n'atteint toujours pas le point
précis du waypoint et le vert disparaît. Les logs prouvent que l'Unit active et
le preset waypoint ont le même couple client, tandis que des milliers de rooms
restent `collision-pending`. Le correctif source 0.9.9 remplace donc le graphe
joueur par le DRLG client déjà capturé par Reveal, initialise les niveaux Vis,
matérialise leurs rooms, et publie le waypoint brut ainsi que le milieu exact
des ouvertures de collision. Son build Release `/W4 /WX` passe sans lancer
CTest; il mesure 759 296 octets, porte PE 0.9.9 et vaut SHA-256
`11C7D4B6276903FE4A85E1F387E8EF318B57C02C559BB139B96C515781527C03`.
Après le GO explicite de Vincent, la DLL 0.9.9 a été déployée byte-identique dans
le profil BKVince mod-local; le TOML actif est demeuré byte-identique à
`7FF8F95AA7A151BB8D44440AD5C7B5ACFAE318F868B61C669EBB755DE376CB9D`.
Le cold start frais D2R 3.3.93847 accepte l'empreinte native complète, charge
MapSense 0.9.9, installe les hooks D3D12 in-frame, capture la command queue,
initialise l'hôte ImGui et atteint `24/24` avec la pile complète : 36 plugins
chargés, les mêmes deux échecs de plugins préexistants et 18 patches actifs.
Ce résultat est **PASS technique**, sans valoir verdict de précision gameplay.
Le jeu reste ouvert pour le gate séparé **Navigation Direct 0.9.9 — précision
gameplay acte I** : confirmer aucune ligne en Rogue Encampment, puis sous Tab
dans Cold Plains que le point bleu coïncide avec l'objet waypoint et le point
vert avec l'ouverture normale vers Stony Field, même si le waypoint est acquis
et la sortie découverte, sans ligne verte vers The Cave. Une fois
ce correctif confirmé, vérifier la route personnalisée mauve Tamoe Highland →
Pit Level 1 ainsi que l'activation, les couleurs, l'épaisseur, la persistance,
les performances et la coexistence Floating Damage. Seule la liste mauve reste
une édition TOML manuelle. Les actes II–V et les objectifs de quête ne font
aucune revendication dans 0.9.9. Le lot Immunities demeure ouvert et `NOT RUN`.

Le 27 août 2026, MapSense 0.10.0 remplace byte-exactement la 0.9.9 dans le
profil BKVince normal. Le cold start officiel 3.3.93847 atteint `24/24` avec
37 plugins et 18 patches; les logs MapSense confirment l'empreinte fail-closed,
les hooks D3D12 in-frame, la command queue et l'hôte ImGui. Dans Stony Field,
le diagnostic publie déjà le waypoint et la progression vers Underground
Passage 1 avec une preuve `room-tile`, mais cela ne ferme pas l'alignement
visuel. Vincent confirme ensuite le mauve Tamoe Highland→Pit Level 1 **PASS** :
le runtime publie la cible niveau 12 au preset `(15094,5231)` avec preuve
`room-tile`. Il rejette encore le vert Tamoe Highland→Monastery Gate en
**FAIL précision** : la cible progression niveau 26 reste calculée à
`(15060,5090)` depuis une preuve `outdoor-collision` de largeur 40, donc au
milieu d'une large bordure traversable et non à l'entrée normale visible. Le gate
actif reste **Navigation Direct 0.10.0 — gameplay acte I**. Vincent confirme
ensuite les lignes de Barracks, des Jails et des Catacombs **PASS gameplay**.
Les logs frais montrent notamment les progressions 28→29, 29→30, 30→31,
31→32, 33→34, 34→35 et 35→36, toutes avec une cible trouvée et publiée.
Vincent confirme ensuite Underground Passage **PASS gameplay**. Stony Field
publie l'entrée d'Underground Passage 1 avec une preuve `room-tile`; dans
Underground Passage 1, le runtime trouve trois sorties exactes et publie la
progression vers Dark Wood au preset `(7637,8020)`, également `room-tile`.
Underground Passage Level 2 est bien découvert comme cible niveau 14 au preset
`(7551,8168)`, mais n'est pas publié en mauve puisque cette destination ne figure
pas dans le TOML actif; cela reste `NOT RUN`, pas un échec. Le seul défaut de
navigation confirmé dans la matrice gameplay 0.10.0 est donc l'ancre extérieure
Tamoe→Monastery Gate. La validation des immunités demeure aussi ouverte.

### Correctif autorisé 0.11.2 — unités monde et deux ancres de progression

Le 28 août 2026, Vincent autorise par `GO` un lot unique MapSense 0.11.2 sans
déploiement ni lancement du jeu. La DLL demeure le plugin autonome RuffnecKk
Suite, hybride globale/mod-locale, avec son TOML indépendant. Trois correctifs
sont retenus : la portée des monstres est exprimée et calculée en vrais
subtiles monde; Tamoe Highland→Monastery Gate exige une ouverture collision
traversable des deux côtés de la frontière; Arcane Sanctuary→Canyon suit le
portail runtime classe `60` lorsqu'il existe, sinon le preset du Summoner
classe `250`, puis le tome d'Horazon classe `357` en dernier repli. Le
signalement `Toggle Reveal All` est explicitement retiré par Vincent et ne fait
l'objet d'aucune modification dans ce lot.

PrimeMH épinglé au commit
`92b6a97d8e56346f8b63a88bb647c1af044d2c8b` confirme que l'Arcane n'a pas de
`next exit` ordinaire mais publie le Summoner niveau `74`, classe `250`, comme
`NPCSpawn` et les objets nommés `Tome` comme `QuestItem`; ses lignes Boss et
Quest assurent donc cette direction séparément. D2MOO épinglé au commit
`19019806df7f3e877fa105b05395d1e3597e2316` confirme sémantiquement que le
portail permanent classe `60` vers Canyon n'est créé qu'après l'interaction
avec le tome. Aucune adresse ou ABI 32 bits n'est transposée.

Implantation technique terminée : le schéma 7 accepte 30–220 vrais subtiles
monde, défaut 60, et migre les valeurs des schémas 3–6 par division arrondie par
16. Le filtre de rayon utilise les X/Y unsigned du `DynamicPath`; les
coordonnées client ne servent plus qu'à la projection automap. Les trois
surfaces natives ajoutées sont déjà gouvernées avec confiance haute et
empreintes strictes : `UNITS_GetDynamicPath` `0x34AE80`, `PATH_GetX`
`0x341A20` et `PATH_GetY` `0x341A30`. L'ouverture extérieure intersecte les
cellules bord/intérieur libres des deux rooms et publie le milieu côté
destination. Arcane scanne désormais ses presets même sans chaîne RoomTile et
classe les preuves `runtime portal > Summoner > Tome > RoomTile > collision`.

Validation hors jeu : le workbench D2R 3.2/3.3 retourne `selfTest=PASS`; le
build Release strict `/W4 /WX` passe; CTest retourne `1/1`; l'audit trouve les
quatre exports attendus et PE 0.11.2. L'artefact mesure 766 464 octets et vaut
SHA-256
`8B7CF8140C895BDBA3017741E64CDD6932C861378A0B894FB39384C86DEA2A4E`.
Après l'autorisation séparée de Vincent de lancer le test, la DLL 0.11.1 active
est sauvegardée sous
`analysis-cache/runtime-sync-backups/mapsense-0.11.2-20260828-141429/`, puis la
0.11.2 est déployée byte-identique dans le profil BKVince mod-local. Le TOML
actif reste byte-identique à
`CE171C42A7B2FFA5B2124096C4FA2386A89AB08888E7691DC3EC2967DE914743` : son
schéma 6 et sa valeur historique `2500` sont migrés en mémoire vers environ 156
vrais subtiles, sans réécriture silencieuse.

Le cold start frais D2R 3.3.93847, build key
`623f7a1f73eabb08ccb2b2046e3f9164`, charge MapSense 0.11.2 depuis la portée
`[mod]`, les cinq plugins eezstreet et toute la pile active. D2RLoader rapporte
36 plugins chargés, le seul échec Revive Overhaul préexistant, 18 patches et
`D2R startup complete` à `24/24`. Une seule instance demeure ouverte pour le
témoin gameplay. Le log frais vaut
`80A208D6AD829D0C0B7DB9991B9EF9308FD4015F9FA0F47A12795E04606CB7C2` au relevé
post-démarrage. Le cold start 0.11.2 est donc **PASS technique**; rayon visuel,
Tamoe→Monastery et Arcane sont **EN COURS**, sans verdict anticipé.

Ce témoin ciblé ne ferme pas les autres gates : tous les hostiles sans top-N,
portée 30–220 vrais subtiles, formes, expiration, mercenaires/invocations/monstres convertis,
déplacement, zoom, automap centré/coins, performance,
coexistence visuelle Floating Damage, Save & Exit → nouvelle partie, manette,
16:9, ultrawide, actes III–V, difficultés, seeds, portée globale et sidecars
restent ouverts. Locale `Auto` et thèmes demeurent hors de ce gate.

### Correctif autorisé 0.12.0 — collecte client complète et tombe de Duriel

Le 28 août 2026, Vincent autorise par `GO FAIS TOUT ÇA` le candidat MapSense
0.12.0, incluant implantation, preuves gouvernées, tests, déploiement et cold
start dans le profil BKVince normal. La DLL demeure le plugin autonome hybride
de la RuffnecKk D2RLoader Suite, attribué à `RuffnecKk`, avec son TOML
indépendant et sans `ModScopedOnly`.

Le lot ferme deux comportements distincts. Premièrement, le rayon 30–220 reste
exprimé en vrais subtiles monde, mais la collecte des monstres doit suivre le
modèle fonctionnel de PrimeMH : parcourir la table client complète des Units
monstre plutôt que dépendre uniquement des Units remises au rendu natif de
l'automap. Les offsets historiques PrimeMH ne sont jamais transposés; la table,
ses 128 buckets, son stride par type et la chaîne `Unit` doivent être prouvés
dans le corpus D2R 3.3 gouverné. Le scan sera borné, cycliquement protégé et
exécuté au plus une fois par cadence de collecte, jamais une fois par Unit ni
depuis une traversée lourde de `Present`. Les diagnostics distingueront Units
chargées côté client, rayon, projection, clip et publication afin qu'une icône
hors écran ne soit plus confondue avec un rayon inactif. Un serveur qui n'a pas
encore transmis une Unit reste une limite honnête du client.

Deuxièmement, Canyon of the Magi doit toujours pointer vers la vraie tombe de
Tal Rasha avec une même destination exacte : rouge tant que la quête Acte II
Q6 n'a pas accordé sa récompense, puis verte après récompense pour farmer
Duriel dans les parties suivantes. La sélection ne doit ni choisir une tombe
approximative ni publier sept lignes. Elle doit lire l'identité native de la
bonne tombe depuis le DRLG actif, résoudre son RoomTile exact, lire l'état de
quête client par le getter natif gouverné et échouer fermée lorsqu'une preuve
manque. Arcane Sanctuary→Canyon déjà confirmé reste inchangé.

Le signalement `Toggle Reveal All` est classé intermittent : le comportement
n'est pas modifié sans reproduction. Le candidat ajoute seulement les preuves
diagnostiques nécessaires autour de Canyon et conserve la voie de Reveal sûre
qui n'initialise jamais en masse les niveaux déconnectés. L'assertion
`BC_VERIFY: iter` ne doit pas réapparaître. La validation ciblée comprend les
seuils 79/80/81 et 219/220/221, un A/B 30/80/140/220 dans une scène fixe, la
tombe rouge puis verte, un changement de partie, et les témoins performance
Frigid Highlands/Halls of Pain; aucune tournée manuelle de toutes les zones
n'est requise.

Implantation terminée : MapSense parcourt les 128 buckets du type monstre dans
la table client gouvernée, suit uniquement le lien `Unit+0x158`, borne chaque
bucket et chaque scan, et limite la collecte à une fois par 50 ms. Le pipeline
hostile/rayon/projection/clip existant demeure unique; les compteurs ajoutés
distinguent les bandes 0–80, 81–140, 141–220 et au-delà. Les tests couvrent
exactement 79/80/81 et 219/220/221. Cette portée reste honnêtement limitée aux
Units déjà répliquées chez le client.

Dans Canyon, le resolver lit `Drlg+0x120` par le getter gouverné, n'accepte que
les tombes 66–72, initialise uniquement la tombe produite et réutilise son
RoomTile exact. Le même endpoint est rouge avant `A2Q6/RewardGranted`, puis vert
après. Les tests de politique prouvent l'identité de l'id, des coordonnées
client exactes et du point publié entre les deux états. Le schéma 8 expose la
ligne de quête; le TOML runtime schéma 7 reste byte-identique, mais sa valeur
cachée historique `false` migre une fois à `true` en mémoire, comme le prouve le
test de migration. Arcane→Canyon demeure inchangé. `Toggle Reveal All` ne reçoit
aucune initialisation massive : seuls les diagnostics `ActChanged` armé/queue
sont ajoutés tant que l'inconstance n'est pas reproduite.

Le workbench gouverné reste `ready`; le build intégré passe `/W4 /WX`, CTest
`1/1`, quatre exports et PE 0.12.0. La DLL finale dépôt/runtime mesure 777 216
octets et vaut SHA-256
`7E4F56DC7A47BB6B2EC73232D0A71248F10DB81BCD753F37FFE8BD38A5AB04D9`.
Le cold start complet confirme le chargement du lot avec la pile active et le
renderer partagé. Les gates **source/fingerprint/politique/migration/build/
déploiement/cold start sont PASS**. L'A/B fixe 30/80/140/220, le Canyon rouge
puis vert et la constance Reveal All restent **NOT RUN gameplay**; aucun succès
visuel n'est déduit du cold start.

### Correctif de stabilité D3D12 intégré au candidat 0.12.0

L'incident du 28 août 2026 à `15:45:48` est un retrait de périphérique GPU
distinct des assertions TACT : Windows enregistre `nvlddmkm` classe `0x855`
`Subchannel 0x0 Mismatch`, puis un `LiveKernelEvent 0x141`; onze millisecondes
plus tard D2Prism reçoit `DXGI_ERROR_DEVICE_REMOVED` `0x887A0005` dans
`Present`. D2Prism constate donc le retrait après le TDR et n'en est pas la
cause initiale.

Le host 0.11.2 vérifiait que la première command queue `DIRECT` observée
appartenait au même device D3D12, mais ne prouvait pas qu'elle était la queue
exacte fournie à la création du swapchain. Ce propriétaire heuristique est
supprimé. Le candidat 0.12.0 intercepte les variantes DXGI `CreateSwapChain*`,
enregistre l'identité COM exacte `{swapchain, command queue}` et ne soumet
aucune commande GPU depuis `Present` sans cette preuve. Il ne hooke plus
`ExecuteCommandLists` globalement. Une association absente ou modifiée, un
échec de fence/allocator/list/Signal ou un résultat DXGI de retrait empoisonne
le renderer pour le reste du processus sans masquer le HRESULT original.

Un client ABI v2, notamment Floating Damage, peut continuer à produire sa
frame ImGui CPU, mais une frame sans command list ou vertex ne produit plus
aucun reset, barrier, execute ou signal GPU. Le build Release x64 passe
`/W4 /WX`, CTest `1/1`, quatre exports et PE 0.12.0. La DLL finale de 777 216
octets vaut SHA-256
`7E4F56DC7A47BB6B2EC73232D0A71248F10DB81BCD753F37FFE8BD38A5AB04D9`.

Le même hash est déployé dans le profil BKVince mod-local, avec le TOML actif
préservé byte-exact à
`B71F272932AECA9CC7B325161252BD88D96B29F198E46C5E7241B015A56AB3E7`.
Le cold start frais D2R 3.3.93847 charge 36 plugins, conserve le seul échec
Revive Overhaul préexistant, applique les 18 patches et atteint `24/24`. Les
hooks DXGI précoces s'installent avant l'initialisation graphique; le log
enregistre ensuite la command queue exacte pendant `CreateSwapChain`, initialise
le host ImGui et rend la première frame Floating Damage partagée. Windows ne
produit aucun nouvel événement `nvlddmkm` ou `LiveKernelEvent 0x141`. Ce gate
est **PASS source/build/déploiement/cold start technique**; il ne revendique pas
encore les témoins gameplay distincts du lot Canyon/rayon.

## Validation future

- configurations absente, valide et invalide;
- portées globale et mod-locale avec repli exact;
- pile RuffnecKk complète et cinq DLL eezstreet, toutes fonctionnalités
  actives, sans retrait ni neutralisation;
- mode autonome sans fournisseur, puis coexistence avec Floating Damage dans
  les ordres de chargement pertinents;
- panneau ImGui in-frame au frontend et en jeu, souris/clavier/manette, curseur,
  absence de click-through, drag, réduction et clics répétés sans gel;
- contrôles futurs, persistance différée et échecs d'écriture dans un gate
  distinct des actions Reveal;
- futur rendu automap aux résolutions et modes d'affichage supportés, avec
  déplacement, zoom, Tab, Alt-Tab et changements de niveau;
- actes, difficultés, seeds, transitions et changements de partie;
- deux mods aux données différentes afin de prouver l'absence de dépendance à
  BKVince;
- huit locales, dont glyphes coréens et chinois;
- clavier, souris, manette et résolutions supportées;
- approbation visuelle explicite des maquettes puis du témoin en jeu;
- solo offline, TCP/LAN hôte et client;
- mesures CPU, mémoire, génération et cache;
- build Release x64, exports, versions et hashes build/dépôt/runtime/archive;
- aucune écriture directe de gameplay/réseau ni format de sauvegarde
  propriétaire; réécritures natives D2R des sidecars inventoriées.

Chaque case restera `not run`, `passed`, `failed` ou `blocked`; aucune preuve
statique ou cold start ne sera transformé en succès gameplay.

## Rollback et frontière Git

Le runtime BKVince normal porte MapSense 0.12.0, SHA-256
`7E4F56DC7A47BB6B2EC73232D0A71248F10DB81BCD753F37FFE8BD38A5AB04D9`;
le TOML runtime demeure sur `diagnostics = false`, SHA-256
`B71F272932AECA9CC7B325161252BD88D96B29F198E46C5E7241B015A56AB3E7`.
La copie byte-identique de rollback 0.11.2 (`8B7CF814…DEA2A4E`) et le
TOML pré-déploiement sont conservés sous
`analysis-cache/runtime-sync-backups/mapsense-0.12.0-tdr-20260828-162856/`.
La copie exacte de rollback 0.11.0 (`72858DDE…E32B706`) est conservée sous
`analysis-cache/runtime-sync-backups/mapsense-0.11.0-performance-20260828-0828/`.
La copie 0.10.4 (`AE5F1198…EFF5E5B5`) demeure à côté du plugin runtime sous le
suffixe `pre-0.11.0.20260828-072918.bak`. Aucun profil jetable ni sidecar n'a
été créé dans l'installation. La copie exacte de rollback 0.9.9
(`11C7D4B6…1527C03`) est
conservée sous
`analysis-cache/runtime-sync-backups/20260827-144746/mods/BKVince/d2rloader/plugins/`.
La copie exacte de rollback 0.9.8 (`BB8626AB…FE9F9DF6`) est conservée sous
`analysis-cache/runtime-sync-backups/mapsense-0.9.9-20260826-160205/`. La
copie exacte de rollback 0.9.6 (`57FE5474…E8CF900`) est
conservée sous
`analysis-cache/runtime-sync-backups/mapsense-0.9.7-20260826-121713/` jusqu'au
verdict gameplay, puis pourra être supprimée. La
0.4.1 demeure le dernier squelette dont l'interface, les Reveal, le curseur,
l'isolation des clics, les performances et Floating Damage avaient tous été
confirmés ensemble. MapSense ne crée aucun format de sauvegarde propriétaire;
les données automap déjà écrites par D2R dans ses sidecars peuvent toutefois
rester présentes.

Le workstream couvre cette mission et les futurs fichiers propres à
`RuffnecKkMapSense`. `ROADMAP.html`, `Mission/CURRENT.md`, le cadastre et les
registres natifs restent partagés. Le `reveal-map.toml` préexistant n'est pas
revendiqué avant identification de son propriétaire. Aucun commit ni push
n'est effectué sans demande explicite de Vincent.
