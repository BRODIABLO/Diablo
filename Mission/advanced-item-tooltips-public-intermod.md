# Advanced Item Tooltips — Public Intermod Release

Dernière mise à jour : 5 août 2026 — mission clôturée

## Décision produit

Vincent choisit une version publique **autonome permanente**, distincte de la
version personnelle BKVince 2.2.0. Elle demeure attribuée à `RuffnecKk`, peut
être installée globalement ou dans le dossier d'un mod et ne sera ni mergée au
PluginPack ni dépendante de Transmogrify, d'une DLL eezstreet ou de BKVince.

La séquence retenue est **compatibilité d'abord** : définir et tester le contrat
des tables du mod actif avant de créer le package public. Le code et la DLL
BKVince restent figés pendant ce travail; la déclinaison publique vit sous
`addons/AdvancedItemTooltips/` afin que les deux produits puissent évoluer
séparément.

## Configuration publique retenue

Le plugin utilisera son propre `AdvancedItemTooltips.json`, recherché d'abord
dans le mod actif puis dans le dossier global. L'absence du fichier applique
les valeurs par défaut; un fichier présent mais invalide doit être refusé avant
l'installation des hooks.

```json
{
  "enabled": true,
  "showMaxSockets": true,
  "showMaxSocketsOnSocketedItems": false,
  "showBaseDefenseRange": true,
  "showPropertyRanges": true,
  "includeSocketedContributionsInRanges": false
}
```

`showMaxSocketsOnSocketedItems=false` retire seulement la ligne `Max Sockets`
dès que l'objet possède déjà des sockets, qu'il s'agisse d'une base vide, d'un
objet gemmé ou d'un runeword. Cette décision d'affichage doit rester isolée du
pipeline des plages : elle ne peut supprimer, déplacer ou recolorer une autre
ligne du tooltip.

`includeSocketedContributionsInRanges=false` est le défaut public choisi par
Vincent. Les plages représentent alors uniquement les sources intrinsèques de
l'objet : affixes, propriétés automagic ou superior, propriétés fixes de craft,
uniques, sets et propriétés de runeword. Les contributions directes des gems,
runes et jewels insérés sont exclues. Pour un runeword, les propriétés définies
par `runes.txt` demeurent intrinsèques, tandis que les bonus individuels des
runes provenant de `gems.txt` sont exclus. La valeur principale affichée par
Diablo reste toujours la valeur finale; elle peut donc dépasser une plage
intrinsèque lorsque des sockets contribuent à cette statistique.

Lorsque l'option vaut `true`, le moteur additionne les plages de toutes les
sources portées par l'objet, comme dans la version personnelle BKVince : affixes
et propriétés natives plus gems, runes et jewels insérés. Toute attribution
ambiguë demeure fail-closed.

## Faits acquis réutilisables

- BKVince 2.2.0 transforme le tooltip final sans importer Transmogrify,
  Extended Item Stats ni une DLL du PluginPack.
- Les sept call-sites directs de `ITEMS_BuildItemTooltip` sont validés pour
  D2R 3.2.92777 et le prologue natif reste disponible pour un autre propriétaire.
- Les cumuls affixe, craft, automagic, superior, unique, set, runeword et contenu
  de sockets ont été stress-testés sous BKVince.
- Les staffmods hardcodés et les calculs poison dépendants de la durée restent
  volontairement sans annotation lorsqu'aucune plage fiable n'est démontrée.

Ces faits prouvent la base technique, mais ne prouvent pas encore la
compatibilité publique avec les tables d'un autre mod.

## Phase 1 — contrat intermod avant code public

1. Inventorier chaque table, header, clé et invariant réellement consommé par
   le moteur de plages.
2. Éliminer toute hypothèse implicite BKVince concernant les chemins, lignes,
   IDs, nombre de records ou présence d'une table.
3. Construire des fixtures déterministes couvrant au minimum les tables vanilla
   3.2, les tables BKVince et des variantes synthétiques : table absente, ordre
   différent, record désactivé, valeur fixe, double source et provenance
   ambiguë. Il ne faut coder aucune adaptation spéciale pour BK, BT ou un autre
   mod nommé.
4. Prouver que chaque donnée inconnue ou incohérente entraîne l'omission de la
   seule annotation concernée, sans crash ni plage inventée.
5. Tester séparément les modes intrinsèque et combiné, notamment Enhanced
   Damage, dégâts minimum/maximum, résistances, stats individuelles plus
   `All Attributes`/`All Resistances`, crafts et runewords.

## Premier audit reproductible du 31 juillet 2026

Le gate natif est vert sans nouvelle recherche d'adresse : l'image canonique
92777 (`CC59119D…14715`), l'image d'analyse (`673E8C0B…E63AB`) et l'index de
105 850 fonctions sont vérifiés. `ITEMS_BuildItemTooltip` demeure identifié à
haute confiance à `0x2BD480`, avec les sept call-sites déjà gouvernés. La
référence PluginPack est propre et épinglée à
`eezstreet/D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`;
ses sources ne contiennent ni ce RVA ni les sept call-sites. Cela ferme le gate
statique de collision connu, mais pas encore la matrice runtime avec les cinq
DLL.

L'auditeur
`scripts/reverse-engineering/advanced-item-tooltips-public-compatibility.mjs`
formalise les headers réellement consommés et applique une politique par
fonctionnalité : ordre des headers et colonnes supplémentaires acceptés;
header requis absent, header normalisé dupliqué ou clé stable dupliquée = seule
la table concernée est rejetée; le nombre de lignes reste informatif. Ses 12
tests synthétiques prouvent notamment qu'une absence de `gems.txt` désactive
les contributions de sockets et les plages combinées de runewords sans toucher
à `Max Sockets`, Base Defense, aux affixes ni aux propriétés intrinsèques de
runeword.

Les quinze tables de BKVince et de vanilla 3.2 satisfont actuellement tous ces
contrats, avec round-trip byte-exact et CRLF. Leurs quantités diffèrent pourtant
fortement — par exemple 502 contre 433 uniques explicites, 113 contre 99
runewords actifs et 2321 contre 227 recettes Cube — ce qui confirme qu'aucun
nombre de records BKVince ne peut devenir un invariant public. Les quatre
headers BKVince supplémentaires de `misc.txt` ne sont pas consommés.

L'audit du source personnel 2.2.0 révèle trois adaptations obligatoires avant
de créer le source public :

1. `RangeCatalog::Load` charge aujourd'hui les quinze tables en transaction
   unique; une seule table absente retire toutes les plages. Le loader public
   devra produire des catalogues/capacités indépendants et un diagnostic par
   source.
2. `Number` confond actuellement cellule absente, texte invalide et zéro valide.
   Le loader public devra conserver ces trois états et omettre seulement le
   record ou la propriété non démontrable.
3. Les tests d'intégration personnels imposent volontairement les comptes
   BKVince et des IDs/codes BKVince. Ils restent des preuves de non-régression
   du produit personnel, mais ne seront ni copiés ni transformés en hypothèses
   du produit public.

Le mode public par défaut
`includeSocketedContributionsInRanges=false` possède un gate supplémentaire :
ignorer simplement les fillers ferait échouer la validation d'une ligne finale
qui contient déjà leur valeur. Les bonus fixes de rune/gem peuvent être
soustraits exactement à partir de `gems.txt`; une jewel à roll variable exige
en revanche la valeur réellement roulée de l'unité enfant, pas seulement sa
plage d'affixe. Il faut donc prouver un accès natif exact à cette contribution
ou omettre uniquement la ligne chevauchée. Aucune plage intrinsèque ne sera
inventée à partir d'une décomposition ambiguë.

La meilleure voie statique actuelle est `STATLIST_GetUnitStat` à `0x2F5020`,
déjà gouverné à haute confiance avec l'ABI
`(unit, statId, layer) -> int32`. Le plugin sait aussi énumérer les unités
fillers du parent. Il reste à démontrer en runtime que l'appel sur une jewel
socketée retourne bien son roll propre pour les statistiques scalaires visées;
la preuve voisine sur une autre unité ou une autre stat ne suffit pas.

Enfin, la 2.2.0 valide déjà les signatures du résolveur de runeword, des deux
résolveurs de langue, des trois routines d'inventaire utilisées et de
l'agrandissement de chaîne, puis les cinq octets de chacun des sept call-sites.
Elle ne vérifie pas encore à l'installation les entrées de tous les autres
helpers directement appelés (`ITEMS_GetMaxSockets`, `STATLIST_GetUnitStat`,
`GetItemDataContext`, `UNITS_GetItemData` et `GetItemsTxtRecord`). Le public
devra refuser le chargement si une de ces signatures indispensables ne
correspond pas, même lorsqu'elle possède déjà une identification gouvernée.

Commande de reprise :

```powershell
node scripts/reverse-engineering/advanced-item-tooltips-public-compatibility.mjs
```

## Prototype public 3.0.0-rc.1 — 2 août 2026

Le premier prototype public autonome est créé sous
`addons/AdvancedItemTooltips/`, sans modifier le produit personnel BKVince
2.2.0. Il possède son propre source, son propre JSON et son propre artefact de
package. Il n'importe et ne lie ni BKVince, ni Transmogrify, ni une DLL
d'eezstreet; les cinq DLL du PluginPack restent seulement des partenaires de
coexistence runtime.

Le parseur JSON est strict : la racine doit être un objet, les six clés connues
doivent être booléennes et toute clé inconnue provoque un refus avant hooks. La
priorité mod-locale puis globale est implantée, et l'absence du fichier conserve
les défauts intégrés. Les deux modes demandés sont actifs :

- mode intrinsèque par défaut : les affixes, crafts, automagic, superior,
  uniques, sets et propriétés de runeword restent pris en compte, tandis que les
  contributions directes des fillers socketés sont exclues;
- mode combiné : gems, runes et jewels sont réintégrés aux plages comme dans le
  produit personnel stress-testé.

Les entrées natives directement appelées sont maintenant contrôlées avant
activation pour `ITEMS_GetMaxSockets`, `STATLIST_GetUnitStat`,
`GetItemDataContext`, `UNITS_GetItemData` et `GetItemsTxtRecord`, en plus des
sept call-sites du constructeur de tooltip. Le build Release x64 et les trois
suites CTest — placement, plages et configuration — passent `3/3`. L'auditeur
intermod synthétique passe également. Le témoin Call to Arms vérifie notamment
la distinction attendue : Enhanced Damage `[200 - 240]` en mode intrinsèque et
`[250 - 290]` en mode combiné.

Artefacts de test :

- DLL package et runtime :
  `CC22A2F7FDFBCA0F84CBA5E537362E21D96B1A32C0DAD98CCAA9E94F36D47F8A`;
- JSON package et runtime :
  `B2C3E3B268FBDE4F5538DA053B3833ECA9D257E9FCEE2D4E829F667F2D1F3DB7`;
- cold start mod-local BKVince : `scanned=10 active=10 disabled=0 rejected=0
  failed=0`, avec journal explicite du chemin JSON et du mode intrinsèque.

Cette RC reste volontairement fail-closed. Le loader de catalogues n'est pas
encore séparé en capacités complètement indépendantes et les états cellule
absente, valeur invalide et zéro valide doivent encore être distingués. En mode
intrinsèque, si une contribution variable d'une jewel empêche de valider
exactement la valeur finale contre la plage intrinsèque, seule l'annotation
chevauchée est omise; le plugin ne fabrique pas une décomposition.

## Prototype public 3.0.0-rc.3 — 3 août 2026

Le test vanilla a révélé une limite de déploiement distincte du contrat des
tables : le SDK public D2RLoader épinglé à
`efcfaaa52eeec9e379b3fc2aad1013bb3dddc970` expose le nom et les chemins du mod
actif, mais aucune API de lecture CASC/MPQ. La création d'un faux mod contenant
une copie des quinze tables vanilla aurait seulement validé une fixture; elle
ne constitue pas une installation publique autonome et est rejetée comme
solution produit.

La RC3 adopte donc un chargement hybride automatique : les tables physiques du
mod actif restent prioritaires; en vanilla pur, et uniquement lorsqu'aucun mod
n'est actif, la DLL charge un catalogue vanilla 3.2.92777 embarqué dans ses
ressources. Aucun fichier TXT, faux mod ou argument `-txt` n'est requis chez
l'utilisateur. Un mod actif dont les tables ne sont pas accessibles ne reçoit
jamais silencieusement les plages vanilla : les annotations de plages restent
fail-closed et `Max Sockets`, calculé par la fonction native, demeure actif.

Le build Release passe maintenant `4/4` suites CTest, dont un smoke test du
provider embarqué. Les quinze ressources RCDATA sont présentes et leurs tailles
correspondent exactement aux quinze sources vanilla. L'auditeur de contrat
confirme `15/15` tables et `12/12` capacités pour BKVince comme pour vanilla
3.2. La DLL candidate et sa copie de package sont byte-identiques, SHA-256
`86139FFFBDF5A47C09CC2085AEC5AC0D8689DE63507CA078913625920B34335F`.

Le prochain gate runtime doit lancer vanilla sans profil de fixture et confirmer
dans le journal `catalog=embedded vanilla 3.2.92777`. Conformément à la demande
de Vincent, aucun arrêt ni redémarrage du runtime en cours ne sera fait sans son
accord explicite.

## Gates d'implantation et de livraison

- Valider en jeu les options `showMaxSocketsOnSocketedItems` et
  `includeSocketedContributionsInRanges` dans leurs deux états.
- Valider configuration absente, valide et invalide, avec priorité mod-local
  puis repli global.
- Transformer le chargement monolithique des tables en capacités indépendantes
  et conserver un diagnostic par source avant de déclarer l'intermod public
  complet.
- Prouver ou omettre proprement les contributions variables de jewels en mode
  intrinsèque, sans contaminer les autres plages.
- Valider les deux portées d'installation et la coexistence avec les cinq DLL
  eezstreet, Transmogrify et les plugins autonomes disponibles, sans rejet ni
  échec.
- Vérifier les contextes inventory, stash, Cube, marchand, sol et comparaison,
  ainsi que souris/manette et solo/hôte/joiner sans changement de sauvegarde ni
  de gameplay.
- Produire après ces gates un ZIP public minimal contenant uniquement
  `AdvancedItemTooltips.dll`, `AdvancedItemTooltips.json` et un README court,
  puis inspecter ses entrées et consigner les SHA-256 du ZIP, de la DLL testée
  et du runtime.

## Release publique 3.0.0 — 3 août 2026

Vincent ferme le gate de publication après le stress test BKVince et la matrice
vanilla. La reconstruction de `Base Defense` couvre désormais les armures
plain, superior, ethereal, rares, crafted, uniques, sets et runewords avec
Enhanced Defense et défense plate. Les témoins finaux confirment notamment :

- Spirit Monarch : `Base Defense: 148 [133 - 148]`, sans confondre
  `+250 Defense vs. Missile` avec une défense plate;
- Nightwing's Veil : `Base Defense: 159 [114 - 159]` pour `352 Defense` et
  `+120% Enhanced Defense`, avec remappage sûr de la règle native `maxac + 1`;
- Mechanic's Full Plate Mail socketée par affixe :
  `Base Defense: 156 [150 - 161]`, sans régression liée à `Socketed (1)`.

Le build final Release x64 est estampillé `3.0.0`; les quatre suites CTest
passent. Le cold start final en vanilla pur journalise `Active mod: <none>`,
`catalog=embedded vanilla 3.2.92777`, `scanned=1 active=1 disabled=0 rejected=0
failed=0`. Le README source documente séparément les emplacements de la DLL et
du JSON sous le dossier `d2rloader/config` global ou mod-local, ainsi que la
priorité du fichier mod-local.

L'archive publique minimale
`addons/AdvancedItemTooltips/AdvancedItemTooltips-3.0.0.zip` contient uniquement
`AdvancedItemTooltips.dll`, `AdvancedItemTooltips.json` et un README court;
aucun source, PDB, log ni binaire tiers n'est redistribué. Les défauts publics du JSON masquent
`Max Sockets` après socketing et excluent des plages les contributions directes
des gems, runes et jewels; les deux comportements peuvent être activés par le
modder sans changer de DLL.

SHA-256 finaux :

- DLL package et runtime vanilla :
  `23B2215E6F781FAC4AA266AF952BE0C856E20407E13C8CC7D647D9040859C8C4`;
- JSON public :
  `B2C3E3B268FBDE4F5538DA053B3833ECA9D257E9FCEE2D4E829F667F2D1F3DB7`;
- README public :
  `22E042990D19B4D943B58342B394F83441D53473B169BB170C1233EC8BBF275B`;
- ZIP public :
  `E5AB9862E953ED4DD4C2FA7D9BAC5FA6D28397AE39216CAB9305AF84E8844ED8`.

Les validations manette et hôte/joiner restent des observations de
compatibilité post-release; elles ne changent ni le gameplay, ni les
sauvegardes, ni le format des objets.

## Extension confirmée — plages des propriétés ajoutées par le Cube

Vincent confirme le 3 août 2026 l'ajout de cette extension à la roadmap et
retient le séquencement **A — compatibilité d'abord**. Le plugin doit attribuer
les propriétés ajoutées par des recettes Cube définies dans `cubemain.txt`,
autant pour BKVince que pour un mod public arbitraire, sans dépendre d'un hook
de transmutation ni d'un historique conservé seulement pendant la session.

L'audit byte-exact CRLF de BKVince dénombre 2 321 recettes, dont 1 676 portent
au moins une propriété de sortie. Parmi elles, 1 272 produisent `useitem`, 19
sont des recettes d'augment et 524 utilisent les propriétés de corruption.
BKVince fournit trois marqueurs gouvernés par ses propres tables :
`augmented1`, `corruption1` et `corruption2`. Leurs stats correspondantes
`augmented`, `corrupted` et `corruptordesc` possèdent toutes `Save Bits=10` et
`Send Bits=10`; elles constituent donc une provenance persistante exploitable
après sauvegarde/relecture et chez l'hôte ou le joiner. Les outcomes de
corruption utilisent en plus `op=16` sur la stat `corruptordesc` ID 369 pour
sélectionner le résultat roulé avant de le remplacer par l'état final.

Le moteur public ne présumera toutefois ni ces noms, ni ces IDs. Il chargera
les recettes activées et leurs propriétés depuis les tables du mod actif,
reconnaîtra les contraintes et marqueurs persistants lorsqu'ils existent, puis
énumérera les histoires compatibles avec l'objet fini. Une plage ne sera
ajoutée que si toutes les histoires valides donnent la même attribution pour la
ligne concernée. Une recette indiscernable, répétable sans borne ou privée de
provenance suffisante fera omettre uniquement la plage affectée; aucune plage
ne sera devinée.

Ordre d'implantation retenu :

1. généraliser le catalogue `cubemain.txt` au-delà des seuls outputs crafted;
2. modéliser les prédicats d'entrée, outputs `useitem`/`usetype`, propriétés
   fixes ou variables et marqueurs persistants;
3. résoudre les contributions par consensus borné à partir de l'objet fini;
4. couvrir BKVince avec ses augments/corruptions et des fixtures publiques sans
   marqueur, incluant chevauchement avec affixes, automagic, uniques, sets,
   runewords et fillers socketés;
5. n'étudier un hook de transmutation qu'en amélioration facultative si les
   preuves natives démontrent un gain mesurable sans compromettre les objets
   anciens, sauvegardés ou échangés.

### Implantation statique 3.1.0-rc.1

Le moteur public généralise maintenant les outputs `useitem` et `usetype` sans
nom ni ID BKVince codé en dur. Il découvre une famille de provenance seulement
si sa propriété non affichée revient dans plusieurs outcomes et si la stat
sous-jacente possède à la fois `Save Bits > 0` et `Send Bits > 0`. Les familles
actives se composent en produit cartésien borné, ce qui permet à un objet de
porter plusieurs transformations persistantes indépendantes. Le plafond de
2 048 candidats refuse toutes les annotations de l'objet au lieu de risquer une
explosion ou une attribution inventée.

Pour une recette publique sans marqueur, le moteur conserve comme alternatives
l'objet intact et chacune des histoires compatibles à une étape. Le tooltip
complet tranche lorsqu'une valeur exclut les autres histoires; sinon la ligne
reste sans suffixe. Les répétitions sans compteur persistant ne sont pas
extrapolées. Ce comportement est prouvé par les témoins suivants :

- IAS natif `[3 - 5]` plus recette `[2 - 6]` : un roll final de 7 affiche
  `[5 - 11]`, tandis qu'un roll final de 5 reste volontairement sans plage;
- recette synthétique publique Mana `10-20` sur Great Wyrm's `61-90` : 103
  sélectionne `[71 - 110]`, tandis que 83 demeure ambigu;
- corruption BKVince : la stat persistante 369 à 1001 active les outcomes
  Annihilus et `+7% Increased Attack Speed` reçoit `[5 - 10]`;
- augment BKVince : la stat persistante 370 à 1 sélectionne l'augment MF/GF et
  combine un suffixe MF `20-35` avec le bonus fixe 50 en `[70 - 85]`.

Le workbench 92777 est vérifié et la référence eezstreet PluginPack reste
propre au commit épinglé `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`.
Aucun symbole ou propriétaire Cube/Transmute utile n'est gouverné dans ces
sources; conformément à la séquence A, aucun nouveau hook n'est requis par la
RC. Le build Release x64 réussit sans avertissement et les quatre suites CTest
passent. Le prochain gate est un déploiement BKVince contrôlé et un cold start,
uniquement après l'accord explicite de Vincent pour redémarrer le runtime.

## Extension multilingue confirmée — séquence 1 native d'abord

Vincent confirme le 3 août 2026 la prise en charge des langues livrées avec
D2R et retient la séquence **1 — résolution native avant les libellés créés par
le plugin**. Les propriétés ne sont plus reconnues à partir de leurs phrases
anglaises : le catalogue charge désormais `DescStrPos`, `DescStrNeg`,
`DescStr2` et les clés de groupe de l'`itemstatcost.txt` du mod actif, puis
demande à D2R leur texte dans la langue réellement sélectionnée. Les formats
natifs de dégâts composés, skill tabs, métadonnées, dommages et défense sont
résolus par le même chemin `LANG_GetStringByKey` déjà gouverné pour le build
92777. Sur les 151 stats visibles modélisées, 149 exposent directement une clé
localisable; les deux exceptions structurelles ne doivent pas recevoir de
plage.

Les deux seuls libellés inventés par Advanced Item Tooltips, `Max Sockets` et
`Base Defense`, possèdent une traduction intégrée pour les treize locales D2R :
enUS, zhTW, deDE, esES, frFR, itIT, koKR, plPL, esMX, jaJP, ptBR, ruRU et zhCN.
La locale est reconnue à partir du format natif `ItemStats1h`; les empreintes
ont été comparées à l'extraction vanilla 3.2 gouvernée
`local/lng/strings/item-modifiers.json`. Le séparateur des doubles plages est
lui aussi résolu nativement avec `ItemStast1k`, sans `to` anglais imposé.

La RC publique `3.1.0-rc.2` compile en Release x64 avec `/utf-8`. Les quatre suites CTest passent,
incluant les treize labels, les placeholders asiatiques positionnels `%0/%1`,
un tooltip coréen avec IAS, métadonnée Strength, défense plate, Enhanced
Defense et dégâts de feu min/max séparés. L'audit intermod reste vert sur les
quinze tables et toutes les fonctionnalités BKVince/vanilla. Le gate restant
est une matrice visuelle minimale en anglais, coréen et chinois, après accord
explicite de Vincent pour déployer et redémarrer le runtime.

## Clôture produit — 5 août 2026

Vincent déclare Advanced Item Tooltips terminé et ferme la mission sur la RC
publique `3.1.0-rc.2`. Les staffmods hardcodés demeurent explicitement hors
périmètre et aucune plage n'est inventée lorsque la provenance n'est pas
démontrable. Les observations manette, hôte/joiner et la matrice visuelle
anglais/coréen/chinois n'ont pas été exécutées exhaustivement; elles sont
retirées des gates de livraison par décision produit et ne deviennent pas des
succès implicites.

Le setup personnel final utilise une installation globale unique :

- DLL : `<D2R>/d2rloader/plugins/AdvancedItemTooltips.dll`;
- JSON : `<D2R>/d2rloader/config/AdvancedItemTooltips.json`;
- `includeSocketedContributionsInRanges=true` pour combiner les contributions
  des runes, gems et jewels;
- `showMaxSocketsOnSocketedItems=false` pour masquer `Max Sockets` dès que
  l'objet possède des sockets.

La copie mod-locale BKVince est neutralisée. La DLL globale porte le SHA-256
`282652C6C637927A4A4D6265B3C2E88D50908A4342B0013857F6643EEDD3F764` et le
JSON global le SHA-256
`F425617759A69C6EF09BB630C9971469D53CEE7773B5D3C1F5686CB859509E5A`.
Le déploiement statique et l'unicité de portée sont vérifiés; aucun cold start
n'a été exécuté pendant cette bascule finale.

## Maintenance 3.2.2 — coexistence avec PluginPack

Le cold start BKVince du 5 août 2026 a reproduit un refus propre de la 3.2.1 :
`plugin-skills.dll` installe d'abord son hook inline gouverné sur
`LANG_GetStringByKey 0x5F4B90`, puis Advanced Item Tooltips exigeait encore le
prologue vanilla de cette même entrée qu'il appelle sans la posséder. La DLL
refusait donc son chargement avec `runeword ABI signature mismatch`, tandis que
les versions antérieures demeuraient compatibles avec la pile.

La 3.2.2 conserve les signatures strictes sur les fonctions et les sept
call-sites qu'elle possède. Pour l'entrée partagée `0x5F4B90`, elle exige
désormais seulement une adresse mémoire exécutable, puis appelle normalement
la chaîne installée par `plugin-skills`. Les quatre suites CTest passent,
incluant les régressions de lignes `setitems.txt` à `*ID` dupliqué et de
plusieurs variantes actives partageant une clé de runeword.

La DLL Release, la copie de package et le runtime global sont byte-identiques
au SHA-256
`BFBFAEC8A64D7FAF7151C0D149A4E54DA63329D779D526DAC615C742A81EFCCA`.
Le cold start complet suivant charge `plugin-skills` et son hook `0x5F4B90`,
puis active `AdvancedItemTooltips 3.2.2` avec les sept call-sites, les tables
BKVince, la configuration globale, les contributions socketed incluses et
`Max Sockets` masqué sur les objets socketed. Le résumé termine à
`scanned=13 active=12 disabled=0 rejected=0 failed=1`; l'unique échec restant
est le problème préexistant et distinct de `RepeatableServices.dll`.

## Maintenance 3.2.3 — plages ED, groupes de statistiques et recettes craft

Les captures runtime BKVince ont exposé une régression générale : la fonction
`ac%` était réduite à sa borne maximale pendant le décodage de `properties.txt`.
Cela supprimait les plages Enhanced Defense des objets Superior, des affixes
magiques (Saintly) et des runewords Dream/Exile. La 3.2.3 conserve maintenant
l'enveloppe min/max définie par la table propriétaire.

Les formats localisés groupés (`All Resistances`, `All Attributes`) sont aussi
séparés de leurs composantes individuelles afin d'éviter qu'une même ligne ne
corresponde simultanément à plusieurs statistiques. Enfin, les sorties
`usetype,crf` sont indexées comme recettes craft intrinsèques et les marqueurs
persistants d'augment/corruption sont sélectionnés sans confondre les bonus de
gameplay visibles avec la provenance de la recette.

Les régressions automatisées couvrent le Superior Leather Armor, les gants
Saintly of Chance, Dream, Exile, le Caster Amulet, l'augment et la corruption
sur les tables réellement chargées par le runtime BKVince.

La DLL Release, la copie de package et le runtime global 3.2.3 sont
byte-identiques au SHA-256
`5906A7876CC191677C9D20B4F9785518B2A846CF66B73375262A6DD1910C712A`.
Le démarrage BKVince du 2026-08-05 à 20:11 active les sept call-sites avec le
catalogue runtime BKVince, les contributions socketed incluses et `Max Sockets`
masqué sur les objets socketed. Le résumé reste à
`scanned=13 active=12 disabled=0 rejected=0 failed=1`; l'échec unique demeure
le problème préexistant et distinct de `RepeatableServices.dll`.

## Extension 3.3.0 — affichage des plages avec Shift

La configuration publique accepte désormais `rangeDisplayMode` avec les
valeurs strictes `Always` et `HoldShift`. Le défaut `Always` conserve le
comportement 3.2.4. En mode `HoldShift`, les plages de propriétés et la ligne
`Base Defense` ne sont ajoutées que pendant que Shift est maintenu; la ligne
`Max Sockets` demeure gouvernée séparément par ses options existantes.

L'état visible/caché appartient à la clé du cache exact du tooltip. Les deux
variantes ne peuvent donc pas se contaminer après une pression ou un
relâchement de Shift. La variante cachée retourne avant toute résolution des
tables, affixes, runewords ou socket fillers afin de ne pas réintroduire le
délai de hover corrigé en 3.2.4. Aucun nouveau hook D2R ni aucune nouvelle ABI
n'est ajouté : les sept call-sites 92777 existants restent les seuls points
d'intégration.

Le build Release x64 passe les six suites CTest (policy, ranges, catalogue
vanilla embarqué, référence intermod, BKVince et configuration). La DLL Release
et la copie de package sont byte-identiques au SHA-256
`2A704B77B1FF4973DABCD8C7913F807A3E0F968B6672F860513381FEE8621FD7`.
La validation visuelle en jeu des transitions press/release et de la
comparaison Shift reste `not run`; aucune installation runtime 3.3.0 n'a encore
été effectuée.

## Archive publique 3.3.0

L'archive publique autonome
`addons/AdvancedItemTooltips/AdvancedItemTooltips-3.3.0.zip` contient uniquement
`AdvancedItemTooltips.dll` et `AdvancedItemTooltips.json`. Son SHA-256 est
`82CFF21427E5605AD63360081F83EDA6CA67D9A26CC661F0BD9C7702605DAFEF`.
La DLL embarquée est byte-identique au build Release 3.3.0 et conserve le
SHA-256
`2A704B77B1FF4973DABCD8C7913F807A3E0F968B6672F860513381FEE8621FD7`;
la configuration publique par défaut a le SHA-256
`44BBD4CC9E27CA98BBD7014AB9208BA629A8F5EB4C40CDBA4EBB7E34DD6C37B1`.
Les six suites CTest Release avaient passé avant la création de l'archive;
aucun cold start ni test visuel en jeu supplémentaire n'a été exécuté pour
cette livraison.
