# Mission courante

Dernière mise à jour : 29 juillet 2026

## Priorité active

[Intégration sélective au PluginPack d’eezstreet](eezstreet-pluginpack-integration.md)

État : Vincent a ouvert explicitement la fondation du PluginPack. Le laboratoire
modifiable est isolé sous `analysis-cache/pluginpack-foundation` au commit amont
exact `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`; la référence officielle
gouvernée demeure intacte. Le checkpoint de fondation `2a23212` est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation`.

Les checkpoints poussés `a51c865`, `387dff8`, `8d41581`, `a4d8dbb`,
`5ef599f`, `78d6290`, `ab5fd53`, `25811d8`, `c4566d4`, `d6562d4`,
`be1ff95`, `9835aa8`, `bc3ea0d`, `51b3871`, `183f955`, `2748167` et
`4f8b276` ont successivement porté le bloc unique
`items.etherealItemRules`, puis les fonctionnalités indépendantes
`items.repairCostsCap`, `items.gambleScreenLimit`, `items.groundItemLabels` et
`items.enhancedDamageMinMaxFix`, `items.itemDurability` et
`items.charmAuraTriggerFix`, l'infrastructure `ExtendedItemStats` et
`items.qtyDisplayIssue` dans
`plugin-items.dll`, `skills.bulkSkillPointAllocation` dans `plugin-skills.dll`,
`quests.larzukSockets` dans `plugin-quests.dll`, puis
`misc.cubeQuickMoveBottomRight`, `misc.equippedItemToCube` puis
`misc.transmuteHotkey` et `misc.preventMercDeathInTown` dans
`plugin-misc.dll`; `items.vendorStockRefresh` appartient à `plugin-items.dll`.
Les configurations, hooks et tests restent indépendants;
leur présence dans les DLL du pack ne crée aucun bloc fonctionnel commun.

Les 16 fonctionnalités canoniques sont portées. Le durcissement final corrige
aussi trois réglages historiques d'eezstreet qui étaient lus sans être appliqués :
les caps physique, élémentaire et d'absorption. Les cinq DLL Release compilent,
`22/22` CTest passent et le manifeste porte `135/135` sites à propriétaire
unique, chacun relié exactement une fois à une écriture source gouvernée. Le
JSON joueur SHA-256
`660664986598F076A45843C2712672EA933375E726388F057192BD35F71F8D5F`
conserve les valeurs vanilla et seulement les cinq fixes explicitement retenus
actifs par défaut.

Un cold start isolé avec uniquement les cinq DLL et le JSON public atteint
`24/24`, avec `scanned=5 active=5 disabled=0 rejected=0 failed=0` et aucun
memory patch chargé. Un second démarrage prouve dans le processus vivant les
trois caps témoins : absorption `41`, physique `51`, élémentaire `96`. Les
`36/36` fichiers temporairement neutralisés sont restaurés par SHA-256 après
chaque essai et aucun processus ne reste. Le document public du fork décrit le
pack comme un produit indépendant et conserve le plan de tests joueur par
sous-système.

## Prochain gate

Faire les smoke tests gameplay intégrés par lots — UI/limites, règles d'items,
Cube, services de ville, quêtes/skills — puis soumettre la branche à la revue
d'eezstreet. Il n'est pas nécessaire de relancer le jeu une fois par plugin;
chaque observation gameplay non exécutée reste néanmoins ouverte et ne doit pas
être déduite des cold starts techniques.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`4d482c6` (`Harden PluginPack integration prototype`), sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le dépôt principal
conserve les preuves gouvernées et les trois RVA ajoutés. Le runtime
BKVince a été restauré exactement à son état antérieur aux tests; le pack de
travail n’y est donc pas laissé déployé.
