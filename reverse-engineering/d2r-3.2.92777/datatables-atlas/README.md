# Atlas D2R 3.3 des DataTables compilées

Cet atlas évite de redécouvrir les mêmes pointeurs, comptes, strides et fragments
de records à chaque chantier de reverse engineering. Il ne prétend pas que les
headers des fichiers `.txt` correspondent directement à une structure C++ : le
loader compile les sources TSV, peut fusionner plusieurs tables et peut construire
des index ou linkers après la compilation.

La cible produit est `D2R.exe 3.3.93847`. Les preuves natives proviennent du
corpus gouverné commun conservé sous le chemin historique
`reverse-engineering/d2r-3.2.92777/`; la couverture de `92777` et `93847` est
déclarée séparément dans le catalogue et doit rester justifiée par des preuves
byte-exact.

## Contenu A0+A1

- `atlas.schema.json` définit l'enveloppe machine-readable et les trois niveaux
  de confiance `proven`, `candidate` et `unknown`.
- `catalog.json` consolide les preuves existantes pour `States`, `Skills`,
  `ItemStatCost`, `ItemTypes`, `Objects`, `Items` et `Shrines`.
- `scripts/reverse-engineering/d2r33-datatables-atlas.mjs` valide le schéma, les
  citations, les claims exacts, la géométrie des slots et des fields, ainsi que
  la couverture des builds.
- `scripts/reverse-engineering/d2r33-datatables-atlas.test.mjs` couvre les refus
  fail-closed essentiels.

Chaque table A1 ferme au minimum le triplet suivant :

| Élément | Sens |
|---|---|
| `records` | offset du pointeur de records dans le conteneur `DataTables` |
| `count` | offset du compte dans le même conteneur |
| `recordSize` | stride du record compilé |

Les `recordFields` sont volontairement clairsemés. Un champ absent n'est pas
zéro, inutilisé ou inexistant : il est simplement hors de la preuve consolidée.
Les zones `postProcessing` séparent explicitement le layout brut des linkers,
LUT et index construits après la compilation.

## Sorties A2

Le dossier `generated/` est entièrement dérivé de `catalog.json` :

- `d2r33_datatables_atlas.hpp` fournit sept vues de records partiels et
  `DataTablesKnownPrefixView`; ses 45 `static_assert` ferment les strides,
  les 21 fields admis et les 15 slots connus;
- `d2r33_datatables_ghidra.h` fournit les mêmes vues sous forme de header C
  importable avec **Parse C Source** dans Ghidra, plus les constantes d'offset;
- `d2r33_datatables_atlas_witness.cpp` force la compilation de toutes les
  assertions;
- `manifest.json` lie le catalogue et les trois sorties de code par SHA-256,
  taille et runtime couvert.

Les octets non établis restent des tableaux `unknownXXXX`. Le type
`DataTablesKnownPrefixView` s'arrête au dernier slot gouverné et ne prétend pas
décrire la fin réelle du conteneur. Les commentaires `candidate` de
`Objects.Parm0` et `Shrines.LevelMin` sont conservés dans les deux formats.

## Validation

Depuis la racine du dépôt :

```powershell
node scripts/reverse-engineering/d2r33-datatables-atlas.mjs
node --test scripts/reverse-engineering/d2r33-datatables-atlas.test.mjs
node scripts/reverse-engineering/d2r33-datatables-generate.mjs --check
node --test scripts/reverse-engineering/d2r33-datatables-generate.test.mjs
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/reverse-engineering/d2r33-datatables-compile.ps1
```

Le validateur refuse notamment :

- une valeur `proven` ou `candidate` sans preuve;
- une valeur qui ne correspond plus au claim exact de sa preuve;
- un slot `DataTables` ou un field de record qui chevauche un autre;
- un field qui dépasse le stride du record;
- une valeur concrète marquée `unknown`;
- une RVA gouvernée, un site natif, un pointeur JSON ou un témoin texte devenu
  introuvable;
- l'ajout d'un build couvert sans justification byte-exact distincte.

## Limites de la gate

A2 génère les deux formats documentaires, mais aucun extracteur statique, hook,
DLL, patch mémoire ou test runtime. Les headers ne deviennent pas une ABI SDK
complète : ils consomment seulement les facts admis par le catalogue et
conservent le reste inconnu. L'inventaire statique de nouveaux descriptors
appartient à A3 et exige un gate séparé.
