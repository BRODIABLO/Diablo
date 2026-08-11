# PD2 Affixes Merge — fusion sélective et ID-stable pour BKVince

Dernière mise à jour : 10 août 2026

## Décision gouvernée

Vincent a confirmé le 10 août 2026 que **PD2 Affixes Merge** devient un
chantier distinct mené en parallèle, puis a donné la directive explicite
`IMPLEMENTE`. Cette autorisation ouvre l'implantation du comparateur, du
manifeste et des lots de données décrits ici.

Ce chantier parallèle ne remplace jamais la priorité désignée par
`Mission/CURRENT.md`. La qualification gameplay de `ProgressiveAffixesPlugin`
reste indépendante, avec des responsabilités différentes :

- `ProgressiveAffixesPlugin` gouverne le **nombre** de préfixes et suffixes
  générés selon la qualité, le niveau et le type d'objet ;
- `PD2 Affixes Merge` gouverne le **contenu** des pools, les niveaux
  d'éligibilité, les valeurs, groupes, types d'objets et localisations des
  affixes.

La directive `IMPLEMENTE` n'autorise ni le remplacement intégral des tables
BKVince, ni la copie indifférenciée de la source, ni la réinterprétation des IDs
d'affixes déjà enregistrés dans des objets.

Vincent a clarifié le 10 août 2026 que la sélection produit devait précéder
toute importation : identifier ce que PD2 ajoute, modifie ou supprime, comparer
chaque occurrence avec vanilla et BKVince, puis choisir explicitement. Le
prototype AFM-01/02 précédemment appliqué et poussé a donc été retiré des tables
de développement et du profil runtime. Il demeure uniquement une projection
technique et une simulation historique ; **aucun de ses affixes n'est approuvé
pour importation**.

## Objectif produit

Adapter à BKVince une sélection traçable d'affixes provenant des tables
officielles de **Project Diablo 2 Season 13** afin d'améliorer la qualité et la
diversité des rolls,
sans perdre les retunes BKVince, sans importer implicitement les systèmes de
maps ou de quivers et sans invalider les personnages, objets ou coffres
existants.

Chaque lot doit annoncer avant implantation un gain observable et sa mesure :
par exemple une réduction des tiers devenus indésirables à haut `ilvl`, une
augmentation du nombre d'options réellement utiles pour une famille d'objets,
ou l'ajout d'un concept absent. Aucun gain gameplay n'est encore démontré par
le seul écart de nombre de lignes.

## Périmètre

### Tables propriétaires

- `magicprefix.txt` ;
- `magicsuffix.txt` ;
- `automagic.txt`, dans un lot séparé après les préfixes et suffixes ;
- les variantes moderne et legacy de `item-nameaffixes.json` lorsque de
  nouvelles clés sont réellement nécessaires.

### Dépendances conditionnelles

Une dépendance n'entre dans un lot que si une ligne sélectionnée l'exige et si
son graphe est fermé : `itemtypes.txt`, `properties.txt`, `itemstatcost.txt`,
skills ou stats paramétrées et chaînes associées. L'ajout d'une dépendance ne
doit jamais devenir un prétexte pour fusionner sa table entière.

### Hors périmètre par défaut

- `RarePrefix.txt` et `RareSuffix.txt`, qui composent les noms rares mais ne
  portent pas les propriétés des affixes ;
- la progression du nombre d'affixes, possédée par
  `ProgressiveAffixesPlugin` ;
- les affixes réservés aux maps ou autres systèmes absents de BKVince ; les
  quivers ne sont pas dans cette catégorie, car `Arrows` et `Bolts` sont des
  bases spawnable BKVince, mais leur nouveau pool exige une décision produit ;
- une nouvelle DLL ou un patch mémoire ;
- toute migration destructive de sauvegardes ou renumérotation d'IDs.

## Baselines vérifiées au 10 août 2026

### Provenance de la source

La source autoritaire des trois tables d'affixes est l'archive officielle
**Project Diablo 2 Season 13** installée localement :
`C:\Games\Diablo II pd2\ProjectD2\pd2data.mpq`, SHA-256
`196F9DA7F5A7EEAD7BC000137514E6B564E50E893CA89BB4711359D03C29CE63`.
L'extraction contrôlée de `magicprefix.txt`, `magicsuffix.txt` et
`automagic.txt` prouve donc une provenance `pd2_core_confirmed` pour le
périmètre de ce chantier.

Le snapshot **PD2 Single Player Plus** au commit
`3debc6781f33c3c1474a995b80369a4e618cd386` reste un miroir de travail
hash-gaté. Pour ces trois tables seulement, la comparaison structurée montre
des cellules identiques à l'extraction officielle ; les hashes de fichiers
diffèrent uniquement parce que l'archive officielle emploie CRLF et le miroir
LF. Le générateur accepte l'une ou l'autre entrée uniquement lorsque son hash
épinglé correspond. Cette équivalence ne constitue pas une preuve de
provenance PD2 core pour les autres tables du miroir.

| Table | BKVince : lignes compilées | SHA-256 BKVince | PD2 S13 : lignes compilées | SHA-256 officiel CRLF | SHA-256 miroir LF |
|---|---:|---|---:|---|---|
| `magicprefix.txt` | 742 | `9A720A7A551489C19EEDF05FE0AF14317B774FEA66F9F13B418B68B05DA4A232` | 933 | `C819D18D85B5495685F610DBF03CC874F70D02C5F3B87198AF0B977CB3F36B06` | `8ECE2A56898E5ABEBC9B49D1AFFCCD02FF2F1FC815509FBFE2A1596FEE28941D` |
| `magicsuffix.txt` | 794 | `71725CF1C0AAD191BB35074474EA1B7E08558851D097B3B7283C72A9BE7B0C97` | 1 037 | `5652D6777196C8C8D0F3BE1A67A8D58EA0DF238DE1AFCE4EAD2DFC688684C470` | `9E7E952696C833710B58367FF3B7C83F3661B715B75DDF49132E8E8597F13711` |
| `automagic.txt` | 45 | `ACB99D4F703FEC5DC486144DD8856A1E8566EAC859FE49E526209E6604991C6A` | 58 | `848809670F5667D26D882B9D6BAE84A02C97D5F6E68216A41BFB64719BE1BCFD` | `AAF8A8ED63FD45C6AF2EEE4BE1F553843A62B413E0A86F1F6E038B7C41F4BD89` |

Les comptes compilés excluent la sentinelle `Expansion` des tables de
préfixes et suffixes. Les extractions et le miroir restent locaux sous
`analysis-cache/` et ne sont pas destinés à être commités.

## Faits vérifiés

- Les IDs d'affixes sont liés à l'ordre compilé des records. Les déplacer,
  insérer de nouvelles lignes avant eux ou déplacer la sentinelle `Expansion`
  peut faire relire un ancien objet avec une autre propriété.
- Le problème est observable dans `automagic.txt` : l'ID 37 correspond à
  `of Shadows` dans la baseline BKVince et à `Medium` dans le snapshot
  PD2/SP+. Une substitution de table changerait donc la signification d'un ID
  existant.
- `Name` n'est pas une clé unique : des noms et des lignes vides sont répétés.
  Aucun merge ne peut être fondé seulement sur ce champ.
- L'audit structurel initial a trouvé des ItemTypes, Properties et clés de
  localisation employés par des lignes spawnable de la source mais absents de
  BKVince. Ces dépendances doivent être reproduites dans un rapport versionné
  et fermées candidat par candidat avant toute sélection.
- La source contient des familles liées à des fonctionnalités absentes de
  BKVince. Leur présence dans la source n'établit ni leur utilité ni leur
  compatibilité.
- `AdvancedItemTooltips` et le BKVince Hero Editor consomment les ordinals et
  propriétés des affixes ; leurs tests sont donc des régressions obligatoires,
  pas des validations optionnelles voisines.

## Hypothèses à tester

1. Des retunes de `maxlevel`, `level`, `levelreq`, `frequency` ou de valeurs
   peuvent réduire les rolls faibles à haut `ilvl` sans ajouter de nouvel ID.
2. Certains concepts PD2 S13 append-only peuvent accroître la diversité utile
   de BKVince après fermeture de leurs dépendances.
3. L'ajout strictement en fin de table préserve les objets existants, à
   condition que tous les records BKVince et la sentinelle conservent leur
   ordinal compilé exact.
4. Les groupes et ItemTypes de la source peuvent modifier indirectement les
   fréquences et exclusions BKVince même lorsque les valeurs de stats semblent
   compatibles.
5. `automagic.txt` présente un risque de compatibilité supérieur, car ses IDs
   existants divergent déjà fortement ; il doit être qualifié après les deux
   tables Magic et jamais absorbé dans leur premier lot.

Ces points restent des hypothèses tant que les simulations, tests de
sauvegardes et témoins en jeu correspondants ne sont pas acquis.

## Architecture retenue

### Comparaison à trois voies

Le générateur compare séparément :

1. vanilla D2R 3.2 vers BKVince ;
2. vanilla D2R 3.2 vers les tables officielles PD2 S13 ;
3. BKVince vers PD2 S13 pour détecter les collisions et dépendances.

Cette structure distingue un changement déjà propre à BKVince d'un changement
de la source et évite d'écraser une retune locale sous prétexte de parité.

### Manifeste explicite

Chaque occurrence, y compris les noms dupliqués, reçoit une identité stable
composée au minimum de la table, de l'ordinal de baseline et d'un hash de ligne
normalisé. Chaque candidat porte obligatoirement :

- sa provenance et les hashes de baselines ;
- une disposition `keep_bkvince`, `retune`, `append`, `blocked` ou `exclude` ;
- la liste exacte des cellules possédées ;
- les dépendances et clés de localisation ;
- le gain attendu et sa mesure ;
- les tests et le rollback du lot.

Le générateur refuse une baseline dont le hash a changé, un candidat ambigu,
une dépendance non résolue ou une modification située hors de l'allowlist du
manifeste.

### Contrat ID-stable

- Tous les records compilés BKVince existants gardent leur ordinal et leur
  identité.
- Un `retune` modifie seulement les cellules manifestées de la ligne en place.
- Un `append` est ajouté exclusivement après le dernier record existant de la
  table ; aucune insertion ni réorganisation n'est permise.
- La sentinelle `Expansion` reste byte-identique et au même emplacement.
- Toute dépendance nouvelle est ajoutée atomiquement avec son premier
  consommateur, jamais avant ou après dans un état partiel.
- Après qu'un ID ajouté a pu être écrit dans une sauvegarde, sa ligne n'est
  plus supprimée ni déplacée : un retrait devient un tombstone non spawnable.

## Séquencement d'implantation

### AFM-00 — Oracle et manifeste reproductible

- épingler les six hashes de baseline et les schémas de headers ;
- générer l'inventaire occurrence par occurrence, le graphe des dépendances,
  les collisions de groupes et les clés de localisation ;
- produire la carte des IDs BKVince avant mutation ;
- classer chaque candidat sans modifier les tables gameplay.

### AFM-01 — Premier lot de retunes ID-neutres

- projection technique : **214 cellules sur 71 lignes** de
  `magicprefix.txt` et `magicsuffix.txt`, désormais restaurées à la baseline ;
- retenir seulement des changements de cellules sur des lignes BKVince
  existantes ;
- publier les distributions avant/après par famille d'objets et `ilvl` ;
- appliquer uniquement les retunes dont le gain est mesuré et approuvé ;
- ne toucher ni `automagic.txt` ni les dépendances structurelles dans ce lot.

### AFM-02 — Nouveaux préfixes et suffixes append-only

- projection technique retirée : **107 préfixes** aux IDs **743–849** et
  **93 suffixes** aux IDs **795–887** avaient été ajoutés après les records
  BKVince existants, puis restaurés avant toute utilisation par Vincent ;
- les **72 localisations modernes** et **79 legacy** projetées ont également
  été restaurées à la baseline ;
- sélectionner uniquement des concepts complets, indépendants des systèmes
  exclus ;
- fermer ItemTypes, Properties, stats, paramètres et localisations dans le
  même lot ;
- simuler fréquences, groupes et éligibilité avant écriture ;
- réserver définitivement chaque nouvel ordinal.

Les exclusions explicites de ce lot sont :

- `Artificer's`, occurrence source relocalisée d'une ligne vanilla déjà
  présente et retunée dans BKVince ;
- `Virulent`, `of Swords` et `of Decay`, dont au moins une valeur excède les
  bornes sérialisables de l'`ItemStatCost` BKVince correspondant.

### AFM-03 — AutoMagic séparé

- différer les **26 cellules sur 7 lignes** de retune identifiées et les
  **11 candidats** append-only tant que les collisions d'IDs/groupes et les
  gates propres à AutoMagic restent ouverts ;
- repartir de la carte complète des 45 IDs BKVince ;
- exclure toute substitution par ordinal depuis la source ;
- retuner en place ou ajouter en fin de table selon les mêmes règles ;
- valider séparément les bases portant déjà un AutoMagic et les objets sauvés.

## Gates observables

### Gate statique avant chaque lot

- [ ] Les hashes de baseline correspondent au manifeste.
- [ ] Chaque record compilé BKVince existant conserve table, ordinal et
  fingerprint d'identité hors cellules de retune explicitement autorisées.
- [ ] La sentinelle `Expansion` reste au même emplacement et byte-identique.
- [ ] Zéro ItemType, Property, stat, skill, paramètre ou clé de localisation
  manquant pour les candidats retenus.
- [ ] Zéro collision de groupe ou élargissement d'éligibilité non manifesté.
- [ ] Les TXT conservent leur transport d'octets gouverné, CRLF, saut de ligne
  final et round-trip byte-exact via `scripts/build-data/tsv.js`.
- [ ] `npm run verify:data`, les tests du générateur, le BKVince Hero Editor et
  les régressions AdvancedItemTooltips passent.
- [ ] `git diff --check` et le cadastre régénéré sont verts lorsque le lot est
  structurel.

### Gate de simulation

- [x] Les distributions Magic et Rare sont publiées aux `ilvl` 1, 45, 65, 85
  et 99 pour chaque famille touchée.
- [x] Les groupes, fréquences, exclusions et bornes min/max sont comparés avant
  et après sur un nombre d'échantillons déclaré.
- [ ] Le gain produit annoncé est visible dans la mesure et aucune famille non
  ciblée ne change sans décision explicite.

Le rapport gouverné `Mission/pd2-affixes-merge.distribution.json`, SHA-256
`A9ACD5594E68DF4F18D2E7B6AE182B2FEE0B4C9BE41C7E5B909DF79FD574DC83`,
compare les commits `756df5f5…` et `9a9c1a1f…`. Le modèle mesure un slot de
préfixe ou suffixe pondéré ; le nombre de slots demeure la responsabilité de
`ProgressiveAffixesPlugin`. Il couvre 64 familles concrètes, dont 43 changent
et 21 restent identiques, sur 698 pools Magic/Rare. Les probabilités exactes
sont contrôlées par 34 900 000 tirages déterministes, avec une erreur absolue
maximale de `0,0103678`, sous la tolérance gouvernée de `0,02`. Les 71 lignes
retunées et 200 lignes ajoutées sont toutes retrouvées, sans retrait.

La revue produit reste ouverte : `bowq` et `xboq` passent d'un pool vide à un
pool non vide, soit une distance de variation totale de `1`; les suffixes Rare
des orbes atteignent `0,504854` au niveau 65 et les préfixes Rare de `helm`
atteignent `0,432836`. Ces écarts sont mesurés, mais ne constituent pas encore
une approbation gameplay. Les quivers existent réellement dans BKVince comme
bases `Arrows` et `Bolts` spawnable ; ils ne peuvent donc plus être décrits
comme un système absent.

### Gate runtime

- [ ] Cold start du build 92777 avec la pile complète active, aucun plugin ni
  fonctionnalité du PluginPack neutralisé.
- [ ] Génération et inspection de témoins Magic, Rare et Crafted pour chaque
  candidat retenu ; AutoMagic reçoit sa propre matrice.
- [ ] Les plages, couleurs, noms et tooltips correspondent aux tables et ne
  débordent pas.
- [ ] Un jeu de sauvegardes antérieures au merge charge, conserve ses affixes
  et se resauvegarde sans réinterprétation d'ID.
- [ ] Les nouveaux objets survivent à sauvegarde/rechargement et à un rollback
  compatible par tombstone.
- [ ] Solo, hôte et joiner produisent les mêmes pools avec des données
  identiques ; aucune désynchronisation ni assertion.

Le prototype a connu un cold start technique, mais cette exécution est
supersédée par la restauration demandée par Vincent. Les quatre fichiers du
profil installé correspondent de nouveau aux baselines BKVince ; le jeu a été
arrêté et le seul personnage témoin contrôlé `PDTwoAffixes` a été retiré du
profil. Sa copie locale reste disponible uniquement comme preuve récupérable
sous `analysis-cache/`. Tous les gates runtime restent donc ouverts jusqu'à ce
qu'une sélection produit soit approuvée et réimplantée.

## Rollback

- Une retune se retire en restaurant uniquement ses cellules manifestées depuis
  la baseline hashée, jamais la ligne ou la table entière.
- Avant toute exposition à une sauvegarde, un append non validé peut être
  retiré avec son lot atomique. Après exposition possible, son ID demeure et la
  ligne devient `spawnable=0` avec une disposition tombstone documentée.
- Les dépendances append-only suivent la même règle de stabilité que leur
  consommateur ; aucune réutilisation d'ordinal libéré n'est permise.
- Les variantes moderne et legacy d'une localisation sont restaurées ensemble.
- Chaque lot conserve un manifeste inverse et les hashes source/runtime afin de
  prouver le retour à l'état attendu.
- Le rollback ne désactive jamais `ProgressiveAffixesPlugin` ni un composant de
  la pile complète pour obtenir artificiellement un démarrage réussi.

## État au 10 août 2026

- Chantier distinct : **confirmé**.
- Exécution en parallèle : **confirmée**.
- Directive `IMPLEMENTE` : **reçue**.
- Architecture de fusion sélective, ID-stable et manifestée : **retenue**.
- Source officielle PD2 S13 et miroir hash-gaté des trois tables :
  **équivalence structurée vérifiée**.
- AFM-01 et AFM-02 : **prototypes retirés**. `magicprefix.txt` est restauré au
  SHA-256 `9A720A7A551489C19EEDF05FE0AF14317B774FEA66F9F13B418B68B05DA4A232`
  et `magicsuffix.txt` au SHA-256
  `71725CF1C0AAD191BB35074474EA1B7E08558851D097B3B7283C72A9BE7B0C97`.
- Localisations moderne et legacy : **restaurées** aux SHA-256
  `EAAE1BB9A69A944D5E451CA26D0D299FEFD7F5FAF33FBFD5D1F7105102E4E998`
  et `6A9166FDA2CB3C50F1280BFEB8BCCD3BF8D9D0C7F88A2E4ADA41808164EE2C55`.
- AutoMagic : **différé** avec 26 cellules sur 7 lignes et 11 candidats encore
  soumis à ses collisions et gates propres ; `automagic.txt` demeure
  byte-identique, SHA-256
  `ACB99D4F703FEC5DC486144DD8856A1E8566EAC859FE49E526209E6604991C6A`.
- Tables de développement et runtime : **baseline BKVince restaurée**, CRLF et
  round-trip gouverné verts ; aucun affixe PD2 importé n'est actif.
- Comparateur produit V3 : **2 117 occurrences** dans
  `Mission/pd2-affixes-review.html`, avec Vanilla D2R 3.2, BKVince et PD2 S13
  côte à côte, les trois comparaisons bilatérales, les effets complets et tous
  les champs divergents. Les catégories de revue sont séparées, les familles
  sont regroupées et AutoMagic demeure masqué par défaut et différé.
- Documentation : carte gouvernée V2 de **510 correspondances
  occurrence-exactes** (`table`, `sourceRow`, `fingerprint`, nom, propriétés et
  types d'objet), matérialisées par **33 claims** et **33 règles**. La couverture
  dédupliquée comprend les **125** occurrences prouvées de `Removed Affixes`,
  **458** de `Affix Changes Compilation` et **19** de `Highest-Level Affixes`,
  figées sur la révision MediaWiki **23938** du `2026-07-26T11:49:54Z` (SHA1
  MediaWiki `018f1d4a6ec618b5a52fcc5307be0e538366fa5a`). Tous les liens
  `DOCUMENTED` visent `oldid=23938`. La couverture V3 distingue désormais
  **510 `DOCUMENTED`**, **701 `TABLE_ONLY`** et **906 `UNMAPPED`** ; aucun nom
  seul ni aucune règle ambiguë ne suffit à déclarer une occurrence documentée.
- Décisions : schéma V3 par champ et, pour chaque nouvel affixe, décision de
  ligne explicite (`IMPORT_PD2_AFFIX`, `EXCLUDE_PD2_AFFIX`, `DISCUSS` ou
  `IMPORT_CUSTOMIZED`), avec notes, reprise inter-machine et refus des exports
  dont le hash de comparaison, la source PD2, la baseline BKVince ou le
  fingerprint d'occurrence diffèrent. Tout `maxlevel` BKVince existant est
  protégé et préservé par défaut. Un champ `CUSTOM` rend
  `IMPORT_PD2_AFFIX` invalide et l'interface bascule vers `IMPORT_CUSTOMIZED` ;
  `UNCHANGED_BY_PD2` et `AUTOMAGIC_DEFERRED` sont en lecture seule et leurs
  anciens choix éventuels sont ignorés par le preview.
- Prévisualisation : `pd2-affixes-decisions-preview.mjs` produit seulement le
  manifeste proposé, les cellules, lignes, dépendances, localisations,
  conflits, décisions incomplètes et diff textuel. Il ne possède aucun chemin
  d'application gameplay. Avant toute projection, il exige les hashes exacts
  de `properties.txt`, `itemtypes.txt`, `itemstatcost.txt` et `skills.txt` côté
  PD2 officiel et BKVince, ainsi que les fichiers de localisation d'affixes et
  les manifestes complets des espaces moderne et legacy.
- Highest-Level Affixes V3 : rapport séparé de **147 occurrences** classé
  **PARTIALLY_RELEVANT**, avec analyse indépendante des versions vanilla,
  BKVince et PD2 selon présence, `spawnable`, `level`, `maxlevel`, catégories
  d'objets et accessibilité théorique. `of Vita` à la ligne source 340 est
  identique et `spawnable` dans les trois versions, mais son `alvl=110` dépasse
  la borne 99 :
  il reste **informatif, en attente de confirmation historique**, et ne prouve
  aucune régression BKVince. Le rapport distingue drops, crafts, rerolls et
  gambling sans inventer un chemin d'acquisition non prouvé.
- Suite ciblée comparateur + preview V3 : **27/27**, y compris le runtime HTML
  embarqué, les catégories en lecture seule, les conflits CUSTOM, tous les pins
  de dépendances et l'interdiction de chaque chemin `--apply`.
- Validation conjointe merge + preview + revue : **35/35**.
- La suite Hero globale conserve **six attentes hors périmètre déjà décalées**
  dans les baselines belt/monsters/sets/properties ; ce blocage n'est pas une
  régression du merge d'affixes.
- AdvancedItemTooltips : build Release vert, CTest **6/6** et témoin BKVince
  **1/1**.
- Distribution statique : **publiée et reproductible** — 64 familles, 698
  pools modifiés et 34 900 000 tirages de contrôle ; le test de merge passe
  désormais **8/8**.
- Déploiement du prototype : **retiré** ; profil runtime revenu aux quatre
  hashes de baseline et témoin contrôlé retiré sans toucher aux personnages de
  Vincent.
- Sélection produit, nouvelle implantation, validation runtime,
  sauvegarde/rechargement et hôte/joiner : **encore ouverts**.
- Positionnement : **chantier parallèle** ; `Mission/CURRENT.md` demeure
  inchangé par ce workstream.

## Prochain gate

Vincent examine `Mission/pd2-affixes-review.html`, décide chaque champ divergent
avec `KEEP_BKVINCE`, `ADOPT_PD2`, `CUSTOM` ou `DISCUSS`, tranche explicitement
chaque nouvelle ligne, ajoute ses notes puis exporte
`pd2-affixes-decisions-v3.json`. L'agent lance ensuite uniquement le compilateur
de prévisualisation, résout les conflits et dépendances, présente le lot exact et
attend une nouvelle directive `IMPLEMENTE` avant toute mutation gameplay. Les
chemins `--apply` restent interdits, aucun runtime ne reprend avant ce gate et
AutoMagic reste séparé et différé.
