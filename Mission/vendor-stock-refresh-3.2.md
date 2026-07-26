# Vendor Stock Refresh — D2R 3.2

## Statut et séquencement

- Statut : **prototype autonome 0.1.0 compilé et cold start mod-local vert —
  validation gameplay à ouvrir**.
- Cible éventuelle : `D2R.exe 3.2.92777` sous D2RLoader.
- Vincent a demandé de commencer ce chantier le 26 juillet 2026; cette décision
  remplace l’attente initialement prévue après Transmogrify puis Readable Items /
  Clue Scrolls et fait de Vendor Stock Refresh la priorité courante.
- Vincent a confirmé la catégorie future `items`, la DLL propriétaire
  `plugin-items.dll` et la clé prévue `items.vendorStockRefresh`.
- Pendant l’incubation, la DLL autonome est
  `VendorStockRefresh.dll`, hybride globale/mod-locale et attribuée exactement à
  `RuffnecKk`.

## Intention joueur

Ajouter à l’interface des marchands normaux un bouton qui régénère leur stock
sans devoir quitter puis rouvrir la boutique, sur le modèle de l’action de
rafraîchissement disponible dans l’écran de gamble.

Le message source fourni le 26 juillet 2026 présente explicitement cette
fonctionnalité comme une idée dont l’utilité personnelle reste incertaine. Le
besoin est donc enregistré, mais sa valeur n’est pas encore démontrée.

## Faits vérifiés

- Le gate `npm run re:d2r32 -- status` est vert pour le build `92777` : image
  canonique SHA-256
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`, image
  d’analyse SHA-256
  `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`, index
  vérifié et projet Ghidra persistant présent.
- La référence officielle épinglée
  `eezstreet/D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`
  est propre et vérifiée.
- Son `README.md:28` attribue à `plugin-items.dll` le filtre de gamble et le
  vendor overhaul.
- `src/plugin-items/items-main.cpp:40-41` répertorie
  `D2GAME_NPC_FillStoreInventory` et `D2GAME_NPC_GenerateStoreItem`; le hook du
  remplissage commence à `items-main.cpp:287`, tandis que
  `items-main.cpp:518-533` charge les politiques du vendor overhaul.
- `src/plugin-shared/include/plugin-shared.h:368-403` décrit le proxy du cache
  d’objets marchand, l’entrée d’état par NPC, son timestamp de rafraîchissement
  et la chaîne de vendeurs portée par `D2GameStrc`.
- Le PluginPack existant contrôle la génération et la qualité des objets de
  boutique. Les recherches gouvernées `vendor`, `gamble` et `store` n’ont pas
  identifié de bouton de rafraîchissement manuel du stock normal.
- `GambleScreenLimit` ne fournit pas ce comportement : sa mission gouverne la
  limite de remplissage du seul écran de gamble et son patch minimal écrit une
  plage distincte dans `D2GAME_STORES_FillGamble`.
- `D2GAME_NPC_FillStoreInventory` est appelé une seule fois à `0x540960` par le
  dispatch d’ouverture de boutique. Son ABI est `(game, player, npc) -> void`;
  il résout l’entrée `VendorChainEntry`, écrit `GetTickCount64()` à `+0x38`,
  puis reconstruit les objets aléatoires et permanents par
  `D2GAME_NPC_GenerateStoreItem` à `0x540EA0`.
- Le flux normal teste `VendorChainEntry+0x35`. Lorsque ce drapeau est armé, il
  appelle d’abord `0x502F00` à `0x540952`, remet l’état rempli `+0x34` et le
  timestamp `+0x38` à zéro, détruit/synchronise l’inventaire vendeur, puis
  rappelle `FillStoreInventory`.
- La routine `0x503290` reproduit le cycle vanilla : après `0x3A980` ms, soit
  quatre minutes, elle arme `VendorChainEntry+0x35` et actualise le timestamp.
  Deux callers directs à `0x502DEB` et `0x502EE2` prouvent son intégration au
  traitement serveur des changements de zone/état.
- Le gamble suit une branche distincte : le même dispatch appelle
  `D2GAME_STORES_FillGamble` à `0x541880` depuis `0x540913` lorsque le type
  d’interface vaut `3`.
- Le layout natif `vendorpanellayouthd.json` déclare déjà le `ButtonWidget`
  `button_refresh`, son sprite `Gambling_Refresh_Button`, le son
  `cursor_gamble_refresh_hd`, le tooltip localisé `@refresh` et le message
  `VendorPanelMessage:RefreshAll`. Aucun widget ImGui ou nouvel asset n’est
  nécessaire.
- La configuration du panel à `0x2411E0` stocke son mode à `panel+0x168` : `0`
  pour une boutique normale, `1` pour la réparation et `2` pour le gamble. Les
  deux gates uniques à `0x24137D` et `0x241391` rendent `button_refresh`
  disponible seulement en mode `2`; le gate d’entrée unique à `0x240E0D`
  applique la même restriction au chemin souris/manette direct.
- `VendorPanelMessage:RefreshAll`, hash `0xB7AA1748D66EFCAF`, rejoint le sender
  `0x10F520`. Ce sender a exactement deux références dans le panel vendeur et
  construit l’action `2` avant de rejoindre `0xEC730`.
- `0xEC730` construit le paquet client natif de neuf octets
  `{ opcode:u8, action:u32, npcGuid:u32 }` et l’envoie par la queue existante.
  Les callers du build 92777 prouvent `opcode 0x38`, action `1` pour la boutique
  normale, `2` pour le gamble et `3` pour les mercenaires. La routine
  `0x10CAC0`, déjà utilisée par le panel, expose l’état gamble courant.
- Le callback serveur `0x4B0470` exige neuf octets, valide le NPC, l’acte et la
  distance, puis route les actions `1` et `2` vers le dispatch boutique à
  `0x540850`. La taille 13 de D2MOO 1.10f est donc explicitement rejetée pour
  92777.
- Juste avant le dispatch, `0x502F60` reçoit `(game, npc, player, mode)` et
  remplace `PlayerData+0x100`; les modes `2`, `3` et `4` correspondent
  respectivement à boutique normale, gamble et mercenaire. Observer l’ancienne
  valeur `2`, la classe vendeur conservée à `+0xFC`, une entrée remplie à
  `VendorChainEntry+0x34` et la même classe NPC permet de reconnaître une
  réouverture manuelle fail-closed avant d’armer `+0x35`.
- Le prototype autonome `0.1.0` réutilise ces chemins sans nouvel opcode : trois
  patches UI stricts, un hook client du sender natif et un hook serveur de
  `0x502F60`. Il compile en Release x64, exporte les trois symboles D2RLoader et
  son test de politique passe `1/1`. La première DLL de build porte le SHA-256
  `DFF8D56AF9C7C84B74FEEE650D5CADBABEFAD65FCC651F4C353D7A485F91825E`.
- Le déploiement mod-local du 26 juillet 2026 a copié uniquement la DLL et le
  JSON. Les hashes source/runtime sont identiques : DLL
  `DFF8D56A…F91825E`, JSON `B7510070…65C41`.
- Le cold start frais de 18:00 a chargé `VendorStockRefresh 0.1.0` sur 92777,
  résolu le JSON depuis
  `mods/BKVince/BKVince.mpq/VendorStockRefresh.json`, installé les hooks
  `0x10F520` et `0x502F60`, puis atteint le démarrage graphique complet. Le log
  `active` n’est écrit qu’après acceptation des trois patches UI; aucun `ERROR`,
  rejet, échec ou mismatch frais n’a été trouvé.

Ces faits justifient le propriétaire `items` et prouvent le mécanisme serveur de
renouvellement normal, le bouton natif, ses deux chemins d’entrée et le paquet
client→serveur réutilisé par le prototype. Le comportement en jeu reste à
valider; aucune preuve runtime n’est encore revendiquée.

## Hypothèses à tester

- Le chemin natif `RefreshAll` devrait conserver son focus souris/manette lorsque
  ses gates de mode acceptent aussi le mode normal; cela doit être confirmé avec
  une manette réelle.
- Le flux `0x502F00` devrait resynchroniser la grille pendant que l’écran normal
  reste ouvert, comme il le fait lors de la prochaine ouverture après le timer;
  le rafraîchissement visible sans fermeture reste à observer au runtime.
- Plusieurs clics rapides pourraient produire plusieurs paquets séquentiels. La
  sérialisation effective et le besoin éventuel d’un coalescing purement
  transport restent à mesurer avant d’affirmer « une requête en vol ».

## Inconnues et décisions produit

- L’autorité réelle et la portée partagée du cache vendeur en hôte/joiner.
- Le rendu et la resynchronisation des objets permanents, consommables, objets
  déjà achetés et entrées propres au vendor overhaul pendant un panel ouvert.
- Le besoin réel d’un coalescing des clics rapides, sans ajouter de coût ni de
  cooldown gameplay non demandé.
- Le focus manette et le comportement aux différentes résolutions et échelles
  d’interface.
- Comportement lorsqu’un objet est survolé, sélectionné ou en cours d’achat au
  moment de la demande.

## Contrat du premier prototype

- Action gratuite, sans coût en or ni cooldown artificiel; une seule requête
  peut être en vol et les clics supplémentaires sont ignorés jusqu’à sa fin.
- Portée limitée au marchand normal actuellement ouvert par le joueur; gamble,
  trade joueur, récompenses et interfaces sans `VendorChainEntry` sont refusés.
- Reconstruction complète selon le comportement vanilla, y compris les codes
  permanents qui doivent réapparaître, afin de ne pas inventer une deuxième
  politique de stock en parallèle du vendor overhaul.
- Le serveur refuse la demande si le joueur, le NPC, la session de boutique ou
  l’état transactionnel ne peuvent pas être validés sans ambiguïté.
- Le bouton doit utiliser le style natif du gamble, être localisable et recevoir
  un focus souris/manette; un overlay ImGui n’est pas accepté comme UI finale.

## Architecture exigée avant prototype

- L’hôte demeure l’unique autorité : le client transmet une intention de
  rafraîchir le marchand actuellement ouvert; il ne génère ni ne choisit aucun
  objet.
- Le serveur revalide le joueur, le NPC, la session de boutique et la politique
  avant d’invalider le stock, puis renvoie un état complet cohérent au client.
- Toute erreur conserve le stock précédent; aucune opération partielle ne doit
  supprimer, dupliquer ou désynchroniser un objet.
- Le plugin d’incubation reste autonome, hybride et sans `ModScopedOnly`. Il ne
  modifie, ne lie ni ne redistribue aucune DLL d’eezstreet.
- Une éventuelle configuration utilise uniquement `VendorStockRefresh.json`, en
  anglais, recherchée d’abord dans le mod actif puis dans le dossier global du
  jeu. Aucun TOML n’est autorisé.
- Après validation, la fonctionnalité doit rejoindre `plugin-items.dll` et
  l’unique `D2RPlugins.json` sous `items.vendorStockRefresh`; la DLL et le JSON
  autonomes seront alors supprimés.
- Description prévue : `Refreshes a vendor's stock with one click.`

## Gates observables

1. **Valeur joueur — accepté par décision** — Vincent a explicitement demandé le
   démarrage le 26 juillet. La mesure des dix renouvellements reste une baseline
   UX à capturer, pas un blocage de l’investigation.
2. **Workbench 92777 — fermé pour le prototype statique** — cycle serveur,
   bouton, modes UI, callback, paquet neuf octets, actions, handler serveur,
   état de session et signatures strictes sont prouvés. La resynchronisation
   pendant un panel ouvert relève maintenant du runtime.
3. **Audit PluginPack — fermé** — `plugin-items` est propriétaire du remplissage
   et des structures du cache; aucun hook UI ou refresh manuel concurrent n’a
   été trouvé au commit épinglé.
4. **Contrat produit — fermé pour le prototype** — gratuit, marchand normal
   actif seulement, stock complet vanilla/vendor-overhaul et refus fail-closed
   pendant un état transactionnel ambigu.
5. **Prototype statique — fermé** — sources, README, JSON anglais, tests et DLL
   Release x64 autonomes construits; trois exports D2RLoader, build gate 92777,
   signatures strictes et refus fail-closed présents.
6. **Cold start mod-local — fermé** — hashes DLL/JSON identiques, configuration
   mod-locale prioritaire, cinq sites acceptés, plugin actif et zéro erreur
   fraîche. Le repli global reste `not run`.
7. **Matrice gameplay — ouverte** — marchands de tous les actes, stock aléatoire et
   permanent, achats avant/après rafraîchissement, or insuffisant, souris,
   manette, résolutions, solo, hôte/joiner, nouvelle partie et retour au menu;
   zéro perte, duplication, objet fantôme, crash ou désynchronisation.
8. **Distribution** — portées globale et mod-locale, repli de configuration,
   coexistence avec les cinq DLL eezstreet, Release x64, exports D2RLoader,
   hashes source/runtime et ZIP public strict DLL + JSON seulement.

## Prochain gate

Tester en jeu un marchand normal avant/après achat et un écran de gamble.
Capturer le stock avant/après, les compteurs de la commande
`vendor-stock-refresh` et les logs. Ensuite seulement, étendre à la manette, aux
cinq actes, au spam de clics, au repli global et à la matrice hôte/joiner.

## Frontière Git

Le lot actif comprend cette mission, son entrée ROADMAP, son workstream, les
preuves gouvernées du workbench, `VendorStockRefresh-src/`,
`VendorStockRefresh.json` et la DLL autonome. Aucun fichier ni aucune DLL
d’eezstreet n’est modifié, lié ou redistribué.
