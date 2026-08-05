# Four-Character Item Codes — D2R 3.2

Dernière mise à jour : 5 août 2026

## Décision finale

- Le correctif est un memory patch D2RLoader généraliste, pas une DLL.
- Il n’est pas fusionné dans `plugin-items.dll` et ne possède aucune clé de
  configuration.
- Il s’installe indifféremment dans le dossier `patches` global ou mod-local.
- BKVince a servi uniquement de banc runtime réversible pour le témoin `rk4x`.

## Effet joueur

Afficher correctement les graphismes HD des items dont le code de base comporte
quatre caractères significatifs. Les codes plus courts conservent leur padding
vanilla par espaces.

## Preuves natives

- Le compilateur TXT, le lookup ItemsTxt, la sauvegarde et les chemins réseau
  conservent déjà les 32 bits du code.
- `HDItems_ResolveBaseItemAsset` à `0x662FB0` compare le DWORD complet.
- Le seul défaut est le padding du chargeur `items.json` dans
  `HDItems_LoadBaseItemAssetsJson`.
- La signature originale de 26 octets à `0x6639FF` est unique dans `.text` :
  `0F B6 46 02 84 C0 75 09 66 C7 44 24 26 20 20 EB 09 88 44 24 26 C6 44 24 27 20`.
- Le remplacement de même longueur lit successivement les octets 2 et 3,
  transforme chaque NUL en espace, puis rejoint naturellement `0x663A19` :
  `8A 46 02 84 C0 75 02 B0 20 88 44 24 26 8A 46 03 84 C0 75 02 B0 20 88 44 24 27`.
- Le désassemblage du remplacement couvre exactement 26 octets; aucun relais,
  mutex, hook, allocation exécutable ou DLL n’est nécessaire.

## Validation runtime

- Portée mod-locale : `four-character-item-codes.json [mod]`, patches
  `scanned=18 applied=18 disabled=0 failed=0`, plugins
  `scanned=12 active=12 disabled=0 rejected=0 failed=0`.
- Portée globale : `four-character-item-codes.json [global]`, mêmes totaux et
  zéro échec.
- Le témoin sauvegardé `rk4x` entre en jeu, affiche le modèle HD de Hand Axe et
  l’infobulle `Four-Character Code Witness`; le contrôle `hax` reste visible.
- La sauvegarde et sa relecture à froid ont conservé le témoin.
- Les quatre assertions du profil BKVince sont identiques avec le correctif
  actif, désactivé et absent; elles ne sont pas causées par ce chantier.
- Après test, les trois fichiers data, `plugin-items.dll` et `D2RPlugins.json`
  ont été restaurés à leurs cinq hashes initiaux, les fichiers `DummyTester*`
  ont été retirés du profil, et aucun processus Diablo ne reste actif.
- Un test hôte/joiner physique n’a pas été exécuté; le chemin réseau quatre
  octets est toutefois prouvé statiquement et le patch ne touche que le registre
  local des assets HD.

## Artefacts

| Artefact | SHA-256 |
|---|---|
| `four-character-item-codes.json` | `2F4F986FD95E5B4F2DA90EFCCE77E5D425FD45E48AB06FC39865BB997B75C79E` |
| `FourCharacterItemCodes.zip` | `A109719C3B116C7D42D5A61D6C949A388C3064FE76E096AE74DC693CF16F186C` |

Le ZIP public contient strictement le JSON de memory patch. Auteur :
`RuffnecKk`.

## Assainissement PluginPack

L’ancienne tentative de DLL et de merge a été retirée. Le fork PluginPack est
revenu en version `2.1.0`, valide `139/139` écritures gouvernées et passe
`26/26` CTest Release. Aucune DLL d’eezstreet n’est modifiée ou redistribuée par
cet add-on.

## État

Le correctif généraliste est prêt. Le seul suivi facultatif restant est une
confirmation hôte/joiner avec deux clients possédant les mêmes données de mod.
