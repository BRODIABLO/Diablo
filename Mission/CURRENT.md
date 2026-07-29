# Mission courante

Dernière mise à jour : 28 juillet 2026

## Priorité active

[Intégration sélective au PluginPack d’eezstreet](eezstreet-pluginpack-integration.md)

État : Vincent a ouvert explicitement la fondation du PluginPack. Le laboratoire
modifiable est isolé sous `analysis-cache/pluginpack-foundation` au commit amont
exact `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`; la référence officielle
gouvernée demeure intacte. Le checkpoint de fondation `2a23212` est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation`.

Les checkpoints poussés `a51c865`, `387dff8`, `8d41581` et `a4d8dbb` ont
successivement porté le bloc unique `items.etherealItemRules`, puis les
fonctionnalités indépendantes `items.repairCostsCap`,
`items.gambleScreenLimit` et `items.groundItemLabels` dans `plugin-items.dll`.
Les configurations, hooks et tests restent indépendants; leur présence dans la
même DLL ne crée aucun bloc fonctionnel commun.

`GroundItemLabelLimit 1.2.0` est maintenant intégré dans le même module sous
`items.groundItemLabels`, sans nouvelle DLL ni nouvel identifiant runtime. Le
template joueur livre `enabled=false` et `limit=64`, donc la limite effective
reste vanilla à 32. L’activation accepte exactement 64 ou 128. Les sept
signatures sont vérifiées ensemble avant toute écriture et le manifeste commun
porte maintenant 74 sites à propriétaire unique.

Les cinq DLL Release compilent, 6/6 CTest passent et `plugin-items.dll` porte le
SHA-256 `E4EB70FB62D01D144CF4AE5F4DB52DFFB069F7F938032CA3280459C37E6F01A5`
dans le build et la source BKVince. Trois cold starts isolés prouvent les limites
effectives 32, 64 et 128, chacun avec
`scanned=30 active=28 disabled=2 rejected=0 failed=0` et startup `24/24`.
Le runtime est restauré byte-exact après chaque cas et aucun processus ne reste.
Le JSON autonome et la DLL standalone désactivée ont été retirés de la source;
les sources et presets autonomes demeurent seulement des témoins de
développement. Les régressions gameplay restent des matrices indépendantes et
non bloquantes. Transmogrify demeure exclu de ce lot.

## Prochain gate

Porter ensuite `EnhancedDamageMinMaxFix` sous l’option indépendante
`items.enhancedDamageMinMaxFix` de `plugin-items.dll`, premier petit hook opaque
de la tranche 2. Les régressions gameplay ouvertes pour `EthItemRules`, Repair
Costs Cap, Gamble Screen Limit et Ground Item Label Limit restent séparées et
ne bloquent pas ce prochain port.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`a4d8dbb` (`Integrate Ground Item Label Limit prototype`), synchronisé à `0/0`
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
