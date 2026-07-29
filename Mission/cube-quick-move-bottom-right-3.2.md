# Cube Quick Move Bottom-Right — D2R 3.2

## Statut et séquencement

- Statut : **CubeQuickMove 0.1.3 est validé en jeu sur l'épée `1x3`, puis
  intégré dans `plugin-misc.dll` sous `misc.cubeQuickMoveBottomRight`**.
- Cible : `D2R.exe 3.2.92777` sous D2RLoader.
- Vincent confirme le 27 juillet 2026 la catégorie future `misc`, le propriétaire
  `plugin-misc.dll` et la clé `misc.cubeQuickMoveBottomRight`.
- Vincent retient le séquencement A : après la clôture de Vendor Stock Refresh,
  ce chantier devient prioritaire avant Equipped Item to Cube.
- Pendant l’incubation, la fonctionnalité reste dans la DLL autonome hybride
  `CubeQuickMove.dll`, attribuée exactement à `RuffnecKk`, pilotée par un JSON
  autonome et sans TOML.

## Intention joueur

Lors d’un déplacement rapide vers le Cube, choisir la première position libre
selon un parcours pondéré partant du coin inférieur droit, quelle que soit la
taille de l’objet. Le glisser-déposer manuel et les autres conteneurs doivent
conserver leur comportement vanilla.

La demande source indique que les objets `1x1` sont aujourd’hui rangés en bas à
droite tandis que les autres remontent en haut à gauche. Le comportement natif
vérifié est légèrement plus large : le choix dépend de la **hauteur égale à 1**,
donc un objet `2x1` suit aussi la branche bas-droite.

## Friction observée et gain mesurable

- **Fait observé** — le rangement rapide du Cube change de direction selon les
  dimensions de l’objet, ce qui disperse visuellement les objets transférés.
- **Gain attendu** — pour `1x1`, `2x1`, `1x2`, `2x2` et `2x3`, le transfert
  rapide choisit la première case valide du même parcours bas-droite pondéré.
- **Non-régression mesurable** — aucun changement du glisser-déposer manuel, des
  déplacements rapides hors Cube, des sauvegardes ni des coordonnées d’objet.

## Faits vérifiés

- `npm run re:d2r32 -- status` est vert pour le build `92777` : image canonique
  SHA-256
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`, image
  d’analyse SHA-256
  `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`, index
  vérifié et projet Ghidra persistant présent.
- La référence sémantique épinglée
  `D2MOO@19019806df7f3e877fa105b05395d1e3597e2316` montre dans
  `D2Common/src/D2Inventory.cpp` que `INVENTORY_GetFreePosition` choisit le
  parcours bas-droite pour un propriétaire joueur lorsque la hauteur de l’objet
  vaut `1`; sinon il choisit le parcours haut-gauche. D2MOO 1.10f ne fournit
  aucune adresse ni ABI transposable au build 92777.
- L’équivalent 92777 `INVENTORY_FindFreePosition` est identifié au RVA
  `0x3865B0`. Le branchement `0x386735` compare la hauteur de l’objet à `1`;
  les routines natives pondérées sont `0x38D8F0` pour bas-droite et `0x38DCC0`
  pour haut-gauche.
- Le chemin indirect autour de `0x4BB8C0` vérifie le code de base `box ` et
  appelle `INVENTORY_FindFreePosition` au site `0x4BBA73`. Cette signature de
  cinq octets reste unique, mais le site n'est pas l'unique producteur
  automatique de coordonnées pour la page Cube.
- Le croisement des `36` xrefs de `INVENTORY_FindFreePosition` avec l'écriture
  explicite `C6 44 24 28 03` de l'argument de page prouve huit call-sites Cube :
  `0x0FA33D`, `0x2C7306`, `0x471D62`, `0x4BBA73`, `0x4C21D6`, `0x4F2C8B`,
  `0x527DC2` et `0x528053`. Tous portent un `CALL rel32` strict vers `0x3865B0`.
- La référence officielle
  `eezstreet/D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` est
  propre. Son `plugin-misc` ne possède que les clés
  `misc.playersCommandLimit`, `misc.monsterHpPlayerCountCap` et
  `misc.monsterExperiencePlayerCountCap`, avec des hooks étrangers au Cube et à
  l’inventaire. Aucun conflit de propriétaire ou de site n’est actuellement
  identifié.

## Retour gameplay du 28 juillet 2026

- **Échec confirmé par Vincent sur 0.1.0** — les objets `1x1` arrivaient au coin
  inférieur droit, mais les grands objets continuaient d'être placés en haut à
  gauche. Le chargement technique ne constituait donc pas une validation
  fonctionnelle.
- Le call-site runtime restait bien patché vers le relais de CubeQuickMove; la
  page `3`, l'ABI des dimensions et l'ordre des arguments ont été reconfirmés
  dans le workbench 92777.
- La dépendance ajoutée en 0.1.0 à
  `INVENTORY_SearchBottomRightWeighted 0x38D8F0` est retirée du chemin runtime.
  Le correctif 0.1.1 lit la grille d'occupation résolue par D2R et teste chaque
  ancre valide dans l'ordre droite vers gauche, puis bas vers haut. Ce parcours
  couvre réellement toute l'empreinte de l'objet et restaure les coordonnées
  vanilla au moindre échec.
- **Échec confirmé par Vincent sur 0.1.1** — l'épée continue d'arriver en haut à
  gauche. Le balayage n'était pas en cause : 0.1.1 ne possédait que
  `0x4BBA73`, alors que le build 92777 dispose de sept autres appels automatiques
  avec page Cube `3`, dont le chemin paquet de 21 octets autour de `0x4C21D6`.
- Le correctif 0.1.2 conserve un seul relais et un seul wrapper, mais remplace
  strictement les huit `CALL rel32` prouvés. Les placements explicites et la
  fonction partagée `INVENTORY_FindFreePosition` restent intacts.
- **Échec confirmé par Vincent sur 0.1.2** — l'épée continue d'arriver en haut à
  gauche. Une lecture directe du processus encore chargé prouve simultanément
  que les huit calls visaient le relais `0x3E80000` et que les quatre compteurs
  `CubeCalls`, `redirected`, `vanilla` et `safeFallbacks` étaient à zéro. Le
  Ctrl-clic testé n'empruntait donc aucun des huit producteurs page `3` écrits
  en dur.
- Parmi les `36` xrefs directs, neuf écrivent constamment une page non-Cube
  (`0`, `2` ou `4`). Les `27` autres comprennent les huit pages `3` explicites
  et dix-neuf pages dynamiques ou héritées. Le contrôle client `0x15A25C`
  reprend `r14b`, alimenté par `cl=3` à `0x15A2BB`; la branche serveur
  `0x4FBC0E` choisit la page `0` ou `3`, consomme les coordonnées puis appelle
  `ITEMS_PlaceItemForPlayer 0x471500`.
- **Succès confirmé par Vincent sur 0.1.3** — le 28 juillet 2026, le même
  Ctrl-clic place désormais visiblement l'épée `1x3` au coin inférieur droit du
  Cube.
- La sonde de ce témoin relève `EITC_TRANSFER_IN` avec destination native `3`,
  puis le paquet client `0x54` avec les coordonnées `4,3`. Le producteur réel de
  ce placement est l'appel dynamique `0x15F94F` dans la routine client
  `0x15F8B0` : ses sorties x/y sont copiées dans la structure de placement avant
  l'émission du paquet. Le serveur accepte ensuite le transfert (`result=1`).
  `0x15A25C` reste le contrôle préalable d'espace; `0x4FBC0E` appartient à une
  autre branche serveur et n'est pas le témoin de cette action.

## Implantation et preuves techniques

- `CubeQuickMove 0.1.3` remplace les 27 appels directs capables de transporter
  la page Cube par un relais proche partagé suivi d’un wrapper natif. Le wrapper appelle
  d’abord `INVENTORY_FindFreePosition` vanilla, conserve ce résultat pour les
  objets de hauteur `1`, puis reconstruit le contexte de grille natif et balaie
  directement les ancres libres pour les objets plus hauts. Toute précondition
  invalide, exception ou recherche infructueuse restitue les coordonnées
  vanilla et journalise le premier repli sûr.
- Les ABI et signatures strictes sont gouvernées pour
  `ITEMS_GetDimensions 0x371850`, le gate de hauteur `0x386735`,
  `INVENTORY_ResolveOccupancyGrid 0x38B070`,
  `INVENTORY_BuildGridContext 0x3C6D80` et les 27 call-sites capables de porter
  la page Cube listés ci-dessus.
- Le relais est alloué dans la plage positive d’un `rel32`, rendu RX avant
  l’installation du patch et conservé jusqu’à la fin du processus. D2RLoader
  possède les 27 patches de call-site; le plugin expose la commande
  `cube-quick-move` et journalise les appels Cube, redirections, replis vanilla
  et replis sûrs.
- Le build gouverné Release x64 passe son test de politique `1/1`. La DLL expose
  exactement `D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin` et
  `D2RLoaderUnloadPlugin`; son manifeste v2 porte `NativeHooks`, sans
  `ModScopedOnly`. SHA-256 DLL :
  `AF2E2282D562C2E8055B44B583C8430330669550B6D3A026632A6AB5FB2CF7C9`.
- `CubeQuickMove.json` expose uniquement `enabled`, utilise la priorité mod actif
  puis le repli global et refuse une configuration inconnue ou mal formée.
  SHA-256 JSON :
  `70CD57A3B6076C764A000C77E37EFCA407C20A3AC767986E95DA05B4CCE65C72`.
- Les cold starts 0.1.0 mod-local et global restent des preuves de portée, mais
  pas de gameplay. Le cold start frais 0.1.3 mod-local charge
  `CubeQuickMove.dll [mod]`, applique `20/20` patchsets, atteint `24/24`, active
  `28` plugins sur 30 avec deux désactivés, sans rejet ni échec. Les hashes DLL
  source/runtime sont identiques. Le wrapper observe d'abord `0x15A25C` pour un
  objet `1x3`, puis le producteur de placement `0x15F94F`; le paquet `0x54`
  transporte `4,3` et Vincent confirme le résultat visuel en bas à droite.
- L’archive candidate `addons/CubeQuickMove/CubeQuickMove.zip` contient
  strictement `CubeQuickMove.dll` et `CubeQuickMove.json`. SHA-256 ZIP :
  `B9D826DEFD02C9F6159002405D27E9507B3094B4ABE4868FD9E1BA2EC0614ED6`.

## Intégration au PluginPack — 28 juillet 2026

- Le checkpoint `9835aa8` porte la politique 0.1.3 dans `plugin-misc.dll` sous
  `misc.cubeQuickMoveBottomRight`, avec `enabled=false` dans le template joueur.
  Aucun call-site n'est donc redirigé par défaut.
- Les 27 appels Cube-capables sont les seuls sites ajoutés au manifeste. Leurs
  octets originaux sont exacts `27/27` dans l'image canonique 92777; les cinq
  signatures de helpers sont chacune uniques. Le manifeste atteint 123 sites à
  propriétaire unique.
- Les cinq DLL Release compilent, 15/15 CTest passent et `plugin-misc.dll`
  mesure `120832` octets avec le SHA-256
  `EB2EBA83A19E693A6FCE07D0807407A6B6D23351C23ED13205951312530D1463`.
- Le cold start vanilla atteint `24/24`, termine à
  `scanned=29 active=27 disabled=2 rejected=0 failed=0` et n'installe aucune
  redirection Cube.
- Le cold start actif atteint les mêmes compteurs; la lecture mémoire confirme
  que les 27 `CALL rel32` convergent vers l'unique relais `0x3E80000`. Aucun
  crash frais n'est créé.
- Le runtime est restauré byte-exact et aucun processus ne reste. La DLL et le
  JSON autonomes sont retirés de BKVince; les sources, l'archive et la preuve
  gameplay `1x3` à `4,3` restent l'oracle différentiel.
- Les preuves sont conservées sous
  `analysis-cache/pluginpack-foundation-runtime-validation/20260729-cube-quick-move-bottom-right/`.

Le cold start ne remplace pas l'équivalence gameplay intégrée. La matrice des
dimensions, de fragmentation, d'UI, de persistance et d'autorité reste ouverte.

## Hypothèses à tester et inconnues

- **Hypothèse** — souris et manette convergent vers l'un des 27 chemins de
  placement automatique gouvernés; la matrice runtime doit le confirmer.
- **Hypothèse** — le placement est autoritaire côté hôte. Un joiner qui charge
  seul le plugin ne devrait pas pouvoir imposer la position au serveur.
- **Inconnue** — une prédiction client peut afficher brièvement une autre case
  avant la réponse du serveur; les logs et une capture réseau doivent trancher.
- **Inconnue** — l'association exacte entre les autres origines UI et les
  call-sites dynamiques reste à inventorier; pour le Ctrl-clic testé,
  `0x15A25C` contrôle l'espace et `0x15F94F` produit les coordonnées envoyées au
  serveur.

## Architecture retenue

1. Conserver `INVENTORY_FindFreePosition` intact afin de ne pas modifier les
   inventaires, coffres et placements manuels globaux.
2. Vérifier strictement le build, la signature et l’ABI des 27 appels qui
   peuvent porter la page `3`, puis les remplacer par un relais partagé vers le
   wrapper autonome; laisser intacts les neuf appels constamment non-Cube.
3. Faire exécuter au wrapper la recherche vanilla, puis recalculer la position
   sur la grille d'occupation native avec un balayage borné bas-droite pour les
   objets de hauteur supérieure à une case lorsque la cible est bien le Cube et
   que l’option est activée.
4. En cas de précondition, signature ou recherche invalide, conserver le
   résultat vanilla sans altérer l’objet.
5. Distribuer pendant l’incubation uniquement `CubeQuickMove.dll` et son JSON
   autonome. Description retenue :
   `Places quick-moved Cube items from the bottom-right.`
6. Après validation autonome, préparer séparément une éventuelle promotion dans
   `plugin-misc.dll` sous `misc.cubeQuickMoveBottomRight`, sans modifier, lier ni
   redistribuer une DLL d’eezstreet.

## Gates observables

1. **Portée et ABI — franchi pour le témoin épée** : 36 xrefs inventoriés, neuf
   pages non-Cube exclues, ABI et signatures strictes gouvernées; `0x15A25C` et
   le producteur `0x15F94F` observés en runtime, paquet `0x54` à `4,3` accepté.
2. **Prototype autonome — franchi** : Release x64, test `1/1`, trois exports
   D2RLoader, auteur `RuffnecKk`, JSON anglais, aucun TOML et repli sûr.
3. **Dimensions et fragmentation — témoin `1x3` franchi sous 0.1.3** : Vincent
   confirme l'épée en bas à droite. La matrice complémentaire `1x1`, `2x1`,
   `1x2`, `2x2`, `2x3`, Cube vide, fragmenté et plein reste à rejouer après le
   port dans `plugin-misc.dll`; aucune superposition ni perte d’objet.
4. **Périmètre UI — non exécuté en jeu** : Ctrl + clic inventaire→Cube, autres
   origines indirectes, souris, manette et clics rapides; glisser-déposer manuel
   et autres conteneurs inchangés.
5. **Autorité et persistance — non exécuté en jeu** : solo, hôte, joiner,
   sauvegarde/rechargement, nouvelle partie et retour au menu; aucune
   duplication, désynchronisation ni migration de sauvegarde.
6. **Distribution — autonome prêt pour intégration, release finale ouverte** : portées
   globale et mod-locale, coexistence PluginPack, cold starts frais, hashes
   source/runtime et ZIP strict DLL + JSON sont prouvés. Le ZIP demeure une
   candidate technique jusqu’à validation des gates 3 à 5.

## Prochain gate

Rejouer le port intégré avec l'épée `1x3`, puis `1x1`, `2x1`, `1x2`, `2x2` et
`2x3` dans un Cube vide, fragmenté et plein. Fermer ensuite le périmètre UI, la
persistance et l’autorité hôte/joiner avant de déclarer la distribution finale.

## Frontière Git

Le lot autonome demeure l'oracle historique dans ses sources et son archive.
La distribution BKVince courante conserve uniquement `plugin-misc.dll` et
`D2RPlugins.json`; aucun binaire autonome CubeQuickMove n'y subsiste.
