# Intégration des plugins RuffnecKk au PluginPack d’eezstreet

## Décision

Option A lancée par Vincent le 22 juillet 2026 : stabiliser d’abord
l’inventaire ABI et la propriété des hooks, puis intégrer les plugins par
tranches. Le dépôt cible audité est `eezstreet/D2RL-Plugins` au commit
`dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`.

Le prototype est développé uniquement dans le clone externe placé sous
`analysis-cache/`. Aucun commit ou push n’est autorisé par ce document. Il
établit aussi le contrat technique à appliquer dans une future branche de
travail du PluginPack lorsque le périmètre des plugins aura été accepté par les
deux auteurs.

Le dépôt amont est désormais enregistré comme référence gouvernée
`d2rlplugins` dans `reverse-engineering/references.json`. Le clone propre
`analysis-cache/references/D2RL-Plugins` reste byte-identique au commit épinglé
et sert aux citations reproductibles; le clone du workbench 92777 demeure la
copie modifiable réservée aux prototypes. Avant chaque nouvelle fonctionnalité,
`npm run ref:d2rlplugins -- status` vérifie le dépôt, le commit, la propreté et
la déclaration de licence. Une mise à jour amont n’avance jamais silencieusement
le pin : elle doit être examinée avant de devenir la nouvelle référence.

## Périmètre canonique du lot accepté

Le 28 juillet 2026, Vincent confirme le lot suivant. Cette liste gouverne
l’inclusion au futur PluginPack; le tableau d’audit plus bas mesure seulement
les collisions et la difficulté technique et ne vaut pas inclusion implicite.

| Propriétaire final | Fonctionnalité source | Configuration dans `D2RPlugins.json` |
|---|---|---|
| `plugin-items.dll` | `GambleScreenLimit` | `items.gambleScreenLimit` |
| `plugin-items.dll` | `GroundItemLabelLimit` | `items.groundItemLabels` |
| `plugin-items.dll` | Item Durability / `DurabilityResistance` | `items.itemDurability` |
| `plugin-items.dll` | Charm Aura Trigger Fix | `items.charmAuraTriggerFix` |
| `plugin-items.dll` | `EnhancedDamageMinMaxFix` | `items.enhancedDamageMinMaxFix` |
| `plugin-items.dll` | bloc unifié `EthItemRules`, incluant l’exclusion d’ItemTypes | `items.etherealItemRules` |
| `plugin-items.dll` | `ExtendedItemStats` | aucune clé externe; infrastructure intégrée |
| `plugin-items.dll` | `RepairCostsCap` | `items.repairCostsCap` |
| `plugin-items.dll` | Qty Display Fix / `QtyDisplayIssue` | `items.qtyDisplayIssue` |
| `plugin-misc.dll` | Cube Quick Move Bottom-Right | `misc.cubeQuickMoveBottomRight` |
| `plugin-misc.dll` | Equipped Item to Cube | `misc.equippedItemToCube` |
| `plugin-misc.dll` | Assign Transmute Hotkey | `misc.transmuteHotkey` |
| `plugin-misc.dll` | `VendorStockRefresh` | `misc.vendorStockRefresh` |
| `plugin-misc.dll` | `PreventMercDeathInTown` | `misc.preventMercDeathInTown` |
| `plugin-quests.dll` | `ForceLarzukSockets` | `quests.larzukSockets` |
| `plugin-skills.dll` | `BulkSkillPointAllocation` | `skills.bulkSkillPointAllocation` |

Le lot contient donc 16 fonctionnalités sources. `EthItemRules` est un seul
composant et un seul bloc de configuration : l’exclusion d’ItemTypes fait partie
de ses règles et ne possède aucune identité sœur. `ExtendedItemStats` n’ajoute
aucune option publique. `plugin-levels.dll` ne reçoit actuellement aucune
fonctionnalité confirmée.

Le `D2RPlugins.json` livré aux joueurs doit conserver le comportement vanilla :
toutes les nouvelles fonctions configurables sont désactivées par défaut et
leurs autres valeurs initiales reprennent les valeurs vanilla lorsqu’elles en
ont une. L’infrastructure `ExtendedItemStats` doit rester sans effet gameplay
visible tant qu’aucun consommateur explicitement activé ne l’utilise.

Sont explicitement hors de ce lot : `Transmogrify`,
`ConfigurableCharsiReward`, la DLL autonome finale `EtherealItemRules.dll`, et
tout autre candidat seulement cité dans l’audit. Les autonomes ethereal restent
des témoins différentiels temporaires jusqu’à la validation du port intégré.

## Preuve de l’environnement ciblé

Le workbench persistant `D2R.exe 3.2.92777` est vérifié :

- image canonique SHA-256
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`;
- image d’analyse SHA-256
  `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`;
- index SQLite vérifié avec 105 850 fonctions, 1 057 329 références et
  59 patch sites connus;
- projet Ghidra persistant présent.

Le clone local du PluginPack contient les cinq plugins `items`, `levels`,
`misc`, `quests` et `skills`, ainsi que `plugin-shared` sous forme de
bibliothèque statique.

## État ABI du PluginPack

`plugin-shared.h` centralise déjà les types suivants :

- `D2TxtFieldDesc` et les conteneurs TXT;
- `D2ItemsTxt`, taille `0x1C0`;
- `D2SkillsTxt`, taille `0x2EC`;
- `D2GameStrc`, avec assertions sur `difficultyLevel`, `expansion`,
  `wItemFormat`, la chaîne vendeur et les seeds;
- `D2StatListStrc`, taille 112;
- `D2UnitStrc`, taille 448, avec champs prouvés à `0x28`, `0x2C`, `0x88`,
  `0x124` et `0x1BD`.

La définition complète de `D2UnitStrc` n’est utilisée directement que par
`plugin-items`, `plugin-skills` et deux helpers de `plugin-shared`. Les nouveaux
plugins ne doivent pas copier cette structure. Ils doivent :

1. employer un type opaque lorsqu’ils ne déréférencent aucun champ;
2. utiliser un accesseur partagé lorsqu’ils lisent un champ déjà canonique;
3. ajouter un champ au type canonique uniquement si son offset 92777 est prouvé
   et couvert par `static_assert`;
4. isoler les structures propres à un sous-système dans un header dédié plutôt
   que d’agrandir systématiquement `plugin-shared.h`.

## Audit des plugins RuffnecKk

La recherche sur les sources versionnées ne trouve aucune définition privée de
`D2UnitStrc`, `D2GameStrc` ou `D2StatListStrc`. Les pointeurs du jeu sont
généralement opaques. Les dépendances ABI restantes sont des fonctions natives
ciblées et, pour quelques plugins, des offsets locaux documentés.

| Plugin | Écritures ou hooks 92777 | Surface ABI | Tranche proposée |
|---|---|---|---|
| `GroundItemLabelLimit` | sept patches entre `0x1516EBE` et `0x1519AF9` | aucune structure gameplay | 1 |
| `GambleScreenLimit` | patch `0x541A7C`, immédiat à `0x541A7E` | aucune structure | 1, fusion dans `plugin-items` |
| `EnhancedDamageMinMaxFix` | hook `0x2FA430` | accès ciblés stat/unit via fonctions natives | 2 |
| `NoEtherealItemTypes` + `Ethereal Item Rules` | hook `0x373890` et quatre opérations du patch | records `ItemTypesTxt`, taux, sets et durabilité effective | 2, fusion autonome commune avant `plugin-items` |
| `BulkSkillPointAllocation` | hooks `0x0EC700`, `0x5F4B90`, `0x843D90` | appels natifs, aucune structure complète | 2, fusion dans `plugin-skills` |
| `AdvancedItemTooltips` | hook `0x2DC4B0` | tooltip et résolveur natif de sockets | 2 |
| `PotionAutoPickup` | hook `0x4B9DF0` | accesseurs natifs d’unités | 2 |
| `FloatingDamage` | hook `0x427150`, plus D3D12/MinHook | vue locale de l’événement de dégâts et rendu privé | 3 |
| `CharmInventoryAuras` | hooks `0x42D2C0`, `0x491960`, `0x502D00` | offsets skill/item/stat-list ciblés | 3 |
| `ReviveOverhaul` | hooks `0x4A3A20`, `0x4A7270`, `0x4A8090`, `0x596720` | ABI IA et return-sites stricts | 3 |
| `DurabilityResistance` | hooks `0x2F48C0`, `0x314110`, `0x441B10` | `ItemsTxt` et `ItemTypesTxt` par offsets ciblés | intégré; conflit externe ranged documenté |
| `Transmogrify` | hooks `0x2BD480`, `0x314110`, `0x43D530`, `0x4F40C0` | records items, tooltip, création et placement | hors lot; témoin externe seulement |
| `ConfigurableCharsiReward` | hooks `0x325C00`, `0x441300`, `0x5DA1C0` | difficulté `D2GameStrc+0x104`, class/TXT ID unité `+0x04` | 4, conflit PluginPack à résoudre |

Les tranches mesurent le coût d’intégration au PluginPack, pas la qualité ou la
valeur gameplay des plugins.

### Correction canonique prouvée pour `D2UnitStrc+0x04`

Le header partagé du PluginPack nomme actuellement le dword à `D2UnitStrc+0x04`
`unitFlags`. Ce nom est sémantiquement incorrect sur le build 92777 : le getter
natif à `0x349860` retourne directement `dword [unit+0x04]` comme class/TXT
record ID. `ConfigurableCharsiReward` lit cette valeur pour comparer le class ID
`monstats` — la cible configurée Andariel se résout à l’ID 156 — tandis que
`Transmogrify` appelle le getter natif pour obtenir le class ID d’un item.

L’usage actuel de `plugin-items`, qui tronque ce même champ à 16 bits pour son
`npcId`, est cohérent avec un TXT record ID et non avec des flags. La structure
canonique devrait donc nommer ce champ `classId` ou `txtRecordId`; un vrai champ
de flags ne doit pas être inventé à cet offset.

## Collisions prouvées

### `0x441300` — propriétaire unique obligatoire

`plugin-items` intercepte l’entrée de la fonction de drop de treasure class à
`0x441300` pour les conditions de drop. `ConfigurableCharsiReward` intercepte
exactement la même entrée afin d’observer la mort d’une cible après l’exécution
du drop original.

Le workbench confirme à `0x441300` le prologue
`40 53 55 56 57 41 54 41 55 41 56 41 57 ...`. Deux hooks indépendants avec
le même tableau d’octets attendus ne constituent pas une coexistence sûre.

Solution recommandée : `plugin-items` demeure propriétaire de l’unique hook et
expose un callback post-drop interne au PluginPack. La logique Charsi s’abonne à
ce callback. Si `ConfigurableCharsiReward` reste une DLL autonome, une seule des
deux fonctionnalités peut posséder ce site sans dispatcher fourni par le
loader.

### `0x314110` — collision déjà présente entre plugins RuffnecKk

`DurabilityResistance` et `Transmogrify` interceptent tous deux
`GetItemsTxtRecord` à `0x314110` :

- le premier active la durabilité des bows/crossbows ciblés;
- le second force `useable=1` pour les records Transmogrify.

Solution appliquée au lot canonique : Transmogrify demeure entièrement hors du
pack. `items.itemDurability` ne pose `0x314110` que lorsque l'extension
bows/crossbows est explicitement activée; le défaut joueur la désactive. Les
résistances et le maximum éthéré coexistent donc avec le Transmogrify autonome,
qui reste seul propriétaire de ce site. Une activation simultanée de l'extension
ranged et de l'autonome exige un broker externe ou un propriétaire unique; le
transformateur Transmogrify ne doit pas être copié dans le pack pour contourner
ce contrat.

### Sous-système gamble — recouvrement sémantique

`plugin-items` possède déjà `D2GAME_STORES_FillGamble` à `0x541880` et modifie
le branchement de filtre à `0x541A28`. `GambleScreenLimit` modifie la limite de
boucle dans la même fonction à `0x541A7C`.

Les plages d’octets ne se chevauchent pas et le hook actuel de `plugin-items`
appelle l’original. La combinaison est donc techniquement composable dans cet
état précis, mais elle doit devenir une option de `plugin-items`, pas un second
plugin ignorant la propriété du sous-système.

## Registre des hooks

`plugin-shared` est une bibliothèque **statique**. Chaque DLL reçoit donc sa
propre copie de ses variables globales. Un registre runtime ajouté naïvement à
`plugin-shared` ne détecterait pas les hooks enregistrés par les autres DLL.

Le PluginPack doit employer deux niveaux :

1. un manifeste source unique recensant, pour chaque RVA, le propriétaire, le
   type d’écriture, la longueur, la fonctionnalité et les octets attendus;
2. un contrôle de build/CI qui refuse les plages chevauchantes et exige un
   propriétaire commun pour les hooks identiques.

Lorsqu’un même RVA doit servir plusieurs fonctionnalités, celles-ci sont
réunies derrière un hook propriétaire unique. Un vrai dispatcher inter-DLL ne
sera envisagé que si D2RLoader fournit explicitement ce service ou si
`plugin-shared` redevient un composant runtime partagé.

## Séquencement retenu

Vincent retient l'option A le 28 juillet 2026 : une fois la fondation de la
tranche 0 disponible, les deux fonctions ethereal sont réunies puis portées
ensemble dans `plugin-items` avant les autres ports du lot. Le prototype
autonome unifié fournit une preuve et un témoin de comparaison; Vincent précise
ensuite qu'il ne doit pas devenir un composant à part ni bloquer le port après
ses validations techniques. Cube Quick Move reste en pause avant son propre
port dans le pack, avec son prototype autonome et ses preuves conservés.

### Tranche 0 — fondation

- ajouter le manifeste des hooks et le contrôle de chevauchement;
- conserver `D2UnitStrc` canonique dans `plugin-shared`;
- introduire des vues/accesseurs minimaux pour les offsets réellement partagés;
- documenter la règle de propriété unique des hooks.

**État local — 28 juillet 2026.** La fondation est implantée dans le clone de
travail séparé `analysis-cache/pluginpack-foundation`, épinglé sur le commit
officiel `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`; la référence gouvernée reste
intacte. Le checkpoint de fondation inventorie 57 plages d’écriture uniques sur les
cinq DLL avec owner, feature, type, RVA, longueur, octets attendus et fichier
source. CMake valide ce manifeste à la configuration et avant chaque build; un
fixture négatif prouve qu’un chevauchement inter-DLL bloque effectivement la
validation. Les anciens alias de `plugin-quests` qui écrivaient deux fois sept
mêmes sites sont consolidés en une seule écriture par RVA. Le champ canonique
`D2UnitStrc+0x04` devient `classId`, avec assertions d’offset et accesseurs
partagés minimaux pour le type et le class ID. Le port ethereal ajoute ensuite
cinq sites, portant le manifeste courant à 62 propriétaires uniques. La
compilation Release x64 des cinq DLL réussit et les trois CTest passent. Le
checkpoint code `2a23212`
(`Establish PluginPack foundation prototype`) est poussé sur le fork
`RuffDood/D2RL-Plugins`, branche `codex/pluginpack-foundation`, synchronisée à
`0/0`; l’`origin` local reste la référence officielle eezstreet en lecture seule.
Le port ethereal conjoint est ensuite livré dans le checkpoint `a51c865`
(`Port ethereal rules into plugin-items prototype`), poussé sur la même branche
et vérifié synchronisé à `0/0`.

#### Cold start de fondation — 28 juillet 2026

Les cinq DLL Release du commit `2a2321271b81e58d038319d143b777571686c453`
ont été déployées temporairement dans le profil BKVince, avec un
`D2RPlugins.json` de test qui désactive explicitement toutes les options, y
compris `skills.selfHealParams`. Les plugins RuffnecKk autonomes sont demeurés
en place et n’ont pas été remplacés.

| Cible temporaire | SHA-256 testé | SHA-256 original restauré |
|---|---|---|
| `plugin-items.dll` | `CCB2FA2B9FED219171997E6A3912086DECE6CE31B3FB486E8F1495EFF3F6C041` | `C621BED261D9FA3A91EF547D7CD570E6E3E5A4B757D86BF5F9E380279E9CA130` |
| `plugin-levels.dll` | `00B649002F5D7E91E2714786733CB7F58554248AE200365A9CEEF8385F571A54` | `F16FB9AF9BDBCD4F21A265806249728C6089590B8BBA316B12D2CFCBCD3693D9` |
| `plugin-misc.dll` | `E4B7BC4B855C33C336F10ADD02506FECE79E9BA80424CE4D5EF66BB26D226018` | `4831EDDE0FBBFD3F01EB6F5AAFF7B9EFA476B78AB78850E9830CFEC519B50194` |
| `plugin-quests.dll` | `E4CF72FBCB55539970F9EDB27BC10EC0CDD6E642F90A7006CBA405C4E73E7DDD` | `D1F93BB1AB463656BAFD04A1DA02768846D723292868DCD598B6AE5B047B2AFF` |
| `plugin-skills.dll` | `B4FF5536CD0F63344A78925B6DBDD4AA79A726D3E4FB145246ADD6046D9232BD` | `65CBF15EAD04C59CB654F5DA5A3841BC376353097071DC4FFBBFDB4B80CBAD23` |
| `D2RPlugins.json` | `CB3CB12112C3C28228D2A02D187D5E078D3B6B350C5F3590C077C1E63D5C9D4A` | `AD81571FF3944DBBD7B7F1D47651D61CCC9A5AEEE0DD729D8CB3325909747460` |

Le log frais confirme les cinq IDs `eezstreet-plugin-*` exactement une fois,
`scanned=29 active=27 disabled=2 rejected=0 failed=0`, aucun hook installé par
les cinq DLL désactivées et le démarrage `24/24`. Cette preuve ferme le gate de
chargement de la fondation; elle ne vaut pas validation gameplay des options.
Le premier rollback a rencontré un verrou transitoire sur `plugin-items.dll`;
après arrêt vérifié du processus, le retry a restauré les six hashes originaux
et laissé zéro processus D2R actif. L’incident et sa résolution sont conservés
dans `analysis-cache/pluginpack-foundation-runtime-validation/20260728-191656701/report.json`.

### Tranche 1 — intégrations sans structure gameplay

1. `GambleScreenLimit`, intégré comme option indépendante de `plugin-items`;
2. `GroundItemLabelLimit`, intégré comme option indépendante de `plugin-items`.

### Tranche 2 — petits hooks à ABI opaque

1. `EnhancedDamageMinMaxFix`;
2. `NoEtherealItemTypes` et `Ethereal Item Rules` réunis puis portés ensemble
   dans `plugin-items`; validation gameplay fusionnée ouverte;
3. `BulkSkillPointAllocation`, intégré comme option de `plugin-skills`;
4. `AdvancedItemTooltips`;
5. `PotionAutoPickup`.

L’ordre final de cette tranche peut suivre les plugins effectivement acceptés
par eezstreet sans modifier la fondation.

## Candidats et ports locaux

| Plugin | Version validée | Propriétaire cible | Configuration | État |
|---|---|---|---|---|
| `GambleScreenLimit` | `1.2.0` | `plugin-items.dll` | `items.gambleScreenLimit` dans `D2RPlugins.json` | port intégré, Release et cold starts vanilla/actif validés; régression gameplay intégrée optionnelle |
| `GroundItemLabelLimit` | `1.2.0` intégré | `plugin-items.dll` | `items.groundItemLabels` dans `D2RPlugins.json` | port intégré, Release et cold starts 32/64/128 validés; régression gameplay intégrée ouverte |
| Item Durability / `DurabilityResistance` | `1.2.0` intégré | `plugin-items.dll` | `items.itemDurability` dans `D2RPlugins.json` | port intégré, Release et trois cold starts validés; compatibilité ranged avec Transmogrify externe ouverte |
| Charm Aura Trigger Fix / `CharmInventoryAuras` | `1.6.0` intégré | `plugin-items.dll` | `items.charmAuraTriggerFix` dans `D2RPlugins.json` | port intégré, Release et cold starts vanilla/actif validés; équivalence gameplay town-respawn ouverte |
| `EnhancedDamageMinMaxFix` | `1.2.0` intégré | `plugin-items.dll` | `items.enhancedDamageMinMaxFix` dans `D2RPlugins.json` | port intégré, Release et cold starts vanilla/actif validés; équivalence gameplay intégrée ouverte |
| `BulkSkillPointAllocation` | `1.2.4` intégré | `plugin-skills.dll` | `skills.bulkSkillPointAllocation` dans `D2RPlugins.json` | port intégré, Release et trois cold starts validés; équivalence gameplay intégrée ouverte |
| bloc unifié `EthItemRules` | témoin historique `EtherealItemRules 0.1.0` | `plugin-items.dll` | bloc unique `items.etherealItemRules` | contrat unifié compilé et cold-starté; équivalence gameplay ouverte |
| `RepairCostsCap` | `1.4.0` | `plugin-items.dll` | `items.repairCostsCap` dans `D2RPlugins.json` | fonctionnalité indépendante intégrée et cold-startée; équivalence gameplay ouverte |
| `ExtendedItemStats` | `0.3.17` intégré | `plugin-items.dll` | aucune clé publique | transport 4096 octets et tooltip défilable intégrés; deux cold starts et broker externe validés; équivalence gameplay intégrée ouverte |
| Qty Display Fix / `QtyDisplayIssue` | `1.1.0` intégré | `plugin-items.dll` | `items.qtyDisplayIssue` | patch natif intégré, Release et cold starts vanilla/actif validés; preuve gameplay autonome conservée |
| `ForceLarzukSockets` | `0.1.0` intégré | `plugin-quests.dll` | `quests.larzukSockets` | quinze valeurs vanilla, signature unique, Release et cold start conjoint avec Item Durability validés; preuve gameplay autonome conservée |
| Cube Quick Move Bottom-Right | `0.1.3` intégré | `plugin-misc.dll` | `misc.cubeQuickMoveBottomRight` | 27 producteurs Cube intégrés, Release et cold starts vanilla/actif validés; preuve gameplay autonome `1x3` conservée |
| Equipped Item to Cube | `0.2.0` intégré | `plugin-misc.dll` | `misc.equippedItemToCube` | deux hooks intégrés, composition avec ExtendedItemStats et cold starts vanilla/conjoint validés; preuve gameplay autonome conservée |
| Assign Transmute Hotkey | `0.2.0` intégré | `plugin-misc.dll` | `misc.transmuteHotkey` | deux hooks intégrés, deux régimes du dispatcher UI validés et cold starts vanilla/actifs verts; preuve gameplay autonome conservée |
| `VendorStockRefresh` | `0.1.5` intégré | `plugin-misc.dll` | `misc.vendorStockRefresh` | quatre hooks uniques, séparation du vendor overhaul et cold starts vanilla/actif validés; preuve gameplay autonome conservée |
| `PreventMercDeathInTown` | `0.1.0` intégré | `plugin-misc.dll` | `misc.preventMercDeathInTown` | hook unique, composition avec Item Durability et cold starts vanilla/actif validés; preuve gameplay externe conservée |

Le port `GambleScreenLimit` ajouté après le checkpoint `387dff8` conserve le
contrat autonome 1.2.0 sans créer de nouveau plugin runtime : un bloc
`items.gambleScreenLimit` indépendant, désactivé par défaut, applique 32 lorsqu’il
est activé. Le manifeste porte maintenant 67 sites uniques. Les 5/5 tests et les
deux cold starts ciblés sont verts; la preuve est conservée sous
`analysis-cache/pluginpack-foundation-runtime-validation/20260728-185755322-gamble/`.
La DLL et le JSON de développement portent respectivement les SHA-256
`453F6D548B032104A6A8DF51C959A9215DAA6D08A183715E68281D03B38276D8` et
le défaut joueur `enabled=false`. Le runtime a été restauré après la validation.

Le port `GroundItemLabelLimit 1.2.0` conserve lui aussi une identité de
fonctionnalité, pas de plugin supplémentaire. Son bloc unique
`items.groundItemLabels` est livré avec `enabled=false` et `limit=64`, ce qui
laisse la limite effective vanilla à 32. Lorsqu'il est activé, seuls 64 et 128
sont acceptés. Les sept signatures complètes sont validées avant toute écriture,
et les sept plages synchronisées `0x1516EBE..0x1519AF9` portent un propriétaire
unique dans le manifeste commun.

Le pack complet valide maintenant 74 sites sans chevauchement, les cinq DLL
Release et 6/6 CTest. Le `plugin-items.dll` testé puis copié dans la source
BKVince porte le SHA-256
`E4EB70FB62D01D144CF4AE5F4DB52DFFB069F7F938032CA3280459C37E6F01A5`.
Trois cold starts isolés prouvent les limites effectives 32, 64 et 128 avec
`scanned=30 active=28 disabled=2 rejected=0 failed=0` et startup `24/24`.
Chaque test restaure ensuite le runtime byte-exact et laisse zéro processus.
La preuve consolidée réside dans
`analysis-cache/pluginpack-foundation-runtime-validation/20260728-194227633-ground-label-final/report.json`.
Le JSON autonome et la DLL standalone désactivée ont été retirés de la source
BKVince après fusion; les sources et presets autonomes restent seulement des
témoins de développement. La régression gameplay du port reste une matrice
indépendante et non bloquante.

Le port `EnhancedDamageMinMaxFix 1.2.0` est maintenant une option indépendante
de `plugin-items.dll`. Le défaut `enabled=false` ne pose aucun hook. L'activation
attribue uniquement `STATLIST_EvaluateAndUpdateStat` à `0x2FA430` à cette
fonctionnalité; ses quatre autres RVA sont des appels natifs. L'appel à
`0x373890` demeure compatible avec le hook d'`EthItemRules`, qui délègue ce
retour non éthéré au chemin original. Le manifeste atteint 75 sites sans
chevauchement, les cinq DLL Release compilent et 7/7 CTest passent.

Le `plugin-items.dll` intégré porte le SHA-256
`73FB7D9597BC42C997726FB7FE99623A5A4CB079A1BE050DBE667760E651CF11`.
Les cold starts vanilla et actif atteignent `24/24` avec
`scanned=29 active=27 disabled=2 rejected=0 failed=0`; le premier ne pose aucun
hook `0x2FA430` et le second en pose exactement un, après neutralisation du
témoin autonome. Le runtime est ensuite restauré byte-exact. L'équivalence
gameplay intégrée reste ouverte et indépendante de la preuve melee/throwable
déjà obtenue avec l'autonome.

Le checkpoint code `5ef599f`
(`Integrate Enhanced Damage Min/Max Fix prototype`) est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation` et synchronisé à `0/0`.

`BulkSkillPointAllocation 1.2.4` est maintenant une option indépendante de
`plugin-skills.dll`. Le bloc `skills.bulkSkillPointAllocation` livre
`enabled=false`; aucun de ses trois hooks n'est donc posé avec le JSON joueur.
Le manifeste atteint 78 sites uniques, les cinq DLL Release compilent et 8/8
CTest passent. `plugin-skills.dll` porte le SHA-256
`8C07EBA4D589F6DA05E9CED51EA5C8338AAFD8EF410BDB5EDF1FB30CF1B231B0`.

Le port arbitre aussi le dispatcher partagé `0x843D90` avec
`RemoteStash 0.1.6`. Le premier module chargé en reste l'unique propriétaire et
l'autre utilise un broker exporté; sans RemoteStash, `plugin-skills` possède
seul le site. Les cold starts vanilla, actif conjoint et actif sans RemoteStash
atteignent tous `24/24`, sans rejet ni échec. Le checkpoint code `78d6290`
(`Integrate Bulk Skill Point Allocation prototype`) est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation` et synchronisé à `0/0`.
L'équivalence gameplay intégrée reste une matrice indépendante; le témoin
autonome n'est jamais chargé en même temps que l'option.

Item Durability / `DurabilityResistance 1.2.0` est maintenant une option
indépendante de `plugin-items.dll`. Son bloc strict
`items.itemDurability` livre `enabled=false`, résistances `0/0`, maximum éthéré
`50`, maximum forcé/ranged/diagnostics désactivés : le JSON joueur ne pose donc
aucun hook et conserve le comportement vanilla. Les signatures complètes
uniques couvrent `0x441B10`, `0x2F48C0` et `0x314110`.

Le manifeste atteint 81 sites uniques, les cinq DLL Release compilent et 9/9
CTest passent. `plugin-items.dll` porte le SHA-256
`2DE85E30792C163281EBC9FEE461456128CAD32DE9C76ED2AA0E6D7F3807751C`.
Les cold starts vanilla, résistances/max éthéré actifs avec Transmogrify, puis
ranged actif sans les témoins Transmogrify atteignent tous `24/24`, sans rejet
ni échec. Le scénario conjoint active aussi Repair Costs Cap avec usure `0 %`
et confirme sa composition derrière le hook `0x2F48C0`. Le checkpoint code
`ab5fd53` (`Integrate Item Durability prototype`) est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation` et synchronisé à `0/0`.

Transmogrify reste hors du lot. Il coexiste avec les résistances et le maximum
éthéré lorsque ranged est désactivé; l'extension bows/crossbows partage encore
`0x314110` avec son autonome et exige un broker externe ou un propriétaire
unique pour une activation simultanée. L'équivalence gameplay intégrée reste
ouverte et indépendante de la preuve autonome.

Charm Aura Trigger Fix / `CharmInventoryAuras 1.6.0` est maintenant une option
indépendante de `plugin-items.dll`. Le bloc strict
`items.charmAuraTriggerFix` livre `enabled=false` et `diagnostics=false`; le
JSON joueur ne pose donc aucun de ses hooks et conserve le comportement
vanilla. L'activation attribue exactement `0x502D00`, `0x491960` et `0x42D2C0`
à cette fonctionnalité. Les retours uniques `0x486AE5`, `0x4B35A6` et
`0x4B6650` filtrent respectivement transition, récupération du cadavre et
réapparition en ville. Son appel de `CheckItemType` à `0x373890` ne revendique
pas ce hook : il traverse le propriétaire actuel et compose donc avec
`EthItemRules`.

Le manifeste atteint 84 sites uniques, les cinq DLL Release compilent et 10/10
CTest passent. `plugin-items.dll` porte le SHA-256
`4C59668F0011256DFFD19B695CCA053D8A4000397BDF57D36B6F9A08181A210F`.
Les cold starts vanilla et actif atteignent `24/24` avec
`scanned=29 active=27 disabled=2 rejected=0 failed=0`; le premier ne pose aucun
hook Charm Aura et le second en pose exactement trois après neutralisation du
témoin autonome. Le runtime, le JSON et `CharmInventoryAuras.dll` sont ensuite
restaurés byte-exact, sans processus résiduel. Le checkpoint code `25811d8`
(`Integrate Charm Aura Trigger Fix prototype`) est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation` et synchronisé à `0/0`.
Les preuves gameplay transition/oskill et récupération du cadavre du témoin
autonome restent acquises; la réapparition en ville et l'équivalence intégrée
restent des matrices séparées ouvertes.

`ExtendedItemStats 0.3.17` est maintenant une infrastructure interne de
`plugin-items.dll`, sans clé dans `D2RPlugins.json`. Le contrat complet du
témoin est conservé : transport fragmenté `EIT1` jusqu'à `4096` octets,
réassemblage borné, capture des descriptions de stats tronquées, fenêtrage du
tooltip, entrées souris/clavier/manette et overlay hébergé par `FloatingDamage`
ou rendu par son repli D3D12. Les deux exports
`ExtendedItemStatsOwnsTooltipPipeline` et
`ExtendedItemStatsTransformTooltip` sont désormais portés directement par
`plugin-items.dll`; avec les trois exports D2RLoader, la DLL expose exactement
cinq symboles publics.

Les dix signatures strictes sont uniques dans l'image canonique 92777 et le
manifeste commun atteint 94 sites sans chevauchement. Les cinq DLL Release
compilent et 12/12 CTest passent. `plugin-items.dll` mesure `622592` octets et
porte le SHA-256
`C63DB14DD3715009DB4044B39FFB470543BCCA01A1F454C45DDC737266F52161`.
Le cold start sans Transmogrify pose les dix hooks sous le propriétaire
`eezstreet-plugin-items`, atteint `24/24` et termine avec
`scanned=27 active=26 disabled=1 rejected=0 failed=0`; il utilise le template
vanilla SHA-256
`65768DFADB33D2E3EE29FA7BFAB6D48665A09AF50BFBC46F674C333EEA4BD2B1`.

Transmogrify reste hors du lot. Son autonome reçoit seulement un pont de
découverte compatible : il cherche les exports historiques dans
`ExtendedItemStats.dll`, puis dans `plugin-items.dll`. Cette symétrie rend
l'ordre de chargement indifférent. Dans le cold start conjoint, Transmogrify
chargé en premier possède seul `0x2BD480`, `plugin-items` délègue ce site et
pose les neuf autres hooks; le démarrage atteint `24/24` avec
`scanned=29 active=27 disabled=2 rejected=0 failed=0`. Sans Transmogrify,
`plugin-items` reprend seul le dixième hook. Le témoin autonome
`ExtendedItemStats.dll` est retiré de la source après fusion; ses sources
restent comme témoin de comparaison. Tous les fichiers runtime touchés sont
restaurés byte-exact et aucun processus ne demeure. Les preuves sont conservées
sous
`analysis-cache/pluginpack-foundation-runtime-validation/20260728-extended-item-stats/report.json`.

Le checkpoint code `c4566d4` (`Integrate Extended Item Stats prototype`) est
poussé sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation` et synchronisé à
`0/0`. Les tests et cold starts ne remplacent pas la matrice gameplay : le
survol intégré, le cycle de vie d'un objet de 4096 octets et les cas
solo/hôte/joiner demeurent des régressions indépendantes et non bloquantes.

Qty Display Fix / `QtyDisplayIssue 1.1.0` est maintenant une option indépendante
de `plugin-items.dll` sous `items.qtyDisplayIssue`. Le bloc strict accepte
uniquement `enabled` et livre `false`; le JSON joueur conserve donc les octets
vanilla `75 0F` à `0x2BE118`. Lorsqu'il est activé, l'unique signature de 33
octets à `0x2BE103` est vérifiée puis seul ce branchement devient `90 90`, ce
qui rétablit l'appel au formateur natif de quantité sans texte personnalisé.

Le manifeste commun atteint 95 sites à propriétaire unique. Les cinq DLL
Release compilent et 13/13 CTest passent. `plugin-items.dll` mesure `626688`
octets et porte le SHA-256
`44D9992D67D1F1E02A5E2050DB7C461FD1D6E8DD8DD00204859EDBC075EEC006`.
Les cold starts actif et vanilla atteignent `24/24` avec
`scanned=26 active=25 disabled=1 rejected=0 failed=0`; la lecture mémoire du
processus confirme respectivement `90 90` et `75 0F`. Le témoin autonome et
son JSON sont retirés de la source BKVince après fusion, tandis que les sources,
le ZIP et la preuve gameplay autonome restent disponibles. Le runtime est
restauré byte-exact, sans processus ni crash frais. Le checkpoint code
`d6562d4` (`Integrate Qty Display Fix prototype`) est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation` et synchronisé à `0/0`.

Les deux refus de commandes console observés pendant ces démarrages proviennent
de vieux témoins autonomes Gamble et Enhanced Damage encore déployés dans le
profil de test; ils ne touchent pas le patch Qty et devront être retirés avec
tous les DLL autonomes remplacés avant le cold start final du lot complet.
L'équivalence visuelle intégrée reste une régression indépendante; le témoin
1.1.0 a déjà validé en jeu le cas central du stackable socketé.

`ForceLarzukSockets 0.1.0` est maintenant intégré dans `plugin-quests.dll` sous
`quests.larzukSockets`. Le bloc strict contient les trois difficultés et les
cinq qualités avec les quantités visibles vanilla : Magic `1..2`, puis Rare,
Set, Unique et Crafted à `1`. Les règles `null` ou absentes délèguent à vanilla;
le diagnostic est désactivé dans le template joueur.

Le hook propriétaire unique demeure `ITEMS_AddSockets` à `0x375560`. Sa
signature a été étendue de 16 à 24 octets et ne possède plus qu'une occurrence
dans l'image canonique 92777. Le manifeste commun atteint 96 sites sans
chevauchement. Les cinq DLL Release compilent, 14/14 CTest passent et
`plugin-quests.dll` mesure `141824` octets avec le SHA-256
`754F372772B5D1E15ACDD74E65E35ECE2BC9E4DECAB63B3496833805A452F387`.

Le premier cold start a découvert une incompatibilité de validation, pas une
collision de hook : Larzuk vérifiait encore les octets originaux de
`GetItemsTxtRecord` après qu'Item Durability avait installé son hook composable
à `0x314110`. Larzuk ne possède pas cette entrée et son appel peut traverser le
hook en conservant le même contrat. La fausse revendication a donc été retirée,
tandis que le build guard, la signature unique du hook Larzuk et huit autres
signatures de helpers demeurent stricts. Le retest avec le hook Durability déjà
présent installe `0x375560`, atteint `24/24` et termine à
`scanned=29 active=27 disabled=2 rejected=0 failed=0`, sans crash frais. Le
runtime est restauré byte-exact, le témoin autonome est retiré de BKVince et ses
sources/preuves gameplay sont conservées. Le checkpoint `be1ff95`
(`Integrate ForceLarzukSockets prototype`) est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation`.

Cube Quick Move `0.1.3` est maintenant intégré dans `plugin-misc.dll` sous
`misc.cubeQuickMoveBottomRight`. Le bloc strict accepte uniquement `enabled` et
livre `false`; le JSON joueur n'installe donc aucune redirection par défaut.

Le port conserve `INVENTORY_FindFreePosition` intact et possède seulement les
27 calls dynamiques ou explicitement page `3` déjà prouvés. Les neuf callers
constamment page `0`, `2` ou `4` restent vanilla. Les 27 séquences originales
ont été revérifiées byte-for-byte dans l'image canonique 92777 et les cinq
signatures de helpers sont uniques. Le manifeste commun atteint 123 sites sans
chevauchement. Les cinq DLL Release compilent, 15/15 CTest passent et
`plugin-misc.dll` mesure `120832` octets avec le SHA-256
`EB2EBA83A19E693A6FCE07D0807407A6B6D23351C23ED13205951312530D1463`.

Le cold start vanilla atteint `24/24` sans redirection. Le cold start actif
atteint aussi `24/24` avec
`scanned=29 active=27 disabled=2 rejected=0 failed=0`; une lecture directe du
processus confirme que les 27 calls convergent vers l'unique relais
`0x3E80000`. Aucun crash frais n'est créé. Le runtime est restauré byte-exact,
le témoin autonome est retiré de BKVince et ses sources, son ZIP et sa preuve
gameplay de l'épée `1x3` placée à `4,3` sont conservés. Le checkpoint
`9835aa8` (`Integrate Cube Quick Move prototype`) est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation`.

Equipped Item to Cube `0.2.0` est maintenant intégré dans `plugin-misc.dll`
sous `misc.equippedItemToCube`. Le bloc strict accepte uniquement `enabled` et
livre `false`, de sorte que le comportement vanilla reste inchangé et qu'aucun
hook ne soit installé par défaut.

Le module possède les deux sites uniques `0x0EE2A0` (file de paquets sortants)
et `0x2CACF0` (clic réel dans un emplacement équipé). Les six signatures des
helpers non possédés ont été revérifiées dans l'image canonique 92777. L'appel
à `ResolveHoveredUnit` à `0x2A7810` traverse volontairement le hook composable
d'ExtendedItemStats : `plugin-items` en est le propriétaire unique, donc le
module Equipped ne revalide pas les octets originaux de cette entrée après son
chargement.

Les cinq DLL Release compilent, 16/16 CTest passent et le manifeste commun
atteint 125 sites sans chevauchement. `plugin-misc.dll` mesure 129024 octets et
porte le SHA-256
`D286CF6D7B74372CC9BFB299F6F8871021297BF18E506E296B486D6FB534E0C3`.
Le cold start vanilla atteint `24/24` sans hook Equipped. Le cold start actif,
avec Cube Quick Move et Equipped Item to Cube simultanément activés, atteint
`24/24` et `scanned=28 active=26 disabled=2 rejected=0 failed=0`. La lecture
mémoire confirme les deux hooks et les 27 appels Cube vers l'unique relais
`0x3E80000`; le hook ExtendedItemStats `0x2A7810` était déjà installé avant le
chargement de `plugin-misc`, sans conflit. Aucun crash frais n'est créé et le
runtime est restauré byte-exact. Le rapport est conservé sous
`analysis-cache/pluginpack-foundation-runtime-validation/20260729-equipped-item-to-cube/report.json`.
Le checkpoint `bc3ea0d` (`Integrate Equipped Item to Cube prototype`) est poussé
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`.

Assign Transmute Hotkey / `TransmuteHotkey 0.2.0` est maintenant integre dans
`plugin-misc.dll` sous `misc.transmuteHotkey`. Le JSON livre `enabled=false`,
`hotkey="CTRL+SHIFT+T"`, `consume=true` et `diagnostics=false`; aucun hook ni
worker d'entree n'est installe par defaut, donc le comportement vanilla demeure
intact.

Le composant possede les deux sites uniques `0x23ECD0` et `0x2CDA90`, puis
appelle le dispatcher UI vivant `0x843D90` sans en revendiquer le hook. Les cinq
DLL Release compilent, le manifeste atteint 127 sites uniques et 17/17 CTest
passent. `plugin-misc.dll` porte le SHA-256
`C6BB54B872415C257D91808CD1CF1D6A36C16F1634BC27C12443B6278D6B1FC3`.
Les checkpoints pack `51b3871` et `183f955` sont pousses sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation`.

Le cold start vanilla atteint `24/24` avec
`scanned=26 active=24 disabled=2 rejected=0 failed=0`. Deux cold starts actifs
prouvent ensuite les deux regimes du dispatcher : avec l'ancien RemoteStash,
Bulk sans confirmation n'exige plus `0x843D90` et les 24 plugins actifs chargent
sans echec; sans RemoteStash, `plugin-skills` possede seul `0x843D90` lorsque la
confirmation Shift est active, et Transmute appelle correctement cette chaine.
Les lectures memoire confirment les deux hooks Transmute, le hook paquet Bulk et
le proprietaire attendu du dispatcher dans chaque regime. Le relais UI MOUSE4
est pret dans les deux cas. Le runtime, les standalones et RemoteStash sont
restaures byte-exact, sans processus restant. Le rapport est conserve sous
`analysis-cache/pluginpack-foundation-runtime-validation/20260729-transmute-hotkey/report.json`.

Le JSON et la DLL standalone gouvernes sont retires de BKVince apres cette
validation integree; les sources, l'archive et la preuve gameplay standalone
restent les oracles de comparaison. La regression gameplay MOUSE4 dans le pack
reste une matrice independante et ne bloque pas les ports suivants.

Vendor Stock Refresh / `VendorStockRefresh 0.1.5` est maintenant integre dans
`plugin-misc.dll` sous `misc.vendorStockRefresh`. Le bloc strict livre
`enabled=false`; aucun hook n'est installe par defaut et le stock vendeur reste
vanilla. Active, la fonctionnalite conserve le bouton natif, son paquet neuf
octets et l'autorite serveur du chemin autonome valide.

Le port possede les quatre hooks `0x2411E0`, `0x502F60`, `0x4B0470` et
`0x10F520`. L'audit a durci le premier guard : son prologue autonome de 32
octets apparaissait 36 fois, tandis que le prefixe minimal de 36 octets est
unique dans l'image canonique. Les trois autres hooks et les cinq helpers non
possedes sont eux aussi uniques. Le vendor overhaul de `plugin-items` demeure
proprietaire de son remplissage et de ses structures de cache; aucune plage ne
se chevauche avec l'action UI de `plugin-misc`.

Les cinq DLL Release compilent, 18/18 CTest passent et le manifeste atteint 131
sites sans chevauchement. `plugin-misc.dll` mesure 175104 octets et porte le
SHA-256 `189115496962912D6B781E4B89A3F4926882513B429830663B3DCD9ADC04CA4F`.
Les cold starts vanilla et actif atteignent `24/24` avec
`scanned=26 active=24 disabled=2 rejected=0 failed=0`; le cas vanilla ne pose
aucun hook Vendor et la lecture memoire active confirme les quatre detours.
Le runtime et les standalones sont restaures byte-exact, sans processus restant.
Le rapport est conserve sous
`analysis-cache/pluginpack-foundation-runtime-validation/20260729-vendor-stock-refresh/report.json`.

Le checkpoint pack `2748167` (`Integrate Vendor Stock Refresh prototype`) est
pousse sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le JSON et la
DLL standalone gouvernes sont retires de BKVince; sources, archive et preuve
gameplay Charsi restent l'oracle. La regression gameplay integree demeure une
matrice independante.

Prevent Merc Death in Town / `PreventMercDeathInTown 0.1.0` est maintenant
intégré dans `plugin-misc.dll` sous `misc.preventMercDeathInTown`. Le bloc strict
livre `enabled=false`; aucun hook n'est installé par défaut et le tick vanilla
reste intact. Activé, le module protège seulement le résultat projeté d'un tick
négatif létal appliqué à l'une des cinq classes de mercenaires dans une room de
ville; il ne guérit pas, reprogramme `STATREGEN` au frame suivant et délègue tous
les cas hors cible au trampoline vanilla.

Le module possède l'unique hook `0x448C00`. Les helpers `0x2F5020`, `0x335E80`,
`0x34B440`, `0x2F0750` et `0x48B720` conservent leurs signatures uniques. Pour
`GetUnitBaseStat` à `0x2F48C0`, le port valide le corps intact à `+5` et appelle
le point d'entrée vivant : il compose donc avec Item Durability lorsque celui-ci
possède déjà le hook. `D2UnitStrc`, le type et le class ID viennent du contrat
canonique partagé, sans structure locale dupliquée.

Les cinq DLL Release compilent, 19/19 CTest passent et le manifeste atteint 132
sites sans chevauchement. `plugin-misc.dll` mesure 183296 octets et porte le
SHA-256 `9FDF2C1B89DC9CC5F3F8CE11EAF02D53242F04A8428CACD02812F5AE7EC723A6`.
Les cold starts vanilla et actif atteignent `24/24` avec
`scanned=26 active=24 disabled=2 rejected=0 failed=0`; le cas vanilla ne pose
aucun hook Prevent, tandis que le cas actif installe `0x448C00` après le hook
Durability déjà vivant à `0x2F48C0`. Le runtime et les quatre standalones misc
temporairement neutralisés sont restaurés byte-exact, sans processus restant.
Le rapport est conservé sous
`analysis-cache/pluginpack-foundation-runtime-validation/20260729-prevent-merc-death-in-town/report.json`.

Le checkpoint pack `4f8b276` (`Integrate Prevent Merc Death in Town prototype`)
est poussé sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le JSON et la
DLL standalone gouvernés sont retirés de BKVince; les sources et la preuve
gameplay externe du 27 juillet restent l'oracle. Les 16 fonctionnalités du lot
canonique sont maintenant portées. La validation finale du pack complet et le
plan de tests joueur constituent le prochain gate; les régressions gameplay
intégrées restent des matrices indépendantes.

Le 28 juillet 2026, Vincent précise que le témoin autonome
`EtherealItemRules 0.1.0` ne fait pas partie du pack final : le propriétaire est
simplement `plugin-items.dll`, au même titre que les autres ports. Il précise
ensuite que les deux entrées historiques ne doivent subsister ni comme plugins,
ni comme blocs JSON distincts. Le composant final unique s’appelle
`EthItemRules`; le bloc unique `items.etherealItemRules` porte le taux, les
exceptions set/indestructible et la liste des ItemTypes exclus. La clé sœur
`items.etherealExclusions` est supprimée du contrat final. Le manifeste conserve
le hook `0x373890` et les quatre opérations `0x4434DF`, `0x443315`, `0x4432F4`
et `0x46D840` sous ce propriétaire unique.

La fusion décrite ici concerne exclusivement les deux anciennes composantes
ethereal. `RepairCostsCap` n’est pas fusionné avec `EthItemRules` : ce sont deux
fonctionnalités sœurs de `plugin-items.dll`, avec deux blocs JSON, deux drapeaux
d’activation, deux modules source, des hooks distincts et des tests distincts.
Leur présence dans la même DLL ne crée ni bloc de configuration commun, ni
dépendance fonctionnelle entre elles.

Le JSON joueur final doit livrer ce bloc unique désactivé, le taux vanilla de
5 %, les exceptions set/indestructible à `false`, une liste d’exclusions vide,
et conserver `skills.selfHealParams=false` afin que le pack entier demeure
passif par défaut. `items.repairCostsCap` est lui aussi livré désactivé, avec
`maximumGold=2147483647`, `durabilityWear.enabled=false` et `chance=0.0` : ses
valeurs restent vanilla même si seul le drapeau principal est activé.

Le checkpoint `387dff8`, poussé le 28 juillet 2026 sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation`, intègre les deux
fonctionnalités indépendantes dans le même `plugin-items.dll`. Il ne reste
aucun bloc `items.etherealExclusions` et aucun patch Repair Costs Cap séparé à
appliquer. Le manifeste commun porte 66 sites uniques, dont les cinq opérations
`EthItemRules` et les quatre écritures `RepairCostsCap`; les cinq targets Release
et les quatre CTest sont verts. `plugin-items.dll` porte le SHA-256
`508D5A77F155D74C02B45C054F642A7CAA3B165287CE28694279561DD9F50EE0`.

La matrice cold-start conjointe est verte. Le cas vanilla ne pose aucun hook des
deux composants. Le cas actif utilise un seul bloc `items.etherealItemRules`,
`items.repairCostsCap.maximumGold=100` et une usure à `1.0`; le propriétaire
`eezstreet-plugin-items` installe le hook éthéré et les trois hooks Repair, puis
reste actif après les patches directs. Les deux cas terminent avec
`scanned=28 active=26 disabled=2 rejected=0 failed=0` et startup `24/24`.
Les anciens propriétaires ethereal et leur patch JSON ont été neutralisés
pendant le test. Les cinq fichiers runtime touchés ont ensuite été restaurés par
SHA-256, sans processus D2R restant. Les preuves sont dans
`analysis-cache/pluginpack-foundation-runtime-validation/20260728-181150128/`.

Repair Costs Cap conserve les métadonnées eezstreet du module propriétaire et
crédite RuffnecKk dans le source, le log et la mission. Son ancien patch local,
qui ciblait directement le commit amont et ne s’appliquait plus proprement sur
`a51c865`, a été retiré de `data-BKVince` et archivé avec les preuves runtime.
La DLL autonome désactivée reste seulement un témoin jusqu’au test d’équivalence
gameplay du port `RepairCostsCap` intégré.

Vincent a validé en jeu le 26 juillet 2026 le comportement final : clic normal
inchangé, Ctrl et Ctrl+Shift en lot natif de cinq, Shift en assign-all natif
immédiat par défaut, puis le même Shift derrière le modal Diablo lorsque
`confirmShiftAllocation=true`. La localisation par clé du `ui.json` actif, le
cold start conjoint et l’absence de collision avec le PluginPack 2.0.1 sont
également acquis. La branche officielle `master` d’eezstreet pointe toujours au
commit audité `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` lors de cette promotion.

Ce statut autorise la préparation d’un port source interne à `plugin-skills`; il
ne vaut ni acceptation amont ni merge déjà effectué. Jusqu’à une décision
commune avec eezstreet, la DLL RuffnecKk et ses JSON autonomes demeurent la
distribution officielle, sans modification ni redistribution d’une DLL tierce.

### Tranche 3 — sous-systèmes complexes

`FloatingDamage`, `CharmInventoryAuras` et `ReviveOverhaul` sont
traités séparément, avec validation de leurs dépendances graphiques, offsets de
structures ou chemins IA.

### Tranche 4 — conflits à arbitrer

- conserver Transmogrify hors lot et exiger un broker externe ou un propriétaire
  unique si son autonome doit coexister avec l'extension ranged d'Item Durability;
- faire de `plugin-items` le propriétaire du hook drop `0x441300` et brancher
  `ConfigurableCharsiReward` sur son callback post-drop;
- valider toutes les combinaisons d’options concernées.

## Contrat de squelette pour les futurs plugins

Toute fonctionnalité RuffnecKk destinée au PluginPack doit être développée dès
le départ comme un composant du pack :

1. choisir d’abord le module propriétaire parmi `plugin-items`, `plugin-levels`,
   `plugin-misc`, `plugin-quests` et `plugin-skills` selon le sous-système touché;
2. ajouter des fichiers `.cpp`/`.h` internes à ce module et les appeler depuis
   son point d’entrée existant, sans créer une nouvelle DLL par défaut;
3. créer une nouvelle target `plugin-*` seulement si aucun module existant n’est
   un propriétaire logique ou si le sous-système exige réellement une isolation;
4. placer la configuration sous la section du module propriétaire dans l’unique
   `D2RPlugins.json`, désactivée par défaut dans le dépôt du pack;
5. ne copier aucune structure gameplay déjà canonique et ne créer aucun TOML
   autonome, sauf décision explicite des mainteneurs de changer le standard;
6. conserver les métadonnées et crédits du plugin propriétaire, tout en créditant
   exactement `RuffnecKk` dans le fichier source de la fonctionnalité, son log et
   la documentation;
7. conserver l’installation globale ou mod-locale, le contrôle strict du build,
   des signatures et de l’ABI, ainsi que l’inventaire des RVA;
8. compiler les cinq DLL Release x64 du pack complet, puis effectuer un démarrage
   à froid et un test fonctionnel avec les autres fonctions actives.

Ce contrat rend les ajouts indépendants côté développement, mais prévisibles à
fusionner : eezstreet peut faire évoluer ses plugins, RuffnecKk peut préparer les
siens, et chaque contribution arrive déjà dans le même format de configuration,
de compilation et de validation.

## Correction du premier pilote — 27 juillet 2026

Vincent confirme que `GroundItemLabelLimit` appartient à la catégorie `items`,
avec `plugin-items.dll` comme propriétaire futur et
`items.groundItemLabels` comme clé prévue. Le premier pilote du 22 juillet sous
`plugin-misc` est donc retiré : ses deux fichiers internes, son appel de
chargement et `misc.groundItemLabels` ne sont plus présents dans le clone de
travail ni dans BKVince.

Le `plugin-misc.dll` reconstruit conserve uniquement le correctif local
indépendant qui déclare `PluginFlags::NativeHooks`, requis par son propre hook
eezstreet. Son SHA-256 source/runtime est
`4831EDDE0FBBFD3F01EB6F5AAFF7B9EFA476B78AB78850E9830CFEC519B50194`.
Le `D2RPlugins.json` sans la section label possède le SHA-256
`3119AC5C934A08287068C832467BF7D5711B4B362AD623665CF605FF2C73A87A` dans
la source et le runtime.

Le cold start du 27 juillet à 07:49 charge `plugin-misc` 2.0.1 avec son drapeau
`0x2` et son hook `0x542F40`, sans aucune ligne d’activation Ground Item Label.
Les résultats sont `20/20` patchsets, 24 plugins actifs, zéro rejet, zéro échec
et démarrage `24/24` en 4,062 secondes. Le processus de test est ensuite fermé.

L’incubation a repris dans la DLL autonome hybride RuffnecKk 1.1.0. Son JSON
autonome accepte uniquement `enabled` et `limit`, avec exactement 64 ou 128;
il cherche la configuration mod-locale avant le repli global, ne crée aucun
TOML et n’était pas déployé dans le profil actif. Cette isolation historique a
pris fin avec le port 1.2.0 autorisé dans `plugin-items.dll` le 28 juillet; le
JSON et le binaire standalone gouvernés ont alors été retirés.

## Contribution SDK prête à envoyer

Le lot court retenu par Vincent est préparé dans
`reverse-engineering/d2r-3.2.92777/sdk-contribution/`. Il répond directement à
la demande de Dimentio d’obtenir une liste de tables et d’offsets sans lui
transmettre le workbench brut :

- `README.md` explique en anglais les preuves et leurs limites dans la voix de
  RuffnecKk;
- `sdk-candidates.json` fournit la version machine-readable;
- `verified-layouts.hpp` fournit uniquement les fragments C++ vérifiés et
  compile en C++20 avec MSVC sous `/W4 /WX`;
- `discord-message.md` est prêt à copier dans la discussion;
- `RuffnecKk-D2RLoader-SDK-notes-92777.zip` contient les trois fichiers
  techniques partageables, SHA-256
  `0A18B93C751025AA5FBB0F7715D76CC0B26CD8717EDD62994F8A8F8582E8D01D`.

Le noyau proposé comprend l’accesseur de contexte à `0x300A90`, les entrées
`skills`, `itemtypes` et `items`, les champs réellement exercés des records
`ItemTypesTxt` et `ItemsTxt`, puis la correction canonique prouvée de
`D2UnitStrc+0x04` : le getter natif `0x349860` le traite comme class/TXT record
ID, pas comme `unitFlags`. Les résultats de `BulkSkillPointAllocation` et
`AdvancedItemTooltips` sont classés séparément comme helpers SDK candidats
(`0x214220`, `0x14C3DA0`, `0x36EAD0`) afin de ne pas les présenter à tort comme
de nouveaux offsets de tables. Le lot exclut les cartographies spéculatives,
les caches locaux, les images du jeu et les patches de balance sans surface SDK
réutilisable.

## Gates d’acceptation

- périmètre des plugins accepté par RuffnecKk et eezstreet;
- un seul type canonique pour chaque structure partagée;
- aucun champ ajouté sans preuve 92777 et assertion d’offset;
- manifeste exhaustif des écritures et hooks;
- zéro chevauchement non arbitré;
- compilation Release x64 de chaque configuration CMake;
- plugins installables globalement ou dans un mod, sans `ModScopedOnly`;
- build, signatures et ABI strictement refusés sur cible incompatible;
- cold-start avec toutes les options désactivées, puis activation isolée;
- matrice de coexistence par paire pour les plugins touchant un même
  sous-système;
- validations fonctionnelles en jeu sur `D2R.exe 3.2.92777`.
