# Equipped Item to Cube — D2R 3.2

## Statut et séquencement

- Statut : **planifié; aucune implantation commencée**.
- Cible éventuelle : `D2R.exe 3.2.92777` sous D2RLoader.
- Vincent confirme le 27 juillet 2026 que la catégorie future est `misc`, que
  la DLL propriétaire sera `plugin-misc.dll` et que la clé prévue sera
  `misc.equippedItemToCube`. Cette décision remplace le classement antérieur
  `items`, `plugin-items.dll` et `items.equippedItemToCube`.
- Vendor Stock Refresh demeure la priorité courante. Cette mission commencera
  après la confirmation de son bouton gamble original, d'un vendeur de mode
  `0` et d'un layout réellement modifié ou agrandi, sauf nouvelle décision
  explicite de Vincent sur le séquencement.

## Intention joueur

Rétablir `Ctrl + clic` sur un objet équipé afin de le déplacer directement dans
le Horadric Cube, comme l'action encore annoncée par le tooltip, sans transit
manuel par l'inventaire.

## Faits vérifiés

- Le tooltip affiche encore l'action et les chaînes
  `InventoryItemTooltipAppenderMove` et
  `InventoryItemTooltipAppenderMoveCube` subsistent dans le build actuel.
- Le workbench gouverné du build `92777` est vérifié : image canonique SHA-256
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`,
  image d'analyse SHA-256
  `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`,
  index SQLite et projet Ghidra persistants présents.
- `UNITS_GetInventoryGrid` à `0x34A410` identifie la page native `3` comme le
  Horadric Cube.
- `ITEMS_PlaceItemForPlayer` à `0x471500` place un objet selon la page portée
  par son état natif de 16 octets. Les primitives de capture, recherche de
  position, retrait et rafraîchissement d'inventaire sont déjà gouvernées par
  les preuves de Transmogrify.
- La référence PluginPack épinglée
  `eezstreet/D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`
  est propre et vérifiée. Son module futur désigné par Vincent est désormais
  `plugin-misc`.

## Hypothèses à tester

- Blizzard pourrait avoir neutralisé uniquement un branchement client entre la
  détection de `Ctrl` et le constructeur du mouvement déjà existant. Dans ce
  cas, un patch statique strict serait préférable à une DLL comportant une
  nouvelle transaction.
- Si le trajet équipement vers Cube n'est plus construit ou accepté, une DLL
  devra réutiliser le protocole et le handler serveur natifs après preuve de
  leurs champs, de leur ABI et de leur rollback.
- Le paquet de 21 octets observé est un candidat de mouvement d'objet, mais son
  opcode, ses champs, son dispatcher et son autorité ne sont pas encore
  suffisamment prouvés pour une implantation.

## Inconnues et risques

- Point exact où divergent Shift équipement vers inventaire, Ctrl inventaire
  vers Cube et Ctrl équipement vers Cube.
- Règles serveur de retrait d'équipement, recalcul des statistiques, weapon
  swap, exigences de présence du Cube et choix de la première cellule libre.
- Comportement atomique quand le Cube est absent ou plein, et rollback si le
  retrait réussit mais que le placement échoue.
- Compatibilité souris, manette, solo, hôte/joiner et client sans plugin.
- Collision éventuelle avec un hook ou dispatcher déjà possédé par
  `plugin-misc`, une DLL RuffnecKk ou le PluginPack épinglé.

## Architecture gouvernée

- Conserver l'hôte comme autorité : le client exprime une intention de
  déplacement; le serveur revalide propriété, emplacement équipé, Cube,
  destination et transaction complète.
- Préférer un patch statique avec signature stricte si la seule régression est
  un gate client neutralisé.
- Si une DLL est nécessaire, incuber `EquippedItemToCube.dll`, autonome,
  hybride globale/mod-locale, attribuée exactement à `RuffnecKk`, avec la
  description courte `Moves equipped items directly to the Horadric Cube.`
- Utiliser uniquement un JSON anglais si une configuration est réellement
  nécessaire, recherché d'abord dans le mod actif puis dans le dossier global.
  Ne créer aucun TOML.
- Le merge futur rejoindra `plugin-misc.dll` et l'unique `D2RPlugins.json` sous
  `misc.equippedItemToCube`. L'autonome ne sera supprimé qu'après validation du
  binaire fusionné.

## Gates observables

1. **Séquencement — ouvert** : fermer le jalon court Vendor Stock Refresh ou
   obtenir une décision explicite de Vincent qui remplace cet ordre.
2. **Traçage client — ouvert** : comparer Shift équipement vers inventaire,
   Ctrl inventaire vers Cube et Ctrl équipement vers Cube, puis identifier le
   premier branchement divergent.
3. **Transaction serveur — ouvert** : prouver opcode, taille, champs, handler,
   ABI, validations, retrait d'équipement, placement Cube, synchronisation et
   rollback.
4. **Audit PluginPack — ouvert** : inventorier dans `plugin-misc` les fichiers,
   callbacks, structures, configurations, RVA et plages de hooks; désigner un
   propriétaire unique pour chaque site partagé.
5. **Choix d'implantation — ouvert** : retenir le patch statique minimal si la
   preuve le permet; sinon construire l'autonome hybride avec gardes de build,
   signatures et ABI strictes.
6. **Validation runtime — ouverte** : tous les slots, deux weapon sets, Cube
   absent/plein, inventaire plein, recalcul des statistiques,
   sauvegarde/rechargement, souris/manette, solo/hôte/joiner, client sans plugin
   et portées globale/mod-locale, avec zéro perte, duplication, objet fantôme,
   crash ou désynchronisation.
7. **Promotion `plugin-misc` — ouverte** : porter la fonctionnalité sous
   `misc.equippedItemToCube`, compiler le pack complet et répéter les gates de
   non-régression avant de retirer l'autonome.

## Prochain gate

Après le jalon court Vendor Stock Refresh, tracer les trois chemins d'entrée
comparatifs et borner le premier branchement divergent avant toute implantation.

## Frontière Git

Cette décision autorise la mission documentaire et le classement `misc`; elle
ne constitue ni une demande de commit/push, ni une autorisation de modifier une
DLL d'eezstreet. Toute future incubation restera autonome jusqu'au merge
explicitement demandé et validé.
