# Mission courante

Dernière mise à jour : 28 juillet 2026

## Priorité active

[Intégration sélective au PluginPack d’eezstreet](eezstreet-pluginpack-integration.md)

État : Vincent a ouvert explicitement la fondation du PluginPack. Le laboratoire
modifiable est isolé sous `analysis-cache/pluginpack-foundation` au commit amont
exact `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`; la référence officielle
gouvernée demeure intacte. Le checkpoint de fondation `2a23212` est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation`.

Les checkpoints poussés `a51c865`, `387dff8`, `8d41581`, `a4d8dbb` et
`5ef599f` ont successivement porté le bloc unique
`items.etherealItemRules`, puis les fonctionnalités indépendantes
`items.repairCostsCap`, `items.gambleScreenLimit`, `items.groundItemLabels` et
`items.enhancedDamageMinMaxFix` dans `plugin-items.dll`. Les configurations,
hooks et tests restent indépendants; leur présence dans la même DLL ne crée
aucun bloc fonctionnel commun.

`EnhancedDamageMinMaxFix 1.2.0` est maintenant intégré sous
`items.enhancedDamageMinMaxFix`. Le template joueur livre `enabled=false`, donc
le comportement vanilla reste inchangé et le hook `0x2FA430` n'est pas posé par
défaut. Lorsqu'elle est activée, la fonctionnalité est l'unique propriétaire de
ce hook; ses quatre autres RVA sont seulement des appels natifs. Son appel au
résolveur `0x373890` reste compatible avec le hook d'`EthItemRules`, qui délègue
ce chemin au comportement original.

Les cinq DLL Release compilent, 7/7 CTest passent et le manifeste porte 75 sites
à propriétaire unique. `plugin-items.dll` porte le SHA-256
`73FB7D9597BC42C997726FB7FE99623A5A4CB079A1BE050DBE667760E651CF11` dans
le build et la source BKVince. Les cold starts vanilla et actif atteignent tous
deux `24/24` avec `scanned=29 active=27 disabled=2 rejected=0 failed=0`; ils
prouvent respectivement zéro et exactement un hook `0x2FA430`. Le témoin
autonome a été neutralisé pendant ces tests, puis la DLL, le JSON et le runtime
ont été restaurés byte-exact. L'équivalence gameplay intégrée reste ouverte; la
preuve melee/throwable autonome est conservée. Transmogrify demeure exclu de ce
lot.

## Prochain gate

Porter ensuite `BulkSkillPointAllocation 1.2.3` sous l'option indépendante
`skills.bulkSkillPointAllocation` de `plugin-skills.dll`. Le composant ethereal
est déjà intégré; il ne constitue pas une étape séparée. Les régressions
gameplay ouvertes des ports précédents restent des matrices indépendantes et ne
bloquent pas ce prochain port.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`5ef599f` (`Integrate Enhanced Damage Min/Max Fix prototype`), synchronisé à `0/0`
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
