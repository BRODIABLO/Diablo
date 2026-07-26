# Mission courante

Dernière mise à jour : 26 juillet 2026

## Priorité active

[Vendor Stock Refresh — D2R 3.2](vendor-stock-refresh-3.2.md)

État : le prototype autonome `VendorStockRefresh 0.1.0` est implanté, compile en
Release x64 et passe son cold start mod-local sur 92777 : hashes DLL/JSON
identiques, configuration mod-locale sélectionnée, cinq sites acceptés et zéro
erreur fraîche. Il réutilise le bouton natif `button_refresh`, le paquet vanilla
`0x38` de neuf octets et le cycle serveur `VendorChainEntry+0x35` sans nouvel
opcode. La catégorie `items`, la destination future `plugin-items.dll` et la clé
`items.vendorStockRefresh` restent confirmées. La DLL est attribuée exactement à
`RuffnecKk`, hybride globale/mod-locale, configurée uniquement par
`VendorStockRefresh.json` et ne modifie, lie ni redistribue une DLL d’eezstreet.

## Prochain gate

Observer en jeu un rafraîchissement normal avant/après achat et la non-régression
du gamble. Capturer logs, compteurs et stock avant/après; le focus manette, le
spam de clics, tous les actes, le repli global et l’hôte/joiner suivent après ce
témoin.

## Frontière Git

Le lot Vendor Stock Refresh comprend sa mission, son entrée ROADMAP, son
workstream, les preuves `known-rvas.json`/`findings.md` et ses sources/DLL/JSON
autonomes. Les changements concurrents de Transmogrify,
Readable Items, Repair Costs Cap, ExtendedItemStats et des autres plugins sont
préservés. Le registre assigne ce périmètre au workstream
`vendor-stock-refresh`.
