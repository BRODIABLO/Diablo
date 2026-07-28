# Mission courante

Dernière mise à jour : 28 juillet 2026

## Priorité active

[Cube Quick Move Bottom-Right — D2R 3.2](cube-quick-move-bottom-right-3.2.md)

État : après les échecs 0.1.0 à 0.1.2, `CubeQuickMove 0.1.3` possède les `27`
appels capables de transporter la page Cube `3`. Vincent confirme le 28 juillet
2026 que le même Ctrl-clic place désormais visiblement l'épée `1x3` en bas à
droite. Le contrôle client `0x15A25C` et le producteur de placement `0x15F94F`
sont observés; le paquet `0x54` porte les coordonnées `4,3` et le serveur accepte
le transfert. Le prototype autonome est prêt pour intégration dans
`plugin-misc.dll`. Vincent retient le
séquencement A et confirme
le propriétaire futur `plugin-misc.dll` sous la catégorie `misc`, avec la clé
`misc.cubeQuickMoveBottomRight`. Le wrapper 0.1.3 balaie directement la grille d'occupation native depuis
le coin inférieur droit et conserve un repli vanilla sûr. Son cold start
mod-local atteint `24/24`, `20/20` patchsets et `28` plugins actifs sans rejet
ni échec, avec les hashes source/runtime identiques.

Vendor Stock Refresh a été déclaré réglé par Vincent le 27 juillet 2026 et
retiré de la ROADMAP active. Sa mission et ses artefacts restent conservés comme
preuves techniques. Le pilote I8 d’atmosphère macabre des Catacombes est
abandonné par Vincent le 28 juillet 2026 avant toute implantation, après des
essais ElevenLabs gratuits non concluants; aucun WAV n’a été ingéré dans
BKVince. Equipped Item to Cube redevient planifié immédiatement après le gate
gameplay de la présente mission.

## Prochain gate

Porter séparément la politique 0.1.3 dans `plugin-misc.dll` sous
`misc.cubeQuickMoveBottomRight`, puis obtenir le même cold start et le même
témoin épée avant de retirer l'autonome. Rejouer ensuite les Ctrl-déplacements
`1x1`, `2x1`, `1x2`, `2x2` et `2x3` dans un Cube vide, fragmenté et plein, puis
fermer les contrôles de non-régression : placement manuel, autres conteneurs,
manette, persistance et autorité hôte/joiner. L’archive DLL + JSON reste une candidate technique tant
que cette matrice gameplay n’est pas observée.

## Frontière Git

Le lot Cube Quick Move comprend la mission, le pointeur courant, le workstream,
la ROADMAP, les preuves 92777, les sources, le JSON, la DLL autonome, l’archive
candidate et le cadastre. Les artefacts Vendor Stock Refresh et tous les autres
changements concurrents sont préservés. Aucun commit ni push n’est inclus.
