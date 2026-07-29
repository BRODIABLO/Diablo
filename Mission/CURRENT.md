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
`misc.transmuteHotkey`, `misc.vendorStockRefresh` puis
`misc.preventMercDeathInTown` dans
`plugin-misc.dll`. Les configurations, hooks et tests restent indépendants;
leur présence dans les DLL du pack ne crée aucun bloc fonctionnel commun.

Les 16 fonctionnalités canoniques sont maintenant portées et la validation
finale du pack complet est verte. Les cinq DLL Release proviennent du même
commit, 19/19 CTest passent et le manifeste porte 132 sites à propriétaire
unique. Le JSON joueur SHA-256
`F4077ED0C2CBF69AF5C394BAE704A647117668F3ED21BB256FC76F69E3A7532D`
conserve les effets configurables désactivés et les valeurs vanilla.

Les cold starts vanilla et conjoint actif atteignent tous deux `24/24` avec
`scanned=16 active=14 disabled=2 rejected=0 failed=0`. Le cas actif prouve
notamment Item Durability `0x2F48C0` avec Prevent `0x448C00`, EthItemRules
`0x373890`, ExtendedItemStats `0x2A7810`, le relais Cube `0x3E80000`, ainsi que
RemoteStash propriétaire de `0x843D90` et Transmogrify externe propriétaire de
`0x314110`. Aucun crash frais n'est créé. Les 93 fichiers runtime contrôlés sont
restaurés par SHA-256 et aucun processus ne reste. Le document public
`RUFFNECKK-INTEGRATION.md` du fork contient l'inventaire, les limites de
compatibilité et le plan de tests joueur par sous-système.

## Prochain gate

Faire les smoke tests gameplay intégrés par lots — UI/limites, règles d'items,
Cube, services de ville, quêtes/skills — puis soumettre la branche à la revue
d'eezstreet. Il n'est pas nécessaire de relancer le jeu une fois par plugin;
chaque observation gameplay non exécutée reste néanmoins ouverte et ne doit pas
être déduite des cold starts techniques.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`5b56690` (`Document complete PluginPack validation`), poussé
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
