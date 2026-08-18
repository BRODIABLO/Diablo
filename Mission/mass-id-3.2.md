# MassID — D2R 3.2

Dernière mise à jour : 18 août 2026

## Décision produit

Vincent confirme le 18 août 2026 **MassID** comme plugin autonome permanent de
la RuffnecKk D2RLoader Suite. L’ancien projet de merge dans `plugin-items.dll`
sous `items.massIdentify` est annulé; il reste seulement une décision
historique. MassID conserve sa DLL, sa version, son archive et son JSON
indépendants, sans modifier, lier ni redistribuer une DLL d’eezstreet.

Le geste retenu est `Shift + clic droit` sur un Tome of Identify. L’autorité
serveur identifie les objets non identifiés de l’inventaire principal, puis ceux
du Horadric Cube, puis ceux du coffre personnel et partagé, dans cet ordre
déterministe.

Le JSON autonome porte le contrat propre de MassID :

```json
{
  "enabled": true,
  "freeIdentification": false
}
```

- `freeIdentification=true` identifie tous les objets admissibles même avec un
  tome vide et ne modifie jamais sa quantité;
- `freeIdentification=false` identifie au plus autant d’objets que le tome
  contient de charges et consomme exactement une charge par objet nouvellement
  identifié;
- un objet déjà identifié ne coûte aucune charge.

## Preuves natives gouvernées — build 92777

- Le workbench canonique, l’image d’analyse, l’index SQLite et le projet Ghidra
  sont vérifiés. La référence D2MOO demeure sémantique uniquement.
- Deux pipelines d’inventaire parallèles sont présents. Le handler moderne
  `0x2C7540` mène à `0x2AA9F0`/`0x15F660`, mais le témoin physique 0.1.4 prouve
  que le panneau clavier/souris actif ne les appelle pas. Son handler réel
  commence à `0x228AB0`; il rejette l’état souris `5` à `0x228AF0`, récupère
  l’objet par `widget+0xC8` à `0x228B2D`, teste Shift à `0x228CA4`, puis arme le
  comportement vanilla par `widget+0xD0` à `0x228CED`. MassID 0.1.5 intercepte
  cette entrée et consomme seulement le geste reconnu avant `+0xD0`.
- `CLIENT_SendTwentyOneByteCommandPacket` à `0xEC820` construit le paquet D2R
  3.2 de 21 octets. Le client natif Cain l’appelle avec l’opcode `0x34`.
- `D2GAME_PACKETCALLBACK_Rcv0x34_IdentifyItemsWithNpc` à `0x4C6C90` est le
  callback serveur autoritaire correspondant; il exige exactement 21 octets.
- `D2GAME_ITEMS_Identify` à `0x46E8C0` reçoit `(game, player, item, flag)`.
  Le caller Cain à `0x53C8F5` passe `1`; la routine pose le drapeau identifié,
  envoie l’update item, rafraîchit l’inventaire et appelle
  `SUNIT_AttachSound(player, 6, player)`.
- `D2GAME_ITEMS_IdentifyStoredItem` à `0x46EA70` est un helper partiel utilisé
  notamment pendant la création d’objets. Il pose le flag et met à jour les
  statlists, mais n’envoie pas à lui seul l’update item complet ni le son.
- `SynchronizeItemAndBoundSkillQuantity` à `0x46F090` accepte
  `(game, player, book, delta)` et synchronise `STAT_QUANTITY` avec le skill lié.
- Le chemin MassID n’accroche ni `D2GAME_PACKETCALLBACK_EntityAction` possédé par
  Vendor Stock Refresh dans `plugin-items.dll`, ni `D2GAME_HandleUseItemPacket`
  possédé par Transmogrify, ni la queue générique utilisée par
  EquippedItemToCube.
- `plugin-items.dll` peut posséder `UI_TOOLTIP_ResolveHoveredUnit 0x2A7810`.
  MassID 0.1.5 ne l’appelle et ne l’accroche pas : le tome exact vient de la
  méthode virtuelle `+0xC8` déjà utilisée par le handler réel.
- Les hints finaux viennent des clés natives
  `InventoryItemTooltipAppenderDrop` et `InventoryItemTooltipAppenderMove`.
  Drop passe par `0x2279BD`/`0x2C552D`; Move passe par
  `0x2278DC`, `0x227936`, `0x2C5241`, `0x2C528D`, `0x2C53AB` et `0x2CA2E0`.
  MassID 0.2.5 redirige seulement ces appels de cinq octets. Le wrapper conserve
  le texte localisé et ajoute la ligne Mass ID sans code couleur, de sorte que
  la ligne hérite exactement du gris de l’appender vanilla. Il ne hooke ni la
  localisation globale ni `ITEMS_BuildItemTooltip 0x2BD480`.
- `UNITS_GetInventoryGrid 0x34A410` prouve la cartographie des pages serveur :
  inventaire `0`, Cube `3` et stockage `4`. Le témoin 0.2.5 a toutefois prouvé
  que la liste principale du joueur ne contient que son coffre personnel; les
  pages partagées appartiennent à des unités joueur auxiliaires distinctes.
- Les handlers shared-stash `0x4C5570` et `0x4C6480` résolvent ces unités
  auxiliaires par GUID, exigent leur état marqueur `0xBA` via
  `STATES_CheckState 0x3351B0`, puis vérifient que
  `INVENTORY_GetOwnerId 0x388BA0` correspond au GUID du joueur principal.
  `INVENTORY_GetFirstCorpse 0x388E00`, `INVENTORY_GetNextCorpse 0x38CD70` et
  `INVENTORY_GetUnitGUIDFromCorpse 0x2EF880` exposent la même liste native sans
  exiger de hook sur les handlers possédés par RemoteStash.

## Architecture d’incubation

1. Les appenders de tooltip mémorisent le GUID du Tome réellement survolé. Le
   WndProc de la fenêtre consomme uniquement `Shift + clic droit` sur ce GUID,
   y compris le bouton relâché, avant que vanilla puisse armer le curseur
   Identify. Le paquet est différé au rendu suivant; le clic droit ordinaire
   reste entièrement vanilla.
2. Il consomme ce geste et envoie un paquet `0x34` de 21 octets portant deux
   marqueurs privés et le GUID du tome.
3. Le hook serveur reconnaît uniquement ces deux marqueurs. Tous les paquets
   Cain natifs sont délégués sans modification à l’original.
4. Le serveur revalide le GUID, le type item, le code `ibk `, l’inventaire
   parent et la page. Un paquet privé invalide est consommé sans mutation.
5. Trois passages sur l’inventaire principal traitent inventaire, Cube puis
   coffre personnel. Un quatrième passage parcourt chaque proxy shared-stash
   dont le marqueur natif et le propriétaire sont validés. Tous appellent le
   chemin Cain complet `0x46E8C0` uniquement pour les objets non identifiés.
   Le budget est illimité en mode gratuit, sinon borné à la quantité serveur
   du tome. Chaque succès produit l’update client, le rafraîchissement et le son
   vanilla ID `6`.
6. La consommation non gratuite est appliquée une seule fois par le
   synchroniseur natif avec un delta égal au nombre d’objets réellement
   identifiés.

## Gates de validation

- [x] Destination permanente confirmée : `MassID.dll`, plugin autonome
  RuffnecKk Suite sans catégorie, propriétaire ni clé de merge PluginPack.
- [x] Configuration autonome confirmée : `MassID.json` avec `enabled` et
  `freeIdentification`.
- [x] Handler du geste, protocole Cain 3.2, helper d’identification et ABI de
  quantité prouvés pour 92777.
- [x] Audit de coexistence ciblé : aucun hook autoritaire partagé avec les cinq
  DLL du pack ou Transmogrify; le tooltip est optionnel et coopératif.
- [x] Release x64, CTest, manifeste v2, exports, auteur, description et JSON
  strict validés.
- [x] Cold start BKVince frais sans rejet, échec ni assertion.
- [ ] Gameplay : tome vide, partiel et suffisant; modes gratuit/non gratuit;
  ordre inventory/Cube/stash; coffre personnel et chaque onglet partagé; objets
  déjà identifiés; sauvegarde/relecture.
- [x] Témoin gameplay essentiel : geste capturé sans curseur Identify, paquet
  `0x34` reçu côté serveur, deux objets identifiés et deux charges consommées
  sur un tome de quantité trois.
- [x] Compatibilité technique : portées globale et mod-locale, repli global,
  priorité mod-locale, doublon neutralisé et coexistence avec les cinq DLL
  eezstreet sans rejet ni échec.
- [ ] Qualification complète RuffnecKk Suite avec tous les composants actifs,
  toutes les fonctionnalités PluginPack activées et les ordres de chargement
  pertinents, sans retrait ni neutralisation.
- [ ] Compatibilité fonctionnelle : souris/manette inchangées, solo,
  hôte/joiner.
- [ ] Reconstruire le ZIP public strict avec seulement `MassID.dll` et
  `MassID.json`; conserver le README révisable à côté de l’archive.
- [x] Conserver définitivement la DLL, le JSON et le versionnement autonomes;
  aucune suppression après intégration PluginPack n’est planifiée.

## Validation obtenue le 31 juillet 2026

- Release x64 et CTest : `1/1` test vert.
- DLL finale : SHA-256
  `52E3A3BAA601E4BC7546CE57A271A22E1746992C14AF76E20946251871D44FBA`;
  les hashes build, dépôt, package et runtime sont identiques.
- Les trois exports D2RLoader, le manifeste v2, la version `0.1.0`, l’auteur
  `RuffnecKk` et la description orientée joueur sont présents.
- Le cold start mod-local installe les hooks `0x2C7540` et `0x4AE280`, résout
  le JSON BKVince et coexiste avec les cinq DLL du pack. Le cold start global
  installe les mêmes hooks après le pack et résout le repli `MassID.json`.
- Avec les DLL globale et mod-locale simultanées, une seule instance
  `mass-id` devient active et la configuration mod-locale demeure prioritaire.
- Une valeur `freeIdentification: "yes"` est refusée avant tout hook; le JSON
  valide a ensuite été restauré et un dernier cold start est vert.
- ZIP : `addons/MassID/MassID.zip`, deux entrées à la racine, SHA-256
  `4429F1FFAE84F7CC26C2BEB32DCB473C37E1B27BAA54D6A3E6EA33F4D8181F61`.
- L’automatisation a ouvert `QtyTester`, mais son inventaire visible ne
  contenait pas de Tome of Identify et l’outil ne peut pas maintenir Shift
  pendant un clic droit. Aucun résultat gameplay n’est donc inféré.

## Correctif 0.1.1 — 4 août 2026

- Le premier test physique de Vincent a montré que `Shift + clic droit` ne
  déclenchait aucune identification avec `0.1.0`.
- L’audit du handler a isolé le rejet client : la valeur retournée par
  `UI_TOOLTIP_ResolveHoveredUnit` était nommée et comparée comme un propriétaire,
  alors qu’elle est l’unité cliquée. Un tome ne peut jamais être égal au joueur
  local; la requête était donc filtrée avant l’envoi.
- `0.1.1` utilise directement cette unité comme tome candidat et prouve sa
  propriété avec `INVENTORY_GetParentInventory(item) == inventory`. Le chemin
  virtuel `+0xC8` et la comparaison erronée au joueur ont été retirés.
- Release x64 et CTest : `1/1` test vert. Les trois exports, la version
  `0.1.1`, l’auteur et la description sont conformes.
- DLL build/dépôt/package/runtime : SHA-256
  `67CCE2EFAD6B59EF4ECD7227952788F00F27D799C11D496E92DB909FEA4AFB73`.
  ZIP strict DLL + JSON : SHA-256
  `8F86FD78A690935EAD288ACFBB6DDE9DA971D7A76A6B2A8929B91879417FADB5`.
- Le cold start BKVince frais charge `MassID 0.1.1`, installe les deux hooks,
  résout le JSON mod-local avec `freeIdentification=false` et laisse une seule
  instance de jeu ouverte. Les nouvelles traces distinguent requête envoyée,
  acceptée ou rejetée et rapportent quantité, objets identifiés et charges.
- Le résultat fonctionnel du geste corrigé demeure ouvert jusqu’au nouveau test
  physique de Vincent.

## Correctif 0.1.2 — 4 août 2026

- L’audit suivant le retour « rien ne se passe » a séparé deux chemins natifs :
  `0x46EA70`, utilisé par `0.1.0/0.1.1`, est un helper partiel sans notification
  item complète; `0x46E8C0` est le chemin Cain complet appelé à `0x53C8F5`.
- `0.1.2` appelle désormais `0x46E8C0` avec l’ABI observée
  `(game, player, item, 1)`. Sa signature stricte de 32 octets est unique dans
  `.text`. Le rafraîchissement client et le son vanilla d’identification `6`
  proviennent donc du jeu lui-même.
- Le client intercepte tout Tome of Identify résolu avec Shift, clic droit et
  curseur vide, puis laisse au serveur la validation autoritaire du propriétaire
  et de la page. Le clic intercepté ne délègue pas au comportement vanilla du
  tome et ne transforme donc pas le curseur en mode Identify.
- Release x64 et CTest : `1/1` test vert. DLL build/dépôt/package/runtime :
  SHA-256 `736189A4852708CB59033C44E4FE7AA3BE4A5DB21A29C649BF5C36A9BC94E667`.
  ZIP strict DLL + JSON : SHA-256
  `B93B437963AA0514EB1D6575F2D8D8ADD6ECBF24513EB64E0F7EB017C38843E6`.
- Le cold start BKVince frais charge `MassID 0.1.2`, installe les deux hooks et
  résout `freeIdentification=false`. Le témoin gameplay corrigé reste attendu.

## Correctif 0.1.3 — 4 août 2026

- Le test physique de `0.1.2` montre encore le curseur vanilla Identify et
  aucune action MassID. L’absence de trace prouve que les filtres internes
  `INPUT_IsVirtualKeyDownAsync` et `INPUT_GetMouseState`, puis l’emploi de
  `UI_TOOLTIP_ResolveHoveredUnit` comme tome, empêchent la capture cliente.
- `0.1.3` lit Shift et le bouton droit par `GetAsyncKeyState`, récupère le tome
  avec la méthode virtuelle `widget+0xC8`, trace les événements avant les
  validations et ne délègue pas le geste reconnu au handler vanilla. Le curseur
  doit donc rester normal; le serveur conserve toutes les validations d’autorité.
- Le tooltip du Tome reçoit en gris `Shift + Right Click to Mass ID`. Le hook
  autonome s’installe dans le profil BKVince actuel après les sept call-sites
  d’AdvancedItemTooltips; l’export coopératif permet une composition future.
- Release x64 et CTest : `1/1` test vert. Les quatre exports, la version
  `0.1.3`, l’auteur et la description sont conformes. DLL build/dépôt/package/
  runtime : SHA-256
  `91327D9F120353A5C02ED0C0FF062D1615B633A6E3CB3F26535C51FDB1C7CACC`.
  ZIP strict DLL + JSON : SHA-256
  `91D5F0B0987FF77DCB17F1A2611260B205B7EF689CA2AB6059535848C7EB2377`.
- Le cold start BKVince frais charge `MassID 0.1.3`, installe les hooks
  `0x2C7540`, `0x4AE280` et `0x2BD480`, résout
  `freeIdentification=false`, et garde une instance responsive. La validation
  fonctionnelle du geste et du rendu reste ouverte jusqu’au test de Vincent.

## Correctif 0.1.4 — 4 août 2026

- Le test physique de `0.1.3` a encore laissé le curseur Identify. La ligne de
  tooltip prouve que la DLL est chargée, tandis que l’absence totale de trace du
  hook `0x2C7540` prouve que ce point d’entrée n’observe pas l’événement utile au
  moment choisi.
- Le désassemblage direct de la branche droite montre le test Shift natif à
  `0x2C7CEC`, puis deux destinations exclusives à `0x2AA9F0` et `0x15F660`.
  `0.1.4` hooke leurs signatures uniques de 32 octets et ne capture que les
  retours `0x2C7D1F/0x2C7D59`. Aucun état Win32 externe n’est désormais requis.
- Le premier rendu `0.1.3` a aussi révélé deux défauts : la ligne était placée
  avant le hint de drop et son code gris contaminait le reste du buffer.
  `0.1.4` l’ajoute en dernier, après `Ctrl + Left Click to Drop`, et termine par
  le code blanc vanilla.
- Comme AdvancedItemTooltips, MassID résout `ItemStats1h` dans la base native et
  choisit son seul label inventé parmi les treize locales D2R intégrées. Le JSON
  ne contient aucun réglage de langue.
- Release x64 et CTest : `1/1` test vert. DLL build/dépôt/package/runtime :
  SHA-256 `7ACD3BDB4F983A89642B82B3F2671CE9F8D1074F2DC95C84760CD2EDF6CF3569`.
  ZIP strict DLL + JSON : SHA-256
  `633ABB5C786858E6FC198B7C0DDB376FC64097DF5C9C306E7008BBD8007A0344`.
- Le cold start BKVince charge `MassID 0.1.4`, installe `0x2AA9F0`, `0x15F660`,
  `0x4AE280` et `0x2BD480`, résout `freeIdentification=false` et garde une
  instance responsive. Le nouveau témoin gameplay est en cours.

## Correctif 0.1.5 — 4 août 2026

- Le témoin physique de `0.1.4` invalide les deux suppositions restantes : les
  actions du pipeline `0x2C…` ne voient pas le geste du panneau actif, et une
  ligne ajoutée au buffer principal ne peut pas se placer sous l’appender Drop.
- Le pipeline clavier/souris réellement utilisé est `0x228AB0`. Son flux résout
  l’objet par le vtable `+0xC8` à `0x228B2D` et n’appelle le comportement
  vanilla `+0xD0` qu’à `0x228CED`. `0.1.5` capture le Tome avant cet appel et
  retourne immédiatement après l’envoi de la requête privée.
- Les deux xrefs de la clé `InventoryItemTooltipAppenderDrop` sont prouvés en
  mémoire runtime. Les callsites `0x2279BD` et `0x2C552D` passent par deux relais
  proches qui fournissent l’item au wrapper; celui-ci retourne le Drop natif
  suivi du hint Mass ID gris et d’un reset blanc.
- Release x64 et CTest : `1/1` test vert. Les trois exports D2RLoader sont les
  seuls exports. DLL build/dépôt/package/runtime : SHA-256
  `A03F40B1A9E1B2D988226D838C9766519A046022F7B564486DE3F5FD917EE15E`.
- Le ZIP strict DLL + JSON contient exactement deux entrées à la racine et porte
  le SHA-256 `ED9ED93489BAAE33E46D994BD4B3DD4BA98C6CD66C212C52CC6653934B1E6958`.
- Le cold start BKVince installe `0x228AB0` et `0x4AE280`, accepte les deux
  redirections de callsites, résout `freeIdentification=false` et garde une
  instance responsive. Le témoin gameplay v0.1.5 reste ouvert : aucune réussite
  fonctionnelle n’est inférée du seul cold start.

## Correctif 0.1.6 — 4 août 2026

- Le témoin physique de `0.1.5` confirme que la bonne DLL est chargée mais que
  le clic reste délégué à vanilla : le curseur Identify apparaît, aucun objet
  n'est identifié et aucune trace d'action MassID n'est produite.
- Le désassemblage des deux handlers prouve que leur second argument est un
  état d'événement transmis par valeur. Vanilla le copie dans une variable
  locale et passe l'adresse de cette copie au résolveur virtuel `widget+0xC8`.
  `0.1.5` passait directement la valeur comme un pointeur, ce qui invalidait la
  résolution du tome.
- `0.1.6` reproduit cette ABI, exige Shift et le bouton droit réellement
  enfoncés, et accroche les deux handlers `0x228AB0` et `0x2C7540`. Un geste
  capturé retourne avant toute action vanilla et trace le pipeline exact.
- Release x64 et CTest : `1/1` test vert. DLL build/dépôt/package/runtime :
  SHA-256 `6F08A9A8F6166C63D845D7AEF2D987D91A3D843C57E78E51582E7EA3AD176E0D`.
  ZIP strict DLL + JSON : SHA-256
  `C5272BE91C4E8E8F43FA7E955F34D8BE94934AEF2710357543D4344EBC53AAF7`.
- Le cold start BKVince installe les hooks client `0x228AB0` et `0x2C7540`, le
  callback serveur `0x4AE280` et les appenders de tooltip. La validation
  gameplay de la capture, de l'absence de curseur et de l'identification reste
  ouverte jusqu'au nouveau témoin de Vincent.

## Correctif 0.1.7 — 4 août 2026

- Le témoin physique de `0.1.6` produit encore le curseur Identify et zéro
  trace après le clic. Les deux handlers d'inventaire sont donc définitivement
  invalidés comme surface d'usage du tome dans le panneau actif.
- `0x1AC830` reçoit directement l'objet utilisé, résout son record ItemsTxt,
  lit `pSpell` à `+0x94`, puis appelle `0x1B9720` pour construire l'action
  ciblée qui devient le curseur Identify. Sa signature stricte de 32 octets est
  unique dans le build 92777.
- `0.1.7` remplace les deux hooks UI par ce dispatcher. Son invocation prouve
  déjà l'usage ciblé par clic droit; lorsque Shift est enfoncé et que l'objet
  est `ibk `, MassID envoie sa requête et retourne `1` avant la construction du
  curseur. Un clic droit sans Shift reste entièrement vanilla.
- Release x64 et CTest : `1/1` test vert. DLL build/dépôt/package/runtime :
  SHA-256 `0B552708C735EE8D31E6B9A273084FD17B5C2ED92AB4C5C9C03EA973007B7A62`.
  ZIP strict DLL + JSON : SHA-256
  `5DCAF2FDFAABB4CE709A4DC049D4787BBBF699FC3481C8D7EBF8DBB3951674E5`.
- Le cold start BKVince installe `0x1AC830` et `0x4AE280`, conserve les deux
  appenders de tooltip et charge `freeIdentification=false`. La capture et
  l'identification restent à confirmer par le témoin gameplay.

## Correctif 0.2.4 — 4 août 2026

- Le WndProc et le GUID produit par le tooltip ont finalement prouvé la capture
  du geste et supprimé le curseur vanilla. Le témoin 0.2.3 a ensuite montré le
  paquet privé byte-exact dans la dernière entrée de
  `CLIENT_QueueOutgoingPacket`, mais aucune invocation du hook serveur.
- La cause était une mauvaise attribution de RVA, pas le geste ni le transport.
  La table de callbacks commence à `0x1D2A790`, ancrée par le transport
  RemoteStash fonctionnel (`opcode 0x18 -> 0x4BFF30`). `0x4AE280` occupe donc
  le slot `0x2E`; le vrai callback `0x34` est `0x4C6C90`, stocké à
  `0x1D2A930`. MassID 0.2.4 accroche ce slot exact.
- Le témoin physique de Vincent à 16:28:04 confirme toute la chaîne : paquet
  `0x34` de 21 octets reçu, Tome GUID 75 accepté, quantité serveur 3, deux
  objets identifiés et deux charges consommées avec
  `freeIdentification=false`. Le curseur Identify ne revient pas et le chemin
  `D2GAME_ITEMS_Identify` conserve le son vanilla ID.
- Release x64 et CTest : `1/1` test vert. DLL build/dépôt/package/runtime :
  SHA-256 `E94311C31861DF9373F5886B0AB4ED04FE9DBE7444C711673F0F6E0E91418477`.
  ZIP strict DLL + JSON, deux entrées à la racine : SHA-256
  `AD327E3E777BA342927419B644543F3A8E2EFB7BAEBFCC509EEF45BF77ECAAF2`.
- Le cold start BKVince charge `MassID 0.2.4`, installe les hooks client
  `0x1C7A30` et serveur `0x4C6C90`, conserve les deux appenders de tooltip,
  résout le JSON mod-local et garde une instance responsive.

## Correctif 0.2.5 — 4 août 2026

- Le témoin visuel de Vincent prouve que l’ouverture du Cube remplace le hint
  Drop par le hint Move. Les six appels exacts de
  `InventoryItemTooltipAppenderMove` sont maintenant redirigés vers le même
  wrapper Tome que les deux appels Drop.
- Le code gris artificiel et son reset blanc sont supprimés. La ligne Mass ID
  est concaténée sans balise de couleur au texte natif : elle hérite donc du
  style exact de Drop ou Move, y compris lorsque le Cube est ouvert.
- Le serveur effectue désormais un troisième passage sur la page `4`, après
  l’inventaire et le Cube. Le témoin gameplay confirme que ce passage couvre le
  coffre personnel, mais pas les unités auxiliaires du shared stash; cette
  limite est corrigée en 0.2.6.
- Release x64 et CTest : `1/1` test vert. Les trois exports D2RLoader sont les
  seuls exports. DLL build/dépôt/package/runtime : SHA-256
  `060492246579D3F0286667F4EE00BB5FB734E8072FE1CEB05681CC940281344E`.
  ZIP strict DLL + JSON, deux entrées à la racine : SHA-256
  `E33585A2BF9B284518C77FE36D69523DBE940E150C6F142467E514A5B075CBF6`.
- La synchronisation runtime limitée à `MassID.dll` et `MassID.json` est verte;
  aucun processus n’a été arrêté ou lancé. Le cold start reste en attente, car
  Vincent demande désormais une autorisation explicite avant tout démarrage du
  jeu.

## Correctif 0.2.6 — 4 août 2026

- Le témoin physique isole le défaut : la 0.2.5 identifie un objet du coffre
  personnel avec `stash=1`, mais retourne `identified=0` devant les pages
  partagées. Le geste, le paquet serveur, l’identification et le budget sont
  donc fonctionnels; seule la découverte du conteneur shared manquait.
- Les deux handlers natifs du shared stash prouvent que chaque stockage partagé
  est l’inventaire d’un `UNIT_PLAYER` auxiliaire enregistré dans la liste native
  d’unités auxiliaires du joueur. MassID 0.2.6 parcourt cette liste dans son
  callback serveur existant, puis filtre chaque candidat par marqueur `0xBA` et
  par GUID propriétaire avant de visiter sa page `4`.
- L’ordre du budget devient explicitement inventaire, Cube, coffre personnel,
  puis shared stash. En mode non gratuit, une charge demeure consommée par
  identification réussie sur l’ensemble des conteneurs.
- Aucun nouveau hook n’est installé. Les handlers `0x4C5570` et `0x4C6480`
  restent la propriété exclusive de RemoteStash; MassID appelle seulement
  quatre accesseurs natifs protégés par signatures strictes.
- Release x64 et CTest : `1/1` test vert. Les trois exports D2RLoader sont les
  seuls exports. DLL build/dépôt/package/runtime : SHA-256
  `F96077F9AD64AE88E7EB613AF5CB9FC71AB110D66F991C960E1D38F9813879A9`.
  ZIP strict DLL + JSON, deux entrées à la racine : SHA-256
  `27D4F04E9BEE2E7AC3F68C6CAB43D21EDFFB54F9723ED726EA673BAC63ED4BC1`.
- La synchronisation runtime limitée à `MassID.dll` et `MassID.json` est verte :
  une instance 0.2.5 a été arrêtée pour libérer la DLL, 0.2.6 a été copiée
  et aucun jeu n’a été relancé. Le cold start attend l’autorisation explicite
  de Vincent.

## Correctif 0.2.7 — 5 août 2026

- Les tests de Vincent via le shared stash vanilla et RemoteStash échouent de
  la même façon. La trace 0.2.6 isole `sharedContainers=0`, tandis que le geste,
  le paquet, le budget et l’identification inventory/Cube restent fonctionnels.
- La cause est une erreur de nature du marqueur : `0xBA` est un identifiant de
  state, pas un identifiant de stat. Les deux handlers natifs chargent `EDX=0xBA`
  et appellent `STATES_CheckState 0x3351B0` avec seulement `(proxy, stateId)`.
  La 0.2.6 appelait `STATLIST_GetUnitStat(proxy, 0xBA, 0)` et rejetait donc tous
  les proxies valides.
- La 0.2.7 reproduit l’appel natif exact et protège le nouveau RVA avec une
  signature stricte de 32 octets. Le parcours des records, la résolution du
  proxy, la validation du propriétaire et l’ordre du budget sont inchangés.
- Release x64 et CTest : `1/1` test vert. Les trois exports D2RLoader sont les
  seuls exports. DLL build/dépôt/package : SHA-256
  `6D16D89C1A27C6197634C6C1B1F06D427898929B4B8730E16BB9D7A9230E7D7B`.
  ZIP strict DLL + JSON : SHA-256
  `7F226262FE366D6450990E89A170A2C17C935E4507CB38E6B6E76CAFEA4A9523`.
- Après arrêt ciblé de l’instance BKVince qui verrouillait la DLL, le runtime
  a été synchronisé sans redémarrage. Les hashes build, dépôt, package et
  runtime sont identiques; le cold start 0.2.7 attend l’autorisation explicite
  de Vincent.

## Correctif 0.2.8 — 5 août 2026

- Le témoin physique 0.2.7 a identifié quatre objets, dont trois dans le shared
  stash, mais les trois updates shared sont apparus comme des objets gelés dans
  le coffre personnel. La trace serveur confirme
  `sharedStash=3; sharedContainers=1001` : la découverte et la mutation
  autoritaire étaient fonctionnelles, contrairement au routage client.
- L’inspection byte-exact des sauvegardes après arrêt du jeu confirme que les
  neuf objets de la première page partagée sont toujours dans
  `ModernSharedStashSoftCoreV2.d2i`; aucun de leurs GUID n’est présent dans le
  `.d2s` du personnage. Il n’y a donc eu ni déplacement persistant ni perte :
  les objets du coffre personnel étaient des fantômes client créés par un
  mauvais acteur de paquet.
- `ITEMS_SendItemUpdate 0x535F60` traite explicitement les joueurs-proxy portant
  l’état `0xBA` à `0x53623C`, résout leur client propriétaire et expédie leur
  update par le chemin `0x536410`. Le handler natif de retrait confirme la
  sémantique : l’update de retrait shared à `0x4C690C` reçoit le proxy, puis
  l’update d’ajout personal à `0x4C694B` reçoit le joueur principal.
- La 0.2.7 appelait `D2GAME_ITEMS_Identify(game, mainPlayer, sharedItem, 1)`.
  Cette combinaison identifiait correctement l’objet serveur, mais sérialisait
  son update dans le conteneur du joueur principal. La 0.2.8 conserve le chemin
  Cain complet et passe désormais le proxy validé comme acteur uniquement pour
  les objets de son inventaire partagé. Les chemins inventaire, Cube et coffre
  personnel restent inchangés.
- Le jeu a été arrêté immédiatement après la collecte des preuves. Le runtime a
  été remis sur la 0.2.6 sûre, SHA-256
  `F96077F9AD64AE88E7EB613AF5CB9FC71AB110D66F991C960E1D38F9813879A9`;
  aucune restauration de sauvegarde n’a été effectuée.
- Release x64 et CTest : `1/1` test vert. Les trois exports D2RLoader sont les
  seuls exports. DLL build/dépôt/package : SHA-256
  `BC87601C3EE2FCAFEA5317DD7215FD6E84FC753D399617C7F63B1557E6BF1E6A`.
  ZIP strict DLL + JSON : SHA-256
  `302754DB939CE8D6B03B52DEDA4B27C55663F7E59872DEACFED9196513F7D621`.
  Avant déploiement, `QtyTester.d2s` et `ModernSharedStashSoftCoreV2.d2i` ont
  été copiés dans
  `analysis-cache/runtime-backups/MassID-0.2.8-pretest-20260805-1003/`.
  La DLL runtime 0.2.8 porte le même SHA-256 que le build; aucun jeu n’a été
  démarré. La validation gameplay reste obligatoire avant toute conclusion sur
  le rendu shared et le son vanilla.

- Le cold start autorisé suivant charge bien `MassID 0.2.8`, accepte les hooks,
  applique `17/17` patches et active `12/12` plugins sans rejet ni échec. Le
  témoin de Vincent à `10:09:43` identifie trois objets dans l’ordre gouverné :
  inventaire `1`, Cube `1`, coffre personnel `0`, shared stash `1`; il visite
  `1001` conteneurs partagés et consomme exactement trois charges. Vincent
  confirme le rendu correct en jeu, sans déplacement ni objet gelé.

## Release publique 0.2.9 — 5 août 2026

- Vincent demande une installation personnelle globale, une distribution
  Discord hybride et la suppression de toute variante propre à BKVince. Les
  sources sont déplacées vers
  `addons/MassID/src/`; la DLL, le JSON et les sources sous `data-BKVince/` sont
  retirés. L’historique de validation dans cette mission reste conservé comme
  preuve et ne constitue plus une installation distribuée.
- La configuration suit désormais les dossiers D2RLoader standards : config du
  mod actif en priorité lorsqu’elle existe, chemin de configuration de la portée
  du plugin ensuite, puis `<D2R>/d2rloader/config/MassID.json`. L’ancien JSON à
  la racine du MPQ n’est plus consulté.
- Le README public `addons/MassID/README.md` documente séparément les
  installations globale et mod-locale, la priorité de configuration mod actif →
  portée du plugin → globale, le geste, l’ordre inventaire → Cube → coffre
  personnel → shared stash, ainsi que les deux politiques :
  budget/consommation d’une charge par objet lorsque
  `freeIdentification=false`, ou identification illimitée sans consommation
  lorsque la valeur est `true`.
- Release x64 et CTest : `1/1` test vert. La DLL 0.2.9 est x64, porte les trois
  exports D2RLoader attendus et a pour SHA-256
  `038FC28FB65965C4637D24FB6850F1C34CBD6E2A3256673A9ADB1AD0CCDDEA6B`.
- À la demande explicite de Vincent, le ZIP public contient exactement
  `MassID.dll`, `MassID.json` et `README.md` à sa racine; cette inclusion du
  README constitue l’exception confirmée à l’archive d’incubation minimale.
  SHA-256 :
  `34FBA14176EBDAB379474CBAF789E158FA572C7B3C1B3DBD50A1215C88C5AB46`.
  Sources, symboles, logs et DLL tierces restent exclus.
- Le runtime global reçoit la DLL byte-exacte sous
  `<D2R>/d2rloader/plugins/` et le JSON byte-exact sous
  `<D2R>/d2rloader/config/`. Les deux artefacts mod-locaux sont retirés du profil
  de test.
- Le point `CLIENT_QueueOutgoingPacket 0xEE2A0` est un appel composable, pas un
  hook possédé par MassID. La validation exige désormais une adresse exécutable
  au lieu d’un prologue intact, ce qui autorise le chargement global après le
  PluginPack sans relâcher les signatures des hooks réellement installés.
- Le cold start global BKVince du 5 août charge MassID après les cinq DLL
  eezstreet, résout
  `C:\Games\Diablo II Resurrected\d2rloader\config\MassID.json` et termine avec
  `scanned=12 active=12 rejected=0 failed=0`. La DLL build, package et runtime
  est byte-exacte au hash ci-dessus.

## Compatibilité Cube Output Quantity — 0.2.10, 5 août 2026

- Le rapport coréen fournit deux démarrages reproductibles. D2RLoader accepte
  bien le build `3.2.92777` et charge `CubeOutputQuantity 1.0.2`, qui installe
  d’abord son hook inline à `0x36CFE0`. MassID 0.2.9 vérifie ensuite le prologue
  vanilla de la même entrée `ITEMS_GetInvPage`, refuse la signature modifiée et
  produit l’unique échec du lot : `scanned=18 active=17 rejected=0 failed=1`.
  La branche `kr` et la locale `koKR` ne causent pas cet échec.
- Les preuves reçues sont `mass-id.log`, SHA-256
  `19EC417970AABCB76A27C0BC89525F908868907641914328286A0F92072BA940`,
  et `d2rloader.log`, SHA-256
  `AB2B5B77E40056A9D2690A7E4D996AD1B2EEDC194A807A2DF954133D61FA5860`.
  Les assertions Excel/RapidJSON ultérieures appartiennent au mod actif et sont
  distinctes du refus de MassID.
- Le workbench vérifié prouve que `ITEMS_GetInvPage 0x36CFE0` appelle
  `UNITS_GetItemData 0x34A500`, puis retourne exactement l’octet `ItemData+0x55`.
  La signature de 32 octets de `UNITS_GetItemData` utilisée par 0.2.10 est
  unique dans `.text`. MassID lit désormais ce champ via son propre accesseur et
  ne référence plus l’entrée `0x36CFE0`; aucun hook tiers ni ordre alphabétique
  de chargement n’est requis.
- Release x64 et CTest : `1/1` test vert. La DLL 0.2.10 porte exactement les
  trois exports D2RLoader et son SHA-256 est
  `3C7F3812053F524121D9F5DA38BCF5F069C49EA3FBDB75B78473F0DDCB591FCA`.
  Les copies build, package et runtime global sont byte-identiques.
- Le ZIP public contient exactement `MassID.dll`, `MassID.json` et `README.md`;
  SHA-256
  `EA4D5F553C1F4ACBBACD0159BB740515B9CF85ACAB74A9BDD60B899B2187BFB5`.
  La politique du dépôt autorise explicitement ce seul `README.md` pour MassID
  tout en continuant d’interdire les autres fichiers documentaires aux plugins
  incubés. Le validateur ne signale aucune erreur MassID; son exécution globale
  reste rouge uniquement à cause de huit archives non déclarées appartenant à
  AdvancedItemTooltips, RemoteStash et Transmogrify.
  La DLL globale a été synchronisée sans lancer le jeu.
- Le cold start BKVince explicitement autorisé suivant lance
  `D2RLoader.exe -mod BKVince -txt`, charge MassID 0.2.10 depuis la portée
  globale et résout la configuration globale. Il applique `17/17` patches,
  active `12/12` plugins sans rejet ni échec et atteint `24/24`. Le processus
  reste ouvert pour le témoin gameplay de Vincent.
- Le témoin gameplay local à `12:46:53` capture le Shift-clic droit, accepte le
  paquet privé et identifie deux objets : inventaire `1`, Cube `0`, coffre
  personnel `0`, shared stash `1`, avec `1001` conteneurs partagés visités et
  deux charges consommées sous `freeIdentification=false`. Vincent confirme le
  résultat en jeu. Ce cas est une non-régression MassID 0.2.10 seulement :
  `CubeOutputQuantity.dll` n’était pas installé sur ce profil et la collision
  signalée par le rapporteur coréen reste donc à retester séparément.
- Vincent fournit ensuite le témoin tiers `CubeOutputQuantity.dll`. L’audit
  local confirme une image x64 de `16 384` octets portant exactement les trois
  exports D2RLoader, sans métadonnée de version et sans signature Authenticode;
  son SHA-256 est
  `105482DE46732DA5FC8A0C9245CC059D3E406AE18028439625EC602D00A74B30`.
  Après arrêt propre du jeu, cette DLL est synchronisée byte-exactement dans
  `C:\Games\Diablo II Resurrected\d2rloader\plugins\CubeOutputQuantity.dll`.
  Aucune copie mod-locale homonyme n’est présente et le jeu n’est pas relancé.
- Le cold start BKVince explicitement autorisé à `12:53:03` accepte le build
  `3.2.92777`. `Cube Output Quantity 1.0.2` installe ses trois hooks, y compris
  celui de `0x36CFE0`, puis `MassID 0.2.10` installe ses deux hooks et s’active
  depuis la portée globale sans collision de signature. Les patches terminent
  à `17/17`; le résumé plugins `scanned=13 active=12 rejected=0 failed=1`
  contient un échec distinct de `RepeatableServices.dll`, tandis que MassID et
  Cube Output Quantity sont tous deux actifs. Le jeu reste ouvert pour le
  témoin gameplay Shift-clic droit de Vincent.
- Vincent confirme le témoin gameplay de coexistence. À `12:54:33`, MassID
  capture le Shift-clic droit sur le Tome GUID `38`, reçoit son paquet privé
  `0x34`, puis accepte l’action avec `quantity=39`. Il identifie deux objets
  (`inventory=1`, `cube=1`, `personalStash=0`, `sharedStash=0`) et consomme
  exactement deux scrolls sous `freeIdentification=false`; Cube Output Quantity
  1.0.2 demeure actif dans la même instance. Le gate local de coexistence est
  donc `passed`.

## Prochain gate

La release publique est promue à `1.0.0` après la validation locale complète du
chemin payé 0.2.10 avec Cube Output Quantity 1.0.2; ce changement de version ne
modifie aucune politique, ABI ni surface de hook. Faire confirmer sur le profil
coréen que MassID 1.0.0 charge et exécute la même action avec Cube Output
Quantity 1.0.2 actif. Les validations
`freeIdentification=true`, sauvegarde/relecture et hôte/joiner restent ouvertes
avant de fermer toute la matrice fonctionnelle et la qualification MassID dans
la RuffnecKk D2RLoader Suite.
L’échec distinct de Repeatable Services demeure hors de cette matrice MassID.

La DLL Release x64 `1.0.0` et son test de politique sont reconstruits avec
`1/1` test vert. Les copies build, package et runtime global sont byte-exactes au
SHA-256
`D46ABFB09057130068DF1B23A23A1032E054C661384F4F07FE8C110C40E02460`;
les ressources Windows exposent `FileVersion=1.0.0`, `ProductVersion=1.0.0` et
`CompanyName=RuffnecKk`. Le ZIP public contient exactement `MassID.dll`,
`MassID.json` et `README.md`, pour un SHA-256
`243737E62541CA19A1F6439B5620F3907ACF6782FAB752279C9420AC435E38F8`.
La DLL globale est synchronisée et le jeu demeure fermé, conformément à la
consigne de ne pas relancer sans autorisation explicite.
