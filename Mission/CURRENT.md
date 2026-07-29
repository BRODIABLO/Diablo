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
`5ef599f`, `78d6290`, `ab5fd53` et `25811d8` ont successivement porté le bloc unique
`items.etherealItemRules`, puis les fonctionnalités indépendantes
`items.repairCostsCap`, `items.gambleScreenLimit`, `items.groundItemLabels` et
`items.enhancedDamageMinMaxFix`, `items.itemDurability` et
`items.charmAuraTriggerFix` dans
`plugin-items.dll`, puis `skills.bulkSkillPointAllocation` dans
`plugin-skills.dll`. Les configurations, hooks et tests restent indépendants;
leur présence dans les DLL du pack ne crée aucun bloc fonctionnel commun.

Charm Aura Trigger Fix / `CharmInventoryAuras 1.6.0` est maintenant intégré sous
`items.charmAuraTriggerFix`. Le template joueur livre `enabled=false` et
`diagnostics=false`; aucun de ses trois hooks n'est donc posé par défaut et le
comportement vanilla reste inchangé. Lorsqu'il est actif, les retours uniques
sélectionnent seulement transition, récupération du cadavre et réapparition en
ville. L'appel de `CheckItemType` traverse le propriétaire actuel de `0x373890`
et compose avec `EthItemRules` sans revendiquer un second hook.

Les cinq DLL Release compilent, 10/10 CTest passent et le manifeste porte 84
sites à propriétaire unique. `plugin-items.dll` porte le SHA-256
`4C59668F0011256DFFD19B695CCA053D8A4000397BDF57D36B6F9A08181A210F`.
Les cold starts vanilla et actif atteignent tous deux `24/24`, avec
`scanned=29 active=27 disabled=2 rejected=0 failed=0`; ils montrent
respectivement zéro puis exactement trois hooks Charm Aura. Le runtime, le JSON
et le témoin autonome sont restaurés byte-exact; aucun processus ne reste. Les
preuves gameplay autonomes transition/oskill et récupération du cadavre restent
acquises, tandis que town-respawn et l'équivalence intégrée restent ouvertes.

## Prochain gate

Porter ensuite `ExtendedItemStats` comme infrastructure interne de
`plugin-items.dll`, sans clé JSON publique. Les régressions gameplay ouvertes
des ports précédents restent des matrices indépendantes et ne bloquent pas ce
prochain port.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`25811d8` (`Integrate Charm Aura Trigger Fix prototype`), synchronisé à `0/0`
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
