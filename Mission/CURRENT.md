# Mission courante

Dernière mise à jour : 28 juillet 2026

## Priorité active

[Intégration sélective au PluginPack d’eezstreet](eezstreet-pluginpack-integration.md)

État : Vincent a ouvert explicitement la fondation du PluginPack. Le laboratoire
modifiable est isolé sous `analysis-cache/pluginpack-foundation` au commit amont
exact `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`; la référence officielle
gouvernée demeure intacte. Le checkpoint de fondation `2a23212` est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation`.

Le checkpoint `a51c865` a porté conjointement les hooks ethereal dans
`plugin-items.dll` comme une fonctionnalité ordinaire du pack, sans nouvelle
DLL, nouvel identifiant de plugin ni chargeur autonome. Son manifeste passe de
57 à 62 sites à propriétaire unique; les cinq DLL compilent en Release x64 et
les trois CTest réussissent. `plugin-items.dll` porte le SHA-256
`FB4AD6015DFCEE02FFECEFBF2056155F4EB1E7E8CAB9A54AE13DC0B0BBBC823D`.

Vincent clarifie toutefois que le découpage JSON de ce checkpoint est incorrect.
`Exclude ItemTypes from Rolling Ethereal` et `Ethereal Item Rules` forment un
seul composant `EthItemRules`, dans un seul bloc
`items.etherealItemRules`. L’exclusion d’ItemTypes est un champ interne de ce
bloc; la clé sœur `items.etherealExclusions` doit disparaître.

Le `D2RPlugins.json` testé conservait vanilla par défaut : les anciennes
sections ethereal étaient désactivées, le taux visible valait 5 %, les exceptions
set/indestructible valaient `false`, et `skills.selfHealParams` était également
désactivé. Le cold start de ce fichier n'installait aucun hook ethereal;
les cinq plugins eezstreet chargent, avec
`scanned=28 active=26 disabled=2 rejected=0 failed=0` et startup `24/24`.

Un second cold start historique active ensemble les exclusions `belt`/`armo`, le taux 6 %,
les sets et les objets indestructibles. Les anciens propriétaires autonomes et
le patch JSON sont neutralisés pendant le test : seul
`eezstreet-plugin-items` installe le hook gouverné `0x373890`; les appels de
patch réussissent, les cinq plugins du pack restent actifs, aucun plugin
ethereal autonome n'est chargé, le résumé reste à zéro rejet/échec et le
démarrage atteint `24/24`. Les neuf fichiers runtime d'origine ont ensuite été
restaurés et vérifiés par SHA-256 après un retry de verrou tardif; aucun processus
D2R/D2RLoader ne reste actif. Les preuves locales sont sous
`analysis-cache/pluginpack-foundation-runtime-validation/20260728-170819582/`.

Le checkpoint `387dff8` porte maintenant le contrat final en un seul bloc
`items.etherealItemRules` et intègre `items.repairCostsCap` dans le même
`plugin-items.dll`. Le manifeste valide 66 sites sans chevauchement, les
cinq targets Release et 4/4 CTest sont verts. Les cold starts vanilla exact et
conjoint actif atteignent `24/24` avec zéro rejet/échec; le runtime a été restauré
par SHA-256. Les prototypes autonomes restent seulement des témoins. Cube Quick
Move 0.1.3 demeure en pause et Transmogrify reste exclu de ce lot.

## Prochain gate

Reprendre l’équivalence gameplay du code fusionné : exclusions par code précis et
parent, descendant non ciblé, taux 6 %, sets, sept uniques indestructibles et
base sans durabilité, Cube/`ALWAYSETH`, plafond Repair individuel/Repair All,
usure et persistance, solo, hôte/joiner et save/reload.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le checkpoint unifié `387dff8`
(`Unify EthItemRules and integrate Repair Costs Cap prototype`) est poussé et
synchronisé à `0/0` sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le dépôt principal
conserve les témoins autonomes et les preuves gouvernées de mission/ROADMAP. Les
autres changements actifs du workspace restent préservés. Le runtime BKVince a
été restauré exactement à son état
antérieur au test; le pack de travail n'y est donc pas laissé déployé.
