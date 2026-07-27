# Mission courante

Dernière mise à jour : 27 juillet 2026

## Priorité active

[Vendor Stock Refresh — D2R 3.2](vendor-stock-refresh-3.2.md)

État : `VendorStockRefresh 0.1.5` est déployé en DLL autonome hybride, sans
override de layout. Le bouton natif unique est centré dynamiquement sous l’or
des boutiques normales et Charsi est confirmé visuellement et
fonctionnellement. Le test autonome, les trois exports, les hashes
source/runtime et le cold start mod-local sont verts.

Le chantier Rift/Cow a été déclaré terminé par Vincent le 27 juillet 2026 et
retiré de la ROADMAP active. Sa mission reste conservée comme preuve technique.

## Prochain gate

Confirmer que le bouton original retrouve sa position et son comportement
vanilla dans l’écran de gamble. Tester ensuite un vendeur de mode `0`, puis un
layout vendeur réellement modifié ou agrandi avant d’étendre la matrice à tous
les actes, à la manette, au spam de clics et au réseau hôte/joiner.

## Frontière Git

Le lot Vendor Stock Refresh comprend sa mission, son entrée ROADMAP, son
workstream, `VendorStockRefresh-src/`, `VendorStockRefresh.json` et la DLL
autonome. Les missions Rift/Cow, Transmogrify, Readable Items, Extended Item
Stats et les autres changements concurrents sont préservés.
