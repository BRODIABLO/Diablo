# Mission courante

Dernière mise à jour : 27 juillet 2026

## Priorité active

[Cube Quick Move Bottom-Right — D2R 3.2](cube-quick-move-bottom-right-3.2.md)

État : mission de conception et de preuve active, sans implantation. Vincent
retient le séquencement A et confirme le propriétaire futur `plugin-misc.dll`
sous la catégorie `misc`, avec la clé `misc.cubeQuickMoveBottomRight`. Le chemin
92777 vers la page Cube `3`, la règle native dépendant de la hauteur et l’appel
unique à `INVENTORY_FindFreePosition` au site `0x4BBA73` sont identifiés.

Vendor Stock Refresh a été déclaré réglé par Vincent le 27 juillet 2026 et
retiré de la ROADMAP active. Sa mission et ses artefacts restent conservés comme
preuves techniques. Equipped Item to Cube demeure planifié juste après la
présente mission.

## Prochain gate

Fermer l’audit ABI/signatures et la portée exacte du call-site `0x4BBA73`, puis
présenter le plan du wrapper autonome `CubeQuickMove.dll` avant toute
implantation. La validation devra couvrir toutes les dimensions d’objet, un Cube
fragmenté ou plein, les contrôles manuels, les autres conteneurs, la manette et
l’autorité hôte/joiner.

## Frontière Git

Le lot courant est documentaire : nouvelle mission, pointeur courant, workstream,
ROADMAP et cadastre. Les artefacts Vendor Stock Refresh et tous les autres
changements concurrents sont préservés; aucun code, JSON, binaire, commit ou push
n’est inclus.
