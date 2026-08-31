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

## Validation

Depuis la racine du dépôt :

```powershell
node scripts/reverse-engineering/d2r33-datatables-atlas.mjs
node --test scripts/reverse-engineering/d2r33-datatables-atlas.test.mjs
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

A0+A1 ne génèrent aucun header C++, type Ghidra, extracteur statique, hook, DLL,
patch mémoire ou test runtime. Ces sorties appartiennent aux gates ultérieurs et
ne pourront consommer que les facts admis par ce catalogue.
