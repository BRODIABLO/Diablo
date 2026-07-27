# Mission courante

Dernière mise à jour : 27 juillet 2026

## Priorité active

[Vendor Stock Refresh — D2R 3.2](vendor-stock-refresh-3.2.md)

État : `VendorStockRefresh 0.1.0` est implanté en DLL autonome hybride. Le
bouton natif, les modes UI, le paquet vanilla `0x38`, les actions
normal/gamble, le callback serveur et le gate de session `PlayerData` sont
prouvés sur le build 92777. Le test autonome, les trois exports, les hashes
source/runtime et le cold start mod-local sont verts.

Le chantier Rift/Cow a été déclaré terminé par Vincent le 27 juillet 2026 et
retiré de la ROADMAP active. Sa mission reste conservée comme preuve technique.

## Prochain gate

Confirmer un rafraîchissement chez un vendeur normal avant et après un achat,
tout en préservant le bouton et le comportement vanilla de l’écran de gamble.
Étendre ensuite la validation à la manette et au réseau hôte/joiner.

## Frontière Git

Le lot Vendor Stock Refresh comprend sa mission, son entrée ROADMAP, son
workstream, `VendorStockRefresh-src/`, `VendorStockRefresh.json` et la DLL
autonome. Les missions Rift/Cow, Transmogrify, Readable Items, Extended Item
Stats et les autres changements concurrents sont préservés.
