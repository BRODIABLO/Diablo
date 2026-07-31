# MassID — D2R 3.2

Dernière mise à jour : 31 juillet 2026

## Décision produit

Vincent demande l’implantation de **MassID** comme plugin autonome d’incubation,
destiné à être mergé plus tard dans `plugin-items.dll` sous la clé
`items.massIdentify`. L’incubation ne modifie, ne lie et ne redistribue aucune
DLL d’eezstreet.

Le geste retenu est `Shift + clic droit` sur un Tome of Identify. L’autorité
serveur identifie les objets non identifiés de l’inventaire principal, puis ceux
du Horadric Cube, dans cet ordre déterministe.

Le JSON autonome et le futur bloc du PluginPack partagent exactement le même
contrat :

```json
{
  "enabled": true,
  "freeIdentification": false
}
```

- `freeIdentification=true` identifie tous les objets admissibles même avec un
  tome vide et ne modifie jamais sa quantité;
- `freeIdentification=false` identifie au plus autant d’objets que le tome
  contient de charges et consomme exactement une charge par objet nouvellement
  identifié;
- un objet déjà identifié ne coûte aucune charge.

## Preuves natives gouvernées — build 92777

- Le workbench canonique, l’image d’analyse, l’index SQLite et le projet Ghidra
  sont vérifiés. La référence D2MOO demeure sémantique uniquement.
- `UI_InventorySlotWidget_HandleClick` à `0x2C7540` est le handler générique des
  clics d’inventaire. Il résout le propriétaire local, récupère l’objet cliqué
  par le slot virtuel `+0xC8` et sépare le clic gauche (`mouse state == 5`) de la
  branche clic droit.
- `CLIENT_SendTwentyOneByteCommandPacket` à `0xEC820` construit le paquet D2R
  3.2 de 21 octets. Le client natif Cain l’appelle avec l’opcode `0x34`.
- `D2GAME_PACKETCALLBACK_Rcv0x34_IdentifyItemsWithNpc` à `0x4AE280` est le
  callback serveur autoritaire correspondant; il exige exactement 21 octets.
- `D2GAME_ITEMS_Identify` à `0x46EA70` applique le drapeau identifié et les
  mises à jour natives nécessaires à un objet stocké.
- `SynchronizeItemAndBoundSkillQuantity` à `0x46F090` accepte
  `(game, player, book, delta)` et synchronise `STAT_QUANTITY` avec le skill lié.
- Le chemin MassID n’accroche ni `D2GAME_PACKETCALLBACK_EntityAction` possédé par
  Vendor Stock Refresh dans `plugin-items.dll`, ni `D2GAME_HandleUseItemPacket`
  possédé par Transmogrify, ni la queue générique utilisée par
  EquippedItemToCube.
- `plugin-items.dll` accroche aussi `UI_TOOLTIP_ResolveHoveredUnit` à
  `0x2A7810`. MassID ne revendique pas ce hook : il compose avec le propriétaire
  déjà chargé et signe la plage interne intacte et unique à `0x2A7820`. Ce
  témoin passe lorsque MassID charge avant ou après les cinq DLL du pack.

## Architecture d’incubation

1. Le hook client filtre seulement un clic droit avec Shift, curseur vide,
   propriétaire local et code compilé `ibk ` sur les pages inventory `0` ou
   Cube `3`.
2. Il consomme ce geste et envoie un paquet `0x34` de 21 octets portant deux
   marqueurs privés et le GUID du tome.
3. Le hook serveur reconnaît uniquement ces deux marqueurs. Tous les paquets
   Cain natifs sont délégués sans modification à l’original.
4. Le serveur revalide le GUID, le type item, le code `ibk `, l’inventaire
   parent et la page. Un paquet privé invalide est consommé sans mutation.
5. Deux passages inventaire puis Cube appellent le helper natif uniquement pour
   les objets non identifiés. Le budget est illimité en mode gratuit, sinon
   borné à la quantité serveur du tome.
6. La consommation non gratuite est appliquée une seule fois par le
   synchroniseur natif avec un delta égal au nombre d’objets réellement
   identifiés.

## Gates de validation

- [x] Destination future confirmée : `plugin-items.dll`,
  `items.massIdentify`.
- [x] Configuration confirmée : `enabled`, `freeIdentification`.
- [x] Handler du geste, protocole Cain 3.2, helper d’identification et ABI de
  quantité prouvés pour 92777.
- [x] Audit de coexistence : aucun hook partagé avec les cinq DLL du pack ou
  Transmogrify.
- [x] Release x64, CTest, manifeste v2, exports, auteur, description et JSON
  strict validés.
- [x] Cold start BKVince frais sans rejet, échec ni assertion.
- [ ] Gameplay : tome vide, partiel et suffisant; modes gratuit/non gratuit;
  ordre inventory/Cube; objets déjà identifiés; sauvegarde/relecture.
- [x] Compatibilité technique : portées globale et mod-locale, repli global,
  priorité mod-locale, doublon neutralisé et coexistence avec les cinq DLL
  eezstreet sans rejet ni échec.
- [ ] Compatibilité fonctionnelle : souris/manette inchangées, solo,
  hôte/joiner.
- [x] ZIP public strict limité à `MassID.dll` et `MassID.json`, entrées
  inspectées et hash calculé.
- [ ] Après validation intégrée future, supprimer seulement le binaire et le
  JSON autonomes; conserver les sources comme oracle jusqu’à équivalence.

## Validation obtenue le 31 juillet 2026

- Release x64 et CTest : `1/1` test vert.
- DLL finale : SHA-256
  `52E3A3BAA601E4BC7546CE57A271A22E1746992C14AF76E20946251871D44FBA`;
  les hashes build, dépôt, package et runtime sont identiques.
- Les trois exports D2RLoader, le manifeste v2, la version `0.1.0`, l’auteur
  `RuffnecKk` et la description orientée joueur sont présents.
- Le cold start mod-local installe les hooks `0x2C7540` et `0x4AE280`, résout
  le JSON BKVince et coexiste avec les cinq DLL du pack. Le cold start global
  installe les mêmes hooks après le pack et résout le repli `MassID.json`.
- Avec les DLL globale et mod-locale simultanées, une seule instance
  `mass-id` devient active et la configuration mod-locale demeure prioritaire.
- Une valeur `freeIdentification: "yes"` est refusée avant tout hook; le JSON
  valide a ensuite été restauré et un dernier cold start est vert.
- ZIP : `addons/MassID/MassID.zip`, deux entrées à la racine, SHA-256
  `4429F1FFAE84F7CC26C2BEB32DCB473C37E1B27BAA54D6A3E6EA33F4D8181F61`.
- L’automatisation a ouvert `QtyTester`, mais son inventaire visible ne
  contenait pas de Tome of Identify et l’outil ne peut pas maintenir Shift
  pendant un clic droit. Aucun résultat gameplay n’est donc inféré.

## Prochain gate

Observer en jeu la matrice tome vide/partiel/suffisant dans les deux modes,
l’ordre inventory/Cube, la sauvegarde/relecture, puis solo, hôte et joiner avant
de déclarer le comportement prêt à merger dans le prochain lot PluginPack.
