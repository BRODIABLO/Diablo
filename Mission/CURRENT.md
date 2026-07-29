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
`5ef599f`, `78d6290`, `ab5fd53`, `25811d8`, `c4566d4` et `d6562d4` ont successivement porté le bloc unique
`items.etherealItemRules`, puis les fonctionnalités indépendantes
`items.repairCostsCap`, `items.gambleScreenLimit`, `items.groundItemLabels` et
`items.enhancedDamageMinMaxFix`, `items.itemDurability` et
`items.charmAuraTriggerFix`, l'infrastructure `ExtendedItemStats` et
`items.qtyDisplayIssue` dans
`plugin-items.dll`, puis `skills.bulkSkillPointAllocation` dans
`plugin-skills.dll`. Les configurations, hooks et tests restent indépendants;
leur présence dans les DLL du pack ne crée aucun bloc fonctionnel commun.

Qty Display Fix / `QtyDisplayIssue 1.1.0` est maintenant intégré sous
`items.qtyDisplayIssue`. Le template livre `enabled=false` : aucune écriture
n'est faite et les octets vanilla `75 0F` restent à `0x2BE118`. L'activation
vérifie l'unique signature de 33 octets puis écrit seulement `90 90`, laissant
D2R produire la ligne de quantité native.

Les cinq DLL Release compilent, 13/13 CTest passent et le manifeste porte 95
sites à propriétaire unique. `plugin-items.dll` porte le SHA-256
`44D9992D67D1F1E02A5E2050DB7C461FD1D6E8DD8DD00204859EDBC075EEC006`.
Les cold starts actif et vanilla atteignent `24/24`, sans rejet ni échec; la
lecture mémoire confirme `90 90` puis `75 0F`. Le runtime est restauré
byte-exact et aucun processus ne reste. Le DLL et le JSON autonomes Qty sont
retirés de BKVince; leurs sources, ZIP et preuve gameplay restent disponibles.
Les doublons de commandes Gamble/Enhanced encore présents dans le profil de
test sont consignés pour le nettoyage final des témoins remplacés.

## Prochain gate

Porter ensuite `ForceLarzukSockets` dans `plugin-quests.dll` sous
`quests.larzukSockets`, avec les quinze valeurs vanilla déjà validées dans le
témoin autonome. Les régressions gameplay ouvertes des ports précédents restent
des matrices indépendantes et ne bloquent pas ce prochain port.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`d6562d4` (`Integrate Qty Display Fix prototype`), synchronisé à `0/0`
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
