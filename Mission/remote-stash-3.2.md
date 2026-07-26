# RemoteStash — D2R 3.2.92777

Dernière mise à jour : 26 juillet 2026

Statut : phase 1 mise en pause à son gate de preuve pendant la priorité
Transmogrify. Le chemin serveur natif d’ouverture du stash est identifié
statiquement, mais sa confirmation dynamique reste suspendue par la validation
officielle hors ligne de Battle.net. Aucun prototype public, aucun bouton, aucun
sprite et aucune archive n’existent encore.

## Décisions confirmées

- Après une pause temporaire au profit de Repair Costs Cap, Vincent a repris
  RemoteStash selon l’Option A le 24 juillet 2026. Configurable Larzuk Sockets
  reste intacte à son gate de validation en jeu.
- La mission est remise en pause le 26 juillet 2026 lorsque Vincent promeut
  Transmogrify; les preuves et le prochain gate RemoteStash restent intacts.
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

## Gates observables

- identifier et borner la fonction native d’ouverture, ses callers et son ABI;
- distinguer l’action client, les éventuels paquets et le contexte serveur;
- prouver les signatures strictes et l’unique propriétaire de chaque hook;
- déclencher l’ouverture depuis un contrôle technique minimal, sans sprite final;
- fermer proprement le panneau et préserver inventory, personal stash et shared stash;
- vérifier ville/hors ville, changement d’acte, souris/manette, solo, hôte et joiner;
- démontrer zéro perte, duplication, sauvegarde corrompue, crash ou désynchronisation;
- seulement ensuite définir le contrat des layouts vanilla et personnalisés.

## Prochain gate

Après renouvellement normal de la validation en ligne du jeu, confirmer qu’une
interaction native avec le coffre entre dans `OBJECTS_OperateFunction32_Bank`
`0x528270`. Déclencher ensuite cette même voie depuis une commande ou un raccourci
technique minimal exécuté dans un contexte serveur valide, avant tout bouton ou
sprite. Vérifier d’abord la ville, la fermeture et le layout BKVince; les cas
hors ville, multijoueur et layouts tiers suivront seulement si ce gate est vert.
