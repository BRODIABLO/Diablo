# RuffnecKk MapSense — D2R 3.2.92777 et 3.3.93847

Dernière mise à jour : 3 septembre 2026

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

État actuel : **MapSense 0.13.35 est déployé en portée mod-locale et son cold
start officiel passe; le verdict gameplay humain reste ouvert**. Zig 0.16.0
ReleaseSafe, Release x64 `/W4 /WX`, CTest `1/1`, la matrice MS1 v3/MSA1 v2 de
quatre seeds × cinq actes et les gates custom BKVince/LevelId 733 passent. Le
gate visuel mort/chargement est implanté sans nouvelle RVA, signature, ABI ni
propriété de hook. Le runtime doit encore confirmer la disparition immédiate
à la mort et au chargement, puis la restauration correcte après transition.

La 0.13.34 corrige le raccourci outdoor incomplet du helper : masque DT1 par
pièce, passes `LvlSub` waypoint/shrine/terrain, ordre de seed et coutures entre
pièces sont maintenant fidèles sans collision rasterisée ni room gameplay dans
D2R. Une couche n'est plus créditée avant qu'un vrai passage de rendu automap
retrouve ses témoins exacts dans les arbres natifs floor/wall. Le verdict
visuel Actes III/V, les transitions inter-actes, les FPS et l'absence de freeze
restent ouverts sur les hashes 0.13.34 documentés plus bas. Les acquis coffre
PrimeMH, navigation, marqueurs live, immunités, boss et master dynamique restent
dans leurs pipelines existants et ne sont pas régénérés par le helper.

Le catalogue de session immuable charge en priorité `levels.txt`, `shrines.txt`,
`superuniques.txt`, `monstats.txt` et `objects.txt` depuis les racines du mod
actif exposées par D2RLoader. Il copie les noms UTF-8 résolus par
`LocalizationServiceV1` avant le rendu, refuse les overrides BIN-only ou TXT
invalides et ne cherche jamais implicitement BKVince. Le catalogue actif exige
désormais le lancement D2R avec `-txt`; sans cette preuve, labels et objets se
désactivent plutôt que de risquer une divergence TXT/BIN. Les IDs runtime de
`MonStats.txt` et `Objects.txt` suivent l'ordre des lignes hors séparateur
`Expansion`, jamais les colonnes commentaire `*hcIdx`/`*ID`. Les lignes
techniques sans clé d'affichage restent valides. Les tests intégrés prouvent la
priorité du TXT actif sur un fallback explicite, ces lookups ordinaux, la
localisation UTF-8, les lignes spéciales de `levels.txt`, le rejet des doublons
et headers invalides ainsi que l'absence de fallback BKVince. Le prédicat shrine
générique est `InitFn == 1 && (SubClass & 1) != 0`; il conserve les Healing/Mana
Wells qui utilisent réellement le contrat shrine et exclut les Fountain/Well
`InitFn 16`. Les correctifs
`sFillLocation` et les lignes verte/rouge des versions 0.12.3–0.12.5 restent en
place; le GPS suivant les corridors est différé et ne fait pas partie de 0.13.0.

La 0.13.6 refuse maintenant tout succès de localisation qui renvoie simplement
la clé technique (`ShrId9`, `Cellar of Pity`, etc.). Les champs humains des TXT
servent uniquement de témoins pour détecter cet écho; ils ne remplacent jamais
le texte du jeu. Le libellé affiché vient donc toujours du service de
localisation D2R dans la langue locale du client. L'empreinte native des POI est
validée tôt au chargement, avant que les autres plugins puissent accrocher le
getter partagé, puis le catalogue localisé est lié tard après l'initialisation
de la langue. Enfin, une intention Reveal rejouée après Save & Exit n'est plus
considérée satisfaite avant une vraie observation native de l'automap de la
nouvelle partie.

Limite de packaging assumée pour ce candidat : la DLL 0.13.0 ne redistribue pas
les cinq TXT vanilla de Blizzard. Un mod partiel doit livrer la table concernée,
un sous-dossier `base` cohérent ou un compagnon explicite `vanilla-excel`; une
famille absente se désactive proprement. BKVince fournit les cinq tables actives,
donc ce point ne bloque pas sa qualification runtime actuelle.

Le lot 0.12.1 traite ensemble les régressions observées : `Present` ne bloque
plus sur la fence d'un back buffer encore en vol; la découverte complète des
monstres passe à 10 Hz tandis que leurs positions courantes sont rafraîchies à
environ 60 Hz depuis des IDs recopiés; la sortie extérieure verte utilise la
frontière collision exacte du niveau entier et refuse les ouvertures ambiguës;
Reveal Level, Reveal Act et Reveal All conservent leur intention entre parties
à difficulté identique pendant la vie du processus, puis l'effacent uniquement
lors d'un changement réel de difficulté. Le Canyon demeure inchangé : même
RoomTile exact, rouge pendant la quête et vert après récompense. Les compteurs
`mapsense status` distinguent maintenant réplication client, découverte,
résolution des IDs, coût de rafraîchissement, bandes de distance et rejet par
clip; aucun résultat visuel ou de performance n'est déduit de ces preuves
statiques.

L'architecture visible reste inchangée : aucun second `HWND`, processus,
canvas plein écran, backend OpenGL/WGL ni thread de rendu. `Map & Reveal`
conserve `Additions Opacity`, `Reveal Level`, `Reveal Act`, `Toggle Reveal All`
et `Reveal All Off`; `Monsters` expose portée, épaisseur, forme, couleur, alpha
et taille; `Immunities` conserve `colored_i`, `split_halo` et ses six couleurs.
Les familles Direct restent waypoint bleu, progression verte Acts I–V, tombe
de quête rouge et sorties personnalisées mauves; seule la liste mauve
`level_id`/`level_name` exige une édition TOML manuelle.

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
- les chests, leurs états locked/trapped, les super chests étoilés, les shrines,
  weapon racks et armor racks;
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
restent `NOT RUN` et soumis à l'approbation de Vincent. Les lignes Direct
qualifiées restent soumises à leurs régressions visuelles. Les labels et objets
sont maintenant implantés dans le candidat 0.13.0, mais leurs couleurs, tailles,
polices, priorités d'empilement et comportement en jeu restent **NOT RUN**
jusqu'au témoin humain. Le GPS reste un lot futur séparé.

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

Le candidat MapSense 0.13.0 est **IMPLANTÉ ET VERT EN SOURCE/BUILD/TESTS
STATIQUES**. Son prochain gate est le déploiement byte-identique avec backup,
le cold start pile complète, les logs frais, puis la validation runtime et
visuelle humaine des labels localisés, noms SU/boss, chests, super chests,
racks et contrôles du menu. Les lignes rouge/verte, les correctifs
`sFillLocation`, les marqueurs et les immunités font partie de la régression.
Le GPS suivant les corridors reste différé et n'est pas un gate de 0.13.0.

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

### Candidat autorisé 0.12.1 — fluidité, frontière exacte et Reveal persistant

Le 28 août 2026, après les témoins montrant des marqueurs devenus saccadés, une
baisse à environ 100 FPS à Lut Gholein, aucune différence visuelle au-delà
d'environ 80–90 subtiles, une ancre Tamoe→Monastery encore incorrecte et la
perte des Reveal après Save & Exit, Vincent autorise par `GO` un correctif
unique sans négliger aucun de ces signalements. Reveal Level, Reveal Act et
Reveal All doivent se comporter comme une exploration déjà faite entre parties
de même difficulté; seule une transition réelle Normal/Nightmare/Hell efface
leur intention mémorisée.

Le host D3D12 conserve l'association exacte swapchain→command queue et tous les
gates fail-closed de 0.12.0. Il ne fait toutefois plus de `Wait` de fence dans
le chemin critique `Present` : si l'allocator du back buffer courant est encore
en vol, seule la frame overlay est omise et D2R poursuit immédiatement son
`Present`. Les attentes de fence restent réservées au cycle de vie du renderer,
notamment resize et shutdown. Cette propriété ferme le blocage source; elle ne
vaut pas encore preuve FPS en jeu.

Le pipeline monstre est séparé en deux cadences bornées. Toutes les 100 ms au
plus, la découverte lit les 128 buckets client et ne conserve que
`{unitId, rank, immunityMask}` dans des vecteurs préalloués. Toutes les 16 ms au
plus, un rafraîchissement résout chaque ID par le getter natif gouverné, copie
sa position `DynamicPath` courante, projette et publie; aucun `Unit*`,
`DynamicPath*`, contexte automap, pixel ni seed n'est retenu. La commande
`mapsense status` ajoute le nombre suivi, les scans et rafraîchissements avec
temps moyen/maximal, les IDs résolus/manquants, puis les acceptations et rejets
de clip par bandes 0–80, 81–140, 141–220 et >220. Si aucune Unit hostile
n'existe dans une bande lointaine côté client, MapSense ne peut pas l'inventer;
si elle existe mais reste rejetée, les compteurs identifient rayon, projection
ou viewport. Le prochain A/B runtime doit utiliser ces preuves et non la seule
impression visuelle.

La résolution extérieure abandonne le produit quadratique des flags de
visibilité avec toutes les rooms et le fallback « ouverture la plus large ».
Elle construit une seule fois par refresh la frontière collision extérieure du
niveau source complet, fusionne ses spans contigus, collecte les spans cible
par les vrais liens `RoomsNear`, puis exige exactement une ouverture partagée
disjointe. Une ambiguïté échoue fermée; un `RoomTile` exact reste prioritaire.
Les tests couvrent l'ouverture étroite correcte face à une large couture
interne, le rejet de deux ouvertures, l'endpoint exact sur la frontière, la
priorité RoomTile, les débordements et la réutilisation du cache source. Le
résultat Tamoe→Monastery reste néanmoins **NOT RUN gameplay**, à confirmer sur
au moins deux seeds.

La persistance Reveal est intentionnelle mais limitée au processus MapSense :
aucun TOML, save de personnage, sidecar propriétaire, DRLG, pointeur, pixel ou
coordonnée générée n'est conservé. Les LevelIds, ActIds et l'intention Reveal
All restent mémorisés; une nouvelle partie de même difficulté réinitialise
seulement l'acceptation/déduplication de session et rejoue la demande lorsque la
géométrie correspondante devient disponible, avec huit tentatives au plus
espacées de 250 ms. L'acte est dérivé des bornes LevelId autoritaires 1–39,
40–74, 75–102, 103–108 et 109–137 plutôt que de l'événement d'entrée. La
difficulté provient du témoin strict `Drlg+0x830`; seules les valeurs 0–2 sont
acceptées et toute valeur inconnue échoue fermée. Les tests prouvent la reprise
de même difficulté, le rejet invalide et les transitions 0→1→2→0 qui effacent
toutes les intentions. Le comportement effectif après Save & Exit et changement
de difficulté reste **NOT RUN gameplay**.

Le resolver Canyon n'est pas modifié : il conserve la bonne tombe générée et
le même RoomTile exact, rouge pendant Acte II Q6 avant `RewardGranted`, vert
ensuite pour farmer Duriel. Sa régression visuelle est explicitement incluse au
gate, sans en anticiper le résultat.

Le build intégré Release x64 passe `/W4 /WX`, CTest `1/1` et l'audit des quatre
exports. La DLL mesure 1 320 960 octets, porte PE 0.12.1 et vaut SHA-256
`DAB61AADE352C87B9CA4F57DF1184EAA663617103AC7B1B9B80182066F8C39B4`.
Le même hash est déployé dans BKVince sans changer le TOML actif. Le cold start
frais D2R 3.3.93847 / Build Key `623f7a1f73eabb08ccb2b2046e3f9164`
charge MapSense 0.12.1 avec la pile complète, rapporte 36 plugins chargés, le
seul échec Revive Overhaul préexistant, 18 memory patches et `24/24`. Les logs
installent les hooks DXGI précoces, enregistrent la command queue exacte pendant
la création de la swapchain, initialisent le host ImGui partagé et confirment la
première frame Floating Damage. Aucun nouvel événement `nvlddmkm`,
`LiveKernelEvent 0x141` ni Windows Error Reporting correspondant n'apparaît après
ce lancement. L'état est **PASS source/build/tests statiques/déploiement/cold
start technique; NOT RUN gameplay**.

### Candidat autorisé 0.12.2 — suppression du faux rayon et des workers Reveal

Le 28 août 2026, Vincent tranche que le réglage de « distance de scan » est
trompeur et autorise par `GO` sa suppression complète. MapSense ne scanne pas
spatialement des subtiles : il énumère la table monstre cliente complète, puis
le rayon 0.12.1 rejetait après coup des Units déjà trouvées. La configuration
0.12.2 ne contient donc plus `detection_radius`; les fichiers schéma 1–8 qui le
portent encore l'acceptent et l'ignorent, puis l'omettront à la prochaine
sauvegarde. Toute Unit hostile connue du client passe désormais jusqu'aux seuls
gates de projection et de clip automap natifs. Les bandes 80/140/220 restent
uniquement des diagnostics de portée réelle, jamais des filtres.

La fluidité ne repose plus sur un plafond artificiel de 16 ms. Les positions
copiées sont résolues à chaque pulse automap du joueur local, tandis que le scan
coûteux de métadonnées/immunités reste limité à 100 ms. La revue concurrente a
fermé deux risques avant déploiement : le hot path reste borné par la capacité
table absolue de 32 768 IDs et rafraîchit le set suivi complet à chaque pulse;
le producteur remplace un snapshot complet par les dernières
positions au lieu d'empiler les mêmes IDs lorsque `Present` tarde. Les buffers
préalloués, l'epoch et le compteur de writers restent les frontières de
concurrence; aucune allocation n'est requise dans ce chemin par pulse.

Tamoe Highland→Monastery Gate conserve maintenant la paire `RoomsNear` native
exacte pendant toute la collecte collision : identité room source, identité room
cible et coordonnée fixe de couture. Seuls les fragments de cette même paire
peuvent fusionner ou s'intersecter. L'endpoint est le midpoint de l'ouverture
sur la cellule source (`fixed` pour Left/Top, `fixed - 1` pour Right/Bottom), et
deux paires valides restent ambiguës et échouent fermées. Les fixtures couvrent
la paire exacte, le rejet d'une paire croisée, deux coutures parallèles et
l'ambiguïté explicite de deux paires.

Tous les chemins `ExecuteConsoleCommand` / `revealmap` et leur bridge D2RCore
sont supprimés. Reveal Level révèle et mémorise le LevelId actif; Reveal Act et
Reveal All révèlent le niveau actif par le callback natif déjà fingerprinté,
puis répètent seulement cette opération courante à l'entrée de chaque niveau
correspondant. Aucun worker d'initialisation massive d'acte ne peut donc être
amorcé par MapSense. La persistance reste process-lifetime et se vide seulement
sur un changement Normal/Nightmare/Hell validé. Chaque reprise est liée au
LevelId exact : un `LevelChanged` différent renouvelle les huit essais, un
`ActChanged` sans LevelId ne remplace pas une cible précise, et un DRLG encore
sur l'ancien niveau est refusé sans créditer le mauvais niveau.

Un build intermédiaire 0.12.2 a passé `/W4 /WX`, CTest `1/1`, PE 0.12.2 et les
quatre exports. Après ce gate, deux fixtures ont été renforcées et les
durcissements finaux de reprise Reveal et de publication monstre ont modifié la
source. Vincent a ensuite explicitement demandé de ne lancer aucun autre test
avant d'aller dormir. Conformément à cette demande, l'instance D2R existante
n'a pas été fermée, aucun runtime n'a été déployé et aucun cold start n'a été
lancé. L'état exact 0.12.2 demeure donc **READY FOR FINAL OFFLINE BUILD; NOT RUN
final CTest/deployment/cold start/gameplay**. Le prochain lot doit reconstruire,
déployer mod-local après backup, retirer seulement la clé runtime
`detection_radius`, redémarrer une instance BKVince complète, puis vérifier
absence de `sFillLocation`, FPS, fluidité, Tamoe→Monastery et les régressions
Canyon/Reveal déjà passées.

### Correctif autorisé 0.12.3 — suppression autonome du spam sFillLocation

Le 29 août 2026, après reproduction et trace native de la rafale généralisée,
Vincent exige que la correction appartienne à MapSense pour tous ses
utilisateurs et refuse qu'elle dépende d'un patch JSON propre à BKVince. Le
correctif 0.12.3 reste donc dans la DLL autonome hybride globale/mod-locale;
aucun sidecar de patch BKVince ne fait partie du produit.

MapSense réduit d'abord les matérialisations de rooms qui ne portent aucun
preset de sortie pertinent et déduplique les demandes nécessaires pendant un
refresh. Les matérialisations indispensables à Reveal, au waypoint ou à la
frontière collision exacte Tamoe→Monastery restent natives. Pour celles-ci,
la DLL devient propriétaire unique du CALL de diagnostic négatif à
`0x3E1F2B` : elle exige l'empreinte exacte `E8 60 FC 63 00` et remplace ces
cinq octets par des NOP via le service suivi de D2RLoader avant le premier
usage natif. Toute différence refuse proprement le chargement; le nom et le
numéro de build restent seulement diagnostiques. La branche native saute déjà
le remplissage après le message, donc aucun accès hors limites ni comportement
de room n'est modifié.

Le même lot remplace la fausse sélection par largeur : Tamoe utilise l'ancre
native unique du Monastery Gate `(Level 26 + 27, +13)`, tandis que les jungles
acceptent un passage exact et stable lorsque leur générateur expose plusieurs
sorties légitimes vers le même niveau. Les deux côtés exigent les bits de
visibilité réciproques, la paire `RoomsNear` exacte et le masque de collision
joueur complet `0x1C09`.

État : **PASS**. Release `/W4 /WX`, CTest `1/1`, PE 0.12.3, quatre exports,
self-test natif, déploiement byte-identique, cold start pile complète et témoins
gameplay anti-spam/Tamoe/Spider Forest/Great Marsh/Flayer Jungle sont fermés sur
D2R 3.3.93847. Le corpus commun gouverné couvre également 3.2.92777 par
équivalence byte-exact de ces surfaces. La référence PluginPack épinglée
`dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` ne possède ni ce symbole ni cette
plage.

### Candidat autorisé 0.12.4 — routes rouges statiques des quêtes normales

Le 29 août 2026, Vincent approuve le modèle simple inspiré du comportement de
PrimeMH sans copier sa liste telle quelle : la progression principale reste
verte, tandis qu'une whitelist indépendante du journal de quêtes publie en
rouge les embranchements associés aux quêtes normales. Pit, Crypt, Mausoleum,
Ancient Tunnels, Arachnid Lair, Swampy Pit, autres temples de Kurast, caves de
farming glacées, red portals, Cow et Pandemonium restent exclus. Les routes de
campagne partagées — Andariel, Claw Viper, Durance, Chaos Sanctuary, Ancients
et Baal — restent vertes. L'exception Canyon demeure inchangée : tombe correcte
rouge avant récompense, verte après.

La whitelist de sorties couvre Den of Evil, Burial Grounds, Forgotten Tower et
ses cinq cellars; Lut Gholein Sewers, Halls of the Dead et Maggot Lair; Spider
Cavern, Flayer Dungeon, Kurast Sewers depuis Bazaar ou Upper Kurast et Ruined
Temple; Frozen River et la chaîne de Nihlathak. Une continuation à l'intérieur
d'un donjon de quête devient rouge au lieu de superposer une ancienne verte.
Le même scan passif de `PresetUnit` publie seulement des objets de quête prouvés
dans les données 3.3 actives. Frigid Highlands conserve toutes les cages
générées comme points immuables, mais le rendu choisit dynamiquement la plus proche du joueur,
sans nouvelle lecture du DRLG ni rescan périodique.

Séquence retenue : moteur commun et priorité de couleur; matrice cinq actes et
POI terminaux sûrs; fixtures exhaustives et exclusions; build/self-test/PE;
déploiement byte-identique et cold start pile complète; enfin témoins gameplay
ciblés et logs propres. État au 29 août 2026 : **SOURCE, BUILD, TESTS, PE,
DEPLOYMENT, COLD START et LOGS PASS; GAMEPLAY PARTIAL PASS**. Vincent a confirmé
en jeu la coexistence de la ligne verte de progression et de la ligne rouge
Dry Hills vers Halls of the Dead, ainsi que la continuation rouge Sewers 1 vers
Sewers 2 pour la quête de Radament. La matrice complète des cinq actes reste le
prochain gate fonctionnel. La DLL Release et sa
copie mod-locale sont byte-identiques, 2 113 024 octets, SHA-256
`132A1A3D977E4BA046EE6392E90E41B04B3A5FC7487A93B573DED5C3F6C02270`.
Le runtime officiel 3.3.93847 charge MapSense 0.12.4, accepte l'empreinte native,
installe les deux suppressions suivies `sFillLocation`, initialise le renderer
et atteint `D2R startup complete` avec 36 plugins chargés et 18 patches. L'unique
échec Revive Overhaul est préexistant et hors MapSense; aucune rafale
`sFillLocation()` fraîche ni erreur MapSense n'apparaît dans les logs du test.

### Candidat autorisé 0.12.5 — synchronisation visuelle D3D12

Vincent observe depuis les derniers candidats des flashes ou artefacts brefs à
droite de l'écran, y compris au menu principal. Une capture reproductible montre
un artefact transitoire dans cette région; les logs ne montrent ni retrait du
périphérique ni réinitialisation GPU. La régression plausible est le changement
0.12.3 qui sautait un frame overlay lorsque la fence du back buffer n'était pas
terminée. Le candidat 0.12.5 restaure l'attente stricte par back buffer avant la
réutilisation de son command allocator, sans modifier les correctifs
`sFillLocation` ni la navigation verte/rouge de 0.12.4.

État : **PASS — SOURCE, BUILD, CTEST, SELF-TEST NATIF, PE, EXPORTS,
DÉPLOIEMENT, COLD START, LOGS ET CONFIRMATION VISUELLE HUMAINE**. La DLL Release x64 mesure
2 113 024 octets, porte PE 0.12.5, expose les quatre exports attendus et vaut
SHA-256 `67AA43A32657EBB3AC9AFA2D1D719C80C2B2E2EEEE512DEE4C9531EFC1301CD9`.
Sa copie mod-locale est byte-identique. Le cold start officiel 3.3.93847 atteint
36 plugins, 18 patches et l'unique échec Revive Overhaul déjà connu. MapSense
initialise son renderer D3D12 sur la queue exacte sans timeout de fence, erreur
DXGI, erreur MapSense ni nouvelle rafale `sFillLocation`. Une frame du menu est
visuellement propre et Vincent confirme ensuite que le glitch intermittent a
disparu avec la 0.12.5.

### Candidat implanté 0.13.0 — labels localisés et Objects

Le 29 août 2026, Vincent autorise l'implantation en bulk après fermeture du spam
`sFillLocation` et stabilisation des lignes de navigation. La feature GPS qui
suivrait les corridors est remise à plus tard : 0.13.0 ajoute de l'information
à l'automap native sans calculer de chemin.

PrimeMH à la révision épinglée
`92b6a97d8e56346f8b63a88bb647c1af044d2c8b` reste une référence visuelle
seulement. Son code, ses assets et ses catalogues statiques ne sont pas copiés.
MapSense réutilise le passage automap natif déjà possédé par son unique hook et
publie des snapshots bornés sans pointeur D2R vivant. Les sorties générées sont
conservées comme labels persistants et résolues par `levels.txt`. Une shrine
active garde exclusivement l'icône fournie par D2R; MapSense ajoute au-dessus le
texte du buff issu de `shrines.txt` et de son `InteractType`, sans seconde icône.

Les snapshots monstre transportent maintenant `classId` et index Super Unique.
Le nom vient de `superuniques.txt` pour un Super Unique, ou de `monstats.txt`
pour les boss marqués `boss`/`primeevil`; il s'empile au-dessus du marqueur et
des immunités. Le panneau `Monsters > Super Unique / Boss` expose
`show_names`, `name_color` et `name_size` sans retirer le marqueur lorsque le
nom est désactivé.

La nouvelle catégorie `Objects`, placée avant Navigation, possède un master et
des contrôles indépendants pour chaque famille. Les chests ordinaires utilisent
la couleur normale, la couleur locked verte ou la couleur trapped rouge, avec
priorité au piège. Les super chests ont leur propre marqueur et des étoiles
activables, colorables et redimensionnables. Armor racks et weapon racks ont
chacun switch, couleur et taille. Les shrines exposent seulement switch,
couleur du texte et taille du texte; aucun contrôle d'icône MapSense n'existe.

Le catalogue immuable de session lit les cinq TXT physiques du mod actif à
partir de `PluginContext.activeMod` et `modDirectory`, puis résout et copie les
chaînes UTF-8 avec `LocalizationServiceV1` dans la langue locale du client avant
tout `Present`. Un override BIN-only, un header invalide, un doublon ou une
source ambiguë échoue fermé pour la famille concernée. Aucun chemin, nom ou
fallback implicite BKVince n'existe. Les tests couvrent la priorité du TXT actif
sur un repli vanilla explicitement fourni, les lignes spéciales Null/Expansion,
les lookups de chaque famille, la localisation UTF-8 et les refus précédents.

État : **SOURCE, CONFIG SCHÉMA 10, DEUX BUILDS RELEASE REPRODUCTIBLES `/W4
/WX`, CTEST `1/1`, PREUVES STATIQUES, DÉPLOIEMENT BYTE-IDENTIQUE, COLD START
PILE COMPLÈTE ET TÉMOIN VISUEL CIBLÉ PASS**. Le runtime affiche le nom persistant
de la sortie Stony Field en jaune dans Cold Plains et expose tous les contrôles
0.13.0 attendus. Les objets aléatoires réels, les noms SU/boss rencontrés, les
autres locales, TCP/IP, la manette et la matrice de résolutions restent
explicitement `NOT RUN`; aucun de ces suivis n'invalide le lot implanté.

### Validation runtime 0.13.0 — 29 août 2026

Les deux clean builds finaux
`analysis-cache/mapsense-0130-final-a-20260829` et
`analysis-cache/mapsense-0130-final-b-20260829` passent chacun CTest `1/1` et
produisent la même DLL de 2 617 856 octets, PE/PluginInfo `0.13.0`, avec les
quatre exports attendus et SHA-256
`F70C93B3001730C082C48544F92F82540707213DCB704F3BECD56D6583D5C329`.
Cette DLL est déployée byte-identique dans le profil BKVince mod-local. Le TOML
personnel schéma 9 est accepté par la migration mémoire, n'est pas réécrit et
reste byte-identique au SHA-256
`F24BA394B59D431D0F66C25EA413DBBA5F114107F0F3092DD39B2DF936ABFDB0`.

Le cold start frais utilise exactement
`D2RLoader.exe -mod BKVince -txt -offline` sur D2R officiel `3.3.93847`, build
key `623f7a1f73eabb08ccb2b2046e3f9164`, avec la pile complète inchangée. D2RLoader
charge MapSense 0.13.0 mod-local, rapporte 36 plugins chargés, l'unique échec
Revive Overhaul déjà connu et 18 memory patches, recompile 190 tables depuis
TXT puis atteint `24/24` et `D2R startup complete`. L'assertion TACT récurrente
est capturée et ignorée par `ignored_asserts.txt`; aucune nouvelle erreur
MapSense, erreur DXGI ou occurrence `sFill` n'apparaît dans les logs frais.

Le témoin gameplay entre avec `QtyTester`, voyage de Rogue Encampment vers Cold
Plains, ouvre l'automap puis confirme le libellé jaune persistant `Stony Field`
sur son ancre physique, avec les lignes verte/rouge et les marqueurs existants
toujours actifs. Le panneau in-frame confirme `Monsters > Super Unique / Boss >
Names`, puis la catégorie `Objects` et son master : labels de sortie, texte seul
des shrines, chests normal/locked/trapped, super chests avec étoiles, armor
racks et weapon racks possèdent tous leurs switches, couleurs et tailles. La
partie est quittée par `Save and Exit`, puis D2R est fermé proprement; aucun
processus D2R/D2RLoader ne subsiste.

Le backup récupérable du runtime 0.12.5 et du TOML pré-déploiement se trouve
sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.0-20260829T192433/`.

### Correctifs 0.13.1–0.13.6 — panneaux, vrais noms et reprise Reveal

Les retours gameplay suivant 0.13.0 ont mis en évidence trois régressions
distinctes : le rendu MapSense traversait certains panneaux natifs; la
localisation pouvait accepter comme nom joueur une clé technique renvoyée en
écho; les intentions Reveal rejouées trop tôt au chargement pouvaient être
marquées acceptées avant que l'automap de la nouvelle partie existe réellement.
Les itérations 0.13.1–0.13.5 ont stabilisé l'occlusion native sans cacher le
menu MapSense ni neutraliser ses hotkeys. Le 30 août, Vincent confirme les
témoins Inventory, Skill Tree et Quest panel : **PASS gameplay**.

La 0.13.6 sépare ensuite l'initialisation POI en deux phases. L'empreinte
fail-closed des fonctions, signatures et ABI natives est validée au chargement
du plugin, avant la prise de hook concurrente observée sur le getter de classe;
le catalogue immuable est seulement lié après `LocalPlayerReady`, lorsque le
service de langue D2R peut produire de vraies chaînes joueur. Un retour identique
à la clé demandée, notamment `ShrId*` ou une clé technique de `Levels.txt`,
reste non localisé et n'est jamais dessiné. Les colonnes humaines du TXT sont
des témoins d'écho, pas un fallback d'affichage. Les tests couvrent notamment
`Cellar of Pity`/`Frozen River`, `Crystalized Cavern Level 1`/`Crystalline
Passage` et `ShrId9`/`Resist Cold Shrine` sans hardcoder leur traduction.

La reprise Reveal conserve toujours uniquement des intentions process-locales
par difficulté, sans sidecar ni mutation des sauvegardes. Après `GameLeft`, une
nouvelle session réarme l'intention; une tentative précoce ne la consomme plus.
Seul un passage automap natif réel de la nouvelle partie peut confirmer le
niveau rejoué. Le test unitaire exécute explicitement session 101, `GameLeft`,
session 102 et prouve la conservation de l'intention avec remise à zéro des
acceptations propres à la session.

Les clean builds finaux
`analysis-cache/mapsense-0136-final-a-20260830` et
`analysis-cache/mapsense-0136-final-b-20260830` passent chacun CTest `1/1` et
produisent une DLL byte-identique de 2 632 192 octets, PE/PluginInfo `0.13.6`,
avec les quatre exports attendus et SHA-256
`B4F837E4AD85991DC43FBB43BDEAC43423975F62D3465A2DDD8B409BA422E14B`.
La DLL mod-locale BKVince déployée possède le même hash. Le TOML personnel
reste byte-identique au SHA-256
`ADF1CDD796772EC34E53CCA1BFB8D2981CECB73FA2D26089E560CF73413023A6`.

Le cold start frais 0.13.6 utilise la pile complète avec
`D2RLoader.exe -mod BKVince -txt -offline` sur D2R officiel 3.3.93847. Il charge
36 plugins et 18 patches, recompile 190 tables TXT, atteint `24/24` et
`D2R startup complete`. MapSense installe ses deux suppressions `sFillLocation`,
ses hooks DXGI/D3D12 fail-closed et son hôte in-frame sans reproduire l'échec
POI `0x349860` au chargement. L'instance reste ouverte au frontend : le bind
localisé après entrée en partie ainsi que les témoins vrais noms de shrine,
vrais noms d'exits et Reveal → Save & Exit → retour restent **NOT RUN**.

Le backup récupérable du runtime 0.13.5 et du TOML pré-déploiement se trouve
sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.6-localization-reveal-20260830T000126/`.

### Candidat 0.13.7 — lisibilité D2R et libellés de sorties distantes

Le 30 août 2026, Vincent demande un lot visuel cohérent plutôt que plusieurs
gates : texte des shrines réellement au-dessus de l'icône native, police D2R,
jaune par défaut identique pour shrines et sorties, tailles largement
configurables, et coffres translucides dont les lignes structurelles sont
bleues, l'intérieur or, sans motifs décoratifs, avec lock conservé. Un coffre
verrouillé utilise désormais l'ambre et non le vert; un coffre piégé conserve
le rouge. Les labels sont soumis dans une passe finale afin qu'aucun coffre ou
rack ne puisse les recouvrir. L'asset PrimeMH consulté reste une référence
visuelle read-only : sa licence interdit la redistribution et aucun de ses
pixels n'est repris dans le rendu clean-room MapSense.

Le premier témoin 0.13.7 invalide le diagnostic initial de simple refresh : la
projection fonctionnait, mais le resolver ne publiait que le Level courant.
Reveal Level reconstruit donc uniquement ce Level; Reveal Act, Reveal All et le
replay whole-act parcourent maintenant la chaîne bornée des `Level*` appartenant
au DRLG actif et remplacent le catalogue par les sorties physiques de tous les
Levels matérialisés. Le changement de Level conserve ce catalogue; seuls les
changements d'acte ou de session le vident. Les lignes Direct-navigation restent
volontairement courantes au Level. Le nom d'une shrine n'est plus déduit de sa
seule Unit vivante : le hook observe aussi que le renderer automap natif vient
réellement de soumettre cette shrine, afin de ne jamais laisser de texte sans
l'icône D2R correspondante. La marge mesurée label/icône passe à 12 pixels.

Le même témoin a montré Aanishu recouvert par son pack. La passe monstre est
maintenant ordonnée `normal < minion < champion < unique < superunique/boss`;
un boss `MonStats` est promu même si son rang runtime n'est pas superunique, et
tous les noms de boss sont rendus après les icônes et indicateurs d'immunité.

Le build Release x64 et CTest passent (`1/1`). La DLL corrigée mesure 2 656 768
octets, porte PE/PluginInfo `0.13.7`, expose les quatre exports attendus et vaut
SHA-256
`EF57D9CA16705BE6F2934942CD412700CCDF2B456B94EEAC60C13739EB217643`.
Le même hash est déployé dans le profil BKVince mod-local. Le TOML personnel
schéma 10 n'a pas été écrasé et reste au hash
`9BF683A053F0DAA71A0EFE02379DE0859304D100FD0A0977BA3AB09C60CFB9FC`;
sa migration mémoire vers le schéma 11 ne remplace que les trois anciennes
couleurs exactement égales aux defaults, et préserve toute couleur custom.
Le premier candidat visuel 0.13.7 (`4EEEA69A…AAC`) et ce TOML sont conservés sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.7-act-labels-shrine-priority-20260830T081528/`;
le rollback exact 0.13.6 reste sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.7-visual-exits-20260830T075047/`.

Le cold start frais utilise la pile complète avec
`D2RLoader.exe -mod BKVince -txt -offline` sur D2R officiel 3.3.93847, Build Key
`623f7a1f73eabb08ccb2b2046e3f9164`; `.build.info` et `D2R.exe` conservent leurs
hashes gouvernés. MapSense 0.13.7 accepte son empreinte fail-closed, trouve
Exocet dans le package actif, installe ses hooks D3D12 et suppressions
`sFillLocation`, puis lie le catalogue localisé après l'initialisation langue.
D2RLoader charge 36 plugins et 18 patches, recompile 190 tables TXT, atteint
`24/24` et `D2R startup complete`. L'échec Revive Overhaul et l'assertion TACT
capturée restent les deux incidents préexistants documentés; aucune erreur
MapSense fraîche n'apparaît. Le cold start exact du hash corrigé à 08:19 charge
également 36 plugins, applique 18 patches, recompile 190 tables, atteint `24/24`
et initialise Exocet ainsi que l'hôte D3D12 in-frame; seul l'échec Revive
Overhaul déjà connu demeure.

La validation gameplay visuelle demeure **EN COURS / NOT RUN** pour le hash
exact : confirmer les ancres au-dessus des icônes, la police et les couleurs,
le nouveau coffre translucide, les vrais noms localisés de shrine/exit, puis
Reveal All avec déplacement de l'automap vers toutes les sorties distantes sans
déplacer le personnage. Reveal → Save & Exit → nouvelle partie reste aussi un
témoin gameplay explicite; le build et le cold start ne le transforment pas en
PASS implicite.

### Lot 0.13.8 approuvé — intersections orientées, rendu protégé et contrôle global

Le 30 août 2026, Vincent donne explicitement **GO** pour ce lot dans la DLL
autonome `RuffnecKkMapSense`, membre indépendante de la RuffnecKk D2RLoader
Suite. La portée globale/mod-locale, la configuration TOML dédiée, la baseline
SDK, l'empreinte native fail-closed et les propriétaires de hooks existants
restent inchangés; aucune DLL d'eezstreet n'est modifiée, liée ou redistribuée.

Le contrat approuvé traite chaque frontière physique comme une arête non
orientée et n'affiche qu'un seul nom à son intersection. Le nom présenté est
celui de l'autre côté relativement au Level courant : depuis une zone
antérieure on voit la zone suivante, depuis une zone ultérieure on voit la zone
précédente. La distance dans le graphe matérialisé tranche d'abord; l'ordre de
progression canonique ne sert que de départage stable, notamment dans les
bifurcations de l'acte III. Les labels de Level et de shrine sont rendus dans
une couche protégée finale; les icônes, immunités et noms de monstres doivent
s'en écarter sans modifier la priorité de rang monstre déjà validée.

La détection des special/sparkly chests est globale et sémantique, jamais
spécifique à Lower Kurast. Elle combine les classes runtime, leurs fonctions
d'initialisation et les presets dédiés matérialisés par le DRLG; Lower Kurast
est seulement le témoin gameplay le plus simple. Tous les special chests
réutilisent une géométrie clean-room commune de coffre en trois-quarts : rail
supérieur horizontal, contours bleus, intérieur or translucide, lock conservé,
aucun motif, et étoiles configurables au-dessus. La configuration sépare le
master coffre, la taille et la palette communes des accents locked/trapped et
des paramètres d'étoiles.

Le panneau obtient un master runtime persistant distinct du kill-switch de
chargement. Son bouton est dynamique : `Disable MapSense` lorsque les features
sont actives, puis `Enable MapSense` lorsqu'elles sont suspendues. Le lanceur,
le panneau et leurs hotkeys restent accessibles dans les deux états; les
préférences individuelles et l'intention Reveal All sont conservées. Cette
suspension ne peut pas remettre du brouillard sur des cellules déjà révélées
par D2R. Les actions deviennent idempotentes et portent les libellés
`Arm Reveal All` et `Disarm Reveal All`.

Le candidat Release 0.13.8 mesure 3 043 328 octets et vaut SHA-256
`FC5BDD5BA37FE8BE9A8F3FEC7C99375CF4DCF18FA876B74B37C84564FCC39A59`.
Deux builds Release `/W4 /WX` issus d'un rebuild propre sont byte-identiques;
CTest `1/1`, PE/PluginInfo 0.13.8 et les quatre exports passent. La DLL déployée
dans le profil BKVince porte exactement le même hash et la même taille.

Le cold start daté du 30 août 2026 charge MapSense 0.13.8 dans la pile complète,
36 plugins et les cinq DLL eezstreet, applique 18 patches, recompile 190 tables
TXT et atteint `24/24` puis `D2R startup complete`. MapSense valide son empreinte
fail-closed, acquiert la command queue D3D12 exacte, initialise l'hôte ImGui
in-frame, charge Exocet et rend le catalogue localisé disponible. Aucune erreur
MapSense fraîche n'apparaît; Revive Overhaul reste l'unique échec plugin connu
et l'assertion TACT ignorée reste l'incident loader préexistant. Le témoin visuel
du même processus montre le launcher et le panneau 0.13.8 en jeu, dont le bouton
actif `Disable MapSense` et les actions idempotentes `Arm Reveal All` /
`Disarm Reveal All`. Les gates build, tests, déploiement et cold start sont donc
**PASS**. Le verdict gameplay ultérieur accepte l'orientation des jungles et
les étoiles des special chests, mais rejette le spam de labels aux frontières,
la silhouette du coffre et l'absence de route verte utile depuis Great Marsh.
Ces trois points sont donc **FAIL 0.13.8** et motivent le lot 0.13.9; la bascule
off/on demeure à revalider sur le nouveau candidat.

### Lot 0.13.9 autorisé — identité physique, route Great Marsh et noms de waypoints

Le 30 août 2026, Vincent donne **GO** à la correction complète. Les sorties
outdoor retiennent maintenant une identité persistable formée de l'axe, de la
coordonnée fixe de la seam et de l'intervalle exact. Deux côtés réciproques de
la même frontière partagent cette identité même si leur point projeté diffère;
ils sont rendus une seule fois. Deux intersections structurellement distinctes
ne sont jamais fusionnées, même sous le seuil spatial historique. Une preuve
native plus forte peut déplacer le point affiché sans effacer l'identité de la
frontière. L'ancre native du Monastery Gate reste canonique et absorbe tous les
fragments de cette seule façade. Les tests couvrent les identités Right/Left,
la fusion au-delà de dix subtiles, la séparation sous dix subtiles, l'ancre
canonique et les deux vrais chemins outdoor de la fixture jungle.

Great Marsh conserve `77 → 78` comme cible principale lorsqu'une sortie directe
Flayer Jungle existe. Quand le seed fait de Great Marsh une branche morte, la
sortie exacte `77 → 76` devient le prochain hop vert vers Spider Forest puis son
bypass Flayer Jungle. Aucun point Flayer Jungle n'est inventé; les fixtures
prouvent la priorité 78, le fallback 76 et l'absence de destination sans preuve.

Le coffre est redessiné à neuf sans reprendre la géométrie rejetée : faces
trois-quarts régulières, rail supérieur, seam du couvercle et base horizontaux,
diagonales cohérentes, contours bleus, aplats or plus transparents, lock centré
et aucun motif. Les étoiles de special chest, déjà acceptées, ne changent pas.

Les waypoints utilisent le preset généré exact déjà résolu par la navigation,
y compris dans les cinq villes. Un POI séparé et retenu affiche
`<nom localisé du niveau> Waypoint` au-dessus de l'icône native; il reste actif
si la ligne bleue est désactivée. Reveal Level capture le waypoint du niveau
révélé; Reveal Act et Reveal All capturent ceux de tous les niveaux effectivement
matérialisés par leur parcours. Au Save & Exit, les coordonnées copiées sont
invalidées avec la session. Seuls les intents Reveal restent en mémoire de
processus, puis leur replay reconstruit le catalogue depuis la nouvelle seed :
aucune position de l'ancienne partie n'est réutilisée. Activation, jaune et
taille sont configurables sous `Objects`; le texte est précomposé dans le
catalogue immuable et rendu dans la couche protégée avec séparation des noms de
sorties. Le schéma 13 ajoute
`[objects.waypoint_labels]`; les schémas 1–12 migrent vers activé, jaune et
28 px sans écraser les préférences existantes.

Le collecteur Reveal-wide est maintenant strictement passif : il ne demande
aucune `ActiveRoom` supplémentaire et parcourt seulement les `DrlgRoom`,
`RoomTile` et `PresetUnit` que D2R a déjà générés. Un preset waypoint unique
donne sa position exacte; une ambiguïté conserve l'ancien owner. Les pending
sorties et waypoint sont publiés indépendamment, donc une famille incomplète
n'efface ni ne bloque une preuve complète de l'autre. La matérialisation
bornée reste réservée au resolver du niveau courant.

Deux builds Release propres et le build normal sont byte-identiques; CTest
`1/1` passe dans les deux arbres propres. La DLL de 3 117 568 octets porte
PE/PluginInfo 0.13.9, expose les quatre exports attendus et vaut SHA-256
`5C0FD8DE0D143FEF4B4D04C86C2F1D1D1B4B35379EE70CF372D46A0CD9BD1E9F`.
Le même hash est maintenant déployé mod-local dans BKVince. Le cold start frais
officiel D2R 3.3.93847 du 30 août accepte l'empreinte fail-closed complète,
initialise l'hôte D3D12/ImGui, compile 190 tables TXT, charge 36 plugins dont les
cinq eezstreet, applique 18 patches et atteint 24/24. Revive Overhaul demeure
l'unique échec plugin préexistant. Le TOML personnel a migré du schéma 12 au
schéma 13 sans perdre ses choix; après le test off/on, il conserve
`features_enabled = true`, les waypoint labels activés et vaut SHA-256
`748CF1EED016DD31EA1B58D13EA2B7C1A1CDDB6C0A2748F3B3B63609B3E3FE46`.

Le témoin live du même processus ferme trois gates 0.13.9 : le master switch
passe de `Disable MapSense` à `Enable MapSense` puis revient sans crash;
`Arm Reveal All` affiche `Kurast Docks Waypoint` en jaune au-dessus du waypoint
natif; après un vrai Save & Exit puis une nouvelle partie Insanity, sans
réarmer, l'automap révélée et ce nom de waypoint sont reconstruits. L'intent
Reveal survit donc au Save & Exit dans le même processus et la nouvelle session
recalcule ses POI au lieu de réutiliser les coordonnées quittées.

La déduplication générique, la silhouette finale du coffre, la route Great
Marsh et les trois transitions Frigid Highlands restent des gates gameplay
ouverts. Le crash Frigid Highlands reste
**non attribué** : le rapport frais montre une écriture nulle pendant
l'allocation native des données d'un portail permanent dans `D2RLoader.exe`,
aucune frame MapSense finale, une frame de retour BurnDamageFix non causale à
elle seule, et un assert TACT antérieur d'ordre temporel inconnu. Une influence
indirecte n'est pas exclue; trois transitions avec la pile complète et, en cas
de récidive, un dump complet restent le gate honnête.

### Candidat 0.13.11 — artwork PrimeMH exact et coordination Reveal

Le 30 août 2026, Vincent confirme avoir obtenu de Joffreybesos la permission
d'utiliser les images exactes de PrimeMH, choisit explicitement l'option A et
donne `GO` : les chests ordinaires emploient `chest.png`; tous les chests
spéciaux emploient `superchest.png` avec les trois étoiles PrimeMH déjà
intégrées. MapSense n'altère ni la palette ni les pixels de ces images. Il
ajoute seulement une serrure d'état séparée lorsque le chest est réellement
locked ou trapped; les valeurs par défaut sont aqua et rouge, le piège garde
la priorité, et les deux couleurs restent configurables.

Les payloads autorisés sont intégrés dans la DLL et décodés une seule fois par
WIC lors de l'initialisation du renderer. `chest.png` mesure 58×50, 7 025
octets et vaut SHA-256
`BA429FA42223DE03E4B347E0AE5F28CE188C4CBB140687C0A526A180BF869BDC`;
`superchest.png` mesure 69×110, 11 627 octets et vaut SHA-256
`D3DC7EE43A74B7BEA491576DC5B7E418D0CB22D38280C773FAAC0DF5D2372D2B`.
Le canvas spécial est ancré sur le centre du chest embarqué à `(36,85)`, pas
sur le centre des 110 pixels, afin que ses étoiles montent au-dessus sans
décaler le coffre. Deux SRV dédiés suivent le même cycle reset/resize/shutdown
que l'atlas ImGui. Un échec de décodage ou d'upload masque les textures et
conserve le dessin procédural historique comme fallback fail-safe.

Le schéma 14 conserve les anciennes clés de palette et d'étoiles pour rollback
et compatibilité de lecture, mais le panneau ne présente plus ces contrôles
trompeurs : seules la visibilité, la taille et les deux couleurs de serrure
d'état restent utiles. L'ancien amber exact `#D89B2BFF` migre vers l'aqua
`#00FFFFFF`; toute couleur personnalisée survit. Le crédit documenté est :
« Exact PrimeMH chest artwork by Joffreybesos, used with permission obtained by
Vincent Barrière on 2026-08-30. »

Le même candidat resserre la séquence Reveal All signalée comme lente. Un
`ActChanged` sans LevelId ne soumet plus une tâche anonyme susceptible de viser
le DRLG précédent; `LevelChanged` ou le premier callback automap fournit la
cible précise. Un Reveal Act/Arm explicite accepté et confirmé par le parcours
direct du niveau courant crédite immédiatement l'acte au lieu de programmer le
même replay 250 ms plus tard. Tant qu'une réconciliation Reveal est active,
les refresh navigation concurrents sont différés; chaque chemin terminal en
demande ensuite un seul. Aucune nouvelle RVA, signature, structure, ABI ou
surface de hook n'est introduite : `npm run re:d2r33 -- status` confirme le
workbench commun 3.2/3.3 prêt et vérifié.

Qualification statique finale : le build normal et deux arbres Release propres
sont byte-identiques. La DLL de 3 153 920 octets porte PE/PluginInfo 0.13.11,
expose `D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin`,
`D2RLoaderUnloadPlugin` et `RuffnecKkMapSenseGetOverlayHostApi`, et vaut
SHA-256
`388A26155ACFDB2B3F38243E88C19C75ED6F5027146339BF799BA18BDB2D69BA`.
CTest rapporte `1/1` dans les trois arbres et `git diff --check` est propre.
Conformément à la demande de Vincent, l'agent n'a ni arrêté/lancé D2R, ni
déployé la DLL, ni transformé ces preuves en validation visuelle ou en mesure
de latence gameplay.

### Fondation missiles native read-only — lot source du 31 août 2026

Vincent autorise explicitement le premier lot d'implantation de la fonction
missiles inspirée de PrimeMH, avec une cible indépendante de BKVince et de tout
autre mod particulier. Ce lot reste volontairement limité à la source native :
aucune classification `Missiles.txt`, option TOML, règle `incoming` ou primitive
de rendu live n'est encore branchée. L'ordre actif des classes demeure une
donnée brute du mod courant et aucun enum ordinal PrimeMH n'est compilé dans le
collecteur.

Le gate obligatoire `npm run re:d2r33 -- status` passe contre le runtime cible
D2R 3.3.93847 et le corpus commun gouverné de provenance 3.2.92777 : images
canonique et d'analyse ainsi que l'index sont vérifiés. Le contrat déjà promu de
`CLIENT_GetUnitByIdAndType 0x9A5D0` adresse 128 buckets par type avec un stride
`0x400`; `CLIENT_FindUnitInTypeBucket 0x9F270` prouve l'identité à `Unit+0x08`,
le type à `Unit+0x00` et le lien suivant à `Unit+0x158`. Le nouveau témoin unique
`UNITS_MissileTypeAndDataWitness 0x3F21E0` exige exactement le type 3 avant son
chemin missile. Son empreinte stricte de 32 octets est maintenant gouvernée dans
`known-rvas.json`; la table cliente des missiles commence donc à
`CLIENT_UnitHashTable 0x2A23910 + 0xC00`.

`native_automap_missile.cpp/.hpp` constitue un observateur séparé sous le
propriétaire existant de `AUTOMAP_RenderUnit 0xD76E0`; aucun second hook n'est
créé. Le passage local déjà prouvé lui prête synchroniquement le contexte
automap, son clip, les dimensions natives, la position monde du joueur et le
tick. Le collecteur parcourt uniquement les buckets clients de type 3, refuse
les pointeurs non alignés et les types inattendus, détecte les cycles par Floyd,
borne chaque bucket à 8 192 unités et la table à 32 768 unités, puis copie ID,
classe active, position monde et projection automap. Une table tronquée ou
cyclique ne publie aucun frame partiel.

Trois slots à états atomiques séparent l'écriture native et la future lecture
renderer. Ils ne contiennent que des valeurs, ne conservent aucun `Unit*` ni
`AutomapContext*`, expirent après 250 ms et sont invalidés avec les resets de
session, de niveau, d'acte, de master switch et de fermeture automap déjà
possédés par MapSense. Des compteurs bornés exposent scans, buckets, limites,
cycles, rejets, faults, publications et temps CPU. La source est activée avec
le master overlay pour rendre le prochain témoin runtime mesurable, mais aucun
pixel missile n'est encore consommé.

Les prochaines gates restent séparées : construire le catalogue mod-actif et
la taxonomie visuelle PrimeMH par nom/données, connecter la configuration et le
rendu d'ellipse, puis décider si une accentuation `incoming` temporelle mérite
une phase distincte. La compatibilité des mods binaires sans TXT exige toujours
une preuve gouvernée de la table `MissilesTxt` compilée avant d'être revendiquée.
Ce lot ne vaut pas validation runtime : build, tests statiques et cadastre sont
les seuls verdicts autorisés avant une qualification en jeu séparée.

Le contrôle statique du lot passe : build Release x64 `/W4 /WX`, CTest `1/1`,
`npm run re:d2r33 -- self-test`, validation JSON du registre RVA,
`git diff --check` et cadastre `VALID`. La DLL de travail mesure 3 170 816
octets et vaut SHA-256
`3938FB3A97003EE5EC89A9EFA867D17D746A7F9B7C62EAC135CBCEDF50DB98EB`.
Elle n'a été ni déployée ni chargée dans D2R; les compteurs live, le coût réel
en scène dense et la stabilité de la pile complète restent donc `not run`.

### Consommateur renderer neutre — lot du 31 août 2026

Vincent autorise la poursuite après le checkpoint de la source native. Le
premier consommateur copie le frame publié dans le vecteur renderer existant et
dessine une petite ellipse grise sous les marqueurs de monstres et sous tous les
libellés protégés. Il réutilise le clip natif/panneaux déjà gouverné et ne lit
aucun pointeur D2R depuis `Present`.

Ce rendu est volontairement non sémantique : toutes les classes actives sont
identiques, sans couleur élémentaire, sans ownership hostile, sans direction et
sans accentuation `incoming`. Il ferme uniquement le raccord technique
source→snapshot→projection→ImGui. La taxonomie mod-active par `Missiles.txt`,
la configuration utilisateur et l'historique temporel de trajectoire restent
des gates séparés. Aucun verdict visuel ou de performance gameplay ne sera
déduit du build statique de ce lot.

Le raccord compile en Release x64 sous `/W4 /WX`; CTest rapporte `1/1`, le
self-test du workbench D2R 3.3 rapporte `PASS`, le cadastre demeure `VALID` et
`git diff --check` ne signale aucune erreur. La DLL de travail mesure 3 175 424
octets et vaut SHA-256
`6ACEDCEC01C3803791D851D49DF56170188A39280DF870E14ABD9A67AD6878C1`.
Elle n'a été ni déployée ni chargée : présence visuelle, densité, clip réel et
coût de rendu restent `not run`.

### Nouvelle ère Reveal Map — atlas externe seed-driven 0.13.22

Le 1er septembre 2026, après le rejet en jeu des méthodes qui matérialisaient
les rooms distantes et provoquaient une chute de 30–40 % des FPS, Vincent
retient l'atlas externe et donne explicitement **GO**. Le produit n'expose plus
qu'une action et un hotkey `Reveal Map`. Le générateur est lancé
automatiquement par la DLL en priorité processus inférieure; aucun exécutable
séparé n'est lancé par l'utilisateur. Il reçoit uniquement le seed courant, la
difficulté, l'acte et le niveau témoin, puis produit des valeurs de géométrie et
de labels. Il ne matérialise aucune room D2R et ne peuple ni monstres, ni
objets, ni missiles.

La DLL charge un MSP1 déterministe contenant les 1 499 frames MaxiMap 16×32 et
les cinq palettes d'acte, puis dessine la topologie seed-exacte dans l'hôte
D3D12/ImGui existant. Cet atlas est strictement la couche MapSense la plus
basse. Les pipelines existants demeurent au-dessus et ne sont ni remplacés ni
redessinés : lignes vertes/rouges/mauves, POI et coffres live, missiles,
marqueurs de monstres par rang, immunités, noms de boss et labels protégés des
niveaux/waypoints/shrines. La projection réutilise le témoin affine natif
client→automap, ainsi que le pan, le zoom et le clip des panneaux gouvernés;
aucun nouveau hook automap natif n'est ajouté.

La publication est immuable et liée à la session, au seed, à la difficulté, à
l'acte et au niveau courant. La composante topologique continue seulement est
dessinée, de sorte qu'un donjon distant ne puisse pas être importé dans les
coordonnées d'une surface sans lien physique. Le cache est validé sous
`%LOCALAPPDATA%\RuffnecKk\MapSense\atlas-cache\v1`. Une intention MSI1 de 16
octets, exactement liée au seed et à la difficulté, restaure `Reveal Map` après
un cold start du même seed et refuse un seed rerollé. L'acceptation du reveal
natif est maintenant indépendante de l'arrivée asynchrone de l'atlas : ni une
génération lente ni un échec helper ne peuvent soumettre le même reveal d'acte
une deuxième fois. Les autres actes sont préchauffés en arrière-plan.

Le seed de spike `1395822899`, Hell, Acte III produit 28 niveaux et 30 276
cellules en environ 511 ms hors processus; la composante extérieure Kurast
75–83 contient 19 202 cellules. Le rendu lie sa texture une seule fois pour la
passe entière puis remplit directement un lot ImGui commun, au lieu de pousser
et retirer la texture pour chaque cellule. Ce changement ferme un coût CPU
évitable mais ne constitue pas une mesure FPS gameplay.

La qualification statique est **PASS** : build Release x64 `/W4 /WX`, CTest
`1/1`, `git diff --check`, cadastre régénéré et `VALID`, parseurs MSA1/MSP1
bornés, régénération MSP byte-identique, deux générations MSA1 byte-identiques
pour chacun des cinq actes et deux previews exactes vérifiées hors jeu —
composante extérieure Acte III cohérente et niveau 92 isolé sans fuite de
surface. Les artefacts courants sont :

- DLL 3 343 872 octets, SHA-256
  `3819A29C3FDFBDE7053BA03DEB6F8AD3928E0AD4481819C16E84AE3492BC2160`;
- helper 11 322 368 octets, SHA-256
  `FCC41FCDE3971B2B084CD556BD6B3D33241EEA16AEE0CD7A6918D9EBC5C5BF73`;
- MSP 790 320 octets, SHA-256
  `AD86C7651D896461902B08A8C0933901BDB95F8830499D581FF958482EDE85C0`.

Le runtime reste **NOT RUN** pendant que la tâche ISC12 possède l'instance D2R.
Les gates encore ouverts sont le cold start avec la pile complète, la présence
et l'alignement de l'atlas/labels/waypoints dans les cinq actes, la conservation
visuelle des couches live, le même-seed/new-seed, les changements de niveau et
de difficulté, puis les mesures FPS avant/après Reveal Map. Aucun ZIP public
n'est produit. `libd2` et D2MOO sont crédités; la provenance et l'autorisation
de redistribution de chaque payload dérivé restent à consigner formellement
avant une release publique.

### Spike natif post-0.13.22 — publication par `Levels.Layer`

Le témoin gameplay 0.13.22 invalide son terrain D3D12/ImGui : D2R dessine déjà
le niveau courant et le second atlas crée un doublon qui change de position et
flashe lorsque le personnage bouge. Vincent donne **GO** à un spike statique
strictement read-only; aucune injection ni DLL runtime n'est autorisée dans ce
gate.

Le corpus x64 commun prouve une route native sans room. Le callback standard
`0xD2240` résout `Levels.Layer`, obtient un propriétaire `0xB0` contenant quatre
arbres floor/wall/object/extra, puis alimente les arbres floor et wall. La clé
cellule de 12 octets contient frame et coordonnées automap; sa conversion exacte
depuis les tiles libd2 vaut `(tx-ty)*8`, `(tx+ty)*4`, avec `+24` en Y pour une
wall. L'insertion `0xD1460` alloue par le jeu, déduplique et équilibre l'arbre.
Au changement de layer, D2R sérialise ces arbres dans son format automap, les
vide et les recharge par le cycle natif; le teardown possède aussi toutes les
libérations.

Le spike est donc **PASS statique borné** pour publier les cellules MaxiMap de
chaque niveau dans son propre `Levels.Layer`, sur le thread UI natif et sans
`DrlgRoom`, `ActiveRoom`, collision, monstre ou objet actif. Le champ `layer` du
MSA1 actuel est toutefois un index de sheet DC6 libd2 et ne doit jamais être
passé comme `Levels.Layer`. Les town montages spéciaux restent délégués au jeu.

L'architecture retenue pour un éventuel prototype supprime entièrement le
terrain ImGui, conserve le helper/cache seed-driven et publie progressivement
chaque niveau vers son layer natif. Elle interdit de recopier tout l'acte dans
chaque layer courant : cela gonflerait les sidecars et détournerait leur modèle.
Les noms distants, waypoints et autres POI restent un gate séparé; ce spike ne
les déclare pas résolus. Les mesures d'insertion, de sidecar et de FPS Tab ouvert
restent `not run`.

### Correction 0.13.24 — propriétaire automap actif et traversées physiques

Le premier runtime du spike natif a affiché l'ensemble des noms et waypoints
avec des FPS jugés corrects, mais a ensuite bloqué un changement par waypoint
pendant plus de 40 secondes. Le log contenait 38 202 passages de replay et le
processus atteignait environ 17,65 Go. Le corpus natif gouverné prouve que
`AUTOMAP_GetOrCreateLayer 0xD5360`, lorsqu'il reçoit un layer différent du
propriétaire courant, emprunte le chemin `0xD1710` qui sérialise les quatre
arbres puis libère leur contenu. Cette preuve invalide le changement temporaire
de propriétaire; elle ne prétend pas, à elle seule, attribuer chaque octet de
mémoire du témoin à une allocation précise.

La correction interdit désormais tout appel à `AUTOMAP_GetOrCreateLayer` : son
RVA ne subsiste que comme témoin d'empreinte. Chaque pompe UI lit seulement
`AUTOMAP_CurrentLayerOwner` à `D2R+0x2A2CF68`, exige que son identifiant à
`+0x00` égale le `Levels.Layer` autoritaire du niveau réellement actif, puis
insère au maximum 2 048 cellules dans ses arbres floor/wall à `+0x08/+0x30`.
Un propriétaire nul ou différent attend de façon bornée puis échoue fermé;
aucun propriétaire étranger n'est créé, sélectionné, retenu ou restauré. La
publication et l'acceptation sont suivies par layer, et les autres layers ne
sont amorcés qu'après une transition réelle du joueur.

Le helper passe au protocole MS1 v2. Pour chaque paire extérieure dirigée, il
choisit exactement une ouverture de largeur joueur prouvée par les RoomLinks et
la collision générée dans son processus privé; il refuse les doublons au lieu
d'en moyenner les centres. Sur le seed témoin `1395822899`, Spider Forest vers
Flayer Jungle vaut `(5000, 4268)` et le sens inverse `(4999, 4268)`, donc deux
subtiles réciproques adjacents, contre l'ancien ancrage moyen déplacé. Les neuf
waypoints exacts de l'Acte III restent issus de la génération libd2.

Le source gouverné du helper réside sous `addons/RuffnecKkMapSense/mapgen/`,
épingle libd2 au commit
`ac4d735e57fcab6a3c356f810bb256da95a93716` et conserve le patch complet. Trois
builds Zig 0.16.0 donnent le même exécutable de 12 090 880 octets, SHA-256
`0A79F65827B1EB40819B90B30E2727625047D0CC451DD460224A1DFE98C7D58C`.
La matrice 4 seeds × 5 actes passe : nombres de waypoints `9/9/9/3/9`, une
traversée unique par paire dirigée, réciprocité à un sous-tile et sortie MS1
déterministe. Le build DLL Release `/W4 /WX` et CTest `1/1` passent; la DLL de
3 360 256 octets vaut SHA-256
`D9E620040A58DB87B8BE654F5714452ADECF52836010A3CD36A8E4DA6623F025`.

Le déploiement mod-local et le cold start 0.13.24 passent sur le runtime
officiel 3.3.93847, Build Key
`623f7a1f73eabb08ccb2b2046e3f9164` : hashes source/runtime identiques pour la
DLL et le helper, helper seed-scoped prêt, empreinte native acceptée, 36 plugins
et 17 patches actifs sans échec, puis initialisation `24/24`. Le rollback exact
des deux binaires précédents réside sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.24-active-owner-predeploy-20260901T133626/`.

Le gameplay 0.13.24 est **FAILED** : noms, waypoints, FPS et ancrage Flayer
Jungle ont d'abord passé, puis le waypoint Acte III vers Dry Hills a crashé le
jeu pendant la sérialisation du layer courant. Le prochain gate 0.13.25 doit
reprendre la même pile complète et les mêmes témoins, puis traverser plusieurs
actes dans les deux sens sans owner bloqué ni crash.

### Correction 0.13.25 — arbres floor/wall et limite du sidecar

Le crash report runtime du waypoint Acte III vers Dry Hills place l'accès
invalide à `D2R+0x12D399E`. Le dernier layer complété comptait exactement
`19202/7556/11646` cellules tentées/insérées/dédupliquées. Dans
`AUTOMAP_SerializeCellTree 0xD7CE0`, chaque nœud devient trois `uint16`, soit
six octets, et la longueur traverse un intermédiaire signé de 16 bits. Le
registre `RBP=0xFFFFB118` ferme le diagnostic :
`0xB118=45336=7556*6`. La taille sign-étendue a ensuite atteint la copie
optimisée; la commutation a donc échoué avant que l'owner Acte II remplace
l'owner Acte III, ce qui explique simultanément le crash et l'automap restée
sur le mauvais acte.

La cause productrice était distincte du changement d'acte. `AutomapCell.wall`
servait à la fois à choisir l'arbre `owner+0x08/+0x30` et à appliquer `+24` sur
Y. Or le premier choix dépend du tableau natif floor/wall, tandis que le second
dépend seulement de `orientation>=0x10`. Le helper possédait déjà la preuve
`PlacedTile.pass`, mais la jetait. MSA1 passe à la version 2 sans grossir ses
records de 16 octets : byte 12=`wallTree`, byte 13=`raised`, bytes 14–15
réservés. Les DS1 conservent directement la provenance de leurs tableaux;
les outdoors utilisent `pass!=0`; les montages de town et objets restent dans
l'arbre floor sans élévation.

La DLL 0.13.25 choisissait l'arbre par `wallTree`, construisait la coordonnée par
`raised`, puis utilisait `tree+0x20` comme si le total des nœuds était le total
émis par le sérialiseur. Le gameplay a ensuite prouvé que ce garde-fou était
trop grossier : les arbres natifs persistants pouvaient déjà dépasser 5 461
nœuds tout en demeurant structurellement valides, ce qui bloquait la
complétion terrain et, par couplage erroné, tous les noms.

La matrice helper 4 seeds × 5 actes génère deux fois chaque MSA1, exige un hash
identique et valide chaque record. Tous les actes contiennent des cellules dans
les deux arbres et au moins une wall non élevée, ce qui aurait immédiatement
rejeté l'ancien amalgame. Pour le seed témoin `1395822899`, l'Acte III produit
30 276 cellules brutes réparties `13 682 floor / 16 594 wall`; leur insertion
reste soumise à la déduplication native et au garde-fou dynamique. Le prochain
gate runtime doit mesurer les comptes finaux des deux arbres, exiger zéro refus,
passer Acte III → Dry Hills puis plusieurs actes dans les deux sens, et confirmer
que noms, waypoints, FPS et automap actif suivent chaque transition.

### Correction 0.13.26 — catalogue de coordonnées indépendant

`AUTOMAP_SerializeCellTree 0xD7CE0` ignore les nœuds dont le premier octet de
clé à `node+0x20` est non nul. Son plafond de 5 461 concerne donc les records
tag-zéro réellement émis, pas le compteur total `tree+0x20`.
`AUTOMAP_FindCellInsertionPoint 0xD4B70` possède l'ABI exacte
`(tree, FindResult*, key) -> FindResult*`; son résultat de 16 octets contient
`{node, insertionSlot}`. Un slot nul prouve que la clé existe déjà et permet à
MapSense d'avancer sans allocation ni budget supplémentaire. Pour une clé
absente seulement, la DLL parcourt l'arbre de façon bornée, compte les records
tag-zéro et refuse une mutation qui rendrait le sidecar non sérialisable. Les
compteurs total et émis sont exposés séparément.

La readiness des labels est maintenant distincte de celle du terrain. Dès que
le snapshot exact seed/difficulté/acte est accepté, son catalogue immuable de
coordonnées rend les noms de niveaux et waypoints éligibles sur le layer natif
autoritaire courant; un terrain encore en insertion ou refusé fail-closed ne
supprime plus les textes. L'Acte I génère les niveaux 1 à 39, Cow Level inclus,
et le cache géométrique passe à la révision 2 tandis que l'intent Reveal Map
reste en révision 1. Les tests de politique et la matrice 4 seeds × 5 actes
passent; le gate runtime doit encore confirmer Acte III → II → I → III, les
labels, waypoints, FPS et l'absence de crash ou de chargement bloqué.

### Corrections 0.13.27–0.13.31 — cellules non sérialisées et matrice Acts I–V

Le corpus natif a finalement fermé l'ambiguïté du sidecar : le loader natif
conserve le premier octet de la clé automap comme tag, tandis que
`AUTOMAP_SerializeCellTree 0xD7CE0` n'émet que les cellules tag-zéro. Le candidat
0.13.30 publie donc les cellules synthétiques de l'atlas avec le tag `1`. Elles
restent dans les arbres floor/wall natifs utilisés par le rendu courant, mais
ne gonflent pas le sidecar borné de D2R lors d'un changement de layer. Cette
route remplace les scans complets et les budgets qui bloquaient ou ralentissaient
les transitions.

Le premier témoin 0.13.30 a validé les Actes II et III, puis a isolé un rejet de
l'Acte V avant toute publication : le helper fournissait 28 labels, niveaux
109–136, alors que le MSA1 de campagne standard contient les 24 géométries
109–132. Les niveaux 133–136 sont les branches Pandemonium/uber conditionnelles,
pas des surfaces de campagne manquantes. Le candidat 0.13.31 exige désormais la
couverture géométrique seulement pour les plages de campagne déclarées de chaque
acte; un label de branche optionnelle absent ne rejette plus tout l'acte. Les
niveaux BKVince custom 137–146, dont les Rifts 138–146, restent explicitement
hors de ce gate et ne sont pas revendiqués par le helper libd2 actuel.

Le build Release x64 strict et CTest passent `1/1`. La DLL source/runtime
byte-identique mesure 3 364 864 octets, version `0.13.31-candidate`, SHA-256
`E6A1DD3774DFC80B4EBA111324A823835B85666F1F897CD4AEC02F98AD2F1FA6`.
Le helper runtime inchangé vaut SHA-256
`F350F001FA5A31AB8D0ABCC08EE3AE3DE8CF8D032147480CC5449F603442A3A4`.
Le rollback exact pré-déploiement réside sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.31-predeploy-20260901-224037/`.

Le cold start mod-local du 1er septembre 2026 sur D2R 3.3.93847 avec la pile
complète active est **PASS**. Dans une même partie Insanity et le même seed
`1231900215`, la matrice waypoint Acte V → I → II → III → IV → V publie :

- `external labels: PASS` avec niveaux/waypoints `39/9`, `35/9`, `28/9`,
  `6/3`, `28/9` pour les actes 0 à 4;
- `native atlas layer: COMPLETE` sur les layers `0`, `27`, `57`, `77`, `79`;
- aucun `ERROR`, assert, crash, chargement bloqué ni rejet
  `missing-from-geometry` dans les logs frais.

Le témoin visuel confirme dans chaque town le layer natif unique, les noms et
waypoints, ainsi que la surface distante adjacente : Blood Moor, Rocky Waste,
Spider Forest, Outer Steppes et Bloody Foothills. Frigid Highlands confirme en
plus l'atlas Acte V hors town. Aucun second terrain ImGui, flash ou automap
restée sur l'acte précédent n'est observé. Le runtime est laissé ouvert à
Harrogath pour la validation humaine prolongée; les mesures FPS instrumentées,
les seeds/difficultés supplémentaires, le multijoueur et le support des niveaux
custom restent des gates séparés.

### Décision du 2 septembre 2026 — atlas alimenté par le mod actif

Vincent a donné `GO` à l’implantation d’un helper libd2 **mod-aware** après la
revue d’architecture `plugin-architect`. Le contrat autonome hybride de
`RuffnecKkMapSense.dll` dans la RuffnecKk D2RLoader Suite demeure inchangé :
aucune nouvelle DLL, aucun nouveau hook natif et aucun exécutable à lancer
manuellement par l’utilisateur. MapSense continue de démarrer son helper privé,
masqué et hors des threads gameplay/rendu.

Le helper doit désormais construire son contexte à partir des tables et assets
du mod actif que la DLL a résolus et bornés, puis utiliser les ressources
vanilla embarquées uniquement comme fallback. La découverte est gouvernée par
les données, pas par BKVince : aucun ID, nom ou chemin propre à `Rift`, à
BKVince ou à un autre mod ne peut servir d’exception d’implantation. Les IDs de
niveaux et presets lus dans les tables deviennent des valeurs ouvertes afin que
des lignes custom arbitraires puissent traverser toute la génération sans trap
d’enum fermée.

La position d’un nom distant est celle de son **entrée dans le niveau source**.
Ainsi, un lien custom `109 -> 138` doit être publié comme un record `MS1 E`
ancré dans la géométrie de 109; les coordonnées internes ou le centre de 138 ne
doivent jamais être projetés sur le layer de 109. Les placements de secours
par centre de niveau ajoutés au candidat 0.13.32 sont rejetés : ils peuvent
prouver la découverte d’une ligne, mais pas sa position automap exacte.

Avant tout nouveau déploiement runtime, un gate déterministe hors jeu doit :

- conserver byte-for-byte les sorties/goldens vanilla pertinentes;
- charger les tables et le DS1 actifs de BKVince et émettre une entrée exacte
  `MS1 E 109 138 ...` sans constante spéciale pour 138;
- réussir un fixture synthétique utilisant un autre ID et un autre nom custom;
- prouver que le cache inclut l’empreinte des données du mod et ne peut donc
  pas réutiliser l’atlas d’un autre jeu de données.

Le support générique des labels et waypoints issus de tables/DS1 custom est le
gate courant. La reconstruction visuelle complète de terrains custom utilisant
des DT1 absents du corpus embarqué, les mods distribués uniquement sous une
forme que MapSense ne peut pas lire et les graphes custom mal formés restent des
gates distincts; aucune couverture universelle n’est revendiquée avant leurs
preuves. Les monstres, objets live et lignes Direct existants ne changent pas.

### Implantation 0.13.33 — preuves hors runtime

Le helper charge maintenant les sept tables actives nécessaires (`Levels`,
`LvlPrest`, `LvlTypes`, `LvlMaze`, `LvlSub`, `LvlWarp`, `Objects`) et résout les
DS1 depuis les racines actives ordonnées avant son fallback vanilla embarqué.
Les enums libd2 de niveaux et presets sont ouvertes. La DLL transmet les
racines actives au processus privé et namespace le cache de géométrie revision
3 par une empreinte de tables/assets; l’augmentation statique 0.13.32 et son
centre inventé ont été supprimés.

Les gates déterministes du 2 septembre 2026 passent : quatre seeds sur les
cinq actes vanilla conservent leurs sorties MS1/MSA1 v2; BKVince émet exactement
`MS1 E 109 138 5020 5100 0` et neuf waypoints pour l’acte V; un fixture
temporaire remappé vers le LevelId arbitraire `733` émet exactement
`MS1 E 109 733 5020 5100 0`. Le test C++ couvre aussi le namespace du cache et
un target de warp arbitraire. Le patch libd2 complet s’applique en reverse sans
erreur sur le commit épinglé `ac4d735e57fcab6a3c356f810bb256da95a93716`.

Le build Release x64 `/W4 /WX` et CTest `1/1` passent. La DLL candidate mesure
3 396 096 octets, porte PE/PluginInfo 0.13.33-candidate, expose les quatre
exports attendus et vaut SHA-256
`D2CDE23036243C3D5187A0F58DCD5E9349DCB558C6F2EA5E2C6F74412FE8C29B`.
Le helper ReleaseSafe mesure 12 121 088 octets et vaut SHA-256
`2291DA9B1A529EFF5BF676D2E31EB01FAF6172B5B19CBD1884A63352E56BEF6D`;
les copies source, package et build sont byte-identiques. Aucun déploiement ni
PASS visuel/runtime n’est attribué à ce hash avant le prochain gate complet.

### Correction 0.13.34 — outdoor fidèle et témoin natif post-rendu

Le témoin humain 0.13.33 a invalidé la complétude outdoor : Spider Forest et
Great Marsh pouvaient perdre leurs waypoints avant visite, tandis que l'Acte V
ne publiait rien. La revue `plugin-architect` a isolé une cause concrète dans
le helper : son chemin automap sans collision chargeait tous les DT1 du
`LevelType`, ignorait `pRoomEx.nDT1Mask`, sautait les trois passes `LvlSub`
waypoint/shrine/terrain et ne reproduisait pas la propriété des coutures entre
pièces. Le runtime pouvait ensuite déclarer la couche complète sur le seul
succès des insertions, sans témoin d'un passage de rendu natif.

La 0.13.34 remplace ce raccourci par une matérialisation outdoor fidèle qui
s'arrête avant toute allocation ou rasterisation de collision. Elle conserve
l'ordre exact des seeds, les masques DT1 de chaque pièce, les trois passes
`LvlSub`, les arbres floor/wall et la première pièce propriétaire d'une
couture. Les remplacements de Blank décidés par une pièce ultérieure sont
répercutés après le parcours complet du niveau. Aucun `ActiveRoom`, monstre,
objet gameplay ou table live D2R n'est créé par ce chemin. Le cache géométrique
passe à la révision 4; l'intention Reveal Map seed/difficulté reste en révision
1.

Après la dernière insertion, la DLL entre désormais dans l'état borné
`AwaitingWitness`. Le prochain vrai passage local-player du hook automap, après
l'appel original D2R, doit retrouver jusqu'à huit clés exactes dans chacun des
arbres floor et wall du même propriétaire/layer natif. Trois échecs consécutifs
font échouer proprement la couche; elle n'est jamais créditée ni persistée
`COMPLETE` avant ce témoin.

Les gates hors jeu passent le 2 septembre 2026 : Zig 0.16.0 ReleaseSafe,
Release x64 `/W4 /WX`, CTest `1/1`, matrice déterministe MS1/MSA1 v2 de quatre
seeds × cinq actes, neuf waypoints par acte, entrée BKVince réelle `109 -> 138`
et remap indépendant vers le faux LevelId `733`. La génération MSA1 BKVince
Acte V seed 1337 produit 24 niveaux et 34 586 cellules en environ 520 ms. Le
patch libd2 régénéré s'applique en sens inverse sans erreur sur le commit épinglé
`ac4d735e57fcab6a3c356f810bb256da95a93716`.

La DLL 0.13.34-candidate mesure 3 399 168 octets, SHA-256
`C6EDC2C577D9687E27D0D3AD2886B5A76FBB7F42C4B1D6221DFBE7E8506D3E40`.
Le helper mesure 12 129 792 octets, SHA-256
`4A14B843651F0D28AB74EE9DCC73DABAA23DAE44E7AC88FD7720FBFA7DEB3D39`.
Les copies build/package/runtime sont byte-identiques. Le rollback 0.13.33
pré-déploiement réside sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.34-predeploy-20260902-0914/`.

Le cold start mod-local sur D2R officiel 3.3.93847 avec la pile complète est
PASS : MapSense 0.13.34 charge, le helper seed-scoped est prêt, 38 plugins et
17 patches restent actifs et le frontend atteint `24/24`. Le verdict visuel
Actes III/V, les transitions inter-actes, les FPS et l'absence de freeze restent
un gate humain ouvert sur ce hash exact.

### Décision du 2 septembre 2026 — baseline externe immuable et preuves natives positives

Vincent autorise par `GO` la correction architecturale du catalogue de labels
dans la DLL autonome hybride `RuffnecKkMapSense.dll`, membre indépendante de la
RuffnecKk D2RLoader Suite. Aucune nouvelle DLL, configuration, RVA, signature,
ABI ou propriété de hook n'est ajoutée. Les monstres live, coffres spéciaux,
shrines, racks, lignes de navigation, renderer ImGui/D3D12, hotkeys et cellules
automap natives restent hors du lot.

Le diagnostic ferme deux défauts distincts. Premièrement, le helper externe
remplace actuellement le catalogue complet, puis les captures natives
remplacent le même propriétaire et peuvent publier une absence sur un niveau
encore non matérialisé; l'ordre d'arrivée décide alors si un waypoint distant
reste visible. Deuxièmement, les waypoints outdoor sans preset déjà exposé sont
encore localisés par un replay LvlSub abrégé, séparé de la matérialisation
outdoor fidèle qui produit les cellules automap.

Le contrat approuvé sépare une baseline externe complète, atomiquement liée au
couple session/seed/difficulté/données, de corrections natives strictement
positives. Une observation native absente ou incomplète ne peut jamais effacer
la baseline. Au rendu, une preuve native positive remplace la définition
externe du même waypoint; sans preuve positive, la baseline demeure. Les
sorties et ancres de niveaux appliquent la même séparation sans modifier la
politique de navigation. La génération helper doit en outre capturer les
waypoints pendant la même passe outdoor fidèle que la géométrie, et distinguer
les transitions de warp/preset des coutures wilderness fondées sur la
collision.

Les gates pré-runtime exigent des régressions positionnelles, et non seulement
des comptes : Stony Field, Dark Wood, Spider Forest, Great Marsh et Arreat
Plateau avant visite; Monastery Gate dans les deux directions; portails
permanents Frigid Highlands/Abaddon, Arreat Plateau/Pit of Acheron et Frozen
Tundra/Infernal Pit; conservation après capture native vide puis positive;
matrice quatre seeds, cinq actes, trois difficultés; build Release `/W4 /WX`
et CTest. Aucun déploiement ni verdict gameplay n'est attribué au prochain
candidat avant ces preuves.

### Pré-gate 0.13.35 — acquis figés, red portals et propriétaires séparés

Le lot implanté conserve sans modification le renderer automap/ImGui, les
monstres live, les coffres Joffreybesos, shrines/racks, hotkeys, cellules
natives et lignes de navigation verte/rouge/mauve. Le helper ne crée toujours
aucune `ActiveRoom`, unité, monstre ou missile dans D2R.

Les catalogues waypoint et niveau possèdent maintenant une baseline externe
immuable et une couche native positive. Une publication native vide devient un
no-op; une preuve positive remplace seulement son propriétaire exact. Les
sorties appliquent la même règle au couple dirigé source/destination, ce qui
préserve un red portal externe même lorsqu'une capture native complète connaît
les sorties ordinaires du même niveau.

MS1 v3 encode `P source target x y class`. Tristram est ancré sur Stone Alpha
du preset Cairn Stones généré avec l'offset `+4,+4` prouvé par
`ACT1Q4_OpenPortalToTristram`. Abaddon, Pit of Acheron et Infernal Pit sont
ancrés sur les tuiles DT1 style 29 qui créent l'objet permanent 60. Le portail
Cow, les town portals et les portails joueur restent exclus parce que leur
position n'est pas déterminée par le seed. La façade Tamoe/Monastery utilise
la tuile double porte générée, et non une autre ouverture valide de la longue
frontière de collision.

Les preuves pré-runtime passent : CTest `1/1`; tests unitaires libd2; quatre
seeds × cinq actes générés deux fois avec labels et géométrie déterministes;
9/9/9/3/9 waypoints; un portail Tristram en Acte I; trois portails Acte V;
régressions exactes Monastery et Spider/Flayer; BKVince `109 -> 138`; fixture
custom indépendante `109 -> 733`. Le déploiement runtime et le verdict visuel
restent le prochain gate.

### Gate visuel 0.13.35 — mort et écrans de chargement

Vincent exige que les features MapSense cessent de linger derrière la mort du
joueur et disparaissent dès l'arrivée d'un écran de chargement, notamment lors
d'un waypoint. Le correctif autorisé conserve intégralement la baseline atlas,
les catalogues, la persistance Reveal Map, les caches POI/monstres/missiles,
les coffres Joffreybesos et les lignes verte/rouge/mauve. Il ne détruit, ne
recalcule et ne remplace aucune de ces données.

Le renderer applique maintenant un unique prédicat fail-closed avant toute
soumission de pixels MapSense : `GameplayReady`, états natifs `UI_GAME` et
`UI_AUTOMAP`, puis témoin vivant publié par la passe automap du joueur local.
Cette passe utilisait déjà `GetUnitMode` à la RVA gouvernée `0x34AB60`; les
modes joueur `DEATH=0` et `DEAD=17` sont corroborés par l'entrée gouvernée
`SUNIT_IsDead` et D2MOO. Une mort révoque le témoin sur la passe native; un
écran de chargement, la fermeture de Tab ou un échec de lecture UI le révoque
avant le rendu. Seule une nouvelle passe du joueur vivant peut le réarmer.
Aucun pointeur D2R n'est conservé et aucune nouvelle surface native n'est
introduite.

Les tests de politique couvrent chaque combinaison jeu/automap/vivant, les
deux modes de mort et les modes vivants adjacents. Le build Release x64 strict
`/W4 /WX` et CTest `1/1` passent. Le binaire pré-runtime mesure 3 491 840 octets
et vaut SHA-256
`8B0F5E98E0727758CB33BBDDDB011EFEBCCFB2EF8E827A681529DEF80FFF44EF`.
Le helper réellement empaqueté reste inchangé, mesure 14 267 904 octets et vaut
SHA-256
`F7FC445E3C8E8CB0BD2F948EA8F2F6BC128AF5260E30479095338420D6F4F923`.
Sa matrice fraîche passe encore quatre seeds × cinq actes, les comptes
waypoints `9/9/9/3/9`, les portails `1/0/0/0/3`, l'entrée BKVince `109 -> 138`
et la fixture arbitraire `109 -> 733`. Le comportement visuel exact à la mort
et pendant les transitions demeure un gate runtime humain, pas une conclusion
tirée des tests statiques.

Le cold start mod-local autorisé du 2 septembre 2026 à 15:00 sur Battle.net
D2R 3.3.93847 (Build Key `623f7a1f73eabb08ccb2b2046e3f9164`, SHA-256 D2R.exe
`E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`)
charge exactement la DLL et le helper documentés ci-dessus. MapSense journalise
le build-name comme diagnostic seulement, accepte son empreinte complète,
installe les hooks D3D12 fail-closed, trouve Exocet et les textures de coffres,
puis initialise son hôte ImGui et son helper seed-scoped. La pile complète
atteint `D2R startup complete` avec 38 plugins chargés et 17 memory patches,
sans nouvel échec, rejet, assert ni crash. L'avertissement sur l'ancien
`automap sprite atlas` est attendu : le double underlay ImGui est retiré et la
géométrie générée appartient maintenant exclusivement aux arbres de cellules
natifs. L'instance demeure ouverte pour le verdict visuel humain; mort,
waypoint/inter-acte, réapparition, noms/waypoints/red portals et FPS restent
`not run` tant que Vincent ne les a pas observés.

### Gate humain 0.13.35 et pré-gate 0.13.36 — niveaux custom sans spécialisation de mod

Le 2 septembre 2026, Vincent confirme en jeu que le candidat 0.13.35 affiche
les noms et waypoints distants attendus dans les cinq actes : les Actes I et
III sont explicitement déclarés complets, puis le balayage de tous les actes
ne laisse plus aucun élément standard invisible. Les FPS demeurent corrects.
Cette preuve ferme le défaut général d'atlas/waypoints; elle ne transforme pas
les gates mort, chargement, portails permanents ou multijoueur en succès
implicites.

Le niveau custom utilisé comme stress test reste absent. Le diagnostic prouve
que l'atlas n'est pas en cause : le helper 0.13.35 émet bien la transition
`109 -> 138` à son ancre Harrogath. Le catalogue journalise exactement neuf
clés de niveaux non résolues; elles correspondent exactement aux neuf noms
custom dont la valeur anglaise est byte-identique à la clé de localisation.
Le garde-fou contre l'écho prématuré de `LocalizationServiceV1` les marquait
donc à tort comme non localisés, puis le renderer les refusait. Le niveau
custom réel n'est qu'un témoin : le contrat produit vise tout niveau et tout
mod inconnus à l'avance.

Vincent autorise par `GO` un correctif strictement générique, sans nom de mod,
nom de niveau ou LevelId en dur. Une traduction exacte non résolue n'est
reconsidérée qu'après qu'une autre traduction différente a prouvé que les
tables de langue sont prêtes; MapSense redemande alors la clé au service. Une
entrée réelle peut réussir avec une valeur identique, tandis qu'une clé
absente reste `NotFound`. L'écho global observé avant initialisation reste
refusé. Cette seconde passe s'exécute une fois lors de la construction du
catalogue, jamais dans le renderer ou par frame; l'atlas, ses catalogues, les
POI, la navigation et les collectors live sont inchangés.

La pré-qualification 0.13.36 passe le build Release x64 strict `/W4 /WX` et
CTest `1/1`. La régression utilise un niveau arbitraire `733` dont la valeur
localisée est identique à sa clé et le place avant la traduction qui certifie
le service, ce qui prouve l'indépendance à l'ordre et à BKVince. Le test
d'écho prématuré existant et le refus des clés réellement absentes passent
toujours. La DLL build/package mesure 3 495 424 octets et vaut SHA-256
`2C42B0361CAA02111C2531600E4A612B2471DD8DE0D7F995B7EF1C618CA8CEE6`;
les deux copies sont byte-identiques. Le déploiement, le cold start et le
verdict visuel d'un niveau custom demeurent `not run` jusqu'à l'autorisation
runtime distincte.

### Décision du 2 septembre 2026 — transaction atlas et watchdogs spécialisés

Le premier gate runtime 0.13.36 invalide la publication de l'atlas, sans
invalider son architecture native : le statut frais montre quatre expirations,
quatre échecs géométriques, quatre réponses périmées et aucun snapshot, tandis
que les 146 noms actifs sont tous localisés. Des mesures read-only du helper
empaqueté avec les racines BKVince actives bornent les labels à 352–669 ms,
mais la géométrie à 3 087–5 873 ms selon l'acte; plusieurs seeds Acte V
dépassent à eux seuls le watchdog universel de 5 000 ms. Ce délai est donc
faux par construction, et non le symptôme d'un helper bloqué.

Vincent autorise par `GO` la réparation transactionnelle de la DLL autonome
hybride MapSense, membre indépendante de la RuffnecKk D2RLoader Suite. Le
helper libd2, le format MSA1, les hooks, RVA, signatures, ABI, cellules natives,
catalogues de POI, monstres live, coffres, shrines, lignes Direct et rendu sous
Tab demeurent inchangés. Une requête exacte reste propriétaire de toute la
transaction labels → géométrie courante → snapshot → publication; une
répétition exacte ne change plus son numéro de série. Les labels conservent un
watchdog de 5 s et la géométrie reçoit un watchdog distinct, borné à 30 s,
asynchrone et annulable. Un changement réel de session, seed, difficulté, acte
ou niveau annule le child privé devenu inutile. Le préchauffage reste
séquentiel, passe en priorité idle, ne publie aucun état visible et cède
immédiatement à une requête gameplay.

Une géométrie absente, invalide ou expirée ne peut plus produire un PASS atlas,
un callback positif ni une requête publiée. Les échecs exacts restent latchés
pour empêcher toute tempête de retry dans la session. La télémétrie distingue
désormais les expirations labels/géométrie, les annulations primary/prewarm,
les misses/invalidations de cache et l'opération active. Le gate unitaire doit
prouver déduplication, conservation du serial pendant une géométrie lente,
annulation sur nouvelle identité, préemption du prewarm, sélection des délais
et refus de publication après échec. Le gate helper est étendu aux racines
actives afin de chronométrer réellement la géométrie modée; aucun déploiement
ne précède ces preuves.

La pré-qualification 0.13.37 ferme ces gates hors runtime. Le build Release
x64 strict `/W4 /WX` et CTest `1/1` passent après extraction de la politique
pure dans le header du provider. La matrice helper conserve deux sorties
byte-déterministes pour quatre seeds × cinq actes vanilla, puis génère et
valide 20/20 géométries supplémentaires avec les véritables racines Excel et
tiles BKVince. Le maximum modé observé est 5 670 ms en Acte V, très inférieur
au watchdog géométrique de 30 s; l'entrée réelle `109 -> 138` et la fixture
indépendante `109 -> 733` passent toujours. Le helper est inchangé, mesure
14 267 904 octets et vaut SHA-256
`F7FC445E3C8E8CB0BD2F948EA8F2F6BC128AF5260E30479095338420D6F4F923`.

La DLL build/package 0.13.37-candidate mesure 3 500 544 octets, expose les
trois exports D2RLoader attendus et vaut SHA-256
`D8F9CD16E04385DFB97DBE288D39D11F140D095E7BA5DE452BE40B2FE016F33A`;
les copies build et package sont byte-identiques. La 0.13.36 pré-emballage est
conservée sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.37-prepackage-20260902-1708/`
avec son SHA-256 exact
`2C42B0361CAA02111C2531600E4A612B2471DD8DE0D7F995B7EF1C618CA8CEE6`.
Vincent a ensuite autorisé la séquence runtime bornée. La DLL 0.13.37 est
déployée seule dans la portée mod-locale BKVince et sa copie runtime est
byte-identique au package : SHA-256
`D8F9CD16E04385DFB97DBE288D39D11F140D095E7BA5DE452BE40B2FE016F33A`.
Le helper n'a pas été recopié et conserve son hash exact
`F7FC445E3C8E8CB0BD2F948EA8F2F6BC128AF5260E30479095338420D6F4F923`.
La 0.13.36 remplacée est sauvegardée sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.37-predeploy-20260902-212524/`
avec son SHA-256
`2C42B0361CAA02111C2531600E4A612B2471DD8DE0D7F995B7EF1C618CA8CEE6`.

Le cold start frais utilise `D2RLoader.exe -mod BKVince -txt -offline` sur
Battle.net D2R 3.3.93847, Build Key
`623f7a1f73eabb08ccb2b2046e3f9164`. MapSense 0.13.37-candidate accepte son
empreinte native complète, installe ses hooks D3D12 fail-closed, prépare le
helper seed-scoped et initialise son hôte ImGui. La pile complète atteint
`38 plugins loaded; 17 memory patches applied`, inclut les cinq DLL eezstreet,
puis termine `24/24` avec `D2R startup complete`; aucun plugin MapSense n'est
rejeté ni échoué et le processus jeu demeure répondant. Ce résultat ferme le
gate de déploiement et de cold start technique. Reveal Map, la publication de
la géométrie distante, les transitions d'acte, la fluidité et les niveaux
custom restent `not run` jusqu'au témoin gameplay humain de ce même processus.

Le témoin gameplay humain de ce même processus ferme ensuite le gate custom :
Vincent rejoint l'Acte V en premier, ouvre l'automap et confirme que le niveau
réel `Rift Level 1` apparaît visuellement au bon endroit. La télémétrie
concordante publie l'Acte V sans erreur ni expiration : 38 niveaux visibles,
58 sorties, 9 waypoints et 30 904 cellules insérées. Ce résultat prouve le
contrat générique recherché sur un niveau custom inconnu du plugin à l'avance;
il ne constitue ni un cas spécial BKVince/Rift ni une allowlist de LevelId.
Les transitions inter-actes, la fluidité et la matrice complète des labels et
waypoints restent à valider séparément.

Vincent poursuit immédiatement la matrice dans les cinq actes et prononce le
verdict gameplay complet : tout est visible, rien n'est laissé pour compte et
tous les éléments sont correctement placés partout. Les logs frais concordent
avec ce parcours et ses retours entre actes. Ils publient sans erreur l'Acte I
avec 39 niveaux/73 sorties/9 waypoints, l'Acte II avec 35/72/9, l'Acte III avec
28/60/9, l'Acte IV avec 6/10/3 et l'Acte V avec 38/58/9. Chaque acte atteint
`native atlas layer: COMPLETE`; aucune expiration, annulation, réponse
périmée ni erreur n'apparaît pendant la matrice, et le processus reste
répondant. Les gates couverture complète, positionnement, niveaux custom et
transitions inter-actes sont donc **PASS gameplay** pour ce seed à l'index de
difficulté 2.

Avec Reveal Map actif et l'Acte III affiché, Vincent observe 183 FPS au repos.
Ce témoin humain n'est pas un benchmark A/B automatisé, mais il ne reproduit
plus la chute antérieure d'environ 130–140 FPS sans Reveal vers 80–100 FPS avec
Reveal. La régression de performance visible des prototypes qui chargeaient
des pièces runtime est donc **non reproduite et considérée corrigée sur ce
run**; une qualification de release pourra encore consigner des mesures
contrôlées sur plusieurs scènes si nécessaire.

### Décision du 2 septembre 2026 — source missile client + serveur

Le témoin gameplay suivant la réussite complète de Reveal Map révèle un défaut
distinct du seed atlas : les missiles cessent d'apparaître sur l'automap dans
l'Acte IV, tandis que de grands cercles et X transitoires peuvent apparaître
dans la même scène. Le statut frais de MapSense confirme que le collecteur
0.13.37 est encore `client-only=true` et publie `current=0` dans River of
Flame malgré une pipeline mécaniquement active. L'atlas, les labels, waypoints,
niveaux custom et lignes Direct restent explicitement hors de ce correctif.

Une lecture live strictement read-only des deux tables de type 3 ferme la cause
du premier symptôme. Au même instant, la table client à
`D2R+0x2A24510` contient zéro missile, tandis que la table serveur à
`D2R+0x2A25D10` contient 11 puis 15 unités type 3 valides, sans fault, cycle ou
type mismatch. Leurs ids, classes, chemins dynamiques et coordonnées monde sont
cohérents. Les classes 144/145/323/325/326 se résolvent dans le `Missiles.txt`
actif en projectiles feu, poison et froid; elles ne sont donc pas du décor
arbitraire.

La preuve statique gouvernée établit la seconde source sans transposition
d'adresse externe. `SERVER_GetUnitByIdAndType 0x9A5A0` possède une signature
stricte de 28 octets unique et résout la base `0x2A25110`; son masque `0x7F`,
son stride `0x400` et son tail-jump vers le même walker `0x9F270` sont
identiques au resolver client adjacent `0x9A5D0`. L'écart exact `0x1800`
correspond aux six tableaux de types client précédant les tableaux serveur.
Le callsite natif `0x3B7D0E` sélectionne ce resolver pour une unité type 3
lorsque le témoin serveur `0x34F8D0` est vrai, sinon il appelle le resolver
client. La référence sémantique épinglée PrimeMH au commit
`92b6a97d8e56346f8b63a88bb647c1af044d2c8b` confirme indépendamment que son
collector concatène `missile_ptrs` et `server_missile_ptrs`.

Vincent autorise par `GO` MapSense 0.13.38 : la passe locale existante scanne
d'abord les 128 buckets client puis les 128 buckets serveur. Chaque source garde
ses limites de 32 768 unités et 8 192 unités par bucket, son cycle guard et sa
frontière SEH. Un set d'identités fixe, allocation-free et remis à zéro
uniquement sur ses slots touchés déduplique par `unitId`, conserve `classId`
comme témoin de cohérence et préfère toujours la copie client. Une copie serveur
exacte est supprimée; un même id avec une classe différente est refusé et
compté. Aucun `Unit*`, chemin ou contexte automap n'est retenu après la passe.
Les diagnostics séparent maintenant scans, buckets, observations, publications
et courant client/serveur, ainsi que doublons, préférences client et conflits.

La pré-qualification statique passe deux builds Release `/W4 /WX` byte-exacts,
CTest `1/1`, le parse JSON du registre natif et `git diff --check`. La DLL build
et package mesure 3 502 592 octets, porte PE/PluginInfo
`0.13.38-candidate` et vaut SHA-256
`B1896B4CB2789A897319607958F783C1084325A4E6EF154B45FBA74AD9B38329`.
La copie package 0.13.37 remplacée est conservée sous
`analysis-cache/mapsense-0.13.38-prequalification/package-before-0.13.38/`.
La DLL runtime demeure volontairement inchangée : le déploiement, le cold start,
la présence des missiles Acte IV, l'absence de doublons et le coût CPU/FPS sont
`not run` jusqu'à une autorisation runtime distincte.

Le grand cercle et le X restent un gate d'attribution séparé. Au premier
checkpoint 0.13.38, le renderer MapSense ne produisait pour un missile qu'une
petite ellipse neutre, la
configuration runtime impose des points pour tous les rangs de monstres et la
navigation ne dessine que des lignes. D2RCore expose toutefois les témoins
`s_drawMissileServerPositions`, `s_drawMissileServerRadius` et
`s_drawMissileServerLink`, exactement compatibles avec les formes observées.
Une matrice A/B fraîche MapSense actif/pausé avec état debug contrôlé doit encore
prouver le propriétaire des pixels; aucune touche aléatoire ni suppression
visuelle par symptôme n'est autorisée.

### Lot intégré 0.13.38 — missiles, identité boss et panneau

Vincent demande de fermer en un seul lot les régressions restantes sans lancer
de gate gameplay pendant son absence. Le seed-atlas, Reveal Map, les labels,
les waypoints, les niveaux custom, les transitions et leurs caches sont gelés :
ce lot ne modifie aucun de leurs contrats après le verdict gameplay complet de
0.13.37.

Le collecteur missile client + serveur qualifié ci-dessus alimente maintenant
le même modèle visible que PrimeMH au commit épinglé
`92b6a97d8e56346f8b63a88bb647c1af044d2c8b` : six familles Fire, Cold/Ice,
Lightning, Poison, Physical et Magic, tandis que les familles SFX, trigger et
dummy restent invisibles. Une comparaison automatisée des 736 classes connues
donne 736 correspondances et zéro divergence avec le `get_missile_type` de
PrimeMH. Les six tailles et couleurs par défaut sont également identiques et
deviennent configurables dans le panneau ImGui et dans le schéma TOML 15.

La compatibilité mod ne dépend pas de BKVince. Le catalogue lit le
`Missiles.txt` du mod actif : une classe stock conserve la taxonomie PrimeMH
tant que son `EType` stock n'a pas changé, un `EType` actif reconnu prend le
dessus, et une classe ajoutée est classée par son `EType` puis, uniquement pour
un projectile à dégâts physiques positifs, comme Physical. Les effets inconnus
restent invisibles. Le `Missiles.txt` BKVince courant contient 763 lignes et
toutes les colonnes numériques requises passent cette validation. Une source
active invalide ou binaire sans TXT échoue fermée et ne retombe pas sur une
classe vanilla portant le même index.

Les faux noms `Rancid Defiler` provenaient du flag comportemental
`MonStats.boss`, que les cinq familles `putriddefiler` de BKVince portent sans
être des boss nommés. Ce champ est retiré du catalogue MapSense. Un nom de boss
exige désormais l'identité live Super Unique (rank ou index) ou le contrat
`primeevil`; le marqueur ordinaire reste visible sans texte.

Le panneau supprime les paragraphes explicatifs signalés pour les monstres,
coffres, coffres spéciaux, waypoints et shrines. Le bouton global devient
`Pause/Resume All MapSense Features`, tandis que Reveal est une action séparée
`Reveal Entire Difficulty` / `Stop Persistent Reveal`, ce qui élimine
l'apparence de deux master switches. Le TOML package supprime également les
commentaires objets correspondants.

Le grand cercle vert, le X rouge et l'affichage `Pathing` ne sont toujours pas
attribués à MapSense : ses nouveaux missiles sont des ellipses pleines bornées à
16 px, ses monstres suivent les formes configurées et sa navigation ne produit
que des lignes. Les symboles de debug missile exposés par D2RCore restent le
candidat statique positif. Aucun global d'un autre composant n'est modifié au
hasard; la preuve finale exige un A/B runtime lorsque Vincent sera disponible.

Le runtime demeure volontairement sur 0.13.37. Ce lot est fermé par compilation
Release `/W4 /WX`, CTest `1/1`, contrôle du paquet et deux builds propres
byte-identiques. La DLL build et package mesure 3 511 808 octets, porte
PE/PluginInfo `0.13.38-candidate` et vaut SHA-256
`156FAD56537BDD080BF1D5EBA2BC805027B0EFC5121D221E904C2BCD2EA2D83B`.
Le cold start, le rendu missile Acte IV, l'absence du faux nom et la nouvelle
ergonomie du panneau restent `not run` jusqu'au prochain gate humain.

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

Le runtime BKVince porte actuellement le candidat MapSense 0.13.37, SHA-256
`D8F9CD16E04385DFB97DBE288D39D11F140D095E7BA5DE452BE40B2FE016F33A`.
Le rollback immédiat vers la 0.13.36 byte-identique réside sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.37-predeploy-20260902-212524/`.
Le rollback 0.13.31 antérieur réside sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.31-predeploy-20260901-224037/`.
Le TOML personnel schéma 13 post-migration vaut SHA-256
`748CF1EED016DD31EA1B58D13EA2B7C1A1CDDB6C0A2748F3B3B63609B3E3FE46`.
Le rollback immédiat sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.9-predeploy-20260830T140216/`
conserve la DLL 0.13.8 exacte de SHA-256
`FC5BDD5BA37FE8BE9A8F3FEC7C99375CF4DCF18FA876B74B37C84564FCC39A59`
et le TOML personnel schéma 12 pré-migration de SHA-256
`8A8D06D2D4837EE594018EB90FF18285B87A0AE1EBF6461C210E27F3A400A760`.
Le rollback antérieur conserve aussi la DLL 0.13.7 de 2 913 280 octets,
SHA-256 `AE95D7A710100A8121F10C99F34739B2444E9AFB2D759E066AC0B0D3D7AFAA15`,
la DLL 0.13.8 exacte et son TOML sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.8-predeploy-20260830T113926/`.
La copie exacte de rollback 0.13.5
(`D1ADEBCCA0B15E1217A7574ABEE62CEE129712E47115FC43294D4B4609C292A4`)
et ce TOML sont conservés sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.6-localization-reveal-20260830T000126/`.
Le runtime 0.13.0 précédemment qualifié, ainsi que la copie exacte de rollback
0.12.5 (`67AA43A3…FC1301CD9`), restent conservés sous
`analysis-cache/runtime-sync-backups/mapsense-0.13.0-20260829T192433/`.
La copie byte-identique de rollback 0.12.0 (`7E4F56DC…5AB04D9`) et ce TOML
pré-déploiement sont conservés sous
`analysis-cache/runtime-sync-backups/mapsense-0.12.1-20260828-181409/`.
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

## Maintenance 1.0.0 — coexistence Automap Serialization Fix

Le 3 septembre 2026, MapSense `1.0.0` devient un consommateur read-only des
deux états complets du témoin à `D2R+0xD7E3F` : les 13 octets vanilla ou les
13 octets possédés par le plugin autonome `Automap Serialization Fix 0.1.0`.
Une troisième séquence demeure fail-closed. Le premier essai runtime a révélé
un faux message d'erreur lorsque le premier état testé échouait avant que le
second réussisse; la comparaison finale est silencieuse entre les deux états
admis et ne journalise qu'un refus réel.

Deux builds propres et CTest `1/1` passent. Le build, le package et l'artefact
runtime final sont byte-identiques : `3 553 280` octets, PE/PluginInfo `1.0.0`,
SHA-256
`2B71748E53084FDE72E36731293251C9776072F690AA7224F4E579AE7CC624A1`.
Les deux ordres de chargement passent sur Battle.net D2R `3.3.93847` avec
`39` plugins, `17` patches, les cinq DLL eezstreet et zéro erreur fraîche.
Le runtime BKVince est ensuite restauré byte-exact à MapSense `0.13.41`; le
candidat `1.0.0` n'y reste pas déployé.

## Maintenance 1.0.2 — crash de fermeture DirectX 12

Le 4 septembre 2026, Vincent autorise par `GO` le correctif immédiat du crash
de fermeture MapSense prouvé par un rapport externe sur D2R `3.2.92777`, tout
en différant l'attribution du conflit MapSense/Floating Damage d'un premier
essai jusqu'à réception des versions, hashes, inventaires globale/mod-locale et
logs Floating Damage/D2RLoader du reporter. Les builds `92777` et Battle.net
`93847` restent couverts par le corpus natif commun vérifié; cette équivalence
de `D2R.exe` ne couvre pas le cycle de vie de `D3D12Core.dll`, du pilote GPU ni
la séquence de terminaison du Loader.

Le rapport de crash de 10 625 octets, SHA-256
`1CDD1226827E5173CE6B616068400646CD0F061D7EF5200FA83C80C5E270FD52`,
montre `FatalExit -> RtlExitUserProcess -> LdrShutdownProcess`, puis le
destructeur CRT MapSense à `+0xEDE23`, son helper à `+0x1C53` et une violation
d'accès dans `D3D12Core.dll`. Le désassemblage de l'artefact public MapSense
`1.0.1` de SHA-256
`ECFC729CB41A0ECF71C5FF4FFA43135B9680D05D711D46109D408103F235D921`
relie exactement ces offsets au destructeur du vecteur global
`SwapChainQueueBindings` et à son appel `IUnknown::Release` sur chaque command
queue. Ce vecteur propriétaire était resté hors de `RendererStorage`, malgré
le contrat existant qui place les autres références COM/DX12 dans un stockage
alloué pour la durée du processus afin d'éviter leur destruction après le
démontage de D3D12Core.

La décision retenue déplace le registre complet des bindings dans le même
`RendererStorage` process-lifetime et conserve son `clear()` déterministe dans
le chemin normal `ShutdownD3D12ImGuiHost`. Aucun nettoyage D3D/COM n'est ajouté
à `DllMain`, où le loader lock rendrait cette stratégie dangereuse. Le gate
statique doit prouver qu'aucun conteneur propriétaire de `ComPtr` ne subsiste à
durée statique hors de ce stockage, que le build Release x64 et les tests
MapSense/Suite passent, et que le nouveau PE ne possède plus le destructeur
tardif observé à l'offset fautif. Déploiement, cold start, fermeture normale et
trajet `FatalExit` restent `not run` jusqu'à une autorisation runtime séparée.

Le correctif source est qualifié statiquement. Deux arbres Release x64 propres,
stricts `/W4 /WX`, produisent la même DLL MapSense `1.0.2` de `3 552 256`
octets, SHA-256
`D7CBA865D672D01FFDF3A6BAFE91FF5C704A76A7FC1EFB3C8EAED243C7D6838C`.
CTest passe `1/1` dans chaque arbre et `scripts/Test-Suite.ps1` retourne
`VALID` pour les 18 plugins présents. Un gate permanent refuse maintenant la
source si `SwapChainQueueBindings` redevient un vecteur statique ou quitte
`RendererStorage` process-lifetime.

L'audit COFF du nouvel objet montre seulement un initialiseur de référence qui
copie `ProcessRendererStorage` vers l'alias `SwapChainQueueBindings` puis
retourne; il n'appelle pas `atexit`. Aucun symbole de destructeur dynamique
`??__FSwapChainQueueBindings` n'est émis. Le destructeur CRT propriétaire qui,
dans `1.0.1`, parcourait les bindings et appelait les deux `Release()` après le
démontage D3D12 est donc éliminé. Les quatre exports publics sont préservés.
La correction est build-qualified, pas runtime-qualified : aucun binaire n'a
été déployé et aucun processus Diablo n'a été lancé ou fermé.

### Qualification runtime locale du correctif de fermeture

Vincent autorise la matrice runtime le 4 septembre 2026 avec les deux états de
Floating Damage. Le candidat MapSense `1.0.2` exact est déployé en portée
mod-locale BKVince par `Sync-SuiteRelease.ps1`; le reçu conserve la DLL
MapSense `1.0.1` précédente. Le runtime testé est Battle.net D2R `3.3.93847`,
Build Key `623f7a1f73eabb08ccb2b2046e3f9164`, D2R.exe SHA-256
`E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`,
avec D2RLoader `1.2.1-beta` SHA-256
`27A79CCD61360CC03E7C623D20A46546E5732B9997C41DC185B5EFC335B5C084`.

Le cas diagnostique sans Floating Damage charge MapSense `1.0.2`, accepte son
empreinte native complète, installe les hooks DX12, capture la command queue et
initialise son hôte ImGui. Le démarrage atteint `24/24` avec `37` plugins,
`17` patches et un doublon global correctement ignoré. `CloseMainWindow`
termine le processus en moins de 30 secondes, sans rapport de crash, événement
Windows Application Error/WER ni processus restant. L'assertion TACT
D2RLoader déjà documentée réapparaît après le startup, mais MapSense initialise
ensuite son renderer et aucune faute MapSense/D3D12 ne suit.

Floating Damage `1.4.3`, SHA-256
`1F6BAE0AAC61FEA227E221719F0B7260C581EFC0F342A26039F4CA54D4E7FE2B`,
est ensuite restauré byte-exact dans sa portée globale. Le second cold start
charge `38` plugins, applique `17` patches et atteint `24/24`. Floating Damage
accepte son empreinte, sélectionne l'hôte prioritaire MapSense puis rend sa
première frame par celui-ci. La même fermeture normale termine sans crash,
événement Windows ni processus résiduel. MapSense `1.0.2` reste finalement
déployé mod-local, Floating Damage est restauré globalement, et aucune
configuration n'a été copiée ou modifiée.

La correction est donc runtime-qualified pour cold start, ownership renderer,
coexistence Floating Damage et fermeture normale sur Battle.net `93847`. Le
trajet `FatalExit` exact du reporter sous `92777` avec le mod `zyb` reste
`not run` localement; il n'existe pas de déclencheur sûr et gouverné dans cette
matrice. La preuve COFF demeure l'assurance directe que son destructeur CRT
fautif n'est plus présent. Les journaux, hashes et le reçu rollbackable sont
conservés sous
`analysis-cache/runtime-deployments/suite/mapsense-1.0.2-shutdown/20260904T171136375Z/`.
