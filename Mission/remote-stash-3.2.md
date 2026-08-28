# RemoteStash — D2R 3.3.93847 (corpus natif commun 3.2.92777)

Dernière mise à jour : 28 août 2026

Statut : `Remote Stash 2.2.0` est la version de release retenue par Vincent.
Son binaire reproductible est installé et validé dans le runtime global D2R
3.3.93847, son manifeste est promu et son archive publique stricte est prête
localement. La release 2.0.0 reste la dernière version déjà publiée tant que
la 2.2.0 n'est pas envoyée au dépôt produit RuffnecKk D2RLoader Suite.

## Décisions confirmées

- Après une pause temporaire au profit de Repair Costs Cap, Vincent a repris
  RemoteStash selon l’Option A le 24 juillet 2026. ForceLarzukSockets
  reste intacte à son gate de validation en jeu.
- Depuis le 26 juillet 2026, Vincent autorise la préparation statique et la
  compilation de RemoteStash en parallèle de la mission active. Le prototype
  0.1.0 est maintenant déployable, mais aucun contournement du gate officiel de
  connexion ni aucune prétention de validation gameplay ne sont autorisés.
- Vincent confirme la version publique comme plugin autonome permanent : aucun
  merge futur, aucune catégorie, aucune DLL propriétaire et aucune clé
  `D2RPlugins.json` ne sont prévus.
- `RemoteStash.dll` reste hybride, attribuée exactement à `RuffnecKk`, sans
  modifier, lier ni redistribuer une DLL d’eezstreet. Elle ne dépend pas du
  PluginPack; la coexistence avec ses cinq DLL reste seulement une compatibilité
  optionnelle.
- `RemoteStash.json` configure uniquement le hotkey optionnel. Son absence ou
  `enabled=false` laisse le bouton pleinement fonctionnel et ne démarre aucun
  worker d'entrée. Le mod hôte conserve la propriété exclusive du widget, de ses
  coordonnées, de son sprite et de son tooltip dans son propre layout.
- La première phase a prouvé le chemin natif d’ouverture du stash et sa
  reproduction depuis un autre contrôle UI. Le chantier couvre maintenant le
  sprite, le placement final et l’adaptation aux layouts personnalisés.
- Le 27 juillet 2026, Vincent a demandé de commencer en parallèle la preuve du
  placement adaptatif. Le futur bouton ne doit utiliser aucune coordonnée propre
  à BKVince : il doit lire le layout runtime actif, se placer relativement à
  des enfants nommés du `PlayerInventoryPanel`, vérifier les collisions et se
  masquer si aucune géométrie sûre n’est disponible.
- Le 4 août 2026, Vincent autorise l'implantation d'un hotkey configurable qui
  appelle le même chemin RemoteStash sans dupliquer la logique d'ouverture. La
  configuration demeure propre à la DLL autonome et ne crée aucune dépendance
  envers `D2RPlugins.json` ou le PluginPack.

## Résultat joueur attendu

Un contrôle placé dans l’écran d’inventaire permet d’ouvrir le stash du joueur
sans interaction directe avec le coffre du monde. Le plugin doit réutiliser le
comportement autoritaire du jeu plutôt que recréer une grille ou déplacer des
objets lui-même.

## Incubation compatible PluginPack

- Description anglaise : `Opens the player stash remotely from a button or configurable hotkey.`
- Le JSON autonome est maintenant justifié par l'option réelle de hotkey; aucun
  TOML n'est créé.
- Rechercher la configuration d’abord dans le mod actif puis dans le dossier
  global du jeu; une configuration présente mais invalide devra être refusée.
- Aucun merge futur n'est prévu : `RemoteStash.dll` et `RemoteStash.json` restent
  autonomes. La coexistence avec le PluginPack ne constitue pas une dépendance.

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
  natif RemoteStash à repositionner. Le prototype 0.1.0 ajoute donc un enfant
  `remote_stash` au layout BKVince et repositionne cette instance après le
  configurateur prouvé du `PlayerInventoryPanel`. La création et la possession
  entièrement natives d’un `ButtonWidget`, sans déclaration JSON, ainsi que le
  graphe de focus manette restent à prouver.
- Le premier clic visuel a montré que `PlayerInventoryPanelMessage` est un enum
  fermé : la valeur privée `RemoteStash` déclenche l’assertion gouvernée
  `TypeDesc.cpp:463`, `Given key not found`, avant que le handler original puisse
  la dispatcher. Le layout utilise donc la valeur native enregistrée `DropGold`.
  La première garde 0.1.1 conservait toutefois le dernier panneau configuré;
  le témoin suivant a ouvert le dialogue `Drop Gold`, prouvant qu’un autre
  panneau avait remplacé cette identité avant le clic. La garde 0.1.2 ne conserve
  plus aucun panneau global : elle remonte depuis le widget cliqué vers son
  parent à `+0x30`, puis exige que `FindWidget(parent, "remote_stash")` retourne
  exactement ce widget. Le vrai bouton d’or ne peut pas satisfaire ce test.
- `RemoteStash-src` contient la politique C++20, sa suite de tests et le plugin
  natif autonome 0.1.5. La politique aligne le bouton sur `grid.x`, vise le
  centre vertical du footer d’or, puis le descend sous la grille quand un sprite
  plus haut l’exige. Elle vérifie les limites du panneau et refuse toujours les
  collisions avec la grille ou l’or. Le sprite coffre personnalisé comporte
  quatre états en `176 × 112` et une variante low-end `88 × 56`; le cas manette
  demeure un repli masqué jusqu’à sa politique dédiée.
- La configuration et le build MSVC Release x64 sont verts :
  `remote-stash-layout-policy` passe `1/1`. Cette preuve couvre le calcul
  d’un layout desktop BKVince-like, un layout redimensionné, le fallback
  sur un seul contrôle d’or, les collisions grille/footer, les géométries
  absentes et le refus explicite du candidat manette.
- Le cadastre a été régénéré après l’ajout structurel de `RemoteStash-src`,
  sa zone porte des métadonnées explicites et
  `node scripts/validate-cartographie/validate.mjs` retourne `VALID`.

## Prototype technique 0.1.5

- Le type runtime `ButtonWidget` expose son objet `onClickMessage` à l’offset
  `+0x558`. `RemoteStash 0.1.5` intercepte le dispatcher UI prouvé à `0x843D90`
  par le broker exporté de `BulkSkillPointAllocation 1.2.4`; si ce broker est
  absent, il peut posséder directement ce hook après vérification stricte de sa
  signature. Seul l’objet message appartenant au `remote_stash` actuellement
  résolu est consommé. Tous les autres messages, dont celui du vrai
  `gold_button`, sont transmis inchangés.
- `UI_PlayerInventory_RefreshGoldControls` `0x22BA70` reçoit directement le
  `PlayerInventoryPanel*`, retrouve `gold_amount` puis `gold_button`, et possède
  une signature stricte de 32 octets. Le plugin appelle l’original avant de lire
  `panel`, `grid`, `gold_button`, `gold_amount` et `remote_stash`, puis applique
  la politique de placement à leurs rectangles effectifs.
- Le bouton est déclaré par le layout BKVince sous le nom `remote_stash` et
  utilise `PANEL\\Inventory\\RemoteStashButton`. Le source PNG des quatre états
  reste avec le plugin et les variantes `.sprite` HD/low-end sont livrées dans
  le MPQ. Un layout tiers peut conserver ses dimensions et son art, mais doit
  actuellement fusionner ce même enfant nommé jusqu’à ce qu’une création native
  de widget soit prouvée.
- Sur un layout desktop sûr, le bouton s’aligne sur `grid.x`, se centre autant
  que possible sur le footer d’or et se décale vers le bas si la grille réelle
  descend davantage. Le rectangle natif de `gold_button` n’est jamais modifié.
  S’il manque une ancre, sort du panneau ou chevauche encore la grille ou l’or —
  notamment avec la politique manette actuelle — il est désactivé et masqué.
- La table de handlers client construite au runtime utilise des entrées de
  24 octets indexées par opcode. Les handlers déjà gouvernés `0x9C` et `0x9D`
  prouvent son pas; l’entrée `0x77` pointe vers `0x12DBC0` et exige exactement
  deux octets. Cette fonction relaie vers `0x1F0AB0`, qui reconnaît explicitement
  l’action `0x10` comme ouverture du stash.
- Le clic appelle donc `0x12DBC0` avec `{0x77, 0x10}` sur le thread UI. Ce chemin
  reproduit fidèlement la réception client du paquet natif, mais ne passe pas par
  `OBJECTS_OperateFunction32_Bank` et ne prouve aucune session serveur. Le statut
  console l’annonce explicitement avec `serverBankSession=false`.
- La DLL autonome et hybride porte l’auteur `RuffnecKk`, la description
  `Opens the player stash from the inventory screen.`, aucun TOML et aucun JSON.
  Elle ne lie ni ne redistribue une DLL d’eezstreet et refuse tout build autre
  que 92777 ou toute signature native divergente.
- Le build MSVC Release x64, les exports D2RLoader, le manifeste API et le test
  `remote-stash-layout-policy` sont verts. Le test couvre BKVince-like, un layout
  redimensionné, les ancres partielles, les collisions et le repli manette.
- Le déploiement mod-local BKVince est byte-exact pour la DLL, le layout et les
  deux sprites. Après retrait de la sonde temporaire, le SHA-256 final de
  `RemoteStash.dll` est
  `21033A82199A550D60DB82EB16CE7071351F6E912BD3DAC8F6FAA7FDBEF8D7C2`;
  celui du broker `BulkSkillPointAllocation.dll` 1.2.4 est
  `985513E5C96ABEE41E4316D6B11D22259FAD26E39AADCB70631570CB793D8FF7`.
  Un cold start frais du 27 juillet 2026 à 13:10 charge les deux DLL, installe
  le hook `0x22BA70` et enregistre l’intercepteur auprès du broker UI partagé.
- Le premier témoin visuel BKVince du 27 juillet n’affichait aucun bouton. Le
  screenshot prouve que le panneau actif était
  `PlayerInventoryExpansionLayout` grâce aux onglets d’armes I/II. Ce layout
  redéclare explicitement sa collection `children` et omettait `remote_stash`,
  bien que son layout de base le fournisse. La déclaration héritée a été ajoutée
  à la variante expansion, validée comme JSON avec commentaires, synchronisée
  byte-exactement et redémarrée avec les deux hooks acceptés. Le second témoin
  confirme le rendu, le survol `PERSONAL` et la position dynamique en bas à
  gauche. Son clic 0.1.0 n’a émis aucune ouverture et a produit l’assertion
  `Given key not found ... valueName = "RemoteStash"`; aucune ligne de dispatch
  RemoteStash n’a suivi. La correction 0.1.1 a supprimé cette assertion, mais
  son témoin a ouvert le dialogue natif `Drop Gold`, prouvant que sa garde
  d’identité globale n’avait pas reconnu le widget. Les itérations 0.1.2–0.1.5
  ont ensuite déplacé l’interception vers le dispatcher UI partagé et borné le
  message par l’identité du widget courant. Vincent confirme que 0.1.5 ouvre le
  panneau stash natif sans afficher `Drop Gold`. Vincent confirme ensuite le
  sprite coffre, sa taille, son placement et son hit-test. Le tooltip précédent
  `PERSONAL` est remplacé par la clé BKVince `remoteStashTooltip` (ID `30429`),
  dont toutes les langues affichent volontairement `YOUR PRIVATE STASH`.
  Vincent confirme maintenant ce rendu au survol ainsi que la conservation de
  la position BKVince du bouton d’or.
- Le 28 juillet 2026, le profil temporaire `RemoteStashRetail` a isolé la DLL
  mod-locale et les seuls assets/layouts requis, sans charger BKVince ni aucun
  autre plugin. Les six fichiers source/runtime étaient byte-exacts, dont
  `RemoteStash.dll` au SHA-256
  `21033A82199A550D60DB82EB16CE7071351F6E912BD3DAC8F6FAA7FDBEF8D7C2`.
  Le cold start du build `3.2.92777` a chargé `Remote Stash 0.1.5`, accepté les
  hooks `0x22BA70` et `0x843D90`, puis terminé avec
  `scanned=1 active=1 disabled=0 rejected=0 failed=0` et `24/24`. Sur le layout
  desktop retail-like `10 × 4`, Vincent confirme visuellement le bouton en bas
  à gauche, le bloc d’or inchangé, le hit-test, le tooltip retail localisé
  `OPEN CURRENT STASH` et l’ouverture du panneau sans dialogue `Drop Gold`.
  Cette preuve ferme le témoin desktop retail-like distinct de BKVince; elle ne
  valide toujours ni un inventaire étendu tiers, ni la manette, ni les opérations
  d’items/or et leur persistance sans session banque serveur.
- Le kit privé `RemoteStash-0.1.5-intermod-test-retail-validated.zip` reprend la
  DLL et les deux sprites byte-exacts testés, deux fragments de layout à fusionner
  et les consignes de validation. Il utilise désormais la clé Blizzard native
  `@OpenCurrentStashLegend`; aucun fichier de localisation, ID custom ou override
  complet de layout BKVince n’est distribué. L’archive contient zéro source,
  TOML, log, PDB ou DLL tierce et porte le SHA-256
  `FE27E9AC96343C4BC8BC1E27B18F42739BA83A40A1986B8C8FF526515D24230F`.
  Il s’agit d’un kit d’intégration destiné au testeur, pas encore du ZIP public
  strict du plugin : la création native du widget et la matrice serveur restent
  ouvertes.
- Le kit documenté `RemoteStash-0.1.5-intermod-kit-20260728.zip` contient un
  `README.txt` anglais volontairement court et lisible en texte brut :
  installation, sprite, tooltip, placement et test. Son allowlist contient
  exactement le README, `RemoteStash.dll`, deux snippets JSON valides et les
  deux sprites; aucun source, layout complet, TOML, log, PDB ou DLL tierce n’est
  distribué. La DLL et les sprites conservent leurs hashes validés; le ZIP de
  182366 octets porte le SHA-256
  `4596EBB5CD6EB35F70DE58C9CF57F188D2F2E7B45FA2F6D59B5DEE557D5794D3`.
  Cette archive demeure un kit développeur privé et non le ZIP public minimal
  gouverné par la checklist d’incubation.

## Hypothèses à tester

- L’appel direct du handler banque `0x528270` depuis le thread et le contexte
  serveur adéquats devrait réutiliser l’événement UI natif `0x77 / 0x10`; cette
  hypothèse reste à confirmer dynamiquement.
- L’ouverture locale 0.1.5 affiche le panneau stash natif. Ses dimensions et
  onglets restent sous la responsabilité du layout actif; l’autorité serveur et
  la compatibilité avec un layout tiers demeurent non prouvées.
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

Planifier puis implanter un hotkey configurable qui déclenche exactement la même
action que le bouton, avec un propriétaire de hook unique, un refus des conflits
de touches et des gardes explicites pour le chat, les modales, les écrans titre
et les répétitions clavier. Le bouton et le hotkey devront partager une seule
fonction d’ouverture et une seule télémétrie.

Confirmer ensuite ce que le panneau permet réellement sans session serveur :
affichage, fermeture, dépôt, retrait et or, avec contrôle de persistance après
rechargement.

En parallèle, confirmer qu’une interaction physique normale entre dans
`OBJECTS_OperateFunction32_Bank` `0x528270`, puis rechercher un déclencheur
autoritaire exécuté dans le contexte serveur valide. Le jalon final devra faire
passer ville/hors ville, solo/hôte/joiner, souris/manette et au moins un inventaire
tiers redimensionné sans perte, duplication, corruption ni désynchronisation.

## Compatibilité PluginPack 0.1.6 — 28 juillet 2026

RemoteStash recherche désormais `plugin-skills.dll` puis le témoin autonome
`BulkSkillPointAllocation.dll` comme brokers possibles de `0x843D90`. Il expose
aussi le même contrat d'enregistrement afin que `plugin-skills.dll`, chargé plus
tard par D2RLoader, puisse s'enregistrer lorsque RemoteStash possède déjà le
dispatcher. La chaîne appelle d'abord son consommateur, puis l'identité stricte
du bouton Remote Stash, et transmet tout autre message au trampoline original.

Le build Release et 1/1 test passent. La DLL 0.1.6 porte le SHA-256
`1447DAE2FE0D7424B76D0ED7C6BBC4DA500BC3C9E7EFA70B4D5464D07ABB102A`.
Le cold start conjoint avec `skills.bulkSkillPointAllocation` actif atteint
`24/24`, `scanned=29 active=27 disabled=2 rejected=0 failed=0` : RemoteStash
possède seul `0x843D90`, tandis que `plugin-skills` possède `0x5F4B90` et
`0x0EC700`. Le runtime est ensuite restauré byte-exact. Cette validation ferme
le conflit de chargement technique; elle ne remplace pas les gates serveur,
items, or et persistance de RemoteStash.

## Session distante 0.2.1 — 29 juillet 2026

- Vincent confirme que le prototype 0.2.0 ouvre visuellement le stash dans
  City of the Damned, mais qu'il est impossible d'y prendre un objet ou d'en
  ajouter un. Le log prouve la réception de la requête privée et l'envoi du
  paquet serveur `0x77 / 0x10`; ce résultat invalide explicitement l'affirmation
  selon laquelle ce seul paquet constituerait une session banque autoritaire.
- Le callback miroir de retrait est borné à `0x4AA100`. Il reçoit le même paquet
  de 17 octets, construit la page et les coordonnées comme état source, construit
  le curseur vide comme destination, puis appelle le même moteur de transition
  `0x471E90` que l'insertion `0x4BFF30`. Son ABI et sa signature stricte de
  32 octets sont établies statiquement; le succès distant reste un gate runtime.
- Deux callsites client propres au cycle de vie du stash, `0x259132` et
  `0x25A11D`, appellent `DUNGEON_IsRoomInTown` `0x2F0750`. RemoteStash 0.2.1
  intercepte ce prédicat, mais ne retourne vrai artificiellement que pour leurs
  adresses de retour exactes pendant une ouverture RemoteStash, ou de manière
  synchrone dans les callbacks serveur de dépôt/retrait du joueur possédant la
  session. Tous les autres appels, y compris les mécaniques de ville des autres
  plugins, passent par le trampoline original.
- Aucun layout, rectangle, sprite, tooltip ni bouton d'or BKVince n'a été
  modifié. Le build Release et le test de placement passent; la DLL gouvernée et
  runtime porte la version 0.2.1 et le SHA-256
  `42FB06F9081D6A3CB9B5FB29EB714C1481093E6F9896A44D17FF838C6BF0F6DA`.
- Le cold start BKVince charge les six hooks RemoteStash, dont `0x2F0750` et
  `0x4AA100`, avec `scanned=30 active=28 disabled=2 rejected=0 failed=0`.
  Le prochain gate est le retest manuel City of the Damned : retrait, dépôt,
  fermeture/réouverture puis persistance. L'or, le client joint et le profil
  retail restent ensuite à valider séparément.

## Barrière de transaction page 4 — 0.2.2 — 29 juillet 2026

- Le retest 0.2.1 dans City of the Damned produit des résultats serveur
  déterministes : les transitions vers la page `0` réussissent avec `result=0`,
  tandis que les dépôts et retraits visant la page `4` échouent avec `result=1`.
  `scopedTownBypasses=0` prouve qu'aucun appel à `DUNGEON_IsRoomInTown` ne
  participe à ces refus; le contournement serveur ajouté en 0.2.1 ciblait donc
  la mauvaise barrière.
- Le moteur commun `0x471E90` valide chaque état de paquet par `0x474700`. Cette
  fonction reçoit l'instantané de transaction, l'état de 16 octets et un booléen
  natif de contournement de proximité. Pour une page `4`, elle refuse si ce
  booléen est faux et si `snapshot+0x20` n'indique pas un coffre proche. Le
  constructeur d'instantané `0x46C690` ne positionne ce champ qu'après avoir
  trouvé l'objet de classe `267` à une distance native d'au plus `50`.
- RemoteStash 0.2.2 promeut ce booléen existant uniquement pendant le callback
  synchrone du joueur possédant une session distante et uniquement lorsque
  l'état exact vise la page `4`. Les validations natives de propriété, curseur,
  coordonnées, dimensions, occupation et cohérence de l'instantané continuent
  de s'exécuter. `DUNGEON_IsRoomInTown` ne conserve que ses deux callsites client
  bornés au cycle de vie visuel du stash.
- Aucun fichier de layout, rectangle, sprite, tooltip ni bouton d'or BKVince
  n'est modifié. Le build Release et le test de placement `1/1` passent. La DLL
  gouvernée et runtime porte le SHA-256
  `17867C42F3EB4472AAD176523FCD324724D5CD3CD86ED38F15AD2F33A26FA264`.
- Le cold start BKVince charge le nouveau hook `0x474700` et atteint
  `scanned=30 active=28 disabled=2 rejected=0 failed=0`. Le gate manuel reste
  le retrait puis le dépôt dans City of the Damned; les logs attendus sont
  `result=0` avec `scopedStashProximityBypasses=1` pour chaque transition page 4.

## Validation fonctionnelle hors ville — 0.2.24 — 29 juillet 2026

- Vincent confirme dans City of the Damned le dépôt et le retrait manuels dans
  le coffre personnel, le shared stash et le crafting tab. Un objet déposé dans
  le shared stash à la page 69 persiste après Save & Exit, puis peut être repris
  et replacé dans l’inventaire.
- Le quick move `Ctrl + clic gauche` nécessitait encore deux callsites client de
  `DUNGEON_IsRoomInTown`, `0xFEE3B` et `0xFF1DC`. RemoteStash 0.2.24 ne les
  contourne que pendant une session distante active, pour un retrait exact du
  stash vers l’inventaire et pendant une fenêtre bornée à 1000 ms. Le hook
  d’état UI conserve toujours le résultat natif et ne fabrique aucun panneau.
- La matrice finale en difficulté Pain, à Outer Cloister, réussit dans les deux
  directions avec `Ctrl + clic gauche` pour le coffre personnel et le shared
  stash, sur les pages partagées 1 et 69. Le processus D2RLoader reste actif et
  répondant; aucun événement Windows Application Error ou WER n’est observé.
- Le build Release et le test de placement `1/1` passent. La DLL gouvernée et la
  DLL runtime BKVince sont byte-identiques au SHA-256
  `A7E4A1C98F68DF432E1DCF2D39C8E78F1607E772895F38F17E44F2B4FE524FF8`.
- Le placement dynamique, le sprite, le tooltip, le bouton d’or et les layouts
  BKVince restent inchangés. Les messages transitoires `CASC container locked`
  du loader se réparent par la résolution résidente/fichier suivante et ne
  correspondent pas à un crash observé.

## Variante publique intermod — 0.2.25 — 29 juillet 2026

- La variante publique est isolée sous `addons/RemoteStash`; elle ne modifie ni
  le source, ni la DLL, ni les layouts, ni les sprites de BKVince 0.2.24. La DLL
  BKVince conserve le SHA-256
  `A7E4A1C98F68DF432E1DCF2D39C8E78F1607E772895F38F17E44F2B4FE524FF8`.
- Cette variante n’a aucun fichier de configuration. Le moddeur fournit le
  widget `remote_stash` dans son propre layout desktop et possède entièrement
  son rectangle, son sprite, ses frames et son tooltip. Le plugin vérifie
  seulement que le rectangle possède une largeur et une hauteur positives; il
  ne contient plus de fonction qui écrit la position du widget.
- Le build Release x64 et le test `remote-stash-intermod-layout-contract` passent
  `1/1`. La DLL 0.2.25 exporte les trois entrées D2RLoader attendues et porte le
  SHA-256 `0DF7A436DD4DEDCD83B4811AD1A185935E1603818B1F1B589A5AC79476F70C2D`.
- Le cold start mod-local du profil isolé `RemoteStashRetail` accepte les douze
  hooks, charge `Remote Stash 0.2.25 by RuffnecKk`, atteint `24/24` avec
  `scanned=3 active=3 disabled=0 rejected=0 failed=0`, puis la session frontend
  se termine sans validation gameplay supplémentaire.
- Le kit public contient la DLL, un `README.txt`, deux fragments JSON et les
  sprites HD/low-end prêts à l’emploi. Aucun source, PDB, log, configuration ou
  DLL tierce n’est inclus. L’archive
  `RemoteStash-0.2.25-InterMod.zip` porte le SHA-256
  `375679E22AF3CAB8BE4B2CF1DA560A1D495F6AD4D1410BCD5A4FD0A726B87E5B`.

## Coexistence PluginPack — 0.2.26 — 30 juillet 2026

- Les variantes publique et BKVince ne possèdent plus les prologues partagés
  `UI_DispatchMessage` `0x843D90`, `DUNGEON_IsRoomInTown` `0x2F0750` et
  `CLIENT_TransferItemToInventoryPage` `0x15F8B0`. RemoteStash redirige plutôt
  ses treize callsites D2R.exe strictement validés vers trois relais proches,
  puis appelle les entrées natives vivantes. Le PluginPack peut donc conserver
  ses hooks pour Bulk Skill Point Allocation, Prevent Merc Death in Town et
  Equipped Item to Cube, sans ordre de chargement imposé.
- La variante publique demeure entièrement contrôlée par le layout du moddeur.
  La variante BKVince conserve exactement son placement dynamique, son sprite,
  son tooltip et la position vanilla du bouton d’or. Aucun fichier de layout ou
  d’asset BKVince n’a été modifié pour cette compatibilité.
- Les builds Release x64 et leurs tests CTest passent `1/1`. La DLL publique
  porte le SHA-256
  `94EA3B54BA5246DDE99854BC4846496DFE817AE0EAE623419CA308BE6378102B`;
  la DLL BKVince porte le SHA-256
  `2649DC8816509BF41BD3A0A85E6BB828DF58BB96486B263B2E6BAF4E9CBA8D92`.
- Le cold start BKVince avec les cinq DLL courantes du PluginPack et les trois
  fonctions conflictuelles activées atteint `24/24` et
  `scanned=17 active=15 disabled=2 rejected=0 failed=0`. Le log de
  `plugin-skills` annonce `UI broker=RemoteStash`; `plugin-misc` active à la
  fois Equipped Item to Cube et Prevent Merc Death in Town. Le profil public
  isolé répète le test avec ses six DLL actives et atteint
  `scanned=6 active=6 disabled=0 rejected=0 failed=0`, puis `24/24`.
- Les deux plugins globaux sans rapport qui perturbaient le premier témoin
  retail ont été écartés uniquement pendant le test puis restaurés avec leurs
  hashes d’origine. Le JSON et les anciennes DLL `plugin-misc` et
  `plugin-skills` de l’installation BKVince ont également été restaurés
  byte-exactement; seule la nouvelle DLL RemoteStash BKVince reste déployée.
- L’archive publique allowlistée `RemoteStash-0.2.26-InterMod.zip` contient
  seulement la DLL, le README, deux fragments JSON et les deux sprites. Elle ne
  contient ni source, PDB, log, configuration ni DLL tierce et porte le SHA-256
  `CAC44BDD70A837DFE6E7372EFEAEABEFF6291B752643DD5FCFFDECD4A43D2293`.
- Cette preuve ferme la coexistence technique au chargement. Les parcours
  gameplay de RemoteStash hors ville ont été validés par Vincent en 0.2.24;
  après le changement d’interception 0.2.26, un témoin manuel doit encore
  reconfirmer le bouton, le dépôt/retrait, le quick move et la persistance avec
  les trois fonctions PluginPack activées simultanément.

## Correction de la variante publique — 0.2.28 — 3 août 2026

- Le témoin BKVince a révélé une régression du routage UI 0.2.26 : le bouton
  public restait visible, mais ouvrait la fenêtre vanilla `Drop Gold`. Le relais
  étroit et le hook `UI_ButtonWidget_OnClick` essayé en 0.2.27 ne consommaient
  pas le message du widget réellement fourni par ce layout.
- La 0.2.28 restaure l’interception éprouvée de `UI_DispatchMessage 0x843D90`
  lorsque RemoteStash charge en premier. Les exports de broker restent présents
  afin que `plugin-skills.dll` puisse ensuite s’enregistrer auprès de
  RemoteStash; les relais composables de portée et de quick move sont conservés.
- La DLL publique, et non la variante privée BKVince, a été installée dans le
  profil mod-local BKVince. Le chargement atteint
  `scanned=13 active=12 disabled=1 rejected=0 failed=0`; Vincent confirme que
  RemoteStash fonctionne sur toutes les tabs du layout BKVince.
- Le build, le package et la DLL publique testée dans le runtime sont
  byte-identiques : version `0.2.28`, taille `38912`, SHA-256
  `002B46835E55D8DD25CF1E589322F2354494934DC3527FEC2CB06A6DFBFF07BC`.
  Les fichiers privés BKVince ont été restaurés à leur état gouverné.
- Le fragment public ne fournit plus les coordonnées BKVince `95,1656`.
  `SET_X_FOR_YOUR_LAYOUT` et `SET_Y_FOR_YOUR_LAYOUT` rendent le fragment
  volontairement inutilisable tant que le moddeur n’a pas fourni les
  coordonnées entières propres à son layout. Le sprite par défaut demeure
  facultatif; la géométrie ne possède aucune valeur universelle. Le README
  donne séparément `93,1347` comme référence desktop vanilla 3.2 vérifiée
  (`grid.x = 93`, `gold_button.y = 1347`), sans l’insérer dans le fragment.
- Le kit d’intégration autonome `RemoteStash-0.2.28.zip` contient exactement
  six entrées allowlistées : la DLL, le README, les deux fragments de layout et
  les deux sprites. Il ne contient aucune DLL du PluginPack et porte le SHA-256
  `C886261333BF1A7567749D60D88653C18137463ACA1680E0CF420B2FC2F1B325`.

## Candidate de test public — 0.2.29 — 4 août 2026

- La fermeture de session serveur suit maintenant le retrait de l'unité joueur
  active. La fermeture côté client est aussi demandée si l'interface du stash
  disparaît sans passer par le bouton de fermeture normal.
- La télémétrie disque synchrone a été retirée des chemins de déplacement
  d'objets. Les compteurs et le temps maximal restent disponibles sans écrire un
  log pour chaque opération.
- Le build Release x64 et CTest passent `1/1`. Le cold start BKVince charge
  `Remote Stash 0.2.29`, installe le hook de retrait d'unité à `0x43EC10` et
  atteint `scanned=13 active=12 disabled=1 rejected=0 failed=0`.
- Les DLL du build, du package et du runtime BKVince sont byte-identiques :
  version `0.2.29`, taille `37888`, SHA-256
  `DC65F01A733F8B1B0C119B7074A83D825FDCD5F78B805693B7669633FEE0F402`.
- La candidate `RemoteStash-0.2.29.zip` contient exactement six entrées : la
  DLL, le README, deux fragments de layout et deux sprites. Elle ne contient ni
  configuration, source, PDB, log ou DLL tierce et porte le SHA-256
  `F86B48B385CFD34098B7D5D94984325E75AEE34F836375E0B76A5BADFED9140C`.
- La validation externe doit encore confirmer deux régressions rapportées :
  absence de gel bref après une tentative de chevauchement invalide, puis retour
  au lobby et reconnexion du même personnage sans redémarrer D2R.

## Resynchronisation après placement refusé — 0.2.30 — 4 août 2026

- La vidéo externe `2026-08-04_22-59-05.mp4` montrait une désactivation après
  une tentative de placement sur des cases occupées; recliquer Remote Stash
  restaurait immédiatement les interactions. La 0.2.30 a tenté de reproduire
  ce réarmement depuis le callback générique d'insertion en renvoyant l'action
  native d'ouverture `0x10`.
- Le test suivant a invalidé l'attribution au chevauchement : le shared stash se
  désactivait aussi sans tentative de placement, environ cinq secondes après
  son ouverture. L'appel de réarmement synchrone depuis un callback serveur
  n'avait par ailleurs aucune preuve d'ordre par rapport aux paquets de refus.
- Le 4 août, l'audit de la 0.2.31 retire donc cette couche expérimentale au lieu
  de l'étendre. La 0.2.30 demeure une candidate historique non validée et ne
  constitue pas la base du correctif final.
- Les DLL du build, du package et du runtime BKVince sont byte-identiques :
  version `0.2.30`, taille `37888`, SHA-256
  `681C4F17C1A497290C22252C602E28C17545C4D81556DE890FF6C4087146A857`.
- Le cold start mod-local charge `Remote Stash 0.2.30 by RuffnecKk`, installe les
  hooks existants et atteint
  `scanned=13 active=12 disabled=1 rejected=0 failed=0`.
- La candidate `RemoteStash-0.2.30.zip` conserve les six entrées publiques : la
  DLL, le README, deux fragments de layout et deux sprites. Elle ne contient ni
  configuration, source, PDB, log ou DLL tierce et porte le SHA-256
  `CFEEC5DC3900415A9B09AC3BB9363D8D2942810041872C664F2A9920140248A6`.
- Le gameplay reste `not run` pour cette candidate. Le test externe doit
  confirmer qu'un placement chevauché refusé laisse immédiatement le stash
  interactif, sans deuxième clic sur Remote Stash, perte, duplication ou objet
  bloqué au curseur. La reconnexion depuis le lobby reste un gate séparé.

## Suppression du timeout de session partagé — 0.2.31 — 4 août 2026

- La vidéo externe `2026-08-04_23-38-37.mp4` et la précision du testeur
  invalident la 0.2.30 : dans le shared stash, les objets cessent de répondre
  environ cinq secondes après l'ouverture, sans dépendre d'un chevauchement.
- Le code client armait exactement `GetTickCount64() + 5000`. Si le layout
  partagé ne publiait jamais l'état UI vanilla `0x16` comme ouvert, l'expiration
  appelait `DeactivateRemoteClientSession(true)` et envoyait la requête de
  fermeture serveur, tandis que le panneau custom demeurait visible.
- La 0.2.31 supprime ce délai et ne ferme plus la session depuis le hook d'état
  UI que lorsqu'un état ouvert a réellement été observé puis devient fermé. La
  fermeture explicite du stash et le retrait de l'unité joueur au retour au
  lobby restent en place; aucun layout, asset, hook ou protocole n'est ajouté.
- L'audit retire aussi le réarmement expérimental de la 0.2.30. Aucun paquet
  d'ouverture n'est injecté depuis un callback d'item : la 0.2.31 corrige
  seulement la cause démontrée, sans supposer que l'overlap emprunte un callback
  particulier ni modifier l'ordre natif des réponses serveur.
- Le build Release x64 et CTest passent `1/1`. Les DLL du build, du package et
  du runtime BKVince sont byte-identiques : version `0.2.31`, taille `37888`,
  SHA-256 `99B1C84D04E3B52DC79E694AA2DAB4A4C2A49077C154DEBA8C0875C5F10B5A83`.
- Le cold start mod-local charge `Remote Stash 0.2.31 by RuffnecKk`, accepte les
  hooks existants et atteint `scanned=13 active=12 disabled=1 rejected=0
  failed=0`, puis le frontend complète `24/24`. Les assertions BKVince déjà
  connues après le frontend sont capturées et ignorées; aucun nouvel échec de
  chargement RemoteStash n'est observé.
- La candidate `RemoteStash-0.2.31.zip` contient exactement les six entrées
  publiques allowlistées et porte le SHA-256
  `85111B55E94E511B6FFFB28AF832FED7A8007A37FAC5730F0F9951BB9C239622`.
- Vincent et un testeur externe confirment le 4 août que cette révision auditée
  est stable et fonctionne correctement. Les scénarios qui reproduisaient la
  désactivation du shared stash et l'interaction après un placement chevauché
  refusé ne reproduisent plus la panne; ces deux gates passent à `passed`.
- Le verdict communiqué ne détaille pas séparément la reconnexion depuis le
  lobby. Cette case reste distincte, tout comme la matrice élargie inventaires
  tiers, manette et hôte/joiner; aucun succès n'est inféré pour ces cas.

## Hotkey configurable autonome — 0.3.0 — 4 août 2026

- `RemoteStash.json` appartient uniquement à `RemoteStash.dll`. Il ne lit ni ne
  modifie `D2RPlugins.json`, n'importe aucune DLL du PluginPack et n'impose aucun
  ordre de chargement. La recherche suit le mod actif, puis son dossier de mod,
  puis le dossier de travail global. Un fichier présent mais invalide refuse le
  plugin avant l'installation de ses hooks; un fichier absent conserve les
  valeurs sûres intégrées.
- Le défaut livré est `enabled=false`, `hotkey=CTRL+SHIFT+S` et `consume=true`.
  Les touches supportées sont `A-Z`, `0-9`, `F1-F24`, `SPACE`, `TAB`, `INSERT`,
  `DELETE`, `HOME`, `END`, `PAGEUP`, `PAGEDOWN`, `MOUSE3`, `MOUSE4` et `MOUSE5`,
  avec les modificateurs exacts `CTRL`, `SHIFT` et `ALT`.
- Le worker Win32 ne démarre que lorsque le hotkey est activé. Il filtre les
  répétitions et les entrées injectées, exige que D2R possède la fenêtre de
  premier plan, borne chaque demande à 250 ms et transfère l'action au thread UI
  du jeu par `WH_GETMESSAGE`. Le thread UI refuse l'action sans joueur local,
  pendant le stash, le chat ou les modales connues.
- Le bouton et le hotkey appellent tous deux `TryQueueRemoteOpenRequest`, lequel
  émet la requête serveur RemoteStash déjà validée. Aucun second protocole stash,
  aucun déplacement d'objet et aucun nouveau propriétaire de hook natif ne sont
  ajoutés par le hotkey. `UI_FindTopLevelPanelByName` `0x846170` est seulement
  appelé sur le thread UI après validation stricte de sa signature 92777.
- Le build Release x64 passe les contrats de layout et de hotkey `2/2`. La DLL
  0.3.0 mesure `174080` octets et porte le SHA-256
  `C1DE53CE098464911165274E00ECFBEEE8977084CCDFBC896ACAA97FF5AA9A52`.
  Les DLL du build, du package et du runtime sont byte-identiques.
- Deux cold starts mod-locaux BKVince couvrent `enabled=false` puis
  `enabled=true`. Les deux atteignent `24/24` avec
  `scanned=13 active=12 disabled=1 rejected=0 failed=0`. En mode activé, le log
  confirme `binding=CTRL+SHIFT+S`, la configuration du mod actif et le handoff
  prêt sur le thread UI. Les assertions BKVince déjà connues après le frontend
  restent capturées et ignorées.
- L'archive `RemoteStash-0.3.0.zip` contient sept fichiers allowlistés : DLL,
  JSON, README, deux fragments de layout et deux sprites. Aucun source, PDB, log
  ou DLL tierce n'est inclus. Elle mesure `249613` octets et porte le SHA-256
  `B933A57113267A6606C7041C8D5C013535DCDFC4C8E02845284877FF48F23645`.
- L'ouverture gameplay par hotkey reste `not run`; le runtime est laissé avec la
  configuration activée pour ce témoin manuel.

## Hotkey consommé et toggle final — 0.3.6 — 5 août 2026

- La configuration publique conserve `enabled=false`, adopte `hotkey=S` et garde
  `consume=true`. La configuration globale de validation active ces trois mêmes
  paramètres sans modifier `D2RPlugins.json` ni créer de dépendance envers une
  autre DLL.
- Le hook d'entrée Win32 consomme maintenant les messages clavier capturés quand
  la demande RemoteStash est acceptée. Le même raccourci choisit l'ouverture ou
  la fermeture selon l'état de la session distante et transmet l'action au
  thread UI du jeu.
- Le build Release x64 et CTest passent les contrats layout et hotkey `2/2`.
  Les DLL du build, du package et du runtime global sont byte-identiques :
  version `0.3.6`, taille `175616`, SHA-256
  `9166B1D1E25F0BF44C79F05CC32FAB3443FD58CAC69E7AA7C129D1CEB3F7A005`.
- Le cold start global charge `RemoteStash 0.3.6` sur D2R `3.2.92777`, résout la
  configuration globale avec `binding=S` et `consume=true`, installe tous ses
  hooks stricts et prépare le handoff du hotkey sur le thread UI.
- Vincent confirme en jeu le 5 août que `S` ouvre et ferme le stash, que le menu
  rapide des skills lié à `S` ne s'ouvre pas et que les micro-chutes de framerate
  ont disparu après le retrait de la trace UI diagnostique synchrone.
- L'archive publique `RemoteStash-0.3.6.zip` contient exactement sept fichiers :
  DLL, JSON, README court, deux fragments de layout et deux sprites. Elle ne
  contient ni source, PDB, log, preuve locale ou DLL tierce. Elle mesure `249096`
  octets et porte le SHA-256
  `6A46C3A371986AE56D14B848D79E69219B238B6F61EC987C871706CEFE5F7D0A`.

## Release publique officielle — 1.0.0 — 5 août 2026

- Vincent et un testeur externe coréen confirment que la candidate 0.3.6 est
  stable sur leurs deux mods, y compris avec le layout personnalisé du testeur.
  Cette preuve promeut le même comportement en première release stable 1.0.0.
- Le passage à 1.0.0 modifie la version du manifeste, des ressources Windows,
  des messages de log et du README; il ne change aucune politique de hotkey,
  session distante, déplacement d'objet ou intégration de layout.
- Le build Release x64 et CTest passent `2/2`. Les DLL du build et du package
  sont byte-identiques : version `1.0.0`, taille `175616`, SHA-256
  `5D3B92044B2731543BA38A373D935B474B61A3252602D11EE804B51CCBA40C5E`.
- La même DLL 1.0.0 est installée dans la portée globale avec un hash identique.
  Aucun cold start supplémentaire n'est lancé pendant la préparation de
  l'archive; la preuve gameplay demeure celle du code fonctionnel identique
  validé par Vincent et le testeur externe avant le bump de version.
- L'archive officielle `RemoteStash-1.0.0.zip` contient exactement sept fichiers :
  DLL, JSON, README court, deux fragments de layout et deux sprites. Chaque
  entrée extraite est byte-identique au package. L'archive mesure `249093`
  octets et porte le SHA-256
  `B992F3585B8616A68F26CDB3975C6548D135780A0FB2B2D954D32E1732AE0D92`.

## Hotkey réactif et interface persistante — candidate 1.1.0 — 8 août 2026

- Le délai client de 250 ms est retiré. Une pression valide arme désormais une
  demande persistante, coalescée jusqu'à son traitement sur le thread UI; la
  demande n'expire donc plus lorsque le personnage se déplace ou que le thread
  UI accuse un retard momentané.
- Lorsque `consume=true`, la touche configurée est consommée dès que la
  combinaison exacte est reconnue, indépendamment du délai du handoff. Le
  raccourci D2R portant la même touche ne reçoit plus une pression qui a été
  attribuée à RemoteStash.
- `UI_CloseInterfaceState` à `0xC7D30` est validé par une signature stricte de
  64 octets, unique sur l'image gouvernée 92777, puis intercepté seulement pour
  l'état stash `0x18` pendant une session distante. Le bouton X est identifié
  par le hash FNV-1a de la commande UI `Close`
  (`0x5E8250FB85D64C23`) et demeure une fermeture explicite, tout comme Escape,
  le hotkey et la fermeture serveur.
- Le teardown général appelant successivement la fermeture des états `0x18`,
  `0x19` et `0x0B` n'est pas allowlisté. Sa tentative de fermeture du stash est
  supprimée pendant la session distante afin qu'un clic de déplacement ne
  ferme plus le panneau. Des compteurs atomiques distinguent ce chemin sans
  ajouter de logging synchrone à l'entrée utilisateur.
- Le build Release x64 et CTest passent `2/2`. Les DLL du build, du package et
  du runtime global sont byte-identiques : version `1.1.0`, taille `176128`, SHA-256
  `0103166B712550E99E8F69FBC44388B3A27A5E99D5FEC31CE8F26BF8DB3807BB`.
- La synchronisation globale ne remplace pas `RemoteStash.json`; la
  configuration de validation reste `enabled=true`, `hotkey=H` et
  `consume=true`.
- Le gameplay reste `not run`. Il doit confirmer une seule pression en
  déplacement, l'absence d'ouverture du raccourci D2R concurrent, le maintien
  du stash pendant un clic de déplacement, puis les fermetures par hotkey,
  Escape et bouton X.

## Suppression ciblée de l'inventaire natif — candidate 1.1.3 — 8 août 2026

- Le témoin gameplay 1.1.2 ouvre encore le stash et l'inventaire ensemble. La
  lecture directe des compteurs de la DLL vivante observe 16 tickets armés et
  16 demandes RemoteStash dispatchées, mais zéro passage et zéro expiration
  dans le hook `UI_ToggleInterfaceState 0xCDE00`. Le délai de 150 ms n'est donc
  pas la cause : l'inventaire emprunte une autre surface native.
- `UI_OpenInterfaceState 0xCD7C0` reçoit `(int32 state, bool secondary) -> bool`,
  indexe l'état demandé dans le tableau UI puis distribue son message propre.
  Sa signature stricte de 68 octets est unique dans l'image gouvernée 92777 et
  ses 49 xrefs incluent plusieurs appels explicites avec l'état inventaire `1`.
  `CLIENT_ApplyUiPacket77` ouvre séparément le stash avec l'état `0x18`.
- La 1.1.3 remplace uniquement le hook de consommation 1.1.2. Pendant le ticket
  d'un hotkey accepté, elle supprime la première ouverture de l'état `1`; tout
  autre état, notamment le stash `0x18`, délègue immédiatement la fonction
  native vivante. En dehors de ce ticket, l'inventaire reste entièrement natif.
- Le build Release x64 et CTest passent `2/2`. Les DLL du build et du package
  sont byte-identiques : version `1.1.3`, taille `177152`, SHA-256
  `DA76E1F44E83058990E5A2FF8485220FB7D1D5D36B4C257F219FD86C4C6415B6`.
- Le cold start global charge RemoteStash 1.1.3 sur D2R `3.2.92777`, accepte le
  nouveau hook à `0xCD7C0`, garde la configuration globale `H / consume=true`,
  applique `18/18` patchsets, charge `14/14` plugins sans rejet ni échec et
  atteint l'étape frontend `24/24`.
- Le témoin gameplay confirme que l'inventaire ne s'ouvre plus, mais la capture
  `codex-clipboard-d2a769df-1515-471b-8cff-e188821e3bfa.png` montre les bordures
  supérieures et latérales du shell à deux panneaux laissées sans contenu à
  droite. Le filtre d'état fonctionne donc, mais la composition desktop native
  ne supporte pas un stash `0x18` isolé. La 1.1.3 n'est pas livrable en l'état;
  aucune nouvelle stratégie n'est implantée avant décision.

## Rétablissement de la composition native — candidate 1.1.4 — 8 août 2026

- Vincent confirme le retrait de la stratégie 1.1.3. Le hotkey doit toujours
  être consommé afin que son action D2R concurrente ne s'exécute pas, mais le
  stash doit conserver le panneau Inventory compagnon requis par son shell
  desktop natif.
- La 1.1.4 retire entièrement le hook `UI_OpenInterfaceState 0xCD7C0`, son
  ticket de 150 ms, sa politique et ses compteurs. La capture clavier/souris
  retourne toujours la valeur de consommation configurée et le dispatch
  RemoteStash reste inchangé. Le plugin ne filtre donc plus aucun état UI
  natif pendant l'ouverture.
- Le build Release x64 et CTest passent `2/2`. Les DLL du build, du package et
  du runtime global sont byte-identiques : version `1.1.4`, taille `176128`,
  SHA-256 `A645DECCDAFE4583FD13249AA7D5877064E284F3D3E385FDC91CF853187FE7FA`.
- La synchronisation globale ne remplace pas `RemoteStash.json`; la
  configuration de validation reste `enabled=true`, `hotkey=H` et
  `consume=true`.
- Le cold start global charge RemoteStash 1.1.4 sur D2R `3.2.92777`, ne pose
  aucun hook à `0xCD7C0`, applique `18/18` patchsets, charge `14/14` plugins
  sans rejet ni échec et atteint l'étape frontend `24/24`.
- Le gameplay reste `not run`. Il doit confirmer le shell stash + Inventory
  complet, une seule pression en déplacement, l'absence de l'action D2R liée
  au même hotkey, le maintien pendant un clic de déplacement, puis les
  fermetures par hotkey, Escape et bouton X.

## Fermeture post-composition de l'inventaire — candidate 1.1.5 — 8 août 2026

- Le témoin gameplay 1.1.4 confirme que la composition native complète est
  propre et que la fermeture ultérieure de l'inventaire ne déforme pas le
  shell du stash. La 1.1.5 conserve donc toute l'ouverture native au lieu de
  restaurer le filtre cassé de la 1.1.3.
- Une ouverture au hotkey vérifie d'abord l'état Inventory `1`. S'il était
  fermé, elle arme un ticket one-shot de 2000 ms. Le bouton d'inventaire
  n'arme jamais ce ticket, et un Inventory déjà ouvert reste ouvert.
- Le hook `UI_OpenInterfaceState 0xCD7C0` appelle toujours l'original en
  premier. Après la construction effective du stash `0x18`, il consomme le
  ticket, ferme l'état Inventory `1` avec `UI_CloseInterfaceState 0xC7D30`,
  puis appelle `MarkUiDirty 0x843FC0`. Aucun état d'ouverture n'est supprimé.
- Un ticket périmé ou une session annulée est effacé sans agir. Des compteurs
  distinguent les tickets armés, les fermetures effectuées et les expirations.
- Le build Release x64 et CTest passent `2/2`. Les DLL du build, du package et
  du runtime global sont byte-identiques : version `1.1.5`, taille `177664`,
  SHA-256
  `76D3FA91A7E1FE644CA4E19861C6C644449DCBBF370FF9165A360EC7B914A3C0`.
- La synchronisation globale ne remplace pas `RemoteStash.json`; la
  configuration de validation reste `enabled=true`, `hotkey=H` et
  `consume=true`.
- Le cold start global charge le nouveau hook post-composition à `0xCD7C0`,
  applique `18/18` patchsets, charge `14/14` plugins sans rejet ni échec et
  atteint l'étape frontend `24/24`. Le journal RemoteStash frais ne contient
  aucune erreur ni alerte autre que les annonces normales d'installation des
  hooks.
- Le gameplay reste `not run`. Il doit confirmer l'absence visuelle de
  l'inventaire quand celui-ci était fermé, la conservation d'un inventaire
  déjà ouvert, puis les ouvertures par bouton et les fermetures normales.

## Transition hotkey fluide et fermeture indépendante — candidate 1.1.6 — 8 août 2026

- Le gameplay 1.1.5 confirme que le hotkey ouvre enfin le stash seul. Deux
  écarts restent ciblés : l'ouverture native du stash interrompt un déplacement
  déjà engagé, et sa fermeture au hotkey ferme aussi un Inventory qui était
  ouvert indépendamment.
- `CLIENT_ApplyUiPacket77` continue d'ouvrir le stash par l'action native
  `0x10`; RemoteStash n'envoie aucun ordre de mouvement. La branche de l'état
  stash `0x18` dans le répartiteur `0xC1E80` appelle la transition UI
  `0x11FB80(2, true)` depuis l'unique callsite `0xC1F01`. La signature de 20
  octets de la cible et l'appel relatif de 5 octets sont uniques dans D2R
  3.2.92777.
- La 1.1.6 redirige uniquement ce callsite. Un ticket hotkey one-shot de 2000 ms
  arme une portée thread-local autour de l'ouverture native `0x18`; dans cette
  portée seulement, l'appel devient `0x11FB80(2, false)`. Tous les autres appels
  et toutes les ouvertures non hotkey conservent leurs arguments natifs.
- Lors d'une fermeture par hotkey, le plugin photographie l'état Inventory `1`,
  ferme normalement le stash `0x18`, puis rouvre Inventory dans le même cycle
  UI seulement si celui-ci était ouvert avant et a été fermé comme compagnon.
  Un Inventory initialement fermé demeure fermé. Escape et le bouton X ne sont
  pas redéfinis par cette restauration.
- Le callsite `0xC1F01` et la cible `0x11FB80` ne sont référencés par aucun
  composant du snapshot PluginPack épinglé
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`. La DLL reste autonome et ses
  dépendances PE sont limitées aux bibliothèques Windows et MSVC habituelles.
- Le build Release x64 et CTest passent `2/2`. La DLL candidate du package porte
  la version `1.1.6`, mesure `179200` octets et son SHA-256 est
  `6C6691045810A11E7E0544FB0BDB05C68F1AC09476B1F4507DF067FC99511FC4`.
- Le runtime reste `not run`. Il doit vérifier le déplacement continu pendant
  ouverture/fermeture, l'ouverture visuelle du stash seul, la conservation d'un
  Inventory indépendant à la fermeture, puis les régressions bouton, Escape,
  drag-and-drop, Ctrl-clic, onglets Shared/custom, save-and-exit et reconnexion.

## Fenêtres indépendantes pendant le déplacement — release 1.1.7 — 8 août 2026

- Le gameplay 1.1.6 confirme l'ouverture fluide du stash seul et les fermetures
  indépendantes, mais un clic de déplacement ferme encore Inventory quand les
  deux panneaux sont visibles. Une première sonde observe le retour générique
  `0xC84CB`; la pile complète prouve ensuite la chaîne
  `0x102590 -> 0xC8240 -> 0xC84C6` propre au close cascade du clic dans le monde.
- `0xC8240` construit la liste des états UI actifs puis appelle
  `UI_CloseInterfaceState` pour chacun. RemoteStash redirige uniquement l'appel
  relatif exact `E8 AB 5C FC FF` à `0x102590` et arme une portée thread-local
  pendant cette cascade seulement.
- Dans cette portée, l'état Inventory `1` est conservé uniquement si la session
  distante est active et si l'état stash `0x18` est encore ouvert. Les hotkeys,
  boutons X, Escape et autres fermetures explicites ne passent pas par cette
  portée et conservent leur comportement natif.
- Le build Release x64 et CTest passent `2/2`. Le build, le package et le
  runtime global sont byte-identiques : version `1.1.7`, taille `179200`,
  SHA-256
  `31D037043EAC49DA10E2736A2D7DBE075220C1A20C55451E2B0164DDD5E34499`.
- Le cold start global BKVince avec toute la pile active applique `18/18`
  patchsets, charge `14/14` plugins sans rejet ni échec et atteint `24/24`.
  Vincent confirme ensuite en jeu que le déplacement continue, que RemoteStash
  et Inventory restent tous deux visibles, et que `H` et `I` ferment seulement
  leur propre panneau.
- L'archive publique stricte `RemoteStash-1.1.7.zip` contient uniquement
  `d2rloader/plugins/RemoteStash.dll` et
  `d2rloader/config/RemoteStash.json`. Elle mesure `79774` octets et son SHA-256
  est `BCC5F1C933E3323C2B2ACC3ECE073581F5D50175449438BB62894AE9F03F31CC`.
  Le JSON public validé conserve `enabled=false`, `hotkey=S`, `consume=true` et
  son SHA-256 est
  `A1F59CB1BCAF2BBD23E20249992B99E90D9803870FCE4B3D622A7678BD854616`.

## Préservation du clic maintenu pendant l'ouverture — candidate 1.1.9 — 11 août 2026

- Le témoin gameplay 1.1.8 invalide la première protection : maintenir le clic
  gauche, ouvrir RemoteStash avec `;`, puis continuer le déplacement exige
  encore un nouveau clic. Le statut runtime prouve pourtant une demande
  acceptée et dispatchée, un ticket de transition consommé, une transition
  appliquée et une fermeture de l'Inventory compagnon. La DLL, le hotkey et le
  ticket ne sont donc pas périmés ou désynchronisés.
- La branche native du stash et la fermeture post-composition de l'Inventory
  appellent toutes deux `CLIENT_ResetMouseButtonState 0x8D510` par
  `UI_ApplyInterfaceLayoutMode 0xB9C20`. La 1.1.8 protégeait seulement le
  premier appel : elle restaurait sa portée thread-local avant de fermer
  l'Inventory compagnon avec `UI_CloseInterfaceState 0xC7D30`, laissant ce
  second appel effacer le clic maintenu.
- La 1.1.9 conserve désormais la même portée thread-local jusqu'après la
  fermeture post-composition. Aucun état brut de souris n'est restauré et
  aucune destination de mouvement n'est synthétisée : les deux transitions
  natives restent exécutées, mais leur remise à zéro commune est supprimée
  uniquement pendant cette ouverture au hotkey.
- Le build Release x64 et CTest passent `2/2`. Les DLL du build et du package
  sont byte-identiques : version `1.1.9`, taille `182272`, SHA-256
  `C332216A02D45B0308B45641CAD3525B498D7754A186785F9074744252B38A19`.
- La DLL candidate est déployée dans la portée globale avec un hash byte-exact;
  le JSON existant est préservé (`enabled=true`, `hotkey=;`, `consume=true`). Le
  cold start BKVince avec toute la pile active charge RemoteStash 1.1.9 et son
  hook `0x8D510`, applique `15/15` patchsets, charge `19/19` plugins avec
  `rejected=0` et `failed=0`, puis atteint l'étape frontend `24/24`.
- Le gameplay reste `not run`. Le prochain témoin doit maintenir le clic gauche,
  ouvrir puis fermer RemoteStash avec `;`, confirmer que le personnage continue
  vers la destination courante sans nouveau clic, puis vérifier dans la console
  que deux remises à zéro ont été supprimées pour une ouverture avec Inventory
  initialement fermé.

## Diagnostic du clic maintenu — candidate 1.1.10 — 11 août 2026

- Le témoin gameplay 1.1.9 invalide la portée élargie : le personnage s'arrête
  encore après l'ouverture au hotkey et exige un nouveau clic. Le statut prouve
  une demande acceptée et dispatchée, une fermeture de l'Inventory compagnon et
  une transition appliquée, mais sa ligne de 1 600 caractères est tronquée par
  la console exactement avant la valeur de `hotkeyMouseResetSuppressions`.
- La 1.1.10 ne change aucun comportement du stash. La commande `remote-stash`
  imprime désormais une seconde ligne courte et non ambiguë :
  `RemoteStash input: hotkeyMouseResetSuppressions=<nombre>`.
- Cette valeur décidera la prochaine branche d'analyse : `2` prouve que les
  deux resets connus sont supprimés et impose de chercher un autre mécanisme;
  `1` prouve qu'un reset échappe encore à la portée; `0` invalide le trajet de
  hook attendu.
- Le build Release x64, CTest `2/2` et le self-test du workbench passent. Les
  DLL du build et du package sont byte-identiques : version `1.1.10`, taille
  `182272`, SHA-256
  `54896479F8FBBE248CE25BFFADB56C2C77FFEBA1E3A97176DC5F62233EA5ABF1`.
  Aucun runtime n'a été lancé ou modifié à cette étape.

## Restauration ciblée du clic maintenu — candidate 1.1.11 — 11 août 2026

- Le témoin gameplay 1.1.10 confirme encore l'arrêt du personnage, tandis que
  la ligne courte rapporte `hotkeyMouseResetSuppressions=2`. Les deux passages
  connus par `CLIENT_ResetMouseButtonState 0x8D510` sont donc réellement
  interceptés; cette hypothèse est invalidée comme cause suffisante.
- L'analyse du build 92777 identifie `0x8D540`, une seconde routine qui lit
  l'index du contexte d'entrée, efface les mêmes six globals de souris, pose
  deux autres états à `0x10`, puis rejoint le finalizer indexé `0xF15C0`. Sa
  signature stricte de 32 octets est unique et l'index recense 16 xrefs.
- La candidate 1.1.11 hooke cette seconde entrée uniquement lorsque la portée
  thread-local de l'ouverture RemoteStash au hotkey est active. Elle capture
  les six globals partagés, exécute intégralement la routine originale et son
  finalizer, puis restaure seulement ces six valeurs. Les deux états à `0x10`
  et la finalisation native restent intacts; aucun mouvement ou clic n'est
  synthétisé.
- La commande `remote-stash` ajoute le compteur court
  `hotkeyMouseStateRestorations`. Le témoin gameplay attendu est une valeur non
  nulle pendant l'ouverture et la poursuite du déplacement sans nouveau clic.
- Le build Release x64 et CTest passent `2/2`; le self-test du workbench 92777
  passe et `git diff --check` ne rapporte aucune erreur. Les DLL du build et des
  deux emplacements du package sont byte-identiques : version `1.1.11`, taille
  `182784`, SHA-256
  `F4A30F70399735075DEDDEA64AFEE771AE701DE6E944A24995696552A2AF4C6A`.
- La même DLL est déployée byte-exact dans la portée globale; le JSON existant
  reste inchangé (`enabled=true`, `hotkey=;`, `consume=true`). Le cold start
  BKVince avec toute la pile active accepte les hooks `0x8D540` et `0x8D510`,
  applique `15/15` patchsets, charge `19/19` plugins avec `rejected=0` et
  `failed=0`, puis atteint l'étape frontend `24/24`.
- Témoin gameplay confirmé par Vincent : pendant un déplacement au clic gauche
  maintenu, l'ouverture de RemoteStash avec `;` laisse le personnage poursuivre
  son mouvement sans relâcher ni cliquer de nouveau. La fluidité recherchée est
  donc validée en jeu pour la candidate 1.1.11.

## Release publique 1.2.0 — 11 août 2026

- Vincent retient `1.2.0` comme version publique officielle du comportement
  validé en jeu dans la candidate 1.1.11. Aucun hook, protocole, layout ou
  comportement gameplay n'est remanié par cette promotion de version.
- Le ZIP public autonome conserve l'allowlist stricte : `RemoteStash.dll` et
  `RemoteStash.json` seulement. Le README avec les crédits RuffnecKk/D2MOO et
  les fichiers d'intégration destinés aux moddeurs restent dans le dépôt, hors
  de l'archive publique.
- Le build Release x64, CTest `2/2`, le self-test 92777 et `git diff --check`
  passent. Les DLL build/package/runtime sont byte-identiques : version `1.2.0`,
  taille `182784`, SHA-256
  `F8FE4FF361825AE2912F368AC503844D01CB6D452F17E3260BE5D51866274A3C`.
- Le runtime global conserve son JSON existant (`enabled=true`, `hotkey=;`,
  `consume=true`). Le cold start BKVince avec toute la pile active accepte les
  hooks `0x8D540` et `0x8D510`, applique `15/15` patchsets, charge `19/19`
  plugins avec `rejected=0` et `failed=0`, puis atteint le frontend `24/24`.
- `RemoteStash-1.2.0.zip` contient exactement
  `d2rloader/plugins/RemoteStash.dll` et
  `d2rloader/config/RemoteStash.json`. L'extraction confirme la DLL distribuée
  byte-identique; le JSON public conserve les valeurs sûres
  `enabled=false`, `hotkey=S`, `consume=true`. Taille du ZIP `81488`, SHA-256
  `E945FE34EF8C652FA5C2CBC6812DE76A86AA09AF735A6EF00A4AA433C2B616C5`.
- L'ajout structurel de l'archive a régénéré le cadastre; son validateur retourne
  `VALID`. Le comportement gameplay 1.2.0 est la promotion sans changement de
  la candidate 1.1.11 déjà confirmée par Vincent.

## Nettoyage du chemin critique UI — candidate 1.2.1 — 12 août 2026

- Un test gameplay prolongé de la 1.2.0 révèle des chutes ponctuelles de
  framerate lors de l'ouverture et de la fermeture du Remote Stash. Le
  comportement fonctionnel demeure correct, mais la 1.2.0 n'est donc pas
  retenue comme preuve finale de fluidité.
- L'audit retrouve un hook diagnostique temporaire sur
  `QueueSerializedPacket 0xEE360`. Tant que le stash était visible, ce hook
  inspectait chaque paquet sérialisé et pouvait construire puis écrire jusqu'à
  256 traces `GOLD-PROBE` synchrones. Le handler gold écrivait aussi une trace
  `GOLD-SERVER-PROBE`; les ouvertures et fermetures au hotkey produisaient
  plusieurs autres `LogInfo` depuis leur chemin UI.
- La 1.2.1 retire entièrement le hook `0xEE360`, ses signatures, compteurs et
  traces temporaires. Elle retire aussi les logs routiniers d'ouverture,
  fermeture et création de session. Les erreurs, les compteurs consultables par
  `remote-stash`, les transferts d'or et toute la logique de mouvement continu
  restent inchangés.
- Le build Release x64 et CTest passent `2/2`; le self-test du workbench 92777
  passe et `git diff --check` ne rapporte aucune erreur. La DLL candidate porte
  la version `1.2.1`, mesure `180224` octets et son SHA-256 est
  `FB0F678934E4A51F35C6D253985606EA0EC531FE4922F4A6EB02D1A0FB10FC13`.
- Le gameplay reste `not run`. Le prochain témoin doit comparer plusieurs
  ouvertures et fermetures au hotkey, puis confirmer que le clic gauche maintenu
  continue encore le déplacement et que les transferts d'items et d'or restent
  fonctionnels.
- La DLL candidate est installée byte-identique dans la portée globale; la
  configuration existante est préservée. Aucun cold start ni lancement gameplay
  n'est effectué automatiquement et aucun ZIP public 1.2.1 n'est produit avant
  le témoin de fluidité.

## Arbitrage natif des panneaux — candidate 1.2.2 — 12 août 2026

- Vincent confirme que la 1.2.1 élimine les chutes de framerate observées à
  l'ouverture et à la fermeture. Le test révèle toutefois que le Remote Stash
  refuse aussi les fermetures demandées par Quest, Character et Waypoint : le
  panneau stash demeure alors dominant au lieu de céder sa place selon
  l'arbitrage UI natif.
- La cause est la politique générale de `HookCloseInterfaceState`, qui
  supprimait toute fermeture de l'état stash `0x18` pendant une session distante
  sauf bouton X, Escape ou réponse serveur. Cette condition bloquait donc une
  décision native valide sans distinguer son origine.
- La 1.2.2 limite désormais cette suppression à la portée thread-local déjà
  prouvée pour la cascade exacte du clic dans le monde
  `0x102590 -> UI_CloseActiveInterfaceStates 0xC8240 -> UI_CloseInterfaceState`.
  Les fermetures demandées par un autre panneau repassent dans le gestionnaire
  natif; aucune liste de panneaux et aucun nouveau hook ne sont ajoutés.
- Lorsqu'une fermeture native fait réellement tomber l'état stash à zéro, la
  session RemoteStash cliente est désactivée et une seule requête de fermeture
  est envoyée au serveur. Une réponse de fermeture déjà initiée par le serveur
  ne produit pas de paquet retour, et la fermeture au hotkey demeure sans
  doublon parce qu'elle désactive la session avant la transition UI.
- Le build Release x64 et CTest passent `2/2`; le self-test du workbench 92777
  passe et `git diff --check` ne rapporte aucune erreur. La DLL candidate porte
  la version `1.2.2`, mesure `180224` octets et son SHA-256 est
  `38BE20C2CC1EB19FDC769B416FB71D2425EC279369EEBBB3187CFFB625822FCA`.
- Le gameplay 1.2.2 reste `not run`. Le témoin doit vérifier Quest, Character et
  Waypoint comme remplacements du Remote Stash, puis reconfirmer la persistance
  du stash pendant le déplacement, la coexistence avec Inventory, le hotkey,
  Escape, le bouton X et les transferts d'items et d'or.

## Repli d'ouverture des panneaux concurrents — candidate 1.2.3 — 12 août 2026

- Le gameplay invalide entièrement la 1.2.2 pour la livraison : le hotkey ouvre
  de nouveau Inventory, le mouvement continu est interrompu et Quest/Character
  ne remplacent toujours pas le stash. Le Waypoint, en revanche, remplace
  correctement RemoteStash; cette différence prouve que sa fermeture explicite
  serveur fonctionne déjà et ne doit pas recevoir de traitement particulier.
- L'audit confirme que le callsite `0x102590` appartient bien à une action dans
  le monde. Quest et Character ne sont donc pas bloqués par la portée de
  mouvement : leur première demande d'ouverture est refusée tant que le stash
  distant occupe le groupe UI concurrent.
- La 1.2.3 restaure la politique de fermeture éprouvée de la 1.2.1, y compris
  l'ouverture du stash seul et la continuité du clic maintenu. Dans
  `UI_OpenInterfaceState`, un repli borné s'applique uniquement si une session
  RemoteStash est active, si le stash est réellement ouvert, si le panneau
  demandé n'est ni Stash ni Inventory et si D2R confirme que cette première
  demande n'a pas ouvert le panneau.
- Ce repli ferme alors la session distante, laisse fermer le stash avec la
  fonction native et rejoue exactement une fois la demande d'ouverture
  originale. Un Waypoint ou tout panneau qui réussit du premier coup ne passe
  jamais par ce chemin; aucun identifiant Quest/Character/Waypoint n'est codé en
  dur et les panneaux de mods utilisant l'arbitrage natif bénéficient du même
  comportement.
- Le chemin général retourne avant toute lecture UI supplémentaire lorsque la
  session distante est inactive ou que Stash/Inventory est demandé. Aucun
  nouveau hook, polling ou log synchrone n'est ajouté. Deux compteurs silencieux
  mesurent seulement les retries et leurs échecs dans la commande de statut.
- Le build Release x64 et CTest passent `2/2`; le self-test du workbench 92777
  passe et `git diff --check` ne rapporte aucune erreur. La DLL candidate porte
  la version `1.2.3`, mesure `180736` octets et son SHA-256 est
  `40822F957D65F73F12CCF75CAA972C6781114A0B6CDA6AC4A3FB1C80BC938191`.
- Le gameplay 1.2.3 reste `not run`. Il doit d'abord reconfirmer les trois
  garanties 1.2.1 — stash seul, mouvement continu et fermeture indépendante —
  puis Quest, Character et Waypoint comme remplacements.

## Cession explicite et ouverture Inventory fluide — candidate 1.2.4 — 12 août 2026

- Le témoin gameplay 1.2.3 confirme que RemoteStash et Inventory ont retrouvé
  leurs ouvertures et fermetures autonomes et que Waypoint remplace correctement
  le stash. Quest et Character restent toutefois derrière le stash. Ouvrir
  Inventory pendant un déplacement avec RemoteStash visible interrompt aussi
  le clic gauche maintenu.
- Le critère de repli 1.2.3 était incorrect : D2R marque Quest ou Character
  ouvert même lorsque le stash demeure visuellement dominant. Tester seulement
  `UI_GetInterfaceState` après la première demande ne permet donc pas de détecter
  cet échec d'arbitrage.
- La 1.2.4 retire ce retry et cède explicitement la session distante avant
  l'ouverture de Character (`2`), Skill Tree (`4`) ou Quest (`0x0F`). Ces états
  sont corroborés par les callsites du build 92777 et la nomenclature sémantique
  D2MOO. Waypoint (`0x14`) conserve son trajet natif déjà validé; Inventory (`1`)
  reste volontairement exclu afin de pouvoir coexister avec RemoteStash.
- Lorsque Inventory est ouvert pendant une session distante visible, la
  transition réutilise exactement la protection thread-local déjà prouvée pour
  l'ouverture au hotkey : les remises à zéro de souris `0x8D510` sont supprimées
  et les six globals effacés par `0x8D540` sont restaurés après son finalizer.
  Aucun clic, mouvement, polling ou hook supplémentaire n'est synthétisé.
- Le build Release x64 et CTest passent `2/2`; le self-test du workbench 92777
  passe et `git diff --check` ne rapporte aucune erreur. Les DLL du build, des
  deux emplacements du package et du runtime global sont byte-identiques :
  version `1.2.4`, taille `181248`, SHA-256
  `92D7652C002A47B86A05E7FA7D0B57AB67864A60AFB5BDA3A7431EBCA7355AC3`.
- Le JSON global est préservé (`enabled=true`, `hotkey=;`, `consume=true`) et
  aucun jeu n'est relancé automatiquement. Le gameplay 1.2.4 reste `not run` :
  valider Quest, Character et Skill Tree comme remplacements, Waypoint sans
  régression, puis l'ouverture d'Inventory pendant le clic gauche maintenu avec
  les deux panneaux autonomes.

## Instrumentation ciblée des arbitrages UI — candidate 1.2.5 — 12 août 2026

- Le test gameplay 1.2.4 invalide la candidate : Quest et Character ne
  remplacent toujours pas RemoteStash, tandis que Waypoint le remplace.
  Ouvrir Inventory pendant un clic gauche maintenu avec RemoteStash visible
  interrompt encore le mouvement. L'autonomie d'ouverture et de fermeture de
  RemoteStash et Inventory demeure fonctionnelle.
- La 1.2.5 ne modifie pas cette politique d'arbitrage. Elle ajoute une
  instrumentation bornée, armée seulement après une ouverture distante, sur
  Inventory (`1`), Character (`2`), Skill Tree (`4`), l'état alternatif
  observé `0x0E`, Quest (`0x0F`), Waypoint (`0x14`) et Stash (`0x18`).
- Chaque trace compare avant/après les états UI `0x18`, `0x16`, Inventory et
  le panneau demandé, ainsi que l'état de la session distante, le résultat de
  l'ouverture et le RVA de l'appelant. Les désactivations de session consignent
  maintenant leur véritable appelant. Les plafonds sont de 32 ouvertures et
  16 désactivations par ouverture distante.
- Le build Release x64 et CTest passent `2/2`. Les DLL du build, des deux
  emplacements du package et du runtime global sont byte-identiques : version
  `1.2.5`, taille `183296`, SHA-256
  `D50283E5EADD12507F35A5383AF13A5B4F108F314A95CC372617C1ABF7C20B5E`.
- Le JSON global reste inchangé, avec le hotkey `;`; D2RLoader a été arrêté
  pour le déploiement et n'a pas été relancé. Le gameplay 1.2.5 reste
  `not run`; reproduire successivement Quest, Character, Waypoint puis
  Inventory pendant un clic gauche maintenu et collecter les lignes
  `RemoteStash DIAG` du log.

## Instrumentation du distributeur natif des panneaux — candidate 1.2.6 — 12 août 2026

- Le test 1.2.5 a confirmé que Quest, Character, Waypoint et Inventory ne
  repassent pas par `UI_OpenInterfaceState` lorsque leurs panneaux existent
  déjà. La sonde précédente ne pouvait donc pas observer ces activations.
- L'atelier gouverné du build 92777 identifie dix appels directs à
  `UI_DispatchMessage` dans le gestionnaire natif des panneaux, aux RVA
  `0x244A09`, `0x244EA6`, `0x245F6C`, `0x24649B`, `0x246930`, `0x246DB0`,
  `0x247140`, `0x247633`, `0x247E64` et `0x248479`.
- La 1.2.6 redirige uniquement ces dix appels vers une sonde transparente :
  le message original traverse la chaîne de coexistence inchangée, puis une
  trace bornée compare avant/après les états Stash, Inventory, Character,
  Skills, `0x0E`, Quest et Waypoint. Aucun nouvel arbitrage ou close n'est
  appliqué par cette candidate diagnostique.
- La validation gameplay doit ouvrir RemoteStash puis activer Quest,
  Character, Waypoint et Inventory. Les lignes
  `RemoteStash DIAG panel-dispatch` permettront d'identifier le callsite et
  l'état réellement associé à chaque commande avant d'implanter le correctif.
- Le cold start global du 12 août charge la 1.2.6 avec le PluginPack comme
  propriétaire du broker UI, le JSON global `hotkey=;` et aucune erreur de
  hook. Le build et les deux tests CTest passent; la DLL déployée mesure
  `185344` octets et porte le SHA-256
  `83A1022FB4FA1975AA8067C30E6C4EFB2D83134B2EA0369FBD0793A1C7A6B2C7`.
- Vincent a reproduit successivement Quest, Character, Waypoint et Inventory
  après l'ouverture distante. Aucune ligne `DIAG panel-dispatch` n'a été
  produite : les dix callsites instrumentés ne sont donc pas le chemin des
  commandes réellement utilisées en jeu. Cette hypothèse est invalidée; la
  1.2.6 ne constitue pas un correctif gameplay.
- Le prochain diagnostic recommandé vise le gestionnaire des panneaux déjà
  créés à `0x27F2B0` et ses sept xrefs gouvernées. Cette fonction maintient la
  liste des états du panneau, ferme l'état courant via `UI_CloseInterfaceState`
  et choisit le nouvel état; elle est une convergence plus basse que les
  branches de distribution invalidées par le test 1.2.6.

## Arbitrage central des panneaux existants — candidate 1.2.7 — 12 août 2026

- L'hypothèse finale de la 1.2.6 était encore trop haute dans la pile. Le
  désassemblage gouverné identifie `UI_ToggleInterfaceState 0xCDE00` comme la
  route centrale des panneaux déjà créés : ABI observée
  `(int32 state, bool secondary) -> bool`, signature stricte unique de 68
  octets, puis appel au garde commun `0xD00B0` avec l'opération toggle `2`.
- La 1.2.7 retire entièrement les dix sondes `UI_DispatchMessage` invalidées.
  Dans le hook central toggle, Character (`2`), Skill Tree (`4`) et Quest
  (`0x0F`) désactivent la session distante et ferment nativement le stash
  `0x18` avant que D2R exécute exactement la transition demandée. Waypoint
  conserve son chemin serveur déjà fonctionnel et Inventory (`1`) reste
  autorisé à coexister.
- Quand Inventory `1` est fermé et qu'il est togglé pendant qu'un RemoteStash
  est visible, la 1.2.7 applique uniquement autour de ce toggle la protection
  thread-local déjà validée contre les remises à zéro du clic maintenu. Une
  fermeture Inventory ou toute transition sans session distante reste native.
- Le build Release x64 et CTest passent `2/2`; le self-test du workbench 92777
  passe et `git diff --check` ne rapporte aucune erreur. La preuve stable de
  `UI_ToggleInterfaceState 0xCDE00` est promue dans `known-rvas.json`.
- Le gameplay 1.2.7 reste `not run`. Il doit confirmer Quest, Character et
  Skill Tree comme remplacements, Waypoint sans régression, puis Inventory
  ouvert pendant un clic gauche maintenu avec ouverture et fermeture autonome
  des deux panneaux.

## Cession par la matrice native de compatibilité — candidate 1.2.8 — 12 août 2026

- Le gameplay 1.2.7 prouve que le hook `UI_ToggleInterfaceState 0xCDE00`
  fonctionne pour Inventory : ouvrir Inventory avec RemoteStash visible ne
  coupe plus le clic de déplacement maintenu. Quest, Character, Skill Tree et
  Waypoint ne remplacent toutefois pas RemoteStash; le compteur de cession
  reste à zéro. Le toggle n'est donc pas leur convergence d'arbitrage.
- Le garde partagé `0xD00B0`, appelé par Close (`0xC7DC0`), Open (`0xCD823`)
  et Toggle (`0xCDE67`), applique la matrice native de compatibilité. Après
  acceptation, il parcourt les 32 états et appelle
  `UI_CloseInterfaceState 0xC7D30` depuis l'unique callsite `0xD05F8` lorsqu'un
  état actif doit céder sa place. La signature exacte `E8 33 77 FF FF` est
  unique dans l'image 92777.
- La 1.2.8 ne code plus aucun identifiant Quest, Character, Skill Tree ou
  Waypoint. Si et seulement si ce callsite natif ordonne la fermeture du stash
  distant `0x18`, le plugin désactive d'abord sa session serveur synthétique,
  laisse la fermeture native s'exécuter, puis compte le résultat. Toute autre
  fermeture automatique demeure protégée comme en 1.2.7. Cette politique
  conserve donc la matrice D2R comme autorité et s'étend aux panneaux de mods.
- Le hook Toggle reste limité à la fluidité Inventory déjà validée. Le build
  Release x64 et CTest passent `2/2`; le self-test du workbench passe et
  `git diff --check` ne rapporte aucune erreur. La DLL candidate mesure
  `181760` octets et porte le SHA-256
  `9923AAFC1C0BC44BFEE23065F582F365D32688732833FF55AB87FA345CE5BBBA`.
- Le gameplay 1.2.8 reste `not run` : reconfirmer la fluidité Inventory, puis
  Quest, Character, Skill Tree et Waypoint comme remplacements de RemoteStash.

## Cession au gestionnaire des panneaux créés — candidate 1.2.9 — 12 août 2026

- Le gameplay 1.2.8 confirme que la fluidité d'Inventory reste correcte, mais
  invalide le callsite `0xD05F8` comme convergence de remplacement : aucun de
  Quest, Character, Skill Tree ou Waypoint ne remplace RemoteStash.
- Le désassemblage gouverné confirme finalement le rôle de `0x27F2B0`. Cette
  entrée reçoit un groupe de panneaux et l'identifiant demandé, recherche cet
  identifiant dans la liste du groupe à `+0x168` / `+0x188`, ferme la sélection
  courante par `UI_CloseInterfaceState`, puis ouvre la nouvelle par
  `UI_OpenInterfaceState`. Ses appels directs passent notamment Character `2`,
  Skills `4` et Quest `0x0E`.
- La 1.2.9 accroche cette entrée unique avec sa signature stricte de 17 octets.
  Si RemoteStash est actif et visible, elle cède la session seulement lorsque
  l'état demandé appartient réellement à la liste du groupe. Inventory `1` et
  Stash `0x18` restent exclus afin de conserver la coexistence et le hotkey.
  Le layout et les coordonnées du mod ne participent pas à cette décision.
- Une trace bornée aux huit premiers cas consigne aussi le RVA d'une fermeture
  stash non liée au mouvement qui serait encore supprimée. Elle doit identifier
  le chemin Waypoint si celui-ci ne traverse pas le gestionnaire `0x27F2B0`.
- Le prochain test doit d'abord reconfirmer la fluidité Inventory, puis essayer
  Character, Skills, Quest et Waypoint comme remplacements. Le gameplay 1.2.9
  confirme la fluidité, mais aucun des quatre remplacements ne fonctionne.

## Cession au teardown général natif — candidate 1.2.10 — 12 août 2026

- La trace bornée de la 1.2.9 capture la fermeture réellement demandée pendant
  les essais de remplacement : `UI_CloseInterfaceState(0x18, false)` revient à
  `0x22A9FE`, donc provient de l'unique callsite `0x22A9F9` dans la routine
  `0x22A7E0`. Cette routine ferme successivement les états `0x18`, `0x19` et
  `0x0B` dans un teardown général de panneaux.
- La politique RemoteStash classait encore ce callsite connu comme une fermeture
  automatique à supprimer. La 1.2.10 le laisse désormais passer uniquement
  hors de `RemoteMovementUiCloseScope`; la cascade de déplacement demeure donc
  protégée, tandis qu'une transition de panneau peut désactiver la session
  distante et effectuer sa fermeture native.
- Le hook expérimental `0x27F2B0` de la 1.2.9 est retiré puisqu'il n'est jamais
  atteint par ce chemin gameplay. La signature unique `E8 32 D3 E9 FF` du
  callsite réellement observé est ajoutée au gate strict du build 92777.
- Validation attendue : la fluidité Inventory doit rester intacte et Character,
  Skills, Quest ainsi que Waypoint doivent chacun remplacer RemoteStash.
- Le build Release x64 et CTest passent `2/2`; le self-test du workbench passe.
  Les trois copies package/runtime sont byte-identiques à la DLL source de
  `181760` octets, SHA-256
  `6752AAFB5CD539F6B493DCC0D14A4E8AB6F9D4A479FBE6EC82D6E53163F2D845`.
  Le cold start charge la 1.2.10 avec le broker PluginPack et le hotkey global
  `;`; la matrice gameplay de remplacement reste `not run` jusqu'au retour de
  Vincent.

## Baseline RuffnecKk D2RLoader Suite — Remote Stash 2.0.0 — 18 août 2026

- Vincent désigne explicitement cette refonte comme le Remote Stash
  autoritaire de son futur dépôt RuffnecKk D2RLoader Suite. La candidate locale
  `1.6.0`, jamais publiée, est donc renumérotée directement `2.0.0`; les mises à
  jour futures partiront de cette baseline.
- La 2.0.0 est générique et ne contient aucune adaptation BKVince. Elle
  enregistre son propre enfant du panneau standard `PlayerInventory`, embarque
  le coffre RuffnecKk et calcule sa position depuis la géométrie du panneau
  réellement chargé. La position, l'ancre, les offsets, les dimensions, les
  quatre frames et des sprites `SpA1` personnalisés sont configurables en TOML.
- Aucun fichier BKVince n'a été modifié pour le test. Son ancien widget
  `remote_stash` et ses sprites externes sont volontairement restés présents :
  le plugin a masqué ce widget au runtime et a conservé le véritable
  `gold_button`. L'ancienne configuration globale 1.5.0 a également été
  conservée, ce qui prouve la migration sans réinitialisation utilisateur.
- Le témoin gameplay de Vincent sur l'implantation finale, alors numérotée
  localement 1.6.0, est `passed` : le bouton physique ouvre Remote Stash, aucun
  modal Drop Gold parasite n'apparaît, le transfert d'or fonctionne et le
  bouton d'or natif conserve son comportement. Les logs du 18 août montrent le
  masquage de l'ancien widget, le placement du bouton plugin à `95,1641` en
  `176x112`, l'activation SDK du bouton et l'ouverture de la session serveur.
- Le renommage 2.0.0 ne modifie que les métadonnées et textes de version. Deux
  builds Release propres avec avertissements fatals sont byte-identiques; les
  deux tests ciblés passent. La DLL mesure `538112` octets et porte le SHA-256
  `D21E80D45398356604CAED4DD724A182AB117F62BC527F0BE964FCEE8A82AB1A`.
  Les contrôles de politique de la Suite et de propriété des écritures natives
  sont `VALID`.
- L'archive publique stricte
  `RuffnecKk-remote-stash-v2.0.0.zip` contient uniquement
  `plugins/d2rl-ruffneckk-remote-stash.dll`; son SHA-256 est
  `519C9B48245324C156E4C363BCCE34A4B4BE0347D7F18F82FDC1B261D8DEA785`.
  Le README anglais reste à côté du ZIP et hors de l'archive, SHA-256
  `B8057F006EC074316B6815AADB58937823479C0DC82EB03BBA9D7F41181745DF`.
- Le runtime global porte exactement la DLL 2.0.0 testée. Le cold start du
  18 août accepte D2R `3.2.92777`, charge Remote Stash 2.0.0 avec le bouton
  plugin et le sprite embarqué, puis termine `24/24` avec `26 plugins` et
  `19 memory patches`; les cinq plugins eezstreet restent actifs.
- La matrice gameplay propre au binaire renuméroté 2.0.0 reste formellement
  `not run`, même si son code fonctionnel est identique au témoin réussi. Le
  placement personnalisé, le sprite personnalisé, le repli de sprite invalide
  et les parcours multijoueur hôte/client restent également `not run` au
  runtime; leurs politiques et bornes sont couvertes statiquement.
- Retour arrière exact : arrêter D2R/D2RLoader et restaurer la DLL 1.6.0
  sauvegardée sous
  `analysis-cache/runtime-backups/remote-stash-1.6.0-before-2.0.0-20260818/`,
  SHA-256
  `FC594437F1363590EF0DD8C7920D85BAFE1953F3E80CC53560CBF1B7B9140DE4`.

## Panel distant gouverné par D2RLoader — preuve de concept — 20 août 2026

- Vincent exige que l'ouverture d'un coffre physique conserve sans changement
  le comportement vanilla. Seule une session explicitement ouverte par Remote
  Stash doit utiliser le cycle de vie d'un panel D2RLoader comparable à Charm
  Inventory.
- L'inspection de Charm Inventory 0.19.0 confirme un panel enregistré auprès du
  `PanelServiceV1`, avec une composition `GameplayLeftSlot`; Remote Stash 2.0.1
  utilise encore l'état natif du stash `0x18`, ce qui explique son caractère
  dominant et le blocage de commandes de gameplay comme Show Items.
- La preuve de concept autorisée doit enregistrer un `BankPanel` propre au
  plugin, basé sur `BankExpansionLayoutHD.json` afin de réutiliser le layout du
  mod actif. L'ouverture `0x18` n'est détournée que lorsque la session cliente
  distante est active; toute ouverture physique continue d'appeler le chemin
  natif original.
- Le premier gate gameplay doit prouver : coffre physique inchangé, Remote
  Stash visible avec les tabs du mod actif, Show Items disponible, remplacement
  normal par les panels de gameplay, hotkey de fermeture fonctionnel et aucun
  changement aux transactions Personal/Shared. Une impossibilité d'instancier
  correctement le `BankPanel` dérivé invalidera le PanelService v1 comme
  solution suffisante et imposera une route de stock dédiée dans D2RLoader.
- Retour arrière : restaurer la DLL globale Remote Stash 2.0.1 conservée avant
  le déploiement de la candidate; aucun fichier de données BKVince ni aucune DLL
  tierce ne doit être modifié.

## Limite confirmée de PanelService v1 — 20 août 2026

- Les candidates expérimentales 2.1.0 et 2.1.1 ont invalidé la dérivation
  directe de `BankExpansionLayoutHD.json`. Avec l'ouverture native du stash,
  D2R réactive le cycle modal de l'état `0x18`; sans le paquet natif
  d'ouverture, le `BankPanel` apparaît sans modèle de stash utilisable. Le
  bouton physique et le hotkey aboutissent alors à un panneau incomplet et ne
  reproduisent pas Charm Inventory.
- La différence avec Charm Inventory est maintenant établie : ce plugin possède
  une page d'inventaire personnalisée que `PanelServiceV1::bindPlayerPageGrid`
  sait lier. Remote Stash doit au contraire composer le modèle et les onglets du
  stash natif déjà résolus par le mod actif; aucune opération équivalente
  n'existe dans `PanelServiceV1`.
- Le SDK officiel courant ne publie que `PanelServiceV1` version `1`, taille
  `96`, dont la dernière opération est `unregisterControllerRoute`. Le
  `D2RCore.dll` 1.1.0-beta installé ne révèle aucune V2 ni liaison stock-stash,
  et le code source du fournisseur `D2RCore` n'est présent ni dans le workspace
  ni dans les dépôts publics de l'organisation D2RLoader. Modifier uniquement
  le header du SDK créerait donc une ABI fictive que le loader installé ne peut
  pas exécuter.
- La solution propre exige une extension versionnée fournie par D2RLoader — une
  V2 ou un service séparé — capable d'initialiser et de composer le stash stock
  dans un panel plugin sans activer le cycle modal `0x18`. Cette route doit
  préserver les layouts et onglets du mod actif, l'arbitrage UISwitcher, Escape,
  clavier/souris et manette, tout en laissant le coffre physique entièrement
  vanilla. Remote Stash pourra ensuite consommer cette API de manière
  autonome, refuser proprement une ABI incompatible et conserver ses sessions
  de transfert existantes.
- L'implantation côté Remote Stash est bloquée par cette capacité manquante du
  fournisseur, et non par une dépendance envers un autre plugin. Aucun nouveau
  hook natif de contournement n'est retenu. Les changements source des
  candidates 2.1.x ont été entièrement retirés. Le runtime global a été remis
  sur l'artefact pré-POC stable (métadonnées 2.0.0), SHA-256
  `9431884405F571DC3E3A5467AF359EF2957A39B2A0091BCF93896EADEE6CFACD`,
  sans relancer le jeu.

## Correctifs d'interaction Remote Stash 2.0.3 — candidate statique — 28 août 2026

- Vincent autorise par `GO` le portage dans la source RuffnecKk des correctifs
  décrits par `RemoteStash-hotkey-item-interaction-bug-report.md`. La DLL externe
  analysée, SHA-256
  `5740D19108C53E4DF47CDA0108B96681615E2D463C55369676F24802EC9BC0AE`,
  contient bien les quatre mécanismes annoncés, mais elle refuse tout build
  autre que 92777 ou 93847. Cette allowlist de numéro de build viole la
  politique Suite du 25 août; la DLL externe n'est ni installée ni publiée.
- Le bypass de proximité couvre maintenant toute page stash `4` lorsque la
  portée serveur distante ou la session cliente distante est active. Il ne
  dépend plus des deux seuls return-sites Quick Move; drag, held-item deposit,
  retrait et Ctrl-click partagent ainsi le même gate distant.
- La transition native de stash conserve toujours le `transitionFlag` demandé.
  La portée hotkey ne force plus `false`, car le mode `2, true` enregistre les
  cibles natives de routage des items.
- `remoteOnly` ne referme plus le panneau Inventory compagnon après l'ouverture.
  Ce panneau reste indépendant du toggle, mais peut demeurer visible pendant la
  session parce que le routage natif du stash en dépend. Le README et le TOML
  anglais documentent explicitement ce comportement.
- L'ancien ticket Cube temporisé de la 2.0.2 est entièrement retiré. La 2.0.3
  détecte l'état `0x19`, les panneaux visibles `HoradricCubeLayout` ou
  `HoradricCubePanel`, ou l'onglet intégré seulement si `BankPanel` et son enfant
  `convert` sont tous deux visibles. Elle ferme l'état Cube s'il est actif,
  garantit Inventory `1`, puis, au premier open réussi, appelle
  `StashInterfaceTransition(2, true)` et marque l'UI dirty. Une récupération
  bornée rejoue le dismiss si le stash est déjà visible mais que la première
  ouverture reste incohérente. Les traces `RemoteStash live[...]` couvrent les
  étapes queue, recovery, finalize et failure.
- La candidate ne contient plus aucune allowlist de build-name : l'identité
  reçue est seulement journalisée, puis l'empreinte fail-closed vérifie les
  fonctions, callsites, signatures et témoins de layout déjà gouvernés, y
  compris `GetUiState 0xCE500`, `UI_OpenInterfaceState 0xCD7C0`,
  `UI_CloseInterfaceState 0xC7D30`, `UI_ApplyInterfaceModeTransition 0x11FB80`,
  `UI_FindTopLevelPanel 0x846170`, `UI_FindChildWidgetByName 0x856220` et
  `UI_MarkDirty 0x843FC0`. Le corpus commun 92777/93847 est vérifié et la
  référence PluginPack demeure épinglée à
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`.
- Trois builds Release avec avertissements fatals produisent byte-exactement la
  même DLL : version PE/PluginInfo `2.0.3`, taille `543744`, SHA-256
  `F82530CAB01451560E21868E2A1CD63C6918F6D60E10BDE5DB60DB7B0C41AFB9`.
  Les deux builds propres passent chacun CTest `1/1`; la DLL expose exactement
  `D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin` et
  `D2RLoaderUnloadPlugin`, reste non signée, contient les nouveaux témoins Cube
  et ne contient ni `92777`, ni `93847`, ni le message d'allowlist interdit.
  `Test-NativeWrites.ps1` est `VALID` avec 18 plugins, 116 écritures Suite et
  quatre call-throughs composables; `Test-Suite.ps1` est également `VALID`.
- Seul le hash canonique du TOML source est actualisé dans le manifeste
  (`DFDF0CB12B837A20AAC5D44766F6E9D43F08475A332500B58DBF195BD744D2AA`).
  La DLL packagée et la DLL du jeu restent inchangées : le hash candidat n'est
  pas promu dans l'allowlist de release avant qualification runtime.
- **Runtime/gameplay : NOT RUN.** Prochain gate distinct : déployer exactement
  la DLL candidate sous D2R 3.3.93847 avec la pile complète active, puis tester
  loin du coffre physique les six scénarios du rapport — held item + hotkey,
  UI fermée + hotkey, Cube + hotkey au premier open, Cube + bouton au premier
  open, contrôle sans Cube et fermeture/reopen — ainsi que cold start, or,
  Personal/Shared, sauvegarde/relecture et absence de régression des cinq DLL
  eezstreet. Aucun rollback runtime n'est requis pour ce lot statique.

## Contrat de fermeture explicite Remote Stash 2.2.0 — décision — 28 août 2026

- Vincent confirme en jeu sous D2R 3.3.93847 les quatre régressions ciblées de
  la 2.0.3 : objet tenu + hotkey, premier open depuis les UI fermées, Cube +
  hotkey et Cube + bouton sont `PASS`. La DLL globale testée porte le SHA-256
  `F82530CAB01451560E21868E2A1CD63C6918F6D60E10BDE5DB60DB7B0C41AFB9`.
- Le cold start complet charge Remote Stash 2.0.3 et les cinq DLL eezstreet,
  mais D2RLoader termine avec `36` plugins chargés et `1` plugin en échec :
  Revive Overhaul mod-local. Cet incident préexistant est extérieur aux quatre
  correctifs Remote Stash; il laisse néanmoins la gate de coexistence Suite
  globale ouverte.
- Vincent autorise par `GO` le remplacement du contrat ambigu
  `hotkey_mode = "remoteOnly" | "remoteAndInventory"` par le booléen TOML
  `close_remote_stash_and_inventory_together`. La valeur `true` ferme
  Inventory avec Remote Stash lorsque le raccourci ferme la session; `false`
  ferme seulement Remote Stash. L'ouverture conserve toujours le compagnon
  Inventory requis par le routage natif des objets.
- Le bouton physique reste hors de ce réglage : il est nécessairement utilisé
  depuis un Inventory déjà ouvert et continue de fermer seulement Remote
  Stash. Les anciens fichiers portant `hotkey_mode` restent acceptés et sont
  traduits `remoteOnly -> false` et `remoteAndInventory -> true`; déclarer les
  deux formes dans le même fichier doit être refusé comme configuration
  ambiguë.
- Les anciennes POC PanelService ont déjà utilisé les numéros expérimentaux
  `2.1.0` et `2.1.1`. Cette évolution reprend donc à `2.2.0` afin de ne pas
  réutiliser une identité binaire historique. Elle ne modifie aucun hook, RVA,
  octet attendu, layout natif ni ABI; seul le parseur TOML, la politique de
  toggle, les diagnostics, les tests et la documentation changent.
- La 2.2.0 remplace l'enum publique par un booléen interne, conserve `false`
  comme valeur par défaut et applique le couplage uniquement au raccourci. Le
  parseur accepte le nouveau booléen et les deux valeurs legacy, mais refuse
  les doublons, les valeurs invalides et tout fichier déclarant simultanément
  l'ancienne et la nouvelle forme. Les tests couvrent l'ouverture compagnon,
  la fermeture couplée `true`, la fermeture indépendante `false`, le bouton
  physique et toutes les routes de migration ou de rejet du TOML.
- Trois builds Release avec avertissements fatals produisent byte-exactement la
  même DLL : PE/PluginInfo `2.2.0`, taille `543744`, SHA-256
  `5FE934305F2387ED01D805BBDAA000A8443576E3908B2FCEAD5FB175D5F4B4CF`.
  Chaque build passe CTest `1/1`; les exports restent limités à
  `D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin` et
  `D2RLoaderUnloadPlugin`. `Test-NativeWrites.ps1` est `VALID` avec 18
  plugins, 116 écritures Suite et quatre call-throughs composables;
  `Test-Suite.ps1` est aussi `VALID` avec 18 plugins déclarés et présents.
- Deux cold starts ont déployé ce hash exact dans le dossier global, d'abord
  avec le legacy `hotkey_mode = "remoteOnly"`, résolu en `closeTogether=false`,
  puis avec `close_remote_stash_and_inventory_together = true`, résolu en
  `closeTogether=true`. Les deux démarrages atteignent `24/24`; Remote Stash
  2.2.0 et les cinq DLL eezstreet chargent. D2RLoader rapporte toujours `36`
  plugins chargés et l'unique échec Revive Overhaul mod-local, extérieur à ce
  changement et non neutralisé pendant les essais.
- Vincent confirme `PASS` en jeu pour le comportement final `true` : loin du
  coffre physique, le premier appui sur `;` ouvre Remote Stash et Inventory,
  et le second ferme les deux. La configuration runtime finale conserve cette
  valeur, SHA-256
  `1CCF73C349DF5715269139A2D0F577A88F651B8AD7C18E0FBEF366614B63FC1A`.
  La DLL installée conserve le hash candidat ci-dessus; aucune relance
  supplémentaire du jeu n'est nécessaire pour la promotion de l'archive.
- Le runtime 2.0.3 précédent et son TOML legacy ont été sauvegardés sous
  `analysis-cache/runtime-backups/remote-stash-2.0.3-before-2.2.0-20260828-150300`.
- Vincent retient explicitement la 2.2.0 comme version de release. L'allowlist
  du dépôt produit promeut la version et le SHA-256 DLL
  `5FE934305F2387ED01D805BBDAA000A8443576E3908B2FCEAD5FB175D5F4B4CF`;
  le TOML public conserve sa valeur par défaut `false` et le SHA-256
  `57E378C69133C485FBDA5B2877E03F111B061AC55597A21706B76C869B1D7399`.
  Le TOML personnel déjà installé reste préservé à `true`.
- L'archive locale
  `addons/RemoteStash/package/RuffnecKk-remote-stash-v2.2.0.zip`, taille
  `546599`, SHA-256
  `F1E57900AD6104E727629E3812B3E8D0DC6592419D2B090021960EAFDA268C63`,
  contient exactement `plugins/d2rl-ruffneckk-remote-stash.dll` et
  `config/ruffneckk-remote-stash.toml`; leurs hashes internes égalent les
  artefacts build/runtime et source gouvernés. Aucun README, source, PDB, log
  ni binaire tiers n'est inclus.
- Le README anglais destiné à la relecture humaine reste à côté du ZIP sous
  `addons/RemoteStash/package/README.txt`, hors archive, SHA-256
  `405DB98024F2FCE7649C236DD762BBF6111E242DF3D048DC6DEF618083480692`.
  La publication GitHub et le commit/push du dépôt autonome RuffnecKk Suite
  restent des opérations séparées. La gate de coexistence Suite globale reste
  ouverte tant que l'échec Revive Overhaul n'est pas résolu et requalifié.
