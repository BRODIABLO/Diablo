# BKVince Hero Editor — parité fonctionnelle et visuelle RuneWizard

Dernière mise à jour : 4 août 2026

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
- la suite locale actuelle passe `62/62`; elle couvre notamment la création des
  huit classes, la préservation byte-exact sans modification, les placements
  d'objets et une sauvegarde v105 réécrite par D2R ;
- un export sans modification restitue les octets source exacts. Les exports
  modifiés recalculent l'enveloppe et sont toujours reparsés avant livraison.

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
- quests et waypoints pour Normal, Nightmare et Hell, avec actions globales
  Quests et cases individuelles conformes aux captures ;
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
- [x] Les catalogues sont régénérables depuis les TXT BKVince courants et leurs
  tests échouent proprement sur header requis absent, clé dupliquée ou donnée
  ambiguë.
- [x] Chaque conteneur refuse les chevauchements, débordements et placements
  illégaux sans corrompre le document ni perdre un objet.
- [ ] Les qualités, affixes, propriétés, sockets et quantités exportées sont
  relues avec les mêmes valeurs et validées sur des objets BKVince réels.
- [ ] Toutes les classes réellement prises en charge par BKVince, leurs skills
  et leurs données custom sont découvertes depuis les sources actuelles plutôt
  que codées selon le roster vanilla. Le snapshot courant en découvre huit ;
  toute classe supplémentaire exige d'abord une preuve dans les tables et le
  format de sauvegarde.
- [x] Quests, waypoints et édition d'un mercenaire existant passent chacun une
  matrice load/edit/save/reload, avec équipement mercenaire natif conservé.
- [x] La création d'un mercenaire absent passe create/export/load/save/reload,
  puis sa suppression marque aussi ses records `jf` et reste réversible par Undo.
- [ ] Shared/virtual stash et fichiers auxiliaires passent chacun leur matrice
  load/edit/save/reload.
- [x] La matrice visuelle couvre desktop et responsive : navigation, grille,
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
- l'élargissement des opérations stackables au-delà du compteur prouvé, les
  emplacements ExtendedMerc et les extensions de classe exigent encore des
  fixtures BKVince réelles et anonymisées ;
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
- build Vite et dix tests unitaires verts, plus parcours local vérifié dans le
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
- `npm.cmd --workspace apps/hero-editor test` passe `10/10` et la build Vite de
  production est verte.

Le gate runtime du premier déplacement d'objet est fermé :

- le témoin portable `HECubeMove.d2s` déplace uniquement le Town Portal Scroll
  vers le Cube en `(0, 0)`. Avant D2R, il fait 1 069 octets et porte le SHA-256
  `EBBB238B352C3A0472CBB19C20034B2C378892D9626BC0504815A5642DA9383F` ;
- D2R `3.2.92777` charge le témoin sans `bad inventory`, le sauvegarde, puis le
  recharge et le sauvegarde une seconde fois. Les deux fichiers runtime font
  1 213 octets et portent respectivement les SHA-256
  `35608907E2C880E2B263A0ACAE5074476D775D1392D7E5914616CC6FC438840C` et
  `078021D273BC4E89450BFD15E071A0ADB20C9E45A99C5C94B95F10F98AB266D2` ;
- après chaque cycle, l'éditeur retrouve neuf objets, le scroll dans le Cube en
  `(0, 0)`, les dix propriétés magiques de l'Annihilus et les quatre valeurs
  realm ajoutées par D2R sur chaque objet. Le no-op reste byte-exact ;
- le codec lit et réécrit désormais les realm data des objets simples v105 et
  respecte l'absence du bit `nr_of_items_in_sockets` sur ce layout. Le correctif
  est explicitement borné à `0x69` pour ne pas modifier les formats antérieurs ;
- le cold start atteint `24/24`, applique `18/18` patchsets et active `11/11`
  plugins, sans échec ni rejet. Les quatre assertions `Items.cpp` surviennent
  avant la sélection comme lors des témoins précédents; aucune nouvelle
  assertion `Items.cpp` n'apparaît pendant les deux chargements. L'assertion
  distincte de zone désacralisée dupliquée reste extérieure au Hero Editor.

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

Le premier lot d'édition manuelle d'un objet est maintenant implanté :

- le générateur byte-exact produit un catalogue trié de 800 bases, 502 entrées
  uniques, 215 objets de set, 711 préfixes, 789 suffixes, 310 propriétés et
  113 types depuis les TXT BKVince courants ;
- le générateur refuse un header obligatoire absent, un code de base ou un ID
  gouverné dupliqué et toute référence Unique/Set non vide vers une base
  inconnue. Les entrées historiques vides restent représentées sans inventer de
  base ;
- la modale expose toujours Base, Quality, Identified, Ethereal et Quantity,
  mais ne propose que les changements compatibles avec la structure binaire du
  record chargé : même mode compact, famille, type, dimensions et mode de pile ;
- les Unique/Set gardent leur base canonique et leur identification verrouillée
  tant que Chronicle n'est pas reconstruit. Les objets simples n'inventent ni
  qualité ni quantité; Ethereal est borné aux lignes Armor/Weapons réelles ;
- le round-trip portable transforme une potion `hp1` en `hp2` et un Hand Axe
  Normal en Superior, non identifié et éthéré, puis reparcourt le D2S avec les
  valeurs exactes et sans changement sur les autres payloads d'objet ;
- `npm.cmd run build -w apps/hero-editor` passe avec 13/13 tests. Le parcours
  visuel local crée un Amazon avec ses huit charms, ouvre Inventory et la
  modale, puis confirme sur `mff` la base canonique et les quatre champs
  incompatibles verrouillés.

Le shell visuel suit maintenant la référence RuneWizard fournie par Vincent :

- Equipment et l'inventory forment la colonne gauche, la stash `16 × 13` la
  colonne centrale, puis Cube et belt la colonne droite sur un canvas sombre et
  plat aux bordures gris-bronze ;
- le personnage et les actions load/create, undo/redo et download occupent la
  barre compacte supérieure ;
- General et tous les attributs validés vivent dans un grand panneau `Stats`
  avec navigation verticale. Quests, Waypoints, Item Bonuses, Skills,
  Chronicle, Mercenary et Demon y sont visibles dans le même ordre que la
  référence, mais restent verrouillés tant que leur écriture n'est pas prouvée ;
- les captures Quests, Waypoints et Skills fournies le 3 août 2026 gouvernent
  leurs futurs layouts : difficultés et actes empilés pour les deux premiers,
  puis arbre de skills par onglets pour le troisième.

Le premier parcours `click-to-add` de la forge est implanté :

- un clic sur une case vide d'inventory, de stash personnelle ou de Cube ouvre
  une modale de recherche alimentée par le catalogue BKVince courant. Elle
  expose 729 bases à la fois `spawnable` et encodables sur quatre caractères,
  avec filtres Armor, Weapons et Misc ;
- les items `compactsave` produisent de vrais records simples v105. Les autres
  bases produisent pour ce gate un record Normal identifié, avec ID unique,
  item level, realm data, défense/durabilité applicables, propriétés vides et
  quantité uniquement lorsque la ligne BKVince est stackable ;
- de 1 à 20 copies peuvent être ajoutées ensemble. Tous les placements sont
  planifiés avant mutation, à partir de la case choisie puis en parcours de
  grille; si une copie déborde ou entre en collision, le groupe entier est
  refusé et le document reste intact ;
- les records ajoutés vivent dans l'historique undo/redo sans modifier le
  document source. L'export les écrit avec les objets existants, recalcule
  l'enveloppe, reparcourt le D2S et compare chaque payload et chaque placement ;
- le hover ou focus affiche le nom, la qualité, la défense, la durabilité,
  l'item level, la quantité et les propriétés décodées. Un clic sur l'item
  ajouté ouvre la modale d'édition existante ;
- la suite passe maintenant `16/16`. Elle prouve une potion `hp1` simple, deux
  Hand Axes Normal ajoutés atomiquement, un Tome complexe à quantité 50, les
  IDs distincts, realm data, durabilité et positions après write/reparse, ainsi
  que le refus atomique de vingt Caps dans le Cube ;
- le parcours navigateur local crée `ForgeTest`, ouvre le catalogue depuis la
  première case du Cube, recherche Hand Axe, ajoute deux copies, affiche leur
  tooltip et ouvre l'éditeur d'un record. Le rendu corrigé n'est plus coupé par
  le panneau et aucune erreur console n'est observée.

Cette tranche simple + Normal est maintenant **validée en jeu** sur un témoin
portable `HEItemForge.d2s` de 1 355 octets :

- la source avant D2R porte le SHA-256
  `3D7812B76E3A3DDB74F7FE6111525682EB91D7733A4880ED3C3A7F4DC0A700C5` ;
- le témoin combine la potion passée de `hp1` à `hp2`, deux Hand Axes dont une
  Superior non identifiée et éthérée, un Tome Normal à quantité 50 et les huit
  charms de départ BK. Les deux Hand Axes sont visibles dans les deux premières
  colonnes de la stash personnelle ;
- deux cycles D2R 3.2.92777 complets de load, `Save and Exit`, reload conservent
  douze items. Les deux sorties portent respectivement les SHA-256
  `51EBD978EDB1FE75387A6D5589D142227A95E70BE6A52397A1C4EFD6707126A3` et
  `1AB38FE863BD569CC0F80FA1B2E9422960029833B3859AB2D432BAF45C4E3D80` ;
- les trois fichiers ont le même SHA-256 de payload items décodé,
  `92033FFADD913366D45B0B6463A2ABFBE1DD4751321DE575FE3DAC945C0D6763`.
  Types, qualités, flags, IDs, placements, realm data, défense, durabilité et
  quantité 50 restent identiques ; chaque export éditeur sans modification est
  byte-exact avec son entrée ;
- le Tome quantité 50 ayant été accepté, sauvegardé et rechargé deux fois par
  D2R, aucun oracle stackable natif supplémentaire n'est requis pour fermer ce
  gate précis ;
- les quatre assertions frontend relevées pendant le témoin se reproduisent à
  l'identique avec `AAHEControl`, un Warlock frais de 922 octets et zéro item.
  Le log témoin vide porte le SHA-256
  `4474533FF5C4C2C292EF2F176C0D37FC21C8256B90C8A1D4C78551ABC80DA614` :
  elles appartiennent donc au baseline frontend BKVince observé et ne sont pas
  causées par `HEItemForge`.

Cette validation ne généralise pas encore la compatibilité à toutes les bases
ni aux qualités Magic, Set ou Unique. Elle ferme uniquement la verticale
effectivement implantée : records simples et records complexes Normal/Superior
avec leurs flags, placements, durabilité et quantité applicables.

La première verticale Magic et import/export est maintenant **validée en jeu**
sur le témoin portable `HEMagicIO.d2s` de 1 313 octets :

- le témoin contient les huit charms de départ BK, un `cbw` Magic éthéré importé
  depuis un `.d2i` natif et un Hand Axe Magic construit par l'éditeur. Le premier
  objet porte les affixes 418/237, la durabilité 28/28 et les stats poison 57 et
  mana after kill 138; le second porte les affixes 13/106 et les stats 0 et 17 ;
- la source canonique porte le SHA-256
  `336B3BE86DE7D2F6C59266297A308CAC0E04CAFE9948393B57EDFCF1FD4F60EE`.
  Deux cycles D2R 3.2.92777 complets produisent les SHA-256
  `7FCD109C7C65426A92574164ABF08BD2ED4C657ED5463C482270FC7BA08EFE5B`
  puis `D98A123FC6B3E3980DB293B217CC61379663E3D614956C8264C4511879B4148B` ;
- les trois fichiers conservent dix items et le même SHA-256 de payload items
  décodé, `9A4536B1762C2F4F8E325A2A1E92C26344F57CEEB8F34FE6C6F92A49B1AFBADB`.
  Types, qualités, affixes, valeurs, IDs, realm data, placements, durabilité et
  flags restent identiques; chaque no-op éditeur est byte-exact avec son entrée ;
- un premier passage a révélé que D2R remet les propriétés Magic en ordre d'ID
  croissant. L'éditeur canonise maintenant cet ordre avant l'écriture; le test
  automatisé couvre explicitement une saisie 17 puis 0 réécrite 0 puis 17 ;
- l'import `.d2i` régénère atomiquement l'ID et les quatre mots realm, refuse les
  octets traînants et planifie tous les placements avant mutation. Le bundle
  nommé ajoute le nombre d'items et le SHA-256 de l'ABI des tables BKVince.

Le compilateur gouverné des déclarations `mod1..3` est maintenant implanté. Il
résout `MagicPrefix.txt` et `MagicSuffix.txt` à travers `Properties.txt` et
`ItemStatCost.txt`, agrège uniquement les sorties compatibles partageant le même
ID et les mêmes paramètres, puis écrit les statistiques dans l'ordre canonique
croissant attendu par D2R. Les affixes sont appliqués explicitement en mode
minimum ou maximum; cette opération remplace les attributs Magic plats présents,
ce qui rend visible le risque de perdre un automagic ou des staffmods non
attribuables. `Undo` conserve la récupération immédiate de l'état précédent.

La couverture déterministe atteint désormais **567/567 préfixes** et
**607/607 suffixes** spawnable :

- les fonctions simples et groupées déjà gouvernées restent prises en charge;
- la fonction 10 encode exactement l'onglet de skills sous
  `[param % 3, floor(param / 3), roll]`;
- la fonction 11 encode les procs prouvés sous
  `[skill level, skill id, chance]`;
- la fonction 21 encode le bonus de classe, Warlock compris, sous
  `[Properties.val, roll]`;
- la fonction 22 encode l'oskill sous `[skill id, roll]`;
- la fonction 3 encode la seconde statistique des skills élémentaires avec son
  paramètre nul gouverné;
- la fonction 14 applique le nombre de sockets aux champs structurels du record,
  borné par `gemsockets` de la base et par les dimensions de l'objet;
- la fonction 19 calcule le niveau et les charges selon l'item level et le niveau
  requis du skill, avec charges courantes minimales ou maximales selon le mode.
  Set, Unique et l'insertion de fillers dans un objet socketed demeurent des
  gates distincts.

Le témoin étendu `HEMagicIO.d2s` ferme ces encodages paramétrés dans le vrai
profil `D2RLoader.exe -mod BKVince -txt` :

- la source de 1 442 octets porte le SHA-256
  `8AEEA48EA9B6F3186A2C7EF60A0F9E6CAEF2E9A9EF9FB00BCD01582E8EA2C907`
  et contient treize items, dont les huit charms BK et quatre nouveaux témoins
  couvrant les fonctions 10, 11, 21 et 22;
- les deux cycles load / `Save and Exit` produisent
  `7D0AC32994CE73BEA6645A636F156A91D96067A5E33671D9340AB4D83288132B`
  puis
  `57FD7A6DFF102A823FC43DE98C5D0D6EA0F34303DE5C5F079ACB776BE6D7FA6C`;
- les trois fichiers conservent treize items et le même payload décodé
  `C5CFADA6E62EE1D7C1896A707346BF4090C4EAF915096BCF96E9C9D285B1EB42`;
  les deux sorties repassent un export éditeur no-op byte-exact;
- l'observation en jeu affiche notamment `Jagged Hand Axe of Might` avec ses
  propriétés compilées et les huit charms BK dans la colonne gelée droite;
- le log frais porte le SHA-256
  `0F196CDB8E6CBAA6CA2DC5641CD33204722D8D7A1B01E892FB74C2849B64BFFC`.
  Il confirme le build 3.2.92777, 18/18 patchsets, 12 plugins actifs, un doublon
  global neutralisé, zéro rejet et zéro échec. Les assertions Items frontend et
  le doublon `Act5-Rifts` restent le bruit baseline BKVince déjà isolé.

Les surfaces demandées **Quests**, **Waypoints** et **Skills** sont également
devenues fonctionnelles dans le shell RuneWizard : 81 états de quête et 117 bits
de waypoint sont éditables sur les trois difficultés; les 240 skills des huit
classes sont générés depuis les coordonnées réelles `SkillPage`, `SkillRow` et
`SkillColumn`, avec les trois arbres Warlock, les prérequis, les niveaux requis,
les points disponibles et le mode `Ignore Game Rules`. Un test dédié modifie une
quête, le marqueur spécial `consumed_scroll` de Prison of Ice, un waypoint et un
rank Warlock, écrit le D2S, le reparse puis confirme un second export no-op
byte-exact. Ce test a révélé un décalage de quatre octets dans l'écriture du
marqueur `act_v.introduced` par `@d2runewizard/d2s`; le patch versionné place
maintenant ce marqueur au même offset que son lecteur. La suite passe **22/22**
et la build Vite de production est verte.

Le gate runtime combiné est fermé avec `HEWorldState.d2s` dans le profil exact
`D2RLoader.exe -mod BKVince -txt` :

- le Warlock niveau 30 est reconnu par l'écran BK Diablo: Ascension v16.0 puis
  chargé dans le camp de l'Acte I;
- le témoin contient Den of Evil complétée, Prison of Ice complétée avec
  `consumed_scroll`, Cold Plains, Arcane Sanctuary et Frigid Highlands activés,
  les skills 373/374/395 aux rangs 3/2/4, onze points de skill inutilisés et les
  huit charms de départ BK;
- la source de 1 227 octets porte
  `169C55FB05DF6604902403AEEB9F45C51C9A151EFF612512063B9BC8D31FFB93`;
  les deux sorties D2R portent respectivement
  `065253E8DC5B1D1B6B39410AE399056F2128E0FF69AA870DACA964B9DDBFEF06`
  et `AC82B0978390ADAF31598B176928C28619748C378B09B29D5C9C29188AF5C479`;
- malgré les champs runtime renouvelés, les deux sorties conservent exactement
  le même état gouverné SHA-256
  `D16D7B5DBE472D196B20FC1857FA6657AAEFD09AB2EFEDD5B5AAC421651E92A2`,
  huit items et un export éditeur no-op byte-exact;
- le log frais porte
  `6FA56183F0BC640741135171ACF03D93A56BC394CE288ED96FB08E1866149231` et
  confirme BKVince, le build 3.2.92777, 18/18 patchsets appliqués, 12 plugins
  actifs, un doublon neutralisé, zéro rejet et zéro échec. Les assertions Items,
  le doublon `Act5-Rifts` et le bruit CASC de fermeture restent visibles dans le
  log et ne sont pas attribués au témoin.

Le gate des derniers affixes Magic est fermé avec `HEAffixOracle.d2s` dans ce
même profil exact :

- la source de 1 620 octets porte
  `2585C9C1AFB01E8EC1390D0E5F4EB80213CC00276479A20D9B2C4995CA1B3200`
  et contient dix objets gouvernés en plus des huit charms BK;
- cinq Grand Charms portent Sparking, Chilling, Burning, Foul et White; un Short
  Staff et une amulette portent les deux suffixes Teleport; une Hand Axe, une
  Crown et une Gothic Plate portent Mechanic, Artisan et Jeweler;
- l'observation directe en jeu affiche `+1 to Lightning Skills`,
  `Level 6 Teleport (52/52 Charges)` et `Socketed (2)` sur les objets attendus;
- les deux sorties D2R de 1 625 octets portent respectivement
  `7364E769AE633DFC172C662812CA7427DEB2865DE307B80AE1D8459965D7FCA3`
  et `75E40959641E251AB2652487E825D1EB4B2E598EB16BE6B32C09A54771577305`;
- les dix objets conservent exactement le même état gouverné SHA-256
  `E396066CE17F32E482B9EA3D88864CFA9A2EB45A0AC7D331D6882A131C6B965C`
  après les deux cycles et chaque sortie reste no-op byte-exacte;
- le log frais porte
  `1972000FA8EC8D0CC3F5CC37B3A7799162C6D2B4F5D18FFC998FC33D31E464CD`.
  Il confirme le build 3.2.92777, 18/18 patchsets, 12 plugins actifs, zéro rejet
  et zéro échec; les quatre assertions Items déjà reproduites par
  `AAHEControl`, le doublon `Act5-Rifts` et le bruit CASC restent le baseline
  documenté. La suite locale passe **29/29** et la build Vite reste verte.

Le gate Set/Unique est maintenant fermé pour tout le catalogue représentable :

- le compilateur reconstruit **215/215 objets Set** et **473/473 objets Unique**
  sélectionnables, aux valeurs minimales et maximales;
- les IDs Unique `413` et `416` remplacent uniquement leurs placeholders
  absents de `Properties.txt` par un choix gouverné et mutuellement exclusif :
  trois arbres Warlock pour Wraithstep et six bonus de dégâts pour Opalvein;
- `HENamedItems.d2s` conserve deux objets Set et Annihilus pendant deux cycles
  D2R avec le SHA-256 sémantique gouverné
  `F21D6B5B4426211E972E22A2BBA5FA41BCA258A1FB91A4C70BD6E0C09EA5386C`;
- le témoin corrigé `HENamedEdges.d2s` couvre Witherstring, Visceratuant, Tomb
  Reaver et Titan's Echo. Sa source de 1 445 octets porte
  `9E98422922FC628B72C37B5ECA4B2F902EAB5AA20A2B2D5B1C45067C65360F33`;
  ses deux sorties D2R de 1 445 octets portent
  `01432F1D98E2B93F04342128D2380A41E315B54B47BA4132E6675BD83508DE8B`
  et `CE15282523FF7946C4329D523A5B480D268B5919BF3E58961184FDDB124B1167`;
- les deux sorties conservent les quatre objets gouvernés, les huit charms BK et
  exactement le même SHA-256 sémantique
  `B23644BD8E406BAACDDEE9E0606C95BFAFEAEEF37A5DA884459785EE45684718`;
  leurs exports éditeur no-op sont byte-exacts;
- l'observation directe a confirmé les tooltips de Witherstring, Visceratuant,
  Tomb Reaver et Titan's Echo. Elle a aussi prouvé que D2R canonise la durée de
  poison en stat 59 autonome et avait ramené la socket demandée par Static
  Accumulator à `Socketed (0)` dans cet ancien témoin. Le payload actuel accorde
  explicitement un socket à cet Unique, mais sa confirmation gameplay exige un
  nouveau cycle D2R;
- le log frais porte
  `5CA711437ED66E6CFD957EB650B2B7D37253986E108FF2DD41FA444A28B1DF72`.
  Il confirme le lancement exact `-mod BKVince -txt -offline`, le build
  3.2.92777, 18/18 patchsets appliqués, 12 plugins actifs, zéro rejet et zéro
  échec. Il contient les mêmes cinq IDs d'assertion du baseline documenté et
  aucun nouvel ID; le bruit CASC de fermeture reste visible.

Le gate des listes Set et des fillers socketed est maintenant fermé :

- le générateur conserve les propriétés `aprop` de `add func=0` comme bonus
  permanents et compile les cinq listes partielles de `add func=1/2` dans les
  bits 0 à 4 du masque Set;
- les masques non contigus `00100` de l'objet Set 77 et `10101` de l'objet Set
  136 sont reconstruits et reparsés exactement;
- la modale RuneWizard expose chaque liste Set disponible, ses valeurs minimales
  ou maximales et sa suppression;
- runes, gems et jewels peuvent être insérés comme sous-records `sock`. Le codec
  refuse atomiquement une capacité dépassée, préserve leur ordre canonique,
  attribue des identifiants uniques et extrait le dernier filler comme objet
  racine;
- `HESetSockets.d2s` porte les deux masques Set, un Hand Axe à deux sockets
  remplies par `r01` et `jew`, puis les huit charms BK. `HESockExtract.d2s`
  conserve `r01` dans l'arme et place le Jewel extrait à la racine de
  l'inventaire avec les huit charms BK;
- les deux témoins ont subi deux cycles complets avec
  `D2RLoader.exe -mod BKVince -txt -offline`. Leurs SHA-256 sémantiques restent
  respectivement
  `DCF0333ADAFEA108FA0E172AFB95264229D8EB9E3E658B394EF1D5BA645FEFE0` et
  `78673BB692E845D8BA5F0B346356F311FC729E2B24EE811AD92296D9597752E5`;
  les quatre sorties D2R de 1 433 octets restent no-op byte-exactes dans
  l'éditeur;
- le log frais porte
  `DEDCDF9A5966AFE1C14503F493E37F666AC13E370FEE871CC296C41D020EFF4F`.
  Il confirme BKVince, le build 3.2.92777, 18/18 patchsets, 12 plugins actifs,
  zéro rejet et zéro échec, sans nouvel ID d'assertion par rapport aux cinq du
  baseline documenté.

Le gate import complexe/compaction socket est fermé sur `HESockCompact.d2s` :

- la modale accepte directement un ou plusieurs `.d2i` ou un bundle
  `.bkitems.json` dans les sockets du parent sélectionné;
- l'import est atomique : capacité dépassée, item non-`sock`, filler imbriqué ou
  record non canonique rejette toute la sélection sans mutation;
- les qualités complexes, propriétés, IDs et realm data sont clonés sans
  collision. L'extraction vise maintenant n'importe quel filler et renumérote
  tous les suivants à partir de zéro dans leur ordre D2S canonique;
- le test dédié importe un Jewel Magic `+7 Strength` et `r02` après `r01`, refuse
  un Hand Axe mélangé au groupe sans mutation, extrait le Jewel central, puis
  reparcourt exactement `r01:0,r02:1`. La suite passe **29/29** et la build Vite
  reste verte;
- la source `HESockCompact.d2s` de 1 352 octets porte
  `1762AC5EEE710CF192AC9E5382C0690DC0F023477FFBAED761B15954BE43E4EE`;
- deux cycles `Save and Exit` avec le profil exact
  `D2RLoader.exe -mod BKVince -txt -offline` produisent deux sorties de 1 355
  octets portant
  `D5E80B359793E08379FEC4389FAF4C079689C2F6B1653EE4D3984AD257C052FE`
  puis `DF4C92EB0B19B4180ABDB167A6FFAA251336D9540C6A2839170CABE01244C259`;
- les deux sorties conservent le Jewel Magic dans l'Inventory, les fillers
  `r01/r02` aux positions 0/1 du Gothic Shield, les huit charms BK et le même
  SHA-256 sémantique
  `DD908F00FF6B8EDC316017D860FFBA675ACA7EFDC02C23D5051D4A6B4B94240D`;
  chaque export éditeur no-op reste byte-exact;
- l'observation visuelle confirme le Jewel extrait en haut à gauche de
  l'Inventory, les huit charms BK dans la colonne gelée droite et le Gothic
  Shield dans la stash personnelle;
- le log frais porte
  `870677B155848325F5DAD56B8553D4B06510EC525DB907090EF0F38D1B8800E1`.
  Il confirme BKVince, le build 3.2.92777, 18/18 patchsets, 12 plugins actifs,
  zéro rejet, zéro échec et 24/24 étapes startup. Il ne contient aucune erreur
  hors les cinq IDs d'assertion du baseline déjà documenté; le bruit CASC reste
  visible.

## Gate runewords et propriétés manuelles fermé

Le générateur byte-exact lit maintenant `Runes.txt` et produit 112 recettes
actives avec leurs IDs D2S, ordre de runes, types autorisés/interdits et neuf
groupes de propriétés. Le codec :

- compile **112/112 runewords** aux valeurs minimales et maximales ;
- exige une qualité Normal/Superior, une base compatible, la structure socketed
  et l'ordre exact des fillers, sans jamais écraser une socket occupée ;
- écrit ou retire atomiquement `given_runeword`, `runeword_id`, le nom et les
  propriétés, et brise automatiquement l'identité après une modification
  structurelle ou l'extraction d'un filler ;
- encode `Chaos` sans modifier `Runes.txt` ni l'ABI d'`ItemStatCost.txt` : la
  déclaration `rep-dur=100` est bornée à une seule occurrence native sûre du
  stat 252 à `63`; D2R réapplique la recette gouvernée en gameplay ;
- reconnaît les oracles natifs Spirit 155, Exile 63 et Dream 55 dans
  `QtyTester.d2s` et les nomme correctement dans les tooltips.

La modale possède aussi un éditeur sémantique de `Properties.txt` pour les
objets Magic, Set et Unique. Recherche, paramètre, minimum, maximum et modes de
roll produisent les groupes D2S gouvernés, y compris dégâts élémentaires,
oskills, sockets et états structurels. Les hints et bornes proviennent des TXT
BKVince plutôt que d'une liste écrite à la main.

La suite locale passe **31/31** et la build Vite est verte. Le parcours navigateur
confirme Spirit, Exile et Dream, la construction minimum/maximum, l'annulation,
la rupture d'un runeword et l'ajout de `Damage Reduced by #%` à 52, sans erreur
console.

Le témoin `HERuneword.d2s` contient les huit charms BK et un Spirit Monarch créé
par l'éditeur. Après plusieurs cycles D2R puis une passe avec ce héros seul :

- D2RLoader démarre explicitement avec `-mod BKVince -txt -offline`, termine
  24/24 étapes, applique 18/18 patchsets et active 12 plugins sans rejet ni
  échec ;
- le héros entre dans le camp, le Spirit est visible dans la stash personnelle,
  puis `Save and Exit` le conserve ;
- la sortie finale de 1 388 octets porte
  `E36923DFE1D93AB9ECB59C9174C7EC62EBB8D87D86A243616BFFC32510695832`,
  garde l'ID 155, `r07/r10/r09/r11` et les sept groupes de propriétés, et son
  export éditeur no-op reste byte-exact ;
- le log isolé porte
  `374484DF7AEF9E9F2CEBA4157A917C5724B6D8B7CFE41A59C326D193F00BCBEC`.
  Il ne contient aucune assertion `Items.cpp`; l'unique erreur est le doublon
  BKVince `Act5-Rifts`, indépendant du fichier de personnage ;
- les 19 autres sauvegardes temporairement isolées ont toutes été restaurées,
  pour un total final de 20 fichiers `.d2s` dans le profil BKVince.

## Gate Item Bonuses et mercenaire existant fermé

Les deux surfaces RuneWizard sont maintenant fonctionnelles :

- `Item Bonuses` calcule en direct les statistiques des objets effectivement
  équipés, suit leurs déplacements et éditions non sauvegardées, regroupe les
  attributs Magic, runeword et fillers socketed comme RuneWizard, puis permet
  d'ouvrir chaque objet source dans la modale partagée ;
- le générateur byte-exact consomme `Hireling.txt` et produit 33 définitions
  BKVince avec type, acte, difficulté, plage de niveau, Name ID et compétences ;
- l'éditeur du mercenaire réécrit les champs natifs `dead`, ID, Name ID, type et
  expérience, affiche les compétences gouvernées et ouvre ses objets dans le
  même Item Editor que l'équipement joueur ;
- les items mercenaire vivent dans leur bloc `jf` dédié. Le validateur refuse
  les types absents du `Hireling.txt` courant, les en-têtes incohérents, les IDs
  dupliqués entre joueur et mercenaire et les slots équipés en collision ;
- la création d'un mercenaire absent et l'extraction de socket sans destination
  d'inventory restent verrouillées fail-closed jusqu'à leur propre preuve.

La suite passe **34/34** et la build Vite est verte. Dans le parcours navigateur
sur `QtyTester.d2s`, `Item Bonuses` affiche trois objets sources et douze groupes;
la surface Mercenary retrouve Lionheart, le ring natif et treize groupes, puis
conserve une édition d'objet et les champs mercenaire dans l'historique undo.

Le témoin runtime `HEMercenary.d2s` ferme le gate existant :

- sa source est dérivée de `QtyTester` sans écraser ce dernier; elle rend le
  mercenaire vivant, conserve le type 0, place le Name ID à 2 et incrémente
  l'expérience à 114 097 217 ;
- le lancement exact `D2RLoader.exe -mod BKVince -txt -offline` montre uniquement
  le témoin isolé, le charge, puis affiche Anor niveau 98 dans le panneau natif
  avec Lionheart au torse et `Triumphant Ring of Nova Shield` au slot étendu 6 ;
- `Save and Exit` réussit. La sortie finale de 3 166 octets porte
  `BC538674C25FDA25D3870C06C66EB90CF8B009A0D4D872A9B477B29D6A9C8723`,
  conserve l'en-tête et les deux items, et son export éditeur no-op reste
  byte-exact ;
- le log `06A404D721723837825EC3FBFD3CB8AA9F2A466DE00F11E0EFB63850A5DF782C`
  confirme BKVince, le build 3.2.92777, 24/24 étapes startup, 18/18 patchsets et
  12 plugins actifs, sans rejet ni échec ;
- l'ouverture du panneau natif révèle deux assertions non bloquantes reliées au
  `SkillDesc` absent du skill BKVince 443. Le héros et sa mercenaire restent
  jouables et sauvegardables; ce diagnostic de données est conservé séparément
  du gate de sérialisation ;
- les 20 sauvegardes originales isolées ont été restaurées. Le profil contient
  maintenant 21 `.d2s`, incluant le nouveau témoin, et le dossier temporaire a
  été supprimé.

## Gate ajout, déplacement et retrait d'équipement mercenaire fermé

La verticale RuneWizard des slots mercenaires est maintenant complète pour un
mercenaire déjà présent :

- le générateur résout `Body`, `BodyLoc1` et `BodyLoc2` depuis `ItemTypes.txt`
  et rattache leurs emplacements gouvernés aux 800 bases du catalogue ;
- les douze slots D2S natifs sont mappés explicitement à `head`, `neck`, `tors`,
  `rarm`, `larm`, `rrin`, `lrin`, `belt`, `feet` et `glov`, y compris les deux
  jeux d'armes ;
- un slot vide ouvre la même modale click-to-add que les grilles joueur, mais
  ne propose que les bases compatibles. Un `.d2i` compatible peut aussi être
  importé directement, un item peut être déplacé par drag-and-drop et le bouton
  rouge `Delete` retire réellement son record `jf` ;
- toutes ces mutations participent au même historique Undo/Redo que le reste de
  l'éditeur. La suppression d'un record original est compactée avant écriture et
  prouvée par write/reparse/no-op plutôt que masquée seulement dans l'UI ;
- le validateur refuse les collisions de slot, les `BodyLoc` incompatibles, les
  records hors bloc équipé, les IDs imbriqués réutilisés et plusieurs imports
  atomiques vers un seul slot.

Le test mercenaire couvre ajout d'un Cap, ajout et déplacement d'un ring,
import d'une amulette, refus d'un slot occupé, refus ring→torse, refus d'un
multi-import, export/reparse, suppression d'un record original et restauration
Undo. La suite passe **34/34** et la build Vite reste verte. Le parcours
navigateur sur le vrai `HEMercenary.d2s` montre 58 bases compatibles pour le
slot tête, ajoute le Cap, retire puis restaure le ring et valide le téléchargement
après checksum, taille et reparse.

Le témoin runtime `HEMercAdd.d2s` ferme le gate :

- il est dérivé de `HEMercenary` sans l'écraser, conserve `scl` au torse 3 et
  `rin` au ring droit 6, puis ajoute `cap` à la tête 1 ;
- le processus exact est
  `D2RLoader.exe -mod BKVince -txt -offline`. Le log monte explicitement
  `mod="BKVince"`, termine 24/24 sur le build 3.2.92777, applique 18/18
  patchsets et active 12 plugins sans rejet ni échec ;
- le branding visible `BKDiablo: Ascension v16.0` reste celui du squelette
  historique de BKVince. Les difficultés Pain/Agony/Insanity, le chemin de save
  BKVince et le montage du log distinguent sans ambiguïté le profil actif ;
- le héros entre en Pain avec Anor vivante niveau 98. Le panneau mercenaire
  natif affiche le Cap à la tête, Lionheart au torse et le ring dans le slot
  droit, puis `Save and Exit` réussit ;
- la sortie finale de 3 202 octets porte
  `C2834549FC805FCE1C51AE4928A7018F120D21EAA90F56F70ACF881C32D97536`.
  Le codec reparcourt `cap:1`, `scl:3` et `rin:6`, conserve le mercenaire vivant
  et produit un export no-op byte-exact ;
- le log frais porte
  `B09BE9C657AE5C950578EEEC40954A27AE3D5678187ABD52B7BA626491AF87A3`.
  Il reproduit les assertions BKVince déjà connues `Act5-Rifts`, niveau et
  `SkillDesc` 443; aucune n'empêche le chargement, l'affichage des trois items
  ou la sauvegarde ;
- les 21 sauvegardes originales ont toutes été restaurées. `HEMercAdd` et ses
  quatre auxiliaires de carte sont conservés uniquement sous
  `analysis-cache/hero-editor-runtime/postgame/`; le dossier d'isolation et son
  marqueur ont été supprimés après contrôle qu'ils étaient vides.

## Gate création d'un mercenaire absent fermé

Le bouton `Create mercenary` produit maintenant un vrai header D2S plutôt qu'un
état d'interface fictif :

- le catalogue `Hireling.txt` gouverne aussi `Exp/Lvl`. Le seuil d'expérience
  natif est calculé par `level² × (level + 1) × Exp/Lvl`; le défaut type 0 donne
  donc une Rogue Acte I niveau 3 avec 3 600 XP ;
- `createMercenarySnapshot` exige un save sans mercenaire ni record `jf`
  orphelin, choisit Name ID 0, type 0, état vivant et un ID hexadécimal non nul
  déterministe pour le snapshot. Tout type absent de la table ou ID nul est
  refusé avant écriture ;
- `removeMercenarySnapshot` remet les cinq champs natifs à zéro et marque tous
  les items mercenaire pour retrait. L'historique Undo restaure le header et
  l'équipement ensemble ;
- le navigateur a couvert création, affichage des champs, retrait et Undo sur
  un Warlock vierge. La suite passe **35/35** et le build Vite reste vert.

Le témoin runtime `HEMercCreate.d2s` ferme la matrice BKVince :

- la source de 1 229 octets porte
  `348A229514CCD984705007408F6BEB8DEE8E60D9CFE87A1F649B880C09DFC840`,
  avec ID `965e4ea2`, Name ID 0, type 0, 3 600 XP et aucun item mercenaire ;
- le processus exact `D2RLoader.exe -mod BKVince -txt -offline` monte
  `mod="BKVince"` sur 3.2.92777, applique 18/18 patchsets, active 12 plugins sans
  rejet ni échec et atteint 24/24 ;
- le personnage entre en jeu avec **Aliza vivante**. Le panneau natif affiche
  `Level 3 Rogue`, `3,600 of 8,000`, 45/45 vie et les slots vides attendus ;
- après `Save and Exit`, le fichier conserve les mêmes cinq champs, reste à
  1 229 octets, porte
  `657F7339184E2157AFD8B9AC5313E7056CAAED506C7318FAAC5BBFE16CC1F779`
  et produit un no-op byte-exact ;
- le log frais porte
  `30F02F855D43177AEF230E29CC2A048482B4110F3CD73F040240B9B9A549B2B8`.
  Les assertions BKVince déjà connues `Act5-Rifts`, niveau et `SkillDesc` 443
  sont reproduites sans bloquer le chargement ou la sauvegarde ;
- les 21 sauvegardes originales sont restaurées. Le témoin et ses quatre
  auxiliaires sont archivés sous
  `analysis-cache/hero-editor-runtime/mercenary-create/postgame-20260804-042054/`.

## Gate Chronicle / Shared Stash fermé

La surface Chronicle édite maintenant le secteur dédié du vrai
`ModernSharedStashSoftCoreV2.d2i`, sans confondre cet état avec le D2S du
personnage :

- le scanner valide chaque enveloppe `0xAA55AA55`, la version 105, la taille
  déclarée et la signature du payload. Il parcourt jusqu'à 4 096 secteurs
  et exige un unique Chronicle `0xC0EAEDC0` final de format 1 ;
- les pages ordinaires, dont la page stackable, sont conservées byte-exactes.
  Le codec ne reprend pas la limite historique de six pages : un test synthétique
  ouvre et réexporte **1 001 pages**, comme le profil runtime BKVince actuel ;
- les records Set, Unique et Runeword conservent l'item ID 32 bits, le Monster
  ID 16 bits et l'horodatage natif en minutes. Les IDs inconnus restent visibles
  et exportables; les mappings spéciaux de runewords sont traités explicitement ;
- si aucun champ ne change, l'export restitue les octets source. Après édition,
  seul le secteur Chronicle est reconstruit, puis le fichier entier est reparsé,
  comparé au snapshot canonique et les pages précédentes sont revérifiées
  byte-exactes ;
- l'interface RuneWizard expose Load/Replace/Download Shared Stash, les onglets
  Set/Unique/Runeword, recherche, ajout individuel ou catégorie complète,
  Monster ID, date, retrait et historique Undo/Redo ;
- sans fichier chargé, la composition est strictement ramenée à `Chronicle`
  puis `Load Shared Stash`, comme la référence auditée. L'emblème, la grande
  carte vide et la copie technique D2I n'apparaissent plus avant le chargement ;
- le témoin gouverné de 680 octets ouvre six pages et trois uniques
  (`Titan's Echo`, `Static Accumulator`, `Gravepalm`). Le navigateur local a
  ajouté puis annulé `The Gnasher`, modifié puis annulé le Monster ID de
  `Gravepalm`, et n'a produit aucune erreur console ;
- la suite passe **39/39** et `npm.cmd --workspace apps/hero-editor run build`
  est vert. L'unique avertissement reste la taille du chunk Vite déjà connue.

## Gate Demon codec et interface fermé — preuve runtime ouverte

La surface Bound Demon reproduit maintenant le périmètre public de RuneWizard
sans inventer le payload natif lorsque la sauvegarde n'en contient pas :

- le générateur lit `monstats.txt`, `superuniques.txt` et `monumod.txt` avec le
  round-trip TSV byte-exact du workspace, puis produit 799 monstres, 70 Super
  Uniques et 45 modificateurs gouvernés avec leurs index natifs de ligne ;
- l'interface expose `Terrorized`, `Super Unique`, la recherche et la sélection
  d'identité, puis les six premiers slots de mods dans le shell RuneWizard ;
- les IDs inconnus restent sélectionnables et préservés. Les slots 7–9 ne sont
  pas exposés, conformément à la surface de référence ;
- le codec refuse de créer ou de supprimer la présence du bloc `lf`. Il exige
  les neuf octets de mods, les sept blocs opaques de longueurs exactes et les
  champs bornés avant toute écriture ;
- après export, le démon reparsé est comparé intégralement au modèle écrit :
  difficultés, zone, niveau, stats terminales, blocs opaques et slots cachés
  doivent rester identiques ;
- un témoin synthétique v105 ferme write/reparse, no-op byte-exact, édition et
  refus de fabrication. Le navigateur ouvre l'état vide fidèle à RuneWizard,
  puis édite Rakanishu, Terrorized, Super Unique et Spectral Hit avec Undo/Redo ;
- la suite passe **42/42** et `npm.cmd --workspace apps/hero-editor run build`
  est vert. L'avertissement de taille du chunk Vite reste le seul avertissement.

Aucune sauvegarde locale BKVince auditée ne contient encore de Bound Demon réel.
Le gate gameplay reste donc explicitement ouvert jusqu'à capture d'un bloc créé
par le jeu, puis load/edit/export/Save and Exit/reparse sur le profil exact
`-mod BKVince -txt -offline`.

## Gate Shared Stash items et runtime fermé — compteur stackable fermé

La première tranche verticale des objets de Shared Stash est implantée sans
affaiblir Chronicle :

- chaque secteur de page est hydraté séparément avec le codec v105. Les pages
  ordinaires utilisent la vraie grille BKVince 16×13 et les placements natifs
  `location=0`, `alt_position_id=5` ;
- click-to-add, import individuel ou groupé, tooltip, Item Editor complet,
  suppression, déplacement, sockets, runewords, propriétés et Undo/Redo
  réutilisent les mêmes opérations gouvernées que le D2S ;
- l'export ne reconstruit que les secteurs logiques modifiés. Les headers de
  64 octets sont conservés, la taille et le compteur `JM` sont actualisés, puis
  toutes les pages et Chronicle sont reparsés et comparés ;
- chaque page non modifiée reste byte-exacte même si une autre page et Chronicle
  changent simultanément. La qualité dérivée `item_quality`, ajoutée par
  l'enrichisseur du codec, est maintenant exclue correctement de la comparaison
  des payloads sources ;
- la page stackable conserve ses coordonnées superposées, son identité et ses
  payloads en lecture seule. Seul son compteur natif 8 bits est éditable de 0 à
  255 après preuve croisée du champ `chest_stackable=1`, du read/write `UInt8`
  du codec et des fichiers BKVince gouverné/runtime ;
- le navigateur charge le fixture à six pages, ajoute un Hand Axe sur la page 1,
  ouvre l'Item Editor, inverse `Identified`, vérifie Undo puis Redo et affiche les
  neuf records de la page stackable 6 ;
- la suite passe **46/46** et `npm.cmd --workspace apps/hero-editor run build`
  est vert. L'unique avertissement reste la taille du chunk Vite déjà connue.

Le profil runtime actif comporte 1 001 secteurs de page, dont une page stackable.
La preuve gameplay du 4 août 2026 ferme maintenant la page ordinaire :

- le fichier actif original de 68 454 octets a été sauvegardé avec le SHA-256
  `4461974E7FF5FE5A5DBF85E8B87AD24E56EED8BDECFE04BC1B630E8599E78318` ;
- l'éditeur a ajouté un Hand Axe `hax`, item level 41, en `(0,0)` sur la page 1.
  La copie exportée de 68 489 octets porte le SHA-256
  `482EAECDA5B60BC77ED87ABB42E059CCE1DCA1192ADA3EA959FD580331500208`,
  se reparse sur 1 001 pages et reste byte-exacte au second export no-op ;
- la copie a été montée temporairement avec le témoin `HEStashRt` sur
  `D2RLoader.exe -mod BKVince -txt -offline`. En jeu, l'onglet Shared affichait
  `PAGE 1 / 1000` et le Hand Axe dans la première colonne sur trois lignes ;
- après `Save and Exit`, le stash conserve exactement son SHA-256 pré-game,
  les 1 001 secteurs sont identiques et le Hand Axe reste à `(0,0)`. Le héros
  conserve ses huit charms BK et son export no-op reste byte-exact ;
- le log frais `95E77351E537D0DD2D4386982C116E26FC806EA45BA0B0A2FBAF76043AE8F6EB`
  confirme BKVince, 3.2.92777, 18/18 patchsets, 12 plugins actifs, zéro rejet,
  zéro échec et 24/24 étapes startup. Les assertions Items et le bruit CASC sont
  le baseline BKVince déjà documenté ;
- le fichier actif a ensuite été restauré byte pour byte au SHA-256 original.
  Les cinq auxiliaires `HEStashRt` ont été archivés dans la preuve locale puis
  retirés du profil actif, et une seule instance BKVince a été relancée.

La preuve runtime du compteur stackable au-delà de quinze est maintenant fermée
sur une copie réversible du profil actif :

- l'original courant de 68 729 octets a été sauvegardé au SHA-256
  `EF3AAE6E727924AE8071466C23C470579A9EFA3E27F1F21094D7D68CEE4D77BF` ;
- l'éditeur a porté uniquement le compteur natif de l'Ohm Rune du secteur
  stackable à `21`. La copie de 68 729 octets porte le SHA-256
  `2431D702D4F77C4A732986A3CF2AFA1984088AE4B825475C9CAA0513C6B69E7B`,
  se rouvre avec `amount_in_shared_stash=21` et ferme un export no-op byte-exact ;
- BKVince 3.2 charge cette copie, ouvre la Shared Stash, puis `Save and Exit`
  réécrit le fichier au SHA-256
  `E6A69D97B7028566DC402381B8B5E495DE8CE6A59D0B5BEB361F8DFC8AA6C00C`.
  Le reparse post-runtime retrouve encore exactement l'Ohm Rune à `21` ;
- l'onglet spécial BKDiablo ne rend pas ce compteur sous forme de nombre
  conventionnel à la souris; la preuve retenue est donc chargement,
  resauvegarde et reparse natifs, pas une lecture OCR supposée ;
- l'original a enfin été restauré avec le même SHA-256 et la même taille, puis
  une seule instance `D2RLoader.exe -mod BKVince -txt -offline` a été relancée.

## Gate Virtual Stash fermé

La Virtual Stash suit la sémantique publique de RuneWizard sans prétendre à une
enveloppe native inexistante dans le D2S ou le Shared Stash :

- elle est un workspace temporaire de navigateur sur une grille BKVince 16×13 ;
- elle réutilise click-to-add, import individuel ou groupé, tooltip, Item Editor,
  déplacement, suppression, sockets, runewords, propriétés et Undo/Redo ;
- les IDs réservés couvrent simultanément le héros, le mercenaire, Shared Stash
  et Virtual Stash pour éviter une collision lors des imports et créations ;
- l'export `.bkitems.json` applique les edits et placements courants, exclut les
  items marqués supprimés et conserve le fingerprint ABI BKVince ;
- aucune action Virtual Stash n'écrit le D2S ou le `.d2i` physique. Le workspace
  disparaît au rechargement de la page s'il n'a pas été exporté ;
- un test ajoute deux rings, édite le premier, déplace le second, supprime le
  premier et reparse le bundle du record actif à sa position finale ;
- la suite passe **47/47** et `npm.cmd --workspace apps/hero-editor run build`
  est vert. L'avertissement de taille du chunk Vite reste le seul avertissement.

## Gate transferts atomiques et Trash fermé

Les workspaces d'items ont maintenant un cycle de déplacement récupérable :

- le codec construit et valide d'abord la destination, puis marque la source
  retirée. Un placement hors grille refuse toute l'opération et conserve les deux
  snapshots byte-for-byte ;
- chaque copie reçoit un ID neuf sans collision avec le héros, le mercenaire,
  Shared Stash, Virtual Stash ou Trash. Qualité Magic, état Identified, affixes et
  attributs numériques traversent Personal → Virtual → Trash → Personal avant
  write/reparse du D2S ;
- Personal Stash, la page Shared Stash ordinaire chargée, Virtual Stash et Trash
  sont des destinations explicites. La page stackable reste refusée ;
- un transfert produit une paire d'entrées d'historique liée. Undo et Redo depuis
  la source ou la destination inversent les deux workspaces ensemble ;
- Trash est une grille BKVince 16×13 de session. Le bouton rouge `Move to Trash`
  n'est affiché qu'une fois, les objets peuvent être restaurés, le workspace peut
  être exporté dans un bundle de récupération, et `Empty Trash` reste une unique
  action annulable ;
- le parcours navigateur sur `TransferUI` prouve les huit charms gelés en colonne
  11, lignes 1 à 8, puis Personal → Virtual, Undo/Redo composé, Virtual → Trash,
  Trash → Personal et Empty Trash → Undo. Les messages d'historique suivent le
  nouvel état au lieu de laisser une confirmation contradictoire ;
- la suite passe **48/48** et `npm.cmd --workspace apps/hero-editor run build`
  est vert. L'unique avertissement reste la taille du chunk Vite déjà connue.

## Première passe de fidélité visuelle RuneWizard fermée

La densité et les parcours centraux des captures de référence de Vincent ont été
resserrés sans modifier les dimensions natives des conteneurs :

- le shell utile est centré sur 760 px et la ligne de stockage mesure 720 px,
  contre 1 213 px avant cette passe. Equipment/Inventory, Stash et Cube/Belt
  occupent respectivement 264, 280 et 90 px ;
- les grilles conservent exactement Inventory 11×8, Stash 16×13, Cube 6×6 et
  Belt 4×4, mais utilisent des cellules visuelles de 22, 16, 12 et 19 px. Les
  huit charms BK restent verticalement dans la colonne gelée droite ;
- le header regroupe désormais les deux parcours sous `Load / create`. La même
  modale ouvre un D2S existant ou crée un héros vierge parmi les huit classes,
  sans personnage preset ni build prédéfini ;
- l'audit vivant recale cette modale à 800 px avec le titre RuneWizard `Load or
  Create a Character`, le bouton pointillé `Load Character from Saved Games
  Folder` et huit cartes portrait en grille 4×2. Un clic crée directement le
  héros sous son nom de classe; Name, Hardcore et Ladder restent dans General.
  La section `Load a premade build` est omise conformément à la décision de
  Vincent. Le navigateur confirme Amazon, Sorceress, Necromancer, Paladin,
  Barbarian, Druid, Assassin et Warlock, chacun avec les huit charms BK ;
- la modale `Choose items to add` est ramenée à 600 px. Son catalogue reste
  masqué jusqu'à une recherche ou un filtre, puis expose les 729 bases réellement
  spawnable, l'import individuel/groupé, le nombre de copies, l'item level et la
  quantité stackable. Le parcours navigateur ajoute atomiquement deux Hand Axes ;
- l'Item Editor mesure 700 px et reprend la lecture en deux colonnes des captures :
  contrôles à gauche, aperçu vivant à droite, footer fixe et `Save Changes`. Le
  passage Normal → Magic actualise l'aperçu immédiatement et Undo restitue Normal ;
- les cases vides portent l'action contextuelle `Click to add` ou `Place here`.
  Un objet placé conserve son tooltip détaillé et ouvre la même modale au clic ;
- le navigateur valide les 84 switches de Quests — 81 états plus les trois
  `consumed_scroll` —, les 117 waypoints et les 30 compétences Warlock sur les
  trois arbres Demon/Eldritch/Chaos. Une mutation suivie d'Undo a été vérifiée
  dans chacun des trois écrans ;
- la suite demeure **48/48**, le build Vite est vert, `git diff --check` ne relève
  aucune erreur et les validateurs mission, cadastre et workstreams sont verts.

## Gate papier-doll, sprites et forge groupée RuneWizard fermé

Le shell utilise maintenant directement les visuels gouvernés de BKVince sans
copier les assets de RuneWizard :

- `generate-ui-assets.mjs` décode le format SpA1 v31 présent dans le dépôt. Il
  produit le papier-doll 1112×714 depuis `classic_background.sprite`, 143 PNG
  distincts couvrant 149 codes d'items, et un manifeste déterministe consommé
  par React ;
- les douze zones d'équipement restent interactives sur le vrai panneau, et les
  huit charms initiaux `mff`, `mfc`, `mfd` utilisent leur art BK réel dans la
  colonne gelée droite. Les bases absentes de l'overlay BKVince conservent un
  fallback lisible plutôt que de recevoir un sprite inventé ;
- la modale `Choose items to add` expose les six raccourcis de la capture
  RuneWizard : Worldstone Shards, Uber Ancients Materials, Warlord's Glory Set,
  Cube, Organ Set et Key Set. Chaque ligne accepte quantité et retrait ; toute la
  sélection est planifiée avant mutation, donc une capacité insuffisante refuse
  le groupe entier ;
- les cinq pièces de Warlord's Glory sont créées comme de vrais records Set
  `127–131`, avec leurs propriétés minimales compilées depuis les tables
  BKVince. Les objets de quête désactivés pour le drop normal restent autorisés
  uniquement dans ces groupes gouvernés ; le catalogue libre demeure limité aux
  729 bases spawnable ;
- l'Item Editor reprend maintenant la rangée haute de la capture : quantité de
  duplications, `Make duplicates`, Ethereal, Identified et Personalized. La
  personnalisation écrit un nom ASCII natif de 1–15 caractères ; chaque copie
  reçoit de nouveaux IDs, conserve qualité, propriétés, sockets et nom, et la
  totalité de l'opération reste annulable en une action ;
- le parcours navigateur `GroupUI` ajoute les cinq pièces Warlord's Glory dans
  Personal Stash, ouvre Warlord's Conquest, écrit `RuffnecKk`, crée deux copies
  puis confirme trois boutons d'édition distincts. L'aperçu vivant affiche
  `Personalized by RuffnecKk` ;
- la suite passe **51/51** et `npm.cmd --workspace apps/hero-editor run build`
  est vert. L'avertissement de taille du chunk Vite reste le seul avertissement.

## Gate tiers Normal, Exceptional et Elite fermé

Les contrôles `Downgrade` et `Upgrade` de la référence RuneWizard pilotent
maintenant les vraies familles de bases BKVince :

- le catalogue généré expose `normcode`, `ubercode` et `ultracode` sur les 800
  bases, ainsi que `minac` et `maxac` pour les armures. Toute référence absente
  ou transition qui changerait la famille de stockage D2S fait échouer la
  génération ;
- les boutons suivent l'ordre Normal → Exceptional → Elite et se désactivent
  aux extrémités. Une transition refuse aussi une cible qui ne pourrait pas
  conserver le nombre de sockets ou le runeword courant ;
- qualité, Set/Unique ID, propriétés, listes de bonus Set, sockets, fillers,
  personnalisation et item level restent attachés au record. Les objets Set et
  Unique restent compatibles avec toute leur famille de tiers au lieu d'être
  verrouillés à leur seule base normale ;
- l'Item Editor expose les champs natifs `Item Level`, `Defense`, `Current
  Durability` et `Max Durability`. La défense est bornée par l'encodage D2S et
  les boutons la ramènent dans la plage gouvernée de la nouvelle base ; la
  durabilité conserve son ratio lors d'un changement de tier ;
- le test dédié monte Warlord's Conquest `hgl` → `xhg` → `uhg`, exporte et
  reparse le Set 127 personnalisé `RuffnecKk`, puis redescend jusqu'à `hgl`.
  Set ID, propriétés, nom, item level 99 et durabilité 24/24 survivent ;
- dans le navigateur, `GroupUI` reproduit le même parcours : défense 12 sur
  Gauntlets, 43 sur War Gauntlets, 62 sur Ogre Gauntlets, puis 53 au retour sur
  War Gauntlets. L'aperçu conserve Set, Strength 15 et Attack Rating 45 ;
- la suite passe **52/52** et le build Vite est vert. L'avertissement connu de
  taille du chunk demeure le seul avertissement.

## Gate couverture visuelle complète des items fermé

Le fallback textuel n'est plus nécessaire pour les 800 bases du catalogue :

- `extract-vanilla-ui-assets.ps1` lit la registry BKVince gouvernée, ouvre une
  seule fois le CASC de l'installation D2R de Vincent et extrait localement les
  sources SpA1 manquantes. Les 330 sprites vanilla obtenus restent sous
  `analysis-cache/`; aucune donnée brute du jeu n'est versionnée ;
- le générateur superpose les 171 sprites BKVince prioritaires aux sources
  vanilla, normalise les chemins historiques et produit 388 PNG distincts. Le
  manifeste final couvre 814 codes, soit les 800 bases ajoutables plus leurs
  alias gouvernés ;
- les deux seules bases sans code direct dans `items.json`, `mfg` et `rds`,
  réutilisent explicitement le sprite de leur asset partagé `invchm` et
  `invbsc`. Un test compare désormais tout le catalogue au manifeste et refuse
  la moindre base sans visuel ;
- Hand Axe, Cap, Gauntlets et le charm de récompense ont été inspectés dans les
  PNG générés. Le navigateur charge aussi Warlord's Conquest depuis
  `/ui/items/glove__gaunlets_h.png` dans l'Item Editor ;
- les tooltips des grilles choisissent maintenant leur ancrage horizontal et
  vertical selon la position réelle de l'objet. Sur petit écran, leur largeur
  est bornée au viewport; l'Item Editor passe en une colonne et empile ses
  champs à 520 px ;
- la suite reste **52/52**, le build Vite est vert et l'avertissement connu de
  taille du chunk demeure le seul avertissement.

## Gate icônes et prérequis des skills fermé

L'écran Skills ne repose plus sur des initiales ni sur des traits verticaux
génériques :

- le générateur lit `IconCel` dans le `SkillDesc.txt` BKVince avec le parseur
  TSV gouverné. Les 240 skills des huit classes doivent couvrir exactement les
  trente cellules paires `0..58` de leur atlas; une cellule impaire, absente ou
  hors plage fait échouer la génération ;
- `extract-vanilla-ui-assets.ps1` extrait les sept atlas vanilla depuis le CASC
  local sans versionner les sources brutes. L'atlas Warlock provient du sprite
  BKVince versionné. Les huit atlas PNG restent régénérables et préservés sur
  une machine qui ne possède que les sorties versionnées ;
- chaque nœud affiche sa frame réelle. La cellule paire est l'état sombre et la
  cellule impaire devient dorée dès que le rang est investi ;
- les liaisons SVG sont calculées depuis `reqskill1..3`, y compris une relation
  latérale sur une même rangée. Une flèche devient dorée lorsque sa source et sa
  cible possèdent toutes deux un rang ;
- le navigateur confirme les dix icônes et dix liaisons de Demon Skills. Avec
  `Ignore Game Rules`, Demonic Mastery et Summon Goatman passent au rang 1,
  utilisent les frames actives et activent leur liaison ;
- la suite passe **53/53**, le build Vite est vert et l'avertissement connu de
  taille du chunk demeure le seul avertissement.

## Gate navigation Stats et motifs Quests fermé

Les captures Quests et Waypoints de Vincent gouvernent maintenant une passe
visuelle et fonctionnelle supplémentaire :

- les huit entrées General, Quests, Waypoints, Item Bonuses, Skills, Chronicle,
  Mercenary et Demon utilisent chacune un pictogramme SVG RuffnecKk distinct à
  la place du losange générique ;
- les 27 quêtes portent un motif visuel explicite issu des sprites d'items
  BKVince déjà gouvernés. Le même motif apparaît sur les trois difficultés, soit
  81 icônes, et son contraste suit l'état complété ;
- le test de couverture refuse toute quête sans `iconCode`, sans entrée dans le
  manifeste des 800 bases ou sans PNG versionné ;
- `Unlock Hell` complète les journaux Normal et Nightmare par le chemin natif du
  codec, sans modifier Hell. L'export/reparse confirme les deux fins Act V et
  conserve `Hell / Den of Evil` incomplète ;
- dans le navigateur, les 81 icônes chargent sans erreur. `Unlock Hell` produit
  exactement 54 switches et 54 motifs actifs; Waypoints conserve ensuite ses
  117 switches individuels. La passe de fidélité finale retire sa note et ses
  actions globales, absentes de la capture de Vincent ;
- la suite passe **55/55**, le build Vite est vert et l'avertissement connu de
  taille du chunk demeure le seul avertissement.

## Gate fiche RuneWizard des items fermée

Les tooltips et l’aperçu vivant de l’Item Editor partagent désormais une même
fiche dérivée des constantes BKVince :

- `describeItem` enrichit une copie du record avec le moteur de description du
  codec. Le payload source et les octets destinés à l’export ne sont jamais
  mutés ; le cache est indexé par objet et niveau du personnage ;
- les attributs bruts `strength: 15` et `tohit: 45` deviennent les libellés
  Diablo `+15 to Strength` et `+45 to Attack Rating`. Les dégâts de base,
  exigences de force/dextérité/niveau, item level, quantité, états et sockets
  utilisent le même composant dans le hover et la modale ;
- le niveau requis combine la base, l’identité Magic/Set/Unique et les fillers
  socketés. Les sockets affichent capacité occupée et miniatures de leurs vrais
  sprites ; les fiches dépassant le viewport deviennent défilables ;
- les boutons d’items exposent maintenant `aria-describedby`. Un `Cancel
  selection` explicite sort du mode déplacement et restaure immédiatement
  `Click to add` sur les cellules vides ;
- dans le navigateur, Warlord’s Conquest affiche défense 12, durabilité 24/24,
  force requise 60, niveau requis 1, item level 99 et ses deux propriétés dans
  le tooltip puis à l’identique dans l’Item Editor. Monarch affiche force 156 et
  niveau requis 54. L’annulation de sélection permet ensuite un nouvel ajout ;
- les tests couvrent aussi Phase Blade 47–53, force 25, dextérité 136, niveau 54,
  ainsi que Spirit et ses quatre fillers `r07/r10/r09/r11`. La suite passe
  **56/56**, le build Vite est vert et l’avertissement connu de taille du chunk
  demeure le seul avertissement.

## Gate interactions clavier, focus et cellules occupées fermé

Les trois modales reposent désormais sur une primitive commune qui ferme un
écart fonctionnel observé dans le navigateur : avant ce lot, `Escape` depuis le
champ Character name laissait `Load / create` ouverte et le focus n’était pas
confiné au dialogue.

- `Escape` ferme maintenant `Load / create`, `Choose items to add` et l’Item
  Editor. Tab/Shift+Tab sont bouclés sur les contrôles visibles, le fond de
  l’application devient `inert` et `body` reçoit `overflow: hidden` pendant
  l’ouverture ; les deux états sont restaurés au démontage ;
- le focus revient à la cible exacte qui a ouvert la modale. Le navigateur le
  confirme successivement sur le bouton `Load / create`, la case libre
  `Inventory column 9, row 2` et le bouton `Edit Warlord's Conquest` ;
- hors dialogue, `Escape` annule le mode déplacement sans modifier la save.
  Les raccourcis `Ctrl+Z`, `Ctrl+Shift+Z` et `Ctrl+Y` appliquent Undo/Redo à
  l’historique actif et sont exposés par `aria-keyshortcuts` ;
- les cellules recouvertes par un item ne rendent plus de bouton invisible
  `Add item` ou `Place selected item`. Le parcours confirme zéro fausse action
  sous la cellule du Blank Charm gelé en colonne 11, tout en conservant les six
  vrais boutons `Edit Blank Charm` ;
- les modales sont bornées en unités `dvh`, deviennent plein écran sous 520 px,
  conservent un footer compatible avec la safe area et empilent les actions de
  sélection. Sur périphérique tactile grossier, le clic ouvre directement
  l’éditeur sans tooltip hover persistant ;
- le build régénère toujours 800 bases, 388 PNG/814 codes et huit atlas. La
  suite reste **56/56**, le build Vite est vert et le diff-check ciblé ne
  remonte aucune erreur.

## Gate contrôles fins RuneWizard de l’Item Editor fermé

Une comparaison directe avec les captures de Vincent ferme les écarts restants
dans la partie propriétés et sockets de la modale :

- la première rangée suit désormais l’ordre visuel `Quality` puis `Base item`.
  Les champs Quantity et Runeword sont masqués tant que le record ne peut pas
  réellement les utiliser, ce qui réduit le bruit vertical ;
- les Item Attributes sont résumés par leur texte Diablo violet. Chaque ligne
  possède un crayon qui déploie uniquement ses valeurs encodables et une
  corbeille. L’ajout est replié derrière `Add magic attributes…`, puis propose
  le choix rapide des stats numériques ou le compilateur gouverné
  Properties.txt ;
- les listes Set actives ne montrent plus seulement le code `swing3` : leur
  texte enrichi apparaît en vert dans la ligne et dans l’aperçu vivant. Le test
  couvre aussi Poison Resist +25%, +15 Vitality et Physical Damage Received
  Reduced by 15% sur un masque non contigu ;
- `Sockets` devient un sélecteur borné par la capacité de la base et le nombre
  de fillers. Chaque filler affiche son PNG réel dans un cercle vert, avec
  `Extract` et une corbeille comme dans la référence ;
- `removeSocketFillerSnapshot` supprime uniquement le sous-record ciblé,
  compacte les positions restantes, invalide automatiquement un runeword si sa
  structure a changé et laisse Undo restaurer l’opération. Le test exporte et
  reparse insert → delete → extract sans créer de record racine au delete ;
- le parcours navigateur modifie Warlord’s Conquest de +15 à +52 Strength,
  ajoute +20 Dexterity avec ouverture immédiate de son éditeur, active le bonus
  Set +20% Increased Attack Speed et le voit dans l’aperçu. Une Phase Blade est
  réglée à trois sockets, reçoit une El Rune avec son sprite, la supprime puis
  la récupère par Undo ;
- la suite passe **56/56**, le build Vite est vert et le seul avertissement
  reste la taille connue du chunk principal.

## Gate densité supérieure de l’Item Editor fermé

La structure complète était fonctionnelle, mais son bandeau restait plus large
et plus horizontal que la référence fournie par Vincent :

- la largeur desktop est ramenée de 700 à **640 px**, proche de la modale
  RuneWizard, sans réduire les contrôles ni l’aperçu vivant ;
- quantité et duplication forment la première ligne compacte; Ethereal,
  Identified et Personalized occupent une seconde ligne alignée à gauche ;
- Downgrade/Upgrade sort du panneau de gauche et précède maintenant directement
  la rangée Quality/Base item sur toute la largeur utile ;
- titre, texte de sûreté et espacements sont resserrés, tandis que l’aperçu
  demeure à droite et le footer Delete/Download/Save reste sticky ;
- sous 760 px, l’aperçu et les contrôles restent empilés; sous 520 px, la modale
  conserve son contrat plein écran et sa safe area ;
- le navigateur ouvre Warlord’s Conquest, exerce Normal → Exceptional → Normal,
  Identified off/on et le compteur `Make 2 duplicates`. La modale mesure 640 px
  et n’émet aucune erreur; la suite passe **74/74** et le build Vite est vert.

## Gate import portable et erreurs atomiques fermé

La surface auxiliaire d’import reprend maintenant un cycle visible et contrôlé
avant toute mutation de la sauvegarde :

- les fichiers choisis sont mis en attente dans `Choose items to add`, avec nom,
  format, taille et retrait individuel. Le bouton d’import reste séparé du
  bouton de création depuis le catalogue ;
- les `.d2i` bruts affichent l’absence d’empreinte de tables, tandis que les
  bundles `.bkitems.json` sont identifiés comme fingerprintés. Aucun record
  n’est ajouté tant que toute la sélection n’a pas été décodée et validée ;
- `importItemFiles` refuse explicitement une extension étrangère, un JSON
  illisible, une ABI BKVince différente, un payload base64 invalide, un fichier
  de plus de 16 MiB ou plus de vingt records. Chaque échec nomme le fichier et,
  pour un bundle, le record fautif ;
- une erreur reste dans la modale sous `role=alert` et conserve la file pour
  correction. Le navigateur présente volontairement le Shared Stash de 680
  octets comme un item : la fenêtre demeure ouverte et affiche
  `ModernSharedStashSoftCoreV2.d2i: Possible malformed data at position 314` ;
- le parcours positif exporte un vrai Hand Axe en `.d2i`, le remet en attente,
  lance l’import puis confirme la fermeture de la fenêtre, un seul bouton
  `Edit Hand Axe` et le message d’allocation atomique des nouveaux IDs ;
- la suite reste **56/56**, le build Vite est vert et le seul avertissement est
  toujours la taille connue du chunk principal.

## Gate click-to-add de l'équipement joueur fermé

Le papier-doll joueur n'est plus une simple vue des records existants :

- chacune des douze cases vides ouvre `Choose items to add` avec son libellé et
  un catalogue limité aux bases dont `BodyLoc1/BodyLoc2` accepte ce slot ;
- le catalogue et l'import portable écrivent un seul record natif dans le bloc
  joueur. Les groupes rapides sont volontairement refusés dans une case unique ;
- un item joueur sélectionné peut être équipé directement. Les collisions et les
  mauvais BodyLoc restent fail-closed avant toute mutation ;
- une sélection Virtual Stash, Shared Stash ou Trash ne réutilise jamais par
  accident son index dans `document.model.items` : le clic redevient un ajout
  joueur explicite ;
- le navigateur crée une Amazon vierge, ouvre Head, filtre 58 bases compatibles,
  équipe un Cap et vérifie Undo/Redo. Le test ajoute aussi un ring, importe une
  amulette, refuse ring→Torso et ring→Head, puis exporte et reparse les trois
  placements avec des IDs uniques ;
- la suite passe **57/57**, le build Vite est vert et le seul avertissement reste
  la taille connue du chunk principal.

## Gate Belt RuneWizard fermé

La zone Belt visible dans les captures de Vincent est maintenant interactive :

- les seize cases restent visibles en 4×4; les indices D2S 0–15 sont projetés
  du bas vers le haut et seules les cases couvertes par la capacité native sont
  cliquables ;
- `Misc.txt:belt` gouverne exactement vingt bases spawnables 1×1. `Armor.txt:belt`
  et la référence read-only `Belts.txt` ferment les capacités 4/8/12/16 ;
- les trois tables ont été lues par `tsv.js` avec CRLF et round-trip byte-exact :
  103 920 octets/278 lignes, 81 707 octets/218 lignes et 4 182 octets/15 lignes ;
- ajout groupé d'un même consommable, import de plusieurs records, déplacement,
  collisions, slot hors capacité et retrait d'un belt qui laisserait des potions
  hors capacité sont validés avant écriture ;
- le navigateur ajoute une Minor Healing Potion au slot 1, vérifie Undo/Redo,
  ouvre son Item Editor complet, équipe un Sash et observe 8 slots actifs. Le
  nom anglais vient du header réel
  lorsque les marqueurs couleur/icône du texte localisé ne produisent qu'un
  chiffre ;
- le test exporte/reparse sept consommables, conserve des IDs uniques et ferme
  le no-op byte-exact. La suite passe **58/58**, le build Vite est vert et le
  seul avertissement reste la taille connue du chunk principal.

## Gate bandeaux d'or RuneWizard fermé

Les compteurs montrés à côté des onglets Player/Mercenary et
Stash/Virtual Stash/Trash sont maintenant des contrôles natifs :

- la capsule Equipment écrit `attributes.gold` et la capsule Stash écrit
  `attributes.stashed_gold`; les champs General restent synchronisés par le
  même snapshot et le même historique racine ;
- `Max` suit la règle de jeu 10 000 par niveau, plafonnée à 990 000, et
  2 500 000 pour le stash. L'encodage D2S plus large reste préservateur pour une
  sauvegarde modifiée hors règle ;
- le navigateur crée une Amazon niveau 1, vérifie les maxima 10 000/2 500 000,
  passe au niveau 99 et voit le bouton porté devenir 990 000. Les deux valeurs
  sont identiques dans les capsules et General ;
- Undo ramène l'or porté de 990 000 à 10 000 sans annuler le niveau ni l'or du
  stash; Redo le restaure ;
- le test écrit 123 456 et 2 345 678 dans les stats D2S 14/15 puis les reparse.
  La suite reste **58/58**, le build Vite est vert et le seul avertissement
  demeure la taille connue du chunk principal.

## Gate attributs libres et action de grille nette fermé

Le retour visuel et fonctionnel de Vincent ferme deux défauts de la verticale
objets :

- le badge `Click to add` passe au-dessus des cellules voisines, utilise une
  taille de texte lisible et ne capture plus le pointeur. Il reste centré sur la
  case ciblée dans Inventory, Stash, Cube et Belt ;
- les Item Attributes ne sont plus arbitrairement verrouillés aux seules
  qualités Magic, Set et Unique. Tout record D2S complexe peut maintenant
  ajouter une propriété gouvernée, modifier une valeur sûre ou retirer une
  propriété sans changer sa qualité ;
- les records compacts `simple_item` restent fail-closed, car leur payload natif
  ne contient aucune liste d'attributs ; le message de la modale explique cette
  limite de format au lieu de demander un changement de qualité inutile ;
- le parcours navigateur crée une Hand Axe Normal, ajoute Strength, change sa
  valeur de 0 à 21, la voit dans l'aperçu, sauvegarde la modale et retrouve
  `+21 to Strength` en la rouvrant ;
- le nouveau test exporte puis reparse le même scénario au format v105 et
  confirme que la qualité reste Normal. La suite passe **59/59** et le badge
  net a été validé visuellement dans l'app locale.

Décision de Vincent du 4 août 2026 : la preuve runtime du Bound Demon Warlock
est retirée des priorités. La surface déjà implantée peut rester disponible,
mais elle ne bloque plus la livraison ni les prochains lots du Hero Editor.

## Gate portraits de classes fermé

Les initiales temporaires du header et de la fiche de personnage ont été
remplacées par des visuels gouvernés :

- l'extracteur CASC local récupère les sept portraits officiels D2R 120×120
  d'Amazon, Sorceress, Necromancer, Paladin, Barbarian, Druid et Assassin ;
- BKVince ne fournit pas de portrait Warlock dédié. Son emblème 120×120 est
  donc généré depuis une frame de son atlas de skills BKVince, sans dépendance
  aux assets RuneWizard et sans travail sur le Bound Demon ;
- les huit fichiers sont générés sous `public/ui/portraits`, exposés par le
  manifeste d'assets et gardent les initiales comme fallback si une image ne
  charge pas ;
- le test exige les huit classes, la signature PNG et des dimensions exactes
  de 120×120. Le navigateur confirme le vrai portrait Barbarian et l'emblème
  Warlock dans le header ;
- la suite passe **60/60**, le build Vite est vert et le seul avertissement
  reste la taille connue du chunk principal.

## Gate qualités Rare et Crafted fermé

La verticale d'édition qui restait verrouillée est maintenant reconstruite sur
le vrai payload D2S v105 :

- le générateur lit les tables read-only D2R 3.2 `rareprefix.txt` et
  `raresuffix.txt` avec `tsv.js`; leurs 46 préfixes et 155 suffixes passent le
  round-trip byte-exact et reçoivent les IDs ABI attendus, de `Beast` #156 à
  `Bite` #1 ;
- les 563 préfixes et 614 suffixes BKVince marqués `rare=1` sont filtrés selon
  le type exact, l'item level et les exclusions. Le validateur refuse aussi
  deux affixes du même groupe ;
- Rare et Crafted exposent leurs deux mots de nom ainsi que les six slots
  natifs entrelacés — trois préfixes et trois suffixes. Les rolls minimum ou
  maximum recompilent atomiquement toutes leurs propriétés gouvernées ;
- l'aperçu vivant affiche immédiatement le nom rare, sa base et ses propriétés.
  Le navigateur ajoute une Hand Axe au stash, crée `Beast Bite`, applique
  `Jagged` et `of Bashing`, sauvegarde, rouvre le record, puis confirme la même
  surface en Crafted ;
- le test exporte et reparse les deux qualités, conserve les IDs de noms, les
  six affixes et les attributs 17/120, puis ferme un export no-op byte-exact.
  La suite passe **61/61**, le build Vite est vert et seul l'avertissement connu
  sur la taille du chunk principal demeure.

Cette preuve ferme le codec, l'interface et le round-trip; elle ne prétend pas
encore constituer une validation gameplay D2R de toutes les combinaisons Rare
et Crafted.

## Gate qualité Low fermé

La dernière qualité native courte qui restait verrouillée est maintenant
éditable :

- le générateur lit `LowQualityItems.txt` depuis la référence read-only D2R
  3.2 avec le même contrôle TSV byte-exact et expose les quatre variantes
  `Crude`, `Cracked`, `Damaged` et `Low Quality` ;
- Low est offerte uniquement aux records complexes Armor/Weapons compatibles.
  La variante choisie est validée puis écrite dans son champ natif 3 bits ;
- l'aperçu et le tooltip composent le nom complet, par exemple
  `Low Quality Hand Axe`, sans masquer l'édition libre des Item Attributes ;
- le test écrit les quatre variantes dans une même sauvegarde, exporte/reparse
  les IDs 0–3 et ferme un no-op byte-exact. Le navigateur confirme la sélection,
  l'aperçu, la sauvegarde et la réouverture de la variante ;
- la suite passe **62/62**. Comme pour Rare/Crafted, ce gate est une preuve de
  codec et d'interface; il ne remplace pas une matrice gameplay exhaustive.

## Gate responsive et édition d'attributs fermé

La matrice réelle couvre maintenant les largeurs desktop, tablette et mobile :

- le shell a été inspecté à 1 440×960, 820×1 000 et 390×844 sans débordement
  horizontal. Equipment, inventory, stash, Cube et belt passent de trois
  colonnes à une pile mobile sans réduire leurs grilles ni masquer les charms ;
- `Load / create`, auparavant caché sous 860 px, reste disponible en libellé
  complet sur tablette et devient un bouton `+` de 38×38 px sur mobile ;
- les fenêtres Load/Create, Choose items to add et Item Editor occupent le
  viewport mobile, gardent leur scroll interne et leurs actions accessibles ;
- Quests passe en une colonne, la navigation Stats reste défilable et l'arbre
  Skills conserve ses dix nœuds visibles sans largeur excédentaire ;
- un Jawbone Cap a été ajouté au slot Head depuis l'interface mobile. Dans son
  Item Editor, `Add magic attributes…` a créé Strength, sa valeur a été portée
  de 0 à 21, sauvegardée puis retrouvée à la réouverture sous
  `+21 to Strength` dans la ligne et l'aperçu ;
- la page stackable du vrai Shared Stash reste lisible en une colonne à 390 px.
  Le compteur Chipped Emerald a été modifié 0→21, annulé 21→0 puis rétabli
  0→21 par les commandes Shared Stash, sans débordement horizontal.

Ces vérifications ferment le défaut signalé sur l'ajout et la modification des
attributs ainsi que le gate visuel responsive. Elles ne remplacent pas les
preuves codec/runtime listées séparément dans la matrice.

## Gate variantes visuelles natives fermé

La parité fonctionnelle inclut maintenant la sélection d'apparence publiée par
RuneWizard pour les rings, amulets, charms et jewels :

- les tableaux `ig` des constantes BKVince gouvernent neuf bases et 35 choix,
  soit 23 images classiques distinctes ;
- l'extracteur récupère les DC6 et la palette `units` depuis l'installation D2R
  locale, puis le générateur produit leurs vrais PNG. Aucun asset ni code
  RuneWizard n'est repris ;
- l'Item Editor affiche `Default` et chaque variante avec thumbnail réel. La
  sélection change immédiatement l'aperçu et le sprite de grille ;
- le codec écrit le bit `multiple_pictures` et le `picture_id` natif 3 bits,
  refuse un ID hors catalogue et préserve les items simples qui ne portent pas
  ce bloc ;
- un Ring Variant 5 exporté se reparse avec `multiple_pictures=1` et
  `picture_id=4`, puis son second export sans modification reste byte-exact ;
- le navigateur confirme six choix de Ring, la persistance après Save Changes
  et réouverture, puis le sélecteur mobile à 390×844 sans débordement ;
- la suite passe **64/64** et le build Vite de production est vert.

La référence de portée est le changelog public RuneWizard du 15 octobre 2025 :
`https://d2runewizard.com/hero-editor`.

## Gate sélecteur humain des propriétés fermé

Le moteur `Properties.txt` était déjà complet, mais son interface restait plus
technique que la référence RuneWizard. Le parcours principal de
`Add magic attributes…` est maintenant un picker visuel :

- les 236 propriétés supportées sont recherchables par libellé, code et notes ;
- chaque résultat annonce son nombre réel de groupes D2S et la liste est bornée
  à 80 lignes visibles avant raffinement de la recherche ;
- Flèche haut/bas et Entrée pilotent la sélection. Échap referme le picker,
  restaure son bouton et ne ferme plus l'Item Editor parent ;
- les 390 stats numériques brutes restent accessibles dans un bloc avancé
  replié, sans encombrer le parcours normal ;
- le catalogue expose les IDs de groupes produits. `res-all` gouverne exactement
  39/41/43/45, `dmg-elem` 48/50/54 et `str` le groupe 0 ;
- les groupes dont le rendu combiné forme une propriété Diablo unique sont
  fusionnés dans l'éditeur. `All Resistances +25` devient une ligne et un champ
  partagé, puis +40 met à jour les quatre stats ensemble ;
- l'aperçu déduplique la ligne sémantique sans supprimer aucun attribut du
  payload. Save Changes et la réouverture conservent `All Resistances +40` ;
- le navigateur valide aussi recherche `damage`, deuxième choix par clavier et
  mobile 390×844 avec liste interne scrollable et footer accessible ;
- la suite reste **64/64** et le build Vite de production est vert.

## Gate recherche visuelle des items fermé

La fenêtre `Choose items to add` ne repose plus sur des tuiles de codes pour
identifier les objets :

- les résultats affichent les vrais sprites BKVince/vanilla déjà gouvernés,
  avec nom, source, dimensions et code secondaire compact ;
- Flèche haut/bas et Entrée pilotent le résultat actif depuis la recherche ;
- une carte `Selected item` montre explicitement le sprite et l'objet qui sera
  ajouté avant toute mutation du document ;
- les entrées des six groupes rapides montrent elles aussi leur vrai sprite,
  sans changer leur ajout atomique ni leurs contrôles de quantité/retrait ;
- le navigateur crée `VisualItem`, sélectionne Ring Mail au clavier, l'ajoute au
  Personal Stash, puis ajoute Demon’s Ear, Baal’s Eye et Mephisto’s Brain comme
  groupe. Les quatre boutons d'édition sont ensuite retrouvés dans la grille ;
- le même catalogue et sa carte de sélection ont été inspectés à 390×844 sans
  débordement horizontal. Un test impose un PNG gouverné à chaque entrée
  quick-add ; la suite passe **65/65** et le build Vite de production est vert.

## Gate catalogue nommé et Runewords fermé

La recherche de `Choose items to add` reproduit maintenant les propositions
mixtes de la référence sans dépendre de son code ni de son catalogue :

- les 729 bases ajoutables, 215 Sets, 473 Uniques encodables et 112 Runewords
  compilables proviennent exclusivement des tables BKVince gouvernées ;
- une même requête peut rendre plusieurs familles. `call` place CTA avant les
  Sets Orphan's Call; `plague` montre Plague (PB), Plague Bearer et Hellplague ;
- chaque Set ou Unique est créé directement avec sa base canonique, sa qualité,
  son ID et la compilation minimum/maximum de toutes ses propriétés ;
- chaque Runeword expose seulement les bases compatibles à sa déclaration
  `itype` et à son nombre de runes. Les préférences visuelles choisissent Flail
  pour Call to Arms et Phase Blade pour Plague, sans verrouiller les autres
  bases valides ;
- la mutation reste atomique : identité et base incompatibles, recette invalide,
  collision ou débordement ne laissent aucun record partiel ;
- `CatalogHero` exporte/reparse Annihilus, Warlord's Conquest, CTA et Plague,
  puis ferme un second export byte-exact. L'interface confirme les propriétés
  d'Annihilus, les cinq fillers de CTA et le rendu 390×844 ;
- la suite passe **66/66** et le build Vite de production est vert.

## Gate drag-and-drop inter-grilles fermé

Le placement RuneWizard ne dépend plus uniquement du flux click-to-place :

- les items d'Inventory, Personal Stash, Cube, Belt, Shared Stash ordinaire,
  Virtual Stash et Trash sont déplaçables directement vers une cellule libre ;
- les slots d'équipement joueur acceptent le drop d'un item Player, Shared Stash
  ordinaire, Virtual Stash ou Trash dont le `BodyLoc` est compatible ;
- souris utilise le drag natif; touch et stylet utilisent un appui maintenu de
  220 ms, avec cibles dorées et destination active surlignée ;
- le clic continue d'ouvrir l'Item Editor et le click-to-place accessible reste
  disponible après fermeture de la modale ;
- un transfert inter-workspaces transmet maintenant le conteneur et les
  coordonnées exactes au codec. La cible est toujours écrite avant la suppression
  de la source : rejet BodyLoc, Belt, collision ou débordement reste atomique ;
- la page Shared Stash stackable demeure hors drag, conformément à son ABI
  volontairement limitée au compteur prouvé ;
- le test de transfert passe par Trash → Cube en `(2,2)`, retourne dans Virtual
  puis équipe Right hand. Le record final reparse `location_id=1` et
  `equipped_id=4`; un drop du même Hand Axe vers Head échoue sans muter la source
  ou le Player ;
- le navigateur confirme le clic Stash → Cube, une source Virtual `draggable`,
  les douze destinations Equipment typées et un `scrollWidth` contenu dans le
  viewport 390 px. La suite reste **66/66** et le build Vite de production est
  vert.

## Gate corbeille globale RuneWizard fermé

Le bouton carré sous Belt est maintenant une vraie destination récupérable :

- un clic ouvre le workspace Trash et son badge expose le nombre d'items encore
  restaurables ;
- le drag natif souris et le geste tactile/stylet utilisent la même cible typée
  que les grilles. Tout objet provenant d'un autre workspace y est auto-placé
  dans la première cellule libre ;
- l'auto-placement évite la collision implicite en `(0,0)` lors de plusieurs
  suppressions successives, sans assouplir les placements exacts des autres
  destinations ;
- chaque déplacement reste target-first et atomique. Undo remet simultanément
  la source et Trash dans leur état précédent; Redo rejoue les deux historiques ;
- le test codec transfère successivement Hand Axe et Cap vers `(0,0)` puis
  `(1,0)` sans perte de source ni collision d'ID ;
- le navigateur crée un Cap dans Personal Stash, l'envoie dans Trash, constate
  le badge `1`, le restaure par Undo puis confirme Redo et zéro erreur console.
  La suite passe **74/74** et le build Vite de production est vert.

## Gate recherche sémantique des items fermé

La documentation publique RuneWizard indique qu'un item peut être recherché par
nom, type ou propriété. Le catalogue BKVince ferme maintenant ce dernier axe :

- les 215 Sets, 473 Uniques et 112 Runewords compilables reçoivent un index de
  recherche construit depuis leur payload de propriétés gouverné, jamais depuis
  une liste parallèle de mots-clés ;
- l'enhancer Diablo produit les descriptions humaines des stats paramétrées,
  skills, oskills et procs; le code `Properties.txt`, Socketed et Ethereal restent
  aussi indexés comme replis gouvernés ;
- la liste affiche `Matches:` et jusqu'à deux propriétés qui expliquent pourquoi
  le résultat est proposé ;
- le test verrouille `Cannot Be Frozen` sur Raven Frost, `Teleport` sur Enigma et
  `All Skills` sur Annihilus avec les comptes courants 215/473/111 ;
- le navigateur obtient huit résultats pour `teleport`, 49 pour `cannot be
  frozen`, conserve Annihilus parmi les résultats `all skills`, sélectionne
  Enigma par Flèche/Entrée et l'ajoute dans le Personal Stash. Le message de
  succès utilise maintenant l'identité matérialisée (`1× Enigma`) plutôt que le
  seul nom de base Archon Plate ;
- le même résultat Enigma est visible à 390×844 avec `clientWidth=390` et
  `scrollWidth=390`. La suite passe **66/66** et le build Vite est vert.

## Gate panier libre multi-items fermé

La demande d'ajout « individuellement ou en groupe » couvre maintenant autre
chose que les six groupes rapides prédéfinis :

- le panier mélange librement Bases, Sets, Uniques et Runewords avec une
  quantité indépendante par ligne, jusqu'à vingt records ;
- chaque ligne fige base compatible, item level, stack, rolls et identité au
  moment où elle rejoint le panier, puis reste ajustable ou retirable ;
- une seule mutation planifie toutes les positions et applique toutes les
  identités. Un échec de place dans la destination ne publie aucun snapshot
  partiel ;
- le test round-trip ferme `2× Annihilus + Warlord's Conquest + Call to Arms`
  sur Flail et prouve qu'un lot de vingt Archon Plates impossible dans le Cube
  laisse l'original byte-for-byte logique intact ;
- le navigateur confirme les quatre boutons d'édition, Undo complet, Redo
  complet et `clientWidth=scrollWidth=390` sur mobile. La suite passe **67/67**
  et le build Vite est vert.

## Gate Empty personal stash et seuil 320 px fermé

- `Empty personal stash` marque retirés uniquement les placements du Personal
  Stash dans un snapshot joueur atomique et laisse Inventory, Equipment, Belt,
  Cube ainsi que les huit charms BK gelés intacts ;
- le test codec place deux Hand Axes dans la stash et un Cap dans le Cube,
  prouve l'absence de mutation de la source, puis exporte/reparse zéro objet en
  stash, un Cap au Cube et les huit charms en Inventory ;
- le navigateur place `2× Annihilus` dans la stash et un Cap au Cube : le
  vidage retire seulement les Annihilus, Undo les restaure ensemble et Redo les
  retire de nouveau ;
- le `min-width` global ne force plus une page de 320 px dans la largeur de
  contenu de 304 px produite par la barre de défilement. Shell chargé et modale
  restent sans débordement avec `clientWidth=scrollWidth=304` à la largeur
  nominale 320 px. La suite passe **68/68** et le build Vite est vert.

## Gate sprites contenus, forge Perfect et paper-doll fermé

- les vignettes du catalogue imposent une surface intérieure de 35×35 px,
  `object-fit: contain` et deux niveaux d'overflow caché. Dacian Falx, Dagger,
  Calamity et CTA ne traversent plus les résultats voisins en desktop ou à
  319 px ;
- la création catalogue n'expose plus le roll minimum : Bases défensives,
  Sets, Uniques, Runewords et groupes rapides utilisent toujours les maxima
  gouvernés. Les imports portables conservent leurs valeurs source ;
- le test codec forge et reparse un Cap à sa défense maximale, Annihilus,
  Warlord's Conquest et CTA avec leurs propriétés maximales par défaut ;
- le navigateur confirme Annihilus à +20 all attributes, +20 all resistances et
  +15% XP, ainsi qu'un Cap à 5 de défense dans sa plage 3–5 ;
- les douze cases vides du paper-doll superposent désormais des silhouettes
  assombries provenant des vrais sprites gouvernés. Elles sont décoratives,
  disparaissent sous l'objet équipé et la preuve place un Cap en Head puis
  restaure la silhouette par Undo. La suite passe **69/69** et le build Vite
  est vert.

## Gate formulaires sémantiques de toutes les propriétés fermé

- les champs techniques `Parameter / Minimum / Maximum` ont été remplacés par
  un schéma couvrant les 24 familles d'encodage des 236 propriétés supportées ;
- chaque propriété expose uniquement ses valeurs réelles : valeur simple,
  dégâts minimum/maximum, skill, skill tab, classe, monstre, bonus, chance,
  niveau de proc, charges, durée ou sockets. `oskill` fournit `Skill + Bonus` et
  les trois variantes `randclassskill*` fournissent `Class + Bonus` ;
- le même catalogue de 431 skills nommés BKVince, incluant les trente skills
  Warlock, alimente l'ajout et l'édition des attributs paramétrés déjà présents ;
- une matrice automatisée compile, écrit et reparse un D2S pour chacune des 236
  propriétés. Les groupes élémentaires partiels sont complétés canoniquement ;
- la correction du signe de `dmg-ac` récupère Guardian Angel, portant le
  catalogue à **462/473 Uniques** et laissant onze cas fail-closed ;
- le navigateur confirme `+7 to Whirlwind`, `+5 to Warlock Skills`, l'édition
  par menus, aucun placeholder `%+d` et aucun `â€¦` ;
- chaque formulaire compile maintenant son état courant avant de rendre l'action
  disponible et affiche un aperçu du texte final en jeu au lieu de conserver le
  placeholder `+#`. Une valeur ou une combinaison invalide désactive l'ajout et
  expose l'erreur du même compilateur que l'export ;
- les neuf collisions d'identifiants du catalogue de monstres sont regroupées
  en 789 IDs natifs uniques avec leurs deux noms conservés, par exemple
  `Goatman / Pit Lord` pour 742. Le rendu spécial du stat 155 devient
  `100% Reanimate as: Goatman / Pit Lord` et aucun token `%0`, `%1` ou
  placeholder gouverné ne subsiste dans la matrice des 236 propriétés ;
- les valeurs numériques d'ajout et d'édition gardent un unique
  `input[type=number]`, mais ses micro-flèches navigateur irrégulières sont
  retirées avec `appearance: textfield`. La saisie directe et les touches
  fléchées restent disponibles sans le bloc de deux boutons qui déformait les
  chevrons ;
- le parcours navigateur confirme `+7 to Whirlwind`, `+4 to Warlock Skills`,
  `25% Chance to cast level 10 Whirlwind on striking`, `Adds 12-34 fire damage`
  et la réanimation à 100%, puis rouvre l'attribut oskill stocké avec ses menus
  Skill et Bonus à 7. La suite passe **77/77** et le build Vite de production
  est vert.

## Gate des derniers Unique sérialisables fermé

Un audit exhaustif des onze exclusions a séparé six familles réelles. Neuf
objets supplémentaires sont maintenant créables en mode Perfect :

- The Grim Reaper, Umbral Disk et Swordguard préservent leurs totaux hors plage
  par plusieurs attributs natifs additifs, chacun restant dans ses SaveBits;
- Messerschmidt's Reaver applique le booléen gouverné implicite `noheal=1`;
- Baranar's Star, Azurewrath, Demon's Arch et Elemental Union conservent leurs
  dégâts maximaux et saturent uniquement `coldlength` au plafond natif 255;
- Static Accumulator reçoit son socket Unique explicitement déclaré sans rendre
  les autres Boots arbitrairement socketables;
- un test unique construit les neuf objets, vérifie leurs payloads exacts,
  exporte, reparse et ferme un no-op byte-exact. La suite passe **71/71** et le
  build Vite de production est vert;
- à ce jalon, le navigateur affiche **471 Uniques**, trouve Static Accumulator, l'ajoute au
  Personal Stash et expose exactement un socket dans l'Item Editor.

## Gate des variantes Wraithstep et Opalvein fermé côté codec/UI

Les deux derniers Unique ne dépendent plus de propriétés placeholder impossibles
à compiler :

- Wraithstep propose exactement Chaos, Demon ou Eldritch, chacun matérialisé par
  un seul `skilltab` natif à +1;
- Opalvein propose exactement Magic Skill Damage, Enhanced Damage, Fire, Cold,
  Lightning ou Poison. Un seul bonus est présent dans le payload;
- Chaos et Magic Skill Damage sont les choix par défaut. Toute sélection est
  immédiatement reconstruite au roll **Perfect**;
- après export/reparse, l'éditeur ré-identifie la variante depuis les attributs
  D2S natifs; aucun champ propriétaire n'est ajouté au fichier;
- un test couvre les neuf variantes, leur exclusivité, leurs maxima, leur
  export/reparse et le no-op byte-exact. La suite passe **72/72** et le build
  Vite de production est vert;
- le navigateur confirme le passage Wraithstep Chaos → Demon et Opalvein Magic
  Skill Damage → Enhanced Damage sans conserver l'ancien bonus; aucun log
  d'erreur applicatif n'est émis.

Le catalogue atteint ainsi **473/473 Unique** côté codec et interface. Le témoin
runtime `HEVariantRt` confirme maintenant les neuf variantes à travers
`Save and Exit`; l'effet gameplay des trois gros scalaires reste distinct à
observer après le second chargement, car D2R canonicalise leurs fragments dans
le fichier resauvegardé.

## Gate des 112 Runewords fermé côté codec/UI

`Chaos`, dernière recette exclue, est maintenant représentée dans les limites
natives persistantes du format :

- la recette BKVince gouvernée reste `Fal–Ohm–Um` sur une base `h2h`; le catalogue
  choisit une Katar trois sockets par défaut et conserve les vingt-et-une bases
  compatibles;
- les sept déclarations de `Runes.txt` sont compilées au roll **Perfect**, y
  compris +290% Enhanced Damage, 216–471 Magic Damage, +5 Whirlwind et le proc
  Frozen Orb;
- `rep-dur=100` dépasse le champ D2S 0–63. L'éditeur écrit une seule occurrence
  sûre du stat 252 à `63`; aucun élargissement d'`ItemStatCost.txt` ni changement
  de `Runes.txt` n'est appliqué;
- une matrice compile les **112/112 recettes** aux rolls minimum et maximum. Le
  témoin `ChaosForge` ajoute les trois runes, exporte/reparse le record v105,
  retrouve les sept attributs Runeword puis ferme un no-op byte-exact;
- la suite passe **73/73** et le build Vite de production est vert.

Le gate gameplay D2R de Chaos est maintenant fermé par le témoin
`HEVariantRt`; la limite des stats répétées a conduit au correctif codec
documenté ci-dessous.

## Preuve runtime Wraithstep / Opalvein / Chaos — limite des stats répétées

Le 4 août 2026, `HEVariantRt.d2s` a été généré par le vrai codec puis déployé
dans le profil exact `Saved Games/Diablo II Resurrected/mods/BKVince/`. Sa
source fait 2 063 octets et porte le SHA-256
`9B6AA716BC53B48DF55F65877689E78BE6A0377B209B91B5A4913D00E2B6650D`.
Le cold start charge 17/17 patchsets mémoire et 12/12 plugins actifs, sans rejet
ni échec.

- D2R reconnaît l'Assassin niveau 99, entre en jeu et resauvegarde les treize
  objets gouvernés;
- les neuf variantes Wraithstep/Opalvein restent exactement identifiables dans
  le D2S post-jeu : Chaos, Demon, Eldritch, Magic, Enhanced Damage, Fire, Cold,
  Lightning et Poison;
- Chaos est équipé sur une Katar `Fal–Ohm–Um`. Sa durabilité passe de 1/48 à
  8/48 au premier cycle, puis de 8/48 à 15/48 pendant un second séjour mesuré à
  24,59 secondes. Le gameplay réapplique donc bien le `rep-dur=100` gouverné;
- le premier fichier post-D2R fait 2 055 octets, SHA-256
  `B6F71D498D0A97D88F52E8327081F2B72DE58A8B0096C93A6E878B67B4251DD1`;
  le second porte
  `33B56B29CD632F760EF71084EB791F577A940CA99BB559E3F15727742E8D05BE`.
  Les deux restent no-op byte-exacts dans l'éditeur;
- fait runtime important : lors de `Save and Exit`, D2R canonicalise les IDs de
  stat répétés et ne conserve que le dernier fragment dans le fichier :
  The Grim Reaper `-32 + -18 → -18`, Umbral Disk `63+63+63+11 → 11`,
  Swordguard `63+12 → 12` et Chaos `63+37 → 37`. Le deuxième cycle conserve
  encore `37` dans le D2S tout en reproduisant la cadence de réparation issue
  de la recette BKVince.

Conclusion gouvernée : les variantes sont fermées runtime et l'effet de Chaos
est prouvé, mais des fragments identiques ne constituent pas une représentation
D2S persistante. Modifier `Save Bits` ou `Save Add` dans `ItemStatCost.txt`
changerait l'ABI de tous les items concernés et exige une stratégie de migration;
aucun changement de table n'est donc inféré depuis ce témoin.

Le codec ne produit plus aucun fragment répété pour ces scalaires. Il écrit une
seule valeur native déterministe : The Grim Reaper stat 7 à `-32`, Umbral Disk
stat 214 à `63`, Swordguard stat 20 à `63` et Chaos stat 252 à `63`. Le témoin
`HEGrimScalar` a confirmé en jeu que l'ancien `-32 + -18` ne s'additionnait pas :
le personnage restait à 37/37 HP, soit uniquement le dernier `-18` sur sa base
55. Le nouveau témoin `HEUmbralMax` ferme l'autre effet : au niveau 99, l'écran
du personnage affiche exactement **862 Defense**, soit 68 venant de 275
Dexterity, 15 venant de la défense parfaite de l'objet et
`floor(63 × 99 / 8) = 779` venant du stat 214. Après `Save and Exit`, le fichier
runtime passe du SHA-256
`DF5769CC7E195FD69C7BF71C47E6DD66874CC689DFF88DE9340F0AB9CBD4BA69` à
`09B55A61E773395FDB7D30C8AA44A08D9CF31E2BFC91C79C6E88EBAD440D0F82`,
reparse encore une seule valeur `63` et reste no-op byte-exact. Les tests ciblés
et la suite complète passent **77/77**; le build Vite de production est vert.

## Gate Skills natifs RuneWizard fermé

L'audit visuel et comportemental direct du 4 août remplace la première
approximation CSS/SVG :

- les huit `*Skilltree.sprite` Blizzard sont extraits du CASC local, puis leurs
  trois frames 897×1169 sont recadrées sans rééchantillonnage à 895×1169. Les
  **24 fonds natifs** portent les
  cadres, flèches, cases de rang et onglets exacts; aucun asset RuneWizard n'est
  repris ;
- le shell BKVince affiche ces fonds dans la scène RuneWizard compacte mesurée
  sur la capture de référence : **314×410 px**, au ratio 895/1169. Les onglets
  occupent 7,5 % du haut et la grille proportionnelle 3×6 reprend les mêmes
  paddings et gaps. Les frames 132×130 de l'atlas
  Blizzard sont recadrées en carrés 130×130, sans étirement des colonnes
  transparentes. Le rang est vide à zéro et se place dans la petite case en bas
  à droite ;
- l'en-tête reprend la densité de la capture RuneWizard : 16 px de haut, 14 px
  d'espacement avant l'arbre, switch 24×13 px avec texte 8 px et compteur rouge
  régulier 12 px. Les tooltips locaux ont été retirés et le survol se
  limite à l'éclaircissement natif; le halo investi est exactement de 4 px ;
- les skills sans prérequis restent visuellement disponibles comme dans
  RuneWizard. Les prérequis gouvernent le verrou gris
  `grayscale(1) brightness(.35)`; le niveau requis est vérifié au clic ;
- le compteur `Unused Skill Points` reste informatif et stable pendant l'édition,
  conformément au comportement RuneWizard observé. La surface ne consomme ni ne
  rembourse ce champ et le bouton `Reset skills` de l'ancienne interprétation a
  été retiré ;
- `Ignore Game Rules` retire les gardes et autorise 0–255. Clic, Shift-clic,
  clic droit et Undo/Redo restent reliés au modèle natif ;
- le navigateur prouve Amazon Jab 0→1 avec compteur stable à 0, Power Strike
  déverrouillé par le prérequis mais refusé au niveau 1, Undo/Redo et clic droit
  vers zéro. Lightning Fury ferme 0→1→0 en mode libre; les trois fonds Amazon
  changent avec leur onglet. Warlock affiche ses dix nœuds `Chaos Skills` sur le
  fond `WaSkilltree` et expose aussi `Eldritch Skills / Demon Skills` ;
- les tests vérifient les huit manifestes de trois PNG, leur signature et leur
  taille 895×1169. La suite passe **77/77** et le build Vite de production est
  vert.
- la comparaison directe avec RuneWizard au même viewport de 1 265 px ferme le
  gate de densité : le cadre de référence mesure environ 884 px, sa navigation
  176 px et sa scène native 420 px. Le breakpoint desktop à 960 px applique
  exactement cette croissance depuis la composition compacte 664/132/314 px;
  les icônes et les fonds Blizzard restent à leur résolution native, sans
  rééchantillonnage. Le navigateur confirme ensuite Howl 0→1→0, Undo et le mode
  libre dans la géométrie élargie ;
- après création ou chargement d'un héros, un reset différé à `scrollY=0`
  empêche le header sticky de couvrir les titres du workspace. Sur le témoin
  Amazon, Equipment/Stash/Cube commencent à 75 px alors que le header se termine
  à 57 px.

## Gate fiche General compacte RuneWizard fermé

La page General suit désormais la hiérarchie visuelle de la capture de Vincent :

- aucun titre interne ne répète `General`. Les cinq panneaux Identity, Character
  state, Source-preserving export, Core attributes, Resources et Progression ne
  fragmentent plus la surface ;
- Name occupe 300 px, suivi de `Level / Map Seed`, puis de trois colonnes pour
  les quatre stats de base, l'expérience et les points disponibles, et les
  ressources Hp/Mana/Max Stamina. Les champs mesurent 22 px de haut ;
- Level n'apparaît qu'une fois. Les bornes techniques, la classe verrouillée et
  les champs d'or dupliqués sont retirés; les capsules Equipment/Stash restent
  les contrôles d'or RuneWizard gouvernés ;
- Expansion, Hardcore, Died et Ladder utilisent une seule rangée de switches
  compacts. Une note violette conserve l'explication de l'export reparsé sans
  recréer un grand panneau technique ;
- le navigateur mesure 538 px de contenu et 471 px de hauteur, confirme trois
  colonnes, un Level, aucun hint de bornes et zéro débordement horizontal.
  Strength 30 → 41 et Expansion true → false traversent Undo/Redo dans les deux
  sens puis reviennent au snapshot initial, sans erreur console ;
- la suite reste **74/74** et le build Vite de production est vert.

## Gate chrome Quests et Waypoints RuneWizard fermé

Les deux pages suivent maintenant leurs captures respectives sans chrome inventé :

- Quests retire le titre interne et remplace la toolbar horizontale par l'ordre
  `Unlock Hell` → note violette → `Complete All / Reset All` → Normal. La note
  explique la relation native entre complétion et récompense ;
- les trois difficultés et leurs actes conservent les 81 états de quête et les
  trois drapeaux `consumed_scroll`. Les séparateurs horizontaux absents de la
  référence sont retirés au profit de l'espacement vertical ;
- Waypoints retire titre, description, note technique et actions globales, tous
  absents de la capture. Normal commence directement en haut du contenu et les
  117 bits restent éditables individuellement ;
- le navigateur confirme 84 contrôles Quests, puis 0 → 54 avec `Unlock Hell`,
  retour 0 par Undo, 81 avec `Complete All`, 0 avec `Reset All`, Undo 81 et Redo
  0. Waypoints confirme 117 contrôles et Rogue Encampment false → true → Undo →
  Redo → false restauré ;
- aucune note ou action globale Waypoints ne subsiste et aucun log d'erreur ou
  d'avertissement applicatif n'est émis. La suite reste **74/74** et le build
  Vite de production est vert.

## Gate modal Choose items to add compact fermé

La modale suit maintenant la densité de la capture RuneWizard :

- le titre est suivi directement de la recherche et des six groupes rapides;
  le contexte technique de destination reste accessible sans ajouter un bandeau
  visible ni une longue description interne ;
- lorsqu'un groupe est sélectionné, les filtres du catalogue et le champ global
  `Item level` sont masqués. Les lignes mesurent 38 px, les sprites 30 px et les
  suppressions utilisent une icône de corbeille nette ;
- l'action `Add` précède l'import portable et reste visible pendant le scroll.
  Le navigateur mesure 600×532 px pour Warlord's Glory et confirme cinq lignes
  compactes, puis `Add 5 items` → `Add 6 items` après duplication de Warlord's
  Conquest ;
- l'ajout crée six records Set; Undo/Redo prouve 0 → 6 → 0. La recherche
  `call to arms` ne retourne qu'une proposition et crée le Runeword sur Flail
  `FLA · 2×3` ;
- Undo après fermeture de l'Item Editor efface aussi la sélection devenue
  invalide : aucune fiche `Unknown item` ne subsiste et la case de stash redevient
  immédiatement cliquable ;
- la suite reste **74/74** et le build Vite de production est vert.

## Gate workspace supérieur RuneWizard recalé

Equipment, Stash, Cube et Belt suivent maintenant la composition de la capture
sans sacrifier les dimensions natives BKVince :

- le header chargé mesure 56 px et ses actions 28 px. L'action de download en a
  été retirée comme dans RuneWizard; `BKVince v105` et `Save to file` forment
  désormais le footer aligné à droite sous Stats, après la note de compatibilité
  D2S v105 ;
- les notifications de succès ou d'état deviennent des toasts fixes non
  bloquants et disparaissent visuellement après 2,8 secondes. Une erreur reste
  inline et visible ;
- les colonnes Equipment, Stash et Cube/Belt mesurent 252, 262 et 84 px, séparées
  par 24 px. Le paper-doll mesure 252×184 px ;
- les grilles restent les vraies grilles BK : Inventory 11×8 avec la colonne
  gelée et son pack de départ, Personal Stash 16×13, Cube 6×6 et Belt 4×4.
  Le pack contient huit charms verticaux, le Horadric Cube et le Town Portal
  Scroll; leurs cellules sont respectivement recalées à 21, 15, 12 et 18 px ;
- la rangée Personal Stash montre les deux actions de la référence, `Empty
  personal stash` et `Load Shared Stash`, avec une hauteur compacte de 27 px ;
- le navigateur ajoute un Cap Perfect, le transfère vers Trash, prouve Undo/Redo
  et restaure le témoin vierge. `Max` écrit 990000 carried gold et Undo revient à
  zéro. À 390×844, Equipment fait 252 px, Stash 262 px et
  `clientWidth=scrollWidth=375` ;
- aucun avertissement ou erreur navigateur n'est observé. La suite passe
  **74/74** et le build Vite de production est vert.

## Gate fenêtre Item Editor RuneWizard fermé

La fenêtre d'édition reprend maintenant la hiérarchie de la capture de référence :

- largeur desktop de 600 px, contrôles utiles sur 305 px et aperçu vivant sur
  195 px, sans en-tête technique visible ni longue copie de sérialisation ;
- quantité/duplication, trois petits switches et actions de tier précèdent
  Quality/Base item, l'identité Set/Unique, Item Level et Defense ;
- `Magic Attributes`, `Item Set Bonuses` et `Socketed` apparaissent avant les
  durabilités et compilateurs avancés, qui restent accessibles dans des volets
  repliés au lieu de charger le parcours courant ;
- les listes Set inactives n'affichent plus leurs codes `Properties.txt` : le
  sélecteur `Add set attributes…` construit le roll Perfect, tandis que les
  listes réellement actives restent éditables et supprimables ;
- le footer sticky reprend les actions rouge/gris de RuneWizard. Le navigateur
  prouve Strength +15 → +52, Save, réouverture, Undo vers +15 et Redo vers +52,
  puis restaure le héros vierge ;
- la suite reste **74/74** et le build Vite de production est vert.

## Gate Item Bonuses RuneWizard fermé

- la surface commence directement par `Attributes`, `Resistances`,
  `Breakpoints` et `Misc`; le dashboard technique, ses compteurs et sa liste de
  sources ont été retirés car ils n'existent pas dans RuneWizard ;
- le cadre mesure 664×337 px, la navigation 132 px, ses rangées 42 px, les
  colonnes sont séparées de 36 px et les quatre groupes reprennent les dix-neuf
  métriques visibles de la référence avec leurs formats exacts ;
- les valeurs sont calculées depuis les attributs D2S effectifs de l'équipement,
  des Runewords et des fillers. Les groupes All Attributes/All Resistances et
  les rolls dépendants du niveau alimentent chaque colonne sans approximation ;
- une matrice réelle équipe Harlequin Crest, Mara's Kaleidoscope, The Gnasher,
  Arachnid Mesh, Sandstorm Trek et Chance Guards, exporte/reparse les six objets
  puis confirme 40 Strength, 25 pour les trois autres attributs, 148 Life/Mana,
  30/30/30/100 résistances, 10 Physical, 20 FCR, 20 FHR, 15 FBR, 25 IAS,
  +6 All Skills, 90 MF et 200 GF avec no-op byte-exact ;
- le navigateur confirme Harlequin Crest au niveau 1 puis 99, Life/Mana 1→148,
  Undo jusqu'aux dix-neuf valeurs nulles et Redo jusqu'au calcul niveau 99 avant
  de restaurer le héros vierge. La suite passe **76/76** et le build Vite est
  vert.

## Gate pack de départ BKVince complet fermé — 5 août 2026

L'audit de `CharStats.txt` et du témoin runtime `ama.d2s` corrige le contrat de
création : un héros BKVince neuf porte dix objets natifs, pas seulement les huit
charms déjà implantés.

- les huit classes produisent désormais `mff` en `(10,0)`, `mfc` en `(10,1)`,
  six `mfd` en `(10,2..7)`, le Horadric Cube `box` en `(0,0)` et le Town Portal
  Scroll `tsc` en `(9,7)` ;
- les dix records portent `starter_item=1`, des IDs uniques et leur structure
  D2S correcte; `box` reste complexe Normal et `tsc` reste compact ;
- l'Inventory 11×8 souligne visuellement sa colonne 11 gelée. Cette distinction
  reste informative : aucune interdiction de placement non prouvée n'est
  inventée dans le codec ;
- le navigateur crée un Warlock neuf et montre le Cube 2×2, le scroll et les
  huit charms aux dix positions exactes ;
- `HEStarterTen.d2s`, Amazon niveau 1, charge dans le profil exact
  `D2RLoader.exe -mod BKVince -txt -offline`. L'inventaire en jeu montre les dix
  objets aux positions attendues ;
- après `Save and Exit`, D2R conserve 1 283 octets, les dix records et toutes
  leurs coordonnées. Le checksum est valide et le fichier réécrit produit un
  export no-op byte-exact ;
- la suite passe **77/77** et le build Vite de production est vert.

## Gate tooltip Click to add aux bords fermé — 5 août 2026

Le libellé d'aide d'une case vide ne dépend plus d'un centrage fixe qui le
faisait couper par les bords de la grille. Chaque cellule déclare maintenant
ses ancrages horizontaux et verticaux (`start`, `center`, `end`) et le tooltip
s'aligne vers l'intérieur de Inventory, Personal Stash, Cube et Belt.

- le navigateur ouvre puis ferme `Choose items to add` depuis la colonne 10 de
  l'Inventory et confirme le retour de focus avec `Click to add` entièrement
  lisible ;
- l'audit côte à côte de Skills à largeur identique confirme que le cadre, la
  navigation latérale, la scène native, les onglets et les flèches sont déjà
  alignés sur RuneWizard ; aucune retouche non démontrée n'est introduite ;
- la suite reste **77/77** et le build Vite de production est vert.

## Gate création Perfect au niveau 99 fermé — 5 août 2026

La référence vivante tranche le niveau des nouveaux items : RuneWizard crée
Harlequin Crest à `Item Level 99` sur une Amazone niveau 1. BKVince utilisait le
niveau courant du héros, ce qui produisait un Unique niveau 1 malgré ses rolls
Perfect.

- le codec expose désormais `PERFECT_ITEM_LEVEL = 99` et l'utilise comme défaut
  commun pour les créations en grille, les slots d'équipement, les groupes
  rapides et les lots de catalogue ;
- l'interface ouvre chaque sélection à 99, tout en conservant le champ éditable
  lorsqu'un niveau volontairement différent est requis ;
- les items chargés depuis un D2S, importés depuis `.d2i` ou dupliqués ne sont
  jamais normalisés silencieusement ;
- Harlequin Crest créé sur le Warlock niveau 1 affiche `Item Level: 99`,
  `Defense: 141` et ses six groupes Perfect dans la modale et le tooltip ;
- fermer ou sauvegarder l'Item Editor efface la sélection transitoire et revient
  directement au workspace, sans la grande barre `Selected item` absente de
  RuneWizard. Le drag reste la voie directe de déplacement ;
- la suite reste **77/77** et le build Vite de production est vert.

## Gate dimensions desktop de l'Item Editor fermé — 5 août 2026

La comparaison au même viewport 1 280×720 corrige une hypothèse antérieure :
la modale RuneWizard vivante mesure 800×647,5 px, tandis que BKVince restait à
600×680 px. La largeur de référence est maintenant reproduite sans sacrifier
les captures compactes ni le vrai mobile.

- au-dessus de 820 px, l'Item Editor BKVince mesure exactement 800 px et sa
  composition utile devient 400 px de contrôles, 16 px d'écart et environ
  320 px d'aperçu ;
- sous 820 px, la modale conserve le format 600 px avec les colonnes 305/195 px
  déjà validées dans les captures étroites ;
- l'empilement aperçu/contrôles ne commence plus à 760 px : il est réservé au
  plein écran mobile sous 520 px ;
- le tooltip adopte le texte exact `[click to edit]/[drag to move]` de la
  référence, sans exposer l'instruction tactile supplémentaire dans le chrome ;
- Harlequin Crest est ouvert dans les deux éditeurs; BKVince mesure 800 px,
  garde son footer sticky et conserve `Item Level: 99`, `Defense: 141` et ses
  propriétés Perfect ;
- la hauteur BKVince est ramenée à 648 px contre 647,5 px dans RuneWizard au
  même viewport, sans tronquer le contenu défilable ;
- l'aperçu récupère la transformation native des objets nommés et magiques.
  Harlequin `cgrn` affiche exactement `Cyan Green tint` et la pastille
  `rgb(0, 255, 127)` ;
- le nom, la base, le niveau d'objet et les propriétés de Harlequin diffèrent
  de moins de 0,3 px sur l'axe horizontal et de moins de 0,1 px sur l'axe
  vertical. Leur ordre suit la liste éditable, comme RuneWizard ;
- la sélection transitoire reste masquée derrière la modale et disparaît à sa
  fermeture ;
- la suite passe **78/78** et le build Vite de production est vert.

## Gate propriétés Based on Character Level fermé — 5 août 2026

La fiche RuneWizard de Harlequin Crest ne se contente pas de la valeur calculée :
elle affiche aussi `(Based on Character Level)`. Les constantes BKVince prouvent
que cette seconde ligne `d2` gouverne 37 stats `ItemStatCost`, de la défense par
niveau aux dégâts, attributs, résistances, absorptions et chances par niveau.

- `formatMagicAttribute` ajoute désormais la seconde ligne gouvernée à toute
  propriété qui la déclare, sans liste spéciale limitée à Harlequin ;
- Harlequin Crest affiche `+1 to Life (Based on Character Level)` et la même
  forme pour Mana sur le Warlock niveau 1, comme RuneWizard ;
- au niveau 99, le même payload affiche `+148 to Life (Based on Character
  Level)` et `+148 to Mana (Based on Character Level)` ;
- Item Bonuses conserve ses nombres effectifs et produit toujours 148 Life et
  148 Mana au niveau 99; aucun record, roll ou encodage D2S n'est modifié ;
- un test parcourt les 37 définitions et exige leur seconde ligne, puis le
  loadout de six Uniques ferme export, reparse et no-op byte-exact ;
- la suite passe **78/78** et le build Vite de production est vert.

## Gate parité visuelle Skills desktop fermé — 5 août 2026

La comparaison vivante utilise la même Amazone niveau 1 et le même viewport
1 280×720 dans RuneWizard et BKVince. Elle isole les dimensions du cadre plutôt
que de juger la ressemblance à l'œil.

- la navigation mesure 175 px et le contenu environ 708 px dans le cadre commun
  de 884 px ;
- le panneau Skills utilise 16 px de padding, une toolbar de 24 px, un gap de
  16 px et une scène native de 420×548,6 px ; son origine diffère de la
  référence de moins de 0,1 px sur les deux axes ;
- `Ignore Game Rules` reprend le toggle 32×16 px, le texte 12/18 px et le gap
  8 px de RuneWizard ; le compteur reprend 16/24 px ;
- la pile `Montserrat, Verdana, sans-serif` reproduit exactement les largeurs
  observées de 116 px et 189 px pour les deux libellés de la toolbar ;
- les boutons latéraux deviennent flex, avec icône 24 px, gap 8 px, padding
  16 px et libellé 16/24 px. `Item Bonuses` ne se replie plus sur deux lignes ;
- le navigateur active l'override, ajoute Jab à 1 par clic puis le remet à 0
  par clic droit ;
- les huit bandes horizontales de 7 800 px ne sont plus déplacées par des
  translations subpixel. Le générateur produit 240 images individuelles
  130×130 à partir des sprites gouvernés, une pour chacun des 30 skills de
  chacune des huit classes ;
- les trois arbres Amazon ont été comparés nœud par nœud : mêmes 30 couples
  nom/fichier que RuneWizard, même fond par onglet et delta maximal de géométrie
  relative égal à 0 px. Le glow investi et le filtre verrouillé calculés sont
  identiques. La suite reste **78/78** et le build Vite est vert.

## Gate parité Quests / Waypoints desktop fermé — 5 août 2026

L'audit vivant reprend la même Amazone niveau 1 et le même viewport 1 280×720
dans les deux éditeurs. Il remplace les approximations issues des captures par
des mesures DOM comparables :

- Quests reproduit le padding 16 px, les actions, la note 676×87 px, les titres
  20/30 px, les actes 16/24 px, les rangées 23,49 px et les switches 32×16 px ;
- le panneau vide mesure 2 458,446 px contre 2 458,368 px dans RuneWizard. Après
  `Complete All`, il mesure 2 554,410 px contre 2 554,332 px : le delta reste
  **0,08 px** dans les deux états ;
- `Complete All` active les 81 quêtes et les trois `consumed_scroll`, comme la
  référence. Reset, Undo et Redo restaurent respectivement 0, 0 et 84 cases
  visibles sans erreur ni état orphelin ;
- Waypoints expose maintenant `Unlock All / Reset All`. Leur absence supposée
  venait d'une capture tronquée; la page RuneWizard vivante prouve ces actions ;
- Waypoints ferme une hauteur de contenu identique de **2 001,354 px**, une
  grille de 539,792 px, des cartes de 257,899 px, des rangées de 17,995 px et
  des toggles 32×16 px, avec les mêmes espacements 24×12 px ;
- les 117 waypoints passent 0→117 par les actions globales, puis Undo/Redo
  restaure les deux états. Une création niveau 1 commence avec les trois
  `Rogue Encampement` actifs, comme RuneWizard ;
- la suite passe **80/80**, le build Vite est vert, `git diff --check` est propre
  et le cadastre reste valide.

## Gate Item Bonuses / Chronicle / Mercenary desktop fermé — 5 août 2026

La même Amazone niveau 1 au viewport 1 280×720 ferme les trois surfaces
suivantes par comparaison DOM directe avec RuneWizard :

- `Item Bonuses` mesure 676,007×290,772 px dans les deux éditeurs; les quatre
  colonnes conservent leurs hauteurs de référence, un gap de 48 px, des titres
  16/24 px et des métriques 13,2/19,8 px. Le cadre complet ne diffère que de
  0,07 px à cause de l'arrondi du navigateur ;
- l'état vide `Chronicle` mesure exactement 676,007×77,101 px, avec titre
  20/30 px et bouton `Load Shared Stash` 146,927×31,102 px ;
- le chrome vide `Mercenary` reprend le titre 20/30 px, le switch 32×16 px et
  un libellé `died` de 17,994791 px, valeur identique à RuneWizard ;
- `Create mercenary` reste une extension BKVince volontaire : elle construit le
  record natif requis lorsqu'un héros n'en possède pas, ce qui sert directement
  les tests de plugins ;
- le navigateur prouve création, Name ID 0→321, Undo→0, Redo→321, suppression,
  restauration puis retour final à l'état sans mercenaire ;
- la suite reste **80/80**, le build Vite est vert, `git diff --check` est propre
  et le cadastre reste valide.

## Matrice de parité RuneWizard — audit vivant

| Fonction cible | État BKVince | Preuve ou limite gouvernée |
|---|---|---|
| Load D2S / création vierge / download | Fermé | Modale RuneWizard 800 px avec bouton de dossier et huit cartes de classes; chaque création porte le pack natif de dix objets : huit charms BK, Horadric Cube et Town Portal Scroll. Huit classes encodables, checksum/taille/reparse et no-op byte-exact; aucun preset. |
| Undo / Redo | Fermé | Historiques joueur, Shared Stash, Virtual Stash et Trash; raccourcis clavier et invalidation du futur testés. |
| General / Stats / flags / gold | Fermé | Fiche RuneWizard unique 538×471, champs 22 px en trois colonnes, Level/Map Seed non dupliqués, switches compacts, valeurs D2S natives, Undo/Redo et capsules d'or synchronisées. |
| Equipment / Inventory / Stash / Cube / Belt | Fermé | Composition RuneWizard 252/262/84 px, paper-doll 252×184, header/actions compacts et toasts hors flux; vraies grilles BKVince 11×8 gelée, 16×13, 6×6 et 4×4. La colonne gelée est soulignée, le pack natif de dix objets est positionné et validé par `HEStarterTen`; silhouettes, BodyLoc, belt capacity, collision, import, click-to-place, drag souris/tactile, Trash atomique et cycles D2R sont prouvés. |
| Shared Stash ordinaire | Fermé | Pages natives éditables; secteurs voisins et Chronicle byte-exacts; cycle D2R sur le profil 1 001 pages. |
| Shared Stash stackable | Fermé dans son ABI sûr | Compteur `UInt8` 0–255 éditable; une Ohm à 21 survit au chargement BKVince et à `Save and Exit`. Record, propriétés, imports, suppressions et coordonnées restent verrouillés. |
| Virtual Stash / Trash | Fermé côté éditeur | Workspaces locaux, drag inter-grilles ou vers Equipment à destination exacte, transferts target-first atomiques, Undo/Redo et bundle portable; aucun faux format natif inventé. |
| Click to add / groupes / import multiple | Fermé | Modale compacte 600 px : recherche et groupes immédiats, panier 38 px à sprites 30 px, action Add visible avant l'import et sélection invalide nettoyée après Undo. Catalogue mixte : 729 bases, 215 Sets, 473 Uniques et 112 Runewords; recherche nom/code/rune/propriété, base compatible, création Perfect au `Item Level 99` par défaut, panier libre, quick groups, `.d2i` et bundle fingerprinté atomiques. |
| Hover / click to edit / preview | Fermé | Tooltip Diablo avec `[click to edit]/[drag to move]`, seconde ligne `(Based on Character Level)` sur les 37 stats gouvernées, clic d'édition préservé pendant le drag et Item Editor fidèle à la hiérarchie RuneWizard : 800×648 px avec contrôles 400 px, gap 16 px et aperçu 320 px sur grand desktop, 600 px en compact avec 305/195 px, puis une colonne sous 520 px; attributs/set/socket avant les volets avancés, footer sticky et Save/réouverture/Undo/Redo prouvés. La teinte native `cgrn`, la pastille, l'ordre et les coordonnées de l'aperçu Harlequin correspondent à RuneWizard. Fermer ou sauvegarder efface la sélection transitoire et revient directement aux grilles, sans barre supplémentaire. |
| Variantes visuelles ring/amulet/charm/jewel | Fermé | 35 choix `picture_id` gouvernés, 23 DC6 vanilla convertis en PNG, aperçu/grille en direct et round-trip byte-exact. |
| Duplicate / Delete / Download item | Fermé | IDs neufs sans collision, corbeille réversible et export/reparse d'un record natif. |
| Low / Normal / Superior / Magic | Fermé codec/UI | Variantes Low 0–3, Magic 567/567 préfixes et 607/607 suffixes spawnable; témoins éditeur et runtime disponibles. |
| Set / Rare / Unique / Crafted | Fermé codec/UI et runtime ciblé | Set 215/215 et Unique 473/473 encodables sont recherchables et ajoutables directement avec identité/propriétés gouvernées; noms et six affixes Rare/Crafted; les neuf variantes Wraithstep/Opalvein survivent à `Save and Exit`. Les gros scalaires ne sont plus fragmentés; Umbral Disk à 63 produit exactement 862 Defense au niveau 99 et persiste après resauvegarde D2R. |
| Base / tier / item level / défense / durabilité | Fermé | Familles Normal/Exceptional/Elite, défense maximale lors de la création, upgrade/downgrade et préservation des payloads nommés. |
| Identified / Ethereal / Personalized | Fermé | Contrôles compatibles au record, nom ASCII 1–15 et conservation après reparse. |
| Attributs / affixes / Properties.txt | Fermé | Picker humain de 236 propriétés, formulaires générés pour les 24 familles d'encodage, 431 skills nommés, groupes sémantiques éditables, aperçu compilé exact avant ajout et matrice 236/236 export/reparse sans token non résolu; les 789 Monster IDs sont uniques et conservent leurs aliases. Les 390 stats avancées restent disponibles et les records compacts verrouillés. |
| Set bonuses / runewords | Fermé codec/UI, Chaos fermé runtime | Masques `plist`, listes permanentes et sélecteur `Add set attributes…` au roll Perfect sans codes techniques inactifs; 112/112 recettes recherchables, CTA→Flail, Plague→Phase Blade et Chaos→Katar. Chaos répare 1→8 puis 8→15 sur deux cycles parce que D2R réapplique la recette gouvernée; le payload persistant de l'éditeur porte une seule valeur native 63. |
| Sockets / fillers | Fermé | Nombre borné, insert/import/extract/delete, compactage canonique et aperçu des fillers. |
| Quantités d'items | Fermé codec/UI | Champs natifs gouvernés par `stackable/maxstack`, ajout en groupe et reparse; distincts du compteur Shared Stash. |
| Quests / Waypoints | Fermé | Quests suit Unlock Hell → note → Complete/Reset → difficultés et ferme 84 switches, récompenses `consumed_scroll` incluses. Waypoints expose les actions RuneWizard `Unlock All / Reset All`, 117 switches et les trois `Rogue Encampement` initiaux. Les deux panneaux reprennent les métriques desktop vivantes à 0,08 px près ou exactement, avec Undo/Redo et round-trip D2S prouvés. |
| Skills / Ignore Game Rules | Fermé | 24 fonds natifs Blizzard issus des huit `*Skilltree.sprite`; composition compacte 664/132/314 px puis breakpoint desktop RuneWizard mesuré à 884/175/420 px, contenu 708 px, padding 16 px et toolbar 24 px. Toggle 32×16 px, navigation 24/8/16 px et typographie `Montserrat, Verdana, sans-serif` reproduisent les métriques vivantes. Les 240 frames utiles sont servies comme images individuelles 130×130, sans translation d'un atlas géant; les trois arbres Amazon ferment noms, fichiers et rectangles avec un delta maximal de 0 px face à RuneWizard. Onglets, cadres, cases, flèches, glow investi et filtre verrouillé sont alignés; prérequis visuels, niveau requis vérifié au clic, compteur Unused informatif et stable, clic/Shift-clic/clic droit, Undo/Redo et override raw 0–255 restent prouvés sur Amazon, Barbarian et Warlock. |
| Item Bonuses | Fermé | Composition RuneWizard exacte en quatre colonnes sans chrome technique : Attributes, Resistances, Breakpoints et Misc. Les dix-neuf métriques visibles agrègent équipement, Runewords, fillers, groupes All Attributes/All Resistances et rolls par niveau; une matrice de six Uniques, export/reparse/no-op et le parcours Harlequin niveau 1→99 avec Undo/Redo sont prouvés. |
| Mercenary | Fermé | Création/retrait, 33 types gouvernés, header et douze slots `jf`, cycles D2R. Le chrome vide reprend titre 20/30 px, switch 32×16 px et libellé `died` exactement aligné; création, édition, suppression et Undo/Redo sont prouvés en direct. Le bouton de création reste l'extension BKVince nécessaire aux héros sans record natif. |
| Chronicle | Fermé | Secteur natif Set/Unique/Runeword, recherche/actions groupées et reparse isolé. L'état vide mesure exactement 676,007×77,101 px avec titre et bouton alignés sur RuneWizard. |
| Bound Demon | Hors priorité | Surface préservatrice disponible; Vincent a explicitement retiré ce gate. |
| Rotation d'item | Non applicable | Le format D2S/BKVince ne porte aucun bit de rotation prouvé; largeur/hauteur viennent des TXT et restent fixes. |
| Clavier / focus / tactile / erreurs / états vides | Fermé | Modales accessibles, Escape, boucle Tab, retour de focus, click-to-place clavier, drag tactile par appui maintenu, messages inline et aucune action cachée sous un item. Le tooltip `Click to add` s'ancre vers l'intérieur aux quatre bords des grilles et reste entièrement lisible après retour de focus. |
| Responsive visuel complet | Fermé | Shell 1 440/820/390 px, trois modales plein écran, Quests, Skills, Item Editor et Shared Stash vérifiés sans débordement; Item Editor 800 px au grand desktop, 600 px compact et empilé seulement sous 520 px; Load/Create reste accessible. |
| Découverte de classes custom | Ouvert gouverné | Huit classes prouvées; toute classe additionnelle exige tables, mapping ABI et trente skills réels. |

## Prochain gate

Poursuivre la matrice de parité RuneWizard sur les preuves codec/runtime et les
surfaces auxiliaires encore ouvertes. La limite des stats répétées est maintenant
tranchée sans modifier l'ABI `ItemStatCost`, Umbral Disk à 63 est mesuré en jeu
et la Shared Stash stackable à 21 est fermée par chargement, `Save and Exit` et
reparse. Le gate des formulaires d'attributs est maintenant fermé par une
matrice 236/236 et les parcours navigateur oskill, class skill, proc, plage de
dégâts et réanimation. Le prochain lot doit donc revenir aux autres écarts
visibles et interactifs que Vincent constatera encore face à RuneWizard, sans
rouvrir les encodages déjà prouvés. Le pack de départ complet est également
fermé par test des huit classes, parcours navigateur et cycle runtime
`HEStarterTen`.
Les imports, suppressions, propriétés et coordonnées de la page stackable restent
verrouillés par son ABI. Le Bound Demon n’est plus un gate.
