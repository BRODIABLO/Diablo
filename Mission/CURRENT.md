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
`5ef599f`, `78d6290`, `ab5fd53`, `25811d8`, `c4566d4`, `d6562d4` et
`be1ff95` ont successivement porté le bloc unique
`items.etherealItemRules`, puis les fonctionnalités indépendantes
`items.repairCostsCap`, `items.gambleScreenLimit`, `items.groundItemLabels` et
`items.enhancedDamageMinMaxFix`, `items.itemDurability` et
`items.charmAuraTriggerFix`, l'infrastructure `ExtendedItemStats` et
`items.qtyDisplayIssue` dans
`plugin-items.dll`, `skills.bulkSkillPointAllocation` dans `plugin-skills.dll`,
puis `quests.larzukSockets` dans `plugin-quests.dll`. Les configurations, hooks et tests restent indépendants;
leur présence dans les DLL du pack ne crée aucun bloc fonctionnel commun.

Qty Display Fix / `QtyDisplayIssue 1.1.0` est maintenant intégré sous
`items.qtyDisplayIssue`. Le template livre `enabled=false` : aucune écriture
n'est faite et les octets vanilla `75 0F` restent à `0x2BE118`. L'activation
vérifie l'unique signature de 33 octets puis écrit seulement `90 90`, laissant
D2R produire la ligne de quantité native.

`ForceLarzukSockets 0.1.0` est maintenant intégré sous
`quests.larzukSockets`. Le JSON joueur contient les quinze règles vanilla :
Magic `1..2`, puis Rare, Set, Unique et Crafted à `1`, dans chacune des trois
difficultés. Sa signature Larzuk a été étendue de 16 à 24 octets et est unique
à `0x375560` dans l'image canonique 92777.

Les cinq DLL Release compilent, 14/14 CTest passent et le manifeste porte 96
sites à propriétaire unique. `plugin-quests.dll` porte le SHA-256
`754F372772B5D1E15ACDD74E65E35ECE2BC9E4DECAB63B3496833805A452F387`.
Le premier cold start a révélé que Larzuk revalidait à tort le helper
`GetItemsTxtRecord` après le hook composable d'Item Durability. Cette fausse
revendication a été retirée : le retest avec le hook Durability déjà présent
atteint `24/24`, installe Larzuk et termine à
`scanned=29 active=27 disabled=2 rejected=0 failed=0`, sans crash frais. Le
runtime est restauré byte-exact et aucun processus ne reste. La DLL et le JSON
autonomes Larzuk sont retirés de BKVince; leurs sources et preuves gameplay
restent disponibles.

## Prochain gate

Porter ensuite Cube Quick Move Bottom-Right dans `plugin-misc.dll` sous
`misc.cubeQuickMoveBottomRight`, en conservant le clic vanilla et le témoin
autonome comme oracle gameplay. Les régressions gameplay ouvertes des ports
précédents restent des matrices indépendantes et ne bloquent pas ce prochain
port.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le dernier checkpoint poussé est
`be1ff95` (`Integrate ForceLarzukSockets prototype`), poussé
sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le clone de fondation
est propre.
Le dépôt principal conserve la DLL gouvernée, la configuration vanilla et les
preuves de mission/ROADMAP. Le runtime BKVince a été restauré exactement à son
état antérieur aux tests; le pack de travail n’y est donc pas laissé déployé.
