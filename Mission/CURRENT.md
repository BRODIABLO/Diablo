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
`be1ff95` et `9835aa8` ont successivement porté le bloc unique
`items.etherealItemRules`, puis les fonctionnalités indépendantes
`items.repairCostsCap`, `items.gambleScreenLimit`, `items.groundItemLabels` et
`items.enhancedDamageMinMaxFix`, `items.itemDurability` et
`items.charmAuraTriggerFix`, l'infrastructure `ExtendedItemStats` et
`items.qtyDisplayIssue` dans
`plugin-items.dll`, `skills.bulkSkillPointAllocation` dans `plugin-skills.dll`,
`quests.larzukSockets` dans `plugin-quests.dll`, puis
`misc.cubeQuickMoveBottomRight` dans `plugin-misc.dll`. Les configurations, hooks et tests restent indépendants;
leur présence dans les DLL du pack ne crée aucun bloc fonctionnel commun.

Cube Quick Move `0.1.3` est maintenant intégré sous
`misc.cubeQuickMoveBottomRight`. Le template livre `enabled=false`, donc aucun
call-site n'est redirigé par défaut. L'activation conserve les neuf producteurs
prouvés non-Cube et possède les 27 appels dynamiques ou explicites capables de
porter la page Cube.

Les cinq DLL Release compilent, 15/15 CTest passent et le manifeste porte 123
sites à propriétaire unique. `plugin-misc.dll` porte le SHA-256
`EB2EBA83A19E693A6FCE07D0807407A6B6D23351C23ED13205951312530D1463`.
Les cold starts vanilla et actif atteignent `24/24` et
`scanned=29 active=27 disabled=2 rejected=0 failed=0`, sans crash frais. La
lecture mémoire confirme que les 27 calls actifs convergent vers l'unique relais
`0x3E80000`. Le runtime est restauré byte-exact et aucun processus ne reste. La
DLL et le JSON autonomes Cube sont retirés de BKVince; leurs sources, ZIP et
preuve gameplay de l'épée `1x3` à `4,3` restent disponibles.

## Prochain gate

Porter ensuite Equipped Item to Cube dans `plugin-misc.dll` sous
`misc.equippedItemToCube`, en conservant son témoin autonome comme oracle. Les
régressions gameplay ouvertes des ports précédents restent des matrices
indépendantes et ne bloquent pas ce prochain port.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`9835aa8` (`Integrate Cube Quick Move prototype`), poussé
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
