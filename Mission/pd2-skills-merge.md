# PD2 Skills Merge Workbench

Statut : **implantation du comparateur autorisée; toute implantation gameplay reste interdite**.

Produit : **PD2 Skills Merge Workbench — Comparateur PD2 / BKVince / Vanilla D2R 3.2**.

Cette mission est un chantier parallèle de sélection. Elle ne déplace ni
`Mission/CURRENT.md`, ni la priorité BKVCombat courante, ni le prochain gate de
la ROADMAP. Aucun skill PD2 n'est approuvé par défaut.

## Objectif et frontière

Le Workbench doit fournir une revue autonome, exhaustive et gouvernée des
skills PD2/SP+, BKVince et Vanilla D2R 3.2. Il génère un oracle JSON, un seul
HTML utilisable sous `file://`, un fichier de décisions validable et un preview
d'implantation strictement read-only.

Le chantier peut écrire uniquement ses sources, tests et artefacts documentaires
sous `scripts/`, `Mission/` et `analysis-cache/`. Il ne peut écrire dans aucune
table gameplay, sauvegarde, configuration D2RLoader ou installation runtime. Il
n'expose aucun mode `--apply`.

## Faits vérifiés au gel v1

- Autorité au gel initial : HEAD `948fc130ecbf726f9c791f4e09491aaeb678d02c`.
- Autorité finale revalidée : HEAD `357ac120b53de9539cf5774b851e806dab57926d`; les commits concurrents intervenus pendant le chantier n'ont modifié aucune source, table ni contrat V3 consommé par le Workbench, et les 45 hashes de sources sont inchangés.
- Base architecturale acceptée pour le gate opérationnel : commit
  `a22fad7bb3a5da15619bc86c7dc06b3d358fbe94`. Le durcissement de transport,
  le smoke Chromium et le gate CI ne changent ni son modèle logique, ni le
  `comparisonHash`, ni les fingerprints.
- Baseline PD2/SP+ : commit documenté
  `3debc6781f33c3c1474a995b80369a4e618cd386`, tree
  `6f51e17e5f65abdd50b2fd33190c571fef296ccf`; le snapshot local n'a pas de
  répertoire `.git`, donc les hashes de fichiers constituent le gate local.
- `Skills.txt` PD2 : 603 lignes, 256 colonnes, LF, SHA-256
  `AEEFC3F2C0C80811D62FC1A17C3B031DE2164E5606BF9779F34024B35BC87B8B`.
- Vanilla D2R 3.2 : extraction gouvernée sous
  `data-vanilla3.2/data/data/global/excel`; `skills.txt` compte 429 lignes et
  porte le SHA-256
  `EFAF7AC4BA0493109C698EF32ACF4A2B3A577E13500D0B50258C80B600986F51`.
- BKVince HEAD : `skills.txt` compte désormais **451** lignes, 322 colonnes,
  CRLF, SHA-256
  `08497CC0BD8B2B5CBD895F7477AD0CBF272571FB67B78061EDFABC31C48B8B77`.
  Le prochain ordinal potentiel est donc calculé à **451**, jamais codé en dur.
- Le rapport historique du 8 août portait 449 lignes BKVince et 108 collisions.
  Le HEAD actuel porte 451 lignes et **110 collisions**; cet écart doit être
  expliqué et testé, pas masqué.
- `pettype.txt` BKVince a été supprimé volontairement au commit
  `cab9776193327bf8470dda051cc368fe09687de2`. Son absence est modélisée comme
  héritage explicite de Vanilla 3.2 avec provenance; le Workbench ne restaure ni
  n'invente une table BKVince.
- La page PD2 épinglée reste `Skill Changes`, révision `23785` du
  17 juin 2026. Son contenu est documentaire; une affirmation wiki ne devient
  jamais une preuve native ou une autorisation de portage.
- Le workbench natif D2R 3.2.92777 est vérifié : image canonique SHA-256
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`,
  image d'analyse SHA-256
  `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`.
  Une absence de preuve reste `NATIVE_UNPROVEN`.

### Dérive gouvernée de la baseline du 8 août

L'audit analytique du 8 août reste une **baseline historique immuable** : son
SHA-256 est
`BE9385A532CBD3DF80D94E83F04293FB9238DFD10101C5A9F442DB8DAC07D565` et
son état observé s'arrêtait à l'ordinal BKVince 448. Il comptait donc 449
lignes, 108 occupations d'ordinal en collision avec PD2 et proposait 449 comme
prochain ordinal append-only. Le Workbench ne réinterprète pas ces nombres comme
la baseline courante et ne régénère pas silencieusement cet audit historique.

Le commit BKVince
`b731e9a2d2f53f1fbb9fc60078880bb7d817888f` du 10 août 2026
(`Implement PD2 monster merge lots (prototype)`) a ensuite ajouté exactement
deux lignes à la fin de `skills.txt`, sans déplacer les 449 lignes existantes :

| Ordinal runtime | Ligne physique | Skill BKVince | `*Id` documentaire | `charclass` | `skilldesc` | Occupant PD2 du même ordinal |
|---:|---:|---|---:|---|---|---|
| 449 | 451 | `BKV BloodRaven Immo` | 10000 | vide, technique/classless | `immolation arrow` | `Iceboss Blizzard` |
| 450 | 452 | `BKV Baal Lowres` | 10001 | vide, technique/classless | `lower resist` | `Lightning Strike Cowboss` |

Les fingerprints de lignes BKVince sont respectivement
`29C2448FBC6EB6C1DEDF95C9405E41EB3773AA3C213019BD7C454314A88B47F7` et
`37376A66ACB06D91AC352EF8590FCCC9E2D88368B5DAD06DEEF2A3651CE9963D`.

Ces deux occupations sont deux nouvelles collisions techniques
`SAME_ORDINAL_DIFFERENT_SKILL`, respectivement
`collision:pd2-bkvince:449` et `collision:pd2-bkvince:450`. Elles restent
`UNRESOLVED_NO_AUTOMATIC_MERGE`; les deux lignes BKVince ne sont ni des skills
joueurs ni de nouveaux candidats PD2. Elles expliquent exactement le passage de
449/108/prochain 449 à **451 lignes / 110 collisions / prochain ordinal 451**.
Leurs fingerprints de collision sont
`CBAEA1B2B85E31B1D676062E84207FA567CFE480D4002E11E13A25406CE49B12` et
`37076F81903A90B62B69D54250036C94D47684F624155F89BB121906A6A1CF7B`.

L'interface et les exports doivent toujours qualifier 449/108 de
`historicalBaseline` et 451/110 de `currentBaseline`. L'allocation append-only
utilise exclusivement `coverage.nextAppendOrdinal`, calculé depuis la longueur
de la baseline courante; le champ `*Id` 10000/10001 ne participe jamais au
calcul.

## Choix d'architecture

### Références V3 auditées

Le chantier a étudié au HEAD les contrats de
`Mission/pd2-affixes-review.html`, `Mission/pd2-affixes-review.json`,
`scripts/migrate-bkvince/pd2-affixes-review.mjs`,
`scripts/migrate-bkvince/pd2-affixes-decision-rules.mjs` et
`scripts/migrate-bkvince/pd2-affixes-decisions-preview.mjs`, ainsi que leurs
tests. Les jalons historiques `665a627a90bef5e06f57ccb41939aef76cb02aad`,
`d9e0d5928debe9321d5bfc4a07b572bd3bb2d068` et
`3b08c22b1355c69c6a86f562d6dca1f41b7d35bd` ont été inspectés pour distinguer
la première enveloppe V3, le durcissement des gates et les clarifications de
classification.

Les principes réutilisés sont la comparaison trois voies, l'enveloppe de
décisions liée au hash, les fingerprints, les protections, les exports et le
preview séparé. Le modèle d'occurrences d'affixes n'est pas réutilisé : les
skills possèdent leurs propres identités sémantiques, ordinals physiques,
arbres, formules et graphes de dépendances.

### Option A — étendre le comparateur d'affixes

Elle réutiliserait directement ses occurrences et catégories. Elle est rapide,
mais confondrait familles d'affixes, graphes de skills, collisions d'ordinals,
arbres, formules et dépendances natives. Le risque de régression du V3 existant
est élevé.

### Option B — un générateur/HTML monolithique

Elle minimise le nombre de modules, mais rend la persistance, les décisions, les
tests et le preview difficiles à isoler. Le coût de maintenance et le risque de
divergence Node/navigateur sont élevés.

### Option C retenue — fork skill-specific à contrats purs

Le V3 fournit les principes de gouvernance, sans imposer son modèle affixe :

1. contrat partagé et moteur de décision pur;
2. générateur déterministe et oracle JSON canonique;
3. générateur d'un HTML autonome avec oracle et runtime embarqués;
4. preview compiler séparé, atomique et read-only;
5. tests structurés du moteur, des données, du HTML et du preview.

Cette voie conserve une distribution simple, réduit les régressions, permet une
validation fine et reste entièrement réversible puisque seules des décisions et
des prévisualisations sont produites.

## Contrat canonique d'un skill

Le fichier autoritaire des enums et interfaces est
`scripts/migrate-bkvince/pd2-skills-review-contracts.mjs`. Son hash gelé est
embarqué dans l'oracle.

Un skill canonique contient au minimum :

- `stableId` sémantique;
- `canonicalName`, aliases et classe/portée;
- trois nœuds physiques optionnels `vanilla32`, `bkvince`, `pd2`;
- coordonnées d'arbre issues de `skilldesc.txt` et ordre déterministe
  page/row/column;
- `mappingTypes`, arêtes sémantiques et collisions d'ordinal séparées;
- statut général, résumé joueur dérivé, différences significatives;
- douze composantes gameplay, chacune avec champs et comparaison trois voies;
- courbes L1/5/10/20/30/40 et scénarios de synergies gouvernés;
- formules brutes, preuve de calcul et valeurs symboliques lorsqu'elles ne sont
  pas calculables;
- dépendances directes et fermeture transitive avec provenance;
- consommateurs textuels, ordinals et propriétés connus;
- classification de portabilité explicable, risques et preuves requises;
- fingerprint canonique couvrant identité, mapping, lignes brutes, dépendances
  et protections.

La forme JSON figée de l'oracle est :

```text
{
  schemaVersion, reviewId, productName, state, frozenContractHash,
  comparisonHash, sourceManifest, sourceHashes, policyHashes, levels, enums,
  coverage, navigation[], nodes[], skills[], collisions[], documentation
}

nodes[] = {
  id, source, ordinal, line, declaredId, name, normalizedName, classCode,
  skilldescKey, rowFingerprint, raw, tree, linkedRows, formulaFindings
}

skills[] = {
  stableId, fingerprint, canonicalName, aliases[], names, classCode, scope,
  playerSkill, newPd2PlayerSkill, bkvinceOnlyPlayerSkill, nodeIds, ordinals,
  tree, mappingTypes[], primaryMappingType, identical, readOnly, status,
  collisionIds[], summary, evidence, portability, components[], curves,
  dependencies[], consumers[], documentation[], newSkillPlan?
}

components[] = {
  id, label, fingerprint, proofStatus, portability, changed, fields[]
}

fields[] = {
  id, table, header, label, values, displayValues, changed, protected,
  protectionReasons[], proofStatus, formula?, dependencyIds[]
}
```

`nodes[].raw` conserve les cellules brutes une fois par nœud. Les composants
référencent ces valeurs sans dupliquer une ligne entière. `navigation[]` ne
contient que des `stableId`; une skill répétée dans une vue collision ou une vue
de classe partage toujours une décision unique.

Les nœuds physiques sont identifiés par
`<source>:skills.txt:<ordinal-zero-based>`. Le champ PD2 `Id` et le champ BKVince
`*Id` restent documentaires. Ils ne participent ni aux IDs stables, ni au
mapping, ni aux collisions, ni au choix d'un nouvel ordinal.

L'identité sémantique utilise `skill:<scope>:<slug>`, avec discriminateur
gouverné seulement en cas de collision de slug. Un changement d'ordinal ne
change pas cet ID; il change le nœud physique et donc le fingerprint.

## Graphes et catégories de mapping

Deux graphes indépendants sont obligatoires :

1. identité sémantique entre versions;
2. occupation des ordinals runtime.

Les catégories gelées sont :

- `SAME_SKILL_SAME_ORDINAL`;
- `SAME_SKILL_MOVED_ORDINAL`;
- `RENAMED_ALIAS`;
- `SLOT_REPLACEMENT`;
- `PD2_ONLY_PLAYER_SKILL`;
- `BKV_ONLY_PLAYER_SKILL`;
- `SAME_ORDINAL_DIFFERENT_SKILL`;
- `TECHNICAL_OR_CLASSLESS`;
- `IDENTICAL`.

Elles sont cumulables. `Cold Enchant` conserve donc son arête sémantique PD2 40
vers BKVince 408 et une arête de collision PD2 40 vers `Frozen Armor` BKVince
40. Aucune fusion automatique n'est permise.

Les nouveaux candidats joueurs sont déterminés par classe, métadonnées
`skilldesc`, emplacement réel et politique gouvernée. Un simple `charclass`
n'est pas suffisant : slots temporaires, lignes Proc, maps, mercenaires,
monstres, helpers et classless restent dans les vues techniques ou collisions.

## Composantes de gameplay

Les douze groupes gelés sont :

1. identité et disponibilité;
2. coût et timing;
3. modèle de dégâts;
4. zone et ciblage;
5. projectiles et collisions;
6. animation et séquence;
7. buffs, debuffs, auras et passifs;
8. synergies;
9. summons;
10. fonctions moteur;
11. interface et localisation;
12. consommateurs.

Chaque champ appartient à exactement un groupe primaire. Les arêtes vers une
autre table conservent leur propre composante et leur provenance.

## Preuves et formules

Les statuts autorisés sont :

- `EXACT_TABLE`;
- `EXACT_FORMULA`;
- `EXACT_DERIVED`;
- `SYMBOLIC`;
- `MALFORMED_SOURCE`;
- `UNSUPPORTED_IDENTIFIER`;
- `NATIVE_UNPROVEN`.

Une formule malformée est conservée octet pour octet. Le moteur n'ajoute jamais
une parenthèse, ne substitue jamais un alias et ne transforme jamais une valeur
symbolique ou native non prouvée en nombre. Le scénario standard fixe le niveau
effectif `L`, les hard points `B=min(L,maxlvl)` et les autres stats/masteries à
zéro. Les scénarios de synergies ne calculent que les expressions dont toutes
les dépendances sont prouvées.

Les fonctions `srv*`, `clt*` et missile sont des nombres propres à une source.
Une égalité numérique ne prouve pas une égalité comportementale; une divergence
est `NATIVE_FUNCTION_MISMATCH`, et une copie envisagée reste bloquée tant que la
preuve D2R 3.2 requise n'est pas disponible.

## Portabilité

Les catégories gelées sont :

- `DATA_ONLY_PROVEN`;
- `DATA_WITH_LINKED_TABLES`;
- `APPEND_ONLY_REQUIRED`;
- `NATIVE_FUNCTION_MISMATCH`;
- `NATIVE_UNPROVEN`;
- `BLOCKED_DEPENDENCY`;
- `SAVE_OR_ID_RISK`;
- `NETWORK_OR_CLIENT_SERVER_RISK`;
- `NOT_APPLICABLE`.

Une skill et chacune de ses composantes peuvent porter plusieurs catégories.
Le résumé doit expliquer raisons, tables, dépendances, collisions, fonctions,
risques save et réseau, effort et preuve encore requise. Aucun score opaque ne
remplace ces raisons.

## Décisions

Décisions globales : `KEEP_BKVINCE`, `ADAPT_PD2_SELECTIVELY`,
`ADOPT_PD2_MODEL`, `IMPORT_NEW_PD2_SKILL`, `REJECT_PD2`,
`DEFER_NATIVE_PROOF`, `DISCUSS`.

Décisions composante/champ : `KEEP_BKVINCE`, `ADOPT_PD2`, `CUSTOM`,
`DISCUSS`, `NOT_APPLICABLE`.

Décisions de ligne d'un nouveau skill : `IMPORT_APPEND_ONLY`,
`IMPORT_CUSTOMIZED`, `REJECT_PD2_SKILL`, `DEFER_NATIVE_PROOF`, `DISCUSS`.
La décision de ligne précède obligatoirement les décisions de champs.

`CUSTOM` exige une valeur ou formule explicite et une justification. Le modèle
conserve aussi objectif gameplay et plan de test. Une décision protégée exige un
objet d'override explicite avec justification, avertissement et preuve suffisante.

Le statut d'implantation est indépendant : `NOT_REVIEWED`,
`DECISION_INCOMPLETE`, `DECISION_COMPLETE`, `SELECTED_FOR_PROTOTYPE`,
`IMPLEMENTATION_NOT_AUTHORIZED`, `IMPLEMENTATION_AUTHORIZED`, `IMPLEMENTED`,
`TESTED`, `REJECTED`. Le Workbench ne produit jamais automatiquement
`IMPLEMENTATION_AUTHORIZED`.

La complétude d'une décision suit une règle gouvernée et non un simple clic :

- `KEEP_BKVINCE`, `ADAPT_PD2_SELECTIVELY`, `ADOPT_PD2_MODEL`,
  `IMPORT_NEW_PD2_SKILL` et `REJECT_PD2` exigent une justification finale;
- `ADAPT_PD2_SELECTIVELY`, `ADOPT_PD2_MODEL` et `IMPORT_NEW_PD2_SKILL`
  exigent aussi un plan de test;
- `DISCUSS` et `DEFER_NATIVE_PROOF` exigent au moins une note générale ou une
  justification finale décrivant la question ou la preuve manquante;
- tout statut `SELECTED_FOR_PROTOTYPE` ou plus avancé exige un plan de test;
- les objectifs de design et le problème BKVince restent disponibles sans être
  rendus artificiellement obligatoires lorsqu'ils ne s'appliquent pas.

Les skills strictement identiques sont auto-résolus en lecture seule. Les
décisions injectées sur une entrée read-only sont refusées à l'import et au
preview.

L'enveloppe de décisions figée est :

```text
{
  schemaVersion: 1,
  kind: "pd2-skills-review-decisions",
  reviewId, comparisonHash, frozenContractHash, sourceHashes,
  exportedAt, exportScope: "ALL" | "COMPLETE_ONLY",
  entries: {
    <stableId>: {
      fingerprint,
      globalDecision,
      newSkillLineDecision?,
      implementationStatus,
      componentDecisions: { <componentId>: Choice },
      fieldDecisions: { <fieldId>: Choice },
      notes: {
        general, designObjective, bkvinceProblem, finalJustification, testPlan
      }
    }
  }
}

Choice = {
  decision,
  customValue?,
  justification?,
  gameplayObjective?,
  testPlan?,
  protectedOverride?: {
    approved: true,
    justification,
    acknowledgedProofStatus,
    nativeRiskAccepted?,
    malformedResolution?
  }
}
```

Un choix de composante fournit le fallback de tous ses champs. Un choix de champ
le remplace. Ainsi le preview résout chaque cellule exacte sans exiger une saisie
répétitive lorsque Vincent adopte ou conserve une composante entière.

## Champs protégés

Sont protégés par défaut : ordinal runtime, ordre des lignes BKVince, lignes
Warlock, IDs persistants de states/ItemStatCost, save/send bits, `maxlvl`,
`charclass`, fonctions natives divergentes, toute traduction `delay` vers
`localdelay/globaldelay`, formules malformées et champs liés à une collision non
résolue.

Les actions en lot remplissent uniquement les décisions indécises par défaut.
Le remplacement intégral exige confirmation et ne peut pas écraser silencieusement
un `CUSTOM` ou une note. L'adoption en lot conserve tous les champs protégés.

## Persistance, migration et exports

La clé locale est exactement
`pd2-skills-review-decisions-v1:<comparisonHash>`. L'enveloppe complète est
stockée et validée à chaque chargement. L'import applique un JSON Schema strict
et vérifie `reviewId`, `comparisonHash`, hashes des sources/dépendances,
fingerprints, enums et champs inconnus.

Une migration est une opération explicite et rapportée. Elle conserve une
décision seulement si `stableId` et fingerprint restent compatibles; elle
produit les listes `retained`, `stale`, `dropped` et leurs raisons. Aucun état
ancien n'est absorbé silencieusement.

Les exports JSON et Markdown n'accordent aucune autorisation gameplay. Les
dossiers classe/skill contiennent différences, courbes, formules, dépendances,
consommateurs, portabilité, risques, décisions, questions et notes.

## Interfaces figées

### Générateur → oracle

Le générateur lit les sources gouvernées avec `scripts/build-data/tsv.js`, exige
un round-trip byte-exact et produit `Mission/pd2-skills-review.json`. Le contenu
hashé exclut les timestamps variables. L'oracle embarque sources, politiques,
contrat, couverture, nœuds, skills, collisions et documentation.

### Oracle + moteur → HTML

Le générateur UI reçoit l'oracle complet et le runtime de décision pur. Il
produit `Mission/pd2-skills-review.html`, sans CDN, fetch, module ou ressource
réseau. L'interface limite le DOM aux vues actives, utilise la délégation
d'événements et conserve une table accessible derrière chaque SVG.

### Oracle + décisions → preview

`pd2-skills-decisions-preview.mjs` vérifie toujours la baseline courante avant
toute projection. Il n'émet un manifeste applicable que si le lot complet est
prêt. Sinon il produit uniquement des diagnostics atomiques : incomplets,
conflits, dépendances, localisations, consommateurs, preuves natives et collisions.

Le preview peut écrire seulement un artefact explicite hors des racines gameplay;
il refuse `--apply` et tout chemin sous un mod, une source read-only ou un profil
runtime.

## Règles absolues de non-modification gameplay

1. Aucun fichier sous `data-BKVince/`, `data-TCP/`, les mods de référence ou
   `data-vanilla3.2/` n'est écrit.
2. Aucun fichier du snapshot PD2/SP+ n'est écrit.
3. Aucune ligne BKVince existante n'est déplacée, insérée ou modifiée.
4. Les ordinals append-only sont des propositions de preview calculées après le
   dernier ordinal réel au moment de la génération.
5. Aucune sauvegarde, DLL, configuration D2RLoader ou installation de jeu n'est
   lue pour mutation ou déployée.
6. Aucun succès runtime ou gameplay n'est revendiqué par ce chantier.
7. L'autorisation `IMPLEMENTE` reçue couvre uniquement le Workbench, son oracle,
   ses décisions, son preview et ses tests.

## Plan d'implantation, validation et rollback

1. geler les contrats ci-dessus;
2. construire l'oracle déterministe et la documentation map;
3. construire le moteur de décisions et son schéma strict;
4. construire le HTML autonome et ses exports;
5. construire le preview atomique read-only;
6. tester les témoins imposés, la couverture, les hashes, les dépendances, le
   HTML `file://`, la déterminisme et l'absence de chemin gameplay;
7. régénérer les artefacts puis valider le cadastre.

Le rollback consiste à supprimer uniquement les nouveaux fichiers Workbench et
les scripts npm associés. Comme aucune table ou donnée runtime n'est touchée,
aucun rollback gameplay ni sauvegarde n'est requis.

## Livraison du Workbench v1

La génération finale gouvernée porte le `comparisonHash`
`EA43860658BFAFEE97A4E4ABA9962EFFA283E8E88DAAFE2C1AD648D67233957D` et le
contrat gelé
`3A0C347476D16366FE1557446E03BD33705AC7AF14CA6BBA4F172935B675A69C`.

Le durcissement opérationnel conserve exactement ce `comparisonHash`, les
fingerprints, les preuves et les exports logiques. Seul le transport des deux
gros artefacts change : JSON compact pour l'oracle autonome et oracle JSON gzip
déterministe embarqué dans le HTML. Les tailles et hashes avant → après sont :

| Artefact | Taille avant | Taille durcie | SHA-256 durci |
|---|---:|---:|---|
| `Mission/pd2-skills-review.json` | 100 552 008 octets | 48 655 229 octets | `D3F44D871BDD0B685B2BDEB58A87A33C60D33973AADC3FB73B03C6730BAA6D4F` |
| `Mission/pd2-skills-review.html` | 48 790 054 octets | 6 234 332 octets | `D09A17D62133BCBDDF6E1E1F33DA5A82D7E813DA91DF98BD3294ABC89093F8C4` |
| `Mission/pd2-skills-documentation-map.json` | 401 017 octets | 401 017 octets | `37516AB117716D1BF2C4B59B2FAEE82F058A422AFFD3B4D669F92F0098BDA7CF` |

Cette réduction supprime seulement indentation et duplication de transport.
L'oracle complet reste exportable, rehydraté octet-logiquement dans Chromium et
vérifié contre le modèle canonique. Les options de dictionnaire global de
chaînes, de cellules indexées, de fragmentation par classe et d'artefact
technique séparé ont été différées : elles auraient accru la complexité des
fingerprints, des exports ou du chargement `file://` sans gain nécessaire après
la compression locale. Aucune donnée, preuve ou dépendance n'a été supprimée.

La couverture finale est de 429 lignes Vanilla, 451 lignes BKVince et 603
lignes PD2, toutes représentées exactement une fois dans 707 entités
sémantiques. Elle contient 15 nouveaux candidats joueurs, des cibles preview
append-only contiguës `451..465`, 40 skills joueurs propres à BKVince, 110
collisions et zéro mapping ambigu. Les vues joueur comptent Amazon 33,
Sorceress 34, Necromancer 35, Paladin 34, Barbarian 36, Druid 31, Assassin 32
et Warlock 30 skills.

Le HEAD explique explicitement l'écart avec l'audit historique 449/108 : deux
lignes techniques BKVince aux ordinals 449 et 450 ajoutent deux collisions
techniques, pour une baseline courante 451/110.

Validation officielle :

```powershell
npm.cmd run generate:pd2-skills-workbench
npm.cmd run validate:pd2-skills-workbench
npm.cmd run smoke:pd2-skills-workbench
```

Le workflow `.github/workflows/pd2-skills-workbench.yml` applique le même gate
sur Ubuntu avec Node 24 et un vrai Chromium. Pour chaque changement du
Workbench, de son générateur ou d'une source gouvernée, il provisionne le
snapshot PD2/SP+ au commit et au tree épinglés, contrôle les hashes de
`Skills.txt` et `patchstring.tbl`, régénère les artefacts, refuse tout diff
généré, exécute la validation structurée, le smoke `file://` et les seuils de
performance. Un manifeste SHA-256 avant/après de tous les fichiers gameplay
suivis dans les racines gouvernées, complété par l'état Git du snapshot PD2,
fait échouer le job à la moindre écriture. La suite ciblée du preview est
rejouée séparément comme gate read-only.

La suite finale exécute **69 tests structurés, 69 réussis, zéro échec**, puis
un `--check` byte-identical du générateur. Son run local de référence dure
18,1 s au total, dont 13,660 s pour `node --test`, sous Node v24.18.0 sur
Windows x64. Le cadastre est régénéré et valide.

Le smoke réel Playwright 1.62.1 ouvre directement l'URL `file://` sous Chromium
151.0.7922.34. Sur la machine locale Windows x64 de validation, il passe ses
18 checkpoints en 27,617 s, sans erreur JavaScript :

| Mesure Chromium | Résultat | Seuil bloquant |
|---|---:|---:|
| Chargement à froid | 971,47 ms | 20 000 ms |
| Première interaction | 1 044,76 ms | 20 000 ms |
| Recherche globale | 7,12 ms | 3 000 ms |
| Filtre | 951,38 ms | 3 000 ms |
| Changement de classe | Sorceress 593,76 ms; Necromancer 621,06 ms | 3 000 ms |
| Export des décisions | 203,37 ms | 10 000 ms |
| Pic RSS Chromium | 889,37 MiB (932 569 088 octets) | 1 500 MiB |
| Pic heap JavaScript | 68,21 MiB (71 523 764 octets) | 512 MiB |

Le parcours couvre Sorceress, Necromancer, Amplify Damage hybride et CUSTOM,
autosave/rechargement, export/import JSON, export Markdown de classe, prochain
skill incomplet, nouveau skill append-only, collision Warlock et preuve
`MALFORMED_SOURCE` de Fire Ball. Le bandeau du dashboard distingue sans
ambiguïté la baseline courante 451/110/prochain 451 de l'audit historique du
8 août 449/108/prochain 449.

Pour produire un preview documentaire à partir d'un export de décisions :

```powershell
npm.cmd run preview:pd2-skills-decisions -- <decisions.json>
```

Les limites conservatrices restent visibles et bloquantes : callbacks natifs
PD2/D2R non prouvés, couverture des consommateurs encodés par ordinal,
fermeture transitive des dépendances des nouveaux skills et texte de
localisation PD2 binaire. Les formules malformées restent malformées; `delay`,
`localdelay`, `globaldelay` et `perdelay` ne sont jamais assimilés. Le DPS et
les dégâts totaux poison restent symboliques lorsque leur conversion runtime
n'est pas prouvée.

Aucune table gameplay, sauvegarde, configuration D2RLoader, installation de jeu
ou priorité courante n'a été modifiée. Aucun skill n'a été importé, implanté ou
préapprouvé.
