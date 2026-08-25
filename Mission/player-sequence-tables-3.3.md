# Softcoded Player Sequence Tables — D2R 3.3.93847

Dernière mise à jour : 24 août 2026

## Intention produit

Mettre à disposition de la communauté D2RLoader les tables de séquences des
joueurs sous une forme documentée, calculable et, dans une phase ultérieure,
modifiable par les mods comme le sont les séquences de monstres dans
`monseq.txt`.

La phase 1 autorisée par Vincent établit la baseline native reproductible. Le
24 août 2026, après revue du chargement, des caches de pointeurs, des formats et
de la coexistence avec la Suite, Vincent autorise par `GO` l'incubation de la
solution retenue.

## Décision d'architecture et autorisation — 24 août 2026

La solution devient une DLL autonome `PlayerSequenceTables`, attribuée
exactement à `RuffnecKk` et membre de la RuffnecKk D2RLoader Suite. Elle reste
hybride globale/mod-locale, conserve sa version, ses métadonnées et son TOML
indépendants et ne modifie, lie ni redistribue aucune DLL d'eezstreet.

Le format public retenu est une paire TXT normalisée : une table explicite des
350 routes `(seqnum, weaponclass, recordset)` et une table des recordsets
partagés `(recordset, mode, frame, dir, event)`. Le plugin compile ces sources
au démarrage dans une arène immuable de durée de vie processus, puis remplace
uniquement les 25 pointeurs de groupes à `0x2386658..0x238671F`. Aucun hook de
code n'est ajouté. Le fichier absent préserve strictement vanilla; une source
présente mais invalide est refusée avant toute écriture; le reload à chaud est
hors contrat et exige un redémarrage.

L'audit statique de la baseline Suite 1.2.0 ne trouve aucun chevauchement entre
cette plage et les 191 écritures Suite/patches/tiers ni les 139 écritures des
cinq plugins eezstreet. Cette absence de collision autorise l'implantation mais
ne constitue pas encore une preuve runtime, gameplay ou multijoueur.

## Phase 1 — baseline gouvernée

État : **TERMINÉE techniquement; baseline relue et architecture approuvée par
Vincent le 24 août 2026**.

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

## Phase 2 — implantation autorisée

État : **DLL et format public implantés; gates statiques, reproductibilité,
politiques d'entrée, coexistence Loader et cold starts `24/24` globaux et
mod-locaux PASS; gameplay, reload et multijoueur encore non exécutés**.

Le candidat autonome `PlayerSequenceTables` 0.1.0 produit :

- `playerseqmap.txt`, matrice exhaustive de 350 routes;
- `playerseq.txt`, 44 recordsets dédupliqués et 757 records;
- un parseur strict qui borne `seqnum`, classes d'armes, modes, frames,
  directions, events, tailles et références;
- une arène native immuable regroupant les 350 descripteurs et tous les
  records de durée de vie processus;
- une unique transaction SDK `PatchBytes` de 200 octets sur les pointeurs des
  groupes 1 à 25, sans hook de code;
- un TOML anglais indépendant, un README, une matrice de validation et la
  commande console `player-sequences` avec hash SHA-256 combiné.

Les tables générées restent en CRLF, sans BOM et passent un round-trip
byte-exact. Leurs SHA-256 sont respectivement
`3D00E1BB391E2A19878B164ECAE45CDC2C35239ADC081816C93D3CCFCA3E47F9`
et `BDD70BC115EC8A3E9B207DDFDD1C999B23D41A6561E60DE21FEC0C7B8D245589`.
Deux builds Release propres produisent la même DLL de SHA-256
`66D5C5EF9BA530740082A0C1C6BAFCABC02116E7C65D7A7C1F424AA20E4B2F2B`.

L'audit automatisé recoupe les 191 écritures gouvernées de la Suite et de ses
patches/tiers avec les 139 écritures du PluginPack eezstreet. Aucun des 330
sites ne chevauche `0x2386658..0x238671F`; le candidat n'appelle
`InstallInlineHook` nulle part et ne porte pas `ModScopedOnly`.

## Faits, hypothèses et inconnues après implantation

### Faits vérifiés

Le sélecteur natif utilise `seqnum`, le mode du joueur et la classe d'arme pour
choisir un descripteur. L'initialisation d'une unité met en cache le pointeur de
record dans `Unit+0x40`; l'arène ne doit donc jamais être libérée avant la fin
du processus. Le nombre de records vient du descripteur et les routes nulles
font partie du comportement vanilla. Les groupes 24 et 25 sont préservés. Le
runtime confirme aussi que les 25 valeurs attendues sont déjà relocatées au
chargement : la transaction passe dans les portées globale et mod-locale.

Les quatre politiques d'entrée ont été exercées sans neutraliser le reste de la
pile. L'absence des deux tables charge le plugin en laissant vanilla inchangé;
la paire valide journalise 235 routes, 44 recordsets, 757 records et le hash
combiné `F1C043E1D66E48C86BB4ED4E0A4FF7E8B57F4B68ACA5854340C20D60B1EA4EAA`;
une table manquante ou un mode invalide à la ligne 2 refuse le chargement avant
construction de l'arène ou écriture. Les deux portées conservent 32 plugins et
18 patches, dont les cinq plugins eezstreet. Avec les tables valides, les cold
starts globaux puis mod-locaux atteignent `24/24`, compilent 190 tables TXT et
journalisent `D2R startup complete`.

Deux tentatives antérieures avaient reproduit l'incident graphique intermittent
connu `dxgi.dll + 0x38B1C1`. Il reste consigné, mais n'est plus un blocker actif :
trois relances inchangées franchissent ensuite le rendu, dont la portée globale
et le profil mod-local final, sans nouveau rapport de crash.

### Inconnues bloquantes avant publication

- gameplay représentatif des groupes legacy, 24 et 25 et d'une route nulle;
- modification réversible d'une frame ou d'un event après redémarrage;
- transitions menu/partie et sécurité d'un unload/reload D2RLoader;
- autorité hôte/joiner et égalité obligatoire du hash des tables;
- canal de distribution des deux TXT essentiels, le ZIP de plugin restant
  limité par politique à la DLL et au TOML.

## Prochain gate

Exécuter les témoins gameplay legacy/Cleave/Mirrored Blades, route nulle et edit
réversible après redémarrage. Couvrir ensuite les transitions menu/partie,
unload/reload et hôte/client avec hash identique. Ces preuves et le canal de
distribution des TXT restent obligatoires avant catalogue public ou ZIP.

## Validation et rollback

Les phases documentaires et statiques sont validées par la régénération
déterministe, quatre tests Node, CTest, le build MSVC Release reproductible,
l'audit des 330 écritures, le round-trip TSV byte-exact, les quatre politiques et
les cold starts complets des deux portées. Le profil est restauré mod-local sans
doublon global et aucun processus D2R ne reste actif. Le rollback runtime consiste à
retirer DLL, TOML et les deux TXT puis à redémarrer; aucun payload de sauvegarde
ni migration n'existe.
