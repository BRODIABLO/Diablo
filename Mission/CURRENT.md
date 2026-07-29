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
`5ef599f` et `78d6290` ont successivement porté le bloc unique
`items.etherealItemRules`, puis les fonctionnalités indépendantes
`items.repairCostsCap`, `items.gambleScreenLimit`, `items.groundItemLabels` et
`items.enhancedDamageMinMaxFix` dans `plugin-items.dll`, puis
`skills.bulkSkillPointAllocation` dans `plugin-skills.dll`. Les configurations,
hooks et tests restent indépendants; leur présence dans les DLL du pack ne crée
aucun bloc fonctionnel commun.

`BulkSkillPointAllocation 1.2.4` est maintenant intégré sous
`skills.bulkSkillPointAllocation`. Le template joueur livre `enabled=false`, le
lot Ctrl 5, Shift immédiat sans modal et les diagnostics désactivés; aucun hook
Bulk n'est donc posé par défaut et le comportement vanilla reste inchangé.

Les cinq DLL Release compilent, 8/8 CTest passent et le manifeste porte 78 sites
à propriétaire unique. `plugin-skills.dll` porte le SHA-256
`8C07EBA4D589F6DA05E9CED51EA5C8338AAFD8EF410BDB5EDF1FB30CF1B231B0`.
Le dispatcher `0x843D90` est arbitré avec `RemoteStash 0.1.6` par un broker
bidirectionnel : le premier module chargé reste l'unique propriétaire, l'autre
s'enregistre comme consommateur. Les cold starts vanilla, actif conjoint et
actif sans RemoteStash atteignent tous `24/24`, sans rejet ni échec. Le runtime,
les cinq DLL, le JSON, RemoteStash et le témoin Bulk sont restaurés byte-exact;
aucun processus ne reste. L'équivalence gameplay intégrée demeure ouverte; la
preuve Ctrl/Shift autonome est conservée. Transmogrify demeure exclu de ce lot.

## Prochain gate

Porter ensuite Item Durability / `DurabilityResistance` sous l'option
indépendante `items.itemDurability` de `plugin-items.dll`. Les régressions
gameplay ouvertes des ports précédents restent des matrices indépendantes et ne
bloquent pas ce prochain port.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`78d6290` (`Integrate Bulk Skill Point Allocation prototype`), synchronisé à `0/0`
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
