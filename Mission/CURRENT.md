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
`be1ff95`, `9835aa8`, `bc3ea0d`, `51b3871`, `183f955` et `2748167` ont successivement porté le bloc unique
`items.etherealItemRules`, puis les fonctionnalités indépendantes
`items.repairCostsCap`, `items.gambleScreenLimit`, `items.groundItemLabels` et
`items.enhancedDamageMinMaxFix`, `items.itemDurability` et
`items.charmAuraTriggerFix`, l'infrastructure `ExtendedItemStats` et
`items.qtyDisplayIssue` dans
`plugin-items.dll`, `skills.bulkSkillPointAllocation` dans `plugin-skills.dll`,
`quests.larzukSockets` dans `plugin-quests.dll`, puis
`misc.cubeQuickMoveBottomRight`, `misc.equippedItemToCube` puis
`misc.transmuteHotkey` puis `misc.vendorStockRefresh` dans
`plugin-misc.dll`. Les configurations, hooks et tests restent indépendants;
leur présence dans les DLL du pack ne crée aucun bloc fonctionnel commun.

Vendor Stock Refresh `0.1.5` est maintenant integre sous
`misc.vendorStockRefresh`. Le template livre `enabled=false`; aucun de ses
quatre hooks n'est installe par defaut. Le port durcit le guard de configuration
du panel a 36 octets uniques et conserve les chemins natifs UI, paquet et
serveur sans chevaucher le vendor overhaul de `plugin-items`.

Les cinq DLL Release compilent, 18/18 CTest passent et le manifeste porte 131
sites a proprietaire unique. `plugin-misc.dll` porte le SHA-256
`189115496962912D6B781E4B89A3F4926882513B429830663B3DCD9ADC04CA4F`.
Les cold starts vanilla et actif atteignent `24/24`, zero rejet et zero echec;
la lecture memoire confirme les quatre hooks du cas actif. Le runtime et les
standalones sont restaures byte-exact, sans processus restant. La DLL et le JSON
standalone sont retires de BKVince; leurs sources et la preuve gameplay Charsi
demeurent l'oracle.

## Prochain gate

Porter ensuite Prevent Merc Death in Town dans `plugin-misc.dll` sous
`misc.preventMercDeathInTown`, en conservant son temoin autonome valide comme
oracle. Les regressions gameplay ouvertes des ports precedents restent des
matrices independantes et ne bloquent pas ce prochain port.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`2748167` (`Integrate Vendor Stock Refresh prototype`), pousse
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
