# Mission courante

Dernière mise à jour : 30 août 2026

## Priorité active

[ISC12 — ItemStatCost 12-bit clean-sheet format](isc12-3.3.md)

État : **ISC12 0.2.0 : loader G0 implanté et validé statiquement, désactivé par
défaut; runtime NOT RUN**. Le ledger gouverné est `VALID` avec 63 sites répartis
dans 14 groupes. Le jalon remplace la tail `DescFunc` fixe par un helper borné à
4 095 entrées derrière un relais RX persistant et un état RW séparé, puis tente
le cap `0xFFF` seulement après publication de la tail sûre et d'un guard
conservateur. Un retour incertain du patch tail réserve maintenant les pages
RX/RW avant l'appel, journalise les huit octets observés et termine fail-closed
sans jamais libérer une cible possible. Deux builds Release `/W4 /WX` sont
byte-identiques à 179 200 octets et SHA-256 `C2B461CF…0D1A4D93`; CTest `1/1`,
PE x64, version 0.2.0, auteur,
manifeste API v3 et trois exports passent. Les codecs item, sauvegarde et réseau
restent 9 bits; aucune sauvegarde réelle ni aucun runtime n'a été touché.

## Prochain gate

Fermer G10-A en prouvant les quatre seams file-level : D2S lecture externe,
D2S écriture finale/checksum, D2I lecture de tout le fichier avant le premier
item et D2I écriture finale, avec nettoyage et commit atomique. Figer ensuite
l'enveloppe, le fingerprint de schéma et les golden vectors. G1 reste non publié
jusqu'à la fermeture de G10 et G9. La pose non alignée du saut G0 exige encore
une preuve de quiescence ou de transaction avant activation. Aucun lancement
D2R ni aucune sauvegarde réelle ne sont autorisés.

## Frontière Git active

Le lot couvre `Mission/isc12-3.3.md`, `addons/ISC12/**`, les preuves et scripts
ISC12 et les registres partagés strictement nécessaires. Les changements
MapSense existants restent hors propriété ISC12 et doivent être préservés. Le
GO autorise l'implantation et ses validations; il n'autorise aucun commit, push,
tag, asset GitHub ni outil externe de migration.

## Priorité précédente conservée — Softcoded Player Sequence Tables

[Softcoded Player Sequence Tables — D2R 3.3.93847](player-sequence-tables-3.3.md)

État : **le candidat autonome 0.1.0 est implanté et reproductible; politiques
d'entrée, coexistence Loader et cold starts globaux/mod-locaux `24/24` qualifiés;
gameplay encore non exécuté**. Deux TXT normalisés exposent 350 routes et 44 recordsets/757 records.
Tests Node, CTest, intégrité TSV, audit de 330 écritures et deux builds Release
passent; la DLL vaut SHA-256
`66D5C5EF9BA530740082A0C1C6BAFCABC02116E7C65D7A7C1F424AA20E4B2F2B`.
Les cas absent/valide/incomplet/invalide passent avant écriture, les portées
globale et mod-locale atteignent `24/24` avec la pile complète à 32 plugins/18
patches et 190 tables compilées. Le profil final est restauré mod-local sans
doublon global et aucun processus D2R ne reste actif.

### Gate conservé — Softcoded Player Sequence Tables

Exécuter gameplay legacy/Cleave/Mirrored Blades/route nulle et un edit réversible
après redémarrage, puis transitions menu/partie, unload/reload et hôte/client
avec hash identique. Le canal de distribution des deux TXT doit aussi être fixé
avant catalogue public ou ZIP.

### Frontière Git conservée — Softcoded Player Sequence Tables

La mission active couvre `Mission/player-sequence-tables-3.3.md`,
`addons/PlayerSequenceTables/**`, les scripts `player-sequence*` et
`Capture-PlayerSequences.ps1`, ainsi que les preuves sous
`reverse-engineering/d2r-3.2.92777/player-sequences/`. La ROADMAP, le pointeur
courant, le workstream, le cadastre, `package.json`, l'outil de build natif,
`.gitattributes`, `known-rvas.json` et `findings.md` sont partagés. Les captures
et builds restent locaux et ignorés. Aucun commit ni push n'est effectué sans
demande explicite de Vincent.

## Priorité précédente conservée — BKVCombat

[BKVCombat 0.1 — Release 1 Damage Core](bkvcombat-0.1.md)

État : **build/package et coexistence current-stack terminés; gameplay NOT RUN**.
Vincent a confirmé une DLL RuffnecKk autonome permanente, hybride
global/mod-local et compatible par contrat avec PluginPack et les autres plugins
du workspace. La configuration reste `default-off` et chaque politique peut être
activée indépendamment.

La DLL candidate de 215 552 octets vaut SHA-256
`3EFCEB7374E26207FE603FF5AC43DAFBC8246E85C37426B62D0AEF1F38663D50`; le build
Release x64 et CTest `1/1` passent. La base courante descend du Monster Merge.
Crushing Blow applique la priorité `MajorBoss > PrimeEvil > Elite > Ordinary`;
le marqueur Herald/Ascendant appartient à `Elite` et le profil BKVince réserve
la CBE à la stat `393`. Open Wounds supporte trois stacks, puis les rafraîchit
tous au cap dans le contrat offline/local mono-source. Life/Mana Steal conserve
et valide le baseline natif sans hook général.

Le ZIP public strict vaut SHA-256
`A6E89B7B4B8723704A44F95386AB841A6ABD4AD9C2C27003EE61A4B90331BE24`.
Les cold starts default-off, policies-on et ordre inverse exact atteignent
`19/19` plugins, `15/15` patchsets et `24/24`; le profil final est mod-local,
default-off et aucun processus D2R ne reste actif. La matrice universelle reste
NO-GO parce que plusieurs fonctionnalités PluginPack sont désactivées dans la
baseline et que les incidents render préexistants `dxgi/plugin-items` et
`PopcornUber` ne sont pas fermés.

L’update officielle D2R `3.3.93847` a ouvert un gate environnemental temporaire :
[BKVince — alignement data D2R 3.3.93847](bkvince-d2r33-data-alignment.md).
Le port TXT ciblé, les hashes runtime et le cold start pile complète sont
maintenant validés : `27` plugins, `17` patches, `24/24`, aucune récidive de
`Items.cpp:1990`, puis confirmation de Vincent que le jeu fonctionne. Ce gate
ne suspend donc plus la reprise de BKVCombat. La mission data reste ouverte
pour les observations Council/Herald/Shard/Sunder et ne qualifie pas encore les
DLL ou memory patches 92777 comme compatibles « 3.2+ ».

## Prochain gate

Reprendre la matrice solo BKVCombat : Critical/Deadly, les quatre classes CB,
ranged/player-count/CBE, les trois stacks OW, Life/Mana Steal et Life Tap. Le
premier hit doit aussi prouver la négociation lazy MeleeSplash→BKVCombat et
l’absence de double CB/OW. Reprendre la matrice universelle seulement après
fermeture des fonctions PluginPack désactivées et des incidents render, sans
neutraliser de composant tiers. En parallèle, observer en jeu les deltas data
3.3 Council/Herald/Shard/Sunder; `QtyTester.d2s/.d2rl` ayant été réécrits lors
du démarrage réussi, ne pas revendiquer un contrôle 149/149 byte-exact. Aucun
succès gameplay BKVCombat n’est encore revendiqué.

## Frontière Git active

La mission active couvre `addons/BKVCombat/**`, son profil BKVince, les données
CBE, les preuves natives, la mission, la ROADMAP, le workstream et le cadastre.
Les autres changements du workspace restent hors périmètre et sont préservés.
Aucun commit ni push n’est effectué sans demande explicite de Vincent.

## Priorité précédente conservée — ProgressiveAffixesPlugin

[ProgressiveAffixesPlugin autonome — D2R 3.2.92777](progressive-affixes-plugin-3.2.md)

État : **hotfix v0.2.1 déployé; création neuve sans crash; qualification statistique ouverte**.
Vincent avait placé le plugin en option B dans la phase itemisation, puis a
donné le 10 août 2026 la directive explicite `Implemente`. Cette directive ouvre
le lot immédiatement sans modifier la destination confirmée : plugin autonome
permanent RuffnecKk, hybride global/mod-local, TOML indépendant et aucune
mutation d’une DLL eezstreet.

La DLL Release x64 et le TOML joueur PD2-inspired sont présents dans `addons/` et dans le profil
source BKVince. Les trois anciens patchsets force Magic/Rare/Crafted sont retirés
atomiquement. Deux clean builds `/Brepro` du hotfix produisent la même DLL de
161 280 octets, SHA-256
`6F70AB9EAA6238DAB8CE685B7C4C2E3E6A961326F71A82C566B86CFBD684DD0D`.
La suite CTest passe `1/1`; les métadonnées, trois exports D2RLoader et hashes
build/package/BKVince sont cohérents.

Le TOML expose maintenant quatre sections simples et des pourcentages lisibles.
Les Rare Jewels suivent `50/50`, `37.5/62.5`, `25/75`, puis `0/100` pour trois
ou quatre affixes aux ilvl 1/45/65/85. Le format avancé v0.1.0 reste accepté
sans pouvoir être mélangé au format joueur.

Les trois patchsets runtime obsolètes ont été archivés hors des dossiers actifs.
Deux cold starts frais passent sur le build 92777 : la portée mod-locale puis la
portée globale avec son propre repli de configuration chargent chacune
ProgressiveAffixesPlugin 0.2.0. Chaque démarrage conserve la pile de production
et termine avec `18/18` plugins actifs, `15/15` patchsets appliqués, zéro rejet
et zéro échec. La portée globale de test a été retirée et le profil BKVince
mod-local restauré avec les hashes source/runtime exacts; une seule instance
mod-locale stable a été relancée.

Le 11 août, le rapport de crash de création Necromancer a révélé que le relais
prefix `0x442C78` de la v0.2.0 écrasait `RCX/RDX` avant leur sauvegarde par le
prologue natif. La v0.2.1 appelle le helper avec shadow space et alignement,
puis restaure les deux arguments. Le cold start complet passe avec `19/19`
plugins, `15/15` patchsets et `24/24`; aucun nouveau rapport de crash n’est
apparu. Vincent a créé un nouveau personnage et confirmé l’entrée en jeu sans
crash. L’archive publique stricte v0.2.1 vaut SHA-256
`F95D4BBDA94D296B3632E9F44B87B2B9137B4177079191D9CEC0B3D77DE6A82E`.

### Gates encore ouverts de ProgressiveAffixesPlugin

La régression de création est fermée. Produire encore les témoins gameplay qui
couvrent explicitement chacun des codes `weap`, `armo`, `jewl`, `ring`, `amul`,
`char`, puis vérifier les bornes et les distributions réelles.

La qualification complète exige ensuite :

- toutes les fonctionnalités du PluginPack actives et aucun composant
  neutralisé ;
- deux ordres pertinents pour toute surface composable ;
- bornes Magic 64/65, jewelry 84/85 et charms 89/90 ;
- distributions Rare aux ilvl 1/45/65/85, y compris la progression 3/4 des Rare Jewels ;
- distributions Crafted aux ilvl 1/31/51/71 ;
- sauvegarde/rechargement et hôte/joiner.

Les conflits full-stack préexistants `0x589736`, `0x314110` et
`0x18885B/0x18887F` restent étrangers au plugin et bloquent toute affirmation de
compatibilité universelle tant qu’ils ne sont pas fermés sans désactiver de
fonctionnalité.

### Frontière Git historique de ProgressiveAffixesPlugin

La mission couvre `addons/ProgressiveAffixesPlugin/**`, son profil BKVince, la
mission, les preuves RE, la ROADMAP, le cadastre et l’archive publique stricte.
Les autres changements du workspace restent hors périmètre et sont préservés.
Aucun commit ni push n’est effectué sans demande explicite de Vincent.
