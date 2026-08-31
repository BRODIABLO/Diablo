# AutoSort — tri configurable de l’inventaire et du coffre courant

Derniere mise a jour : 31 aout 2026

## Décision produit

Vincent autorise l’implantation le 25 août 2026. AutoSort est une **DLL
autonome RuffnecKk** membre de la RuffnecKk D2RLoader Suite, versionnée,
configurée et livrée indépendamment de Bulk Currency Deposit. Elle reste
hybride : installation globale ou mod-locale, sans `ModScopedOnly`.

Vincent autorise le 31 aout 2026 la migration minimale du prototype vers la
baseline D2RLoader 1.2 / PluginSDK API v4. Cette migration ne change ni le
comportement joueur ni le trajet transactionnel natif.

Le séquencement retenu est **AutoSort maintenant**, tout en conservant le
chantier Steam/Battle.net actif derrière son gate testeur externe. L’autre
séquencement évalué consistait à attendre ce retour Steam avant AutoSort; il
n’apportait aucune preuve utile au tri et aurait immobilisé un chantier
indépendant.

## Résultat joueur retenu

- Une même action trie uniquement le conteneur visible : inventaire joueur ou
  onglet de coffre courant. Aucun autre onglet n’est touché.
- Les categories sont configurables : armor, weapons, rings/amulets, charms,
  potions, keys, scrolls, books, runes, gems, jewels, quest items, misc et
  custom mod items.
- Les destinations symboliques couvrent les neuf ancres
  `top_left/top_middle/top_right`, `middle_left/middle/middle_right` et
  `bottom_left/bottom_middle/bottom_right`. `middle` désigne une zone centrale
  compacte, pas une coordonnée unique.
- Les règles custom peuvent cibler des item codes, item type codes et qualités.
  La première règle correspondante gagne afin de rendre les conflits
  déterministes et explicables.
- Le classement visuel est prioritaire sur le compactage : categorie, groupe
  custom, item code semantique, puis ancre. Les exemplaires identiques restent
  ensemble; les potions suivent healing `hp1..hp5`, mana `mp1..mp5`, puis
  rejuvenation `rvs` avant `rvl`, tandis que les runes suivent `r09`, `r10`.
  Parmi les plans qui respectent cette cohesion, le solveur favorise
  ensuite l'espace utile libre. Le resultat doit etre deterministe et
  idempotent : relancer AutoSort sur un etat deja trie ne doit produire aucun
  deplacement.
- Potions, keys, scrolls et books sont des categories de premier rang. Le vrai
  `itemtypes.txt` BKVince ne declare aucun type generique `curr`; une monnaie
  mod-specific encore presente dans la grille se classe donc par `custom_rules`
  plutot que par une fausse categorie integree. AutoSort ne route jamais ces
  items vers Advanced Stash et ne remplace pas Bulk Currency Deposit.
- L'action native a la manette, l'action Controls SDK configurable avec
  `Shift+H` par defaut et un eventuel bouton opt-in doivent converger vers le
  meme planificateur. Si la DLL refuse de charger, le comportement AutoSort
  vanilla doit rester intact.

## Architecture retenue

AutoSort installe un seul hook inline fail-closed sur le planificateur natif
`INVENTORY_AutoSortPlanner 0x15E790`. Le hook appelle d'abord le planificateur
original afin de conserver ses validations, construit ensuite un plan complet
pur, puis remplace la totalite du tableau de destinations seulement si toutes
les pieces tiennent. Un echec produit donc zero mouvement.

L'action Controls appelle sans modification le wrapper natif
`INVENTORY_AutoSort 0x160B00`, deja emprunte par le comportement AutoSort du
jeu. Le wrapper envoie le paquet natif `0x4E`; le callback serveur autoritaire
valide le nombre d'items, les dimensions, les coordonnees, l'occupation et les
conflits avant toute mutation. AutoSort n'ecrit jamais directement dans les
coordonnees d'item ou la sauvegarde.

AutoSort ne hooke ni `INVENTORY_FindFreePosition 0x3865B0`, ni l'emetteur de
paquet, ni le callback serveur, ni `ITEMS_GetInvPage`. L'action manette et
l'action clavier convergent ainsi vers le meme planificateur et le meme trajet
transactionnel natif. Le bouton opt-in reste differe.

## Faits vérifiés

- Le workbench natif commun aux cibles D2R `3.2.92777` et `3.3.93847` est prêt.
  Image canonique :
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`;
  image d’analyse :
  `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`.
- La référence PluginPack épinglée est
  `eezstreet/D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`;
  elle ne contient ni AutoSort ni référence à `INVENTORY_FindFreePosition`.
- La baseline officielle emploie D2RLoader `1.2` et PluginSDK API v4 au commit
  `6eb8f8b6192868214706bd6d528c5294f2f551b7`.
- `ItemServiceV1::executeExistingItemTransaction` ne couvre au plus que 64
  operations et exclut notamment le shared stash, tandis que le wrapper natif
  AutoSort enumere jusqu'a 256 items. `InventoryServiceV1` n'expose pas de
  transaction bulk same-container equivalente. La migration v4 conserve donc
  un seul trajet transactionnel natif au lieu d'introduire un repli divergent.
- Le wrapper `INVENTORY_AutoSort 0x160B00` enumere au plus 256 items de la page
  demandee, appelle `INVENTORY_AutoSortPlanner 0x15E790`, applique les
  destinations puis appelle `CLIENT_SendAutoSortPacket 0xECC70`.
- Le paquet natif utilise l'opcode `0x4E`, la page, le GUID joueur, le nombre
  d'items et cinq octets par item. `SERVER_AutoSortCallback 0x4AE440` prouve le
  contrat same-container atomique et autoritaire.
- `INVENTORY_BuildGridContext 0x3C6D80` produit un descripteur de 24 octets dont
  la largeur et la hauteur sont les octets `+0` et `+1`. Les octets
  `+0x10/+0x11` appartiennent a la structure d'occupation resolue et ne sont
  pas les dimensions de ce descripteur.
- L'audit read-only de `itemtypes.txt` BKVince compte 118 lignes, conserve CRLF
  et confirme les types reels `poti`, `rune`, `gem`, `jewl`, `ques` et `misc`,
  sans type `curr`. Aucune table n'a ete modifiee.
- L'audit read-only des 278 lignes de `misc.txt` prouve que BKVince reutilise
  `spot`, enfant de `poti`, pour des Tokens, Essences, keys et quest materials.
  Tester directement le parent `poti` les classerait donc a tort comme potions.
  Le classificateur courant utilise les familles concretes `hpot`, `mpot`,
  `rpot`, `apot`, `wpot`, `tpot` et le code stamina `vps`; aucune table n'a ete
  modifiee.
- L’ancienne source tierce D2RHUD 2.4 nomme une fonction
  `INVENTORY_AutoSort(unitId,page)`, mais son motif du build `1.4.71510` ne
  correspond pas au corpus courant. Cette source est une piste historique,
  jamais une preuve d’adresse, de signature ou d’ABI pour `92777/93847`.
- La pile complete D2R `3.3.93847` mod-locale a accepte l'empreinte native
  AutoSort. L'action Controls a bien recu `Shift+H`; le premier diagnostic a
  refuse proprement le plan a cause de la mauvaise lecture des dimensions.
- Apres correction de `+0x10/+0x11` vers `+0/+1`, Vincent confirme que le tri
  fonctionne en inventaire. Cette observation ferme le trajet clavier,
  planificateur, paquet et serveur sur ce scenario.
- La premiere politique visuelle reste rejetee : les runes et jewels occupent
  des trous et les potions de types ou tiers differents sont melangees. La
  candidate de cohesion par categorie et item code naturel deployee avant la
  consigne d'arret des tests vaut
  `B01DA6E1EC33E59A14D4903E41AD2CD5796D40D018DEF586B111CC7BB2CDF6BD`;
  aucun temoin gameplay de cette candidate n'a ete recueilli.
- La candidate `EF55953E...DEC44` conserve cette cohesion en premiere phase et
  ajoute un
  repli dimensionnel seulement si aucun plan strict ne tient dans une grille
  tres remplie. Cette ancienne candidate laissait le plus grand rectangle libre
  gagner avant la distance aux ancres; le diagnostic ulterieur prouve que cette
  priorite pouvait annuler une destination TOML comme `middle_left`. Il compile en Release avec
  `BUILD_TESTING=OFF`, `/W4`, `/WX`, `/permissive-` et `/EHsc`; la DLL locale
  non deployee mesure 423 936 octets et vaut
  `EF55953E7964B1C3A762C6C575626D107CEEFBEDB9EE181E17ED2EDFF01DEC44`.
  Aucun CTest n'avait ete lance sur cette candidate avant son deploiement.
- Le 28 aout 2026, cette DLL et le TOML courant ont ete deployes dans la portee
  mod-locale BKVince sans lancer le jeu. Les hashes runtime concordent avec les
  sources : DLL
  `EF55953E7964B1C3A762C6C575626D107CEEFBEDB9EE181E17ED2EDFF01DEC44`
  et TOML
  `C69D784EE9B33E76E77E3CD01BFB8F44737D4AA7C2E9DE6AC28AA09CFA79D4A0`.
  Le runtime installe est D2R `3.3.93847`; son `.build.info` vaut
  `2EBCAD0521DBF038D5A7FE5395E96B4BEF6D4F0774F7B1F840E03C3DE9CB067A`
  et `D2R.exe` vaut
  `E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`.
  L'ancienne DLL `B01DA6E...CDF6BD` et son TOML sont conserves dans un rollback
  local sous `analysis-cache/runtime-rollbacks/autosort/`. Chargement, cold
  start et gameplay restaient alors `NOT RUN`.
- Le temoin de Vincent du 28 aout prouve que le binding `Shift+H` fonctionne :
  le log frais enregistre `default Shift+H`, puis la pression atteint le plugin
  a 15:17:10. La candidate `EF55953E...DEC44` refuse cependant atomiquement
  `page=0 grid=11x8 items=78` avec `packing failed`; zero item ne bouge. Le
  probleme n'est donc ni le binding ni le chargement, mais l'incompletude du
  packing glouton sur une grille presque pleine.
- Le source corrige ce cas par un troisieme repli d'urgence, execute seulement
  apres les familles cohesion-first et category-dimension-first. Il place alors
  globalement les objets mult cases avant les `1x1` tout en conservant leurs
  ancres et le determinisme. Le fixture exact `11x8 / 78 items`, avec un `2x2`
  et un `1x3` tardifs, passe; CTest vaut `1/1`. Deux builds Release x64 propres
  `/W4 /WX /permissive- /EHsc /Brepro` concordent a 246 784 octets et SHA-256
  `6219A9EA363A0FCB6A2CD3B87AE2A5EC0CBED3C660ECB83832D6EF6FDC1DD3AD`.
  La DLL PE 0.1.0 expose exactement les trois exports D2RLoader attendus.
- Cette nouvelle DLL et le TOML
  `D41A2EEB84A6282A9DEC233FEB37CD72DFE3F99FF8765D6942F3D1AB572DB2FD`
  sont deployes byte-identiques en portee mod-locale BKVince. La candidate
  precedente et son TOML sont sauvegardes sous
  `analysis-cache/runtime-rollbacks/autosort/2026-08-28-pre-6219A9EA/`.
  Le jeu n'a pas ete relance par l'agent; chargement et gameplay du correctif
  global restent `NOT RUN`.
- Vincent teste ensuite cette candidate sur la grille reelle. La transaction
  fonctionne, mais le resultat visuel est **FAILED** : les healing potions, les
  mana potions et les rings ne restent pas chacun dans un bloc coherent. Le
  repli global a donc resolu la faisabilite en sacrifiant l'objectif produit.
  Cette observation invalide le modele ou le regroupement n'est qu'un ordre de
  passage avant un placement individuel par ancre.
- Vincent retient le 28 aout 2026 une hierarchie explicite
  `categorie -> sous-groupe -> item code/tier/qualite`. Armor contient notamment
  body armor, helms, shields, gloves, belts et boots; weapons contient les
  familles concretes; jewelry separe rings et amulets; charms separe les
  tailles; potions separe healing, mana, rejuvenation, utility et throwing.
  L'ordre des sous-groupes est configurable en TOML.
- La meme decision ajoute des `[[exclusions]]` prioritaires : un item exclu
  reste exactement a ses coordonnees et devient un obstacle fixe. Les regles
  ciblent item codes, item type codes et qualites. Elles precedent les
  `[[custom_groups]]`, qui precedent les categories integrees; le spelling
  historique `[[custom_rules]]` reste accepte seul pour compatibilite.
- Le premier candidat de cette nouvelle architecture est volontairement
  **inspect-only**. `diagnostics.enabled`, `dry_run` et `log_items` sont actifs;
  chaque item journalise GUID, code, types resolus, qualite, dimensions,
  position, source, groupe, categorie, sous-groupe, ancre et exclusion, puis la
  transaction est refusee sans mouvement. Les tests couvrent les sous-groupes
  armor/weapon/jewelry/potions, la priorite exclusion > custom group, les
  ordres TOML complets et les configurations invalides; CTest vaut `1/1`.
- Deux builds Release x64 propres de ce candidat concordent a 276 992 octets et
  SHA-256
  `79A5C239F9248DD0CEB7709817BF015F446C50FAFD785791FFA38AC29EE773CF`.
  La DLL PE 0.1.0 expose exactement les trois exports D2RLoader. Le TOML vaut
  `E726DA11AC6D45C7B68ED7F3C1606CA43CE3BAB8E03D7A334A12081EA57C97C5`.
  Les deux fichiers sont deployes byte-identiques en portee mod-locale BKVince;
  la candidate `6219A9EA...DD3AD` est sauvegardee sous
  `analysis-cache/runtime-rollbacks/autosort/2026-08-28-pre-79A5C239/`.
  Aucun processus D2R n'a ete relance par l'agent.
- Le candidat hierarchique suivant est charge mod-local et Vincent confirme que
  la transaction reelle reste visuellement **FAILED** malgre un planner rapide :
  `33/34` objets deplaces, `677 us` et rectangle libre `9x3`. Les rejuvenation
  potions et les runes ne forment pas les blocs attendus; des runes apparaissent
  entre des potions et le Cube est envoye au centre. La performance n'est donc
  plus le probleme prioritaire et ce temoin n'est pas transforme en succes.
- Le diagnostic du 28 aout identifie deux causes de politique. Premierement, le
  TOML public avait laisse actifs des exemples de `subgroup_anchors` : healing
  et rejuvenation allaient en `top_right`, mana en `middle_right`, rings en
  `bottom_right` et amulets en `bottom_left`. Ces valeurs divisaient
  volontairement les categories que le defaut devait garder continues.
  Deuxiemement, les preuves runtime ont revele deux representations des codes
  courts : `ITEMS_GetItemCode` renvoie les codes d'objets completes par NUL,
  tandis que les records compiles d'ItemTypes utilisent l'espace ASCII. Le
  candidat qui imposait `0x20786F62` (`box `) partout resolvait les types mais
  cassait les selecteurs `box`, `rvs`, `rvl` et `r01` issus des objets.
- Le correctif normalise maintenant les deux representations D2 vers une forme
  canonique completee par NUL avant toute comparaison, retire les sous-ancres
  actives du defaut livre et garde
  ces exemples commentes. Healing, mana et rejuvenation heritent tous de
  l'ancre potion `top_right`; rings et amulets heritent ensemble de l'ancre
  jewelry. Les overrides restent disponibles et divisent une categorie
  seulement lorsqu'un joueur les active explicitement.
- Vincent precise ensuite que le Cube ne doit pas etre exclu : c'est un quest
  item que les joueurs conservent normalement dans l'inventaire, mais le
  moddeur doit choisir sa destination. Le TOML livre donc un
  `[[custom_groups]]` actif `Horadric Cube`, selecteur `item_codes = ["box"]`
  et ancre independante `middle_left`. Cette ligne se modifie sans deplacer les
  autres quest items et conserve la priorite custom > categorie integree.
- Deux builds Release x64 propres du correctif concordent a 321 536 octets et
  SHA-256
  `6DB24A42B9C9252873CDE66D12FDF53741328B066F3355CA258E996D947EED13`.
  Chaque build passe CTest `1/1`; la DLL PE 0.1.0 expose exactement les trois
  exports D2RLoader. Le TOML de 4 440 octets vaut
  `156C6562F931CF1B8110ABE19692F87FE0561642880A69F30BB58A0DA4621419`.
  Ces tests imposaient a tort la forme espacee a tous les codes courts.
- DLL et TOML sont deployes byte-identiques en portee mod-locale BKVince sur la
  baseline officielle D2R `3.3.93847`. Le `.build.info` conserve
  `2EBCAD05...B067A` et `D2R.exe` `E1F5436E...CC8936`. Le candidat precedent
  est sauvegarde sous
  `analysis-cache/runtime-rollbacks/autosort/2026-08-28-pre-6DB24A42/`; le
  rollback anterieur `2026-08-28-pre-557E7389/` conserve egalement la candidate
  qui precedait le premier correctif.
  Aucun processus D2R n'etait actif et l'agent n'a ni lance le jeu ni execute
  de test gameplay apres ce deploiement.
- Le temoin gameplay suivant invalide ce candidat : les potions et runes restent
  visuellement fragmentees et le Cube reste au centre. Les anciens logs
  d'inspection prouvent que `ITEMS_GetItemCode` fournit les codes courts avec
  NUL alors que la resolution d'ItemTypes exige la variante espacee. Un second
  test pur reproduit aussi le Cube hors de `middle_left` lorsque le rectangle
  libre est prioritaire sur les ancres.
- Le nouveau candidat local normalise les deux paddings, donne aux ancres et a
  `category_order` priorite sur l'optimisation d'espace, puis n'utilise les
  ordres de blocs alternatifs qu'en repli si l'ordre configure ne tient pas.
  Deux builds Release x64 concordent a 322 048 octets et SHA-256
  `AA7F34AD3B60E64000C796F29BC46BE6CB75FB64780CC378CD417C0CC3B90880`.
  CTest passe `1/1` deux fois, y compris le fixture observe `11x8`, la cohesion
  potions/runes, `box / box ` et le Cube exactement a `0,3` pour
  `middle_left`. Ce candidat n'est pas deploye pendant la partie active.
- Le 31 aout 2026, la migration minimale porte ce meme code comportemental a
  AutoSort `0.1.1`, D2RLoader `1.2` et PluginSDK API v4 au commit exact
  `6eb8f8b6192868214706bd6d528c5294f2f551b7`. Deux configurations et builds
  Release x64 independants passent CTest `1/1`, produisent chacun une DLL de
  325 120 octets et concordent au SHA-256
  `BAB688B90347ADF8CACA5D2F3BE11BEBDCB7810A7151CD69425F2E5387F7BD17`.
  La ressource manifeste contient exactement l'API `4`, la version PE est
  `0.1.1` et les trois seuls exports sont `D2RLoaderGetPluginInfo`,
  `D2RLoaderLoadPlugin` et `D2RLoaderUnloadPlugin`. Aucun service de transaction
  v4 ni second chemin de tri n'est introduit. Cette DLL n'est pas deployee et
  aucune validation runtime ou gameplay n'est revendiquee par cette migration.

## Hypotheses a tester

- Les codes et types observes sur la grille reelle correspondent aux nouveaux
  sous-groupes; tout item mod-specific restant en `unknown` devra etre route
  par `custom_groups` plutot que devine par le plugin.
- Un solveur qui traite categorie et sous-groupe comme contraintes spatiales,
  puis optimise les ancres et l'espace libre, peut garder la grille `11x8 / 78`
  faisable sans intercaler healing, mana, rings ou familles d'equipement.
- Le meme trajet cible seulement l'onglet de coffre actuellement visible,
  incluant les pages shared stash exposees par le profil actif.
- Un second lancement sur un conteneur deja trie n'envoie aucun deplacement.

## Inconnues ouvertes

1. Resultat visuel du candidat `AA7F34AD...90880` sur une grille representative,
   incluant les deux formes natives des codes courts et les custom mod items.
2. Comportement sur l'onglet de coffre courant et les pages shared stash proxy.
3. Equivalence gameplay de l'entree manette; Vincent ne possede pas de manette
   pour ce temoin.
4. Qualification du meme artefact sur D2R `3.2.92777`, en portee globale, en
   save/reload et en host/joiner.
5. Bouton opt-in sans second proprietaire de `UI_DispatchMessage`; il reste
   hors du lot courant.

## Plan d’implantation

1. Termine : prouver le trajet natif, promouvoir ses RVA et couvrir toutes les
   surfaces utilisees par une empreinte fail-closed sans allowlist de version.
2. Termine statiquement : implanter la classification, les ancres, le solveur,
   les regles custom, le TOML strict et l'action Controls commune.
3. Termine en gameplay de base : charger mod-local sur `3.3.93847` avec la pile
   complete et confirmer une transaction de tri en inventaire.
4. Termine statiquement : le fallback global par dimensions est retire et
   chaque candidat doit conserver des blocs categorie/sous-groupe/code
   connectes. Le candidat corrige les codes courts et les defaults qui
   fragmentaient volontairement potions et jewelry. La migration minimale
   `0.1.1` vers SDK API v4 est compilee et verifiee sans changement du moteur.
   Le candidat SDK v4 n'est pas deploye et attend le temoin visuel de Vincent.
5. Ensuite : verifier l'idempotence et le coffre courant, puis les matrices de
   configuration, fingerprint negatif, portees, builds, save/reload et reseau.

## Matrice de validation

| Gate | Attendu | État |
|---|---|---|
| Planificateur pur | categories, sous-groupes, exclusions, cascade item > sous-groupe > categorie > defaut, `middle`, idempotence | passed: SDK v4 CTest 1/1 sur deux builds; fixture 11x8, normalisation NUL/espace et Cube `0,3` |
| Diagnostic sans mouvement | chaque item logge puis transaction refusee | disponible via `dry_run=true` + `log_items=true`; non requis pour le prochain temoin nominal |
| Cohesion visuelle | categorie, sous-groupe, code naturel, tiers, exemplaires identiques | `6DB24A42...EED13` failed; meme correctif rebati SDK v4 `BAB688B9...7BD17` not run |
| Espace utile | grand rectangle apres cohesion et ancres | only hierarchy-valid candidates may compete; global dimension fallback removed; gameplay not run |
| Refus atomique | plan impossible = zero mouvement | passed statically and observed fail-closed |
| Deploiement courant | build/TOML = runtime mod-local | runtime reste `6DB24A42...EED13`; candidat SDK v4 `BAB688B9...7BD17` non deploye |
| Inventaire courant | transaction native de tri | trajet passe auparavant; resultat du candidat corrige not run |
| Coffre courant | onglet visible seulement | not run |
| Entrees | Controls et manette utilisent le meme wrapper/planner | Shift+H reached previous planner; corrected candidate not run; controller not run |
| Configuration | absente, valide, invalide, mod-local puis global | partial static; runtime matrix not run |
| Fingerprint | acceptation exacte et refus de chaque temoin altere | exact 3.3 accepted; negative matrix not run |
| D2R 3.2.92777 | global, mod-local, gameplay, coexistence complete | not run |
| D2R 3.3.93847 | global, mod-local, gameplay, coexistence complete | mod-local base path passed; rest not run |
| Sauvegarde | save/reload sans perte, duplication ni coordonnée invalide | not run |
| Hôte/joiner | autorité et résultat cohérents | not run |

## Rollback

Retirer la DLL et son TOML. AutoSort ne modifie aucun format de sauvegarde ni
table TSV. Tant que la DLL n’est pas chargée, l’action manette vanilla demeure
inchangée.

## Prochain gate

Le runtime actif contient encore le candidat invalide `6DB24A42...EED13`.
Le correctif local SDK v4 `BAB688B9...7BD17` passe deux builds et CTest mais
n'est pas deploye. Apres une autorisation distincte de deploiement, le prochain
gate sera un deploiement mod-local puis une seule pression de `Shift+H`. Le temoin doit
confirmer que le Cube rejoint son cas
special `middle_left` (ou l'ancre que Vincent choisira dans le TOML), que
healing, mana et rejuvenation forment
trois sous-blocs continus dans un seul bloc potion, que les tiers suivent
`hp1..hp5`, `mp1..mp5`, `rvs..rvl`, que les runes restent ensemble en
`r01..r33`, et qu'aucune rune ne se place entre des sous-groupes potion. La
latence doit rester imperceptible. Cold start, log frais, coffre, idempotence,
portee globale, save/reload et reseau restent ouverts apres ce verdict visuel.

## Frontière Git

Le lot couvre ce fichier, `addons/AutoSort/**`, les promotions strictement
nécessaires dans le workbench natif, `Mission/WORKSTREAMS.json`,
`Mission/CURRENT.md`, `ROADMAP.html` et le cadastre partagé. Le chantier
Steam/Battle.net demeure actif mais hors de ce lot. Aucun commit, push, tag ni
asset GitHub n’est autorisé par ce GO.
