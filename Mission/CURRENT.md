# Mission courante

Dernière mise à jour : 28 juillet 2026

## Priorité active

[Intégration sélective au PluginPack d’eezstreet](eezstreet-pluginpack-integration.md)

État : Vincent a ouvert explicitement la fondation du PluginPack. Le laboratoire
modifiable est isolé sous `analysis-cache/pluginpack-foundation` au commit amont
exact `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`; la référence officielle
gouvernée demeure intacte. Le checkpoint de fondation `2a23212` est poussé sur
`RuffDood/D2RL-Plugins:codex/pluginpack-foundation`.

Le sous-système ethereal est maintenant porté conjointement dans
`plugin-items.dll` comme une fonctionnalité ordinaire du pack. Il expose les
deux options distinctes `items.etherealExclusions` et
`items.etherealItemRules`; il ne crée ni nouvelle DLL, ni nouvel identifiant de
plugin, ni chargeur de configuration autonome. Le manifeste passe de 57 à 62
sites à propriétaire unique. Les cinq DLL compilent en Release x64 et les trois
CTest réussissent. `plugin-items.dll` porte le SHA-256
`FB4AD6015DFCEE02FFECEFBF2056155F4EB1E7E8CAB9A54AE13DC0B0BBBC823D`.

Le `D2RPlugins.json` destiné aux joueurs conserve vanilla par défaut : les deux
sections ethereal sont désactivées, le taux visible vaut 5 %, les exceptions
set/indestructible valent `false`, et `skills.selfHealParams` est désormais
également désactivé. Le cold start de ce fichier n'installe aucun hook ethereal;
les cinq plugins eezstreet chargent, avec
`scanned=28 active=26 disabled=2 rejected=0 failed=0` et startup `24/24`.

Un second cold start active ensemble les exclusions `belt`/`armo`, le taux 6 %,
les sets et les objets indestructibles. Les anciens propriétaires autonomes et
le patch JSON sont neutralisés pendant le test : seul
`eezstreet-plugin-items` installe le hook gouverné `0x373890`; les appels de
patch réussissent, les cinq plugins du pack restent actifs, aucun plugin
ethereal autonome n'est chargé, le résumé reste à zéro rejet/échec et le
démarrage atteint `24/24`. Les neuf fichiers runtime d'origine ont ensuite été
restaurés et vérifiés par SHA-256 après un retry de verrou tardif; aucun processus
D2R/D2RLoader ne reste actif. Les preuves locales sont sous
`analysis-cache/pluginpack-foundation-runtime-validation/20260728-170819582/`.

Le prototype autonome `EtherealItemRules 0.1.0` et les deux implémentations
initiales restent seulement des témoins de comparaison jusqu'à l'équivalence
gameplay; ils ne constituent plus un composant spécial ni un gate préalable au
port. Cube Quick Move 0.1.3 demeure en pause avant son propre merge et
Transmogrify reste exclu de ce lot.

## Prochain gate

Valider maintenant l'équivalence gameplay du module fusionné : code précis et
parent, descendant non ciblé, taux 6 %, sets, sept uniques indestructibles et
base sans durabilité, Cube/`ALWAYSETH`, solo, hôte/joiner et save/reload. Les
plugins autonomes servent de témoins si un résultat diffère; leur fonctionnement
déjà acquis n'a pas à être requalifié intégralement.

## Frontière Git

Le code du pack vit dans le clone séparé
`analysis-cache/pluginpack-foundation`. Le checkpoint ethereal `a51c865`
(`Port ethereal rules into plugin-items prototype`) est poussé et synchronisé à
`0/0` sur `RuffDood/D2RL-Plugins:codex/pluginpack-foundation`. Le dépôt principal
conserve les sources et binaires autonomes comme témoins ainsi que les preuves
gouvernées de mission/ROADMAP. Les autres changements actifs du workspace
restent préservés. Le runtime BKVince a été restauré exactement à son état
antérieur au test; le pack de travail n'y est donc pas laissé déployé.
