# Mission courante

Dernière mise à jour : 31 août 2026

## Priorité active

[ISC12 — ItemStatCost 12-bit clean-sheet format](isc12-3.3.md)

État : **ISC12 0.2.0 : publication native mod-locale attestée sur D2R
3.3.93847 / D2RLoader 1.2.0-beta**. Deux builds Release reproductibles passent
`/W4 /WX` et CTest `5/5`; la DLL byte-identique de 445 952 octets vaut
`EFCA4EBAECDC7E0EF7BE70D2BE741FD7D73DED0ACA85873507CCA2D2B625F3DB`.
L'unicité des signatures et le ledger `VALID 211/15` passent aussi. Le cold
start a publié G0, G10, G9 et G1–G4 avec 24 sites codec et 102 mutations, puis
a atteint `D2R startup complete`. Le provider dynamique D2S, ses relais et son
unwind vivant ont donc franchi le gate qui était auparavant seulement prouvé
statiquement.

Le cycle de schéma G0 suit maintenant deux phases. `LoadExcelTable` capture les
candidats sans publier prématurément; le callback autoritaire
`DataTablesLoaded` choisit ensuite la vue `Bank::Rotw / ItemStatCost` exacte.
Le cold start a observé deux constructions ItemStatCost, 368 puis 400 lignes,
et a publié la seconde à la révision 1 avec `G0-builds=2` et
`SchemaReady=true`. Cela corrige le refus fail-closed causé par la coexistence
des tables Classic et expansion sans introduire de nouveau service D2RLoader.
La finalisation redécode maintenant les octets RotW après le post-processing;
le pointeur seul ne peut plus valider un snapshot périmé. Le token de révision
est traité selon le vrai contrat SDK, et l'unload concurrent passe par un état
atomique `Stopping` bénin.

Les hooks de persistance gardent aussi un lease schéma partagé non bloquant du
snapshot natif jusqu'au remplacement reader ou au commit atomique writer. Le
reload qui gagne le lock provoque donc un rejet sans commit. La provenance du
buffer natif déjà sérialisé avant le hook physique reste toutefois à prouver ou
à lier à une génération pendant le gate save/reload.

La pile mod-locale complète est restée active : 36 plugins chargés, les cinq
plugins eezstreet chargés, 17 memory patches et 190 tables compilées. Les deux
échecs de chargement observés, Stash Search et Revive Overhaul, sont connus et
sans rapport avec ISC12. Aucun `ERROR` ou `CRITICAL` ISC12 n'apparaît dans ce
run.

La vue empruntée `NativePublicationLeaseView` garde G0, G10 et le codec avant
toute écriture et après chaque tentative, sans ownership ni release côté
plugin. Sa seule instance production est liée au même thread et à la durée du
callback initial `D2RLoaderLoadPlugin`. Le coordinateur préflighte les trois
domaines, réserve les relais process-lifetime, commit G0 puis G10 puis codec,
publie la readiness avant le retour et exige toutes les postconditions avant
de rendre le lifecycle actif. Toute incertitude post-write vide la readiness puis arrête
le processus; l'ancien installateur G0-only est exclu du build production.

Le kit local de contribution `NativePublication V1` reste compilable
sur une branche worktree PluginSDK v4 non commitée. Il conserve volontairement
le draft hors de `api.h`, de l'installation et du registre, fixe l'ABI à
32/24/16 octets et fournit un modèle d'autorité plus une matrice Core. Release
`/W4 /WX`, CTest, 100 répétitions et le CTest racine v4 passent; l'install smoke
reste à 36 fichiers sans le header draft. Le modèle couvre notamment la
sérialisation cross-owner, le refus worker-thread, le pin pendant callback et le
poison post-mutation. La relecture finale indépendante ne trouve plus de
bloqueur ABI/conformance; les gates null service/execute et null lease/validate
sont aussi explicites, et le second est testé. Il ne remplace pas le Core privé
et aucun PR n'est encore publié. Il devient un durcissement upstream optionnel,
pas un blocker du spike ISC12.

D2RLoader `1.2.0-beta` est audité et installé à partir du ZIP officiel fourni
par Vincent. Son SHA-256
`2AABEF2E6838CA3611EA3CB74D318C3BB792549CC4FC6C7D53933245667417D9`,
sa version et son certificat concordent; les 21 exports publics Core sont
identiques à 1.1 et les 27 nouveaux exports sont internes. Les binaires installés
concordent byte pour byte avec le ZIP. La release rend D2R 3.3 officiel et
PluginSDK v4 maintient les plugins v2/v3; elle est maintenant la baseline de
qualification ISC12. Elle ne fournit toujours aucun service
`NativePublication`, qui reste un durcissement optionnel.

V4 livre séparément `ItemInteractionServiceV1` et
`ItemServiceV1::executeExistingItemTransaction`, soit les demandes SDK
interaction typée et opérations atomiques sur items existants. L'audit cible
MassID puis AutoSort, sans migration : shared stash et autorité client/hôte
restent hors du contrat V1. Ces ajouts sont orthogonaux à ISC12; le registre de
services fournis par les plugins demeure absent.

Le coordinateur global one-shot et ses adapters réels G0/G10/codec sont liés,
testés et appelés exactement une fois en production depuis le callback initial.
Les trois preflights produisent des plans immuables avant la réservation; les
commits suivent G0 puis G10 puis codec, avec l'ordre interne G9/G2/G4/G1/G3. Leur
succès s'arrête à `CommittedPendingReadiness`, tous les flags privés encore faux;
la même fenêtre startup publie alors la readiness sans autre write et vérifie
G0/G10/codec/transport avant de rendre le plugin opérationnel. Le CTest dédié
couvre les échecs Patch/write/flush, vue révoquée, réentrance,
mutate-then-uncertain et les deux issues startup. Un byte déjà égal reste un
no-op; toute écriture réellement tentée rend ensuite l'ambiguïté terminale.

G9-A est le premier groupe et suit l'ordre fail-closed queue 9C, queue 9D, entry
9C, entry 9D. Douze témoins exacts couvrent les producers, les séquences scindées,
le walker, les consumers, la vraie queue `0x4817F0` et son dispatch. Une
transaction thread-local sans allocation copie les vrais octets natifs dans un
stockage fixe borné à 64 paquets et `0x4000` octets, avec profondeur 16 et au
plus sept enfants immédiats par nœud. Ces limites ISC12 n'affirment aucune borne
native globale. Après le
retour du root seulement, le batch complet est validé avant toute vraie queue;
un rejet abort/discard avec zéro envoi, une acceptation rejoue par `0x4817F0`.
Les entries utilisent des trampolines enregistrés de 10 octets; l'unwind vivant
est enregistré avec `RtlAddFunctionTable` et échoue fermé. Les témoins exacts
`[0x479E23,0x479E41)` et `[0x47A019,0x47A037)` couvrent cookie check,
désallocation, pops et `RET`. Le loader exige au corps et au `RET` des résultats
`RtlLookupFunctionEntry` identiques (Begin/End/UnwindData), avec End dans l'image
et couverture de l'épilogue. Ce check live, les relais et la publication
canonique ont maintenant passé sur le runtime officiel. Les sorties anormales
passent par un `finally` SEH qui abort/discard. Aucun payload fonctionnel D2S,
D2I, preview, save/reload, gameplay ou multijoueur ISC12 n'a encore été
exécuté. G5 `0x3E`, G6 `0xA8` et G7 `0xAA` restent ledger-only, G8 `0xAC`
reste bloqué, et aucun census des éventuels paquets `uint8 statId` mentionnés
par Necrolis n'est encore fermé.

## Prochain gate

Exécuter les cas fonctionnels jetables 0x9C/0x9D, overflow, réentrance,
backpressure, coexistence et save/reload sur le candidat exact
`EFCA4EBA…25F3DB`, puis qualifier la portée globale. Le census réseau G5–G8 et
des éventuels champs `uint8 statId` doit fermer avant une revendication réseau
complète. Le kit NativePublication V1 reste un durcissement loader général
optionnel et ne bloque pas ces gates.

## Frontière Git active

Le lot couvre `Mission/isc12-3.3.md`, `addons/ISC12/**`, les preuves et scripts
ISC12, la proposition `sdk-contribution/Services-requests.md` et les registres
partagés strictement nécessaires. Les changements
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
