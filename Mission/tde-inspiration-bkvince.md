# Catalogue d’inspiration TDE 3.1d pour BKVince

## Statut

Étude parallèle gouvernée — 31 juillet 2026. Elle ne remplace pas la mission active désignée par `Mission/CURRENT.md` et ne modifie aucun fichier gameplay, plugin, patch mémoire, configuration ou runtime de BKVince.

Source étudiée : `TDE_D2SE_3.1d.1.7z`, SHA-256 `671D0636EF96FF59D94192C3FF0DD73AB69CAAE1D41FC4563F796149AFBC9356`.

Le 10 août 2026, Vincent a choisi de ramener les tables et comportements
mercenaires BKVince à l'état du commit pré-TDE
`e67e66d277230dcdd577e4f608ef293448cd07ea`. Le catalogue TDE demeure une
étude sans effet gameplay; les prototypes data ouverts après ce jalon sont
retirés et devront repartir d'une nouvelle conception avant toute reprise.
`RogueScoutMovement` constitue l'exception explicitement confirmée par Vincent :
le plugin autonome de déplacement demeure actif et indépendant des tables
mercenaires retirées.

Le rollback conserve uniquement des tombstones inertes aux ordinals déjà
occupés (`ItemStatCost` 390, `Properties` 309, `Skills` 442–448, `States`
243–244 et `MonStats` 797–798). Ils empêchent le décalage des IDs ajoutés
ultérieurement et ne portent plus aucun comportement mercenaire. Les tables
`hireling.txt` retrouvent exactement les groupes pré-TDE : 24 Rogue Scout,
45 Desert Mercenary et 36 Eastern Sorceror.

La preuve runtime finale du 10 août 2026 utilise
`D2RLoader.exe -mod BKVince -txt -offline` sur le build 92777. `pettype.txt`
reste retiré, tandis que `RogueScoutMovement 0.1.0` est réinstallé dans la
portée globale avec son TOML; ses deux hooks sont acceptés à `0x4473F0` et
`0x5C1460`. Le cold start atteint `24/24`, applique `18/18` memory patches et
active `17/17` plugins, sans désactivation, rejet ni échec. Le rapport est
`analysis-cache/runtime-sync/20260810-082337827-apply.json` et les logs frais
sont figés sous
`analysis-cache/rogue-scout-movement-reinstall/20260810-082337827/`.

Le catalogue machine est [tde-inspiration-bkvince.catalog.json](tde-inspiration-bkvince.catalog.json), régi par [tde-inspiration-bkvince.schema.json](tde-inspiration-bkvince.schema.json) et `npm run validate:tde-catalog`.

## Intention

Le but n’est pas de porter TDE vers D2R à l’identique. Le but est d’identifier ses idées de gameplay, puis de décider lesquelles méritent une conception originale pour BKVince sous D2RLoader 3.2. Les voies data-only, plugin natif, memory patch et hybride data + natif sont toutes admissibles. Les fonctions qui exigeraient de nouveaux assets visuels ou audio sont hors périmètre.

Les crédits TDE demeurent distincts des futures créations RuffnecKk. Aucun bloc de prose, aucune table, aucun asset et aucun binaire TDE n’est copié dans BKVince ou dans un addon publiable.

## Couverture exhaustive

Le README anglais principal contient exactement 2 038 paragraphes non vides. Ils sont couverts sans reproduction du texte par 21 tranches contiguës, chacune munie d’un SHA-256 calculé sur les paragraphes normalisés et numérotés. Le validateur refuse un trou, un chevauchement ou une fin différente de 2 038.

Les 91 tables TXT libres ont été auditées comme preuves structurelles seulement : toutes sont CRLF et passent le round-trip du parseur TSV byte-exact. Face aux sources BKVince/vanilla 3.2, 81 tables ont un nom commun; 18 ont un header exact, 63 présentent une différence de schéma, et 10 tables existent seulement de chaque côté. Les collisions d’identifiants déjà observées dans `skills.txt`, `missiles.txt`, `states.txt` et `itemstatcost.txt` interdisent tout merge par numéro ou remplacement global.

Cette couverture signifie que tout le document et tout le jeu de tables ont été pris en compte. Elle ne transforme pas chaque phrase descriptive, statistique d’objet ou variante de monstre en chantier indépendant : le catalogue regroupe les concepts réutilisables, consigne les doublons et rejette explicitement les hypothèses incompatibles.

## Faits vérifiés

- TDE 3.1d repose sur le moteur LoD 1.13c/1.13d avec BaseMod et PlugY; ses DLL sont x86, tandis que D2R 3.2.92777 et D2RLoader utilisent des surfaces x64 différentes.
- TDE déclare ne pas être conçu pour le multijoueur. Une adaptation BKVince ne peut donc reprendre aucune hypothèse d’autorité client sans preuve solo, hôte et joiner.
- La documentation TDE indique que ses changements `itemstatcost.txt` rendent les sauvegardes vanilla incompatibles. BKVince doit conserver ses formats actuels tant qu’une évolution de persistance n’a pas été conçue et migrée explicitement.
- TDE exclut lui-même une version D2R parce que plusieurs artworks classiques activés n’ont pas d’équivalent HD. Ces fonctions restent hors périmètre du catalogue retenu.
- Le workbench D2R 3.2.92777 est vérifié. Il contient déjà des preuves gouvernées adjacentes pour les services de récompense, le stock marchand, l’autogold, les mercenaires étendus et plusieurs patches de quête; il ne contient pas encore de preuve pour la plupart des hooks nouveaux proposés ici.
- BKVince possède déjà des propriétaires pour Vendor Stock Refresh, une partie de l’auto-pickup, le health bar Caleb, Force Larzuk Sockets et les récompenses infinies. Ces idées sont enregistrées comme doublons ou couvertures partielles, pas comme nouveaux ports TDE.

## Hypothèses à tester

- La coexistence loups + grizzly pourrait déjà être disponible ou partiellement absorbée par D2R 3.2/BKVince; un fixture en jeu doit précéder toute tâche.
- Le chemin Super TK existant peut peut-être fournir une partie d’un pickup Telekinesis plus général, mais la propriété des objets, les objets de quête et l’autorité réseau ne sont pas prouvés.
- Les chemins gouvernés de Charsi, Larzuk et Anya peuvent probablement soutenir un service payant après consommation du droit gratuit. Le débit d’or autoritaire, les états de quête et les cas host/joiner restent à démontrer.
- ExtendedMerc réduit le risque d’interface et d’équipement d’un mercenaire Act IV, sans prouver la création d’un nouveau vendeur de mercenaires, son identité sauvegardée ni son protocole réseau.
- Les événements de zone corrompue peuvent être reformulés avec les assets déjà présents dans BKVince. Leur cycle de vie, leur coexistence avec les Rifts/Terror Zones et leur autorité serveur demeurent inconnus.

## Top 10 data-first

Le score équilibré sur 100 est `valeur joueur × 6 + adéquation BKVince × 5 + confiance technique × 4 + isolation/réversibilité × 3 + maintenance × 2`. Chaque composante vaut de 0 à 5. Les égalités sont tranchées par la valeur joueur, l’adéquation BKVince, l’effort le plus faible, puis l’identifiant stable.

| Rang | Idée adaptée | Score | Route et prochain gate |
|---:|---|---:|---|
| 1 | Récupérer les composants lors du vidage d’un objet socketed | 100 | Recettes BKVince originales; fixer le coût et vérifier l’économie |
| 2 | Diviser une gem de grade supérieur en trois gems inférieures | 94 | Recettes data-only; composer avec Storage Bag et le barème 1/3/9/27/81 |
| 3 | Faire pivoter large charm et grand charm | 92 | Nouvelles variantes de footprint sans nouvel art; auditer inventaire et stockage |
| 4 | Garantir un plancher utile aux recettes de socket drilling | 87 | Cube seulement; vérifier caps, bases et économie runeword |
| 5 | Reroll des rare items avec coûts par niveau | 87 | Définir ilvl, composants et sinks propres à BKVince |
| 6 | Progression lisible des immunités par difficulté | 86 | Audit trois voies de `monstats.txt` et composition avec Sunder/pierce-res |
| 7 | Création déterministe de rare items depuis des bases ciblées | 83 | Concevoir bases, ilvl et coûts sans reprendre les recettes TDE |
| 8 | Pression élémentaire accrue dans les difficultés hautes | 78 | Seulement si la courbe d’immunités est réellement assouplie |
| 9 | Redistribuer les guest monsters dans la progression | 75 | Préserver Rifts, Heralds, quêtes et groupes Terror Zone |
| 10 | Désigner des zones niveau 85 plus accessibles | 73 | Fixer les zones et populations sur les sources BKVince actuelles |

## Top 10 natif ou hybride

Ce classement établit des candidats d’étude, pas une autorisation de coder une DLL. Pour chacun, la prochaine étape obligatoire est la question d’incubation « Plugin autonome ou plugin à merger au PluginPack ? », suivie des preuves ABI/hooks propres au build 92777.

| Rang | Idée adaptée | Score | Preuve actuelle et prochain gate |
|---:|---|---:|---|
| 1 | Charsi, Larzuk et Anya comme services payants après le droit gratuit | 89 | Preuves partielles existantes; décider propriétaire, prix, autorité et états de quête |
| 2 | Exiger la touche d’affichage des items pour ramasser au sol | 87 | Hook 92777 manquant; définir souris, manette et host/joiner |
| 3 | Étendre Telekinesis à une allowlist de pickups | 81 | Super TK partiel; prouver objets, propriété, quêtes et réseau |
| 4 | Uniformiser les événements de tous les missiles de Multiple Shot | 78 | Prouver la boucle projectile et plafonner l’amplification des procs |
| 5 | Ajouter un mercenaire de soutien en Act IV | 77 | ExtendedMerc adjacent; prouver embauche, roster, sauvegarde et réseau |
| 6 | Rendre configurable le cooldown hardcodé de Cloak of Shadows | 73 | Aucun RVA gouverné; retrouver le chemin 92777 et tester les états/IA |
| 7 | Moderniser la sélection de cible de Guided Arrow | 71 | ABI projectile/cursor et déterminisme réseau inconnus |
| 8 | Introduire des groupes de cooldown indépendants | 67 | Concevoir états, charges, feedback UI et autorité |
| 9 | Recalculer en temps réel les passifs dépendant de l’équipement | 65 | Remplacer le modèle belt/charm TDE par un propriétaire événementiel BKVince |
| 10 | Déclencher occasionnellement des zones corrompues | 62 | Réutiliser uniquement les assets BKVince; prouver cycle de zone et coexistence endgame |

## Doublons, rejets et exclusions

- Vendor Stock Refresh est déjà gouverné et intégré sous `items.vendorStockRefresh`.
- L’auto-pickup par catégories chevauche PotionAutoPickup et les patches autogold; toute extension doit rejoindre ce propriétaire.
- Le health bar dynamique chevauche l’interface Caleb déjà livrée; aucune friction supplémentaire n’est démontrée.
- Le remplacement global des tables TDE est rejeté à cause des versions, schémas, identifiants, sauvegardes, règles réseau et assets.
- Les hypothèses explicitement single-player de TDE sont rejetées comme base d’implantation native.
- Toute fonction dépendant d’un artwork ou d’un son TDE sans équivalent HD existant est exclue.

## Recommandation

Commencer, lorsque Vincent voudra ouvrir un lot TDE, par une seule idée data-first isolée : **préserver les socketables lors du vidage d’un item**. C’est le candidat au meilleur score, le plus réversible et le moins dépendant du runtime natif. L’étude doit d’abord confirmer que BKVince ne possède pas déjà une recette équivalente, fixer un coût cohérent avec son économie, puis utiliser le workflow `diablo-tsv` pour une implantation ciblée et byte-safe.

Le premier chantier natif recommandé est ensuite **les services payants Charsi/Larzuk/Anya**, parce que des preuves 92777 adjacentes existent déjà. Il ne doit toutefois pas commencer avant le gate de destination du plugin et une décision explicite sur le contrat joueur : prix, gratuité initiale, consommation de quête, limites, solo/hôte/joiner et interaction avec les patches Infinite.

## Gates de décision avant implantation

1. Vincent choisit un seul candidat du catalogue et confirme qu’il devient une mission, sans changer implicitement la priorité désignée par `Mission/CURRENT.md`.
2. Pour un candidat natif, appliquer intégralement `d2rloader-plugin-incubation` et fermer le gate autonome ou merge avant tout code/configuration/archive.
3. Pour un candidat data, produire le différentiel BKVince/vanilla 3.2/TDE utile, puis définir le comportement BKVince sans copier les lignes TDE.
4. Pour un candidat natif ou hybride, interroger le workbench 92777, consigner preuves, ABI, signatures, autorité réseau et collisions de hooks.
5. Définir sauvegarde, migration et rollback avant toute modification qui crée des IDs, stats, objets ou états persistants.
6. Exécuter les tests ciblés, le cold start et la matrice gameplay seulement après une implantation explicitement autorisée.

## Prochain gate du workstream

Le 1 août 2026, Vincent retient `alt-required-ground-pickup` pour un futur merge
dans `plugin-items.dll` sous `items.requireItemDisplayForPickup`, selon l’Option A :
le candidat rejoint MassID dans la réserve du prochain lot PluginPack sans
remplacer la mission courante. Sa conception gouvernée est consignée dans
`Mission/require-item-display-for-pickup-3.2.md`.

Poursuivre la revue séquentielle des autres candidats TDE. Le candidat retenu
reste sans effet sur le jeu jusqu’à l’ouverture explicite de son lot et au
franchissement de ses gates RE, hooks, périphériques et réseau.
