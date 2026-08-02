# BaseMod 3.2 — Charm Zone, services répétables, MF linéaire et démarrage Players

Dernière mise à jour : 2 août 2026

## Décisions produit

Vincent retient un chantier unique inspiré de BaseMod. Depuis le 2 août 2026,
ses branches avancent en parallèle selon les preuves disponibles, sans dépendance
artificielle entre les services répétables et le Magic Find linéaire :

1. implanter une Charm Zone stricte propre à BKVince;
2. intégrer les services de quête répétables au prochain lot PluginPack;
3. auditer le player count automatique avant d'ajouter une couche PluginPack;
4. vérifier la nécessité de `UniqueNoLimit` sous D2R 3.2;
5. mesurer la cadence CPU de PotionAutoPickup avant toute optimisation;
6. proposer un mode Magic Find linéaire qui rende la statistique pleinement
   valorisable dans l'économie BKVince.

`CharmZone.dll` est confirmé comme **plugin autonome permanent**, hors PluginPack.
Il reste hybride globale/mod-locale, compatible avec les cinq DLL eezstreet et
utilise son propre `charm-zone.toml` en anglais. BKVince l'installe mod-localement.

Les destinations PluginPack confirmées sont :

- `quests.repeatableServices` dans `plugin-quests.dll`;
- `items.magicFindFormula` dans `plugin-items.dll`.

## Contrat Charm Zone

- Inventaire BKVince : grille 11 × 8.
- Rectangle actif : les quatre rangées du bas, `x=0..10`, `y=4..7`.
- Un charm doit tenir entièrement dans le rectangle; tout chevauchement le rend
  entièrement inactif.
- Tous les effets sont désactivés côté serveur : stats, résistances, skills,
  oskills, auras et déclencheurs.
- L'icône inactive est teintée rouge et son tooltip explique qu'elle est hors
  Charm Zone; l'objet reste déplaçable.
- Le plugin ne doit ni modifier la sauvegarde ni faire confiance au rendu client.

## Contrat des services répétables

Chaque service expose `mode = disabled | free | paid`, `goldPerLevel` et
`minimumGold`. Le prix paid vaut
`max(minimumGold, playerLevel * goldPerLevel)`.

| Service | goldPerLevel | minimumGold |
|---|---:|---:|
| Respec | 3000 | 5000 |
| Imbue | 500 | 5000 |
| Socketing | 1000 | 20000 |
| Personalization | 500 | 10000 |

- Defaults PluginPack : les quatre services sont `disabled`.
- Preset BKVince : les quatre services sont `paid`.
- La récompense native gratuite est consommée avant tout paiement, séparément
  par difficulté.
- Le prix est visible dans le menu NPC et les messages succès/refus sont
  localisés.
- L'autorité serveur recalcule le prix au clic et débite atomiquement l'or porté
  puis le coffre personnel; le coffre partagé est exclu.
- Les quest flags restent exacts. Un refus ne modifie ni or, ni objet, ni flags.
- `infinite-quest-rewards.json` sera retiré de BKVince seulement après équivalence
  gameplay démontrée.

## Contrat Magic Find

Vincent confirme le 2 août 2026 la destination **merge PluginPack**, la catégorie
`items`, le propriétaire `plugin-items.dll` et la clé canonique
`items.magicFindFormula`. L'incubation a d'abord validé une DLL RuffnecKk hybride
et son JSON autonome, puis le même module a été fusionné dans le pack. Après
parité du binaire intégré, le témoin autonome a été retiré du runtime et déplacé
dans `analysis-cache` afin de conserver un propriétaire unique.

- `mode = vanilla | linear` uniquement pour la première version.
- Défaut PluginPack : `vanilla`; preset BKVince : `linear`.
- `vanilla` doit laisser le chemin natif et ses octets entièrement intacts.
- `linear` supprime seulement les diminishing returns positifs appliqués aux
  qualités unique, set et rare. La qualité magic, prouvée déjà linéaire sous
  92777, et tout comportement MF nul ou négatif restent natifs.
- Aucun coefficient, cap, modification d'`itemratio.txt`, changement de sauvegarde
  ou calcul client n'entre dans la première version.
- La génération d'objet reste autoritaire côté serveur. Solo, hôte et joiner
  doivent obtenir la formule configurée par l'hôte sans désynchronisation.
- Le plugin doit refuser le build, la signature, l'ABI ou le JSON incompatibles;
  une configuration absente conserve `vanilla`.
- Les trois écritures sont gouvernées par le manifeste du PluginPack et refusent
  tout build, témoin ou configuration incompatible avant commit transactionnel.

## Décision Starting Players

Aucune nouvelle option PluginPack n'est justifiée. D2R 3.2 conserve déjà
`Offline Difficulty Scaling` par profil de mod dans son `Settings.json` et
BKVince porte la valeur `8`. Trois cold starts distincts ont affiché le message
natif « Game difficulty scale set to 8 » sans plugin dédié. Le patch
`player-difficulty-overrides.json` étend les plafonds; il ne fournit pas la
valeur initiale. Une option `misc.startingPlayersCount` ferait doublon avec ce
réglage natif persistant et est donc abandonnée tant qu'une friction réelle
n'est pas reproduite.

## Faits vérifiés

- Le workbench canonique 92777, son image et son index sont vérifiés.
- Le `BaseMod.ini` fourni expose `[MFLinear] Enabled=0`. Le reverse engineering
  du DLL legacy prouve que son helper remplace les rendements décroissants
  Unique/Set/Rare par `MF+100` au-dessus de `+10 MF`; Magic reste sur son chemin
  déjà linéaire et le gate `MF <= -100` reste intact. Aucun RVA ou ABI 32 bits
  n'a été transposé vers D2R 3.2.
- Les onze classes BKVince commencent avec l'unique `mfc`. Sa ligne `Charm
  Modifiers` donne `mag%=-199` et `mag%/lvl=4`; avec le scale en huitièmes de
  `item_find_magic_perlevel`, le personnage passe approximativement de `-199`
  au niveau 1 à `-150` au niveau 99 avant tout autre équipement.
- BKVince construit déjà une économie MF progressive : potion `mfp` à `+150`
  MF, quivers à `+0,5` MF/niveau et jusqu'à trois sockets, puis plusieurs uniques
  ou runewords convertis d'un bonus fixe vers un bonus par niveau. Avec potion,
  quiver et Ist, le total brut peut traverser le gate négatif puis atteindre
  environ `+139` au niveau 99; sans inserts, le même ensemble atteint environ
  `+49`.
- BKVince ne livre aucun `itemratio.txt` et hérite donc de la table vanilla 3.2.
  Les copies vanilla active et base sont byte-identiques, six lignes, SHA-256
  `F8EBB2128C09311D2F17F9F3EA9E1933441F3322AB2C4C203EB9858A00AF90FF`.
  Le mode linéaire ne doit pas modifier cette table ni contourner ses minima.
- L'image 92777 confirme directement l'oracle : `D2GAME_ITEMS_RollItemQuality`
  `0x4421B0` lit la stat 80, additionne celle du propriétaire d'un minion, puis
  conserve le gate `MF <= -100`. Les branches `7F 04` de Unique `0x4423CD`, Set
  `0x44246A` et Rare `0x4424F7` sélectionnent les constantes 250/500/600; le bloc
  Magic `0x442576` utilise déjà le MF brut. Le mode `linear` remplace seulement
  ces trois branches par `90 90`.
- À titre d'impact, `+100` MF brut reste `+100` partout en mode linéaire, contre
  environ `+71/+83/+85/+100` pour unique/set/rare/magic dans l'oracle vanilla;
  à `+300`, l'écart devient `+136/+187/+200/+300`. Ce sont des MF effectifs,
  pas des probabilités finales : niveau, Treasure Class, `itemratio` et minima
  continuent de s'appliquer.
- Le clone PluginPack local intègre le module RuffnecKk dans `plugin-items.dll`.
  Le manifeste et la couverture source passent à `139/139`, les tests Release à
  `26/26`, et le JSON distribué garde `vanilla`. Le preset BKVince porte
  `linear` dans son `D2RPlugins.json`.
- La matrice runtime du binaire fusionné prouve `linear` avec trois branches
  `90 90`, `vanilla` explicite ou clé absente avec zéro écriture et trois
  branches `7F 04`, puis le refus fail-closed de `legacy` avec
  `plugin-items.dll` inactif et zéro écriture. Le
  cold start final revient à `linear`, charge `9/9` plugins, applique `19/19`
  patches et atteint `24/24`; DLL et JSON sont hash-identiques source/runtime
  (`57FCB38A…DFCD`, `5752CAC3…728B`).
- Les quatre assertions `D2Common\\src\\Items\\Items.cpp` post-frontend sont
  reproduites à l'identique en `vanilla` et `linear`; elles préexistent à ce
  module et restent un chantier BKVince distinct.
- `SetPlayerCount(session, count)` est identifié à `0xD2F020`; le PluginPack
  actuel ne l'appelle pas automatiquement à la création d'une partie.
- Le profil runtime BKVince contient `"Offline Difficulty Scaling": 8` dans
  `Saved Games/.../mods/BKVince/Settings.json`; le profil vanilla reste à `1`.
- Le patch BKVince de récompenses infinies porte six écritures : deux pour Anya,
  deux pour Charsi et deux pour Larzuk. Il ne couvre pas Akara.
- `D2GAME_NPC_TryDeductGold 0x5416D0` fournit déjà le débit serveur atomique
  requis : refus sans mutation si les fonds sont insuffisants, puis or porté et
  coffre personnel seulement. Sa signature stricte est unique sous 92777.
- Les routines serveur de consommation gratuite sont identifiées et uniques :
  Charsi `0x5DA1C0` pour la quête 3, Anya `0x547C60` pour la quête `0x26`, et
  Larzuk `0x548B60` pour la quête `0x23`. Elles opèrent sur les quest flags de
  la difficulté courante; le patch infini neutralise ces chemins.
- Le client 3.2 enregistre déjà les quatre actions/panneaux : respec combiné,
  socketing, personalization et imbue. Le service Akara natif réinitialise
  ensemble stats et skills; les deux commandes séparées de BaseMod 1.13 ne sont
  donc pas retenues sans nouveau besoin produit explicite.
- `CLIENT_BuildNpcInteractionMenu 0x1147A0` filtre l'entrée Akara texte 11168 à
  partir des flags `RewardGranted` et `RewardPending` de la quête `0x29` dans la
  difficulté courante. Une récompense consommée masque l'entrée; la disponibilité
  du menu est donc une construction client fondée sur les flags synchronisés,
  pas une entrée réémise dynamiquement par le serveur.
- Le callback client `0x1130D0` envoie un paquet de cinq octets `{0x39, npcGuid}`.
  Une lecture runtime contrôlée du tableau des callbacks serveur 92777 prouve que
  sa case `0x39` pointe sur `0x4B2530`; les cases témoins `0x38`, `0x41` et
  `0x51` concordent avec leurs handlers déjà gouvernés. Ce handler revalide la
  taille, le NPC, l'interaction et `RewardPending` avant toute mutation.
- La transaction combinée `D2GAME_PLAYER_ResetStatsAndSkills 0x580F20` est
  appelée à `0x4B2A23`, immédiatement après la dernière validation serveur. Elle
  rembourse les skills via `0x4360F0` et les quatre stats de base via `0x52DDF0`.
  Le bookkeeping gratuit `0x5D9AE0` vient seulement après et pose
  `RewardGranted`/efface `RewardPending` pour la quête `0x29`.
- Pour Akara, la couture payante est donc prouvée : conserver le premier usage
  vanilla, autoriser explicitement un repeat déjà consommé, débiter avec
  `D2GAME_NPC_TryDeductGold` après validation et avant `0x580F20`, puis ne pas
  appeler `0x5D9AE0` sur ce repeat. Un hook de `0x580F20` seul est insuffisant :
  il ne peut ni réafficher l'entrée, ni distinguer l'usage gratuit du repeat.
- Le moteur de transaction NPC/item `0x4FC230` contient les chemins Charsi,
  Larzuk et Anya, mais il est partagé et son point post-validation/pré-mutation
  n'est pas encore prouvé. Le hooker maintenant risquerait une facturation sur
  objet invalide ou une collision avec le flux opcode `0x34` utilisé par MassID.
- L'inventaire joueur BKVince mesure 11 × 8 dans `inventory.txt`; l'ombrage est
  visuel et ne constitue aucune règle de gameplay.
- Les accesseurs inventory list, next item, item data, item type, statlist merge,
  statlist expire et refresh sont déjà gouvernés. Le champ `ItemData+0xB8` est
  `nNodePos`, pas une coordonnée de grille.
- PotionAutoPickup parcourt les buckets d'unités et son option
  `minimum_interval_frames` compte actuellement les passages d'un hook réseau,
  pas des frames rendues.
- `ITEMS_CapturePacketState 0x382D20` restitue dans une structure de 16 octets
  la page et les coordonnées `x/y` avec l'ABI `(stateOut, item) -> stateOut`;
  `ITEMS_GetDimensions 0x371850` restitue les dimensions.
- `ITEMS_IsCharmUsable 0x36AE00` est l'équivalent 92777 du prédicat D2Common
  1.13d remplacé par BaseMod. CharmZone 0.3.1 filtre ce prédicat natif après son
  résultat, sans modifier directement les statlists.
- `UI_RenderItemIcon 0x15BB80` reçoit l'item, les coordonnées écran compactées,
  l'échelle et les paramètres de rendu. Le trampoline D2RLoader masque le
  callsite à `_ReturnAddress()`; la sélection visuelle utilise donc les pointeurs
  d'items refusés par le hook gameplay, dédupliqués par frame.
- Le tooltip natif `0x2BD480` est déjà partagé par Transmogrify,
  AdvancedItemTooltips et ExtendedItemStats. FloatingDamage est l'unique hôte
  D3D12/ImGui. FloatingDamage 1.1.0 fournit maintenant un registre multi-overlay
  rétrocompatible; CharmZone l'utilise sans créer un second renderer.
- Le tableau BKVince `uniqueitems.txt` met déjà `nolimit=1` sur 489 lignes, dont
  467 uniques droppables ordinaires. Les six lignes `spawnable=1` laissées vides
  sont les pré-variantes internes des sunder charms. Une feature native
  `UniqueNoLimit` ferait donc doublon avec les données 3.2 actuelles.
- Le cold start final charge 8 plugins, 0 échec. Un charm synthétique +20
  résistance feu donne exactement `0 -> 20 -> 0` hors zone, dedans, puis dehors;
  le masque rouge et le message de survol suivent l'objet, avec zéro échec de
  classification, zéro échec de placement et zéro drop visuel.

## Gates Charm Zone

- [x] Destination autonome confirmée le 31 juillet 2026.
- [x] Géométrie et règle de containment confirmées.
- [x] Prouver les coordonnées de l'item et leur ABI sous 92777.
- [x] Prouver et hooker le prédicat natif d'éligibilité des charms sans mutation
  directe des statlists.
- [x] Prouver le hook client de teinte et le point d'extension du tooltip.
- [x] Auditer la propriété de chaque hook avec les cinq DLL du PluginPack et les
  plugins BKVince.
- [x] Compiler Release x64, exécuter les tests et vérifier manifeste/exports.
- [x] Valider le preset BKVince, la priorité mod-locale et les hashes runtime.
- [ ] Valider config absente, invalide et repli global au-delà du chemin
  mod-local nominal déjà prouvé.
- [x] Valider l'inventaire, les déplacements répétés, la sauvegarde/relecture et
  la souris en solo.
- [ ] Valider stash, Cube, mort, changement d'acte, manette et hôte/joiner.
- [x] Vérifier le ZIP strict DLL + TOML et son readback hash-identique.
- [ ] Valider la portée globale dans une matrice isolée.

## Gates suivants

- [x] Prouver un débit d'or atomique excluant le coffre partagé.
- [x] Identifier la consommation gratuite Charsi, Larzuk et Anya par difficulté.
- [x] Confirmer la disponibilité des quatre actions/panneaux natifs côté client.
- [x] Identifier le filtre client post-consommation et le chemin serveur
  autoritaire Akara/respec.
- [x] Localiser pour Akara une couture de paiement après validation et avant la
  transaction combinée de reset, sans réécriture des quest flags.
- [ ] Localiser la couture équivalente après validation mais avant mutation pour
  Charsi, Larzuk et Anya.
- [ ] Prouver l'affichage dynamique et localisé du prix calculé au niveau.
- [ ] Intégrer `quests.repeatableServices` sans collision avec les chemins
  Charsi/Larzuk existants ni avec ConfigurableCharsiReward.
- [ ] Remplacer le patch de récompenses infinies seulement après matrice complète.
- [x] Confirmer `items.magicFindFormula` dans `plugin-items.dll`, avec modes
  `vanilla|linear`, défaut vanilla et preset BKVince linéaire.
- [x] Prouver sous 92777 la fonction de calcul MF, ses callsites, son ABI, le
  traitement séparé unique/set/rare/magic et le comportement nul/négatif.
- [x] Auditer le manifeste et les hooks de `plugin-items.dll`, puis désigner une
  plage canonique sans collision pour le prototype RuffnecKk destiné au merge.
- [x] Fusionner le module dans `plugin-items.dll`, conserver `vanilla` dans le
  JSON public, activer `linear` dans BKVince et retirer le témoin autonome.
- [x] Valider en runtime les modes `vanilla|linear`, la clé absente, le refus
  d'une valeur inconnue, le preset BKVince et la coexistence des neuf plugins.
- [ ] Échantillonner les drops aux frontières `MF=-199/-100/-99/0/10/11` et à
  MF positif élevé, puis valider minion/propriétaire et hôte/joiner.
- [x] Ne pas intégrer `misc.startingPlayersCount` : le réglage natif persistant
  par mod produit déjà `/players8` à chaque cold start BKVince.
- [x] Ne pas créer `UniqueNoLimit` : `uniqueitems.txt` couvre déjà le besoin par
  ligne en D2R 3.2; un test de double drop reste utile comme témoin, pas comme
  prérequis d'architecture.
- [ ] Mesurer PotionAutoPickup avec 0, 100 et 500 objets au sol; n'ouvrir une
  optimisation que si le coût dépasse 1 % CPU processus ou 0,5 ms au p99.

## Architecture Akara retenue pour l'incubation

1. `disabled` laisse entièrement passer le comportement vanilla : une récompense
   gratuite pending, puis aucune nouvelle offre après consommation.
2. `free` et `paid` ne changent jamais la première charge native. Après
   `RewardGranted`, ils autorisent une entrée répétable côté client et la même
   transaction serveur 0x39; seul `paid` exige le débit atomique.
3. Le serveur recalcule le prix au clic. Sur fonds insuffisants, il retourne avant
   `0x580F20`; aucune stat, aucun skill et aucun quest flag ne changent.
4. Un repeat réussi appelle `0x580F20` mais pas `0x5D9AE0`. Les flags de la quête
   `0x29` restent donc bit-exacts, par difficulté, tandis que l'usage vanilla
   conserve tout son bookkeeping natif.
5. Le libellé prix doit être construit au moment où `0x1147A0` ajoute l'entrée;
   le string id 11168 global reste intact pour le mode vanilla.

## Architecture CharmZone retenue

1. Le hook de `ITEMS_IsCharmUsable` appelle le prédicat natif en premier. Un
   charm nativement valide reste actif seulement si sa page et son rectangle
   complet appartiennent à la zone configurée.
2. Le hook client de `UI_RenderItemIcon` appelle toujours le renderer natif puis
   capture seulement les pointeurs déjà classés invalides par le hook gameplay.
3. FloatingDamage conserve son callback historique ExtendedItemStats et ajoute
   un registre borné de callbacks nommés. CharmZone y enregistre son overlay et
   utilise des primitives exportées pour la teinte rouge et le message de
   survol, sans lier une seconde copie d'ImGui.
4. Si l'hôte visuel manque, l'autorité serveur reste active et le défaut visuel
   est journalisé; BKVince livre FloatingDamage comme dépendance présente.

## Prochain gate

Cartographier la couture post-validation/pré-mutation des trois services d'objet,
puis le point de formatage localisé d'un prix dépendant du niveau. Le chemin
Akara est désormais prouvé de bout en bout, mais aucun code
`quests.repeatableServices` ne sera écrit avant d'avoir aussi démontré que
Charsi, Larzuk et Anya ne peuvent ni facturer un objet refusé, ni laisser une
mutation gratuite, et que l'ensemble coexiste avec ConfigurableCharsiReward,
ForceLarzukSockets, MassID et le patch infini actuel.

En parallèle, l'implantation PluginPack de Magic Find Formula et la parité
des écritures sont fermées; restent l'échantillonnage des drops aux frontières
MF, le cas minion/propriétaire et la matrice hôte/joiner.
