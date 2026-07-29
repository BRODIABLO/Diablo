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
`5ef599f`, `78d6290` et `ab5fd53` ont successivement porté le bloc unique
`items.etherealItemRules`, puis les fonctionnalités indépendantes
`items.repairCostsCap`, `items.gambleScreenLimit`, `items.groundItemLabels` et
`items.enhancedDamageMinMaxFix` et `items.itemDurability` dans
`plugin-items.dll`, puis `skills.bulkSkillPointAllocation` dans
`plugin-skills.dll`. Les configurations, hooks et tests restent indépendants;
leur présence dans les DLL du pack ne crée aucun bloc fonctionnel commun.

Item Durability / `DurabilityResistance 1.2.0` est maintenant intégré sous
`items.itemDurability`. Le template joueur livre `enabled=false`, résistances
`0/0`, maximum éthéré `50`, maximum forcé/ranged/diagnostics désactivés; aucun
hook Durability n'est donc posé par défaut et le comportement vanilla reste
inchangé.

Les cinq DLL Release compilent, 9/9 CTest passent et le manifeste porte 81 sites
à propriétaire unique. `plugin-items.dll` porte le SHA-256
`2DE85E30792C163281EBC9FEE461456128CAD32DE9C76ED2AA0E6D7F3807751C`.
Les cold starts vanilla, résistances/max éthéré actifs avec Transmogrify, puis
ranged actif sans Transmogrify atteignent tous `24/24`, sans rejet ni échec.
Repair Costs Cap actif avec usure `0 %` confirme sa composition derrière le
hook `0x2F48C0`. Le runtime, le JSON, les DLL et les témoins neutralisés sont
restaurés byte-exact; aucun processus ne reste. L'équivalence gameplay intégrée
demeure ouverte. Transmogrify reste hors lot; son autonome partage `0x314110`
avec l'extension ranged, qui exige encore un broker externe ou un propriétaire
unique pour une activation simultanée.

## Prochain gate

Porter ensuite Charm Aura Trigger Fix sous l'option indépendante
`items.charmAuraTriggerFix` de `plugin-items.dll`. Les régressions gameplay
ouvertes des ports précédents restent des matrices indépendantes et ne bloquent
pas ce prochain port.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`ab5fd53` (`Integrate Item Durability prototype`), synchronisé à `0/0`
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
