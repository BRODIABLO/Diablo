# Mission courante

Dernière mise à jour : 1er septembre 2026

## Priorité active

[ISC12 — ItemStatCost 12-bit clean-sheet format](isc12-3.3.md)

État : **ISC12 0.2.0 : tous les gates solo qualifiés en portée mod-locale et
globale sur D2R 3.3.93847 / D2RLoader 1.2.0-beta**. Le candidat comportemental
G5–G8 exact mesure 451 072 octets et vaut SHA-256
`6089619DE3B01FD474669096A8AEC8A470559FAD993DCB939AC976709A7D2D52`;
27/27 nouvelles fenêtres natives exactes uniques et le ledger `VALID 228/15`
le soutiennent. Le rebuild courant au libellé public corrigé mesure aussi
451 072 octets et vaut
`AFB4B2D1F779A368C3139BB5AF9EDC59CFD4B83042C88AD2EE7991C9E62DFF00`.
Deux builds Release reproductibles `/W4 /WX`, CTest `5/5` et les cold starts
pile complète mod-local/global sont verts sur ce hash. Gameplay et TCP/IP n'ont
pas été rejoués pour ce changement de description seulement; leurs preuves
exactes restent attachées au candidat comportemental précédent.
La transaction canonique porte désormais 43 sites mutables, 129 mutations et
85 témoins. Les fixtures sérialisées D2S d'IDs 512 et 4094
ont conservé leurs valeurs 12 et 94 pendant deux cycles froids. Le D2I contrôlé
de shared stash a été écrit, relu après arrêt complet et réaffiché avec un arbre
socketé 3/3. G9 a exercé les payloads réels `0x9C` et `0x9D`, l'overflow à zéro
callback, la réentrance fatale après un callback, la backpressure et l'invariant
natif exact `childTemporaryFlags = parentTemporaryFlags | 0x08`. Le binaire
comportemental a passé un cold start global puis le retour mod-local, sans désactiver
aucun composant : 36 plugins chargés, les cinq eezstreet et 17 patches; Stash
Search et Revive Overhaul restent les deux échecs préexistants. Les sauvegardes
et tables de test ont ensuite été restaurées byte-exact et le jeu a été arrêté.
G5 élargit `0x3E`, G6 `0xA8`, G7 les ItemStatCost internes de `0xAA` avec son
estimateur, et G8 `0xAC`; ce dernier conserve 69 octets de marge. Le census
ferme sans patch `0x1D..0x1F`, `0x9E..0xA2` et `0x20`, déjà WORD des deux côtés.

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

La première écriture persistante a produit un fichier jetable de 499 octets,
soit l'enveloppe ISC12 attendue de 96 octets suivie d'un D2S valide de 403
octets. D2RLoader l'a néanmoins refusé au frontend comme `invalid-character`.
Vincent a donc retiré l'enveloppe du contrat runtime : les hooks valident
maintenant les conteneurs D2S/D2I standards sans remplacer le buffer reader, et
le writer délègue ensuite au couple D2RCore afin de préserver `.d2rl`, backups
et capture d'environnement. Le clean-sheet devient une obligation documentée
— nouveau personnage ou future migration externe, backups obligatoires — sans
promesse de blocage physique de toute mauvaise utilisation. Le lease schéma
partagé non bloquant couvre les écritures; la lecture physique structurelle est
désormais indépendante de `SchemaReady`, car le frontend énumère les D2S avant
`DataTablesLoaded`.

Le personnage jetable `Iiscxiirawtest` ferme la première verticale réelle. D2R
interdit les chiffres du nom demandé `ISC12RawTest`; le témoin roman exact est
une Amazon niveau 1. Son D2S natif header-only de 403 octets a révélé que G4
doit accepter exactement `0x193` avant l'existence du marker de stats, tout en
refusant chaque longueur intermédiaire. Après correction, le personnage
apparaît, entre au Camp des Rogues, puis `Save and Exit` produit un D2S standard
de 1 297 octets `3752FD52…222FB` et un sidecar `.d2rl` de 6 261 octets
`3F68DBFC…EB718`. Les écritures ISC12 sont `error=0/0`, le marker `0x6667` est
présent à `0x341` et le personnage recharge ensuite en jeu avec la pile complète
active. Cela prouve la surface capable de 4 095 définitions; la table actuelle
reste à 400 lignes, tandis que les fixtures sérialisées dédiées exercent
désormais les IDs 512 et 4094 sans prétendre qu'ils appartiennent à cette table
gameplay.

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
commits suivent G0 puis G10 puis codec, avec l'ordre interne
G9/G5/G6/G7/G8/G2/G4/G1/G3. Leur
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
passent par un `finally` SEH qui abort/discard. Le writer d'enveloppe a été
exécuté une fois et retiré. Le create/save/reload standard, le preview
header-only, le second round-trip D2S, le D2I contrôlé, les IDs élevés, les
payloads `0x9C`/`0x9D`, les scénarios de pression G9 et les portées
mod-locale/globale sont maintenant qualifiés. G5–G8 et le census fixed-width
sont fermés statiquement et publiés avec succès dans les deux portées.

Le multijoueur ISC12 réel est maintenant qualifié sur deux instances locales
indépendantes. Le matching baseline passe avec l'empreinte commune
`d47b2fa2…a590`; le matching à exactement 4 095 lignes passe avec
`db5b6b25…47c6`, et l'item portant les IDs élevés est déplacé du shared stash au
personnage puis au sol sans déconnexion du peer. Les deux mismatches sont
refusés fail-closed avant création du joueur. Le sens host ISC12/joiner absent
oppose `d47b2fa2…a590` à `17fe0612…c05`. Le sens inverse est isolé dans un
cleanroom où la seule différence est ISC12 — 17 contre 18 plugins partagés —
et oppose `1e602a9f…c872` à `42bbaffc…9c50`; les deux logs nomment
`environment fingerprint mismatch`. Le runtime, les tables, sauvegardes,
ignore-lists et DLL sont restaurés byte-exact, sans processus restant.

ISC12 Save Converter ferme maintenant son gate technique. La suite passe
`38/38`; l'EXE autonome UX avec overlay vanilla et lecture MPQ binaire de
94 635 520 octets vaut `15697D98…59511E`. `HEBKCharm.d2s` se convertit avec le
dossier BKVince partiel; le vrai `yupgoolg132.mpq` livre ses 22 tables connues
directement en mémoire, mais `abc.d2s` refuse fail-closed sur un stat stream non
décrit par ses TXT et ne produit aucune sortie. Un D2S réellement écrit par D2R 9-bit passe
1 284 → 1 297 → 1 284 octets byte-exact (`4A50BC58…0F994`). Le personnage
charge et se sauvegarde deux fois sous BKVince sans ISC12, puis la sortie ISC12
charge et se sauvegarde deux fois sous ISC12Lab avec ISC12 0.2.0, 36 plugins,
les cinq eezstreet et 17 patches. Le D2S ISC12 final reste à 1 297 octets
(`47AE70FB…983BB`). Les deux échecs connus Stash Search/Revive Overhaul et
l'assertion TACT récurrente sont encore visibles, sans empêcher les quatre
cycles observés. Aucun processus de test ne demeure.

Le parcours public détecte maintenant les `.d2i` voisins d'un `.d2s`, affiche
leurs chemins et propose leur inclusion dans le même batch atomique. Un `.d2i`
reste sélectionnable directement et un dossier convertit tous ses D2S/D2I. Le
menu interactif n'expose plus le JSON interne : il distingue seulement vanilla
propre et dossier du mod installé. Les deux builds EXE sont byte-identiques.
Les témoins console ferment `FAILED` sans sortie puis `SUCCESS` sur un lot
D2S+D2I, avec dossier exact, option Explorer et attente avant fermeture.

Le migrateur à deux schémas source → cible est maintenant implanté hors
runtime. Il charge séparément vanilla, TXT loose, dossier-forme MPQ ou archive
MPQ binaire pour la source et la cible, refuse les mods BIN-only, apparie les
stats par nom exact et réencode `SaveBits`, `SaveParamBits`, `SaveAdd`,
`CSvBits` et signedness. Les références de bases, affixes, uniques, sets et
runewords sont validées ou remappées uniquement lorsqu'elles sont univoques.
La suite passe `60/60`; le CLI public Vanilla → BKVince → Vanilla revient
byte-exact. Le nouvel EXE de 94 676 992 octets vaut
`D498D0FB…B392A40`. Son smoke réel BKVince convertit `HEBKCharm.d2s`
1 226 → 1 239 → 1 226 octets et restaure le SHA-256 original
`E50EEB41…87139`.

Le preflight réel BKVince → Yupgoolg de `HEBKCharm.d2s` et
`ModernSharedStashSoftCoreV2.d2i` refuse atomiquement 15 entrées d'items qui
emploient 10 bases absentes de la cible : `mff`, `mfc`, `mfd`, `gay`, `cct`,
`gav`, `mls`, `hsm`, `bct` et `fel`. Les AutoMagic, les identités TXT brutes,
les payloads Gold incidents et la collecte de tous les blockers ont été corrigés
avant ce verdict. Aucun output n'est écrit et le runtime BKVince reste intact.
Aucune qualification runtime cross-schema n'est encore revendiquée.

## Prochain gate

Fermer le gate fonctionnel du migrateur avec un témoin cross-mod compatible :
sélectionner ou créer, sans modifier les originaux refusés, un personnage et un
shared stash dont chaque base existe dans les deux mods — ou choisir un mod cible
qui contient les bases BKVince concernées — puis convertir et confirmer
load/save/reload sous ISC12. En parallèle, le petit ZIP Discord ISC12 seul peut
être distribué aux testeurs qui utilisent de nouveaux personnages et shared
stashes. Après ces preuves, préparer la release GitHub complète et publier le
convertisseur comme asset séparé. Aucun tag ni asset GitHub n'est encore
autorisé.

## Frontière Git active

Le lot couvre `Mission/isc12-3.3.md`, `addons/ISC12/**`,
`addons/ISC12SaveConverter/**`, le patch ciblé `@d2runewizard/d2s`, les preuves
et scripts ISC12, la proposition `sdk-contribution/Services-requests.md` et les
registres partagés strictement nécessaires. Les changements MapSense et les
autres travaux concurrents restent hors propriété ISC12 et doivent être
préservés. Vincent autorise maintenant le commit et le push ciblés du plugin,
du convertisseur et de leurs registres; aucun tag ni asset GitHub n'est inclus.

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
