# Findings persistants — D2R 3.2.92777

Ce document conserve uniquement les conclusions utiles aux prochaines sessions.
Les sorties volumineuses demeurent sous `analysis-cache/corpus/`.

## ProgressiveAffixesPlugin — sélection du nombre d’affixes

- `0x442C60` est le générateur Magic appelé avec `(itemWrapper, generation)` ;
  l’item est à `itemWrapper+0x00` et l’ilvl gouverné à `generation+0x18`.
  `0x442C78` charge la décision prefix à `generation+0xA8` et `0x442CDC`
  charge la décision suffix à `generation+0xB4`. Les valeurs positives exigent
  les tentatives qui permettent de garantir un prefix et un suffix.
- Le site prefix `0x442C78` précède `mov rsi, rdx` et `mov rdi, rcx` dans le
  prologue Magic. Un helper appelé depuis ce site doit donc préserver les deux
  arguments volatils malgré l’ABI Windows x64. La v0.2.0 ne le faisait pas et
  pouvait transmettre un faux pointeur à `UNITS_GetUnitType` pendant
  `Game::AddPlayerToGame`; la v0.2.1 appelle le helper avec shadow space et
  restaure `RDX/RCX` avant le retour au prologue natif.
- `0x58A120` est le générateur Crafted `(item, generation)`. Le bloc
  `0x58A1E7..0x58A225` prouve les minima ilvl 1/31/51/71 et l’appel au RNG
  général `0x153B00` avec borne 5. Les distributions vanilla correspondent
  exactement aux poids PD2 `2/1/1/1`, `0/3/1/1`, `0/0/4/1`, `0/0/0/1`.
- `0x58BBA0` est le générateur Rare `(item, generation)`. La branche jewel
  `0x58BC65..0x58BC8E` avance directement le seed et produit 3 ou 4 ; la branche
  ordinaire `0x58BC90..0x58BCAD` roule huit entrées avant la boucle native qui
  respecte les limites de trois prefixes et trois suffixes.
- `ProgressiveAffixesPlugin` possède uniquement les décisions de compte aux
  plages `0x442C78`, `0x442CDC`, `0x58A21B`, `0x58A220`, `0x58BC65` et
  `0x58BC90..0x58BCAD`. Le moteur conserve sélection, application, sérialisation
  et synchronisation des affixes.
- Le PluginPack épinglé `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` ne
  contient aucun affix hook ni ces RVA. Les trois anciens patchsets BKVince sont
  les seuls propriétaires locaux antérieurs et doivent être remplacés
  atomiquement.

Commandes de reprise :

```powershell
npm.cmd run re:d2r32 -- status
npm.cmd run re:d2r32 -- xrefs 0x442C60
npm.cmd run re:d2r32 -- xrefs 0x153B00
npm.cmd run ref:d2rlplugins -- status
```

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

## Magic Find Formula — linéarisation positive

- `D2GAME_ITEMS_RollItemQuality 0x4421B0` est la fonction serveur qui choisit
  la qualité d'un drop; son prologue strict de 38 octets est unique et son seul
  caller direct est `0x440FD9`. L'ABI observée est `(game, seedUnit,
  playerOrKiller, itemLevel, itemId, tcQualityMods) -> itemQuality`.
- La fonction lit la stat 80 sur le joueur ou monstre à `0x44236C`, ajoute celle
  du propriétaire d'un minion à `0x44238F`, traite `MF == 0` séparément puis
  rejette les qualités MF pour `MF <= -100` à `0x4423AA..0x4423AD`. Le registre
  `EBP` reçoit ensuite `MF + 100`.
- Les trois gates de diminishing returns comparent `MF + 100` à `110` et portent
  chacun `7F 04` : unique `0x4423CD` avec constante 250, set `0x44246A` avec
  500 et rare `0x4424F7` avec 600. Leurs témoins étendus de 15 octets sont
  individuellement uniques sous 92777.
- Le bloc magic à `0x442576` divise déjà directement par `MF + 100`; il est donc
  linéaire et ne doit pas être patché.
- Le mode `linear` peut remplacer seulement les trois `7F 04` par `90 90`.
  L'exécution tombe alors sur le chemin natif `ECX = MF + 100`, en conservant
  le gate négatif, les bases et minima ItemRatio, les modificateurs Treasure
  Class, le seuil 128, le RNG et la cascade des qualités. `vanilla` n'écrit rien.
- Aucun patch gouverné BKVince, addon ou PluginPack n'occupe ces six octets. Le
  manifeste du clone PluginPack `db420481` ne chevauche ni la fonction ni les
  sites; ses voisins les plus proches restent `0x441B10`, `0x442D2A` et
  `0x4432F4`. Le propriétaire canonique est `plugin-items.dll` sous
  `items.magicFindFormula`.
- Le module RuffnecKk a d'abord passé son incubation hybride, puis a été fusionné
  dans `plugin-items.dll`. Le manifeste/écritures passe à `139/139`, les tests
  Release à `26/26`, et le témoin autonome a été retiré après validation.
- La matrice runtime intégrée prouve `linear` (`90 90` aux trois sites),
  `vanilla` explicite ou clé absente (`7F 04`) et le refus fail-closed d'une
  valeur inconnue. Le cold start
  final BKVince charge `9/9` plugins, applique `19/19` patches et atteint `24/24`.
  Restent les témoins de drops `MF=-199/-100/-99/0/10/11`, MF positif élevé,
  minion/propriétaire et hôte/joiner.

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

- Le témoin 0.1.6 confirme que ni `0x228AB0` ni `0x2C7540` ne reçoit l'usage du
  tome dans le panneau actif : les hooks sont installés, le curseur Identify
  apparaît, mais aucun compteur client ne bouge. Ces deux surfaces sont retirées
  de MassID 0.1.7.
- `0x1AC830` reçoit l'objet utilisé en `RCX`, résout son record ItemsTxt, lit le
  champ `pSpell` à `+0x94`, collecte les paramètres de l'objet et appelle
  `0x1B9720` avant de retourner `1`. Cette chaîne est la construction cliente de
  l'usage ciblé qui arme le curseur Identify. La signature stricte de 32 octets
  est unique. MassID 0.1.7 l'intercepte avant son effet uniquement pour Shift +
  `ibk `; la confirmation gameplay reste ouverte.

- D2R 92777 possède deux pipelines parallèles pour les slots d’inventaire. Le
  premier témoin étudié commence à `0x2C7540`; il lit l’objet par le vtable
  `+0xC8` à `0x2C7801`, teste Ctrl à `0x2C78CD` puis Shift à `0x2C7CEC`. Le test
  physique de MassID 0.1.4 prouve cependant que le panneau clavier/souris actif
  de Vincent ne passe pas par ses actions `0x2AA9F0`/`0x15F660`.
- Le pipeline réellement utilisé commence à `0x228AB0`, avec la même ABI
  `(widget, eventState) -> void`. Il rejette l’état souris `5` à `0x228AF0`,
  résout l’objet exact par le vtable `+0xC8` à `0x228B2D`, teste Ctrl à
  `0x228BA3`, Shift à `0x228CA4`, puis appelle le comportement vanilla par le
  vtable `+0xD0` à `0x228CED`. MassID 0.1.5 hooke cette entrée avant toute
  délégation : un `ibk ` avec Shift, branche droite et curseur vide envoie la
  requête privée puis retourne, donc le curseur Identify n’est pas armé.
- `0x2AA9F0` accepte `(widget, item) -> void`; son unique caller direct est le
  chemin Shift ci-dessus. `0x15F660` accepte
  `(item, owner, page, flag, state) -> void` et possède trois callers, dont le
  même chemin Shift. Leurs signatures strictes de 32 octets sont uniques.
  MassID 0.1.4 les hookait aux retours `0x2C7D1F/0x2C7D59`; le témoin joueur a
  invalidé ce choix pour le panneau actif et 0.1.5 laisse désormais ces deux
  fonctions intactes.
- Le client Cain à `0x1141AB` appelle `0xEC820` avec l’opcode `0x34`.
  `0xEC820` sérialise exactement un opcode et cinq `uint32`, soit 21 octets,
  avant la queue sortante. Le paquet classique D2MOO de cinq octets n’est donc
  pas transposé au build 92777.
- Le callback serveur 92777 de l’opcode `0x34` commence à `0x4C6C90`. Il
  possède l’ABI
  `(game, player, packet, packetSize) -> int32`, exige `packetSize == 0x15`,
  désérialise les cinq champs et rejoint le traitement serveur Cain. Le chemin
  privé MassID peut ainsi être multiplexé avant le flux vanilla sans accrocher
  `D2GAME_PACKETCALLBACK_EntityAction 0x4B0470`, déjà possédé par Vendor Stock
  Refresh dans `plugin-items.dll`.
- Le témoin MassID 0.2.3 a prouvé que la queue cliente acceptait byte-exactement
  la requête privée `0x34`, tandis que le hook serveur à `0x4AE280` ne recevait
  rien. La table autoritaire commence à `0x1D2A790`, ancrée par le transport
  RemoteStash fonctionnel (`0x18 -> 0x4BFF30` à `0x1D2A850`). Elle place
  `0x4AE280` au slot `0x2E` et le vrai callback `0x34` à `0x4C6C90`, stocké à
  `0x1D2A930`. MassID 0.2.4 corrige ce mauvais RVA.
- `D2GAME_ITEMS_Identify 0x46E8C0` accepte `(game, player, item, flag)`. Le
  caller Cain à `0x53C8F5` passe `1`; la routine pose `IFLAG_IDENTIFIED`, met à
  jour les statlists si nécessaire, envoie `ITEMS_SendItemUpdate`, rafraîchit
  l’inventaire puis appelle `SUNIT_AttachSound(player, 6, player)`. Sa signature
  de 32 octets est unique dans `.text`.
- `0x46EA70` accepte `(game, item, player)`, pose le flag et exécute seulement le
  sous-chemin de statlists. Son unique caller à `0x4FD79D` l’utilise pendant une
  création d’objet avant d’autres étapes de placement. Il ne fournit pas seul
  l’update client et le son nécessaires à MassID; son emploi dans 0.1.0/0.1.1
  expliquait le retour joueur « rien ne se passe ».
- `SynchronizeItemAndBoundSkillQuantity 0x46F090` reçoit
  `(game, player, book, delta)`. Le caller tome à `0x5817BD` passe `-1` en `r9d`
  puis le tome en `r8`; la routine lit `STAT_QUANTITY`, calcule la nouvelle
  valeur et synchronise le skill lié. MassID peut donc consommer le nombre exact
  d’identifications réussies sans écrire directement la statistique.
- L’architecture retenue n’accroche ni le callback EntityAction partagé, ni
  `D2GAME_HandleUseItemPacket 0x4F40C0` possédé par Transmogrify, ni
  `CLIENT_QueueOutgoingPacket 0xEE2A0` déjà utilisé par EquippedItemToCube.
- Le client 0.1.5 n’appelle pas `UI_TOOLTIP_ResolveHoveredUnit 0x2A7810`, déjà
  partageable avec `plugin-items.dll`; il réutilise seulement la résolution
  virtuelle `+0xC8` déjà effectuée par `0x228AB0`. Propriété et page restent
  validées par le callback serveur autoritaire.
- Le témoin 0.1.5 invalide l'ABI précédemment attribuée au second argument des
  handlers `0x228AB0` et `0x2C7540`. Il s'agit d'un état d'événement transmis
  par valeur : chacun le sauvegarde sur sa pile et passe l'adresse de cette
  copie à la méthode virtuelle `widget+0xC8`. Le passage direct de cette valeur
  comme pointeur empêchait la résolution du tome et laissait vanilla armer le
  curseur Identify. MassID 0.1.6 reproduit la copie locale et couvre les deux
  handlers à leur entrée; la validation gameplay demeure ouverte.
- `Ctrl + Left Click to Drop/Move` ne fait pas partie du buffer produit par
  `ITEMS_BuildItemTooltip 0x2BD480`. Drop vient de
  `InventoryItemTooltipAppenderDrop`, aux appels `0x2279BD` et `0x2C552D`.
  Lorsque le Cube est ouvert, vanilla sélectionne
  `InventoryItemTooltipAppenderMove`, résolu à `0x2278DC`, `0x227936`,
  `0x2C5241`, `0x2C528D`, `0x2C53AB` et `0x2CA2E0`. Les pipelines legacy et
  alternatif conservent l’item en `r13`; les appels modernes Move le conservent
  en `r12`, tandis que Drop moderne le garde dans `[rbp-0x78]`. MassID 0.2.5
  redirige seulement ces huit appels de cinq octets vers trois relais proches.
  Le wrapper retourne le texte natif suivi du hint Mass ID sans balise de
  couleur : les deux lignes partagent donc exactement le style gris de
  l’appender actif. La localisation globale et `0x2BD480` restent libres pour
  le PluginPack.
- `LOCALIZATION_GetStringByKey 0x5F4B90` résout aussi `ItemStats1h`; son
  fingerprint sélectionne la même famille de treize locales intégrées
  qu’AdvancedItemTooltips.
- La page item `4` ne suffit pas à découvrir le shared stash depuis
  l’inventaire du joueur principal. Le témoin 0.2.5 identifie le coffre
  personnel mais retourne zéro devant les onglets partagés. Les handlers
  shared-stash `0x4C5570` et `0x4C6480` montrent que ces items appartiennent à
  des `UNIT_PLAYER` auxiliaires : ils résolvent le proxy par GUID, exigent
  l’état `0xBA` avec `STATES_CheckState 0x3351B0`, lisent son inventaire et comparent
  `INVENTORY_GetOwnerId 0x388BA0` au GUID du joueur principal.
- La liste auxiliaire est accessible sans nouveau hook par
  `INVENTORY_GetFirstCorpse 0x388E00` (lecture `inventory+0x68`),
  `INVENTORY_GetNextCorpse 0x38CD70` (lecture `record+0x10`) et
  `INVENTORY_GetUnitGUIDFromCorpse 0x2EF880` (premier dword du record). Le
  caller natif `0x425010` prouve la chaîne record GUID vers
  `SUNIT_GetServerUnit(game, UNIT_PLAYER, guid)`. Le témoin 0.2.6 a retourné
  `sharedContainers=0` parce que le marqueur avait été interprété à tort comme
  une statistique. MassID 0.2.7 reproduit l’appel natif à deux arguments
  `STATES_CheckState(proxy, 0xBA)`, puis impose le propriétaire natif avant toute
  identification; les hooks RemoteStash sur les deux handlers restent intacts.
- Le témoin 0.2.7 a validé cette découverte avec `sharedContainers=1001` et
  `sharedStash=3`, mais a révélé un second contrat : l’acteur passé à
  `ITEMS_SendItemUpdate 0x535F60` détermine le conteneur client. En passant le
  joueur principal pour un item appartenant à un proxy, l’identification était
  persistée dans le `.d2i`, tandis que le client créait un fantôme gelé dans le
  coffre personnel. Les GUID partagés sont demeurés absents du `.d2s`.
- `ITEMS_SendItemUpdate` reconnaît précisément un acteur `UNIT_PLAYER` portant
  l’état `0xBA` à `0x53623C`, vérifie son client propriétaire et appelle le
  sérialiseur `0x536410`. Le retrait shared natif passe le proxy à l’update de
  suppression `0x4C690C`, puis le joueur principal à l’update d’ajout
  `0x4C694B`. MassID 0.2.8 passe donc chaque proxy validé comme acteur de
  `D2GAME_ITEMS_Identify` pour ses propres items; ce routage attend encore son
  témoin gameplay avant promotion comme preuve fonctionnelle.

## CharmZone — zone de charmes autoritaire et rendu coopératif

- `ITEMS_CapturePacketState 0x382D20` expose l'ABI
  `(stateOut, item) -> stateOut`. La sortie de 16 octets contient le mode à
  `+0`, la page d'inventaire à `+4`, `x/y` en deux words à `+8/+10` et la page
  de noeud à `+12`. L'ordre des arguments est prouvé par les mouvements d'entrée
  `RCX -> RSI` pour la sortie et `RDX -> RBX` pour l'item. L'ancien ordre inversé
  écrivait 16 octets dans l'unité item; il a été corrigé avant la version 0.3.1.
- `ITEMS_GetDimensions 0x371850` complète cette position par la largeur et la
  hauteur sur un octet. La règle BKVince peut donc exiger le containment complet
  dans `x=0..10, y=4..7` sans lire de champ privé d'`ItemData`.
- `ITEMS_IsCharmUsable 0x36AE00` est l'équivalent 92777 exact du prédicat que
  BaseMod remplaçait dans D2Common 1.13d, ordinal 10415/RVA `0x47090`. Son ABI
  est `(item, player) -> int32` et sa signature stricte de 32 octets est unique.
  CharmZone appelle d'abord l'original puis refuse seulement les charms qui ne
  tiennent pas entièrement dans la zone. Il ne modifie ni owner ni statlist.
- `STATLIST_MergeStatLists 0x2F81A0`, `STATLIST_GetOwner 0x2F8120` et
  `STATLIST_ExpireUnitStatlist 0x2F8290` restent gouvernés, mais ne sont plus
  possédés par CharmZone. Le prédicat natif offre une surface plus étroite et
  reproduit directement l'architecture BaseMod sans toucher au cycle de vie
  partagé des statlists.
- `UI_RenderItemIcon 0x15BB80` possède l'ABI
  `(item, packedScreenXY, scale, renderParams) -> void`, avec `x` dans le dword
  bas et `y` dans le dword haut; ces valeurs sont le coin supérieur gauche en
  coordonnées écran. Ses quatre callers directs sont `0x2267E9`, `0x2A739B`,
  `0x2A776C` et `0x2CE636`. Le trampoline inline D2RLoader masque le callsite à
  `_ReturnAddress()`: CharmZone filtre donc par un cache lock-free borné des
  pointeurs refusés par `ITEMS_IsCharmUsable`, puis déduplique chaque objet dans
  la file visuelle de la frame.
- Le constructeur de tooltip item `0x2BD480` reste volontairement intact : il
  appartient déjà à la chaîne Transmogrify, AdvancedItemTooltips et
  ExtendedItemStats. FloatingDamage demeure aussi l'unique propriétaire du
  hook D3D12/ImGui. Son callback historique reste réservé à ExtendedItemStats;
  une API de registre multi-overlay rétrocompatible fournit à CharmZone la
  teinte rouge et le message de survol sans second renderer.
- Le manifeste PluginPack épinglé au commit
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` ne hooke ni `0x36AE00`, ni
  `0x382D20`, ni `0x15BB80`. Le cold start mod-local 92777 charge huit plugins,
  zéro échec; un charm synthétique de résistance feu +20 produit exactement
  `0 -> 20 -> 0` hors zone, dans la zone, puis hors zone. La version 0.3.1
  affiche le masque rouge aligné, le message au survol, et finit avec
  `classification failures=0`, `placement failures=0`, `drops=0`.

## Services de quête répétables — paiement et charges natives

- `D2GAME_NPC_TryDeductGold 0x5416D0` reçoit `(game, player, amount)` et
  retourne un booléen. Il additionne l'or porté (`STAT_GOLD=14`) et le coffre
  personnel (`STAT_GOLDBANK=15`), refuse sans aucune mutation si le total est
  insuffisant, puis débite l'or porté avant le reliquat du coffre. Le coffre
  partagé n'est jamais lu. Son prologue strict de 32 octets est unique et le
  handler Repair All l'appelle à `0x53FF7B`.
- Les consommations gratuites sont des routines serveur distinctes par service
  et difficulté : Charsi `0x5DA1C0` utilise la quête 3, Anya `0x547C60` la
  quête `0x26`, et Larzuk `0x548B60` la quête `0x23`. Leur ABI observée est
  `(game, player) -> void`; chacune emploie les mêmes helpers gouvernés de
  lecture, pose et effacement des quest flags. Leurs signatures strictes de 32
  octets sont toutes uniques dans le `.text` 92777.
- Le patch BKVince `infinite-quest-rewards.json` neutralise actuellement deux
  appels pour chacun de ces trois services et ne couvre pas Akara. Il remplace
  donc la consommation des charges gratuites; il ne constitue ni un paiement,
  ni une nouvelle offre NPC après la quête.
- Le client possède déjà les actions et panneaux natifs nécessaires : imbue à
  `0x109100`, respec combiné à `0x109200` avec le texte 11168, socketing à
  `0x109300` et personalization à `0x109400`. Les labels localisés D2R existent
  déjà; BaseMod 1.13 proposait séparément Reset Stats et Reset Skills, mais
  cette séparation ne correspond pas au service natif D2R 3.2. La précédente
  attribution de l'imbue à `0x109500` était erronée : ce bloc enregistre Hire.
- `CLIENT_BuildNpcInteractionMenu 0x1147A0` construit le menu à partir du
  registre natif de 72 octets par NPC. Pour l'entrée texte 11168, le bloc
  `0x114C14..0x114C79` lit les flags `RewardGranted=0` et `RewardPending=1` de
  la quête `0x29` dans la difficulté courante : une charge consommée saute
  l'ajout, une charge pending conserve l'entrée et son callback. L'« émission
  serveur du menu » supposée précédemment est donc corrigée : la visibilité est
  filtrée côté client à partir des quest flags synchronisés; le clic reste
  validé côté serveur.
- Le callback affirmatif `0x1130D0` envoie exactement cinq octets, opcode
  `0x39` suivi du GUID NPC. Une lecture runtime contrôlée du build 92777 prouve
  que la case `0x39` du tableau serveur à `D2R+0x1D2A790` pointe vers
  `D2GAME_PACKETCALLBACK_Rcv0x39_ResetStatsAndSkillsWithNpc 0x4B2530`; les
  cases voisines `0x38`, `0x41` et `0x51` concordent avec leurs callbacks déjà
  gouvernés. Le handler exige la taille 5, valide l'interaction/NPC et exige
  `RewardPending` pour la quête désignée par le service Akara.
- La dernière validation Akara précède immédiatement l'appel à
  `D2GAME_PLAYER_ResetStatsAndSkills 0x580F20` à `0x4B2A23`. Cette transaction
  combinée appelle `D2GAME_PLAYER_ResetSkills 0x4360F0`, qui rembourse les
  ranks dans le stat 5, puis `D2GAME_PLAYER_ResetBaseStats 0x52DDF0`, qui remet
  Strength/Energy/Dexterity/Vitality aux bases de classe et rembourse le stat 4.
  Aucune mutation de stats ou skills ne précède cette couture.
- Après le respec gratuit, `QUESTS_ConsumeAkaraRespecReward 0x5D9AE0` pose
  `RewardGranted` et efface `RewardPending` pour la quête `0x29`. Un repeat paid
  doit donc débiter l'or après la validation finale et avant `0x580F20`, puis
  sauter `0x5D9AE0`; la première récompense native doit garder ce bookkeeping
  strictement inchangé.
- Le moteur serveur partagé de transaction NPC/item commence à `0x4FC230`. Il
  résout l'item demandé et contient les chemins Charsi, Larzuk et Anya, mais sa
  sélection d'opération et son ABI complet ne sont pas encore assez prouvés
  pour en faire un hook. Le chemin opcode `0x34` est aussi partagé avec MassID.
- La couture Akara est désormais prouvée, mais celle des trois services d'objet
  reste à identifier après validation de l'objet et avant sa mutation : débiter
  plus tôt pourrait facturer un objet refusé, et débiter plus tard permettrait
  une mutation gratuite en cas d'échec. L'affichage dynamique et localisé du
  prix reste aussi ouvert; modifier le string id global ne suffit pas, car le
  prix dépend du niveau du joueur qui ouvre le menu.

## RogueScoutMovement — suivi walk/run et vélocité absolue

- `AITHINK_Fn061_Hireable` est confirmé autour de `0x5BEC20`. Ses appels à
  `D2GAME_PETAI_PetMove 0x5C1460` utilisent le motion type `0` pour le suivi
  proche et `1` pour le rattrapage. Les quatre xrefs directes de la fonction
  sont `0x5BEC72`, `0x5BECA1`, `0x5BEFCD` et `0x5BF00A`; la signature
  `48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 70 48 8B F1 4D 63 F1 49 8B C8 49 8B F8 4C 8B EA E8 ?? ?? ?? ??`
  ne correspond qu'à l'entrée `0x5C1460` dans le build 92777.
- L'ABI x64 observée est `(game, owner, unit, motionType, run,
  velocityPercent, steps) -> int32`. La fonction appelle
  `AITACTICS_SetVelocity 0x4A7270`; D2MOO confirme que le paramètre de vitesse
  devient le stat temporaire `velocitypercent`, tandis que `Velocity=11` dans
  `monstats.txt` demeure la base de déplacement. Le chemin de rattrapage remplace
  un argument nul par une valeur aléatoire de 50 à 64 : transmettre simplement
  zéro ne suffit donc pas à garantir la base 11.
- `D2GAME_MONSTERMODE_SetVelocityParams 0x4473F0` reçoit
  `(aiParam, pathType, velocityPercent, distance)`. Il écrit les arguments
  non nuls aux offsets `+0x20`, `+0x24` et `+0x28`; zéro signifie « conserver ».
  Le prototype autonome retient par conséquent un second hook à cette entrée,
  armé seulement pendant un appel `PetMove` motion `0/1`, avec correspondance
  thread-local du pointeur `aiParam`. Une vitesse configurée à 11 efface alors
  explicitement `aiParam+0x24`, sans toucher un autre monstre, un autre chemin
  de mouvement ou un autre thread.
- `UNITS_GetUnitType 0x34B9D0` retourne `[unit+0x00]`,
  `UNITS_GetClassId 0x349860` retourne `[unit+0x04]`,
  `UNITS_GetRoom 0x34B440` et `DUNGEON_IsRoomInTown 0x2F0750` fournissent les
  filtres restants. Le plugin cible seulement le type monstre, la classe
  `roguehire=271`, et préserve tous les motion types autres que `0/1`; combat,
  recul, errance, warp et espacement demeurent natifs.
- L'audit du PluginPack eezstreet épinglé à
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` ne trouve aucun owner de
  `0x5C1460` ou `0x4473F0`. `ReviveOverhaul` hooke `AITACTICS_SetVelocity`
  `0x4A7270`, une entrée distincte en amont; le nouveau hook aval reste inerte
  hors de son scope Rogue et conserve donc la chaîne existante.

## Static Field — package PD2 avec fonctions serveur D2R 3.2

- Le handler serveur natif de Static Field commence à `0x5546B0`. Son ABI x64
  observée est `(game, unit, skillId, skillLevel) -> int32`; il résout la ligne
  SkillsTxt, évalue `calc2`, `calc1` et `aurarangecalc`, puis délègue la
  sélection des cibles à `0x4327D0` avec le callback `0x5556A0`. La signature
  wildcardée de 53 octets ne correspond qu'à cette entrée dans le build 92777.
- Le `srvdofunc=20` D2R ne lit ni `auratargetstate`, ni `auralencalc`, ni les
  paires `aurastat/aurastatcalc`. Le portage exact ne peut donc pas être obtenu
  par la seule ligne `skills.txt`; le `srvdofunc=160` de PD2 est spécifique à
  PD2 et correspond à un autre comportement dans D2R 3.2.
- Le handler natif de malédiction/état commence à `0x55D6B0` avec la même ABI.
  Il valide le target state et le premier aura stat, évalue le rayon et la
  durée, évalue jusqu'à six stats d'aura et énumère les cibles selon
  `aurafilter`. Sa signature wildcardée de 64 octets est elle aussi unique.
- `StaticFieldRework` conserve le handler 20 comme autorité pour les 25 % de
  vie, puis appelle le handler 30 avec le même skill et niveau. La ligne
  BKVince fournit donc seulement les données natives : rayon
  `min(ln12 / 2, 14)`, état `staticfield_debuff`, durée liée à Lightning
  Mastery et `lightresist=-min(lvl,100)`. Les statlists, le filtrage et
  l'autorité multijoueur restent entièrement dans les fonctions D2R.
- L'audit du PluginPack épinglé à
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` ne trouve aucun hook à ces deux
  entrées. Le patch Telekinesis de `plugin-skills` à `0x554936` est voisin mais
  hors du corps de Static Field; les quatre autres DLL ne possèdent aucun site
  chevauchant.

## Mechanics 2.0 MEC-00 — sous-graphe damage 92777

- Le workbench vérifié ferme statiquement `D2Damage` à `0x180` octets. Les
  constructeurs/destructeur `0x4494B0/0x4496E0` prouvent le conteneur SBO et le
  sous-objet possédé; les offsets gameplay utiles sont consignés dans
  `Mission/mechanics-native-proof-92777.md`.
- `SUNITDMG_FillDamageValues 0x44C030` résout Critical/Deadly dans l'ordre
  court-circuité weapon mastery, passive Critical Strike, Deadly Strike. Le
  succès double uniquement `damage+0x18` et pose le même bit
  `resultFlags+0x04 & 0x2000`; l'origine Critical ou Deadly est donc perdue
  après résolution.
- `SUNITDMG_ExecuteEvents 0x44CE80` est une couture autoritaire de commit
  partagée, pas une couture exclusivement melee. Le callsite melee
  `0x44B3FA` passe `bMissile=0`; le missile `0x436F95` passe `1`. Toute
  conclusion « hit melee réussi » doit encore corréler ce flag, `SUCCESS`, le
  caller et un témoin runtime read-only.
- Le life/mana leech possède un consumer unique à `0x450C90`, appelé seulement
  par ExecuteEvents à `0x44D038`. Les pourcentages bruts vivent à
  `D2Damage+0x120/+0x124`, puis sont transformés en montants effectifs après
  Drain, diviseurs de difficulté, troncatures et caps.
- Les jets Critical/Deadly, monster critical et overlay leech prennent le seed
  de l'attacker par `UNITS_GetSeed 0x34A1E0`, mais appliquent le LCG et le
  modulo inline. Aucune primitive callable commune de roll n'est démontrée sur
  ce sous-graphe.
- Le dispatcher `0x5881E0` transporte neuf positions aux callbacks, et non les
  sept arguments exacts du préfixe legacy. Les handlers statiques sont Freeze
  `0x583580`, Open Wounds `0x584170`, Crushing Blow `0x583150` et
  SkillOnAttack/Hit/Kill `0x583B30`. Ils sont appelés indirectement; aucune
  table D2R associant les numéros 14/15/16/20 n'a été retrouvée, et les labels
  numériques reposent sur l'isomorphisme sémantique complet avec D2MOO épinglé.
- Open Wounds appelle le helper curse/statlist `0x433D20`; création, refresh,
  remplacement, expiration et callback `0x436240` sont fermés statiquement.
  La constante BSS duration/stat, les stacks multi-attacker, disconnect,
  fin de game et GUID reuse restent des témoins MEC-01.
- `0x44DF10` calcule résistances/réductions/absorb/total et délègue au resolver
  `0x4523E0`; `0x44A9B0` finalise modes, réactions et mort. La chaîne prouve un
  transport natif cohérent, mais pas encore une primitive sûre d'application
  secondaire ni une architecture `Pd2CombatCore`.
- `0x48E060` est confirmé comme prédicat serveur
  `(game, attacker, candidate)->int32`. `0x4398B0` est seulement `partial` :
  son entrée, ses huit callers, son descriptor room/x/y et son callback sont
  observés, mais ses bornes, rooms adjacentes, doublons et LOS restent ouverts.

## Crafted — niveau requis identique au Rare

- `ITEMS_GetRequiredLevel 0x376DE0` possède l'ABI observée
  `(item, unit) -> int32`. Ses trois xrefs indexées sont le wrapper exporté en
  tail jump `0x371AA0`, l'appel du prédicat partagé d'équipement à `0x36C06D`
  et l'appel récursif des objets socketés à `0x3771D3`.
- Le dispatch lit la qualité à `ItemData+0x00`, puis sépare Magic `4`, Set `5`,
  Rare `6`, Unique `7` et Crafted `8`. Le chemin Crafted initialise `r12d` à
  `10` à `0x376EBA`, parcourt les trois couples préfixe/suffixe et ajoute `3`
  à `0x377089` ou `0x377092` lorsque le record correspondant existe.
- Le témoin `44 8D 61 09 44 8D 79 4D` de l'initialisation Crafted est unique
  dans `.text`. Le témoin de boucle de 24 octets commençant à `0x377084` et
  contenant les deux `ADD r12d,3` est également unique sous 92777.
- Après la boucle, le moteur ajoute le bonus à l'exigence maximale des affixes,
  conserve le cap `maxLevel-1`, puis rejoint le chemin commun. Celui-ci conserve
  les exigences des socketables et skills, ajoute la stat `item_levelreq=92` à
  `0x3779E4`, borne le résultat à zéro et applique l'ajustement dépendant du
  personnage. Le patch ne modifie aucun de ces consommateurs.
- La référence sémantique épinglée
  `D2MOO@19019806df7f3e877fa105b05395d1e3597e2316`,
  `source/D2Common/src/Items/Items.cpp:1306-1334`, porte exactement le même
  triplet `10`, `+3 prefix`, `+3 suffix`; aucune adresse ni structure 32 bits
  n'est transposée.
- `crafted-rare-level-requirements.json` neutralise seulement les trois
  instructions gouvernées. Le nombre d'affixes reste entièrement propriétaire
  de `ProgressiveAffixesPlugin`, les recettes restent inchangées et les objets
  Rare ne deviennent pas des entrées de crafting.

## Environnements sonores — héritage des Terror Zones

- `SOUNDENVIRON_GetRecord 0x3B0BA0` reçoit l'index demandé en `ECX`, résout le
  contexte data `3`, lit le compteur compilé à `DataTables+0x508`, puis rejette
  les index négatifs ou supérieurs ou égaux au compteur. Le bloc d'échec à
  `0x3B0C46` rejoint l'assertion à `0x3B0C82` lorsque le compteur est non nul.
- Le prototype No Terror Zone Music supprimait la ligne stable 75 de
  `soundenviron.txt`. Une demande native de l'environnement 75 confrontée au
  compteur 75 satisfait donc exactement `index >= count`; le texte
  `!SoundGetNumSoundEnviron()` affiché par l'assertion ne signifie pas que le
  compteur devait être nul.
- Dans le résolveur d'environnement hérité autour de `0x20C370`, la ligne
  marquée `InheritEnvrionment=1` part de l'environnement normal courant, puis
  remplace onze champs. La paire commençant à `0x20C4EC` charge `Song` depuis
  `record+0x34`; le store de six octets `89 05 1F B5 88 02` à `0x20C4EF` est
  le seul write du morceau dans cette séquence.
- Cette séquence stricte possède une seule occurrence dans `.text` du build
  92777. Le patch BKVince restaure la ligne vanilla 75 et neutralise uniquement
  ce store, de sorte que le morceau de la zone de base reste actif tandis que
  les dix autres champs hérités continuent d'être copiés nativement.

## Discipline de promotion

Une adresse n'entre dans `known-rvas.json` qu'apres preuve par structure de
controle, octets/signature, caller/callee ou validation runtime. Les simples
ressemblances et les anciennes adresses 2.4 restent dans cette page avec une
confiance explicite.
