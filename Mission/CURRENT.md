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
`5ef599f`, `78d6290`, `ab5fd53`, `25811d8` et `c4566d4` ont successivement porté le bloc unique
`items.etherealItemRules`, puis les fonctionnalités indépendantes
`items.repairCostsCap`, `items.gambleScreenLimit`, `items.groundItemLabels` et
`items.enhancedDamageMinMaxFix`, `items.itemDurability` et
`items.charmAuraTriggerFix` et l'infrastructure `ExtendedItemStats` dans
`plugin-items.dll`, puis `skills.bulkSkillPointAllocation` dans
`plugin-skills.dll`. Les configurations, hooks et tests restent indépendants;
leur présence dans les DLL du pack ne crée aucun bloc fonctionnel commun.

`ExtendedItemStats 0.3.17` est maintenant compilé directement dans
`plugin-items.dll` sans clé JSON publique. Le transport fixe `4096` octets, le
réassemblage borné et le tooltip défilable conservent le contrat autonome;
les objets et tooltips vanilla ordinaires restent inchangés. Les deux exports
de broker historiques rejoignent les trois exports D2RLoader de la DLL.

Les cinq DLL Release compilent, 12/12 CTest passent et le manifeste porte 94
sites à propriétaire unique. `plugin-items.dll` porte le SHA-256
`C63DB14DD3715009DB4044B39FFB470543BCCA01A1F454C45DDC737266F52161`.
Le cold start sans Transmogrify pose les dix hooks et atteint `24/24` avec le
JSON vanilla. Le cold start conjoint laisse l'unique hook tooltip à
Transmogrify et pose les neuf autres dans le pack; le pont externe recherche
désormais aussi les exports de `plugin-items.dll`, sans faire entrer
Transmogrify dans le lot. Les deux démarrages terminent sans rejet ni échec.
Le runtime et sa configuration sont restaurés byte-exact; aucun processus ne
reste. Le témoin DLL autonome est retiré de la source, ses sources et preuves
gameplay demeurent disponibles.

## Prochain gate

Porter ensuite Qty Display Fix / `QtyDisplayIssue` dans `plugin-items.dll` sous
`items.qtyDisplayIssue`, avec un template vanilla. Les régressions gameplay
ouvertes des ports précédents restent des matrices indépendantes et ne bloquent
pas ce prochain port.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`c4566d4` (`Integrate Extended Item Stats prototype`), synchronisé à `0/0`
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
