# Findings persistants — D2R 3.2.92777

Ce document conserve uniquement les conclusions utiles aux prochaines sessions.
Les sorties volumineuses demeurent sous `analysis-cache/corpus/`.

## Base d'analyse

- Image canonique gouvernee : SHA-256
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`.
- Image d'analyse deterministe : SHA-256
  `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`.
- Le `.text` de l'image canonique est preserve. Les metadonnees PE non-code
  rehydratees rendent 105 850 fonctions x64 accessibles par `.pdata`.
- L'index compact connait actuellement plus d'un million de references code ou
  RIP, les chaines des sections de donnees et les patches BKVince actifs.

## Routine de quantite des Books

- `0x5817BD` charge le delta `-1` apres le controle du type `Book` (`0x12`).
- L'appel a `0x46F090` part de `0x5817CC`; l'index retrouve six callers directs
  de cette routine centrale.
- `0x46F090` synchronise `STAT_QUANTITY` et la quantite du skill lie au
  scroll/tome.
- `0x47145D` est un chemin Javelin (`0x29`) rejete pour les tomes;
  `0x4F5849` est egalement un faux candidat conserve vanilla.

Commandes de reprise :

```powershell
npm run re:d2r32 -- function 0x5817BD
npm run re:d2r32 -- xrefs 0x46F090
```

## pSpell et consommables a skill

- La structure officielle partagee de D2RL-Plugins place `pSpell` a l'offset
  `+0x94` de l'enregistrement item.
- La documentation D2R 3.2 expose des handlers `pSpell` fixes; elle n'expose pas
  un handler generique prenant directement un ID arbitraire de skill.
- `books.txt` associe `ScrollSkill`/`BookSkill`; le `srvdofunc 113`
  `ItemDoBookSkill` utilise le skill du Book/Scroll et met sa quantite a jour.
  Cette voie constitue le prototype data-only prioritaire.
- Le pack officiel `plugin-skills` intercepte deja la consommation native des
  charges a `0x436830` (`D2GAME_SKILLMANA_Consume`). C'est la fondation prouvee
  pour un plugin exact si le clic droit inventaire arbitraire reste requis.
- Les fonctions autour de `0x1AC881`, `0x1AC8F0` et `0x1AC932` lisent bien le
  champ `+0x94`, mais leur role de dispatcher d'utilisation n'est pas prouve.
- Les routines autour de `0x1A7660`/`0x1A77D0` testent une borne `< 16` et sont
  des candidates possibles pour une table de handlers; confiance faible tant
  que callers, arguments et effets ne sont pas etablis.

Sortie brute conservee :
`analysis-cache/corpus/pspell-2026-07-19/pspell-analysis.txt`.

Prochaine etape efficace : partir des xrefs de l'acces `D2ItemsTxt+0x94`, puis
remonter depuis l'evenement serveur d'utilisation d'objet. Ne pas relancer un
scan global du `.text` avant d'avoir epuise l'index et le projet Ghidra.

## Rafraichissement du stock des marchands normaux

- `0x53C9F0` est `D2GAME_NPC_FillStoreInventory` avec l'ABI
  `(game, player, npc) -> void`; son unique caller est `0x540960`.
- La routine resout le `VendorChainEntry`, ecrit `GetTickCount64()` a `+0x38`,
  puis genere le stock aleatoire et permanent par `0x540EA0`.
- Le dispatch normal teste l'etat rempli `+0x34` et le drapeau de refresh
  `+0x35`. Lorsque ce dernier est arme, l'appel unique a `0x502F00` depuis
  `0x540952` nettoie et synchronise l'inventaire vendeur, remet `+0x34` et
  `+0x38` a zero, puis le remplissage est rejoue.
- `0x503290` parcourt la chaine des vendeurs. Son chemin temporise compare le
  timestamp a `GetTickCount64() - 0x3A980`, soit quatre minutes, arme `+0x35`
  et actualise `+0x38`. Ses deux callers directs sont `0x502DEB` et `0x502EE2`.
- Le gamble est distinct : le dispatch appelle `0x541880` depuis `0x540913`
  pour le type d'interface 3; sa limite de generation reste gouvernee a
  `0x541A7E`.
- Le commit epingle `D2RL-Plugins@dc75b49` confirme les structures
  `NpcItemCacheEntry`/`VendorChainEntry` et possede deja le hook canonique de
  `FillStoreInventory` dans `plugin-items`.
- Le layout natif du panel declare deja `button_refresh` avec
  `VendorPanelMessage:RefreshAll`, le sprite gamble, le son natif et `@refresh`.
  La configuration `0x2411E0` utilise `panel+0x168` : normal `0`, repair `1`,
  gamble `2`. Les gates uniques `0x24137D`/`0x241391` identifient le widget
  exclusif au gamble; `0x240E0D` garde le chemin d'entree direct sur le meme mode.
- Le handler de message `0x241B20` compare le hash RefreshAll
  `0xB7AA1748D66EFCAF` et rejoint `0x10F520`. Ce sender n'a que deux references
  dans le panel vendeur et emet l'action 2 via `0xEC730`.
- `0xEC730` construit exactement neuf octets `{opcode, action, npcGuid}`. Les
  callers 92777 prouvent opcode `0x38`, action 1 normal, 2 gamble et 3 hire. Le
  callback serveur `0x4B0470` exige neuf octets, valide le NPC puis route normal
  et gamble vers `0x540850`; ne pas transposer les 13 octets D2MOO 1.10f.
- `0x10CAC0` retourne l'etat gamble utilise par le panel. Le hook client peut
  donc conserver l'action 2 en gamble et choisir l'action 1 en normal sans
  nouveau protocole.
- `0x502F60` recoit `(game, npc, player, mode)` juste avant `0x540850`. Il lit et
  remplace `PlayerData+0x100`; mode 2 normal, 3 gamble, 4 hire. L'ancienne valeur
  2, la classe stockee a `+0xFC`, la classe du NPC et une entree remplie a
  `VendorChainEntry+0x34` forment le gate fail-closed du prototype avant d'armer
  `+0x35`.
- Cold start mod-local du 26 juillet 2026 : hashes source/runtime identiques,
  JSON resolu depuis `mods/BKVince/BKVince.mpq`, hooks `0x10F520` et `0x502F60`
  installes, trois patches UI acceptes avant le log `active`, demarrage graphique
  complet et zero erreur/rejet/mismatch frais. Gameplay et reseau restent non
  executes.

Conclusion : le serveur fournit deja la primitive atomique nettoyage puis
reconstruction et tout le trajet UI/reseau vanilla est maintenant prouve. Le
prototype autonome 0.1.0 compile et ses tests statiques passent. Le prochain
travail efficace est le cold start puis l'observation runtime de la
resynchronisation d'un panel normal ouvert, sans inventer d'overlay ni d'opcode.

## Cube Quick Move Bottom-Right

- La référence sémantique épinglée
  `D2MOO@19019806df7f3e877fa105b05395d1e3597e2316`,
  `source/D2Common/src/D2Inventory.cpp:658-690`, montre que
  `INVENTORY_GetFreePosition` choisit le parcours pondéré bas-droite uniquement
  lorsque la hauteur de l'objet vaut `1`; D2MOO 1.10f ne fournit aucune adresse
  transposable au build 92777.
- L'équivalent 92777 est `INVENTORY_FindFreePosition` à `0x3865B0`. Son ABI x64
  est `(inventory, item, inventoryRecordId, freeXOut, freeYOut, pageByte) ->
  int32`. `ITEMS_GetDimensions` à `0x371850` écrit largeur puis hauteur.
- Le gate `0x386735` porte les octets `40 80 FE 01`. L'égalité appelle l'unique
  xref vanilla de `INVENTORY_SearchBottomRightWeighted` à `0x38D8F0`; les objets
  plus hauts suivent les branches joueur qui aboutissent au parcours haut-gauche
  à `0x38DCC0`.
- Le chemin indirect du Cube autour de `0x4BB8C0` vérifie le code de base `box `,
  sélectionne la page native `3`, résout l'Inventory.txt actif et appelle
  `INVENTORY_FindFreePosition` à `0x4BBA73`. Les cinq octets
  `E8 38 AB EC FF` sont uniques dans `.text` et le retour est `0x4BBA78`, mais
  cette unicité de signature ne prouve pas l'unicité sémantique du chemin Cube.
- Parmi les `36` xrefs de `INVENTORY_FindFreePosition`, huit call-sites écrivent
  explicitement la page `3` via `C6 44 24 28 03` avant leur `CALL rel32` :
  `0x0FA33D`, `0x2C7306`, `0x471D62`, `0x4BBA73`, `0x4C21D6`, `0x4F2C8B`,
  `0x527DC2` et `0x528053`. `0x4C21D6` appartient au parseur d'un paquet de
  `0x15` octets et constitue le candidat principal du Ctrl-clic équipé.
- La construction native de la grille d'occupation est reproduite sans structure
  inventée : `GetItemDataContext` à `0x34A0E0`, contexte temporaire à
  `0x3C6D80` et résolution de grille à `0x38B070` avec `page + 2`. La grille
  expose sa largeur à `+0x10`, sa hauteur à `+0x11` et le tableau de cellules à
  `+0x18`, comme le prouvent les deux branches de recherche 92777.
- `CubeQuickMove 0.1.2` conservait la fonction et le gate partagés intacts et
  remplaçait les huit appels explicites page `3`. Après l'échec gameplay de
  l'épée, une lecture directe du processus a prouvé que les huit `CALL rel32`
  visaient bien le relais `0x3E80000`, puis que `CubeCalls`, `redirected`,
  `vanilla` et `safeFallbacks` valaient tous `0`. Le Ctrl-clic testé ne traversait
  donc aucun de ces huit sites.
- Les `36` xrefs directs se divisent en neuf appels dont le sixième argument est
  constamment `0`, `2` ou `4`, et `27` appels capables de transporter la page
  Cube `3`. À `0x15A25C`, le contrôle client reprend la page dynamique `r14b`;
  son appelant `0x15A2BD` lui passe explicitement `cl=3`. À `0x4FBC0E`, la
  branche serveur choisit dynamiquement la page `0` ou `3`, consomme les
  coordonnées trouvées puis appelle `ITEMS_PlaceItemForPlayer 0x471500`.
- `CubeQuickMove 0.1.3` redirige ces 27 sites vers le même relais, mais ne
  modifie les coordonnées que si la page runtime vaut exactement `3`. Son cold
  start a observé le premier calcul `1x3` à `3,3` depuis `0x15A25C`, contrôle
  préalable de l'espace client.
- Le témoin gameplay suivant identifie le producteur réel au call-site
  `0x15F94F`, dans la routine client `0x15F8B0`. Cette routine copie la paire x/y
  retournée dans sa structure de placement, puis le paquet `0x54` observé porte
  les coordonnées `4,3`. Le serveur accepte le transfert (`result=1`) et Vincent
  confirme visuellement l'épée `1x3` en bas à droite. `0x4FBC0E` reste une
  branche serveur page 0/3 gouvernée, mais n'est pas le témoin de cette action.
- L'audit du PluginPack épinglé
  `eezstreet/D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`
  ne trouve aucune clé Cube ni hook d'inventaire dans `plugin-misc`; les sites
  existants `0x18885B`, `0x18887F` et `0x542F40` sont distincts.

## Book of Lore

- `objects.txt` 3.2 associe `TowerTome` (`*ID=8`) a `OperateFn=6`. Cette
  donnee compilee est confirmee dans BKVince et dans l'extraction vanilla 3.2;
  elle ne fournit toutefois aucun pointeur natif a elle seule.
- `OBJECTS_OperateFunction06_TowerTome` est identifie a `0x5DC570`. Son ABI
  observee est `(operation, operateMode) -> int32`, avec le second argument
  inutilise et `operation+0x00/+0x08/+0x10/+0x20` correspondant au jeu, a
  l'objet, au joueur et a l'identifiant de classe objet.
- Le handler refuse les modes d'animation superieurs a 2, resout la quete 5 par
  `QUESTS_GetQuestData 0x518220`, exige son gate d'introduction, demarre le mode
  operating et l'evenement ENDANIM, puis envoie l'identifiant de chaine 127 et
  avance l'etat A1Q5. Cette structure de controle est l'equivalent exact du
  Tower Tome D2MOO 1.10f, sans transposer son adresse ni ses structures 32 bits.
- La signature etendue de `0x5DC570`, depuis le prologue jusqu'aux appels mode
  d'animation et quete, est unique. Le prologue court seul ne l'est pas : il a
  egalement un temoin a `0x58EFA0` et ne doit jamais etre utilise comme gate.
- `QUESTS_SendScrollMessage 0x517E90` recoit `(game, player, unit, uint16
  stringId)`, resout le client et construit le paquet serveur `0x27` de 0x28
  octets. Le layout observe est header `+0x00`, unit type `+0x01`, GUID `+0x02`,
  compteur de messages `+0x06` et premier string id `+0x0A`. Ses trois callers
  directs incluent Tower Tome a `0x5DC61E`.
- `CLIENT_HandleScrollMessagePacket27 0x19A630` ferme le trajet reseau. Pour un
  objet (`unitType=2`) et le menu ordinaire du Tower Tome, il conserve le GUID
  et transmet le string id de `packet+0x0A` a
  `CLIENT_ShowScrollMessageById 0x197BF0`.
- `CLIENT_ShowScrollMessageById` appelle `LANG_GetStringById 0x5F4A50`, cree le
  panneau natif de texte defilant et reconnait explicitement l'id 127. Son seul
  caller direct est le handler `0x27`; leurs signatures strictes sont uniques.
- Le callsite `0x197C5F` est l'appel direct de
  `CLIENT_ShowScrollMessageById` vers `LANG_GetStringById`. Ses cinq octets
  `E8 EC CD 45 00` sont verifies et Book of Lore `0.2.0` redirige uniquement ce
  site vers un relais proche; la fonction globale de langue reste intacte pour
  AdvancedItemTooltips, Transmogrify et les autres consommateurs.
- Le temoin client `0.2.0` hooke le handler `0x27`, arme une portee thread-local
  seulement pour `unitType=2`, menu `0` et id 127, puis substitue le premier
  texte configure au callsite `0x197C5F`. Il est compile mais non deploye et la
  configuration livree reste desactivee. La selection autoritaire Tower Tome,
  les identifiants prives, les filtres et l'historique ne sont pas implantes au
  runtime.
- La table runtime `OperateFn` n'expose aucun xref ou pointeur brut vers
  `0x5DC570` dans l'image hydratee. L'identite du handler est haute confiance,
  mais la propriete exacte de cette table et son dispatch indirect restent un
  gate distinct avant installation d'un hook de production.

## ExtendedMerc

- Une trace runtime 92777 sur `QtyTester` a capturé l'équipement et le retrait
  vanilla via le paquet client `0x51` de 17 octets. Le layout observé est
  `{opcode, cursorItemId, mercId, equippedItemId, bodyLoc}`; les callsites
  clients sont `0x15C599` et `0x161875` vers le constructeur `0xEC7D0`.
- Le callback serveur est `0x4C0E20`, ABI
  `(game, player, packet, packetSize) -> int32`. Il exige 17 octets, valide le
  mercenaire par son identifiant, lit les deux identifiants d'objet et accepte
  au gate initial un `bodyLoc < 11`. Le succès d'équipement atteint
  `SUNIT_AttachSound` depuis `0x4C1364` avec le son `0x5E`.
- `0x34A330` retourne en réalité le `UnitId` à `unit+0x08`; le nom gouverné a
  été corrigé en `UNITS_GetUnitId`.
- Une instance ouverte de `HirelingInventoryPanel` contient 42 enfants à
  `panel+0x58/+0x60`; les slots partagent le vtable `0x1CF3F20`, leur BodyLoc
  runtime est à `+0x5D8`, leur rectangle à `+0x70` et `isHireable` à `+0x638`.
  Les rings et l'amulet existent avec le drapeau à zéro; gloves et boots sont
  absents. Le passage à un de `slot_right_hand+0x638` a permis à Vincent
  d'équiper un ring qui est resté équipé.
- `0x2C9850` est la factory `InventorySlotWidget`, ABI effective
  `(descriptor ignoré, name, parent) -> widget*`. Elle alloue `0x640` octets,
  appelle le constructeur de base, pose le vtable `0x1CF3F20` et initialise les
  champs directs jusqu'à `isHireable+0x638`. La signature de 37 octets incluant
  la taille d'allocation est unique dans le `.text` 92777.
- Le vtable runtime place le finalizer `0x2CA970` à `+0x08`. Il charge le fond
  configuré par `0x858990` puis délègue au finalizer de base `0x2A8E60`. Les
  constructeurs natifs appellent ce vtable `+0x08` avant
  `UI_Widget_AddChild 0x854DE0`.
- `0x854DE0` a l'ABI `(parent, child) -> void`, ajoute le pointeur au tableau
  d'enfants à `parent+0x58/+0x60/+0x68` et en gère la croissance/ownership. Son
  prologue strict de 18 octets n'apparaît qu'une fois dans le `.text` 92777.
- Le constructeur `InventoryItemWidget 0x2A6FE0` remet le couple `cellSize` à
  zéro à `+0x5B8/+0x5BC`. Les chemins de rendu `0x2A7574/0x2A75A3` et
  `0x2A7969/0x2A7971` consomment respectivement sa largeur et sa hauteur; les
  layouts 92777 assignent ce champ à chaque `InventorySlotWidget`. Le probe
  reprend donc le couple d'un slot existant sain et refuse la création s'il ne
  peut pas l'obtenir.
- Le toggle d'interface appelle `UI_DispatchMessage 0x843D90` à `0xCE3B1` et
  reprend à `0xCE3B6`. La séquence de 23 octets à partir de `0xCE3A9` est
  unique; ce callsite étroit permet d'agir après la création du panneau sans
  prendre un second hook sur l'entrée globale du dispatcher.
- Le probe jetable Release 0.0.1 construit sous `analysis-cache` vérifie toutes
  ces signatures et le vtable avant de tenter la création de `slot_gloves`.
  Une première copie sans ressource-manifeste a été rejetée par D2RLoader avant
  son point de chargement et n'a installé aucun hook. Le build corrigé avec
  manifeste v2, `cellSize` hérité du layout hôte et sprite de socket natif est
  compilé mais non exécuté, à la demande de Vincent. Il remplace désormais le
  seul call `0xCE3B1` par un relais proche géré via `PatchCallRel32`, appelle le
  dispatcher vivant puis agit après son retour; son SHA-256 est
  `ECB258B6129816B09380272187F67228A0F2406693FFADA1DF526DBB97ADF668`.
- Le manifeste PluginPack exact des checkpoints `4f8b276` et `5b56690`
  contient 132 écritures. Aucune ne chevauche `0xCE3B1..0xCE3B5`, l'entrée
  jetable `0xCDE00`, ni les plages témoins des helpers `0x2C9850`, `0x2CA970`
  et `0x854DE0`. Le clone de travail a depuis évolué; il n'est pas utilisé pour
  reformuler rétroactivement cette preuve de coexistence.
- Le réseau, l'autorité serveur et l'adoption d'un widget existant sont donc
  prouvés. Restent ouverts le premier slot absent créé et attaché au runtime,
  son rendu/hit-test, la navigation controller et un placement réel dans les
  BodyLocs gloves/boots.

## MassID — clic de tome et identification autoritaire

- Le handler générique des slots d’inventaire commence à `0x2C7540`. Son ABI
  observée est `(widget, eventState) -> void`; il valide d’abord le slot par
  `0x2C74D0`, résout le propriétaire local, lit l’objet sous le slot par le
  vtable `+0xC8` et sépare l’état souris `5` de la branche clic droit. Le
  prologue strict de 32 octets est unique dans le `.text` 92777.
- Le client Cain à `0x1141AB` appelle `0xEC820` avec l’opcode `0x34`.
  `0xEC820` sérialise exactement un opcode et cinq `uint32`, soit 21 octets,
  avant la queue sortante. Le paquet classique D2MOO de cinq octets n’est donc
  pas transposé au build 92777.
- Le seul callback serveur 92777 correspondant à cette taille et à ce flux
  commence à `0x4AE280`. Il possède l’ABI
  `(game, player, packet, packetSize) -> int32`, exige `packetSize == 0x15`,
  désérialise les cinq champs et rejoint le traitement serveur Cain. Le chemin
  privé MassID peut ainsi être multiplexé avant le flux vanilla sans accrocher
  `D2GAME_PACKETCALLBACK_EntityAction 0x4B0470`, déjà possédé par Vendor Stock
  Refresh dans `plugin-items.dll`.
- `0x46EA70` accepte `(game, item, player)`, pose `IFLAG_IDENTIFIED`, exécute le
  chemin d’item stocké et rafraîchit l’inventaire. Sa structure correspond au
  helper sémantique D2MOO, mais sa confiance reste moyenne jusqu’au témoin
  gameplay MassID.
- `SynchronizeItemAndBoundSkillQuantity 0x46F090` reçoit
  `(game, player, book, delta)`. Le caller tome à `0x5817BD` passe `-1` en `r9d`
  puis le tome en `r8`; la routine lit `STAT_QUANTITY`, calcule la nouvelle
  valeur et synchronise le skill lié. MassID peut donc consommer le nombre exact
  d’identifications réussies sans écrire directement la statistique.
- L’architecture retenue n’accroche ni le callback EntityAction partagé, ni
  `D2GAME_HandleUseItemPacket 0x4F40C0` possédé par Transmogrify, ni
  `CLIENT_QueueOutgoingPacket 0xEE2A0` déjà utilisé par EquippedItemToCube.
- La portée globale a prouvé que `plugin-items.dll` peut accrocher auparavant
  `UI_TOOLTIP_ResolveHoveredUnit 0x2A7810`. MassID compose avec cet owner sans
  écrire ce RVA et valide la plage interne unique `0x2A7820` (16 octets,
  une occurrence `.text`) afin de rester strict sans refuser le chaînage.

## Discipline de promotion

Une adresse n'entre dans `known-rvas.json` qu'apres preuve par structure de
controle, octets/signature, caller/callee ou validation runtime. Les simples
ressemblances et les anciennes adresses 2.4 restent dans cette page avec une
confiance explicite.
