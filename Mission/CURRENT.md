# Mission courante

Dernière mise à jour : 28 juillet 2026

## Priorité active

[Cube Quick Move Bottom-Right — D2R 3.2](cube-quick-move-bottom-right-3.2.md)

État : prototype autonome hybride `CubeQuickMove 0.1.0` implanté, compilé en
Release x64 et déployé dans BKVince. Vincent retient le séquencement A et confirme
le propriétaire futur `plugin-misc.dll` sous la catégorie `misc`, avec la clé
`misc.cubeQuickMoveBottomRight`. Le wrapper ne redirige que l’appel Cube unique
au site `0x4BBA73`; les portées mod-locale et globale ont chacune franchi un cold
start `24/24` avec les hashes source/runtime identiques.

Vendor Stock Refresh a été déclaré réglé par Vincent le 27 juillet 2026 et
retiré de la ROADMAP active. Sa mission et ses artefacts restent conservés comme
preuves techniques. Le pilote I8 d’atmosphère macabre des Catacombes est
désormais intercalé après le gate gameplay de la présente mission; Equipped Item
to Cube demeure planifié immédiatement après ce pilote.

## Prochain gate

Valider visuellement en jeu les Ctrl-déplacements `1x1`, `2x1`, `1x2`, `2x2` et
`2x3` dans un Cube vide, fragmenté et plein, puis fermer les contrôles de
non-régression : placement manuel, autres conteneurs, manette, persistance et
autorité hôte/joiner. L’archive DLL + JSON reste une candidate technique tant
que cette matrice gameplay n’est pas observée.

## Frontière Git

Le lot Cube Quick Move comprend la mission, le pointeur courant, le workstream,
la ROADMAP, les preuves 92777, les sources, le JSON, la DLL autonome, l’archive
candidate et le cadastre. Les artefacts Vendor Stock Refresh et tous les autres
changements concurrents sont préservés. Aucun commit ni push n’est inclus.
