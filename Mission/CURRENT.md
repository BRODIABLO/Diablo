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

Prevent Merc Death in Town `0.1.0` est maintenant intégré sous
`misc.preventMercDeathInTown`. Le template livre `enabled=false`; aucun hook
n'est installé par défaut. Activé, le port possède uniquement `0x448C00`,
emploie le `D2UnitStrc` canonique et valide le corps intact de
`GetUnitBaseStat+5`, ce qui lui permet de composer avec le hook Item Durability
déjà vivant à `0x2F48C0`.

Les cinq DLL Release compilent, 19/19 CTest passent et le manifeste porte 132
sites à propriétaire unique. `plugin-misc.dll` porte le SHA-256
`9FDF2C1B89DC9CC5F3F8CE11EAF02D53242F04A8428CACD02812F5AE7EC723A6`.
Les cold starts vanilla et actif atteignent `24/24`, zéro rejet et zéro échec;
la lecture mémoire confirme le hook du cas actif et la coexistence avec
DurabilityResistance. Le runtime est restauré byte-exact, sans processus
restant. La DLL et le JSON standalone gouvernés sont retirés de BKVince; leurs
sources et la preuve gameplay externe demeurent l'oracle.

## Prochain gate

Exécuter la validation finale du pack complet : cinq DLL Release au même commit,
JSON joueur vanilla, neutralisation temporaire de tous les standalones remplacés,
cold starts vanilla et conjoint actif, audit des hooks partagés, puis produire le
plan de tests joueur par sous-système. Les régressions gameplay restent des
matrices indépendantes et ne doivent pas être déclarées réussies par inférence.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`4f8b276` (`Integrate Prevent Merc Death in Town prototype`), poussé
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
