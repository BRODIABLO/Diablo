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
`be1ff95`, `9835aa8` et `bc3ea0d` ont successivement porté le bloc unique
`items.etherealItemRules`, puis les fonctionnalités indépendantes
`items.repairCostsCap`, `items.gambleScreenLimit`, `items.groundItemLabels` et
`items.enhancedDamageMinMaxFix`, `items.itemDurability` et
`items.charmAuraTriggerFix`, l'infrastructure `ExtendedItemStats` et
`items.qtyDisplayIssue` dans
`plugin-items.dll`, `skills.bulkSkillPointAllocation` dans `plugin-skills.dll`,
`quests.larzukSockets` dans `plugin-quests.dll`, puis
`misc.cubeQuickMoveBottomRight` puis `misc.equippedItemToCube` dans
`plugin-misc.dll`. Les configurations, hooks et tests restent indépendants;
leur présence dans les DLL du pack ne crée aucun bloc fonctionnel commun.

Equipped Item to Cube `0.2.0` est maintenant intégré sous
`misc.equippedItemToCube`. Le template livre `enabled=false`; aucun de ses deux
hooks n'est donc installé par défaut. Le port possède strictement la file de
paquets `0x0EE2A0` et le clic d'équipement `0x2CACF0`. Il appelle le hook
composable `ResolveHoveredUnit` à `0x2A7810` déjà possédé par
`plugin-items/ExtendedItemStats`, sans revendiquer ni revalider ses octets
originaux.

Les cinq DLL Release compilent, 16/16 CTest passent et le manifeste porte 125
sites à propriétaire unique. `plugin-misc.dll` porte le SHA-256
`D286CF6D7B74372CC9BFB299F6F8871021297BF18E506E296B486D6FB534E0C3`.
Les cold starts vanilla et actif atteignent `24/24` et
`scanned=28 active=26 disabled=2 rejected=0 failed=0`, sans crash frais. La
lecture mémoire confirme les deux hooks actifs et les 27 appels Cube convergeant
vers l'unique relais `0x3E80000`; le hook ExtendedItemStats à `0x2A7810` était
déjà chargé avant `plugin-misc`, sans conflit. Le runtime est restauré
byte-exact et aucun processus ne reste. Le témoin autonome demeure l'oracle de
gameplay et ses changements de travail concurrents restent hors de ce
checkpoint.

## Prochain gate

Porter ensuite Assign Transmute Hotkey dans `plugin-misc.dll` sous
`misc.transmuteHotkey`, en conservant son témoin autonome validé comme oracle. Les
régressions gameplay ouvertes des ports précédents restent des matrices
indépendantes et ne bloquent pas ce prochain port.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`bc3ea0d` (`Integrate Equipped Item to Cube prototype`), poussé
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
