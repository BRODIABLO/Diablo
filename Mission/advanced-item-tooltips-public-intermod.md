# Advanced Item Tooltips — Public Intermod Release

Dernière mise à jour : 31 juillet 2026

## Décision produit

Vincent choisit une version publique **autonome permanente**, distincte de la
version personnelle BKVince 2.2.0. Elle demeure attribuée à `RuffnecKk`, peut
être installée globalement ou dans le dossier d'un mod et ne sera ni mergée au
PluginPack ni dépendante de Transmogrify, d'une DLL eezstreet ou de BKVince.

La séquence retenue est **compatibilité d'abord** : définir et tester le contrat
des tables du mod actif avant de créer le package public. Le code et la DLL
BKVince restent figés pendant ce travail; la déclinaison publique vivra sous
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

Prochain lot de phase 1 : convertir les cas synthétiques en fixtures de tables
déterministes, puis prouver l'extraction de la contribution réellement roulée
d'une jewel avant d'implanter le loader public à capacités indépendantes.

## Gates d'implantation et de livraison

- Créer seulement après la phase 1 le source et le package public sous
  `addons/AdvancedItemTooltips/`.
- Conserver le manifeste v2, les exports D2RLoader, l'auteur `RuffnecKk`, les
  signatures strictes et le refus sûr d'un autre build.
- Valider configuration absente, valide et invalide, avec priorité mod-local
  puis repli global.
- Valider les deux portées d'installation et la coexistence avec les cinq DLL
  eezstreet, Transmogrify et les plugins autonomes disponibles, sans rejet ni
  échec.
- Vérifier les contextes inventory, stash, Cube, marchand, sol et comparaison,
  ainsi que souris/manette et solo/hôte/joiner sans changement de sauvegarde ni
  de gameplay.
- Produire un ZIP public contenant uniquement `AdvancedItemTooltips.dll` et
  `AdvancedItemTooltips.json`, puis inspecter ses entrées et consigner les
  SHA-256 du ZIP, de la DLL testée et du runtime.

## Prochain gate

Auditer le chargeur de tables actuel et formaliser la matrice de fixtures du
contrat intermod. Aucun source public, JSON de distribution ou ZIP ne doit être
créé avant que cette matrice puisse distinguer une donnée supportée d'une
provenance à omettre de façon sûre.
