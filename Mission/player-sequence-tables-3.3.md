# Softcoded Player Sequence Tables — D2R 3.3.93847

Dernière mise à jour : 24 août 2026

## Intention produit

Mettre à disposition de la communauté D2RLoader les tables de séquences des
joueurs sous une forme documentée, calculable et, dans une phase ultérieure,
modifiable par les mods comme le sont les séquences de monstres dans
`monseq.txt`.

Cette mission ne présume pas encore du format public final ni du mécanisme de
chargement. La phase 1 autorisée par Vincent établit uniquement la baseline
native reproductible. Aucune DLL, aucun patch runtime et aucune table gameplay
de BKVince ne sont ajoutés pendant cette phase.

## Phase 1 — baseline gouvernée

État : **TERMINÉE techniquement; revue du résultat par Vincent ouverte**.

Le runtime D2R 3.3.93847 a été capturé avec le profil BKVince complet, sans
retirer ni désactiver de plugin. L'extracteur déterministe recoupe cette
capture avec l'image d'analyse gouvernée, les tables `skills.txt` et
`plrmode.txt` de la référence vanilla 3.3 ainsi que D2MOO au commit épinglé
`19019806df7f3e877fa105b05395d1e3597e2316` pour les noms sémantiques hérités.
D2MOO ne fournit aucune adresse, structure ni ABI à D2R.

La baseline prouve :

- 25 groupes de séquences actifs, indexés par `SkillsTxt.seqnum` 1 à 25;
- 14 classes d'armes, soit 350 routes possibles;
- 235 routes disponibles et 115 routes explicitement nulles;
- 47 tableaux de records runtime, dont 44 contenus uniques;
- 808 records de six octets;
- un descripteur runtime de 24 octets : pointeur vers records, nombre de frames
  de séquence, nombre de frames d'animation et valeur auxiliaire `0x100`;
- les groupes 24 `Cleave` et 25 `Mirrored Blades`, absents de l'oracle legacy;
- l'ambiguïté statique du groupe 6 `Inferno`, présent deux fois à l'identique
  dans l'image gouvernée, mais routé sans ambiguïté par le tableau runtime.

Les 29 lignes de skills joueur qui utilisent actuellement une séquence sont
également inventoriées. Les tables source sont relues avec l'outillage TSV
gouverné; leur round-trip reste byte-exact, en CRLF et sans BOM.

## Preuves natives promues

- `SKILLS_GetSeqNumFromSkill` à RVA `0x33DBC0`; le chemin joueur lit
  `SkillsTxt+0x33`.
- `DATATBLS_GetSeqRecordFromUnit` à RVA `0x3CB890`.
- table runtime des groupes à RVA `0x2386650`.
- table runtime des 14 classes d'armes à RVA `0x2386730`.
- seed statique unique des classes d'armes à RVA `0x19EAF70`.

Le corpus historique `reverse-engineering/d2r-3.2.92777/` reste la provenance
de l'image native gouvernée. Ses octets utiles ont été vérifiés identiques pour
la cible courante D2R 3.3.93847; les sorties et conclusions nomment donc la
cible courante.

## Livrables de la phase 1

- `reverse-engineering/d2r-3.2.92777/player-sequences/d2r-3.3.93847-player-sequence-map.tsv`
- `reverse-engineering/d2r-3.2.92777/player-sequences/d2r-3.3.93847-player-sequence-records.tsv`
- `reverse-engineering/d2r-3.2.92777/player-sequences/d2r-3.3.93847-player-sequence-runtime.json`
- `reverse-engineering/d2r-3.2.92777/player-sequences/d2r-3.3.93847-player-sequences.manifest.json`
- `scripts/reverse-engineering/player-sequences.mjs`
- `scripts/reverse-engineering/player-sequences.test.mjs`
- `scripts/reverse-engineering/Capture-PlayerSequences.ps1`

La capture mémoire brute reste locale sous `analysis-cache/`; le JSON normalisé
commité contient les valeurs utiles et leur provenance sans dépendre d'un PID
ou d'une adresse absolue de session.

## Faits, hypothèses et inconnues

### Faits vérifiés

Le sélecteur natif utilise `seqnum`, le mode du joueur et la classe d'arme pour
choisir un descripteur. Le nombre de records vient du descripteur et les routes
nulles font partie du comportement vanilla. Les groupes 24 et 25 doivent être
préservés par toute solution courante, même s'ils ne figurent pas dans D2MOO.

### Hypothèses à tester

Une table externe peut probablement représenter les groupes, routes et records
sans perte. Cela ne prouve pas encore que les structures compilées peuvent être
remplacées après le chargement des données, ni que leur durée de vie est unique
et stable pendant toutes les transitions de partie.

### Inconnues bloquantes avant implantation

- propriétaire et moment exact de création/destruction des tables runtime;
- mécanisme sûr pour allouer des groupes ou records de longueur variable;
- liste exhaustive des consommateurs et caches de pointeurs;
- comportement des reloads, changements de mod et retours au menu;
- autorité et compatibilité hôte/joiner lorsqu'un mod change les séquences;
- politique de validation pour modes, frames, directions, events et routes
  absentes;
- format public final et stratégie de repli vanilla fail-closed.

## Prochain gate

Faire relire la baseline par Vincent, puis prouver le contrat complet de
chargement, de propriété et de durée de vie des groupes/descripteurs/records.
Comparer ensuite un compilateur TXT dédié, un format déclaratif distinct et une
surcouche de remplacement minimale. La solution retenue doit accepter des
longueurs variables, préserver strictement le vanilla en l'absence de fichier,
refuser proprement les données invalides et ne revendiquer aucun hook déjà
possédé par la pile active.

Ce gate reste une revue d'architecture et de reverse engineering. Si une DLL
est retenue, l'implantation commencera seulement ensuite avec le skill
`d2rloader-plugin-incubation`, comme composant autonome de la RuffnecKk
D2RLoader Suite, hybride globale/mod-locale et configuré indépendamment.

## Validation et rollback

La phase 1 est validée par la régénération déterministe des quatre artefacts,
le test Node dédié, les hashes du manifeste, le round-trip TSV byte-exact et la
validation du workbench. Le rollback consiste à retirer ce lot documentaire et
ses commandes; aucun binaire runtime, fichier de sauvegarde ou comportement de
jeu n'a été modifié.
