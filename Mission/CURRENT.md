# Mission courante

Dernière mise à jour : 27 juillet 2026

## Priorité active

[Vendor Stock Refresh — D2R 3.2](vendor-stock-refresh-3.2.md)

État : `VendorStockRefresh 0.1.5` est déployé en DLL autonome hybride, sans
override de layout. Le bouton natif unique est centré dynamiquement sous l’or
des boutiques normales et Charsi est confirmé visuellement et
fonctionnellement. Le test autonome, les trois exports, les hashes
source/runtime et le cold start mod-local sont verts. Vincent déclare le candidat
prêt pour préparation du merge et remplace le classement initial : propriétaire
futur `plugin-misc.dll`, clé `misc.vendorStockRefresh`.

Le chantier Rift/Cow a été déclaré terminé par Vincent le 27 juillet 2026 et
retiré de la ROADMAP active. Sa mission reste conservée comme preuve technique.

## Prochain gate

Préparer le merge vers `plugin-misc.dll` dans un lot distinct explicitement
demandé, puis valider le binaire fusionné. Conserver comme gates de
non-régression le gamble, un vendeur de mode `0`, un layout réellement modifié,
les cinq actes, la manette, les clics rapides et le réseau hôte/joiner.

## Frontière Git

Le lot Vendor Stock Refresh comprend sa mission, son entrée ROADMAP, son
workstream, `VendorStockRefresh-src/`, `VendorStockRefresh.json` et la DLL
autonome. Les missions Rift/Cow, Transmogrify, Readable Items, Extended Item
Stats et les autres changements concurrents sont préservés.
