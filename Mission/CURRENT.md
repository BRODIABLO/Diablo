# Mission courante

Dernière mise à jour : 28 juillet 2026

## Priorité active

[Intégration sélective au PluginPack d’eezstreet](eezstreet-pluginpack-integration.md)

État : Vincent a ouvert explicitement la fondation du PluginPack. Le laboratoire
modifiable est isolé sous `analysis-cache/pluginpack-foundation` au commit amont
exact `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`; la référence officielle
gouvernée demeure intacte. Le checkpoint de fondation `2a23212` est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation`.

Les checkpoints poussés `a51c865`, `387dff8`, `8d41581`, `a4d8dbb`,
`5ef599f`, `78d6290`, `ab5fd53`, `25811d8`, `c4566d4`, `d6562d4`,
`be1ff95`, `9835aa8`, `bc3ea0d`, `51b3871` et `183f955` ont successivement porté le bloc unique
`items.etherealItemRules`, puis les fonctionnalités indépendantes
`items.repairCostsCap`, `items.gambleScreenLimit`, `items.groundItemLabels` et
`items.enhancedDamageMinMaxFix`, `items.itemDurability` et
`items.charmAuraTriggerFix`, l'infrastructure `ExtendedItemStats` et
`items.qtyDisplayIssue` dans
`plugin-items.dll`, `skills.bulkSkillPointAllocation` dans `plugin-skills.dll`,
`quests.larzukSockets` dans `plugin-quests.dll`, puis
`misc.cubeQuickMoveBottomRight`, `misc.equippedItemToCube` puis
`misc.transmuteHotkey` dans
`plugin-misc.dll`. Les configurations, hooks et tests restent indépendants;
leur présence dans les DLL du pack ne crée aucun bloc fonctionnel commun.

Transmute Hotkey `0.2.0` est maintenant integre sous `misc.transmuteHotkey`.
Le template livre `enabled=false` et `CTRL+SHIFT+T`; aucun hook ni worker
d'entree n'est installe par defaut. Le port possede les deux sites uniques
`0x23ECD0` et `0x2CDA90`, puis appelle le dispatcher UI vivant `0x843D90` sans
en revendiquer le hook.

Les cinq DLL Release compilent, 17/17 CTest passent et le manifeste porte 127
sites a proprietaire unique. `plugin-misc.dll` porte le SHA-256
`C6BB54B872415C257D91808CD1CF1D6A36C16F1634BC27C12443B6278D6B1FC3`.
Le cold start vanilla et deux cold starts actifs atteignent `24/24`, zero rejet
et zero echec. Ils prouvent successivement le defaut passif, la coexistence avec
l'ancien RemoteStash lorsque Bulk n'utilise pas la confirmation, puis la
propriete du dispatcher par `plugin-skills` lorsque cette confirmation est
active. Le runtime, les standalones et RemoteStash sont restaures byte-exact;
aucun processus ne reste. Les sources et preuves autonomes demeurent l'oracle
gameplay, mais la DLL et le JSON standalone ont ete retires de BKVince.

## Prochain gate

Porter ensuite Vendor Stock Refresh dans `plugin-misc.dll` sous
`misc.vendorStockRefresh`, en conservant son temoin autonome valide comme
oracle. Les regressions gameplay ouvertes des ports precedents restent des
matrices independantes et ne bloquent pas ce prochain port.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`183f955` (`Compose Bulk allocation without confirmation UI hook`), pousse
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
