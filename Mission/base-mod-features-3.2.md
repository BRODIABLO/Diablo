# BaseMod 3.2 — Charm Zone, services répétables et démarrage Players

Dernière mise à jour : 31 juillet 2026

## Décisions produit

Vincent retient un chantier unique inspiré de BaseMod, séquencé par valeur joueur :

1. implanter une Charm Zone stricte propre à BKVince;
2. intégrer les services de quête répétables au prochain lot PluginPack;
3. auditer le player count automatique avant d'ajouter une couche PluginPack;
4. vérifier la nécessité de `UniqueNoLimit` sous D2R 3.2;
5. mesurer la cadence CPU de PotionAutoPickup avant toute optimisation.

`CharmZone.dll` est confirmé comme **plugin autonome permanent**, hors PluginPack.
Il reste hybride globale/mod-locale, compatible avec les cinq DLL eezstreet et
utilise son propre `charm-zone.toml` en anglais. BKVince l'installe mod-localement.

La destination PluginPack confirmée qui reste à implanter est :

- `quests.repeatableServices` dans `plugin-quests.dll`;

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
- [ ] Identifier l'émission serveur des menus après consommation et le chemin
  autoritaire Akara/respec.
- [ ] Localiser une couture de paiement après validation mais avant mutation de
  l'objet, puis prouver l'affichage localisé du prix.
- [ ] Intégrer `quests.repeatableServices` sans collision avec les chemins
  Charsi/Larzuk existants ni avec ConfigurableCharsiReward.
- [ ] Remplacer le patch de récompenses infinies seulement après matrice complète.
- [x] Ne pas intégrer `misc.startingPlayersCount` : le réglage natif persistant
  par mod produit déjà `/players8` à chaque cold start BKVince.
- [x] Ne pas créer `UniqueNoLimit` : `uniqueitems.txt` couvre déjà le besoin par
  ligne en D2R 3.2; un test de double drop reste utile comme témoin, pas comme
  prérequis d'architecture.
- [ ] Mesurer PotionAutoPickup avec 0, 100 et 500 objets au sol; n'ouvrir une
  optimisation que si le coût dépasse 1 % CPU processus ou 0,5 ms au p99.

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

Cartographier l'émission serveur des entrées NPC après consommation, le chemin
autoritaire Akara/respec et la couture post-validation/pré-mutation des trois
services d'objet. Le débit atomique et les consommations gratuites sont prouvés;
aucun code `quests.repeatableServices` ne sera écrit avant d'avoir aussi prouvé
que le paiement ne peut ni facturer un objet refusé, ni laisser une mutation
gratuite, et qu'il coexiste avec ConfigurableCharsiReward, ForceLarzukSockets,
MassID et le patch infini actuel.
