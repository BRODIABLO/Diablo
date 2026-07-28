# Cube Quick Move Bottom-Right — D2R 3.2

## Statut et séquencement

- Statut : **prototype autonome hybride 0.1.0 implanté et techniquement validé;
  matrice gameplay encore ouverte**.
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
- Le chemin indirect vers le Cube autour de `0x4BB8C0` vérifie le code de base
  `box `, sélectionne la page native `3`, résout sa grille et appelle
  `INVENTORY_FindFreePosition` au site `0x4BBA73`.
- Les cinq octets de cet appel relatif, `E8 38 AB EC FF`, n’apparaissent qu’une
  fois dans la section `.text` du binaire canonique.
- La référence officielle
  `eezstreet/D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` est
  propre. Son `plugin-misc` ne possède que les clés
  `misc.playersCommandLimit`, `misc.monsterHpPlayerCountCap` et
  `misc.monsterExperiencePlayerCountCap`, avec des hooks étrangers au Cube et à
  l’inventaire. Aucun conflit de propriétaire ou de site n’est actuellement
  identifié.

## Implantation et preuves techniques

- `CubeQuickMove 0.1.0` remplace uniquement l’appel relatif au site
  `0x4BBA73` par un relais proche suivi d’un wrapper natif. Le wrapper appelle
  d’abord `INVENTORY_FindFreePosition` vanilla, conserve ce résultat pour les
  objets de hauteur `1`, puis reconstruit le contexte de grille natif et appelle
  `INVENTORY_SearchBottomRightWeighted` pour les objets plus hauts. Toute
  précondition invalide, exception ou recherche infructueuse restitue les
  coordonnées vanilla.
- Les ABI et signatures strictes sont gouvernées pour
  `ITEMS_GetDimensions 0x371850`, le gate de hauteur `0x386735`,
  `INVENTORY_ResolveOccupancyGrid 0x38B070`,
  `INVENTORY_SearchBottomRightWeighted 0x38D8F0`,
  `INVENTORY_BuildGridContext 0x3C6D80` et le call-site Cube `0x4BBA73`.
- Le relais est alloué dans la plage positive d’un `rel32`, rendu RX avant
  l’installation du patch et conservé jusqu’à la fin du processus. D2RLoader
  possède le patch du call-site; le plugin expose la commande
  `cube-quick-move` et journalise les appels Cube, redirections, replis vanilla
  et replis sûrs.
- Le build gouverné Release x64 passe son test de politique `1/1`. La DLL expose
  exactement `D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin` et
  `D2RLoaderUnloadPlugin`; son manifeste v2 porte `NativeHooks`, sans
  `ModScopedOnly`. SHA-256 DLL :
  `AEABDA0B8ED90765A752B22988F862B995B1739EADDB6A55D46D81D76D22C833`.
- `CubeQuickMove.json` expose uniquement `enabled`, utilise la priorité mod actif
  puis le repli global et refuse une configuration inconnue ou mal formée.
  SHA-256 JSON :
  `70CD57A3B6076C764A000C77E37EFCA407C20A3AC767986E95DA05B4CCE65C72`.
- Les cold starts frais mod-local et global chargent respectivement
  `CubeQuickMove.dll [mod]` et `[global]`, activent le relais au RVA
  `0x3E80000`, appliquent `20/20` patchsets et atteignent `24/24` sans échec du
  plugin. L’état final est restauré en portée mod-locale avec les hashes
  source/runtime identiques. Le dernier démarrage signale séparément le rejet
  d’un artefact concurrent `EquippedItemToCubeProbe.dll`; CubeQuickMove reste
  chargé et actif.
- L’archive candidate `addons/CubeQuickMove/CubeQuickMove.zip` contient
  strictement `CubeQuickMove.dll` et `CubeQuickMove.json`. SHA-256 ZIP :
  `2FE7079C726B5668995843523971C48A1FE9D6B95A57C62D3389E73DCD5DD30F`.

## Hypothèses à tester et inconnues

- **Hypothèse** — souris et manette convergent vers le même paquet de déplacement
  indirect; la portée exacte du site `0x4BBA73` doit le confirmer en runtime.
- **Hypothèse** — le placement est autoritaire côté hôte. Un joiner qui charge
  seul le plugin ne devrait pas pouvoir imposer la position au serveur.
- **Inconnue** — une prédiction client peut afficher brièvement une autre case
  avant la réponse du serveur; les logs et une capture réseau doivent trancher.
- **Inconnue** — le site unique peut couvrir d’autres transferts indirects vers
  la page Cube. Chaque origine doit être inventoriée avant d’affirmer que la
  portée correspond exactement au Ctrl + clic.

## Architecture retenue

1. Conserver `INVENTORY_FindFreePosition` intact afin de ne pas modifier les
   inventaires, coffres et placements manuels globaux.
2. Vérifier strictement le build, la signature et l’ABI du site `0x4BBA73`, puis
   remplacer uniquement cet appel relatif par un wrapper autonome.
3. Faire exécuter au wrapper la recherche vanilla, puis recalculer la position
   avec la routine native bas-droite pour toutes les hauteurs lorsque la cible
   est bien le Cube et que l’option est activée.
4. En cas de précondition, signature ou recherche invalide, conserver le
   résultat vanilla sans altérer l’objet.
5. Distribuer pendant l’incubation uniquement `CubeQuickMove.dll` et son JSON
   autonome. Description retenue :
   `Places quick-moved Cube items from the bottom-right.`
6. Après validation autonome, préparer séparément une éventuelle promotion dans
   `plugin-misc.dll` sous `misc.cubeQuickMoveBottomRight`, sans modifier, lier ni
   redistribuer une DLL d’eezstreet.

## Gates observables

1. **Portée et ABI — franchi statiquement** : graphe du chemin `0x4BB8C0`, ABI
   des routines appelées, call-site unique et signatures strictes gouvernées.
2. **Prototype autonome — franchi** : Release x64, test `1/1`, trois exports
   D2RLoader, auteur `RuffnecKk`, JSON anglais, aucun TOML et repli sûr.
3. **Dimensions et fragmentation — non exécuté en jeu** : `1x1`, `2x1`, `1x2`,
   `2x2`, `2x3`, Cube vide, fragmenté et plein; aucune superposition ni perte
   d’objet.
4. **Périmètre UI — non exécuté en jeu** : Ctrl + clic inventaire→Cube, autres
   origines indirectes, souris, manette et clics rapides; glisser-déposer manuel
   et autres conteneurs inchangés.
5. **Autorité et persistance — non exécuté en jeu** : solo, hôte, joiner,
   sauvegarde/rechargement, nouvelle partie et retour au menu; aucune
   duplication, désynchronisation ni migration de sauvegarde.
6. **Distribution — gate technique franchi, release finale ouverte** : portées
   globale et mod-locale, coexistence PluginPack, cold starts frais, hashes
   source/runtime et ZIP strict DLL + JSON sont prouvés. Le ZIP demeure une
   candidate technique jusqu’à validation des gates 3 à 5.

## Prochain gate

Valider visuellement en jeu les cinq dimensions d’objet dans un Cube vide,
fragmenté et plein, puis fermer le périmètre UI, la persistance et l’autorité
hôte/joiner. La première redirection réelle doit apparaître dans
`cube-quick-move.log` avec les dimensions et les coordonnées vanilla puis
bas-droite avant de déclarer l’archive public-ready.

## Frontière Git

Le lot actuel comprend cette mission, `Mission/CURRENT.md`,
`Mission/WORKSTREAMS.json`, `ROADMAP.html`, les preuves gouvernées 92777, les
sources `CubeQuickMove-src/`, le JSON, la DLL autonome, l’archive candidate et le
cadastre régénéré. Aucun fichier ni aucune DLL d’eezstreet n’est modifié, lié ou
redistribué. Aucun commit ni push n’est inclus.
