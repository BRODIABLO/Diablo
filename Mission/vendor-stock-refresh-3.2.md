# Vendor Stock Refresh — D2R 3.2

## Statut et séquencement

- Statut : **chantier déclaré réglé et clos par Vincent le 27 juillet 2026;
  correctif autonome 0.1.5 conservé comme preuve technique hors ROADMAP active**.
- Cible éventuelle : `D2R.exe 3.2.92777` sous D2RLoader.
- Vincent a demandé de commencer ce chantier le 26 juillet 2026, puis l’a
  déclaré réglé le 27 juillet 2026. La mission et les artefacts restent conservés
  sans constituer une priorité active.
- Vincent a remplacé le classement initial `items` le 27 juillet 2026 : la
  catégorie future confirmée est `misc`, la DLL propriétaire devient
  `plugin-misc.dll` et la clé prévue devient `misc.vendorStockRefresh`.
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
- Dans ce layout vanilla 3.2, `button_refresh` et `button_repair_all` utilisent
  exactement le même rectangle `{ x: 877, y: 1277 }`. Révéler le refresh en
  boutique normale sans déplacer son widget le superpose donc au bouton Repair
  All existant; les captures précédentes ne prouvaient pas que le clic atteignait
  le refresh.
- Vincent a désigné l’emplacement libre centré sous l’affichage de l’or, puis a
  exigé que le gamble conserve son bouton original et que les layouts moddés
  restent compatibles. `0.1.5` ne livre donc plus aucun layout vendeur et ne
  crée aucun clone : il réutilise exclusivement `button_refresh`. En boutique
  normale, il lit les rectangles runtime du bouton et de `StashWidget`, ou à
  défaut l’union de `gold_icon` et `gold_amount`, puis calcule
  `x = anchor.x + (anchor.width - button.width) / 2` et place le contrôle sous
  l’ancre avec un espacement proportionnel à sa hauteur. En gamble, il restaure
  exactement la position originale observée dans le layout actif. Si le bouton
  ou l’ancre n’existe pas, ou expose une géométrie inutilisable, le plugin refuse
  le placement et masque le refresh normal au lieu d’écrire à l’aveugle.
- La configuration du panel à `0x2411E0` stocke son mode à `panel+0x168`. Le
  mode `2` est le gamble; les vendeurs normaux peuvent relever des modes `0` ou
  `1` selon leur classe. Pour Charsi, `monstats.txt` donne `*hcIdx=154`; la table
  native indexée par `154 - 0x93` retourne le sélecteur `1`, qui saute à
  `0x2412DE` et écrit explicitement le mode `1`. Les deux gates uniques à
  `0x24137D` et `0x241391` rendent `button_refresh` disponible seulement en mode
  `2`; le gate d’entrée unique à `0x240E0D` applique la même restriction au
  chemin souris/manette direct.
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
  respectivement à boutique normale, gamble et mercenaire. Le prototype 0.1.0
  supposait que l’ancienne valeur `2`, la classe vendeur conservée à `+0xFC`,
  une entrée remplie à `VendorChainEntry+0x34` et la même classe NPC suffisaient
  pour reconnaître le clic de refresh.
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
- Le témoin joueur du 26 juillet invalide le gate de session 0.1.0 : le contrôle
  montré chez Charsi ne change aucun objet dans la fenêtre. L’analyse ultérieure
  du layout prouve qu’il partageait le rectangle du Repair All; l’absence de
  télémétrie `sent`, `armed` ou `rejected` sous 0.1.1 confirme que ce témoin
  n’avait pas atteint le sender du refresh. Indépendamment de cette collision UI,
  l’action vanilla `1` reste identique à l’ouverture normale et ne porte aucune
  preuve que la demande vient du bouton; l’état précédent lu dans `0x502F60`
  demeure un discriminateur ambigu, d’où le marqueur `VSRF` conservé.
- Le correctif `0.1.1` conserve l’opcode natif `0x38` et ses neuf octets, mais
  encode le clic normal avec le marqueur privé `0x56535246` (`VSRF`). Un hook
  strict supplémentaire au callback serveur `0x4B0470` reconnaît uniquement ce
  marqueur, le convertit en action vanilla `1`, puis laisse le callback original
  revalider le NPC, l’acte et la distance. Le hook de `0x502F60` n’arme `+0x35`
  que dans la portée thread-local exacte de cet appel validé. Une ouverture
  normale sans marqueur ne peut plus armer le refresh.
- `0.1.1` compile en Release x64, exporte les trois symboles D2RLoader et passe
  le test de politique `1/1`. La DLL source porte le SHA-256
  `0EBCC1B8EFB45E078E6636B5E59290CC3CB9953252643EFF998931B9F07BAC2C`,
  byte-identique au runtime; le JSON reste
  `B7510070696AA2DBFE420D816C24840E5A023B969F2BE140EB0FEDAC25C65C41`.
- Le cold start frais de 21:08 charge `VendorStockRefresh 0.1.1`, accepte les
  hooks `0x502F60`, `0x4B0470` et `0x10F520`, puis les trois patches UI avant le
  log `active`. Le bilan global reste vert : `20/20` patchsets, `24` plugins
  actifs, zéro rejet et zéro échec. Chaque clic normal écrit désormais un log
  explicite `armed` ou `rejected` avec compteurs sent/received/armed/rejected.
- `0.1.2` ajoute une télémétrie client explicite au sender et le layout BKVince
  séparé. La DLL Release passe toujours le test natif `1/1`, porte le SHA-256
  `D19804C6969579CE44289D370182A6C20ADD38313FF4ECDAF93C06BF9E1F00A9` et le
  layout porte `FC6016B2567FA9CD8AB1F7E81DFB7F21192B23C21390C516A7264743B5174CE7`;
  les copies runtime sont byte-identiques. Le cold start frais de 21:28 charge
  `VendorStockRefresh 0.1.2`, accepte les trois hooks, puis termine avec `20/20`
  patchsets, `24` plugins actifs, zéro rejet et zéro échec. Le placement, la
  ligne client `sent` et l’effet visible restent à confirmer dans le panneau.
- Le témoin visuel 0.1.2 chez Charsi montre toujours uniquement les deux boutons
  existants. Cette absence est expliquée par la preuve de classe ci-dessus : les
  patches 0.1.1/0.1.2 remplaçaient `mode == 2` par `mode != 1` et excluaient donc
  précisément Charsi ainsi que toute autre classe normale configurée en mode
  `1`.
- `0.1.3` force l’état enabled/visible du widget refresh pour les trois modes UI
  vendeur `0`, `1` et `2`, et ouvre de même son chemin d’entrée direct. Le
  sender conserve `CLIENT_GetVendorPanelGambleMode` comme décision autoritaire
  côté client : action vanilla `2` en gamble, marqueur `VSRF` pour les vendeurs
  normaux des deux familles. La DLL Release passe `1/1`, exporte les trois
  symboles D2RLoader et porte le SHA-256
  `35642F619DBA55AF0AD71916FA8E860EC97B580AB3863FB75379FF0EEBABA374`,
  byte-identique au runtime. Le cold start frais de 21:41 accepte les six sites,
  charge `VendorStockRefresh 0.1.3`, atteint 24/24 et conserve `20/20` patchsets,
  `24` plugins actifs, zéro rejet et zéro échec.
- Le témoin 0.1.3 prouve le trajet fonctionnel : neuf clics écrivent neuf lignes
  client `sent`, puis neuf lignes serveur `armed`, avec zéro rejet. Sa capture
  révèle toutefois deux défauts de présentation : le bouton normal est décalé
  d’environ 24 pixels vers la droite et le déplacement du widget unique déplace
  aussi le refresh de l’écran gamble.
- `0.1.4` sépare les widgets. Le hook strict de configuration du panel à
  `0x2411E0` appelle d’abord le flux original, retrouve
  `button_refresh_normal` par le helper natif `0x856220`, puis utilise les
  méthodes vtable `+0x48` et `+0x50` pour l’activer et l’afficher seulement si
  `CLIENT_GetVendorPanelGambleMode` vaut zéro. Les trois patches UI 0.1.3 sont
  supprimés : `button_refresh` retrouve ainsi ses gates et sa position vanilla
  exclusivement en gamble. La DLL Release passe `1/1`, exporte les trois
  symboles D2RLoader et porte le SHA-256
  `12B6F035FDBB26B12C7F4CBFF438E58A0E956409AA54A4A72C0E2609769D60CD`;
  le layout porte
  `2E59C9BEC8DA4368835C4288C55D9A986A34912EEEC493CD85E0F7C6A0A8FBEE`.
  Les copies runtime sont byte-identiques. Le cold start frais de 21:55 accepte
  les quatre hooks, atteint 24/24 et conserve `20/20` patchsets, `24` plugins
  actifs, zéro rejet et zéro échec.
- `0.1.5` remplace ce prototype à deux widgets par un placement runtime sans
  override de layout. Le helper natif `UI_GetWidgetLocalRect` à `0x8562A0` est
  prouvé avec l’ABI `(widget, rectOut) -> rectOut` et la structure signée
  `{x,y,width,height}` à `widget+0x70`; les chemins de rendu et de hit-test
  consomment la même géométrie. Le hook post-configuration retrouve le bouton et
  les ancres par `UI_FindChildWidgetByName` à `0x856220`, centre le bouton normal
  sur la largeur réellement chargée et conserve son état/rectangle original
  pour le gamble. Les tests couvrent le layout BKVince, un layout agrandi, le
  fallback `gold_icon` + `gold_amount` et les géométries invalides. La Release
  x64 passe `1/1`, exporte les trois symboles D2RLoader et porte le SHA-256
  `21E75601BEB6D1C3A79666FDEA62FCDB320C6B511A16E4B26867D564C67C09FE`,
  byte-identique au runtime; le JSON demeure
  `B7510070696AA2DBFE420D816C24840E5A023B969F2BE140EB0FEDAC25C65C41`.
  Le cold start frais du 27 juillet à 08:03 accepte les quatre hooks, atteint
  `24/24`, conserve `20/20` patchsets, `24` plugins actifs, zéro rejet et zéro
  échec. Chez Charsi, le log prouve le placement à `519,1383` depuis l’ancre
  runtime `421,1305,313,58`; le témoin visuel confirme le centrage sous l’or sans
  chevauchement, puis un clic change immédiatement la grille et produit
  `sent=1`, `armed=1`, `rejected=0`.

Ces faits justifient le propriétaire `items` et prouvent le mécanisme serveur de
renouvellement normal, le bouton natif, ses deux chemins d’entrée et le paquet
client→serveur réutilisé par le prototype. Le démarrage du correctif est prouvé;
son trajet client/serveur est confirmé à neuf reprises sous 0.1.3 et de nouveau
par un clic fonctionnel sous 0.1.5. Le placement dynamique normal est confirmé
chez Charsi; la restauration du bouton original en gamble reste à observer.

## Hypothèses à tester

- Le chemin natif `RefreshAll` devrait conserver son focus souris/manette lorsque
  ses gates de mode acceptent aussi le mode normal; cela doit être confirmé avec
  une manette réelle.
- Le flux `0x502F00` resynchronise effectivement la grille pendant que l’écran
  normal reste ouvert chez Charsi; ce comportement reste à étendre aux autres
  vendeurs, aux achats en cours et au multijoueur.
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

- Action gratuite, sans coût en or ni cooldown artificiel. Le prototype 0.1.5
  traite les clics séquentiellement; le besoin d’un coalescing reste à décider
  après le test de clics rapides et n’est pas revendiqué comme implanté.
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
- Lors du merge approuvé, la fonctionnalité doit rejoindre `plugin-misc.dll` et
  l’unique `D2RPlugins.json` sous `misc.vendorStockRefresh`; la DLL et le JSON
  autonomes ne seront supprimés qu’après compilation, cold start et validation
  fonctionnelle du binaire fusionné.
- Description prévue : `Refreshes a vendor's stock with one click.`

## Gates observables

1. **Valeur joueur — accepté par décision** — Vincent a explicitement demandé le
   démarrage le 26 juillet. La mesure des dix renouvellements reste une baseline
   UX à capturer, pas un blocage de l’investigation.
2. **Workbench 92777 — fermé pour le prototype dynamique** — cycle serveur,
   bouton, modes UI, callback, paquet neuf octets, actions, handler serveur,
   état de session et signatures strictes sont prouvés. La resynchronisation
   pendant un panel ouvert relève maintenant du runtime.
3. **Audit PluginPack — fermé pour l’autonome** — `plugin-items` reste
   propriétaire du remplissage et des structures du cache, mais Vincent classe
   l’action UI de rafraîchissement sous `misc`; aucun hook UI ou refresh manuel
   concurrent n’a été trouvé au commit épinglé. Le merge devra préserver cette
   frontière et la coexistence avec la politique vendor-overhaul de
   `plugin-items`.
4. **Contrat produit — fermé pour le prototype** — gratuit, marchand normal
   actif seulement, stock complet vanilla/vendor-overhaul et refus fail-closed
   pendant un état transactionnel ambigu.
5. **Prototype dynamique — fermé pour 0.1.5** — sources, README, JSON anglais,
   test `1/1` et DLL Release x64 autonomes construits; trois exports D2RLoader,
   build gate 92777, neuf signatures strictes, un seul widget natif repositionné
   depuis les ancres runtime et refus fail-closed présents.
6. **Cold start mod-local — fermé pour 0.1.5** — hashes DLL/JSON identiques,
   absence d’override de layout, configuration mod-locale prioritaire, quatre hooks acceptés,
   démarrage `24/24`, `20/20` patchsets, `24` plugins actifs, zéro rejet et zéro
   échec. Le repli global reste `not run`.
7. **Matrice gameplay — ouverte** — marchands de tous les actes, stock aléatoire et
   permanent, achats avant/après rafraîchissement, or insuffisant, souris,
   manette, résolutions, solo, hôte/joiner, nouvelle partie et retour au menu;
   zéro perte, duplication, objet fantôme, crash ou désynchronisation.
8. **Promotion PluginPack — fermee techniquement** — le candidat est integre
   sous `plugin-misc.dll` avec `misc.vendorStockRefresh`, defaut vanilla,
   quatre hooks uniques, manifeste sans chevauchement, cinq DLL Release,
   18/18 tests et cold starts vanilla/actif a `24/24`. Le gameplay integre
   restant constitue une matrice separee.
9. **Distribution** — portées globale et mod-locale, repli de configuration,
   coexistence avec les cinq DLL eezstreet, Release x64, exports D2RLoader,
   hashes source/runtime et ZIP public strict DLL + JSON seulement.

## Prochain gate

Repeter le clic Charsi valide avec le binaire integre, puis rouvrir les
non-regressions encore non executees : gamble, vendeur de mode `0`, layout
reellement modde ou agrandi, manette, cinq actes, clics rapides, repli global et
hote/joiner.

## Frontière Git

Le lot historique conserve cette mission, son workstream, les preuves du
workbench, `VendorStockRefresh-src/` et l'archive autonome comme oracles. Le JSON
et la DLL standalone sont retires de BKVince apres validation du port. Aucun
layout vendeur n'est livre; les DLL du pack sont reconstruites depuis le fork
source sans modifier, lier ni redistribuer un binaire d'eezstreet.

## Gameplay intégré — 30 juillet 2026

Vincent confirme que le bouton de rafraîchissement renouvelle correctement le
stock du vendeur avec le PluginPack intégré. Le cas nominal est `passed`; les
variantes par acte, manette, clics rapides et réseau restent hors de ce témoin.

## Version RuffnecKk Suite 0.2.0 — 18 août 2026

Vincent conserve Vendor Stock Refresh en série `0.x` et approuve le passage de
`0.1.6` à `0.2.0`. Aucun comportement ni contrat de configuration ne change.
Les builds Debug et Release passent `25/25` tests; deux builds Release
indépendants concordent byte-for-byte, et le runtime live charge
`Vendor Stock Refresh 0.2.0` dans la pile complète à `26` plugins,
`19` patches et `24/24`.
