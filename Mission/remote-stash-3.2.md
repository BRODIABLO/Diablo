# RemoteStash — D2R 3.2.92777

Dernière mise à jour : 29 juillet 2026

Statut : prototype technique autonome `RemoteStash 0.2.24` compilé, déployé et
validé en jeu pour BKVince. Vincent confirme que le bouton desktop ouvre
désormais le panneau stash natif. Le clic passe par le broker partagé du
dispatcher UI, détenu soit par l'autonome Bulk Skill Point Allocation, soit par
`plugin-skills.dll`, sans conflit de hook. Un sprite coffre personnalisé à
quatre états est maintenant déployé
en `176 × 112` avec un placement dynamique qui préserve intégralement le bouton
d’or. Vincent confirme son rendu, son hit-test, l’ouverture au clic et le tooltip
natif `YOUR PRIVATE STASH` résolu par la clé `remoteStashTooltip`. Le bouton d’or
reste à la position imposée par le layout BKVince et n’est jamais déplacé par
RemoteStash. La session distante autoritaire permet désormais le dépôt, le
retrait et le quick move hors ville dans les coffres personnel et partagé;
la persistance du shared stash est confirmée après Save & Exit. Le 28 juillet
2026, un profil isolé
`RemoteStashRetail` sans BKVince valide aussi le bouton sur un inventaire
desktop retail-like `10 × 4` : placement, sprite, hit-test, tooltip retail
`OPEN CURRENT STASH` et ouverture sans dialogue `Drop Gold` sont confirmés.

## Décisions confirmées

- Après une pause temporaire au profit de Repair Costs Cap, Vincent a repris
  RemoteStash selon l’Option A le 24 juillet 2026. ForceLarzukSockets
  reste intacte à son gate de validation en jeu.
- Depuis le 26 juillet 2026, Vincent autorise la préparation statique et la
  compilation de RemoteStash en parallèle de la mission active. Le prototype
  0.1.0 est maintenant déployable, mais aucun contournement du gate officiel de
  connexion ni aucune prétention de validation gameplay ne sont autorisés.
- La catégorie PluginPack future est `misc`, avec `plugin-misc.dll` comme DLL
  propriétaire et `misc.remoteStash` comme clé prévue dans l’unique
  `D2RPlugins.json`.
- Pendant l’incubation, la fonctionnalité restera dans une DLL autonome hybride
  `RemoteStash.dll`, attribuée exactement à `RuffnecKk`, sans modifier, lier ni
  redistribuer une DLL d’eezstreet.
- La première phase a prouvé le chemin natif d’ouverture du stash et sa
  reproduction depuis un autre contrôle UI. Le chantier couvre maintenant le
  sprite, le placement final et l’adaptation aux layouts personnalisés.
- Le 27 juillet 2026, Vincent a demandé de commencer en parallèle la preuve du
  placement adaptatif. Le futur bouton ne doit utiliser aucune coordonnée propre
  à BKVince : il doit lire le layout runtime actif, se placer relativement à
  des enfants nommés du `PlayerInventoryPanel`, vérifier les collisions et se
  masquer si aucune géométrie sûre n’est disponible.
- Le 27 juillet 2026, Vincent confirme le jalon visuel et fonctionnel desktop
  complet dans BKVince. La prochaine évolution demandée est un hotkey
  configurable qui appelle le même chemin RemoteStash sans dupliquer la logique
  d’ouverture; son implantation reste à planifier séparément.

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
