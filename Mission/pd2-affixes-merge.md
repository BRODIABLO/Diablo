# PD2 Affixes Merge — fusion sélective et ID-stable pour BKVince

Dernière mise à jour : 10 août 2026

## Décision gouvernée

Vincent a confirmé le 10 août 2026 que **PD2 Affixes Merge** devient un
chantier distinct mené en parallèle, puis a donné la directive explicite
`IMPLEMENTE`. Cette autorisation ouvre l'implantation du comparateur, du
manifeste et des lots de données décrits ici.

Ce chantier ne remplace pas la priorité courante : la qualification gameplay de
`ProgressiveAffixesPlugin` demeure prioritaire. Les deux missions possèdent des
responsabilités différentes :

- `ProgressiveAffixesPlugin` gouverne le **nombre** de préfixes et suffixes
  générés selon la qualité, le niveau et le type d'objet ;
- `PD2 Affixes Merge` gouverne le **contenu** des pools, les niveaux
  d'éligibilité, les valeurs, groupes, types d'objets et localisations des
  affixes.

La directive `IMPLEMENTE` n'autorise ni le remplacement intégral des tables
BKVince, ni la copie indifférenciée de la source, ni la réinterprétation des IDs
d'affixes déjà enregistrés dans des objets.

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
- les affixes réservés aux maps, quivers ou autres systèmes absents de
  BKVince ;
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

- application démontrée : **214 cellules sur 71 lignes** de
  `magicprefix.txt` et `magicsuffix.txt` ont été retunées en place ;
- retenir seulement des changements de cellules sur des lignes BKVince
  existantes ;
- publier les distributions avant/après par famille d'objets et `ilvl` ;
- appliquer uniquement les retunes dont le gain est mesuré et approuvé ;
- ne toucher ni `automagic.txt` ni les dépendances structurelles dans ce lot.

### AFM-02 — Nouveaux préfixes et suffixes append-only

- application démontrée : **107 préfixes** aux IDs **743–849** et
  **93 suffixes** aux IDs **795–887**, tous ajoutés après les records BKVince
  existants ;
- les localisations ajoutées comptent **72 entrées modernes** et **79 entrées
  legacy** ;
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

- [ ] Les distributions Magic et Rare sont publiées aux `ilvl` 1, 45, 65, 85
  et 99 pour chaque famille touchée.
- [ ] Les groupes, fréquences, exclusions et bornes min/max sont comparés avant
  et après sur un nombre d'échantillons déclaré.
- [ ] Le gain produit annoncé est visible dans la mesure et aucune famille non
  ciblée ne change sans décision explicite.

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
- AFM-01 appliqué : **214 cellules sur 71 lignes** ; hash final de
  `magicprefix.txt` :
  `BBA62664DDAE7A93C748B6E1B8AE09819132A5D1D21BF838104E1AFBC2B3EBC8`.
- AFM-02 appliqué : **107 préfixes aux IDs 743–849** et **93 suffixes aux IDs
  795–887**, avec **72 localisations modernes** et **79 legacy** ; hash final
  de `magicsuffix.txt` :
  `128BA6DBF9438024274DED2D5F03597CF027D43D388DE6B538232DB5D57E956E`.
- AutoMagic : **différé** avec 26 cellules sur 7 lignes et 11 candidats encore
  soumis à ses collisions et gates propres ; `automagic.txt` demeure
  byte-identique, SHA-256
  `ACB99D4F703FEC5DC486144DD8856A1E8566EAC859FE49E526209E6604991C6A`.
- Application aux tables et invariants statiques : **démontrés** — IDs et
  fingerprints historiques préservés, sentinelles `Expansion` inchangées,
  CRLF et round-trip gouverné verts.
- Validations du merge : **7/7**, `npm run verify:data` vert, test affixes du
  Hero Editor **3/3** et build Vite de production vert.
- La suite Hero globale conserve **six attentes hors périmètre déjà décalées**
  dans les baselines belt/monsters/sets/properties ; ce blocage n'est pas une
  régression du merge d'affixes.
- AdvancedItemTooltips : build Release vert, CTest **6/6** et témoin BKVince
  **1/1**.
- Simulation gameplay, validation runtime, sauvegarde/rechargement et
  hôte/joiner : **encore ouverts**.
- Priorité courante : **ProgressiveAffixesPlugin**, inchangée.

## Prochain gate

Publier et valider les distributions/simulations gameplay, puis exécuter la
matrice runtime avec la pile complète : anciens objets, nouveaux témoins
Magic/Rare/Crafted, sauvegarde/rechargement et hôte/joiner. Aucun résultat
gameplay ni aucune compatibilité de sauvegarde ne sera déclaré avant ces
preuves. AFM-03 reste différé jusqu'à la fermeture explicite de ses collisions
et gates propres.
