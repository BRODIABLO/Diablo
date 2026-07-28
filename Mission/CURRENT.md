# Mission courante

Dernière mise à jour : 28 juillet 2026

## Priorité active

[Intégration sélective au PluginPack d’eezstreet](eezstreet-pluginpack-integration.md)

État : Vincent a ouvert explicitement la fondation du PluginPack. Le laboratoire
modifiable est isolé sous `analysis-cache/pluginpack-foundation` au commit amont
exact `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`; la référence officielle
gouvernée demeure intacte. La tranche 0 locale possède maintenant un manifeste
de 57 écritures/hook sites à propriétaire unique, un gate CMake bloquant et un
test négatif de chevauchement. Les sept doubles écritures historiques de
`plugin-quests` sont consolidées. `D2UnitStrc+0x04` est corrigé en `classId`
selon la preuve 92777 à `0x349860`, avec deux accesseurs partagés minimaux. Les
cinq DLL compilent en Release x64 et les deux CTest réussissent.
Le checkpoint code `2a23212` (`Establish PluginPack foundation prototype`) est
poussé et synchronisé sur le fork `RuffDood/D2RL-Plugins`, branche
`codex/pluginpack-foundation`.

Cube Quick Move 0.1.3 reste conservé et validé comme autonome, mais son port dans
`plugin-misc.dll` est mis en pause avant mutation du pack. Transmogrify demeure
exclu de ce lot. Après la fondation, l’Option A impose de réunir d’abord
`NoEtherealItemTypes` et `Ethereal Item Rules` dans un composant autonome commun,
puis de porter ensemble `items.etherealExclusions` et
`items.etherealItemRules` dans `plugin-items.dll`.

## Prochain gate

Cold-starter les cinq DLL de fondation avec toutes les options désactivées, sans
remplacer les témoins autonomes; vérifier cinq plugins actifs, zéro rejet et zéro
échec. Commencer ensuite le sous-système ethereal autonome commun retenu par
l’Option A, avant tout autre port du lot.

## Frontière Git

Le code de fondation vit uniquement dans le clone séparé
`analysis-cache/pluginpack-foundation`; le dépôt principal ne reçoit que la mise
à jour gouvernée de la mission et de la ROADMAP. Les autres changements actifs
du workspace, notamment Cube Quick Move et Configurable Charsi Reward, restent
préservés. Le code est committé et poussé séparément sur le fork; aucun
déploiement runtime n’est inclus.
