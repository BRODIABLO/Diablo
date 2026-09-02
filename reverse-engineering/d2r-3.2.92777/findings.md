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

## Aura Enchanted — pool et scaling PD2

- La table runtime de callbacks à la RVA `0x2395FE0` contient exactement 43
  pointeurs. Son masque nul/non nul correspond aux indices MonUMod hérités et
  l'index `30` pointe sur `MONSTERUNIQUE_UMod30_AuraEnchanted 0x495CD0`.
  L'entrée possède une signature stricte de 32 octets unique sous 92777.
- Le chemin ordinaire lit `STAT_LEVEL=12`, borne le niveau source à au moins 1,
  choisit un record pondéré, calcule
  `multiplier * (mlvl + offset) / divisor`, puis borne le résultat entre 1 et
  99 avant d'ajouter et d'assigner le skill. Lord de Seis impose l'index 5 dans
  Vanilla; Uber Mephisto conserve une branche séparée Conviction 123 niveau 20.
- La table Vanilla à `0x1D1DE70` comporte sept records de `0x18` octets, pas
  huit : `[98/6, 368/6, 108/5, 365/7, 123/8, 122/8, 369/8]`, avec un seuil 20
  sur Holy Shock. La table distincte qui commence à `0x1D1DF20` chevauche
  l'espace qu'occuperait un huitième record; elle ne peut donc pas être écrasée.
- Le patch BKVince place huit records dans la plage `0xA5840..0xA58FF`, à
  l'intérieur d'un intervalle `CC` `0xA583B..0xA5EBF` sans xref indexée. Les
  six lectures de table et les trois bornes de boucle sont redirigées vers
  `[451,452,369,365,368,453,454,455]`, tous pondérés 1 et divisés par 7. Le
  plafond natif devient 13 et l'index spécial de Lord de Seis devient 1, soit
  `MonFanaticism 452` dans le nouvel ordre.
- Les huit rows `skills.txt` sont monster-only et ont une portée doublée.
  Might, Concentration, Fanaticism, Conviction, Holy Fire, Holy Freeze et Holy
  Shock reprennent leurs paramètres PD2. `MonVigor 455` conserve expressément
  la courbe BKVince `Param5=7`, `Param6=50`, soit environ 13–39 % aux niveaux
  d'aura 1–13, sans le passif PD2 additionnel `blvl/2`.
- Au cold start complet du 12 août 2026, D2RLoader a appliqué `17/17` fichiers
  de patches et activé `19/19` plugins, sans rejet ni échec, puis atteint
  `24/24`. La relecture mémoire a confirmé les 19 écritures et les huit records
  exacts. Les tables source/runtime `skills.txt` et `monumod.txt` sont
  hash-identiques; Aura Enchanted reste à `6/6/12`.

La référence sémantique
`D2MOO@19019806df7f3e877fa105b05395d1e3597e2316:source/D2Game/src/MONSTER/MonsterUnique.cpp:601-652`
explique les champs et le flux historique; aucune adresse, structure ou ABI
32 bits n'a été transposée.

## FourthSkillTree Framework — persistance dynamique des skills de classe

- La cible produit est D2R 3.3.93847 et réutilise le corpus natif gouverné de
  provenance 92777. `DATATBLS_GetClassSkillCount 0x33CB30` retourne le compte
  compilé par classe depuis le contexte actif; sa signature stricte de 32 octets
  est unique. `DATATBLS_GetClassSkillIdByIndex 0x33DDE0` retourne l'identifiant
  SkillsTxt d'un index de classe et possède également une signature stricte de
  32 octets unique.
- Le writer de l'en-tête D2S appelle `0x33CB30` à `0x534519`, puis stocke `AL`
  dans le byte `NumSkills`. Le témoin à wildcards
  `E8 ?? ?? ?? ?? 49 8B CF 88 45 9A E8` est unique.
- Le writer de la section skills écrit le magic `if` (`0x6669`) à `0x52F557`,
  puis boucle de zéro jusqu'au compte dynamique obtenu à
  `0x52F58A/0x52F5D7`. Pour chaque index, il appelle `0x33DDE0`, résout le skill
  par `SKILLS_GetSkillById 0x33DCD0`, lit son rang de base par
  `SKILLS_GetBaseLevel 0x33D1E0` et écrit exactement un byte. Il n'existe donc
  aucune constante 30 dans cette boucle native de sérialisation.
- Le lecteur vérifie le même magic `if` à `0x52EC98`, utilise le compte sauvé
  pour parcourir les bytes, borne chaque index contre le compte compilé courant,
  ajoute les rangs par le chemin natif puis avance le curseur de `2 + count` à
  `0x52ED52..0x52ED5A`. Une sauvegarde plus longue reste donc structurellement
  délimitée sans section propriétaire.
- La référence de format épinglée
  `D2SSharp@f26f21897db5c0075e74defca1e31d1930080750` confirme séparément que
  `Character.NumSkills` est un byte lu et écrit dans l'en-tête
  (`src/D2SSharp/Model/Character.cs:32-33,102-104,143-145`) et que la section
  `if` contient un tableau de rangs de longueur `skillCount`, un byte par skill
  (`src/D2SSharp/Model/SkillsSection.cs:7-15,36-37,78-103`). Sa constante 30 est
  le défaut vanilla de la bibliothèque, pas une borne présente dans le layout
  sérialisé.
- Cette preuve ferme l'hypothèse d'un format D2S intrinsèquement limité à 30 :
  le chemin statique natif est déjà piloté par le compte de skills compilé et
  peut représenter jusqu'à 255 entrées par le byte d'en-tête. Elle ne remplace
  pas le gate runtime : un fixture de 31 skills doit encore prouver compilation,
  allocation, Save and Exit, relecture, respec et hôte/joiner sous la pile
  complète avant toute promesse publique.
- Une lecture runtime contrôlée de D2R 3.3.93847 prouve que la case `0x3B` du
  tableau serveur à `D2R+0x1D2A790` contient le pointeur
  `D2GAME_PACKETCALLBACK_Rcv0x3B_AllocateSkillPoints 0x4B3EE0`. Le handler
  exige cinq octets, lit l'identifiant de skill à `packet+1` et le marqueur de
  ranks supplémentaires à `packet+3`, borne l'identifiant contre le nombre
  total de lignes SkillsTxt compilées, résout le skill, son `MaxLvl` et son rang
  de base, puis applique les rangs par le helper serveur `0x438670`.
- Aucun accès à `SkillPage`, `SkillRow` ni `SkillColumn` n'existe dans ce
  callback. L'autorité serveur d'allocation est donc démontrée indépendante de
  la page UI; la validation dynamique du 31e skill reste requise pour fermer le
  trajet complet client, sauvegarde et gameplay.
- Le respec autoritaire `D2GAME_PLAYER_ResetStatsAndSkills 0x580F20`, reçu par
  l'opcode `0x39`, appelle `D2GAME_PLAYER_ResetSkills 0x4360F0`. Cette fonction
  parcourt la liste compilée complète des skills de la classe, retire chaque
  rang de base et crédite leur somme dans le stat 5. Elle n'applique ni filtre
  `SkillPage` ni borne 30; un 31e skill investi doit donc entrer dans le parcours
  natif, sous réserve du témoin dynamique encore ouvert.
- `UI_DispatchMessage 0x843D90` demeure la propriété unique du broker
  `plugin-skills`; RemoteStash redirige seulement le callsite étroit
  `UI_ButtonWidget_OnClick+0xE2`. FourthSkillTree doit composer avec ce broker
  et ne pas installer un second hook sur l'entrée commune.
- Le probe d'allocation du 25 août place le skill 456 sur une cellule native
  Barbarian et étend une sauvegarde gameplay courante de 30 à 31 rangs, avec
  en-tête, checksum et marqueur `JM` cohérents. D2R affiche le personnage level
  99 au menu mais ferme pendant sa matérialisation; le `.d2s` reste
  byte-identique. La greffe directe ne fournit donc aucune preuve d'allocation
  investie et ne sera pas utilisée comme base du prochain témoin.

## Discipline de promotion

Une adresse n'entre dans `known-rvas.json` qu'apres preuve par structure de
controle, octets/signature, caller/callee ou validation runtime. Les simples
ressemblances et les anciennes adresses 2.4 restent dans cette page avec une
confiance explicite.

## Hit Chance Bounds — clamp défensif de la fiche de personnage

- Le runtime courant D2R 3.3.93847 réutilise le corpus natif vérifié provenant
  du build 92777. Le patch BKVince initial appliquait bien ses sept écritures en
  mémoire, mais la capture gameplay conservait `5%` pour
  `Average chance %s will hit you`.
- Le chemin offensif de la fiche de personnage passe par le calculateur
  `0x1514700` puis son formateur à `0x14E7FE0`. Les opérandes déjà gouvernés à
  `0x15149C2/0x15149CD` et `0x14E8068/0x14E8073` appartiennent à ce chemin; les
  deux premiers ne constituent pas un second chemin gameplay autoritaire.
- Le chemin défensif distinct appelle `0x15149F0` depuis `0x14E8242`, conserve
  son résultat brut dans `EBX`, puis appelle le formateur `0x1514CA0` depuis
  `0x14E8262` avec la chance en `ECX` et la sortie texte en `RDX`.
- Le formateur défensif réappliquait le clamp vanilla dans la séquence unique
  `83 F9 05 7D 07 BF 05 00 00 00 EB 0A B8 5F 00 00 00` à `0x1514CBD`.
  L'opérande basse est `0x1514CBF`; l'opérande haute commence à `0x1514CCA`.
- La même fonction sélectionne ensuite les chaînes localisées 10104
  `charmontohit1X` et 10105 `charmontohit2X`. La signature stricte
  `B9 78 27 00 00 E8 ?? ?? ?? ?? 4C 8B CB 89 7C 24 20` est unique à
  `0x1514DBD`, ce qui rattache le clamp oublié au texte observé sans inférence
  fondée sur la seule proximité.
- Le correctif minimal ajoute `05 -> 00` à `0x1514CBF` et
  `5F 00 00 00 -> 64 00 00 00` à `0x1514CCA`. Le jet gameplay gouverné à
  `0x44BD56` reste distinct; la validation visuelle de l'infobulle ne remplace
  pas une validation fonctionnelle du combat.

## Cast Triggers — événement doactive et niveau source

- Le handler serveur central `0x43ACB0` porte l'ABI observée
  `(game, unit, skillId, skillLevel, a5, a6, a7) -> int32`. Ses huit callsites
  directs distinguent les exécutions manuelles `a5=1,a6=0,a7=0` des casts
  d'item `a6=1`. Le retour non nul est produit seulement après le SrvDoFunc ou
  le missile serveur réussi; Cast Triggers dispatch donc après ce retour.
- Le lookup contextuel `0x097790` utilise les tables `+0x11B0/+0x11B8` et le
  stride SkillsTxt `0x2EC` à `0x09780B`. Les offsets `flags +0x24`,
  `anim +0x30`, `seqtrans +0x32` conservent l'ordre sémantique D2MOO après les
  champs modernes insérés. Le filtre accepte `SC=10` ou `SQ=18` transitant
  vers `SC`, et rejette le bit `repeat` 11. Inferno reste donc exclu tandis que
  Lightning/Chain Lightning sont des casts non répétitifs admis.
- L'événement natif `doactive` est l'index 4. Aucun appel natif du dispatcher
  n'a été trouvé avec cet index; le wrapper `0x44D570` accepte un dommage nul,
  construit le contexte attendu et appelle `0x5881E0`. Le plugin ne remplace
  aucune fonction de table d'événements.
- EventFunc20 `0x583B30` lit l'identifiant de stat dans le high word de son
  payload, obtient la chance sur l'unité, effectue le jet modulo 100, puis
  décode le skill id et son niveau à l'aide du shift/masque contextuels. Il
  appelle `0x5896E0` aux callsites `0x583C7F/0x583CB4` ou `0x589820` à
  `0x583CE1`. Melee Splash reste propriétaire de l'entrée EventFunc20 et
  transmet le chemin natif; Cast Triggers ne la hooke pas.
- Les ABI des casters sont `(caster,skillId,skillLevel,target,flag)` pour
  `0x5896E0` et `(caster,skillId,skillLevel,x,y,flag)` pour `0x589820`. Leurs
  signatures strictes étendues à 45 et 43 octets sont uniques. Cast Triggers
  substitue le niveau zéro uniquement dans son TLS `doactive`, puis suspend ce
  contexte pendant le cast déclenché afin qu'aucun proc imbriqué ne l'hérite.
- D2MOO PropertyFunc11 masque le niveau avec `63` avant de l'ajouter au skill
  décalé de six bits. `max=64` devient donc le marqueur zéro sans prendre une
  valeur fixe 1..63. Ce point est une preuve sémantique du format historique;
  le round-trip exact par le compilateur ItemStatCost/Properties D2R 3.3 et le
  niveau effectif observé restent des gates gameplay du fixture intermod.
- Le patch PluginPack Whirlwind CTC à `0x589736` et son équivalent position à
  `0x58986B` se trouvent dans les corps natifs, après les prologues possédés par
  Cast Triggers. Aucun overlap de bytes n'est présent; le cold start pile
  complète reste néanmoins le gate de coexistence autoritaire.
- Références sémantiques uniquement :
  `D2MOO@19019806df7f3e877fa105b05395d1e3597e2316`,
  `source/D2Game/src/SKILLS/Skills.cpp:2445-2582`,
  `source/D2Game/src/SKILLS/SkillItem.cpp:1632-1680,1925-2040` et
  `source/D2Common/src/Items/ItemMods.cpp:3634-3698`. Aucune adresse,
  structure ni ABI 32 bits n'est transposée.

## Burn Damage Fix — production générique et Fire Resistance

- Le corpus commun aux cibles 92777 et 93847 contient à `0x44CB32` le témoin
  unique `81 C3 3C 01 00 00 41 0F 48 DE`. Le chemin a déjà multiplié le Burn
  existant dans `EBX` et avancé le seed unité dans `R8D`, puis additionne à
  tort l'ID de stat `316` comme dommage plat.
- Les stats `burningmin=316`, `burningmax=317` et
  `passive_fire_mastery=329` sont identiques dans les tables 3.2 et 3.3. Le
  producteur missile `0x465799/0x465B40` confirme qu'elles décrivent un range
  de dommage et une maîtrise, pas une constante de DPS.
- Le helper RNG natif `0x4501E0` avance le seed une seule fois puis réduit le
  low32 courant par masque pour une puissance de deux ou modulo sinon. Le
  relais Burn consomme donc `R8D` sans nouvel appel RNG et conserve la borne
  maximum exclusive prouvée sémantiquement par
  `D2MOO@19019806df7f3e877fa105b05395d1e3597e2316`.
- `SUNITDMG_ApplyBurnDamage 0x451380` stocke ensuite le Burn sous forme de
  `HPREGEN` négatif. Le record Fire D2R de `0x40` octets porte résistance `39`,
  maximum `40`, pierce `333`, pierce d'immunité `189`, absorb `%/plat`
  `142/143`, index de réduction `2`, flag `+0x28=1` et log flag `8`.
- Le troisième argument du résolveur `0x4523E0` est conservé dans `R13D`. Le
  témoin unique `0x45251F` prouve qu'une valeur non nulle remplace toute
  résistance positive par zéro via `min(résistance,0)`, puis `0x452658` saute
  aussi l'absorb. La 2.0.0 appelait avec `1` et neutralisait donc par erreur
  résistance positive et immunité. La 2.1.0 appelle avec `0`, utilise les
  sentinelles absorb `-1/-1` et garde explicitement `reductions[2]=0` pour
  exclure MDR sans supprimer les défenses Fire.
- Cette fonction rejette une durée ou un dommage non positif, puis appelle
  `STATES_ToggleState 0x3354C0` avec le défenseur, le state `burning` 115 et
  `enable=1`. Les tables vanilla 3.2, vanilla 3.3 et BKVince relient toutes le
  state 115 à l'overlay `burning` 224, dont l'asset est
  `Expansion\\On_Fire`; l'absence d'overlay signalée n'est donc pas une absence
  de mapping dans les données actuelles.
- Le compilateur StatesTxt passe explicitement un stride `0x44` au callsite
  unique `0x3083D7`, puis installe le vecteur résultant à
  `DataTables+0x290` au témoin `0x30843C`. `STATES_ToggleState` appelle
  `GetItemDataContext 0x34A0E0`, transmet le byte à
  `GetDataTablesForContext 0x300A90` et lit le nombre de states à
  `DataTables+0x298` dans le témoin unique `0x3354E0`.
- Le premier cold start 2.2 a refusé proprement le chargement parce que le
  témoin de stride englobait aussi le `CALL` suivant, que l'intégration du
  compilateur TXT de D2RLoader peut rediriger avant le chargement des plugins.
  Le préfixe instruction-aligné de 22 octets arrêté avant ce `CALL` reste unique
  à `0x3083D7` et prouve intégralement l'argument `0x44`; la cible du `CALL`
  n'est ni consommée ni possédée par Burn Damage Fix.
- La séquence de descripteurs native à `0x307EB3` associe le type
  name-to-word `0x16` à l'offset record `+0x02`, puis aux offsets
  `+0x04/+0x06/+0x08`; le premier word d'overlay est donc directement prouvé à
  `StateRecord+0x02`. L'initialiseur unique de records `0x44` à `0x394640`
  écrit `0xFFFF0000` à `+0`, ce qui combine state id zéro à `+0` et sentinelle
  d'overlay vide `0xFFFF` à `+2`. Ces offsets et cette sentinelle proviennent du
  binaire D2R gouverné, pas d'une transposition de structure D2MOO.
- Burn Damage Fix 2.2 réutilise le hook d'application déjà possédé à
  `0x451380` et n'ajoute aucun hook exécutable. Juste avant le trampoline
  original, il vérifie contexte, count, base, stride, id 115, alignement et page
  writable, puis effectue un compare/exchange atomique strict
  `overlay1 224 -> 0xFFFF`. Une valeur déjà vide est acceptée; toute valeur
  custom est préservée. La mutation reste process-local, ne réécrit aucun
  `states.txt` et est restaurée au déchargement seulement si la DLL possède
  encore la même cellule inchangée.
- Burn Damage Fix 2.0 utilisait `STATES_CheckState 0x3351B0` uniquement comme
  témoin passif après une application positive : il ne créait aucun overlay.
  Le gameplay BKVince du 26 août 2026 a confirmé le DoT, le kill-credit/XP et
  deux states actifs (`resolved=2`, `burning-state=2/0`). Ce comportement est
  conservé ici comme preuve historique, pas comme description de la branche
  2.1.
- Burn Damage Fix 2.1 emprunte `UNITS_SetOverlay 0x349020` sans le patcher et
  rejoue l'overlay `fire_hit` 81 sur la couche 0. Il le fait une première fois
  après une application Burn positive dont le state `burning` 115 est confirmé,
  puis périodiquement pendant les événements de stat-regeneration. Chaque replay
  exige encore le state 115 actif et `SUNIT_IsDead 0x34C2C0 == 0`; il cesse donc
  naturellement à l'expiration du Burn ou à la mort. Aucun pointeur d'unité,
  GUID ou état parallèle n'est conservé entre deux callbacks.
- `D2GAME_EVENTS_PlayerEventDispatcher 0x42CE30` possède l'ABI native à six
  arguments `(game, unit, eventType, callbackArg0, callbackArg1,
  callbackArg2) -> void`. Il borne `eventType` à `0..14`, sélectionne
  `0x42E600` pour le type 3 et possède deux xrefs directes, `0x48CB2F` et
  `0x48CBC4`. Son entrée unique de 32 octets est `48 89 5C 24 08 48 89 6C 24
  10 48 89 74 24 20 57 48 83 EC 30 49 63 D8 41 8B F9 48 8B F2 48 8B E9`.
- `MONSTERMODE_EventHandler 0x447420` possède la même ABI native à six
  arguments. Il vérifie le type monstre, borne l'événement à `0..14`,
  sélectionne `D2GAME_MONSTER_ApplyStatRegen 0x448C00` pour le type 3 et possède
  une seule xref directe, `0x48C83F`. Son entrée unique de 39 octets est `48 89
  5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 50 80 3D 61 76 66 02
  00 41 8B E9 49 63 F8 48 8B DA 48 8B F1`.
- Ces deux dispatchers sont synchrones dans le chemin serveur de la file
  d'événements. Le replay précède le callback original, vérifie explicitement le
  type joueur 0 ou monstre 1, ne requiert ni résolution GUID, ni collection
  globale, ni worker thread, puis transmet les six arguments originaux exactement
  une fois.
- Le témoin unique `0x42E615` lit le frame serveur à `Game+0x170`. Le témoin
  unique de 38 octets à `0x44DF40` lit la difficulté à `Game+0x104`, le contexte
  data à `Game+0x106`, puis appelle `GetDifficultyRecord(dataSet,difficulty)` à
  `0x300830`. La 2.1.0 vérifie ces deux layouts avant toute lecture directe.
- Les CALL uniques `0x42E634` et `0x448CA0` vers `EVENT_SetEvent` prouvent que
  les callbacks reprogramment le type 3 à `gameFrame+1`, mais ne sont pas
  retenus comme hooks : `plugin-misc` possède l'entrée monstre `0x448C00` et
  peut court-circuiter son callsite interne. Les dispatchers sont en amont de
  cette divergence.
- `EVENT_SetEvent 0x48B720` possède sept arguments natifs, et non six :
  `(game, unit, eventType, expireFrame, customId, customParam, arg7) -> void`.
  Après `sub rsp,0x48`, son prologue lit les arguments entrants 5, 6 et 7 à
  `[rsp+0x70]`, `[rsp+0x78]` et `[rsp+0x80]`. La sémantique du septième argument
  reste inconnue; le prototype D2MOO à six arguments est sémantique seulement.
- `UNITS_SetOverlay 0x349020` est confirmé par 39 appels directs, son
  allocation/mise à jour d'une stat-list au flag `0x80`, son ABI
  `(unit, overlayId, unusedLayer) -> void` et sa signature unique de 24 octets.
  Le témoin interne unique de 30 octets à `0x34916C` prouve que le setter écrit
  l'ID dans `unit_dooverlay` stat `178` (`0xB2`); `0x80` n'est pas un ID de stat.
  Cette stat retient le dernier write direct et ne prouve pas qu'une animation
  reste active. Burn Damage Fix 2.1 compte un overlay étranger puis applique
  `fire_hit` selon l'arbitrage natif last-write-wins. Le setter cible bien
  l'unité, mais une particule déjà émise peut rester à son emplacement; le replay
  périodique émet les suivantes à la position courante.
- `SUNITDMG_ApplyResistancesAndAbsorb 0x4523E0` reste un seam partagé que Burn
  Damage Fix ne hooke pas. La 2.1.0 exige soit sa signature vanilla unique de
  32 octets, soit exactement un inline hook suivi par DiagnosticsService et
  possédé par `monsterdisplay`; tous les témoins internes de record, résistance,
  pierce, réduction et absorb restent stricts. Toute modification inconnue,
  non suivie ou multi-propriétaire refuse le chargement.
- L'audit de coexistence ne trouve aucun propriétaire de `0x42CE30` ou
  `0x447420` parmi Monster Display, Bind And Summon, Melee Splash et les autres
  composants actifs de la Suite. Les ressemblances dans Bind And Summon sont
  des entrées `.pdata`, pas du code. Monster Display partage seulement
  `0x4523E0`; BKVCombat emprunte `UNITS_SetOverlay`; Melee Splash possède
  `0x44C030`; et le seam `plugin-misc 0x448C00` est volontairement évité.

## Player sequence tables — baseline D2R 3.3.93847

- `SKILLS_GetSeqNumFromSkill` à `0x33DBC0` lit pour un joueur le byte
  `SkillsTxt.seqnum` à `+0x33` au site `0x33DC42`. Le chemin monstre reste
  distinct; aucune équivalence avec `monseq.txt` n'est inférée.
- `DATATBLS_GetSeqRecordFromUnit` à `0x3CB890` indexe la table runtime
  `0x2386650[seqnum]`, sélectionne une des 14 classes d'armes via la table
  `0x2386730`, puis choisit le record selon le mode et la frame.
- Le descripteur mesure 24 octets. La preuve native à `0x3CB987` construit
  `index * 3 * 8`; les champs capturés sont pointeur de records, nombre de
  frames de séquence, nombre de frames d'animation et QWORD auxiliaire `0x100`.
- Un record mesure six octets : `uint16 sequence`, puis les bytes `mode`,
  `frame`, `direction` et `event`. La baseline contient 808 records, 47
  tableaux runtime et 44 contenus uniques.
- La table contient un slot nul suivi de 25 groupes actifs et 14 classes
  d'armes, soit 350 routes : 235 présentes et 115 nulles. Les groupes 24
  `Cleave` et 25 `Mirrored Blades` sont propres au runtime courant par rapport
  à l'oracle D2MOO utilisé.
- Les 23 groupes legacy et 34 tableaux nommés par
  `D2MOO@19019806df7f3e877fa105b05395d1e3597e2316` correspondent exactement aux
  routes et octets courants qu'ils décrivent. D2MOO reste une preuve sémantique
  seulement; aucune adresse, structure ou ABI 32 bits n'est transposée.
- Tous les groupes, descripteurs et records capturés sous D2R 3.3.93847 ont un
  témoin byte-exact dans l'image d'analyse gouvernée. Le groupe 6 `Inferno` a
  deux seeds statiques identiques (`0x1992660` et `0x1992DF0`), mais une route
  runtime unique; l'extracteur conserve explicitement cette ambiguïté au lieu
  de fabriquer une unicité.
- Le seed statique du mapping des 14 classes est unique à `0x19EAF70` et
  concorde avec la table runtime `0x2386730`.
- Les sorties normalisées, le manifeste de hashes, le captureur runtime et les
  TSV déterministes sont gouvernés par
  `Mission/player-sequence-tables-3.3.md`. Cette phase ne prouve pas encore le
  contrat de propriété, la durée de vie, le remplacement de longueurs variables
  ni l'autorité multijoueur; ces points bloquent toute implantation.

## Scaling des monstres par area level en Normal — garde BKVince

- Le chemin commun 92777/93847 dérive `r15d` de l'index de difficulté effectif :
  zéro en Normal, un en Nightmare et deux en Hell. Le couple natif à
  `0x543D32` est `test r15d,r15d; je 0x543D50`; le saut à `0x543D35` exclut
  donc directement le chemin d'area level en Normal.
- La room passe par le wrapper null-safe `0x2EFC10`, qui retourne zéro en
  l'absence de room puis relaie `DRLGROOM_GetLevelId 0x360FC0`. L'appelant
  conserve ce véritable `LevelId` dans `[rsp+0xB8]` à `0x543C60` avant les
  tests d'éligibilité.
- L'ancienne réécriture RuffnecKk `45 85 F6 7E 19` est invalidée. Le flux
  `0x543CE3..0x543D14` écrase `r14d` avec un booléen avant le gate; il ne teste
  pas le `LevelId` positif annoncé. Le cold start et le témoin mercenaire du
  6 août prouvaient une absence de crash, pas l'effet de scaling.
- La patch externe de `yinyin333333` neutralise correctement le saut Normal
  avec `90 90` à `0x543D35`, mais le test pile complète BKVince du 27 août a
  produit l'assertion `eLevelId > 0` de `LvlTbls.cpp:284` pendant le chargement.
  Le même profil a ensuite chargé avec l'ancien garde, ce qui rend l'admission
  d'un appel BKVince sans room par la version yinyin hautement probable; l'unité
  exacte reste à identifier et ne doit pas être inventée.
- Vincent retient donc une correction privée BKVince Expansion-only. La séquence
  unique de dix octets à `0x543D2D`,
  `80 FA 01 74 1E 45 85 FF 74 19`, devient
  `83 BC 24 B8 00 00 00 00 7E 19`, soit
  `cmp dword ptr [rsp+0xB8],0; jle 0x543D50`. Un `LevelId` positif rejoint le
  chemin d'area level, y compris en Normal; zéro conserve le niveau monstre de
  base. Les contrôles `noRatio`, boss, desecrate et monster-region suivants
  restent natifs.
- Ce remplacement retire volontairement le gate classic-game. Il appartient
  seulement au profil BKVince Expansion et doit être absent de la RuffnecKk
  D2RLoader Suite. Le cold start pile complète est passé le 27 août; un témoin
  effectif Normal et le cas sans room restent requis avant qualification
  gameplay.

## Armageddon et Hurricane en chance-to-cast

- Le helper serveur d'effet d'objet à `0x589930` refuse immédiatement une
  ligne SkillsTxt dont le word `ItemEffect` à `+0x20A` vaut zéro. Une fixture
  Fallen a reproduit l'assertion `ptSkill->nItemEffect != 0`; forcer
  temporairement `ItemEffect=1` supprime cette assertion sans faire apparaître
  Armageddon.
- Le callback partagé `SrvDo124` à `0x575DE0` appelle
  `UNITS_GetUsedSkill`, puis refuse le lancement si le noeud absent ou son
  SkillsTxt ne désigne pas le skill demandé. C'est le second gate indépendant
  du défaut CtC.
- La liste de skills est à `Unit+0x100`, son premier noeud à `+0x00` et le
  used skill à `+0x18`. Un noeud D2Skill porte son SkillsTxt à `+0x00`, son
  suivant à `+0x08`, son seed `Param1` à `+0x24`, son niveau à `+0x40`, son
  owner GUID à `+0x4C` et le filtre du resolver à `+0x54`.
- L'active callback Armageddon à `0x574E90` résout obligatoirement le skill via
  `SKILLS_GetHighestLevelSkillFromUnitAndId` à `0x33DD40`, puis consomme et
  renouvelle `Param1`. L'active callback Hurricane à `0x575600` ne dépend plus
  d'un noeud de skill une fois l'état initial créé.
- Le mécanisme retenu dans `Mission/armageddon-ctc-fix.md` reste synchrone et
  borné : `ItemEffect` est restauré après le helper, le used skill synthétique
  est stack-local pendant SrvDo124, et le noeud Armageddon synthétique est lié
  seulement pendant chaque active callback. Aucun noeud fabriqué n'est
  persistant ni sérialisé.
- Le témoin d'expiration à 250 frames a confirmé que l'état natif s'arrête, mais
  a révélé que la table interne 0.1.0 conservait encore sa graine. La 0.1.1
  emprunte sans le hooker `STATES_CheckState 0x3351B0`, dont la signature
  stricte de 32 octets est unique, pour effacer l'entrée avant le callback si
  l'état a disparu. Après un retour zéro, une seconde vérification efface
  l'entrée seulement si le callback vient de retirer l'état.
- Le retour du callback ne constitue pas seul une preuve d'expiration : la
  référence sémantique D2MOO supprime l'événement et retourne zéro si l'état est
  absent (`SkillDruid.cpp:1074-1081`), mais peut aussi retourner zéro après
  avoir replanifié l'événement lorsque la pièce ou la création du missile ne
  convient pas (`SkillDruid.cpp:1122-1150`). Le prédicat d'état est donc le
  discriminateur correct; aucune adresse D2MOO n'est transposée.
- La preuve sémantique est corroborée par
  `D2MOO@19019806df7f3e877fa105b05395d1e3597e2316`, fichiers
  `D2Game/src/SKILLS/SkillItem.cpp` et
  `D2Game/src/SKILLS/SkillDruid.cpp`. Aucune adresse ni structure 32 bits n'est
  transposée. Les RVA et octets ci-dessus proviennent exclusivement du corpus
  natif gouverné commun aux cibles 3.2.92777 et 3.3.93847.

## MapSense — premier marqueur d'unité dans l'automap native

- `AUTOMAP_RenderUnit 0xD76E0` reçoit `(Unit*, AutomapContext*)`. Sa signature
  stricte de 32 octets est unique et aucun propriétaire concurrent n'apparaît
  dans le PluginPack eezstreet épinglé. L'ancien observateur MapSense qui visait
  la même entrée demeure hors de la cible CMake.
- Le chemin natif appelle `UNITS_GetClientCoordX 0x34AF60`,
  `UNITS_GetClientCoordY 0x34AFB0`, puis
  `AUTOMAP_ProjectClientCoordinatesToScreen 0xD4910`. Le couple projeté est comparé au
  rectangle `context+0x18/+0x1C/+0x20/+0x24`, puis transmis sans translation
  additionnelle à l'icône native `0xD6DB0`.
- `UI_GetNativeWidth 0x7F510` et `UI_GetNativeHeight 0x7F4A0` bornent l'espace
  UI natif. Leur égalité exacte avec `ImGui::DisplaySize` reste une gate
  dynamique : MapSense n'applique un ratio que lorsque les deux échelles sont
  finies, positives et uniformes; toute discordance supprime le marqueur.
- `UNITS_GetUnitMode 0x34AB60` lit `Unit+0x0C`. Le témoin unique `0x51F280`
  prouve le type monstre 1, `MonsterData*` à `Unit+0x10`, les flags de rang à
  `MonsterData+0x1A` et le masque haut rang `0x0E`. Le bit minion `0x10` reste
  corroboré historiquement; son rejet de la catégorie normale est conservateur.
- Le prototype 0.5.0 appelle toujours l'original en premier et exactement une
  fois, filtre un monstre normal vivant dans un rayon circulaire fixe de 60,
  puis publie seulement un POD atomique expirant après 120 ms. Aucun pointeur
  D2R ne traverse vers le thread Present et aucune énumération globale n'est
  ajoutée. Le témoin gameplay doit encore confirmer modes, classification,
  alignement, zoom et comportement centre/gauche/droite/ultrawide.

## MapSense — unités de projection et topologie extérieure Vis/Warp

### Hypothèse rejetée — visibilité native de l'automap et du menu Pause

- PrimeMH au commit `92b6a97d8e56346f8b63a88bb647c1af044d2c8b`
  décrit bien un tableau `MenuStates` où `pause_menu_visible` est à `+0x09` et
  `automap_visible` à `+0x0A`. Son clone local utilise toutefois encore une
  ancienne base codée en dur, `0x1EBD158`; cette structure ne permet donc pas
  de transposer directement les offsets vers le runtime 3.3.93847.
- L'association proposée entre cette structure et les octets `0x2AA6A69` /
  `0x2AA6A6A` était une inférence non démontrée à partir de deux écritures
  adjacentes dans la grande routine de recherche/initialisation `0x1234D0`.
  Les instructions `SETE` à `0x12351D` et `0x1235A9` prouvent leurs seules
  destinations, pas la sémantique des deux octets.
- La preuve runtime du 27 août 2026 réfute l'hypothèse : `0x2AA6A6A` restait à
  zéro pendant que l'automap native était visiblement ouverte. Le candidat
  MapSense 0.10.2 qui exigeait cet octet a donc supprimé tous les marqueurs et
  a été retiré du runtime. Les quatre identifications ont été supprimées de
  `known-rvas.json`; elles ne doivent plus être consommées comme états UI.
- MapSense 0.10.3 traite Tab et Escape à leur message Win32 initial pour vider
  immédiatement ses pixels mis en cache, puis laisse le prochain passage
  gouverné d'`AUTOMAP_RenderUnit` republier les marqueurs au retour en jeu.
  Cette correction ne revendique aucune nouvelle identification native.
- Le contrat visuel est `automap_visible != 0 && pause_menu_visible == 0`.
  Lors du premier passage visible vers caché, MapSense invalide également les
  projections et marqueurs mémorisés : aucune frame âgée de 250 ms ne peut
  réapparaître sur le menu Pause ou après une fermeture remappée.

- Les getters `0x34AF60/0x34AFB0` lisent `Unit+0x38 -> Path+0x08/+0x0C` :
  ce sont les coordonnées client/dimétriques déjà consommées par
  `AUTOMAP_ProjectClientCoordinatesToScreen 0xD4910`, pas les subtiles monde.
  L'écrivain du Path conserve séparément les subtiles à `+0x10/+0x14`, appelle
  `PATH_ConvertSubtileToClientCoordinates 0x334E00`, puis range le résultat à
  `+0x08/+0x0C`. Le corps gouverné de `0x334E00` prouve exactement
  `clientX = 16 * (subtileX - subtileY)` et
  `clientY = 8 * (subtileX + subtileY)`.
- Le résolveur MapSense produisait waypoint et sorties en subtiles
  (`gameTile * 5 + relativeSubtile`), mais la candidate 0.9.6 passait ces
  valeurs directement à `0xD4910` tout en projetant le joueur avec les getters
  client. La 0.9.7 a corrigé cette unité; son témoin gameplay du 26 août montre
  toutefois deux erreurs résiduelles distinctes : le preset du waypoint ne
  coïncide pas avec la position finale de son objet actif, et le centre d'une
  bordure de room ne coïncide pas avec l'ouverture traversable réelle.
- Le waypoint 0.9.8 reste d'abord sélectionné par le preset objet exact et son
  bit `ObjectsTxt.SubClass & 0x40`. Le témoin `0x3289EE` prouve ensuite le
  pointeur `ActiveRoom*` à `DrlgRoom+0x58`; `DUNGEON_GetFirstUnitInRoom
  0x2EFD90` retourne `ActiveRoom+0xA8`, et `UNITS_GetNextUnitInRoom 0x34B4A0`
  suit `Unit+0x160`. Après validation type objet, classe et room, MapSense
  conserve sans conversion les coordonnées de l'Unit données par
  `0x34AF60/0x34AFB0`, exactement comme `AUTOMAP_RenderUnit`. L'absence de
  room ou d'Unit active produit un résultat partiel retryable, jamais le preset
  approximatif.
- Les sorties directes entre niveaux extérieurs ne sont pas garanties dans le
  seul vecteur de rooms voisines déjà matérialisé. Le wrapper pur
  `DRLGLEVEL_GetVisArray 0x360880`, ABI `(uint8 context, Level*) -> int32_t*`,
  lit le niveau et son Drlg puis délègue à `0x360800`. Celui-ci retourne le
  tableau dynamique `Vis[8]` du noeud `Drlg+0x118` (`levelId +0x00`, Vis
  `+0x04`, Warp `+0x24`, suivant `+0x48`) ou le repli LevelsTxt à `+0x48`.
- `DRLGLEVEL_GetWarpId 0x3DAAD0`, ABI
  `(uint8 context, Level*, uint8 slot) -> int32`, applique la même sélection
  dynamique et lit `Warp[slot]`, avec repli LevelsTxt à `+0x68`. La 0.9.8
  accepte seulement une paire réciproque dont les deux Warp valent `-1` et
  dont les rooms portent le bit de slot `1 << (slot + 4)` à `DrlgRoom+0x50`.
- Le linker natif mutateur `0x361750` prouve pour ce chemin `Warp=-1` la règle
  géométrique : les gaps des rectangles `DrlgRoom+0x60/+0x64/+0x68/+0x6C`
  doivent être strictement inférieurs à six game tiles sur les deux axes. La
  0.9.8 utilise ce contrat seulement pour identifier les paires candidates;
  aucun centre ou midpoint géométrique de room ne sert encore de destination.
- `DUNGEON_GetCollisionGridFromRoom 0x2EFB30` retourne `ActiveRoom+0x38` et
  possède une signature stricte unique de 32 octets. Le témoin unique
  `0x36697B` prouve `CollisionGrid+0x20` pour le pointeur de cellules `uint16`,
  l'origine X/Y à `+0/+4` et la largeur à `+8`; `0x3669E6` prouve la hauteur à
  `+0x0C`. Le constructeur `0x363A90` copie les coordonnées subtiles de room
  dans cet en-tête et place les cellules inline à `+0x28`.
- Pour une paire de rooms cardinalement adjacentes, MapSense 0.9.8 balaie le
  bord de collision de la room source déjà active et sa cellule immédiatement
  intérieure. Une position appartient au passage uniquement si les deux masks
  ont le bit mur `1` absent. Les runs de deux subtiles ou moins sont rejetés;
  le centre du plus large run valide devient l'unique ancre, avec un tie-break
  déterministe. Si la room ou sa collision n'est pas encore active, aucune
  ligne approximative n'est publiée et le refresh reste retryable.
- Le parcours des niveaux déjà présents est borné et cyclique-sûr depuis
  `Drlg+0x868`, via `Level+0x1B8`, avec `Level+0x1C8` pour le Drlg et
  `Level+0x1F8` pour l'identifiant. `DRLG_GetLevel 0x3267C0`,
  `DRLG_InitLevel 0x3271C0`, le constructeur de liens `0x361750`, le rebuild
  `0x3608A0`, l'insertion vectorielle `0x3612E0` et toute fonction
  Add/RemoveRoomData ne sont jamais appelés par le nouveau chemin : une donnée
  absente reste partielle et retryable.
- La candidate source 0.9.8 ajoute les signatures strictes des accès Unit,
  ActiveRoom et CollisionGrid à l'empreinte fail-closed commune aux cibles
  3.2.92777 et 3.3.93847. Le build Release `/W4 /WX` et CTest `1/1` passent le
  26 août 2026. Elle n'est ni déployée ni lancée; l'alignement exact observé en
  jeu reste donc `NOT RUN`.
- Le témoin gameplay 0.9.9 affine le défaut intérieur : Tamoe Highland expose
  `raw=1 native=0 exact=0`, Barracks expose également un RoomTile brut mais
  aucune sortie exacte, et Jail Level 1 ne conserve que le lien de retour vers
  Barracks. La ligne mauve Pit est absente et les lignes vertes Barracks/Jail 1
  ne sont jamais publiées. Ce n'est pas une erreur de politique de niveaux :
  les cibles 7→26, 28→29 et 29→30 sont déjà exactes dans la table explicite.
- `DRLGWARP_ResolveRoomTileLink 0x3DA9A0` explique la perte. Après avoir
  matérialisé la source, `0x3DA9C6` lit sa chaîne à `DrlgRoom+0x78`,
  `0x3DA9D0/0x3DA9D4` lisent `RoomTile+0x20 -> LvlWarp+0x2C`, puis
  `0x3DA9FB` suit immédiatement `RoomTile+0x00` vers le `DrlgRoom`
  destination. Le helper exige ensuite à `0x3DA9FE..0x3DAA15` une chaîne
  réciproque à destination et retourne nul si elle n'est pas encore
  matérialisée. Ce gate réciproque est utile à l'ABI complète du helper, mais
  il n'est pas requis pour identifier le niveau cible du preset source.
- La 0.10.0 initialise d'abord, dans le DRLG client gouverné, la cible verte et
  les cibles mauves configurées qui figurent parmi les huit voisins `Vis` du
  niveau courant, par `DRLG_GetLevel 0x3267C0` puis
  `DRLG_InitLevel 0x3271C0` lorsque nécessaire. Elle matérialise ensuite chaque
  room source par `DRLGROOM_CreateActiveRoom 0x3289A0`, lit directement le
  `DrlgRoom*` destination à `RoomTile+0x00`, obtient son LevelId par
  `DRLGROOM_GetLevelId 0x360FC0`, et associe `LvlWarp+0x2C` au `PresetUnit`
  type 5 exact. Le témoin fail-closed de 15 octets à `0x3DA9FB` couvre la
  lecture directe et la preuve de layout; aucun centre de room ni autre point
  approché n'est réintroduit.
- Les tests hors jeu 0.10.0 couvrent toute la progression explicite de l'acte I,
  dont Stony Field→Underground Passage 1, Underground Passage 1→Dark Wood,
  Barracks→Jail 1, Jail 1/2/3, Cathedral et Catacombs 1/2/3/4. Les branches
  configurées Pit Level 1 et Underground Passage Level 2 sont vérifiées comme
  destinations mauves. Release `/W4 /WX`, quatre exports et CTest `1/1`
  passent; la DLL n'est ni déployée ni lancée.
- La trace runtime 3.3.93847 du 29 août 2026 établit que les rafales
  `sFillLocation()` ne sont pas propres à une route : elles sont émises pendant
  la matérialisation native de rooms. Dans `sFillLocation 0x3E1DA0`, la branche
  d'index négatif à `0x3E1F24` appelle uniquement le logger à `0x3E1F2B`, puis
  rejoint à `0x3E1FD8` le chemin qui saute déjà le remplissage. Le CALL exact
  `E8 60 FC 63 00` est unique dans le corpus commun. MapSense 0.12.3 NOPe ces
  cinq octets via le service suivi de D2RLoader sans autoriser d'accès hors
  limites ni modifier la construction des rooms.
- Références sémantiques uniquement :
  `D2MOO@19019806df7f3e877fa105b05395d1e3597e2316`, notamment
  `D2Common/src/Units/Units.cpp:283-323,582-594` et
  `D2Common/src/D2Dungeon.cpp:1303-1310`, ainsi que
  `D2RMH@32d55b8ab9a3e9b380103e73e3c8d328cd4f3ad4`,
  `d2mapapi/mapdata.cpp:55-168,240-380`, pour l'intersection des ouvertures de
  collision. Aucune adresse, structure ou ABI 32 bits n'est transposée.

## MapSense 0.12.0 — table cliente complète et tombe correcte de Duriel

- `CLIENT_GetUnitByIdAndType 0x9A5D0` résout par RIP relatif la table cliente à
  `D2R+0x2A23910`, masque le bucket avec `0x7F` et applique un stride de
  `0x400` octets par type. Cela prouve 128 pointeurs de bucket par type; le type
  monstre 1 commence donc à `table+0x400`.
- Le helper `0x9F270` charge une tête de bucket, compare `Unit+0x08` à l'id et
  `Unit+0x00` au type, puis suit `Unit+0x158`. Son bloc complet de 41 octets et
  l'entrée de 28 octets de `0x9A5D0` sont fingerprintés fail-closed. MapSense
  parcourt uniquement les 128 buckets monstres, toutes les 50 ms au plus,
  avec plafonds par bucket et par scan; aucun pointeur Unit n'est conservé.
- Cette collecte remplace la dépendance incorrecte au sous-ensemble déjà choisi
  par `AUTOMAP_RenderUnit`. Le rayon reste un cercle euclidien en vraies
  coordonnées DynamicPath world-subtile, borné à 30..220. Des compteurs séparés
  `0..80`, `81..140`, `141..220` et `>220` rendent la portée observable.
- `DRLG_GetHoradricStaffTombLevelId 0x326A70` retourne `Drlg+0x120`; MapSense
  n'accepte que les ids 66..72. Le target Level est initialisé seul, puis la
  sortie publiée réutilise l'ancre RoomTile exacte déjà résolue par Navigation.
- `QUESTRECORD_GetQuestState 0x325C50` est appelé sur le record client de la
  difficulté courante chargé depuis `D2R+0x2A48778`, dont le témoin unique est
  à `0x114C20`. L'index sémantique A2Q6=14 et RewardGranted=0 vient de
  `D2MOO@3b21043b99e987bad41cf0f7b49f1f246db52d5c`; RVA, ABI, pointeur et octets
  x64 viennent exclusivement du corpus D2R gouverné.
- Avant RewardGranted, la même ancre de tombe correcte est une destination de
  quête rouge. Après RewardGranted, elle devient la progression verte pour le
  farm de Duriel. Un record, un id ou une sortie encore indisponible reste
  `PartialRetryable`; aucune tombe approximative n'est publiée.

## MapSense 0.13.0 — contrat natif générique des objets et noms de boss

- `UNITS_GetObjectInteractType 0x34AD40` possède un corps complet unique de
  72 octets dans le corpus commun 92777/93847. Il exige un `Unit` type 2, lit
  `ObjectData*` à `Unit+0x10`, puis retourne l'octet
  `ObjectData.InteractType` à **`+0x08`**. Cette preuve x64 remplace toute
  transposition du layout D2MOO 32 bits. MapSense couvre l'entrée et le chemin
  de layout complet avant d'appeler l'accessor.
- `OBJECTS_IsShrine 0x34C470` fournit le classifieur runtime générique : type 2,
  `Unit+0x10`, record `ObjectsTxt` compilé à `ObjectData+0`, puis test du bit
  `0x01` de `SubClass` à `ObjectsTxt+0x127`. Son corps strict de 30 octets est
  unique. Une ligne custom qui conserve ce contrat moteur est donc reconnue
  sans identifiant BKVince ou vanilla.
- La sélection native confirme que `InteractType` est **l'index de ligne
  `ShrinesTxt` sans décalage**. `DATATBLS_GetShrinesTxtRecordCount 0x38FE00`
  lit le compte actif à `DataTables+0x19B8`; le témoin intérieur unique
  `0x50E510` soustrait un, effectue le tirage borné, puis ajoute un. Le domaine
  aléatoire est donc exactement `1..count-1` et la ligne 0 n'est pas choisie.
  `DATATBLS_GetShrinesTxtRecord 0x38FE40` borne ce même index, lit la base à
  `DataTables+0x19B0` et applique un stride compilé de `0x1C`.
- L'entrée véritable de `OBJECTS_InitFunction01_Shrine` est **`0x50E450`**;
  `0x50E510` n'est que sa boucle de sélection, et ne doit pas être publiée
  comme une fonction. Après les remaps natifs `4->2`, `5->3` et `16->18`,
  l'initialiseur passe le même index à `UNITS_SetObjectInteractType 0x34E9D0`
  (`ObjectData+0x08`) et à `DATATBLS_GetShrinesTxtRecord`, puis tail-jump vers
  `UNITS_SetShrineTxtRecordInObjectData 0x34EE70` (`ObjectData+0x10`). Les
  corps D2R uniques recoupent la sémantique de
  `D2MOO@19019806df7f3e877fa105b05395d1e3597e2316`,
  `source/D2Game/src/OBJECTS/Objects.cpp:487-546` et
  `source/D2Common/src/Units/Units.cpp:1972-1982`, sans transposer d'adresse ni
  de layout 32 bits.
- Le prédicat générique fail-closed recommandé pour publier un **texte de
  shrine** est **`InitFn == 1 && (SubClass & 0x01) != 0`**. `InitFn == 1` seul
  est trop large : dans le `objects.txt` BKVince actif, `Obelisk2` porte
  `InitFn=1`, mais `SubClass=2` et `OperateFn=17`; ce serait un faux positif.
  L'intersection exige à la fois le classifieur shrine du moteur et
  l'initialiseur qui assigne réellement une ligne `ShrinesTxt`. Une ligne de mod
  custom demeure donc reconnue si elle conserve ces deux contrats, sans
  allowlist d'ID ou de nom.
- Audit gouverné byte-exact/CRLF du `objects.txt` BKVince actif : 93 lignes ont
  `InitFn=1`, 92 ont aussi le bit `SubClass 0x01`, et l'unique exception est
  `Obelisk2`. Les classes `HealingWell`, `ManaWell1..5`, `JungleHealWell`,
  `HellWell` et `HellManaWell1` portent volontairement `InitFn=1`,
  `SubClass=1`, `OperateFn=2` : malgré leur nom de classe, ce sont des objets à
  sémantique shrine qui reçoivent un vrai record `ShrinesTxt` et leur buff doit
  pouvoir être nommé. Les 14 puits ordinaires `Fountain1..8`, `TombWell`,
  `WellExp`, `WellSnowy`, `WellBaal`, `WellTemple` et `WellIceCave` utilisent
  `InitFn=16`, `OperateFn=22` et `SubClass=0` ou `32`; ils restent donc exclus.
  `OBJECTS_InitFunction16_Well 0x50E600` prouve cette séparation : son corps
  complet unique de 22 octets calcule seulement
  `InteractType = 2 * low8(ObjectsTxt.Parm2)` depuis `ObjectsTxt+0x13C` et
  n'assigne jamais de record `ShrinesTxt`.
- La table compilée active est bornée par
  `DATATBLS_GetObjectsTxtRecordCount 0x38FC70` (`DataTables+0x1530`) et résolue
  par `DATATBLS_GetObjectsTxtRecord 0x38FD00` (base `+0x1528`, stride
  `0x168`, retour nul hors borne). Les dispatchers natifs lisent `InitFn` à
  `+0x15C` et `OperateFn` à `+0x15E`; la corrélation byte-exact entre
  `objects.bin` officiel 3.3 et `objects.txt` établit aussi `SubClass +0x127`
  et `Lockable +0x12F`.
- Le contrat chest fail-closed accepte `InitFn=3` (`ObjectInitChest`) ou
  `InitFn=57` (`ObjectInitBetterChest`). `OperateFn=4` seul est insuffisant :
  la ligne vanilla `MephistoBridge` le porte avec `InitFn=45`, donc serait un
  faux coffre. Inversement, `IceCaveEvilUrn` emploie `InitFn=3` avec
  `OperateFn=68`; le `InitFn` est le meilleur invariant générique démontré.
  Un nouveau callback custom dont la sémantique n'est pas connue reste caché
  ou exige un override sémantique mod-local par clé stable, jamais une liste
  numérique extraite de BKVince.
- Seulement après cette classification coffre, le bit `0x80` de
  `InteractType` signifie locked et les sept bits bas portent le type de trap.
  Le producteur natif de chest lit `Lockable +0x12F` puis pose/efface le bit
  haut à `0x50EA3C`; les consommateurs `0x58E461` et `0x58E837` séparent
  respectivement `>>7` et `&0x7F`. Le dispatcher `0x594630` n'accepte que les
  valeurs `<10`; `0` signifie aucune trap, `1..9` trapped et toute valeur
  supérieure reste invalide/fail-closed.
- `UNITS_GetObjectRuntimeFlagsC8 0x4903D0` retourne l'octet `Unit+0xC8`. Le
  chemin chest `0x58E48D` teste explicitement son bit `0x01`, et la réplication
  cliente copie ce même octet dans `Unit+0xC8`. Après classification coffre,
  ce seul bit autorise l'étoile sparkly/super-chest; les autres bits et objets
  demeurent opaques.
- Les racks se classent sans allowlist par le callback actif :
  `OperateFn=19` pour armor rack et `OperateFn=20` pour weapon rack. Cette voie
  reconnaît aussi les lignes custom qui réutilisent les callbacks moteur; les
  callbacks inconnus restent cachés.
- `UNITS_GetSuperUniqueIndex 0x38E3D0` est couvert par son corps strict complet
  de 92 octets, unique dans le corpus commun. Il vérifie deux fois le type 1,
  lit `MonsterData*` à `Unit+0x10`, retourne le `uint16` à
  `MonsterData+0x2A`, et retourne `-1` sur null, mauvais type ou données
  absentes. Une valeur positive n'est jamais un nom : elle doit être bornée
  par la `SuperUniques.txt` du mod actif, puis résolue par sa localisation
  active. Les boss fixes non-SU ne peuvent être nommés qu'à partir de la ligne
  `MonStats.txt` active et de ses flags `boss`/`primeevil`; les enums statiques
  PrimeMH et le profil test BKVince ne font pas autorité.
- Le filtre de niveau courant partagé par les monstres et POI est maintenant
  entièrement fingerprinté : entrée unique de 32 octets de
  `UNITS_GetRoom 0x34B440`, témoin branches/layout de 36 octets à `0x34B461`,
  accessor complet `ACTIVEROOM_GetDrlgRoom 0x192B20` prouvant
  `ActiveRoom+0x18`, puis corps complet de 14 octets de
  `DRLGROOM_GetLevelId 0x360FC0`. Les deux producteurs refusent donc de se
  charger si une fonction ou un layout de cette chaîne diverge.
- Après durcissement de ces empreintes, la DLL Release se compile sans warning
  traité en erreur et le test `ruffneckk-mapsense-policy` passe. Il s'agit
  d'une validation hors jeu; l'approbation visuelle et la qualification runtime
  complète 0.13.0 restent distinctes.

## 2026-08-30 — ISC12 canonical G1–G4 codec planner

- G1 ajoute neuf mutations sémantiques d'un octet réparties sur quatre
  fenêtres intérieures uniques : `0x37AB2B`, `0x37B7D4`, `0x37F186` et
  `0x37F983`. Elles passent les widths `9→12`, les sentinelles
  `0x1FF→0xFFF` et conservent le seed `previousStatId = -1`. Les entrées
  decoder/serializer restent une preuve statique d'identité et d'ownership au
  ledger, jamais des témoins runtime du groupe; le plan ISC12 ne les revendique
  pas. Le prototype RuffnecKk ExtendedItemStats et son build de fork RuffDood
  les ont historiquement hookées, sans que cela soit attribuable au PluginPack
  officiel eezstreet. Le CALL intérieur du lecteur suivant reste toutefois exact :
  `0x37B7DC` résout le thunk gouverné `0xA1B6C0`; toute redirection est rejetée
  par le préflight avant la première écriture.

- G2 auxiliaire est fermé statiquement par cinq sites mutables exacts uniques :
  CALL exhaustif `0x531A6D` dans la fenêtre `0x531A54`, readers
  premier/suivant `0x530A99`/`0x530BA3`, writer ID `0x5340C0` et terminator
  `0x534139`. Le témoin unique `0x530A6B` gouverne le marker
  `0x6667` et sa branche de rejet; les champs valeur/param pilotés par
  `ItemStatCost` et le contrôle du count compilé restent inchangés. Seuls les
  immédiats ID width `9→12` et terminator `0x1FF→0xFFF` mutent.
- G3 régulier possède les deux CALLs exhaustifs `0x52EC4A`/`0x530A34`, les
  readers `0x53395E`/`0x533A93`, writer `0x5352F6`, terminator `0x5353A8`,
  CALL finalize `0x5353BD` et publication de statut `0x5353C7`. Le témoin
  unique `0x533924` gouverne le marker/bounds moderne. Les huit sites
  constituent un groupe atomique distinct de G2.
- Les consumers de champs sont également gouvernés : `0x530B69` lit en G2
  `CsvParamBits` vers un paramètre 16 bits puis une valeur fixe de 32 bits;
  `0x533A38` et `0x533A52` lisent en G3 le paramètre 16 bits puis dispatchent
  `CsvBits<32` signed/unsigned et `CsvBits==32` unsigned. Le préflight refuse
  aussi tout ID référençant une ligne `CsvBits==0`, comme les readers natifs.
- G4 preview/frontend consomme ce même format sans writer propre. La version
  exacte 105 atteint exclusivement la branche B `0x61D647`/`0x61D690`, dont
  width/sentinelle passent à 12 bits/`0xFFF`. La branche A legacy
  `0x61D247`/`0x61D290` demeure volontairement 9 bits/`0x1FF` comme témoin.
  Le troisième site mutable est le CALL de copie `0x61CF90`, gardé avant B.
- Les seize signatures de mutation G2–G4 ont chacune exactement un match dans
  `.text`. Avec les quatre fenêtres G1, le plan canonique hors runtime contient
  quatre groupes, 20 fenêtres mutables exactes, 49 slots gouvernés et 57 témoins
  runtime inchangés. Il préflight le set G1–G4 complet avant la première
  écriture et classe toute écriture ou flush incertain après ce point comme un
  commit exigeant un cold restart. Aucune de ces mutations n'est publiée.
- G2 alloue exactement `0x4000` octets à `0x534006`; son cap source
  `0x533EAD` et son consommateur `0x53405C` conservent au plus `0x200`
  entrées. Les témoins `0x5340D2`/`0x5340FD` prouvent par entrée la formule
  `12 + CsvParamBits + 32` et son back-edge lié au snapshot. Le snapshot
  clean-sheet impose maintenant `CsvBits <= 32` et `CsvParamBits <= 16`, selon
  les représentations natives 32/16 bits. Le témoin unique `0x5351DD` prouve
  le cas spécial G3 : `CsvBits == 32` saute le clamp `1<<CL` réservé aux
  largeurs 1..31 et rejoint directement l'écriture 32 bits. Le témoin
  `0x535308` charge `CsvParamBits`, réduit le paramètre à 16 bits puis applique
  son guard avant l'écriture, ce qui ferme l'autre moitié du contrat 32/16.
  Le témoin des écritures `0x535352` inclut leur back-edge contre R15 et relie
  ainsi le cap `0x200` au nombre réel d'entrées sérialisées.
  Le pire G2 complet vaut donc
  30 732 bits, soit 3 842 octets plus le marker de deux octets, dans `0x4000`.
  Le coût exact du seul passage 9→12 pour 512 IDs et leur sentinelle reste
  1 539 bits, soit au plus 193 octets selon l'alignement.
- G3 reçoit sa fenêtre de sortie du caller (`0x52F0C3`/`0x52F0E7`), borne son
  snapshot à `0x200` entrées au témoin unique `0x535162`, et son unique caller
  teste le retour à `0x52F522`. Le writer refuse une fenêtre trop petite pour
  son marker à `0x535115`; l'initializer `0xA1B650` met le flag +0x20 à zéro,
  le core `0xA1B7A0` avance les cursors et `0xA1B72C` pose le flag sticky sur
  overflow. Le leaf natif `0xA1B610` calcule le used-end dans RAX.
- Le guard préparé copie dans la page RX persistante un leaf sans stack ni
  registre non volatil : il reproduit exactement le RAX de `0xA1B610` puis
  charge `[bitstream+0x20]` dans EDX. Le CALL `0x5353C2` reçoit un rel32 lié
  uniquement à cette entry par l'autorité loader; ses quatre bytes sont écrits
  et flushés avant le site non chevauchant `0x5353C7`, où `33 C0→8B C2` publie
  EDX strictement en dernier. L'état intermédiaire reste sémantiquement vanilla.
  Les bytes résolus déjà identiques sont des no-op confirmés, mais le range CALL
  complet est flushé. Les bytes `0x5353F0+` appartiennent au PDATA/unwind de la
  fonction suivante et ne sont jamais utilisés comme cave.
- Les deux callers de l'owner player-save sont exhaustifs : `0x41360B` passe
  un buffer stack neuf de `0x4000`; la chaîne
  `0x41E138`/`0x41E186`/`0x41E1E9` alloue et passe `0x8000`. G3 reçoit toutefois
  seulement l'espace restant après un préfixe variable, donc ces capacités
  totales ne remplacent pas le guard du flag d'overflow. Avant publication, la
  façade devra réserver le relais pour la vie du processus avant le premier
  write codec. `CommitPreparedCodecPatchSet` exige maintenant un
  `NativePublicationQuiescenceLease` RAII opaque et vivant pendant chaque
  fingerprint, write et flush. Le SDK loader épinglé ne fournit aucun issuer
  de production et aucun caller loader n'existe. Une perte du lease avant toute
  écriture refuse sans mutation; après la première tentative, elle impose un
  cold restart sans hot rollback.
- Le préflight pur G2/G3 parcourt sans allocation le marker `0x6667`, chaque ID
  12 bits, les champs param/value gouvernés, le cap 512 et la sentinelle
  `0xFFF`; il refuse schéma dangereux, ID hors table, troncation et sentinelle
  absente sans modifier sa sortie. Les fonctions uniques `0x530A00` et
  `0x533760` restent intactes; leurs trois callers exhaustifs sont maintenant
  préparés vers des entries RX process-lifetime. Chaque FRAME ASM transmet
  l'ABI six arguments à un helper `noexcept`, qui accepte seulement v105,
  copie au plus 3 844 octets, tient le lock partagé du snapshot immuable durant
  préflight + appel natif + postcheck et renvoie le statut natif malformé
  `0x12` en cas de rejet. Les trois épilogues exacts uniques
  `0x530BCF`/`0x5338ED`/`0x533ABF` prouvent ce contrat. Un succès natif dont le
  cursor diffère du used-end prédit provoque un fail-fast. Ces rel32 restent
  non publiés.
- La preview alloue puis lit au plus `0x4000` octets (`0x61CF41` et
  `0x61CF81`); son seul gate immédiat exige seulement huit octets. Le thunk
  exact unique `0xA1B6C0` relie les quatre appels ID au cœur du bitreader; le
  core succès unique `0xA1BAD0` avance les cursors, tandis que `0xA1BA92` charge
  la longueur totale, détecte l'overrun et pose `[stream+0x20]=1`. Les boucles
  preview ne lisent pas ce drapeau. Le wrapper G4 préparé appelle l'owner
  partagé `0xA1E110` exactement une fois, après quoi il valide uniquement la
  copie terminée `[temp,temp+N)`: magic, version exacte 105, taille déclarée,
  checksum, contexte `<4`, marker `0x6667` exigé et validé par le wrapper, IDs sous le snapshot,
  champs bornés, cap 512 et sentinelle. Un rejet retourne zéro au gate natif,
  qui rejoint l'exit exact `0x61D87D`, libère `temp` et retourne false.
  G4 reste non publié, mais son contrat de bornes est maintenant fermé.
- Le bloc unique `0x61CF95` exige le magic vanilla `0xAA55AA55` après l'appel
  `0xA1E110`. La trace complète corrige une première lecture trop large :
  `0xA1E110` ne relit pas le filesystem, mais copie sous verrou depuis le buffer
  `SaveObject+0x08`, borné par `SaveObject+0x10`. Le seam G10 `0x9FC654`
  remplace ce même buffer par le payload intérieur accepté avant publication de
  l'état 3; ses callers transmettent ensuite le même SaveObject à la preview.
  G10 domine donc G4 et aucun second unwrap frontend n'est requis. Ce témoin
  verrouille plutôt l'ordre attendu : enveloppe validée/retirée, puis magic
  vanilla du payload intérieur. Les témoins supplémentaires `0xA1E194` et
  `0xA1E1C6` prouvent source/longueur/capacité puis unlock/retour de la copie;
  `0x61D43F`, `0x61D5E4`, `0x61D87D` et le corps complet unique
  `GetDataTablesForContext 0x300A90` ferment respectivement contexte, layout B,
  cleanup et domaine 0..3. Le lookup ItemStatCost peut retourner null et B le
  déréférence sans contrôle, d'où l'obligation du préflight ID<rowCount.

## 2026-08-30 — ISC12 G9 0x9C/0x9D static transport audit

- `D2GAME_SendItemAction9C 0x479CD0` et
  `D2GAME_SendItemAction9D 0x479EA0` passent tous deux `R9D=0` au serializer
  `0x375EE0`. Ils sérialisent donc un seul nœud, jamais tout l'arbre socketé.
  Les fenêtres uniques de 158/163 octets à `0x479D85` et `0x479F76` prouvent
  simultanément ce flag, la capacité `0xF4`, les headers de 8/13 octets, la
  comparaison diagnostique `0xFC`, la queue via `0x4817F0` puis l'appel du
  walker `0x481B50`. Le root `0x9C` est natif jusqu'à 244 octets; chaque `0x9D`
  est sûr jusqu'à 239 octets.
- La comparaison `>0xFC` ne rejette pas le paquet : elle appelle un helper de
  diagnostic puis rejoint le store de longueur et la queue. En `0x9D`, 240..242
  octets donnent des totaux 253..255, 243..244 wrapent la longueur sur un octet,
  et un overflow du serializer retourne zéro à `0x375F25`, produisant un paquet
  header-only de 13 octets. Le même retour produit huit octets en `0x9C`.
- La fenêtre unique de 102 octets `0x481BAD` prouve l'amorce `GetFirstItem`,
  l'appel `0x479EA0` à `0x481BFE`, `GetNextItem` et la branche arrière tant
  qu'un enfant demeure. Le budget est donc une séquence :
  root 9C/9D, puis un 9D indépendant par descendant. Tout guard correct doit
  préflight tout l'arbre avant le premier envoi; refuser seulement l'enfant
  laisserait un état partiellement publié.
- Pour la provenance historique seulement, les hooks d'entrée `0x12E2C0`,
  `0x12E490`, `0x374BF0`, `0x374FF0`, `0x375EE0` et `0x4817F0` appartiennent au
  prototype RuffnecKk ExtendedItemStats, ensuite exercé dans un build du fork
  expérimental `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Ils ne sont
  pas présents dans l'upstream officiel eezstreet et ne constituent ni une
  dépendance ni un gate produit ISC12. Les six témoins G9 restent des fenêtres
  intérieures natives exactes ne revendiquant aucune de ces entrées. Le témoin
  9D commence à `0x12E4B0`; il prouve un buffer `0x101` et la longueur minimale
  13, pas un upper bound client `0xFC`.
- Le planner pur ISC12 calcule `ceil((F + 12*T)/8)` par nœud, où `T` compte un
  token par record et un token sentinelle `0xFFF` par liste, puis applique le cap
  du root selon son type : 244 octets pour `0x9C`, 239 pour `0x9D`, puis 239 à
  tous les descendants `0x9D`. Il valide maintenant un snapshot packed complet
  avant son premier callback : counts, offsets, indices, enfants partagés,
  cycles, nœuds inaccessibles et payload tardif hors budget produisent tous zéro
  callback. Le preorder depth-first `{root, premier enfant, ses descendants,
  sibling suivant}` reproduit le walker natif : le producer 9D queue à
  `0x47A001`, récursive via `0x47A014`, puis le walker parent avance à
  `0x481C06`.
- Le nombre de sockets disponibles et le nombre d'enfants occupés sont deux
  cardinalités distinctes. La référence sémantique épinglée
  `D2MOO@19019806df7f3e877fa105b05395d1e3597e2316`,
  `source/D2Common/src/Items/Items.cpp:6668-6694`, compte et écrit les enfants
  occupés, tandis que `:7211-7214` écrit séparément
  `STAT_ITEM_NUMSOCKETS`; `:3270-3282` compare encore capacité et occupation.
  Le planner impose donc `listed == occupied` et `occupied <= capacity`, jamais
  leur égalité systématique. La borne D2MOO de 3 bits/7 enfants reste seulement
  sémantique; la limite encodée exacte de D2R doit être promue avant de fixer la
  capacité d'une transaction native.
- Vincent retient le 30 août 2026 **G9-A native-only**. Le planner ferme le
  contrat logique hors runtime, mais aucun transport de production ne l'appelle
  encore et zéro callback ne prouve pas encore zéro queue D2R. Aucun transport
  externe ne fait partie du produit. Aucun runtime ni save n'a été lancé pendant
  cet audit.
- Un preflight par seconde sérialisation au hook d'entrée est rejeté. `0x9C`
  sauvegarde puis modifie temporairement les flags `ItemData+0x18` à
  `0x479D54/0x479D67` avant la sérialisation, puis les restaure à
  `0x479DEC/0x479DFD`. `0x9D` applique le même traitement à `0x479F23..0x479F3B`
  et peut aussi modifier temporairement `ItemData+0x54` à `0x479F4A..0x479F5E`.
  Une sérialisation scratch exécutée à l'entrée ne voit donc pas nécessairement
  les mêmes octets que le producer réel. Le wrapper `0x375EE0` reçoit sept
  arguments : item, destination, capacité, `bServer`, `bSaveItemInv`,
  `bGamble`, callback/sink; le layout et l'idempotence d'un sink recréé ne sont
  pas gouvernés non plus.
- Au moment de l'audit statique initial, le seam natif étroit recommandé n'était
  pas encore implanté. Il conservait une seule sérialisation native et devait
  détourner seulement les deux CALL de queue :
  `0x479E10 -> 0x4817F0` pour `0x9C` et `0x47A001 -> 0x4817F0` pour `0x9D`.
  Des wrappers aux entrées `0x479CD0/0x479EA0` ouvriraient une transaction TLS;
  les deux relays copieraient synchroniquement chaque paquet stack sans
  l'envoyer, puis le wrapper root publierait la séquence via l'entrée queue
  intacte seulement après validation globale. Les signatures d'entrée uniques,
  les CALL rel32 exacts et les séquences prep/restore ont été relevés. Restent à
  fermer avant implantation : bornes de profondeur/nœuds, réentrance TLS,
  cardinalité sémantique StatLists/sentinelles/compound stats, stabilité de
  l'arbre pendant la traversée, durée de vie des relays et publication
  quiescente des quatre sites. Une panne de queue pendant le flush final ne peut
  pas être rollbackée; la garantie visée demeure exactement zéro queue pour un
  rejet de validation. `D2GAME_QueueServerPacket` reçoit une longueur `size_t`
  dans `R8`; la fenêtre unique de 39 octets `0x4818B6` prouve qu'elle consomme
  synchroniquement la plage `[packet, packet+length]`, donc le relay doit copier
  le paquet stack avant son retour. Le flush tardif conserve l'ordre des appels,
  mais décale les effets internes de queue après la sérialisation de tous les
  descendants; cette différence de timing/backpressure reste à qualifier.

## 2026-08-30 — ISC12 G9-A native wrapper/relay ABI promotion

- Le workbench gouverné commun aux builds 92777 et 93847 est vérifié avant cette
  promotion. `D2GAME_SendItemAction9C 0x479CD0` possède l'ABI exacte
  `void(client RCX, item RDX, action R8B, temporaryFlags R9D,
  gamble:uint32 stack arg5)`. Ses onze callers directs sont `0x4790DD`,
  `0x47977D`, `0x479B3C`, `0x479BA0`, `0x47D5A1`, `0x47D60D`, `0x47E8FB`,
  `0x47EEF5`, `0x480022`, `0x48007D` et `0x4802D2`; aucun ne consomme `RAX`.
  `D2GAME_SendItemAction9D 0x479EA0` possède l'ABI
  `void(client RCX, parent RDX, item R8, action R9B,
  temporaryFlags:uint32 stack arg5, gamble:uint32 stack arg6)`. Ses douze
  callers directs sont `0x477574`, `0x478804`, `0x4797C8`, `0x479814`,
  `0x479848`, `0x47E934`, `0x47E99C`, `0x480300`, `0x480388`, `0x4808B4`,
  `0x481514` et le walker `0x481BFE`; eux aussi ignorent tous `RAX`.
- Les deux entrées commencent par les cinq octets complets
  `40 53 55 56 57`, soit `push rbx/rbp/rsi/rdi`. Les continuations
  `0x479CD5` et `0x479EA5` commencent respectivement par `push r14` et
  `push r15`. Le producer 9C possède ensuite une allocation `0x160`, un delta
  de frame total `0x188`, son cookie à `RSP+0x150` et son arg5 entrant à
  `RSP+0x1B0`; 9D possède `0x170`, `0x198`, cookie `RSP+0x160` et args5/6 à
  `RSP+0x1C0/0x1C8`. Un `JMP rel32` cinq octets est donc instruction-aligned :
  le trampoline rejoue exactement les quatre pushes puis rejoint `entry+5`.
  Les signatures complètes uniques de 32 octets restent les témoins de
  préflight, car les cinq octets volés seuls sont identiques entre les entrées.
- Le trampoline dix octets `40 53 55 56 57 E9 <rel32 entry+5>` exige une table
  d'unwind process-lifetime. Son `UNWIND_INFO` minimal exact est
  `01 05 04 00 05 70 04 60 03 50 02 30` : version 1, prologue cinq octets et
  quatre `UWOP_PUSH_NONVOL` aux offsets 5/4/3/2 pour RDI/RSI/RBP/RBX. Les
  `UnwindData` du PDATA réhydraté statique ne décodent pas de manière fiable;
  ils ne prouvent donc pas l'unwind natif. Avant publication, le runtime doit
  obtenir `RtlLookupFunctionEntry(entry+5)`, valider les pushes/allocations live,
  enregistrer le trampoline par `RtlAddFunctionTable` et garder code, table et
  unwind jusqu'à la fin du processus. Un wrapper C++ doit nettoyer le TLS sous
  SEH `finally` ou équivalent; la seule RAII `/EHsc` ne ferme pas un SEH natif.
- Les CALLs mutables sont exactement `0x479E10: E8 DB 79 00 00`, continuation
  `0x479E15`, et `0x47A001: E8 EA 77 00 00`, continuation `0x47A006`. Ils
  transmettent tous deux `void(client RCX, packetStack RDX,
  zeroExtendedByteLength:size_t R8)`. Aucun xref ne cible les CALLs, leurs
  continuations ou les intérieurs `0x479D85/0x479F76`; les 23 appels directs
  recensés passent par les deux entrées wrappers. Un relay TLS-inactif après
  publication est donc une invariant violation fail-closed : il supprime
  l'envoi et arme l'état fatal au lieu de contourner le preflight.
- Les anciens témoins contenant les CALLs sont désormais scindés et chacun est
  unique : `0x479D85+139`, CALL `0x479E10+5`, continuation walker
  `0x479E15+14`, puis témoin d'épilogue code `0x479E23+30` jusqu'au `RET`
  inclus/end-exclusive `0x479E41`; ensuite `0x479F76+139`, CALL
  `0x47A001+5`, continuation walker `0x47A006+19`, puis témoin d'épilogue code
  `0x47A019+30` jusqu'au `RET` inclus/end-exclusive `0x47A037`. Les témoins
  d'épilogue prouvent seulement les bytes de load/xor/check du stack cookie,
  deallocation, pops et `RET`; le `.pdata` statique n'est pas autoritatif pour
  les futurs wrappers d'entrée. Leur contrat unwind demeure un gate runtime
  par `RtlLookupFunctionEntry`. Aucune fenêtre déclarée inchangée ne chevauche
  donc une mutation. L'entrée queue intacte
  `0x4817F0+22` fixe l'ABI `(client, bytes, size_t)->void`; le témoin unique
  `0x4818B6+39` construit `[bytes,bytes+length]` avant l'appel du sink. Chaque
  relay doit copier le paquet stack synchroniquement avant son retour, puis le
  wrapper root détache une batch immuable et la flush directement via
  `0x4817F0` seulement après validation complète. Le retour `void` ne permet
  aucun rollback si une panne arrive après le début de ce flush.
- La topologie imbriquée est synchrone et même-thread. Les deux producers
  appellent `0x481B50` après leur relay; le walker appelle chaque enfant à
  `0x481BFE` comme `9D(même client, item parent actif, enfant, action 0x12,
  temporaryFlags propagés, gamble 0)`. Une frame TLS nested peut donc exiger
  exactement ces invariants, un seul relay du bon opcode par producer et aucune
  9C nested. Une 9D entrée depuis l'état Idle demeure une root légitime. Pendant
  un rejet, le corps et le walker natifs terminent afin de restaurer leur état,
  mais chaque relay reste supprimé; succès et réentrée de flush nécessitent des
  banks distinctes afin qu'une batch en cours ne puisse pas être écrasée.
- Les quatre hooks, relays, trampolines et témoins sont maintenant gouvernés
  comme **source préparée**, jamais comme runtime qualifié ou publié. Restent
  bloquants : autorité loader de quiescence, unwind live, nettoyage SEH,
  reentrance pendant le flush, validation des octets capturés et des paquets
  header-only 8/13, stabilité inter-thread de l'arbre, effets de queue différés,
  backpressure et preuve zéro queue pour chaque rejet root/middle/sibling/deep.
  Aucun runtime, paquet ou sauvegarde n'a été exécuté pendant cette promotion.

## 2026-08-30 — ISC12 G1 bounded complete-item serializer and G9 cardinalities

- Le corps natif complet de sérialisation d'un item commence à `0x37D140`.
  Son prologue exact unique de 37 octets, jusqu'à `sub rsp,rax`, fixe `RBP` à
  `final RSP+0x100`, sonde
  puis alloue une frame de `0x1970` octets. L'outer serializer ne possède qu'un
  CALL direct vers ce corps, à `0x3800F0`, couvert par la fenêtre unique
  `0x3800E8`. Ces témoins appartiennent au corpus gouverné 92777 commun aux
  builds ciblés 3.2.92777 et 3.3.93847 par équivalence native byte-exacte; aucun
  RVA ni ABI D2MOO n'est transposé.
- À `0x37F08A`, un `memset` exact unique initialise seulement `0x7FC` octets à
  `[RBP+0x60]`: **511 DWORDs**, donc les indices directs `0..510`. Le snapshot
  suivant commence à `[RBP+0x860]`; sa fenêtre unique `0x37F09B` passe une
  capacité `0x1FF` au helper `0x2F64E0`. Le corps unique `0x2F6527` prouve
  `min(listCount,capacity)` puis la copie de records de huit octets comme deux
  DWORDs. Le DWORD `[RBP+0x85C]` entre les deux régions n'est pas initialisé.
- La fenêtre unique `0x37F0D0` charge un record à
  `[RBP+index*8+0x860]`, effectue `sar eax,16; movzx edi,ax`, puis vérifie
  directement `EDI < DataTables.ItemStatCostRowCount`. L'index de la table de
  suppression est donc le stat ID uint16 lui-même, sans translation ordinale.
  Dès que la sentinelle ISC12 devient 4095, l'ID valide 511 lit le DWORD hors
  table à `+0x85C`; l'ID 512 alias le premier DWORD du snapshot, et les IDs
  suivants avancent dans la frame. Le plan G1 historique limité aux
  width/sentinel ne peut donc pas être publié tel quel.
- Un audit exhaustif des opérandes mémoire basés sur `RBP` dans le corps
  `0x37D140` trouve exactement dix formations/accès à la région
  `[RBP+0x60,RBP+0x860)`: le pointeur du `memset`, une lecture indexée dynamique
  à `0x37F17C` et huit writes fixes de partenaires composés. Les couples natifs
  sont `17→18`, `48→49`, `50→51`, `52→53`, `54→55,56` et `57→58,59`; leurs
  fenêtres exactes uniques sont promues dans `isc12/native-sites.json`. Chaque
  primaire écrit un token ID et plusieurs valeurs, puis la comparaison supprime
  le ou les records partenaires. Remplacer globalement la comparaison par
  `test esi,esi` casserait donc les composés vanilla.
- Le site gouverné complet est la fenêtre exacte unique de **50 octets**
  `[0x37F174,0x37F1A6)`. Ses 42 premiers octets finissent après
  `cmp eax,edx` et constituent un témoin prerequisite/body, pas à eux seuls le
  remplacement complet. Les mutations sont confinées à
  `[0x37F17C,0x37F1A1)`; le `test esi,esi` et sa branche à `0x37F174`, ainsi que
  le CALL natif du bit writer à `0x37F1A1`, restent inchangés. Le relay borné
  doit conserver la comparaison originale pour `statId<511` et rejoindre le
  writer directement pour `511..4094`, puisque la valeur a déjà été prouvée
  non nulle et tous les partenaires composés hard-codés sont sous 511. Sa
  publication doit fingerprint simultanément owner/frame, caller, table,
  snapshot/helper, extraction directe et les huit writes composés.
- Dans le même corps complet, la séquence commençant à `0x37D60B` énumère tous
  les enfants immédiats de l'inventaire de l'item, encode
  `min(occupiedChildren,7)` sur exactement trois bits à
  `0x37D66F..0x37D678`, et écrit donc 7 pour toute occupation supérieure. Ce
  champ demeure présent lorsque les producers `0x9C/0x9D` désactivent la
  récursion inline. `STAT_ITEM_NUMSOCKETS` (`0xC2`) est écrit séparément plus
  loin avec les `SaveBits` de sa ligne ItemStatCost; le natif n'impose pas leur
  égalité. Le preflight doit exiger `occupied<=7` et `occupied<=capacity`.
- La boucle des stat lists admet au maximum la base, cinq slots set et une
  runeword, soit sept listes/sentinelles structurelles. Chaque liste copie au
  plus 511 records; au plus 511 tokens ID peuvent survivre par liste, mais les
  `SaveBits`, valeurs nulles, composés et le skip numérique 326 réduisent le
  nombre réel. Le plafond structurel est donc 3577 tokens ID et sept
  sentinelles, sans preuve qu'un item réel puisse atteindre ce maximum. G9 doit
  valider les octets effectivement capturés plutôt que dériver la taille depuis
  le nombre brut de records.
- Le walker `0x481B50` énumère chaque enfant, appelle le producer 9D à
  `0x481BFE`, puis le chemin 9D rappelle le walker à `0x47A014`. Aucun compteur
  natif de profondeur, de nœuds totaux ou de cycle n'a été trouvé. La borne
  trois bits est locale à un nœud et ne borne pas l'arbre; ISC12 doit définir
  ses propres limites de profondeur/nœuds et refuser les cycles. Toutes ces
  conclusions sont des preuves statiques; cold start, round-trip, paquets et
  sauvegardes réelles restent **NOT RUN**.

## 2026-08-31 — ISC12 G0-BBE width census and publication-authority audit

- Le corpus commun gouverné est vérifié avant l'audit : image canonique
  `CC59119D…A914715`, image d'analyse `673E8C0B…0E63AB`, index 105 850
  fonctions / 1 057 329 références et self-test PASS. Les preuves `.text`
  ci-dessous sont byte-identiques pour les builds couverts 92777/93847; aucun
  RVA ou ABI D2MOO n'est transposé.
- Le census exhaustif du compilateur générique `0xA24290` trouve sept CALLs
  natifs : `0x3B46F9`, `0x3B483D`, `0x3B4A4D`, `0x3B4CCD`, `0x3B4EFD`,
  `0x3B4FBD` et `0x3B533D`. Tous utilisent le mapper universel `0x3B58A0`.
  Trois frontends seulement — `0x3B54E0`, `0x3B55A0`, `0x3B6220` — appellent
  le même resolver core `0x3B5AA0`.
- Le mapping de domaines natif distingue les pools fixes Skills (`+0x138`),
  SkillDesc (`+0x150`), Missiles (`+0x188`), Items combinés (`+0x228`),
  Properties val1..val7 (`+0x12C0`) et le pool condition-calc partagé
  Items/SetItems/UniqueItems/TCEx (`+0xD90`), plus un bridge générique
  dynamique. Le témoin `0x3B5307/0x3B533D` appartient à ce dernier pool
  condition-calc; il ne doit donc jamais être étiqueté comme un scope Skill.
- Le selector exact unique `0x3B5B58` accepte les slots 3..8 et consulte les
  six DWORDs exacts à `0x3B61B0`. Le slot numérique 5 rejoint ainsi
  `0x3B5D80`, seule branche du core qui charge
  `DataTables.ItemStatCostLinker` à `+0x1270`. Cette branche appelle le lookup
  `0xA121F0`, puis écrit l'ordinal retourné par EAX comme DWORD dans `[RBX]`.
  Aucun masque `0x1FF`, shift neuf bits ou store étroit n'existe sur ce trajet.
- Le compilateur générique teste d'abord la plage signée -128..127 à
  `0xA245FC`; au-delà, `0xA24661` teste -32768..32767 et émet le token 5 suivi
  de `word [RSI+1]=BX`. Chaque ID ISC12 valide `512..4094` est donc encodé
  exactement sur 16 bits. Le decoder exact unique `0xA235D5` borne deux octets,
  fait `movsx EDX,word [R12]`, appelle le callback et avance de deux octets.
  Comme `4094 < 0x8000`, le signe ne change aucune valeur ISC12 valide.
- Le census du generic evaluator `0xA234B0` trouve huit CALLs natifs. Sept
  contextes D2R câblés à une table —
  `0x3B460B`, `0x3B49AC`, `0x3B4C1F`, `0x3B4E51`, `0x3B5136`, `0x3B5296`
  et `0x3B54BB` — passent la table `0x1D09C50` et le count 9; le huitième
  appel générique `0xA24A1A` passe une table et un count nuls. Le callback
  `0x3B33F0..0x3B34F9` copie l'ID EDX vers EDI, vérifie le row count
  `DataTables+0x1260`, puis retransmet le DWORD complet à
  `STATLIST_UnitGetStatValue`, `STATLIST_GetUnitBaseStat` ou
  `STATLIST_GetUnitStat`, sans troncature.
- Une attestation gouvernée ferme les deux arêtes protégées le 31 août 2026 à
  `2026-08-31T12:44:45.5847343Z`. Elle lit le processus D2RLoader PID `39116`,
  créé le 30 août à `17:12:21-04:00`, déjà ouvert par
  `D2RLoader.exe -txt -mod BKVince -txt -offline` sur le runtime officiel D2R
  `3.3.93847`, base `0x140000000`, avec seulement
  `PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ` (`0x1010`). Les hashes
  sur disque sont `.build.info`
  `2EBCAD0521DBF038D5A7FE5395E96B4BEF6D4F0774F7B1F840E03C3DE9CB067A`,
  `D2R.exe`
  `E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`,
  `D2RLoader.exe`
  `A926DAA85DE85EADCF98FDB5FB30143CC32D6B4913EB21A6416EBDAA78945128`
  et `D2RCore.dll`
  `013B047612BFF0EB564891037508FD43D03AFDFB20BFEB9B5BC683B36559FFC6`.
  Aucune permission de mutation n'est demandée et aucune écriture n'est
  tentée.
- Avant de croire les RDATA, la capture confirme byte-exact les deux ancres
  `.text`: `0x3B59CA = 488D15BB4C9401` et
  `0x3B54A3 = 488D05A6479501`. Les 16 octets protégés à `0x1CFA68C` valent
  `73746174000000000000000073657466`, soit la chaîne NUL-terminée `stat`; leur
  SHA-256 est
  `83626D991B1A0AD789DDB50D993623989AB1C1257879530005CEE0CA411F0E4F`.
- La capture corrige le modèle statique : `0x1D09C50` n'est pas une suite de
  neuf QWORDs, mais neuf records de 16 octets
  `{callback VA:qword, arity:dword, padding:dword}`. Les callbacks/arity sont
  `3B3200/2`, `3B3210/2`, `3B3220/2`, `3B32F0/2`, `3B33A0/2`,
  `3B33F0/2`, `3B3500/3`, `3B3590/2`, `3B3640/FFFFFFFF`; tous les paddings
  sont zéro. Le record 5 à `0x1D09CA0` pointe donc exactement vers le callback
  full-width `0x3B33F0`. Les 144 octets bruts capturés sont
  `00323B4001000000020000000000000010323B4001000000020000000000000020323B40010000000200000000000000F0323B40010000000200000000000000A0333B40010000000200000000000000F0333B4001000000020000000000000000353B4001000000030000000000000090353B4001000000020000000000000040363B4001000000FFFFFFFF00000000`,
  SHA-256 session/base `B551BA77BCF256B537D3CDB750C0B0246C0C05DA45675AF74EBEFB98522A1D1A`.
  Après remplacement des neuf VA par leurs RVA, le SHA-256 portable est
  `E8B9B76F9D7320BDFA8129F8D22B0CB19B450AC4D655F53A8D2E56E47CF224ED`.
  D2MOO demeure seulement une corroboration sémantique; la fermeture repose
  maintenant sur les données natives D2R 3.3.
- Le ledger promeut dix surfaces G0-BBE proof-only, toutes avec
  `targetValue = unchanged`; son validateur interdit désormais toute mutation
  dans ce groupe. Les signatures `.text` sont exactes et uniques et les deux
  memberships protégés passent de `identified` à `ready`. G0-BBE est fermé
  pour la largeur et l'appartenance natives. Ce lot ne crée aucun patch BBE,
  ne publie aucun octet et n'exécute aucun cold start ISC12; seuls des reads
  externes ont attesté l'instance déjà ouverte.
- L'audit séparé du PluginSDK v3 épinglé
  `4933e2c42cb2592958cd0df3b6dc5003102252d1` confirme que les services publics
  s'arrêtent à l'ID 15 et n'exposent ni quiescence, ni transaction de patch, ni
  exclusion de tous les consumers D2R. `D2RLoaderLoadPlugin` est une fenêtre
  d'installation plausible, pas une garantie documentée. Émettre le lease
  ISC12 depuis ce callback serait donc une hypothèse et demeure interdit.
- Le minimum honnête est une transaction synchrone loader-owned, additive au
  SDK, qui exécute un callback non évadable pendant une phase startup réellement
  quiescente, sérialise les publishers, lie le token au thread/owner/epoch et
  empêche toute reprise sur résultat `Poisoned`. ISC12 devra coordonner sous
  cette seule transaction le preflight global G0 + G10 + codec, la réservation
  process-lifetime, les commits dans cet ordre — codec conservant
  G9/G2/G4/G1/G3 — puis publier readiness et `operational` en dernier. Un mutex
  d'installateurs, une suspension de
  threads implantée par le plugin ou un token sans garantie loader ne satisfait
  pas ce contrat.
- Le refus actuel `enabled=true` avant mutex, préparation et écriture reste
  donc le seul comportement production démontré. Même avec un futur issuer,
  les publishers locaux actuels ne doivent pas être appelés séquentiellement :
  `InstallLoaderExtension` ne publie que G0, le commit codec n'a aucun caller et
  G10 n'est pas installé. Le runtime de publication reste bloqué jusqu'à
  l'autorité loader, aux adapters full-set et à l'attestation live unwind;
  G0-BBE n'est plus un blocker de membership.
- Le coordinateur full-set production-neutral est maintenant implanté et
  compilé sans caller production. Sa machine one-shot preflight G0, G10 et
  codec avant toute réservation/écriture, réserve la durée de vie processus une
  fois, commit les trois domaines dans cet ordre, garde l'ordre interne codec
  G9/G2/G4/G1/G3 et publie la readiness seulement à la fin. Un résultat
  incertain ou post-write appelle exactement une fois l'état poison, devient
  terminal et n'essaie aucun rollback. Les tests exécutables couvrent lease
  absente/révoquée, chacun des trois rejets de preflight, réentrance,
  mutate-then-uncertain, monotonie terminale et move/release unique.
- Le build gouverné Release `/W4 /WX` passe CTest `4/4`; la DLL a le SHA-256
  `FF8D16AF4A6DBCB9BD3AD86A6A6DFCBB4553D26A200DF161B1065C6A5DFE5286`.
  Le coordinateur n'est encore qu'un contrat d'orchestration : G0, G10 et
  codec doivent être séparés en adapters preflight/commit immuables, puis liés
  sans caller production. La proposition SDK gouvernée remplace le faux modèle
  de rollback atomique par un service `NativePublication` optionnel,
  synchrone, loader-owned, owner/thread/epoch-bound et no-resume sur
  `Poisoned`. Le véritable issuer et sa barrière consommateurs restent
  impossibles à implanter sans source D2RLoader/Core upstream.

## 2026-08-31 — ISC12 et les providers D2RCore 1.1/1.2

- Le ZIP officiel `D2RLoader-1.2.0-beta.zip` vaut
  `2AABEF2E6838CA3611EA3CB74D318C3BB792549CC4FC6C7D53933245667417D9`.
  Le runtime installé est byte-identique au ZIP pour `D2RLoader.exe`
  (`651FA9EB33083088349224B1624819F63ED79596F808950CF6468B5D82F7132E`)
  et `D2RCore.dll`
  (`876957AE7AEF627BAC3E56592CA15888A8AAF9B952ED79EDCC1CF6351B3F93CF`).
- D2RLoader compose trois surfaces déjà gouvernées sans en changer l'ABI.
  `D2R+0x31EC89` rejoint `D2RCore!LoadExcelTable`; G1 à
  `D2R+0x37F1A1` rejoint `D2RCore!WriteItemSaveStatId`; G3 à
  `D2R+0x535303` rejoint `D2RCore!WritePlayerSaveStatId`. Les relais proches
  sont des `FF 25` vers des slots vivants et chaque provider retransmet les
  arguments au propriétaire natif exact. ISC12 conserve ces CALLs et ne mute
  que ses propres largeurs/corps gouvernés.
- Les deux providers de stat ID 1.2 ont des corps exacts
  `[0x6364C0,0x63652B)` et `[0x636550,0x6365BC)`, des tuples PDATA/unwind
  exacts et retransmettent `RCX/EDX/R8` une fois à
  `D2R+0xA1B710`. Leur bitmap TLS privé ne suit que `EDX < 0x200`; un ID
  supérieur n'est ni tronqué ni rejeté, il manque seulement au census de
  compatibilité D2RLoader. L'enveloppe et le hash de schéma ISC12 restent
  l'autorité de persistance locale; la couverture metadata/handshake des IDs
  élevés demeure un gate réseau distinct et ne justifie pas d'étendre ce
  bitmap privé.
- Le second caller player-save forme un autre contrat indivisible. Vanilla
  charge `R13D=0x8000` à `0x41E138`, alloue/zéroise ce nombre d'octets et
  appelle directement `D2R+0x52F090` à `0x41E207`. D2RLoader 1.2 change
  seulement la capacité en `0xFFFF`, puis redirige le même CALL via
  `D2R+0x3E2A4AC` et son slot `0x3E29D00` vers
  `D2RCore!WritePlayerSaveWithEnvironmentCapture` à `0x634650`. Le provider
  1.2 possède PDATA `[0x634650,0x636068)`, unwind `0x50EFD0`, corps SHA-256
  `A4A0E2A5E70AEFB613016739E914225CEE2A20BB06F13197CEAC182E11648667`
  et slot `D2RCore+0x5372C0` résolu exactement vers `D2R+0x52F090`. Le
  provider 1.1 équivalent possède PDATA `[0x563D80,0x565103)`, unwind
  `0x452480`, corps SHA-256
  `66C61BC1678375C9E373FD2141F409244B450D1411906B7FF8C27C1241E69F6A`
  et slot `D2RCore+0x480DE8`. Chaque corps réémet `RCX/RDX/R8/R9` et les deux
  arguments stack avant un unique appel natif.
- L'admission ISC12 reste fail-closed : seul le couple exact
  `0x8000 + direct 0x52F090` ou le couple exact
  `0xFFFF + provider D2RCore attesté` est accepté. Le second exige l'export
  RVA exact, le hash du corps complet, PDATA, les 36 octets unwind, une chaîne
  bornée de sauts inconditionnels, le slot vivant et l'entrée exacte du
  serializer natif. Aucun masque générique ni réécriture du provider n'a été
  ajouté.
- Un cold start full-stack du candidat précédent de 422 912 octets
  (`F87546F8D2A940D419464EDC444E08DD1AF1E45D1721C8FBBF2825214E09A0F5`)
  a vérifié `LoadExcelTable` puis `WritePlayerSaveStatId`, avant de refuser
  proprement le couple dynamique encore inconnu à `0x41E138`, sans mutation
  ni readiness ISC12. Après l'attestation ci-dessus, deux builds Release
  reproductibles `/W4 /WX` passent CTest `5/5`; le nouveau candidat de
  425 472 octets vaut
  `4B928E117B930BCA2B9B0F8E3D2CDE8FF10F1C1F4FE2923ECCA5B1BCF6BD69AD`.
  Son prochain cold start attend seulement que la session BKVince humaine
  active libère le runtime; aucune sauvegarde ni action gameplay ISC12 n'a
  encore été exécutée.

## ISC12 — disposition runtime du conteneur persistant

- Le premier write réel du candidat à enveloppe a produit exactement 499
  octets : 96 octets d'enveloppe ISC12 suivis d'un D2S valide de 403 octets.
  Le writer et la primitive atomique ont donc rempli leur contrat local.
- D2RLoader 1.2 a néanmoins annulé le lancement au frontend avec
  `route=prepare result=invalid-character`. Une enveloppe externe opaque est
  incompatible avec ce trajet même lorsque son payload interne est valide.
- La DLL runtime conserve désormais les conteneurs D2S/D2I standards. Le hook
  reader valide le buffer inchangé; le hook writer valide puis retourne à la
  continuation native avant `CREATE_ALWAYS`, afin que le couple D2RCore
  writer/closer compose normalement `.d2rl`, backups et environnement.
- Vincent accepte la persistance native non atomique et fixe le clean-sheet
  comme contrat de support : nouveau personnage ou migration externe future,
  backups obligatoires, aucun chargement direct d'une save vanilla/non-ISC12.
  L'enveloppe et l'atomic writer restent des composants unit-testés réservés à
  l'outillage de migration.

## 2026-09-01 — ISC12 G9 : invariant du walker et qualification runtime finale

- Le désassemblage gouverné du walker socketé `0x481B50` ferme l'invariant
  exact des descendants : le producer enfant 0x9D reçoit les flags du parent
  avec le bit `0x08` forcé, soit
  `childTemporaryFlags = parentTemporaryFlags | 0x08`; l'action vaut `0x12`,
  le parent est l'item actif et gamble vaut zéro. La validation précédente par
  simple égalité de flags était donc trop stricte et a été corrigée.
- Le candidat final ajoute un self-test runtime du transactionnel G9. Il
  atteste un arbre accepté 3/3, un dépassement de 64 nœuds rejeté avant tout
  callback de queue, et une réentrance pendant flush contenue après exactement
  un callback avec passage terminal en `Fatal`. Le sink `0x4817F0` retourne
  `void`; aucun rollback n'est revendiqué après le début d'un flush réel.
- Des transactions gameplay réelles ferment les deux racines 0x9C et 0x9D.
  Une Gothic Plate socketée avec deux runes produit un arbre de trois nœuds :
  `nodes=3`, `captured=3`, `queued=3`, `staging-error=0`,
  `flush-error=0`. Les mêmes preuves repassent en portée globale.
- Le candidat final mesure 446 464 octets et vaut
  `1311F1C4BE44B0918F34C32007C3A19D35D240D8B72DCAD8C1853EEE53EC11B5`.
  Deux builds Release reproductibles, la DLL déployée et CTest `5/5`
  concordent. Les cold starts mod-local, global puis mod-local conservent 36
  plugins chargés, les cinq eezstreet et 17 patches.
- Les fixtures D2S sérialisent les IDs 512/4094 avec les valeurs 12/94 et les
  préservent sur deux cycles froids. Le D2I contrôlé de 68 307 octets conserve
  le SHA-256 `7375F2F7…E96507F` après extinction complète, réaffiche l'armure et
  rejoue son arbre 3/3. Ces preuves ferment le cœur D2S/D2I/G9; le multijoueur,
  G5–G8 et le census fixed-byte restent hors de cette promotion.

## 2026-09-01 — Scripted Domains RE-AI-1 : dispatch normal et impossibilité de l’append in-place

- `npm run re:d2r33 -- status` vérifie l’image canonique, l’image d’analyse,
  l’index et le projet Ghidra du corpus commun 92777/93847. Le runtime final à
  tester demeure D2R 3.3.93847; aucune matrice runtime n’est revendiquée ici.
- `AITHINK_GetAiTableRecord 0x4A36C0` possède trois callers directs :
  `0x4A276A`, `0x4A27B7` et `0x4A2BD1`. Son entrée stricte de 17 octets est
  unique. Les deux premiers callers gouvernent l’initialisation et les
  transitions du callback; le troisième sélectionne le profil de ciblage du
  think courant.
- Le chemin normal lit le WORD signé `MonStatsTxt+0x52`, puis la séquence
  unique `B9 9B 00 00 00 66 3B C1 73 19` à `0x4A3791` exige
  `0 <= AI < 155`. L’index accepté est multiplié par `0x20` et ajouté à la base
  `0x2396E90`, référencée uniquement à `0x4A379F` et par le fallback à
  `0x4A37B9`.
- Le special state non nul admissible utilise la même stride `0x20` et la base
  `0x23981F0`, référencée à `0x4A376E`. L’égalité
  `0x2396E90 + 155*0x20 == 0x23981F0` prouve que la table normale et la table
  des special states sont contiguës. Sur D2R, élargir seulement la borne à 156
  ferait donc lire la première special state comme nouvelle AI; écrire cette
  entrée la corromprait. Le patch Harvest 1.10f n’est pas transposable tel quel.
- Le dispatch x64 lit la catégorie à `record+0x00`. Le chemin
  d’initialisation compare le callback principal à `record+0x10`, teste le
  callback d’entrée à `record+0x08` et le callback de transition à
  `record+0x18`. La taille x64 `0x20` est ainsi prouvée directement; la
  structure legacy D2MOO n’est utilisée que comme sémantique.
- Le callback principal reçoit `Game*`, `Unit*` et `D2AiTickParam*` selon l’ABI
  Windows x64. Le stockage local construit par le caller prouve le sous-ensemble
  minimal suivant : `D2AiControl* +0x00`, cible `+0x10`, distance `+0x20`,
  témoin de combat `+0x24`, `MonStatsTxt* +0x28` et `MonStats2Txt* +0x30`.
  Cette preuve ferme le contexte minimal du bridge sans prétendre connaître la
  structure complète.
- `AITACTICS_IdleInNeutralMode 0x4A6D10` est le premier helper de fallback
  fermé. Il normalise zéro à un frame, place le monstre en mode neutre, appelle
  `0x48B890(game, unit, eventType=2, customId=0)`, puis
  `EVENT_SetEvent 0x48B720(game, unit, eventType=2,
  Game+0x170+frames, 0, 0, ...)`. Sa séquence interne unique à `0x4A6D71`
  commence par `45 8D 41 02 E8 16 4B FE FF 44 8B 8F 70 01 00 00`.
- La référence sémantique est
  `D2MOO@19019806df7f3e877fa105b05395d1e3597e2316`,
  `source/D2Game/src/AI/AiThink.cpp:17051-17408`,
  `source/D2Game/src/AI/AiTactics.cpp:243-266` et
  `source/D2Game/include/AI/AiGeneral.h:19-29,80-86`. Aucune adresse, structure
  ou ABI 32 bits n’est transposée.
- La direction D2R retenue pour instruction est un hook unique et fail-closed
  du résolveur : déléguer tout special state et toute unité non liée à
  l’original, puis retourner un record bridge possédé par la DLL uniquement
  pour une classe présente dans une table custom `aiscript`. Les inconnues
  restantes sont la catégorie bridge, les helpers attaque/chase/retraite/
  errance/cast, leur contrat de rescheduling, le cycle de vie GUID et
  l’ownership runtime. Aucune DLL ni injection n’est encore créée.

## 2026-09-01 — Scripted Domains RE-AI-2 : catégorie bridge, actions et continuation

- Le switch à `0x4A2BD6` est borné à `0..6`; son témoin strict de 28 octets est
  unique. La table de saut donne exactement `0→0x4A2CF1`, `1→0x4A2BF2`,
  `2→0x4A2C7A`, `3→0x4A2BF2`, `4→0x4A2C31`, `5→0x4A2CA0` et
  `6→0x4A2CC9`.
- Les catégories `1/3` appellent le wrapper `0x4A69A0` et ne dispatchent le
  callback qu’avec une cible. La catégorie `2` appelle directement le
  sélecteur nullable `0x595750`, puis rejoint le callback sans test de nullité.
  La catégorie `4` saute le callback principal. Les catégories `5/6` appellent
  `0x4A6760`; `5` exige ensuite une cible tandis que `6` continue même après le
  fallback exécuté par ce wrapper. Le premier record bridge retient donc la
  **catégorie 2** : ciblage natif, callback garanti et aucune action automatique
  avant Lua.
- `AIUTIL_SelectTargetForAiThink 0x595750` suit l’ABI x64
  `Unit*(Game*, Unit*, D2AiControl*, int32_t* distance, int32_t* combat,
  uint8_t context)`. Son corps se termine à `0x595F7F`; le census de ses appels
  directs ne contient ni `EVENT_SetEvent 0x48B720` ni `0x48B890`. Sa signature
  stricte de 32 octets est unique.
- Les primitives terminales V1 sont maintenant fermées dans l’image :
  `AITACTICS_ChangeModeAndTargetUnit 0x4A78E0` pour l’attaque,
  `AITACTICS_UseSkill 0x4A7BC0` pour le cast,
  `AITACTICS_WalkToTargetUnitWithFlags 0x4A8740` pour le chase,
  `D2GAME_AICORE_Escape 0x4A7DF0` pour la retraite et
  `AITACTICS_WalkCloseToUnit 0x4A8320` pour l’errance. Leurs signatures
  strictes étendues sont uniques. Les deux feuilles de marche convergent vers
  `AITACTICS_MoveToTarget 0x4A8A10`, lui aussi identifié par une entrée unique.
- Les ABI utiles sont respectivement `(game, unit, mode, target)`,
  `(game, unit, mode, skillId, target, x, y, flag)`,
  `(game, unit, target, flags16)`,
  `(game, unit, target, distance8, deleteAiEvent32)` et
  `(game, unit, radius8)`. Aucune structure legacy n’est utilisée pour fixer
  ces largeurs ou l’ordre x64.
- `UseSkill` appelle lui-même `AITACTICS_IdleInNeutralMode` pour 10 frames si
  le pipeline de mode refuse le cast. Les quatre autres primitives remontent
  leur rejet sans idle. Le contrat V1 est donc asymétrique mais fermé : une
  action acceptée laisse exclusivement le pipeline de modes natif poursuivre;
  un rejet attaque/chase/retraite/errance, une absence de leaf ou une erreur
  Lua appelle une fois l’idle natif; un rejet de cast n’ajoute aucun second
  idle. On ne sonde ni ne force un `AITHINK` immédiatement après un succès,
  parce qu’un mode/une animation accepté possède alors légitimement la suite.
- Le `monai.txt` officiel 3.3 et sa copie `base/monai.txt` sont byte-identiques,
  contiennent les 155 records attendus et valent
  `7c7c7b8866c46078356bcc6118789699351004f39edea92922425699d6aa86a8`.
  Le round-trip TSV est byte-exact. La table native vit toutefois dans une zone
  virtuelle non adossée au PE brut; aucune extraction live n’est nécessaire
  pour fermer la catégorie bridge par le dispatch lui-même.
- Référence sémantique seulement :
  `D2MOO@19019806df7f3e877fa105b05395d1e3597e2316`, notamment
  `AiThink.cpp:17363-17407`, `AiTactics.cpp:31-88,172-178,214-265,305-369,
  454-565` et `AiUtil.cpp:671-864`. Le prochain gate n’est plus le choix des
  actions : il porte sur le lifecycle GUID, l’ownership du hook, les budgets et
  l’empreinte fail-closed complète. Aucune DLL ni injection n’est créée par ce
  lot statique.

## 2026-09-01 — Extended Act Level IDs : résolveur central et layout `Levels.Act`

- `DRLG_ResolveActFromLevelId 0x326710` suit l'ABI x64 observée
  `uint8 (uint8 dataContext, int32 levelId)`. Son corps utile autonome va de
  `0x326710` à `0x3267A6`, même si l'entrée pdata englobante commence plus tôt.
  Il obtient le vecteur `ActInfo` par `0x3006E0`, sélectionne les DataTables du
  contexte par `GetDataTablesForContext 0x300A90`, lit le compte à `+0x108`,
  parcourt à rebours des records de `0x8C` octets et compare le Level ID au
  `rangeStart` signé à `record+0x04`. Il renvoie l'index d'acte zéro-based ou
  zéro si aucune plage ne correspond.
- Le census du workbench commun trouve **113 appels directs**. La signature
  stricte de 48 octets
  `48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 8B F2 0F B6 D9 E8 B7 9F FD FF 0F B6 CB 48 8B F8 E8 5C A3 FD FF 8B 98 08 01 00 00 83 EB 01 78 43 90`
  n'a qu'un match, à `0x326710`, dans l'image commune 92777/93847.
- Une sonde D2RLoader temporaire, exécutée le 1er septembre 2026 sur le runtime
  officiel D2R `3.3.93847` avec toute la pile BKVince active, a interrogé
  `DataTableServiceV1` pendant `DataTablesLoaded`. Les banques Classic et LoD
  exposent 137 lignes, RotW 147; les trois ont `rowSize=0x18C`. Sur tous les
  Level IDs présents et les frontières `1/40/75/103/109`, l'unique octet qui
  reproduit `Levels.txt → Act` est `row+0x0D`. Une entrée en partie BKVince a
  ensuite observé `dataContext=3`, égal à la valeur ABI `Bank::Rotw=3`.
  Preuve locale :
  `analysis-cache/extended-act-level-ids-probe/evidence/20260901-1918/ruffneckk-extended-act-level-ids-probe.log`
  (SHA-256 `F81B7F76F28D36CC173D7D7A8CB83888D9D026BAF0C5C5B801B40B8B5FC6BA6D`).
- D2MOO 1.10f explique pourquoi cette surface existe :
  `DRLG_GetActNoFromLevelId` applique les seuils fixes
  `{1,40,75,103,109,1024}` et porte déjà le commentaire
  `Lookup the act from Levels.txt`. Cette référence est strictement sémantique;
  aucune adresse, structure ou ABI 32 bits n'est transférée.
- Le contrat produit peut donc rester minimal : un hook propriétaire du seul
  résolveur central, des caches immuables copiés après chaque révision de tables
  et une sélection `dataContext 1..3 → Bank 1..3`. Avant de publier un cache,
  la DLL doit vérifier le service API v4, `rowSize=0x18C`, les actes ancres et
  chaque valeur `0..4`; `dataContext=0`, cache non prêt, ID absent ou valeur
  invalide reprennent toujours l'original. Le nom du build reste diagnostique
  et n'entre dans aucune allowlist.
- Le produit final `0.1.0` ferme aussi le test hors plage du résolveur. Un
  fixture TSV CRLF temporaire `Id=147 / Act=0`, construit sans modifier la
  source BKVince, fait passer la banque RotW de 147 à 148 lignes. L'appel de
  l'entrée native hookée journalise alors `Act index 0 (Act 1), data context 3,
  source=Levels.txt`. Le fixture est ensuite retiré et le `levels.txt` runtime
  retrouve byte-exact le SHA-256
  `A46B5438164ADB1FB9540890103594EA48A79AFA2478CB6865D2E6DB5795EB04`.
  Preuve locale :
  `analysis-cache/extended-act-level-ids-product/evidence/20260901-1947-functional-fixture/ruffneckk-extended-act-level-ids.fixture.log`.
