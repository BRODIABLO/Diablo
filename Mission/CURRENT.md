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

Le cold start de fondation est maintenant acquis sur BKVince et D2R 3.2.92777.
La configuration temporaire a désactivé toutes les options, y compris
`skills.selfHealParams`; les cinq identifiants eezstreet ont chargé exactement
une fois, le résumé a donné `scanned=29 active=27 disabled=2 rejected=0 failed=0`,
aucun hook de fondation n’a été installé et le démarrage a atteint `24/24`. Les
témoins autonomes sont restés en place. Après le test, les cinq DLL et
`D2RPlugins.json` d’origine ont été restaurés avec leurs SHA-256 exacts et aucun
processus D2R n’est resté actif. Le rapport local complet est
`analysis-cache/pluginpack-foundation-runtime-validation/20260728-191656701/report.json`.

Cube Quick Move 0.1.3 reste conservé et validé comme autonome, mais son port dans
`plugin-misc.dll` est mis en pause avant mutation du pack. Transmogrify demeure
exclu de ce lot. Après la fondation, l’Option A impose de réunir d’abord
`NoEtherealItemTypes` et `Ethereal Item Rules` dans un composant autonome commun,
puis de porter ensemble `items.etherealExclusions` et
`items.etherealItemRules` dans `plugin-items.dll`.

## Prochain gate

Figer le nom public et le contrat JSON unique du sous-système ethereal autonome
commun, réauditer le hook `0x373890` et les quatre sites du patch, puis réunir
`NoEtherealItemTypes` et `Ethereal Item Rules` dans ce témoin hybride avant tout
port du lot. Son propriétaire futur dans le pack reste `plugin-items.dll`, sous
`items.etherealExclusions` et `items.etherealItemRules`.

## Frontière Git

Le code de fondation vit uniquement dans le clone séparé
`analysis-cache/pluginpack-foundation`; le dépôt principal ne reçoit que la mise
à jour gouvernée de la mission et de la ROADMAP. Les autres changements actifs
du workspace, notamment Cube Quick Move et Configurable Charsi Reward, restent
préservés. Le code est committé et poussé séparément sur le fork; aucun
déploiement runtime persistant n’est inclus et le profil BKVince a été restauré
après le cold start temporaire.
