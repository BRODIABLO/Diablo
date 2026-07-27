# RemoteStash — D2R 3.2.92777

Dernière mise à jour : 27 juillet 2026

Statut : reprise statique en parallèle le 27 juillet 2026 pour prouver le placement
dynamique du futur contrôle sur des inventaires de tailles différentes, sans
remplacer Vendor Stock Refresh comme mission prioritaire du workspace. Le chemin
serveur natif d’ouverture du stash est identifié statiquement, mais sa confirmation
dynamique reste suspendue par la validation officielle hors ligne de Battle.net.
Aucun prototype public, aucun bouton, aucun sprite et aucune archive n’existent
encore.

## Décisions confirmées

- Après une pause temporaire au profit de Repair Costs Cap, Vincent a repris
  RemoteStash selon l’Option A le 24 juillet 2026. Configurable Larzuk Sockets
  reste intacte à son gate de validation en jeu.
- Depuis le 26 juillet 2026, Vincent autorise la préparation statique et la
  compilation des probes RemoteStash en parallèle de la mission active, sans
  promouvoir ce chantier; aucun déploiement ni test runtime bloqué n’est forcé.
- La catégorie PluginPack future est `misc`, avec `plugin-misc.dll` comme DLL
  propriétaire et `misc.remoteStash` comme clé prévue dans l’unique
  `D2RPlugins.json`.
- Pendant l’incubation, la fonctionnalité restera dans une DLL autonome hybride
  `RemoteStash.dll`, attribuée exactement à `RuffnecKk`, sans modifier, lier ni
  redistribuer une DLL d’eezstreet.
- La première phase porte uniquement sur le chemin natif d’ouverture du stash
  et la possibilité de le déclencher depuis un autre contrôle UI. Les sprites,
  le placement final et l’adaptation aux layouts personnalisés viendront après
  la preuve fonctionnelle.
- Le 27 juillet 2026, Vincent a demandé de commencer en parallèle la preuve du
  placement adaptatif. Le futur bouton ne doit utiliser aucune coordonnée propre
  à BKVince : il doit lire le layout runtime actif, se placer relativement à
  des enfants nommés du `PlayerInventoryPanel`, vérifier les collisions et se
  masquer si aucune géométrie sûre n’est disponible.

## Résultat joueur attendu

Un contrôle placé dans l’écran d’inventaire permet d’ouvrir le stash du joueur
sans interaction directe avec le coffre du monde. Le plugin doit réutiliser le
comportement autoritaire du jeu plutôt que recréer une grille ou déplacer des
objets lui-même.

## Incubation compatible PluginPack

- Description anglaise prévue : `Opens the player stash from the inventory screen.`
- Utiliser un JSON autonome compatible PluginPack seulement lorsqu’une option
  réelle est démontrée; aucun TOML ne sera créé.
- Rechercher la configuration d’abord dans le mod actif puis dans le dossier
  global du jeu; une configuration présente mais invalide devra être refusée.
- Après un merge futur, intégrer la fonctionnalité à `plugin-misc.dll`, déplacer
  ses options sous `misc.remoteStash`, puis supprimer la DLL et le JSON autonomes.

## Audit initial

- Le gate `npm.cmd run re:d2r32 -- status` est vert : images canonique et
  d’analyse, index SQLite et projet Ghidra persistant du build 92777 sont
  vérifiés; aucun redump ni réimport n’a été effectué.
- La référence officielle `D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`
  est propre et vérifiée.
- La recherche `stash` dans cette référence ne retourne aucun résultat. La
  recherche `inventory` ne retourne que la configuration de plafond d’or de
  `plugin-items` (`D2RPlugins.json:11-14`, `src/plugin-items/items-main.cpp:492`).
- `plugin-misc` charge la section `misc` de l’unique JSON
  (`src/plugin-misc/misc-main.cpp:136-140`) et ne possède actuellement que les
  deux callsites `/players` `0x18885B`/`0x18887F` et le hook
  `GAME_GetPlayerCountBonus` `0x542F40` (`misc-main.cpp:5-33,142-158`). Aucun
  chevauchement RemoteStash n’est donc observé à ce stade.
- Les preuves gouvernées existantes connaissent la résolution de grille stash
  par `UNITS_GetInventoryGrid` `0x34A410`, mais aucune fonction d’ouverture du
  panneau stash n’est encore identifiée. Cette fonction ne doit pas être
  confondue avec le chemin UI recherché.
- La référence sémantique D2MOO épinglée
  `@19019806df7f3e877fa105b05395d1e3597e2316` définit `UI_STASH = 0x19`,
  `UPDATEUI_OPENSTASH = 16` et `UPDATEUI_CLOSESTASH = 17`
  (`source/D2CommonDefinitions/include/D2Constants.h:214,501-502`). Son chemin
  historique d’interaction avec l’objet stash envoie au client le paquet UI
  serveur `0x77` avec l’action `UPDATEUI_OPENSTASH`
  (`source/D2Game/src/PLAYER/PlrTrade.cpp:53-64`), au moyen de
  `D2GAME_PACKETS_SendPacket0x77_Ui_6FC3E0B0`
  (`source/D2Game/src/GAME/SCmd.cpp:1159`). Cette preuve est uniquement
  sémantique 1.10f : aucune adresse ni ABI n’est transposée vers D2R 3.2.
- Sur 92777, `D2GAME_QueueServerPacket` `0x4817F0` conserve son ABI prouvée
  `(client, packetBytes, packetLength)`. Une recherche par construction du
  paquet, et non par littéral contigu, identifie maintenant
  `D2GAME_PACKETS_SendPacket0x77_Ui` `0x480650`. Cette fonction place `0x77`
  dans le premier octet, l’action reçue dans le second, puis transmet le paquet
  au client. Sa signature d’entrée stricte de 16 octets est unique.
- Le caller `0x528270` charge le joueur depuis la structure d’opération, exige
  que l’objet stash soit encore en mode neutre, résout le client du joueur par
  `SUNIT_GetClientFromPlayer` `0x48FDE0`, passe l’action `0x10` à `0x480650`
  et retourne le succès. Cette sémantique identifie avec confiance élevée
  `OBJECTS_OperateFunction32_Bank`, avec l’ABI probable
  `(D2ObjOperateFnStrc* operation, int32 operate) -> int32`; sa signature
  d’entrée stricte de 18 octets est unique.
- Dans cette structure moderne, le handler lit l’objet à l’offset `+0x08` et le
  joueur à `+0x10`. Cela concorde avec l’élargissement x64 naturel de la
  structure D2MOO historique (`game`, `object`, `player`, `object region`,
  `object class id`), mais les champs suivants `+0x18` et `+0x20` restent une
  hypothèse tant que le dispatcher moderne n’est pas borné.
- Le handler moderne ne met lui-même à jour ni l’interaction du joueur, ni son
  état occupé, et ne vérifie pas la ville : il ne fait que tester le mode de
  l’objet puis envoyer `0x77 / 0x10`. Ces préconditions appartiennent donc à un
  chemin plus large ou à des callbacks serveur ultérieurs; appeler uniquement
  `0x480650` ne constitue pas encore une ouverture autoritaire complète.
- Un callback serveur de 17 octets commençant à `0x4BA580` parcourt les objets
  de la room du joueur, exige un objet de type `2`, de classe stash `267`, à une
  distance inférieure ou égale à `50`, puis manipule les statistiques `14`
  (or porté) et `15` (or en banque). Sa fonction exacte reste à nommer, mais sa
  sémantique de transaction d’or du stash est établie statiquement. Cette preuve
  exclut déjà l’hypothèse qu’un simple paquet visuel `OPENSTASH` suffirait à
  reproduire toutes les opérations du stash hors de portée du coffre.
- Le handler moderne d’insertion dans une grille est maintenant borné à
  `0x4BFF30–0x4C022F`. Il parse exactement les 17 octets sémantiques du paquet
  historique `InsertItemInBuffer` : GUID de l’item à `+1`, X à `+5`, Y à `+9`
  et page de stockage à `+13`. Il normalise les pages hors de l’intervalle
  `0..4`, construit l’état natif de placement, puis délègue la transaction à
  `0x471E90`. Sa signature stricte de 32 octets est unique dans le build 92777.
- Ni ce handler ni `0x471E90` n’appellent directement `UNITS_GetRoom`,
  `UNITS_GetClassId` ou un calcul de distance. Contrairement à la transaction
  d’or, aucune dépendance directe à un objet stash proche n’est donc observée
  dans le chemin d’insertion. La routine déléguée reconstruit toutefois un état
  complet d’inventaire et peut encore exiger un état de conteneur ou de joueur;
  son acceptation après une ouverture seulement UI reste à tester dynamiquement.
- Le paquet d’insertion ne contient aucune dimension de grille : la page et les
  coordonnées proviennent du panneau natif actif. Un stash étendu ou personnalisé
  n’exige donc pas que RemoteStash connaisse sa largeur, sa hauteur ou ses onglets
  pour émettre ce mouvement; le layout demeure la responsabilité du mod et du
  panneau natif. Cette conclusion ne couvre pas encore un mod qui remplace le
  protocole ou les pages de stockage plutôt que seulement leur présentation.
- Le callback réseau `0x4B2BE0` valide un paquet d’interaction de 9 octets
  contenant le type et le GUID d’une unité. Son ABI, sa signature unique de
  23 octets et sa proximité sémantique avec le handler historique
  `InteractWithEntity` sont établies. Un hook dynamique actif pendant une
  ouverture locale normale du stash n’a toutefois reçu aucun appel : ce
  callback appartient au chemin de réception réseau et ne doit pas être pris
  pour le déclencheur local intégré.
- Le dispatcher client d'interaction avec les objets appelle
  `UNITS_GetClassId` `0x349860`, reconnaît explicitement la classe stash `267`
  (`0x10B`) à `0x1E0F2B`, puis branche exclusivement vers `0x16EC10`.
- L'analyse complète de `0x16EC10` invalide toutefois son identification comme
  fonction d'ouverture. Elle exige l'unité stash dans deux champs, résout sa
  position et soumet les identifiants `0x11C`, `0x128` et `0x125` à une longue
  routine de présentation qui calcule des coordonnées et des effets. Cette voie
  est conservée comme gestionnaire probable d'animation/FX du coffre, confiance
  faible, et n'est ni promue ni appelée par le prototype.
- Une autre routine observée autour de `0x4BA600` parcourt les unités objet,
  reconnaît elle aussi la classe `267` et retient un stash à une distance
  inférieure ou égale à `50`. Ce fragment prouve qu'un résolveur natif existe,
  mais ses limites de fonction et son ABI ne sont pas encore assez sûres pour
  l'appeler depuis le plugin.
- Une ouverture native normale a été observée sous BKVince pendant le probe :
  le panneau affiché conserve ses onglets et ses grilles personnalisés
  `Personal`, `Shared`, `Crafting` et `BKDiablo`. Cela prouve que le panneau
  natif actif rend déjà le layout du mod; cela ne prouve pas encore qu’un appel
  distant à `0x528270` satisfera toutes les préconditions serveur.
- Un probe d’analyse gitignoré a été compilé pour confirmer dynamiquement
  l’entrée dans `0x528270`. Le jeu a ensuite refusé l’accès avec son contrôle
  officiel « pas en ligne depuis 30 jours » avant l’entrée en partie. Le probe
  a été retiré, la DLL d’origine restaurée et aucun processus Diablo n’a été
  laissé actif. Aucune tentative de contournement n’est autorisée ni requise.
- Le probe gitignoré `RemoteStashPathProbe.dll` compile également une commande
  console locale `remote-stash-open`. Elle vérifie la signature unique de
  `0x480650`, reçoit le `Client*` fourni par le SDK et envoie seulement l’action
  `0x77 / 0x10`. Son avertissement indique explicitement qu’un résultat visuel
  ne prouvera ni les mouvements d’objets ni les transactions d’or à distance.
  Le hook de `0x528270` demeure actif dans le même probe pour comparer plus tard
  l’ouverture normale au déclenchement technique. Un second hook strict de
  `0x4BFF30` journalise maintenant le GUID, X, Y, la page de stockage et le code
  de retour de chaque insertion. La DLL compilée reste locale et non déployée
  jusqu’au renouvellement normal de la validation en ligne.

## Audit du placement dynamique

- Le mécanisme gouverné de Vendor Stock Refresh fournit les deux primitives
  génériques requises : `UI_FindChildWidgetByName` `0x856220` retrouve un enfant
  direct par son nom, puis `UI_GetWidgetLocalRect` `0x8562A0` lit son rectangle
  runtime signé `{x,y,width,height}`. Les chemins de rendu et de hit-test relisent
  la même géométrie; un placement relatif peut donc suivre le layout réel sans
  livrer un override JSON.
- Les variantes HD souris `original` et `expansion` de BKVince, TCP, BK, BT et
  VNP ont été comparées en lecture seule. Les variantes manette disponibles de
  BKVince, BK, BT et VNP ont également été inspectées. Les quatre enfants
  directs `background`, `gold_amount`, `gold_button` et `grid` sont communs aux
  layouts observés malgré des rectangles, grilles et fonds fortement différents.
- `close` n’est pas une ancre portable : il est absent des layouts manette
  observés. `click_catcher` et `RightHinge` ne sont pas déclarés par toutes les
  variantes d’expansion, même lorsqu’ils peuvent être hérités par `basedOn`.
- Le contrat desktop proposé utilise `grid.x` pour suivre l’origine horizontale de la
  grille et l’union runtime de `gold_button` avec `gold_amount` pour obtenir la
  ligne verticale du footer. Le rectangle du panneau ou `background` sert de
  limite. Avant d’afficher le contrôle, la politique devra prouver que le bouton
  est contenu dans le panneau et ne chevauche ni `grid` ni le bloc d’or.
- La manette exige une politique distincte : dans le layout BKVince observé,
  `gold_button` commence à `x=201` tandis que `grid` commence à `x=210`; un
  bouton aligné sur `grid.x` chevaucherait donc le contrôle d’or. Le repli sûr
  actuel invalide ce candidat au lieu de le placer; la position et le graphe de
  navigation manette restent un gate explicite.
- Une grille simplement agrandie, déplacée ou redimensionnée demeure donc
  adaptable tant que le `PlayerInventoryPanel` et ces ancres sémantiques sont
  conservés. Un mod qui remplace le panneau, renomme les ancres ou fait occuper
  tout le footer par sa grille devra utiliser un profil de compatibilité; sans
  profil valide, le plugin se repliera en masquant son contrôle.
- Contrairement à Vendor Stock Refresh, l’inventaire ne contient aucun bouton
  natif RemoteStash à repositionner. Le point d’entrée de construction ou de
  configuration du `PlayerInventoryPanel`, la création d’un `ButtonWidget`, son
  rattachement comme enfant, son message de clic et son graphe de focus manette
  restent à prouver avant toute implantation native.
- Les constantes du handler vendeur prouvent aussi le hash de message UI :
  FNV-1a 64 bits, offset initial `0x1C1D8987E0EFCA1A` et prime
  `0x100000001B3`. Ce calcul reproduit indépendamment `Repair`
  `0x20BADA21BC8CB334`, `RepairAll` `0x5680CE604E1D402F`, `RefreshAll`
  `0xB7AA1748D66EFCAF` et `Close` `0x5E8250FB85D64C23`; il produit
  `0xB3B0A478381C4725` pour `PlayerInventoryPanelMessage:DropGold`. La politique
  contient maintenant le même calcul `constexpr` pour borner plus tard le
  message privé `PlayerInventoryPanelMessage:RemoteStash`, hash
  `0x055A7CEA95897DC9`, sans valeur magique non expliquée.
- `RemoteStash-src` contient maintenant uniquement une politique C++20 sans
  appel natif et sa suite de tests. Elle centre le rectangle du futur bouton sur
  le footer d’or, l’aligne sur `grid.x`, vérifie les limites du panneau et refuse
  les collisions avec la grille ou l’or. Elle ne construit aucune DLL, aucun
  hook, aucune configuration et aucun asset; le cas manette y est couvert comme
  repli invalide jusqu’à sa politique dédiée.
- La configuration et le build MSVC Release x64 de cette suite native-free sont
  verts : `remote-stash-layout-policy` passe `1/1`. Cette preuve couvre le calcul
  de hash, un layout desktop BKVince-like, un layout redimensionné, le fallback
  sur un seul contrôle d’or, les collisions grille/footer, les géométries
  absentes et le refus explicite du candidat manette.
- Le cadastre a été régénéré après l’ajout structurel de `RemoteStash-src`,
  sa zone porte des métadonnées explicites et
  `node scripts/validate-cartographie/validate.mjs` retourne `VALID`.

## Hypothèses à tester

- L’appel direct du handler banque `0x528270` depuis le thread et le contexte
  serveur adéquats devrait réutiliser l’événement UI natif `0x77 / 0x10`; cette
  hypothèse reste à confirmer dynamiquement.
- Puisque l’ouverture locale affiche déjà le layout BKVince personnalisé, les
  dimensions et onglets devraient rester sous la responsabilité du panneau
  natif. La compatibilité distante demeure néanmoins non prouvée.
- Le principal risque n’est plus de reconstruire le layout, mais de fournir un
  contexte d’opération valide : joueur, client, objet stash, état d’interaction,
  ville/hors ville et thread serveur. Un déclencheur hors ville ou sans coffre
  résolu pourrait exiger un chemin plus large qu’un simple appel de fonction.
- Les déplacements d’objets et la fermeture doivent encore être inspectés
  séparément. La transaction d’or prouve déjà au moins une validation serveur
  de proximité; on ne doit pas extrapoler qu’elle couvre tous les paquets, ni
  supposer que les autres opérations accepteront une ouverture seulement UI.
- L’insertion paraît indépendante de la proximité physique du coffre, mais cela
  reste une inférence par absence de dépendance directe. Le test décisif sera le
  code de retour de `0x4BFF30` après `remote-stash-open`, suivi d’une vérification
  de persistance après fermeture et rechargement.
- Les ancres communes observées devraient permettre le même calcul runtime sur
  plusieurs inventaires étendus. Cette compatibilité reste une preuve statique
  de structure : elle ne sera considérée validée qu’après observation des
  rectangles effectifs et du hit-test sur au moins BKVince et un layout tiers
  réellement redimensionné.

## Gates observables

- identifier et borner la fonction native d’ouverture, ses callers et son ABI;
- distinguer l’action client, les éventuels paquets et le contexte serveur;
- prouver les signatures strictes et l’unique propriétaire de chaque hook;
- déclencher l’ouverture depuis un contrôle technique minimal, sans sprite final;
- fermer proprement le panneau et préserver inventory, personal stash et shared stash;
- vérifier ville/hors ville, changement d’acte, souris/manette, solo, hôte et joiner;
- démontrer zéro perte, duplication, sauvegarde corrompue, crash ou désynchronisation;
- borner le cycle de vie du `PlayerInventoryPanel` et obtenir son pointeur après
  construction complète des enfants;
- prouver une voie sûre pour créer et rattacher un `ButtonWidget` natif, ou
  rejeter cette voie avant de retenir explicitement une solution d’overlay;
- tester la politique d’ancrage, de collision et de repli sur BKVince, un
  inventaire étendu tiers et les layouts manette.

## Prochain gate

Après renouvellement normal de la validation en ligne du jeu, confirmer qu’une
interaction native avec le coffre entre dans `OBJECTS_OperateFunction32_Bank`
`0x528270`. Déclencher ensuite cette même voie depuis une commande ou un raccourci
technique minimal exécuté dans un contexte serveur valide, avant tout bouton ou
sprite. Vérifier d’abord la ville, la fermeture et le layout BKVince, puis tracer
un dépôt via le hook `0x4BFF30`, un retrait et une transaction d’or afin de
borner les validations serveur réellement nécessaires. Les cas hors ville,
multijoueur et layouts tiers suivront seulement si ce gate est vert.

En parallèle, identifier statiquement le constructeur ou configurateur du
`PlayerInventoryPanel`, son ABI et ses callers. Prouver ensuite comment un nouveau
`ButtonWidget` peut devenir un enfant possédé du panneau et recevoir un message de
clic sans remplacer le JSON du mod. Aucun hook de panneau ne sera implanté avant
signature stricte, propriétaire unique et cycle de destruction démontrés.
