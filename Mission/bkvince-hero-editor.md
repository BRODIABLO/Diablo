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

- [ ] Le parcours Tranche 1 ouvre ou crée un héros, modifie Général/Stats,
  undo/redo et exporte un D2S reparseable dans l'UI cible.
- [ ] Les fixtures no-op possèdent une politique byte-exact ou une allowlist
  documentée octet par octet, avec checksum et champs recalculés exclus de
  toute fausse alerte.
- [ ] Les catalogues sont régénérables depuis les TXT BKVince courants et leurs
  tests échouent proprement sur header requis absent, clé dupliquée ou donnée
  ambiguë.
- [ ] Chaque conteneur refuse les chevauchements, débordements et placements
  illégaux sans corrompre le document ni perdre un objet.
- [ ] Les qualités, affixes, propriétés, sockets et quantités exportées sont
  relues avec les mêmes valeurs et validées sur des objets BKVince réels.
- [ ] Les onze classes prises en charge par BKVince, leurs skills et leurs
  données custom sont découvertes depuis les sources actuelles plutôt que
  codées selon le roster vanilla.
- [ ] Quests, waypoints, mercenaire, stashes et fichiers auxiliaires passent
  chacun une matrice load/edit/save/reload.
- [ ] La matrice visuelle couvre desktop et responsive : navigation, grille,
  tabs, panneaux spatiaux, modals, focus, erreurs et états vides.
- [ ] D2R 3.2.92777 accepte les exports BKVince témoins, puis les recharge après
  `Save and Exit` sans rollback, perte d'objet, bad inventory ou bad dead body.
- [ ] Les sauvegardes sources restent inchangées et aucun octet de personnage
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

## Prochain gate

Restaurer l'ancien workspace dans une tranche bornée, figer des fixtures D2S
anonymisées et livrer le premier parcours complet : **ouvrir ou créer →
Général/Stats → undo/redo → télécharger**, avec le shell visuel cible et une
preuve de préservation byte/checksum/reparse. Ne pas commencer l'inventory tant
que ce gate n'est pas vert.
