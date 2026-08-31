# D2R 3.3 DataTables Atlas

## Statut

Gates **A0+A1 livrés** le 31 août 2026 après le `GO` de Vincent. L’atlas reste
un workstream en pause et ne remplace pas la priorité active ISC12. Aucune
sortie A2, analyse A3, hook runtime, DLL, mutation du jeu ni matrice runtime
n’a été produit.

## Résultat A0+A1

- Le catalogue gouverné ferme `7/7` triplets `{records, count, stride}` pour
  `States`, `Skills`, `ItemStatCost`, `ItemTypes`, `Objects`, `Items` et
  `Shrines`.
- Les 28 preuves résolubles portent des claims exacts; une valeur ne peut plus
  être changée tout en conservant silencieusement la même citation.
- Les layouts de record restent volontairement partiels. `Objects.Parm0` et
  `Shrines.LevelMin` demeurent `candidate` parce que leurs offsets sont
  gouvernés mais que leur largeur n’est pas encore promue par un descriptor de
  compilation.
- Le validateur contrôle le schéma, les sources de preuve, les claims, les
  overlaps, les bounds de records, les statuts `proven/candidate/unknown` et la
  justification byte-exact des builds couverts.
- La suite négative passe `9/9`; aucun nouveau RE, RVA, type complet, hook,
  plugin, runtime ou fichier TXT n’a été ajouté.

## Objectif

Construire une source de vérité machine-readable et gouvernée pour les layouts
de tables compilées de D2R 3.3 : records TXT, slots du conteneur `DataTables`,
compteurs, linkers, strides, durées de vie et preuves associées. L’atlas doit
permettre aux futurs travaux de réutiliser les champs déjà établis sans les
redécouvrir, tout en refusant les structures complètes inventées à partir de
quelques offsets connus.

Le runtime officiellement ciblé est `D2R.exe 3.3.93847`. Le corpus historique
`reverse-engineering/d2r-3.2.92777/` demeure l’atelier natif commun aux builds
92777 et 93847 conformément à l’équivalence byte-exact gouvernée; le nom du
dossier décrit sa provenance et non une baseline runtime distincte.

## Friction observée

### Faits vérifiés

- `GetDataTablesForContext` et plusieurs couples records/compteur/stride sont
  déjà gouvernés dans `known-rvas.json`, `findings.md`, les missions natives et
  le registre `sdk-contribution/`.
- Le registre SDK compact ne centralise encore que `Skills`, `ItemTypes` et
  `Items`, alors que les preuves existantes couvrent aussi au moins `States`,
  `ItemStatCost`, `Objects` et `Shrines`.
- Plusieurs plugins conservent donc localement leurs propres constantes de
  slots, strides ou vues partielles, ce qui augmente la duplication et le coût
  de revue.
- Le dépôt D2RL-Plugins épinglé contient un modèle de descriptor TXT et un
  `D2ItemsTxt` détaillé, mais indique lui-même que le layout complet de
  `sgptDataTables` n’est pas encore cartographié. Cette référence reste une
  source de candidats; chaque promotion RuffnecKk exige une preuve indépendante
  dans l’image canonique gouvernée.

### Hypothèses à tester

- Une extraction statique des descriptors passés aux compilateurs TXT pourrait
  produire la majorité des champs directs sans instrumenter le runtime.
- Une génération de vues C++ partielles et de types Ghidra pourrait réduire les
  offsets recopiés sans introduire de dépendance runtime partagée.

### Inconnues conservées

- La couverture exacte des champs remplis après compilation.
- Les post-traitements, caches et durées de vie qui ne sont pas exprimés par les
  descriptors TXT.
- Le rendement réel d’un extracteur général au-delà du premier lot déjà prouvé.

## Architecture retenue — Option A

La source autoritaire future sera un catalogue JSON validé. Elle séparera :

1. les records compilés des tables TXT;
2. les slots du conteneur `DataTables` — records, compteurs, linkers et caches;
3. les preuves et contrats de durée de vie associés.

Chaque entrée devra porter au minimum le runtime ciblé, la surface couverte par
équivalence native, le nom de table, l’offset, la largeur, le type, le stride,
la confiance, la provenance, les témoins binaires, les consommateurs, le
post-traitement connu et le statut de validation.

Les headers C++ et types Ghidra seront des sorties générées. Les vues C++
resteront volontairement partielles : les zones inconnues seront du padding
anonyme, et les offsets/tailles connus seront fermés par `static_assert`.

Les tables de callbacks et leurs ABI ne font pas partie de la v1. Elles pourront
former une extension distincte après démonstration d’un besoin, afin de ne pas
confondre les records TXT avec les tables de fonctions.

## Séquencement retenu

### A0 — Contrat et validation

- définir le schéma machine-readable et ses invariants;
- définir les niveaux `proven`, `candidate` et `unknown` sans promotion
  implicite;
- implanter un validateur déterministe qui refuse chevauchements, strides
  incohérents, preuves absentes et généralisation de build non justifiée.

### A1 — Consolidation sans nouveau RE

Importer uniquement les preuves gouvernées existantes pour :

- `States.txt`;
- `Skills.txt`;
- `ItemStatCost.txt`;
- `ItemTypes.txt`;
- `Objects.txt`;
- `Armor.txt` / `Weapons.txt` / `Misc.txt` compilés dans `Items`;
- `Shrines.txt`.

Ce lot doit fermer, pour chaque table disponible, le triplet
`{records, count, stride}` ou indiquer explicitement la composante encore
inconnue. Aucun nouveau RVA n’est inventé pour compléter artificiellement le
lot.

### A2 — Sorties générées

- produire les vues C++ partielles et leurs `static_assert`;
- produire un format importable ou reproductible pour Ghidra;
- exiger deux générations byte-identiques à partir du même catalogue.

### A3 — Extracteur statique candidat

- inventorier les descriptors directement dans l’image canonique et les
  callsites de compilation;
- produire seulement des candidats tant que records, compteurs, strides,
  consommateurs et post-traitements ne sont pas indépendamment fermés;
- interdire tout hook runtime dans cette phase.

### A4 — Promotion à la demande

Étendre l’atlas lorsqu’un plugin, patch ou diagnostic démontre un besoin réel.
Une nouvelle preuve stable rejoint `known-rvas.json` avant ou avec sa promotion
dans l’atlas.

## Gates observables

- Schéma et catalogue valides, sans champ prouvé dépourvu de provenance.
- Zéro overlap entre champs, sauf union explicitement documentée.
- Chaque stride et chaque offset de conteneur possède un témoin natif exact.
- Chaque surface déclarée commune à 92777/93847 est byte-exactement couverte par
  la preuve gouvernée.
- Les champs post-compilation restent séparés des descriptors directs.
- Les sorties générées sont reproductibles et compilent avec leurs
  `static_assert`.
- Aucun changement runtime, DLL, configuration joueur, sauvegarde ou table TXT
  n’est produit par la v1.
- Le gain est mesuré par le nombre de tables fermées sur
  `{records, count, stride}`, le nombre de champs à haute confiance et la
  réduction des offsets dupliqués dans les futurs travaux.

## Validation A0+A1

1. `npm.cmd run re:d2r33 -- status` : workbench, image, index et Ghidra prêts.
2. `node scripts/reverse-engineering/d2r33-datatables-atlas.mjs` : `VALID`,
   `7/7` triplets, 28 preuves, builds 92777 et 93847 couverts.
3. `node --test scripts/reverse-engineering/d2r33-datatables-atlas.test.mjs` :
   `9/9` tests réussis.
4. Validation du cadastre, `git diff --check` et examen du diff dédié avant le
   handoff.

La génération déterministe et la compilation des `static_assert` appartiennent
à A2 et n’entrent pas dans le résultat A0+A1.

Aucune matrice runtime n’est requise pour le catalogue documentaire seul. Une
future action qui lirait ou modifierait réellement le processus devra ouvrir un
gate séparé avec `d2r-runtime-validation`.

## Rollback

Le chantier est additif. Avant toute consommation par un plugin, le rollback
consiste à retirer le catalogue, ses sorties et ses scripts, puis à restaurer
les seules entrées ROADMAP/workstream associées. Aucun état runtime ni format de
sauvegarde n’est affecté.

## Frontière Git et prochain gate

Le futur lot possède `Mission/d2r33-datatables-atlas.md`, le répertoire dédié
`reverse-engineering/d2r-3.2.92777/datatables-atlas/**` et ses scripts dédiés.
`known-rvas.json`, `findings.md`, `sdk-contribution/`, `ROADMAP.html`,
`Mission/WORKSTREAMS.json` et le cadastre restent des registres partagés à
modifier chirurgicalement. `Mission/CURRENT.md` demeure sur ISC12.

**Prochain gate : A2.** Après un nouveau `GO`, générer depuis le catalogue les
vues C++ partielles et le format Ghidra reproductible, exécuter deux générations
byte-identiques et compiler un témoin couvrant tous les `static_assert`. A3 et
le runtime restent exclus.
