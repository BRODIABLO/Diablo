# BKVince Hero Editor — parité fonctionnelle et visuelle RuneWizard

Dernière mise à jour : 3 août 2026

## Décision gouvernée

Vincent relance le **BKVince Hero Editor** comme chantier actif en parallèle de
la mission BaseMod 3.2. Il ne remplace pas la priorité désignée par
`Mission/CURRENT.md`.

La séquence retenue est **Option A — tranches verticales fonctionnelles** :
chaque tranche doit livrer un parcours réellement utilisable, avec son interface
au langage visuel RuneWizard, ses tests de sauvegarde et ses preuves BKVince,
avant d'étendre le périmètre. Un clone visuel complet non fonctionnel n'est pas
un jalon accepté.

La cible est **BKVince**, le mod actuel. BK reste un mod de référence distinct
et read-only; il n'est ni le nom court ni la source de données de l'éditeur.

## Objectif produit

Créer sous `apps/hero-editor/` un éditeur web RuffnecKk capable d'ouvrir, créer,
modifier et télécharger des sauvegardes compatibles avec BKVince et D2R
3.2.92777. Le périmètre fonctionnel et le langage visuel doivent atteindre la
parité avec l'éditeur de D2RuneWizard audité le 3 août 2026, tout en restant un
produit indépendant et gouverné par les données BKVince.

Les sauvegardes restent dans le navigateur. L'application ne les téléverse pas,
ne dépend d'aucune base de données et n'écrase jamais le fichier source. Chaque
export est téléchargé comme un nouveau fichier.

## Faits vérifiés

- l'ancien `apps/hero-editor/` est récupérable dans l'historique Git : ajout au
  commit `1ae09f85`, retrait au commit `e32cfd25` ;
- aucune cause technique d'abandon n'est établie : Vincent avait cru disposer
  d'un workaround à l'incompatibilité RuneWizard/BK, puis a retenu son propre
  éditeur comme bonne direction ;
- l'ancien prototype générait un catalogue déterministe depuis les TXT de
  BKVince, validait les D2S v104/v105, forgeait un sous-ensemble d'objets
  uniques `misc` et avait produit un Annihilus accepté en jeu ;
- ce prototype ne fournissait pas encore l'édition complète d'inventaire,
  l'équipement, le belt, le Cube, les stashes, le mercenaire, toutes les
  qualités, les sockets ni les stackables ;
- `@d2runewizard/d2s` `2.0.132` est déjà une dépendance du workspace et peut
  servir de codec derrière un adaptateur local, sans dépendre du site
  RuneWizard ;
- les constantes BKVince actuelles exposent huit classes que le codec sait
  sérialiser avec trente skills chacune : Amazon, Sorceress, Necromancer,
  Paladin, Barbarian, Druid, Assassin et Warlock ; aucune preuve actuelle ne
  justifie d'annoncer onze classes encodables ;
- la suite locale actuelle passe `8/9` : l'unique échec est une assertion de
  quantité de propriétés devenue obsolète après l'évolution des tables
  BKVince (`391` au lieu de `389`) ;
- un aller-retour sans modification sur le témoin QtyTester conserve la taille
  et se reparse, mais modifie huit positions d'octets. Cette différence est un
  gate de préservation à résoudre ou à justifier par une allowlist précise; ce
  n'est pas une cause d'abandon historique.

## Référence fonctionnelle auditée

La parité cible couvre au minimum :

- ouverture d'un D2S, création d'un personnage vierge et export aux versions
  prises en charge, avec undo/redo ;
- identité, niveau, expérience, seed, attributs, points inutilisés, vie, mana,
  stamina, or et flags expansion/hardcore/dead/ladder ;
- équipement joueur et mercenaire, inventory, belt, Cube, stash, virtual
  stash, trash, shared stash et or de coffre ;
- édition visuelle des objets avec déplacements, collisions, rotation lorsque
  permise, qualité, base, item level, identification, ethereal,
  personnalisation, upgrade/downgrade, affixes/propriétés, sockets, contenu des
  sockets, quantité, prévisualisation, suppression et téléchargement ;
- skills par arbre avec mode explicite `Ignore Game Rules` ;
- quests et waypoints pour Normal, Nightmare et Hell, avec actions globales et
  cases individuelles ;
- résumé `Item bonuses`, Chronicle et mercenaire ;
- les surfaces présentes dans la référence mais dépendantes d'un mod, notamment
  `Bound Demon`, après preuve de leur représentation réelle dans la sauvegarde
  BKVince.

La référence visuelle est une interface sombre à grille desktop, panneaux et
tabs compacts, équipement et conteneurs spatiaux, puis un éditeur d'objet en
modal. La reconstruction doit reproduire la hiérarchie, la densité, les
interactions, la palette, la typographie, les espacements et les états utiles,
sans copier le code source ni des assets propriétaires de RuneWizard. Les assets
BKVince ou des créations RuffnecKk gouvernées remplacent les visuels non
redistribuables. Les dépendances réutilisées conservent leurs notices et
licences, notamment l'ISC de `@d2runewizard/d2s`.

### Hors périmètre confirmé

Les **personnages presets** et les **builds prédéfinis** de RuneWizard sont
explicitement exclus sur décision de Vincent du 3 août 2026. L'éditeur permet
de créer un personnage vierge ou d'ouvrir une sauvegarde existante, mais ne
fournit ni catalogue de builds, ni équipement automatique, ni distribution
automatique de stats ou de skills. Cette exclusion ne bloque pas la parité des
écrans d'édition manuelle.

## Séquencement retenu — Option A

### Tranche 1 — fondation jouable

- restaurer le workspace `apps/hero-editor/` depuis l'historique comme matière
  de départ, sans considérer son architecture ni son UI comme définitives ;
- isoler le codec D2S derrière un adaptateur testé ;
- ouvrir un D2S ou créer un héros, afficher Général/Stats, modifier les champs
  de base, annuler/rétablir et télécharger une copie ;
- implanter dès cette tranche le shell, la grille, les tabs, la palette et les
  contrôles du langage visuel RuneWizard ;
- prouver checksum, taille, reparse, absence d'écrasement et diff no-op
  gouverné sur des fixtures anonymisées.

### Tranche 2 — objets et conteneurs essentiels

- inventory, équipement, belt et Cube avec tailles réelles, placement,
  détection de collision, déplacement et suppression ;
- éditeur d'objet pour les qualités prises en charge, propriétés, sockets et
  quantités ;
- catalogue dérivé des TXT BKVince, sans comptes de lignes ni IDs vanilla
  codés en dur ;
- première matrice stackables BKVince, gems, runes, charms et objets custom.

### Tranche 3 — progression complète

- skills et trois arbres de chaque classe disponible dans BKVince ;
- quests et waypoints sur les trois difficultés ;
- règles de jeu par défaut et mode d'override explicitement signalé ;
- expérience, niveau, flags et valeurs dérivées cohérents après relecture.

### Tranche 4 — stockage et mercenaire

- personal stash, shared stash, virtual stash, trash et stashed gold ;
- équipement et données du mercenaire, y compris les emplacements BKVince
  étendus lorsqu'ils sont réellement représentés dans les fichiers concernés ;
- sauvegardes auxiliaires éventuelles, dont D2I/shared stash, seulement après
  identification de leur format, ownership et stratégie d'export atomique.

### Tranche 5 — parité avancée et finition

- `Item bonuses`, Chronicle et actions groupées ;
- surfaces mod-spécifiques démontrées, dont `Bound Demon` si applicable ;
- responsive, clavier, souris, accessibilité, états vides et erreurs lisibles ;
- fermeture complète de la matrice de parité et de la validation runtime.

## Architecture retenue

- React + Vite dans le monorepo npm + turbo ;
- modèle de document immuable avec historique undo/redo ;
- codec D2S encapsulé par une interface locale : le choix de conserver,
  corriger ou remplacer des parties de `@d2runewizard/d2s` dépend des preuves de
  préservation, pas d'une dépendance au site RuneWizard ;
- catalogue généré depuis les tables gouvernées sous
  `data-BKVince/BKVince.mpq/data/global/excel/` ;
- séparation stricte entre bytes source, modèle éditable et bytes exportés ;
- aucune API d'upload, persistance distante ou écriture directe dans une
  sauvegarde locale.

## Gates observables

- [x] Le parcours Tranche 1 ouvre ou crée un héros, modifie Général/Stats,
  undo/redo et exporte un D2S reparseable dans l'UI cible.
- [x] Les fixtures no-op possèdent une politique byte-exact ou une allowlist
  documentée octet par octet, avec checksum et champs recalculés exclus de
  toute fausse alerte.
- [ ] Les catalogues sont régénérables depuis les TXT BKVince courants et leurs
  tests échouent proprement sur header requis absent, clé dupliquée ou donnée
  ambiguë.
- [ ] Chaque conteneur refuse les chevauchements, débordements et placements
  illégaux sans corrompre le document ni perdre un objet.
- [ ] Les qualités, affixes, propriétés, sockets et quantités exportées sont
  relues avec les mêmes valeurs et validées sur des objets BKVince réels.
- [ ] Toutes les classes réellement prises en charge par BKVince, leurs skills
  et leurs données custom sont découvertes depuis les sources actuelles plutôt
  que codées selon le roster vanilla. Le snapshot courant en découvre huit ;
  toute classe supplémentaire exige d'abord une preuve dans les tables et le
  format de sauvegarde.
- [ ] Quests, waypoints, mercenaire, stashes et fichiers auxiliaires passent
  chacun une matrice load/edit/save/reload.
- [ ] La matrice visuelle couvre desktop et responsive : navigation, grille,
  tabs, panneaux spatiaux, modals, focus, erreurs et états vides.
- [x] D2R 3.2.92777 accepte les exports BKVince témoins, puis les recharge après
  `Save and Exit` sans rollback, perte d'objet, bad inventory ou bad dead body.
- [x] Les sauvegardes sources restent inchangées et aucun octet de personnage
  n'est envoyé hors du navigateur.
- [ ] La matrice de parité RuneWizard est documentée ligne par ligne et ne
  contient plus de fonction cible non couverte ou explicitement déclarée non
  applicable avec preuve.

## Risques ouverts

- le format D2S et les extensions BKVince peuvent contenir des champs que le
  codec générique normalise ou ignore lors de la sérialisation ;
- les stackables, les emplacements ExtendedMerc et les extensions de classe
  exigent des fixtures BKVince réelles et anonymisées ;
- shared stash, virtual stash et Chronicle peuvent vivre hors du D2S principal
  et demander plusieurs exports cohérents ;
- la ressemblance visuelle doit rester une réimplémentation indépendante avec
  des assets redistribuables et des crédits propres.

## État d'implantation — 3 août 2026

La première tranche existe maintenant sous `apps/hero-editor/` :

- génération déterministe des constantes depuis les TXT/JSON BKVince, avec
  round-trip TSV byte-exact avant consommation ;
- création vierge et relecture validées automatiquement pour les huit classes
  actuellement encodables ;
- import v105 avec refus d'une taille déclarée ou d'un checksum invalide ;
- écrans General/Stats, limites dérivées d'ItemStatCost, undo/redo et shell
  sombre inspiré de la hiérarchie RuneWizard ;
- export no-op depuis les octets source exacts et export modifié reparsé après
  recalcul de la taille et du checksum ;
- build Vite et neuf tests unitaires verts, plus parcours local vérifié dans le
  navigateur pour création Warlock, modification de niveau, undo/redo et écran
  Stats sans erreur console.

Le gate runtime de la tranche 1 est fermé le 3 août 2026 sur D2R
`3.2.92777` :

- le témoin vierge `HEBlank.d2s` (Warlock niveau 1, Force 15, 0 point libre,
  0 or) et le témoin modifié `HEEdited.d2s` (Force 42, 7 points libres,
  12 345 or) ont été reconnus dans la sélection, chargés dans le camp des
  Rogues, vérifiés dans l'écran Character, puis resauvegardés par
  `Save and Exit` ;
- les exports testés avant D2R portent respectivement les SHA-256
  `AA6DC8298CFDD2CADC77F76E680AC9236D59E987D45A00F219199AACBF355771`
  (922 octets) et
  `9D970CC4D71C211E412F384C533278C18FE0CFB7580655B85E77DBE90F598597`
  (928 octets) ;
- après resauvegarde, les deux enveloppes conservent leur taille, possèdent un
  checksum valide et se rouvrent dans l'adaptateur local. Les valeurs métier
  sont intactes; D2R a seulement renouvelé la seed de carte et marqué Normal
  comme difficulté active. Leurs SHA-256 runtime deviennent
  `738C944CB1C7C686B40531089B59DDDE1E824230AB925A6A0D5F083D72CAB901`
  et `9D1CA7C562842ACC15C9C56D8B1811A640FCCB9F2602DE3B2BDD08195C324701` ;
- le correctif requis encode chaque emplacement visuel vide avec `0xFF`
  (`no item`) plutôt qu'avec l'identifiant `0`; les neuf tests du Hero Editor
  restent verts ;
- les quatre assertions `Items.cpp` observées au frontend sont reproduites à
  l'identique après retrait complet des deux témoins du dossier de sauvegarde :
  elles sont préexistantes et ne sont pas causées par ces exports ;
- le cold start gouverné est consigné dans
  `analysis-cache/runtime-sync/20260803-122005135-apply.json` : démarrage 24/24,
  18 patchsets appliqués sur 18, 11 plugins actifs sur 11, zéro échec ou rejet.

La tranche 1 est donc **validée en jeu**. L'inventory n'est plus interdit par ce
gate, mais chaque nouvelle opération d'objet devra obtenir sa propre preuve
load/edit/save/reload avant d'élargir le périmètre.

Le premier parcours vertical de la tranche 2 est implanté et validé hors jeu :

- l'UI affiche les grilles BKVince réelles — inventory `11 × 8`, Cube `6 × 6`,
  stash personnelle `16 × 13` — ainsi que les douze BodyLoc d'équipement et la
  ceinture `4 × 4` dérivée de son index aplati ;
- un objet existant peut être sélectionné puis déplacé vers inventory, Cube ou
  stash personnelle. Les débordements et chevauchements sont refusés avant
  toute mutation; l'équipement et le belt restent volontairement en lecture
  seule comme destinations tant que leur compatibilité et leur capacité ne
  sont pas prouvées ;
- l'écriture ne modifie que `location_id`, `equipped_id`, `position_x`,
  `position_y` et `alt_position_id` sur le record existant. Une validation
  supplémentaire refuse l'export si tout autre champ d'objet change au
  write/reparse ;
- le test portable `DummyTester-Annihilus` couvre neuf objets racine : quatre
  potions de belt, deux objets équipés et trois objets d'inventory. Le scroll
  de portail est déplacé vers le Cube, l'Annihilus conserve ses dix propriétés
  magiques et le no-op reste byte-exact ;
- `npm.cmd --workspace apps/hero-editor test` passe `9/9` et la build Vite de
  production est verte. La preuve D2R de ce déplacement reste ouverte.

La création des personnages couvre maintenant les huit charms de départ BK et
son gate runtime est fermé :

- chaque classe encodable reçoit automatiquement `mff` en `(10, 0)`, `mfc` en
  `(10, 1)`, puis les six `mfd` en `(10, 2..7)`, soit la colonne verticale
  réservée à droite de l'inventory `11 × 8` ;
- les qualités uniques, IDs `438..440`, propriétés de `mfc`/`mff`, flags de
  départ, données realm v105 et champ Advanced Stash des `mfd` reproduisent le
  témoin BK natif `ama.d2s`. Un patch reproductible de `@d2runewizard/d2s`
  préserve les quatre `uint32` realm et le bit de présence de quantité v105 ;
- le témoin `HEBKCharm.d2s` avant D2R fait 1 225 octets et porte le SHA-256
  `B96785486F59295C79B949303F9601A98E8D800CC930B1245C76BA7DFCC696A5` ;
- D2R `3.2.92777` charge le personnage, affiche les huit charms dans la colonne
  gelée, le sauvegarde, puis le recharge une seconde fois avec la même
  disposition. Le fichier final reste à 1 225 octets, se reparcourt dans
  l'éditeur avec les huit records et toutes leurs propriétés, et porte le
  SHA-256
  `150C3898723BD3DD81B1E6873C17B90F9A51F39E8ADFBAB88AF31547B099F0B7` ;
- aucune assertion `Items.cpp` ne survient pendant les deux chargements. Les
  quatre assertions du frontend précèdent la sélection du témoin et restent
  celles déjà reproduites sans les exports du Hero Editor.

## Prochain gate

Exporter le témoin `DummyTester-Annihilus` avec son scroll déplacé de
l'inventory vers le Cube, puis prouver load/edit/save/reload dans D2R
3.2.92777 sans perte ni `bad inventory`. Le catalogue et l'éditeur d'objet
avancé restent séquencés derrière cette première preuve runtime de placement.
